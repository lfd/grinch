#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Build- and boot-test grinch across the arch x opt x feature matrix.

For every variant we build out-of-tree under ``$outdir/<variant>/build``,
then spawn a fresh QEMU per registered test and drive its serial line
over a TCP socket. Per-test transcripts land in ``$outdir/<variant>/log/``.
A pass/fail table is printed on exit.
"""

import argparse
import fnmatch
import os
import re
import select
import shutil
import signal
import socket
import subprocess
import sys
import time
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path

SRCTREE = Path(__file__).resolve().parent.parent


# ---------------------------------------------------------------------------
# Matrix
# ---------------------------------------------------------------------------

ARCHES = ('riscv64', 'riscv32', 'arm64')
OPTS   = ('-O0', '-O1', '-O2', '-Os', '-O3')
CPUS   = (1, 2, 4)

# Feature -> extra make flags that switch it on. 'plain' is the
# baseline without optional features.
FEATURE_FLAGS = {
    'plain':     ('CONFIG_VMM=0',),
    'vmm':       ('CONFIG_VMM=1',),
    'gcov':      ('CONFIG_GCOV=1',),
    'debug':     ('CONFIG_DEBUG_OUTPUT=1',),
    'initconst': ('CONFIG_INITCONST_STR=1',),
}

# Variant names to skip entirely.
EXCLUDE = ()


@dataclass(frozen=True)
class Variant:
    arch: str
    opt: str
    feature: str

    @property
    def name(self):
        return f'{self.arch}-{self.opt[1:]}-{self.feature}'

    @property
    def make_flags(self):
        return (f'ARCH={self.arch}', f'OPT={self.opt}',
                *FEATURE_FLAGS[self.feature])


def all_variants():
    return [Variant(a, o, f)
            for a in ARCHES for o in OPTS for f in FEATURE_FLAGS
            if Variant(a, o, f).name not in EXCLUDE]


# ---------------------------------------------------------------------------
# QEMU driver
# ---------------------------------------------------------------------------

# Local ports for the guest's serial line and HMP monitor. Tests run
# sequentially, so hardcoding is fine.
SERIAL_PORT  = 22222
MONITOR_PORT = 11111  # matches QEMU_ARGS_COMMON in scripts/kernel.mk

DEFAULT_TIMEOUT = 30  # per expect() / wait_exit() call, seconds


class TestFail(Exception):
    """A test hit an unexpected condition and cannot continue."""


class Timeout(TestFail):
    """A test exceeded its time budget."""


class Qemu:
    """A running QEMU driven over TCP sockets. Use as a context manager.

    On entry we launch ``make qemu`` with the serial line remapped to a
    TCP *server* socket (no ``nowait``) so QEMU blocks until we connect
    -- that way we never miss the very first bytes of output. We then
    connect to that socket plus the HMP monitor.

    On exit we send HMP ``quit`` and, if QEMU refuses to wind down
    within a few seconds, SIGKILL the whole process group. Cleanup is
    unconditional; QEMU never escapes into the background.
    """

    def __init__(self, build_dir, *, cpus=1, log_path=None, tee=False):
        self.build_dir = build_dir
        self.cpus = cpus
        self.log_path = log_path
        self.tee = tee
        self.proc = None
        self.serial = None
        self.monitor = None
        self._log = None
        self._buf = b''

    # -- context manager -------------------------------------------------

    def __enter__(self):
        if self.log_path is not None:
            self._log = open(self.log_path, 'wb')
        # start_new_session so we can SIGKILL the whole tree if make or
        # QEMU refuses to die politely.
        self.proc = subprocess.Popen(
            ['make', '-C', str(self.build_dir), 'qemu',
             'QEMU_DISPLAY=none', f'QEMU_CPUS={self.cpus}',
             f'QEMU_SERIAL=tcp:127.0.0.1:{SERIAL_PORT},server'],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            start_new_session=True)
        try:
            self.serial  = self._connect(SERIAL_PORT)
            self.monitor = self._connect(MONITOR_PORT)
        except BaseException:
            self._kill()
            raise
        return self

    def __exit__(self, *_):
        # HMP 'quit' is fire-and-forget: an OSError here just means the
        # monitor is already gone (typical after a halt test).
        try:
            self.monitor.sendall(b'quit\n')
        except (OSError, AttributeError):
            pass
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self._kill()
        for s in (self.serial, self.monitor):
            if s is not None:
                try: s.close()
                except OSError: pass
        if self._log is not None:
            self._log.close()

    # -- test-facing API -------------------------------------------------

    def send(self, line):
        """Send *line* to the guest shell. gsh treats ``\\r`` as
        end-of-line and ignores ``\\n``, so that's what we send."""
        self.serial.sendall(line.encode() + b'\r')

    def expect(self, pattern, timeout=None):
        """Read serial until *pattern* matches, discarding everything
        up to and including the match. Raise TestFail on timeout or
        if the guest closes the socket."""
        if isinstance(pattern, str):
            pattern = pattern.encode()
        needle = re.compile(pattern)
        deadline = time.monotonic() + (timeout or DEFAULT_TIMEOUT)
        while True:
            m = needle.search(self._buf)
            if m is not None:
                self._buf = self._buf[m.end():]
                return m
            self._buf += self._read(deadline, waiting_for=repr(pattern))

    def wait_exit(self, timeout=None):
        """Block until QEMU exits. Drains any lingering serial output
        into the log so we don't lose the final bytes."""
        deadline = time.monotonic() + (timeout or DEFAULT_TIMEOUT)
        while self.proc.poll() is None:
            if time.monotonic() >= deadline:
                raise Timeout('timeout waiting for QEMU to exit')
            wait = min(0.1, max(0.0, deadline - time.monotonic()))
            ready, _, _ = select.select([self.serial], [], [], wait)
            if not ready:
                continue
            try:
                data = self.serial.recv(4096)
            except OSError:
                continue
            if data:
                self._absorb(data)

    # -- private ---------------------------------------------------------

    def _connect(self, port, timeout=10):
        """Poll-connect to *port* until QEMU opens it, or bail out
        early if make/QEMU exits first."""
        deadline = time.monotonic() + timeout
        last_err = None
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise TestFail(
                    f'make/QEMU exited (rc={self.proc.returncode}) '
                    f'before opening port {port}')
            try:
                return socket.create_connection(('127.0.0.1', port),
                                                timeout=1)
            except (ConnectionRefusedError, OSError) as e:
                last_err = e
                time.sleep(0.1)
        raise Timeout(f'timeout connecting to port {port}: {last_err}')

    def _read(self, deadline, *, waiting_for):
        rem = deadline - time.monotonic()
        if not (rem > 0 and select.select([self.serial], [], [], rem)[0]):
            raise Timeout(f'timeout waiting for {waiting_for}')
        data = self.serial.recv(4096)
        if not data:
            raise TestFail(f'serial closed waiting for {waiting_for}')
        self._absorb(data)
        return data

    def _absorb(self, data):
        if self._log is not None:
            self._log.write(data)
            self._log.flush()
        if self.tee:
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()

    def _kill(self):
        try:
            os.killpg(self.proc.pid, signal.SIGKILL)
        except (ProcessLookupError, PermissionError):
            pass
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            pass


