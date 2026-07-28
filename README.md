Grinch
======

Introduction
------------

Grinch is a minimalist operating system, developed mainly for educational and
testing purposes. It targets RISC-V as its primary architecture and also ships
an ARM64 (AArch64) port. Grinch runs userspace processes in ELF format and, on
RISC-V, can recursively boot itself as a virtual machine through its built-in
Virtual Machine Monitor.

Userland applications get basic POSIX support through grinch's own minimalist
libc; the syscall interface stays as close to Linux as reasonably possible.

Anyway, don't expect anything to work.

<img src="res/logo.png" width="35%"/>

Features
--------

#### General
- Device-tree based hardware discovery
- Simple scheduler
- ELF userland applications with a minimalist, Linux-compatible libc
- Interactive shell (`gsh`) and a small set of userland utilities
- Device-tree probed driver model
- Virtual filesystem layer (tmpfs, devfs, initrd)
- PCIe host bridge (generic ECAM)
- Framebuffer output
- Recursive Virtual Machine Monitor (RISC-V)
- GCOV coverage support

#### RISC-V
- RV64 and RV32
- SV32, SV39 and SV48 paging
  - SV39x and SV48x two-stage paging for guests
- SMP (RV64 only)
- RISC-V platform timer
- PLIC and APLIC interrupt controllers
- SBI console and SBI-based platform services
- Hypervisor (H) extension with a self-virtualising VMM

#### ARM64 (AArch64)
- EL1 kernel
- 4-level MMU (L0-L3) with 1 GB and 2 MB block mappings
- ASID-tagged address spaces (TTBR0/TTBR1)
- SMP via device-tree CPU discovery and PSCI (SMC/HVC) bring-up
- ARM generic timer
- GIC-v2 interrupt controller
- ARM semihosting early console

Drivers
-------
- Serial and console
  - 8250/16550 UART
  - AXI UART Lite
  - APBUART
  - PL011 (ARM)
  - RISC-V SBI console
  - ARM semihosting console
- Interrupt controllers
  - RISC-V PLIC and APLIC
  - ARM GIC-v2
- PCIe host bridge (generic ECAM)
- Framebuffer
  - Bochs VGA
  - Host-backed framebuffer

Platform support
----------------
- RISC-V
  - riscv32: Virtual target (QEMU virt)
  - riscv64: Virtual target (QEMU virt), StarFive VisionFive 2
- ARM64
  - Virtual target (QEMU virt)

Build & Installation
--------------------

All build- and runtime dependencies - the RISC-V and ARM64 cross toolchains,
QEMU, `dtc`, `cpio` and Python - are provided by the Nix flake shipped with
grinch. The recommended way to get a working environment is [Nix][nix] together
with [direnv][direnv]:

    git clone https://github.com/lfd/grinch.git
    cd grinch
    git submodule update --init --recursive
    direnv allow

`direnv` picks up the `.envrc` (`use flake`) and drops you into a shell with the
complete toolchain. Without direnv, enter the environment manually:

    nix develop

[nix]: https://nixos.org
[direnv]: https://direnv.net

For compiling grinch, simply run:

    make

To select the target architecture (default `riscv64`):

    make ARCH=riscv32
    make ARCH=arm64

For additional debug output, run:

    make DEBUG_OUTPUT=1

To enable GCOV coverage instrumentation, run:

    make GCOV=1

To override the optimisation level (default is `-O0`), run:

    make OPT=-O2

For verbose compiler output, run:

    make V=1

This will create `grinch.bin` and `user/initrd.cpio`. `grinch.bin` is the
loadable kernel image, which is directly loadable via Qemu on virtual targets,
or via U-Boot on real platforms. `user/initrd.cpio` contains userland
applications, as well as grinch itself (grinch is able to recursively boot
itself as virtual machine).

Build settings (`ARCH`, `OPT`, `GCOV`, ...) passed on the command line are
persisted to `config.mk` on the first invocation; subsequent invocations
reuse them automatically. Hand-edit `config.mk` to change a setting, or
run `make mrproper` to discard it (along with all build output).

For out-of-tree builds, pass `O=`:

    make O=build ARCH=riscv32 OPT=-O2

A small `Makefile` shim is generated in `build/`, so afterwards you can
work from the build directory directly:

    cd build
    make
    make qemu
    make mrproper

The source tree is checked for in-tree build artefacts and the OOT build
refuses to proceed if any are found, to avoid VPATH silently picking them
up. Run `make mrproper` in the source tree to clean it.

Demonstration in QEMU
---------------------

For running grinch inside QEMU, simply run:

    make qemu


For debugging grinch, you can attach with a debugger:

    make qemudb

In a second terminal, run:

    make debug

Grinch is also able to boot on Qemu via U-Boot. For booting grinch via U-Boot
in Qemu, type:

    make qemuu


Grinch Parameters
-----------------

To pass parameters to grinch on real board, you can use the `bootargs` variable
in U-Boot. Grinch parses those parameters. To pass parameters in Qemu, use:

    make qemu QEMU_APPEND='"arg1=val1 arg2=val2"'

You can also adjust the number of Qemus CPUs:

    make qemu QEMU_CPUS=4

Basic features:

| Parameter   | Values | Description                   |
| ---         | ---    | ---                           |
| timer_hz    | int    | Timer frequency               |
| loglevel    | int    | loglevel. Highest=0, Default=1|
| kheap_size  | int    | Kernel Heap size (e.g., 8M)   |
| ioremap_size| int    | Usable I/O remap size (e.g., 64M) |
| console     | str    | boot console (ttyS0)          |
| init        | str    | init executable               |

Development features:
| Parameter     | Values | Description                   |
| ---           | ---    | ---                           |
| memtest       | /      | Do memory test                |
| malloc_fsck   | /      | Run sanity checker for kalloc |
| ttp_maxevents | int    | No of maxevents for timed TPs |

Testing
-------

Automated tests drive QEMU from Python over a TCP serial socket, with
the HMP monitor used for guaranteed shutdown. `tests/run.py` builds
each variant, runs the applicable tests, and prints a summary table.

To run everything:

    ./tests/run.py

To build and run a single variant:

    ./tests/run.py riscv64-O0-plain

To build all variants without running tests:

    ./tests/run.py --build

To run tests against already-built variants:

    ./tests/run.py --run

By default, output goes to `build/test/`. Use `-o DIR` to change it:

    ./tests/run.py -o /tmp/grinch-tests

Authors & License
-----------------

Grinch is developed by Ralf Ramsauer at OTH Regensburg. It is licensed under
GPLv2. Parts of grinch are copied from Linux kernel sources. These parts
involve:
  - ctype implementation
  - List implementation (list.h)
  - Auxiliary headers (compiler_attributes.h, const.h, minmax.h)
  - vsprintf support
  - ELF headers
  - strtox
  - rwonce