# ---------------------------------------------------------------------------
# Test registry
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class Test:
    name: str
    fn: object          # callable(Qemu) -> None
    requires: dict      # optional {'arch','opt','feature'} filter


TESTS = []


def test(name, **requires):
    """Register the decorated function as a test.

    Optional keyword filters restrict the test to matching variants,
    e.g. ``@test('smp', arch='riscv64')`` skips it on riscv32.
    """
    def register(fn):
        TESTS.append(Test(name, fn, requires))
        return fn
    return register


def test_applies_to(t, v):
    for key in ('arch', 'opt', 'feature'):
        if key in t.requires and getattr(v, key) != t.requires[key]:
            return False
    return True


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
# gsh's prompt looks like ``gsh />`` or ``gsh /tmp>`` depending on cwd;
# the trailing space is common to all forms.
PROMPT = rb'gsh '


def expect_exit_ok(q):
    """gsh prints ``Exit code N`` after every foreground command.
    Insist that N == 0 and match the next prompt."""
    m = q.expect(rb'Exit code (\d+)')
    code = int(m.group(1))
    if code != 0:
        raise TestFail(f'command exited with code {code}')
    q.expect(PROMPT)


@test('boot')
def _boot(q):
    """Kernel reaches the shell prompt."""
    q.expect(PROMPT)


@test('halt')
def _halt(q):
    """``reboot -h`` halts cleanly and QEMU exits."""
    q.expect(PROMPT)
    q.send('reboot -h')
    q.expect(rb'Halting')
    q.wait_exit()


@test('reboot')
def _reboot(q):
    """``reboot -r`` cold-resets the machine; the kernel comes back."""
    q.expect(PROMPT)
    q.send('reboot -r')
    q.expect(rb'Rebooting')
    q.expect(PROMPT)


@test('echo')
def _echo(q):
    """Basic shell + user stdout: echo round-trips its argument."""
    q.expect(PROMPT)
    q.send('echo grinch-echo-marker')
    q.expect(rb'grinch-echo-marker')
    expect_exit_ok(q)


@test('cat')
def _cat(q):
    """Read ``/initrd/test.txt`` (ships as res/test.txt in the cpio)."""
    q.expect(PROMPT)
    q.send('cat /initrd/test.txt')
    q.expect(rb'HELLO, WORLD!')
    expect_exit_ok(q)


@test('ls')
def _ls(q):
    """Directory listing of the initrd mount."""
    q.expect(PROMPT)
    q.send('ls /initrd')
    q.expect(rb'bin')
    q.expect(rb'test\.txt')
    expect_exit_ok(q)


@test('test-app')
def _test_app(q):
    """``/bin/test`` runs the in-tree self-test suite. Walk each phase
    explicitly so a regression that skips or reorders one is caught,
    and require the process to exit cleanly."""
    q.expect(PROMPT)
    q.send('test')
    q.expect(rb'Testing Syscalls')
    q.expect(rb'Testing fork\+wait')
    q.expect(rb'Testing VFS API')
    q.expect(rb' -> devfs')
    q.expect(rb' -> initrd')
    q.expect(rb'Testing tmpfs')
    expect_exit_ok(q)


@test('schedtest')
def _schedtest(q):
    """``schedtest`` stress-tests the scheduler with concurrent
    fork/yield/nanosleep children. It stays silent on success and only
    prints on failure, so a clean ``Exit code 0`` is the pass signal.
    The harness reruns it under smp1/2/4, exercising the SMP task
    migration race it was written to catch."""
    q.expect(PROMPT)
    q.send('schedtest')
    expect_exit_ok(q)


# TODO: Enable once jittertest can be made to return (see project TODO —
# pass argv to init= so a finite run count can be configured).
# @test('vm', arch='riscv64', feature='vmm')
# def _vm(q):
#     q.expect(PROMPT)
#     q.send('vm')
#     q.expect(rb'Grinch VM:')
#     q.expect(rb'Welcome to Grinch')
#     q.expect(rb'Starting Jittertest')
#     expect_exit_ok(q)


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

# Backstop for tests that hang past expect() / wait_exit()'s own
# timeouts (e.g. a tight loop in a test body).
HARD_TIMEOUT = 60


@dataclass
class Result:
    passed:    int = 0
    failed:    int = 0
    timed_out: int = 0
    skipped:   int = 0


@dataclass
class VariantResult:
    build_ok: bool = None   # None = build not attempted (--run only)
    smp: dict = None        # cpus -> Result; populated after a successful build

    def __post_init__(self):
        if self.smp is None:
            self.smp = {}


@dataclass
class TestError:
    message: str
    timeout: bool = False


@contextmanager
def alarm(seconds):
    """SIGALRM after *seconds*; the handler raises TestFail."""
    def fire(_signum, _frame):
        raise Timeout(f'hard timeout after {seconds}s')
    prev = signal.signal(signal.SIGALRM, fire)
    signal.alarm(seconds)
    try:
        yield
    finally:
        signal.alarm(0)
        signal.signal(signal.SIGALRM, prev)


def build(v, build_dir, log_dir, jobs, verbose):
    log_dir.mkdir(parents=True, exist_ok=True)
    cmd = ['make', '-C', str(SRCTREE), f'O={build_dir}',
           *v.make_flags, f'-j{jobs}']
    if verbose >= 2:
        cmd.append('V=1')
    if verbose >= 1:
        return subprocess.run(cmd).returncode == 0
    with open(log_dir / 'build.log', 'w') as log:
        return subprocess.run(cmd, stdout=log, stderr=log).returncode == 0


def run_one_test(t, build_dir, log_path, verbose, cpus):
    """Run one test. Return None on success, TestError on failure."""
    try:
        with alarm(HARD_TIMEOUT), \
             Qemu(build_dir, cpus=cpus, log_path=log_path, tee=verbose >= 3) as q:
            t.fn(q)
        return None
    except Timeout as e:
        return TestError(str(e), timeout=True)
    except TestFail as e:
        return TestError(str(e))
    except Exception as e:
        return TestError(f'{type(e).__name__}: {e}')


def dump_log(log_path):
    if log_path.exists():
        sys.stdout.buffer.write(log_path.read_bytes())
        sys.stdout.buffer.flush()


def run_tests(v, build_dir, log_dir, *, cpus, stop, verbose, tests=None):
    r = Result()
    applicable = [t for t in TESTS if test_applies_to(t, v)]
    if tests:
        applicable = [t for t in applicable if t.name in tests]
    r.skipped = len(TESTS) - len(applicable)
    for t in applicable:
        log_path = log_dir / f'{t.name}-smp{cpus}.log'
        print(f'    {t.name:<26} ...', end='', flush=True)
        err = run_one_test(t, build_dir, log_path, verbose, cpus)
        if err is None:
            print(' ok', flush=True)
            r.passed += 1
        else:
            print(' TIMEOUT' if err.timeout else ' FAIL', flush=True)
            if err.timeout:
                r.timed_out += 1
            else:
                r.failed += 1
            if verbose >= 1:
                print(f'      {err.message}', flush=True)
            if stop:
                dump_log(log_path)
                if err.timeout:
                    sys.stdout.write('\n[TIMEOUT]\n')
                    sys.stdout.flush()
                return r
    return r


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    ap = argparse.ArgumentParser(
        description='Build and run the grinch test matrix.')
    ap.add_argument('--build', action='store_true',
                    help='only build, skip test execution')
    ap.add_argument('--run', action='store_true',
                    help='only run tests, skip build')
    ap.add_argument('-c', '--clean', action='store_true',
                    help='remove the output directory before building')
    ap.add_argument('-s', '--stop', action='store_true',
                    help='stop after the first failing build or test')
    ap.add_argument('-v', '--verbose', action='count', default=0,
                    help='-v: build output + failure messages; '
                         '-vv: also V=1 to make; '
                         '-vvv: also stream test serial output')
    ap.add_argument('-j', '--jobs', type=int, default=os.cpu_count(),
                    metavar='N',
                    help=f'make parallelism (default: {os.cpu_count()})')
    ap.add_argument('-o', '--outdir', default='build/test',
                    metavar='DIR',
                    help='output directory (default: build/test)')
    ap.add_argument('--smp', type=int, action='append', metavar='N',
                    help='only run with this CPU count (repeatable)')
    ap.add_argument('--test', action='append', metavar='NAME',
                    help='only run this test name (repeatable)')
    ap.add_argument('filters', nargs='*', metavar='FILTER',
                    help='variant name globs (default: all)')
    return ap.parse_args()


def select_variants(patterns):
    vs = all_variants()
    if not patterns:
        return vs
    return [v for v in vs
            if any(fnmatch.fnmatch(v.name, p) for p in patterns)]


def announce_build_start(v, verbose):
    if verbose:
        print(f'--- {v.name} ---', flush=True)
    else:
        print(f'{v.name:<32}  build ...', end='', flush=True)


def announce_build_result(v, ok, verbose):
    if verbose:
        print(f'--- {v.name}: {"ok" if ok else "FAIL"} ---')
    else:
        print(' ok' if ok else ' FAIL')


def print_summary(results):
    print()
    print(f'{"variant":<40}  {"build":>5}  {"pass":>4}  {"fail":>4}  {"tout":>4}  {"skip":>4}')
    print('-' * 73)
    for name, vr in results.items():
        if vr.build_ok is None:
            build_str = '-'
        else:
            build_str = 'ok' if vr.build_ok else 'FAIL'
        print(f'{name:<40}  {build_str:>5}')
        for cpus, r in sorted(vr.smp.items()):
            label = f'  smp{cpus}'
            print(f'{label:<40}  {"":>5}  {r.passed:>4}  {r.failed:>4}  {r.timed_out:>4}  {r.skipped:>4}')
    print()


def main():
    args = parse_args()
    outdir = Path(args.outdir)
    want_build = not args.run
    want_run   = not args.build

    if args.clean and outdir.exists():
        shutil.rmtree(outdir)

    variants = select_variants(args.filters)
    results = {}
    overall_ok = True

    for v in variants:
        build_dir = outdir / v.name / 'build'
        log_dir   = outdir / v.name / 'log'
        log_dir.mkdir(parents=True, exist_ok=True)
        (outdir / v.name / 'variant.env').write_text(
            f'ARCH={v.arch}\nOPT={v.opt}\nFEATURE={v.feature}\n')

        vr = VariantResult()
        results[v.name] = vr

        if want_build:
            announce_build_start(v, args.verbose)
            ok = build(v, build_dir, log_dir, args.jobs, args.verbose)
            announce_build_result(v, ok, args.verbose)
            vr.build_ok = ok
            if not ok:
                overall_ok = False
                if args.stop:
                    break
                continue

        if want_run:
            print(v.name, flush=True)
            stopped = False
            cpus_to_run = args.smp if args.smp else CPUS
            for c in cpus_to_run:
                print(f'  smp{c}', flush=True)
                r = run_tests(v, build_dir, log_dir,
                              cpus=c, stop=args.stop, verbose=args.verbose,
                              tests=args.test)
                vr.smp[c] = r
                if r.failed or r.timed_out:
                    overall_ok = False
                    if args.stop:
                        stopped = True
                        break
            if stopped:
                break

    print_summary(results)
    sys.exit(0 if overall_ok else 1)


if __name__ == '__main__':
    main()
