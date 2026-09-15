#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
#
# Grinch, a minimalist operating system
#
# Copyright (c) OTH Regensburg, 2026
#
# Authors:
#  Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
"""gConfig - the grinch build configuration tool.

Reads the gConfig DSL from the source tree and maintains config.mk (the
tunable values, and the source of truth) plus config.h (derived from it).

Commands:
  oldconfig   keep the existing values, add newly declared ones, write both
  defconfig   reset every value to its default, then write both
  menuconfig  edit interactively, then write both

A stored value that still matches the default it would compute today floats
with that default, so changing ARCH also updates CROSS_COMPILE.  A value that
differs is treated as a user override and is left alone.

default fields are stored as list[tuple[expr, expr|None]] - each entry is
(value_expr, condition_expr), where condition_expr is None for the
unconditional fallback.  The first entry whose condition is satisfied (or
whose condition is None) wins.

choices fields are list[str | tuple[str, expr]] - plain strings are always
present; tuple entries are included only when their condition is true.
"""
import argparse, sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

SRCTREE = Path(__file__).resolve().parent.parent

COMMANDS = ('oldconfig', 'defconfig', 'menuconfig')

# Guard against a gConfig whose defaults depend on each other cyclically.
MAX_ROUNDS = 100


@dataclass
class SymRef:
    name: str

@dataclass
class StrLit:
    val: str

@dataclass
class IntLit:
    val: int
    # As written, so that an address stays readable wherever it is rendered.
    text: str = ''

@dataclass
class BoolLit:
    val: bool

@dataclass
class BinOp:
    op: str
    left: Any
    right: Any

@dataclass
class UnOp:
    op: str
    operand: Any

@dataclass
class SymDecl:
    kind:     str
    name:     str
    flat:     str = ''
    label:    str = ''
    fields:   dict = field(default_factory=dict)
    children: list = field(default_factory=list)
    src_file: str = ''
    src_line: int = 0


KEYWORDS = {'const', 'bool', 'int', 'string'}
FIELDS   = {'default', 'value', 'depends', 'header', 'group', 'choices',
            'align', 'base', 'help'}
HEXDIGITS = set('0123456789abcdefABCDEF')


@dataclass
class Token:
    kind: str
    val:  Any
    line: int


def lex(src: str, fname: str):
    tokens, i, line = [], 0, 1
    while i < len(src):
        c = src[i]
        if c == '\n':
            line += 1; i += 1
        elif c in ' \t\r':
            i += 1
        elif c == '#':
            while i < len(src) and src[i] != '\n':
                i += 1
        elif c == '"':
            j = i + 1
            while j < len(src) and src[j] != '"':
                if src[j] == '\\': j += 1
                j += 1
            tokens.append(Token('string', src[i+1:j], line))
            i = j + 1
        elif c.isdigit():
            j = i
            if src[i:i+2].lower() == '0x':
                j = i + 2
                while j < len(src) and src[j] in HEXDIGITS: j += 1
            else:
                while j < len(src) and src[j].isdigit(): j += 1
            tokens.append(Token('number', src[i:j], line))
            i = j
        elif src[i:i+2] == '||':
            tokens.append(Token('op', '||', line)); i += 2
        elif src[i:i+2] == '&&':
            tokens.append(Token('op', '&&', line)); i += 2
        elif src[i:i+2] == '==':
            tokens.append(Token('op', '==', line)); i += 2
        elif src[i:i+2] == '!=':
            tokens.append(Token('op', '!=', line)); i += 2
        elif src[i:i+2] == '<<':
            tokens.append(Token('op', '<<', line)); i += 2
        elif c in '!(){}':
            tokens.append(Token('char', c, line)); i += 1
        elif c.isalpha() or c == '_':
            j = i
            while j < len(src) and (src[j].isalnum() or src[j] == '_'): j += 1
            word = src[i:j]
            if word in KEYWORDS:
                tokens.append(Token('kw', word, line))
            elif word in FIELDS:
                tokens.append(Token('field', word, line))
            elif word in ('y', 'n'):
                tokens.append(Token('bool', word == 'y', line))
            else:
                tokens.append(Token('ident', word, line))
            i = j
        else:
            sys.exit(f'{fname}:{line}: unexpected character {c!r}')
    tokens.append(Token('eof', None, line))
    return tokens


class Parser:
    def __init__(self, tokens, fname):
        self.tokens = tokens
        self.pos    = 0
        self.fname  = fname

    def peek(self):
        return self.tokens[self.pos]

    def consume(self, kind=None, val=None):
        t = self.tokens[self.pos]
        if kind and t.kind != kind:
            sys.exit(f'{self.fname}:{t.line}: expected {kind}, got {t.kind!r} ({t.val!r})')
        if val is not None and t.val != val:
            sys.exit(f'{self.fname}:{t.line}: expected {val!r}, got {t.val!r}')
        self.pos += 1
        return t

    def parse_file(self):
        decls = []
        while self.peek().kind != 'eof':
            t = self.peek()
            if t.kind == 'kw':
                decls.append(self._parse_sym(parent_flat='', parent_kind=None))
            else:
                sys.exit(f'{self.fname}:{t.line}: unexpected token {t.val!r}')
        return decls

    def _parse_sym(self, parent_flat, parent_kind):
        kind = self.consume('kw').val
        ln   = self.tokens[self.pos - 1].line
        nt   = self.consume()
        name = str(nt.val)

        label = ''
        if self.peek().kind == 'string':
            label = self.consume('string').val

        # bool/int/string parents prefix their children; const parents do not
        if parent_kind in ('bool', 'int', 'string') and parent_flat:
            flat = f'{parent_flat}_{name}'
        else:
            flat = name

        self.consume('char', '{')
        fields, children = {}, []
        while self.peek().val != '}':
            tok = self.peek()
            if tok.kind == 'kw':
                children.append(self._parse_sym(parent_flat=flat, parent_kind=kind))
            elif tok.kind == 'field':
                f = self.consume('field').val
                if f == 'header':
                    fields['header'] = self.consume('ident').val if self.peek().kind == 'ident' else True
                elif f == 'default':
                    val  = self._parse_expr()
                    cond = None
                    if self.peek().kind == 'ident' and self.peek().val == 'if':
                        self.consume()
                        cond = self._parse_expr()
                    fields.setdefault('default', []).append((val, cond))
                elif f == 'choices':
                    ch = []
                    while self.peek().kind == 'string' or (self.peek().kind == 'char' and self.peek().val == '('):
                        if self.peek().kind == 'string':
                            ch.append(self.consume('string').val)
                        else:
                            self.consume('char', '(')
                            cval  = self.consume('string').val
                            ctok  = self.consume('ident')
                            if ctok.val != 'if':
                                sys.exit(f'{self.fname}:{ctok.line}: expected "if" in conditional choice, got {ctok.val!r}')
                            ccond = self._parse_expr()
                            self.consume('char', ')')
                            ch.append((cval, ccond))
                    fields['choices'] = ch
                elif f == 'base':
                    bt = self.consume('ident')
                    if bt.val not in ('hex', 'dec', 'bin'):
                        sys.exit(f'{self.fname}:{bt.line}: base must be hex, dec or bin, got {bt.val!r}')
                    fields['base'] = bt.val
                elif f == 'group':
                    fields['group'] = self.consume('ident').val
                elif f == 'help':
                    fields['help'] = self.consume('string').val
                else:
                    fields[f] = self._parse_expr()
            else:
                sys.exit(f'{self.fname}:{tok.line}: unexpected token {tok.val!r}')
        self.consume('char', '}')
        return SymDecl(kind=kind, name=name, flat=flat, label=label,
                       fields=fields, children=children,
                       src_file=self.fname, src_line=ln)

    def _parse_expr(self):  return self._parse_or()
    def _parse_or(self):
        l = self._parse_and()
        while self.peek().kind == 'op' and self.peek().val == '||':
            self.consume(); l = BinOp('||', l, self._parse_and())
        return l
    def _parse_and(self):
        l = self._parse_cmp()
        while self.peek().kind == 'op' and self.peek().val == '&&':
            self.consume(); l = BinOp('&&', l, self._parse_cmp())
        return l
    def _parse_cmp(self):
        l = self._parse_shift()
        if self.peek().kind == 'op' and self.peek().val in ('==', '!='):
            op = self.consume().val; l = BinOp(op, l, self._parse_shift())
        return l
    def _parse_shift(self):
        l = self._parse_unary()
        while self.peek().kind == 'op' and self.peek().val == '<<':
            self.consume(); l = BinOp('<<', l, self._parse_unary())
        return l
    def _parse_unary(self):
        if self.peek().kind == 'char' and self.peek().val == '!':
            self.consume(); return UnOp('!', self._parse_atom())
        return self._parse_atom()
    def _parse_atom(self):
        t = self.peek()
        if t.kind == 'char'   and t.val == '(': self.consume(); e = self._parse_expr(); self.consume('char', ')'); return e
        if t.kind == 'ident':  self.consume(); return SymRef(t.val)
        if t.kind == 'string': self.consume(); return StrLit(t.val)
        if t.kind == 'number': self.consume(); return IntLit(int(t.val, 0), t.val)
        if t.kind == 'bool':   self.consume(); return BoolLit(t.val)
        sys.exit(f'{self.fname}:{t.line}: expected expression, got {t.kind!r} ({t.val!r})')


def parse_file(path):
    if not path.exists():
        return []
    return Parser(lex(path.read_text(), str(path)), str(path)).parse_file()


def eval_expr(expr, ctx):
    if isinstance(expr, SymRef):   return ctx.get(expr.name, 0)
    if isinstance(expr, StrLit):   return expr.val
    if isinstance(expr, IntLit):   return expr.val
    if isinstance(expr, BoolLit):  return expr.val
    if isinstance(expr, UnOp):
        if expr.op == '!': return not bool(eval_expr(expr.operand, ctx))
    if isinstance(expr, BinOp):
        l, r = eval_expr(expr.left, ctx), eval_expr(expr.right, ctx)
        if expr.op == '||': return bool(l) or bool(r)
        if expr.op == '&&': return bool(l) and bool(r)
        if expr.op == '==': return l == r
        if expr.op == '!=': return l != r
        if expr.op == '<<': return l << r
    raise ValueError(f'unknown expr {expr!r}')


def _walk(decls, ordered, by_flat):
    for decl in decls:
        if isinstance(decl, SymDecl):
            if decl.flat in by_flat:
                sys.exit(f'{decl.src_file}:{decl.src_line}: duplicate symbol {decl.flat!r}')
            by_flat[decl.flat] = decl
            ordered.append(decl)
            _walk(decl.children, ordered, by_flat)


def resolve(root_decls):
    ordered, by_flat = [], {}
    _walk(root_decls, ordered, by_flat)

    section_of = {}
    def annotate(decls, section):
        for d in decls:
            if not isinstance(d, SymDecl): continue
            if d.kind == 'const':
                annotate(d.children, d.label or d.name)
            else:
                section_of[d.flat] = section
                annotate(d.children, section)
    annotate(root_decls, '')

    return ordered, by_flat, section_of


def is_tunable(sym):
    """True for symbols the user sets: they live in config.mk and have a default."""
    return (isinstance(sym, SymDecl) and sym.kind != 'const'
            and 'value' not in sym.fields and 'default' in sym.fields)


def deps_met(sym, ctx):
    dep = sym.fields.get('depends')
    if dep is None:
        return True
    try:
        return bool(eval_expr(dep, ctx))
    except Exception:
        return True


def value_complaint(sym, ctx, value):
    """What an int holds against a proposed value, or ''."""
    if sym.kind != 'int':
        return ''
    try:
        v = int(str(value), 0)
    except ValueError:
        return f'{sym.flat}: {value!r} is not a number'
    if 'align' in sym.fields and deps_met(sym, ctx):
        align = eval_expr(sym.fields['align'], ctx)
        if align and v % align:
            return f'{sym.flat} = {v:#x} is not a multiple of {align:#x}'
    return ''


def check_values(ordered, values, ctx):
    """Refuse to hand the build a value an option holds a complaint against."""
    bad = False
    for sym in ordered:
        if not is_tunable(sym) or sym.flat not in values:
            continue
        msg = value_complaint(sym, ctx, values[sym.flat])
        if msg:
            print(f'{sym.src_file}:{sym.src_line}: {msg}', file=sys.stderr)
            bad = True
    if bad:
        sys.exit(1)


def to_ctx_val(sym, v):
    if sym.kind == 'bool':  return v in ('y', '1')
    if sym.kind == 'int':
        try:
            return int(str(v), 0)
        except ValueError:
            return 0
    return v


def build_ctx(values, by_flat):
    """Coerce the config.mk string values into an evaluation context."""
    ctx = {}
    for flat, v in values.items():
        sym = by_flat.get(flat)
        if sym is not None:
            ctx[flat] = to_ctx_val(sym, v)
    return ctx


def norm_value(sym, v):
    """Accept 1/0 for bools on the command line; config.mk stores y or empty."""
    if sym.kind != 'bool':
        return v
    return 'y' if v in ('y', '1') else ''


def seed_default(sym):
    """Return the first unconditional default as a string, without needing ctx."""
    for val_expr, cond_expr in sym.fields.get('default', []):
        if cond_expr is None:
            if isinstance(val_expr, BoolLit): return 'y' if val_expr.val else ''
            if isinstance(val_expr, IntLit):  return val_expr.text
            if isinstance(val_expr, StrLit):  return val_expr.val
            return ''
    return ''


def eval_default(sym, ctx):
    """Return the first matching conditional default evaluated against ctx."""
    for val_expr, cond_expr in sym.fields.get('default', []):
        if cond_expr is None or bool(eval_expr(cond_expr, ctx)):
            if isinstance(val_expr, IntLit): return val_expr.text
            v = eval_expr(val_expr, ctx)
            if isinstance(v, bool): return 'y' if v else ''
            return str(v) if v is not None else ''
    return ''


def active_choices(choices, ctx):
    """Return the choices list with conditional entries filtered by ctx."""
    result = []
    for item in choices:
        if isinstance(item, tuple):
            val, cond = item
            if cond is None or bool(eval_expr(cond, ctx)):
                result.append(val)
        else:
            result.append(item)
    return result


def declared_defaults(sym, ctx):
    """Every value the symbol's default clauses can yield, ignoring conditions."""
    out = []
    for val_expr, _ in sym.fields.get('default', []):
        # A number keeps the form it was written in, so it still compares
        # equal to what config.mk carries.
        if isinstance(val_expr, IntLit):
            out.append(val_expr.text)
            continue
        try:
            v = eval_expr(val_expr, ctx)
        except Exception:
            continue
        out.append('y' if v is True else '' if v is False else str(v))
    return out


def compute_pinned(ordered, values, ctx, stored, cmdline=()):
    """Collect the symbols that must not follow their default any more.

    A stored value that no default clause could have produced is a user
    override and is left alone.  Anything else floats, so switching ARCH
    drags CROSS_COMPILE and the driver selection along with it.
    """
    pinned = set(cmdline)
    for sym in ordered:
        # Never written down and not given on the command line: no user intent.
        if not is_tunable(sym) or sym.flat in pinned or sym.flat not in stored:
            continue
        if values.get(sym.flat, '') not in declared_defaults(sym, ctx):
            pinned.add(sym.flat)
    return pinned


def _pass_computed(ordered, ctx):
    changed = False
    for sym in ordered:
        if 'value' not in sym.fields:
            continue
        try:
            v = eval_expr(sym.fields['value'], ctx)
        except Exception:
            continue
        if ctx.get(sym.flat) != v:
            ctx[sym.flat] = v
            changed = True
    return changed


def _pass_defaults(ordered, values, ctx, pinned):
    changed = False
    for sym in ordered:
        if not is_tunable(sym) or sym.flat in pinned or not deps_met(sym, ctx):
            continue
        v = eval_default(sym, ctx)
        if v != values.get(sym.flat, ''):
            values[sym.flat] = v
            changed = True
        cv = to_ctx_val(sym, v)
        if ctx.get(sym.flat) != cv:
            ctx[sym.flat] = cv
            changed = True
    return changed


def _pass_depends(ordered, values, ctx):
    """Force off symbols whose depends is unmet; reset strings outside their choices."""
    changed = False
    for sym in ordered:
        if not is_tunable(sym):
            continue

        # An unmet symbol is off, whatever its choices say.
        if not deps_met(sym, ctx):
            off_val = '0' if sym.kind == 'int' else ''
            off_ctx = 0   if sym.kind == 'int' else (False if sym.kind == 'bool' else '')
            if values.get(sym.flat) != off_val:
                values[sym.flat] = off_val
                ctx[sym.flat]    = off_ctx
                changed = True
            continue

        choices_raw = sym.fields.get('choices')
        if sym.kind == 'string' and choices_raw is not None:
            choices = active_choices(choices_raw, ctx)
            if values.get(sym.flat, '') not in choices:
                v = eval_default(sym, ctx)
                if choices and v not in choices:
                    v = choices[0]
                values[sym.flat] = v
                ctx[sym.flat]    = v
                changed = True
    return changed


def settle_computed(ordered, ctx):
    """Bring the computed (value-type) symbols up to date with ctx."""
    for _ in range(MAX_ROUNDS):
        if not _pass_computed(ordered, ctx):
            return
    sys.exit('gConfig: computed values did not converge; check for a cycle')


def settle(ordered, values, ctx, pinned):
    """Drive computed values, floating defaults and depends to a fixed point.

    Mutates values and ctx in place.  The three passes are complementary: a
    symbol with unmet depends is only touched by _pass_depends, one that is
    pinned only by _pass_depends, so they cannot fight over the same symbol.
    """
    for _ in range(MAX_ROUNDS):
        changed  = _pass_computed(ordered, ctx)
        changed |= _pass_defaults(ordered, values, ctx, pinned)
        changed |= _pass_depends(ordered, values, ctx)
        if not changed:
            return
    sys.exit('gConfig: defaults did not converge; check for a dependency cycle')


MK_HEADER = "# Auto-generated by scripts/gconfig.py. Edit values; 'make defconfig' resets."
H_HEADER  = "/* Auto-generated from gConfig. Do not edit. */"


def render_mk(ordered, section_of, values, ctx):
    out, cur_section = [MK_HEADER], None
    for sym in ordered:
        if not is_tunable(sym) or not deps_met(sym, ctx):
            continue
        sec = section_of.get(sym.flat, '')
        if sec != cur_section:
            out.append(f'\n# {sec}')
            cur_section = sec
        out.append(f'{sym.flat}={values.get(sym.flat, "")}')
    return '\n'.join(out) + '\n'


def render_h(ordered, ctx):
    out = [H_HEADER]
    for sym in ordered:
        if not isinstance(sym, SymDecl) or 'header' not in sym.fields: continue
        # An option that does not apply stays undefined, rather than defined
        # to something that reads like an answer.
        if not deps_met(sym, ctx): continue
        hdr  = sym.fields['header']
        name = hdr if isinstance(hdr, str) else sym.flat
        v    = ctx.get(sym.flat)
        if sym.kind == 'bool':
            if v in (True, 'y', '1', 1):
                out.append(f'#define {name} 1')
        elif sym.kind == 'int':
            # Hex unless told otherwise, so that an address wide enough to be
            # unsigned still forms a valid C constant.
            fmt = {'dec': '{}', 'bin': '{:#b}'}.get(sym.fields.get('base'), '{:#x}')
            out.append(f'#define {name} ' + fmt.format(v if v is not None else 0))
        elif sym.kind == 'string':
            out.append(f'#define {name} "{v or ""}"')
    return '\n'.join(out) + '\n'


def read_mk(path):
    values = {}
    if path.exists():
        for line in path.read_text().splitlines():
            line = line.strip()
            if line and not line.startswith('#') and '=' in line:
                k, _, v = line.partition('=')
                values[k.strip()] = v.strip().strip('"')
    return values


def write_if_changed(path, text):
    if path.exists() and path.read_text() == text:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + '.new')
    tmp.write_text(text)
    tmp.replace(path)
    return True


@dataclass
class Config:
    roots:      list
    ordered:    list
    by_flat:    dict
    section_of: dict
    values:     dict
    ctx:        dict
    pinned:     set

    def settle(self):
        settle(self.ordered, self.values, self.ctx, self.pinned)

    def set(self, flat, value):
        """Record an explicit choice, then let everything unpinned follow it."""
        self.values[flat] = value
        self.pinned.add(flat)
        self.ctx = build_ctx(self.values, self.by_flat)
        self.settle()


def load(config_mk, cmdline=None, defconfig=False):
    """Parse gConfig, merge in config.mk and any --set values, and settle."""
    cmdline = cmdline or {}
    roots   = parse_file(SRCTREE / 'gConfig')
    ordered, by_flat, section_of = resolve(roots)

    stored = {} if defconfig else read_mk(config_mk)
    values = {}
    for sym in ordered:
        if not is_tunable(sym):
            continue
        if sym.flat in stored:
            values[sym.flat] = norm_value(sym, stored[sym.flat])
        elif sym.flat in cmdline:
            values[sym.flat] = norm_value(sym, cmdline[sym.flat])
        else:
            values[sym.flat] = seed_default(sym)

    # The computed symbols have to be live before a stored value can be told
    # apart from the default it would compute to.
    ctx = build_ctx(values, by_flat)
    settle_computed(ordered, ctx)

    cfg = Config(roots, ordered, by_flat, section_of, values, ctx,
                 compute_pinned(ordered, values, ctx, stored, cmdline))
    cfg.settle()
    return cfg


def save(cfg, config_mk, config_h=None):
    """Write config.mk, then derive config.h from what actually landed in it."""
    changed = []
    if write_if_changed(config_mk, render_mk(cfg.ordered, cfg.section_of,
                                             cfg.values, cfg.ctx)):
        changed.append(config_mk.name)

    if config_h is not None:
        stored = load(config_mk)
        if write_if_changed(config_h, render_h(stored.ordered, stored.ctx)):
            changed.append(config_h.name)
    return changed


@dataclass
class Item:
    sym:   Any    # SymDecl
    label: str
    kids:  list   # list[Item]; non-empty → has submenu


def _items(decls, by_flat):
    out = []
    for d in decls:
        if not isinstance(d, SymDecl):
            continue
        sym = by_flat.get(d.flat, d)
        if 'value' in sym.fields:          # computed — not user-editable
            continue
        kids = _items(d.children, by_flat)
        out.append(Item(sym=sym, label=sym.label or sym.flat, kids=kids))
    return out


def build_tree(root_decls, by_flat):
    return _items(root_decls, by_flat)


C_SEL   = 1   # selected row
C_DIM   = 2   # depends not met
C_HDR   = 3   # title bar
C_VAL   = 4   # value highlight
C_ON    = 5   # depends token: true
C_OFF   = 6   # depends token: false
C_ERR   = 7   # a refused value on the status line


def dep_ok(item, ctx):
    return item.sym is None or deps_met(item.sym, ctx)


def _colored_expr(expr, ctx):
    """Yield (text, attr) pairs for an expression, colouring SymRefs by truth value."""
    if isinstance(expr, SymRef):
        attr = curses.color_pair(C_ON if ctx.get(expr.name) else C_OFF) | curses.A_BOLD
        yield (expr.name, attr)
    elif isinstance(expr, StrLit):
        yield (f'"{expr.val}"', 0)
    elif isinstance(expr, IntLit):
        yield (str(expr.val), 0)
    elif isinstance(expr, BoolLit):
        attr = curses.color_pair(C_ON if expr.val else C_OFF) | curses.A_BOLD
        yield ('y' if expr.val else 'n', attr)
    elif isinstance(expr, UnOp):
        yield ('!', 0)
        yield from _colored_expr(expr.operand, ctx)
    elif isinstance(expr, BinOp):
        pl = expr.op == '&&' and isinstance(expr.left,  BinOp) and expr.left.op  == '||'
        pr = expr.op == '&&' and isinstance(expr.right, BinOp) and expr.right.op == '||'
        if pl: yield ('(', 0)
        yield from _colored_expr(expr.left, ctx)
        if pl: yield (')', 0)
        yield (f' {expr.op} ', 0)
        if pr: yield ('(', 0)
        yield from _colored_expr(expr.right, ctx)
        if pr: yield (')', 0)


class App:
    def __init__(self, scr, cfg, mk_path, h_path):
        self.scr      = scr
        self.cfg      = cfg
        self.mk_path  = mk_path
        self.h_path   = h_path
        self.modified = False
        self.status   = ''
        self.alert    = False
        # stack entries: (title, item_list, cursor, scroll)
        self.stack    = [('Grinch Configuration',
                          build_tree(cfg.roots, cfg.by_flat), 0, 0)]

    @property
    def values(self):
        return self.cfg.values

    @property
    def ctx(self):
        return self.cfg.ctx

    def draw(self):
        scr = self.scr
        scr.erase()
        rows, cols = scr.getmaxyx()
        title, items, cur, scroll = self.stack[-1]
        list_h = rows - 2   # 1 header + 1 footer

        # Header
        mod = ' [modified]' if self.modified else ''
        hdr = f' {title}{mod}'
        scr.attron(curses.color_pair(C_HDR) | curses.A_BOLD)
        try: scr.addstr(0, 0, hdr.ljust(cols))
        except curses.error: pass
        scr.attroff(curses.color_pair(C_HDR) | curses.A_BOLD)

        # Item list
        for i, it in enumerate(items[scroll:scroll + list_h]):
            row = 1 + i
            idx = i + scroll
            sel = (idx == cur)
            ok  = dep_ok(it, self.ctx)
            line = self._fmt(it)

            if sel:
                attr = curses.color_pair(C_SEL)
            elif not ok:
                attr = curses.A_DIM
            elif it.sym.kind == 'const':
                attr = curses.A_BOLD
            else:
                attr = 0
            try: scr.addstr(row, 0, line[:cols].ljust(cols), attr)
            except curses.error: pass

        # Footer
        if self.status:
            hint = f' {self.status}'
        elif len(self.stack) > 1:
            hint = ' [↑↓] Nav  [Spc/Enter] Toggle/Edit  [←/Esc] Back  [?] Help  [S] Save  [Q] Quit'
        else:
            hint = ' [↑↓] Nav  [Enter/→] Open  [?] Help  [S] Save  [Q] Quit'
        footer = curses.color_pair(C_ERR) | curses.A_BOLD if self.alert \
                 else curses.A_REVERSE
        try: scr.addstr(rows - 1, 0, hint[:cols].ljust(cols), footer)
        except curses.error: pass

        scr.refresh()

    def _fmt(self, it):
        sym = it.sym
        v   = self.values.get(sym.flat, '')
        if sym.kind == 'const':
            return f'    {it.label}  --->'
        if sym.kind == 'bool':
            mark   = '*' if v == 'y' else ' '
            suffix = '  --->' if it.kids else ''
            return f'  [{mark}] {it.label}{suffix}'
        if sym.kind in ('string', 'int'):
            suffix = '  --->' if it.kids else ''
            return f'  ({v})  {it.label}{suffix}'
        return f'  {it.label}'

    def run(self):
        curses.init_pair(C_SEL, curses.COLOR_BLACK,  curses.COLOR_WHITE)
        curses.init_pair(C_DIM, curses.COLOR_WHITE,  curses.COLOR_BLACK)
        curses.init_pair(C_HDR, curses.COLOR_BLACK,  curses.COLOR_CYAN)
        curses.init_pair(C_VAL, curses.COLOR_CYAN,   curses.COLOR_BLACK)
        curses.init_pair(C_ON,  curses.COLOR_GREEN,  curses.COLOR_BLACK)
        curses.init_pair(C_OFF, curses.COLOR_RED,    curses.COLOR_BLACK)
        curses.init_pair(C_ERR, curses.COLOR_WHITE,  curses.COLOR_RED)
        curses.curs_set(0)
        self.scr.keypad(True)

        while True:
            self.draw()
            key = self.scr.getch()
            if self._handle(key):
                break

    def _handle(self, key):
        self.status = ''
        self.alert  = False
        title, items, cur, scroll = self.stack[-1]
        rows, cols = self.scr.getmaxyx()
        list_h = rows - 2

        if key == curses.KEY_UP:
            if cur > 0:
                cur -= 1
                if cur < scroll:
                    scroll = cur
            self.stack[-1] = (title, items, cur, scroll)

        elif key == curses.KEY_DOWN:
            if cur < len(items) - 1:
                cur += 1
                if cur >= scroll + list_h:
                    scroll = cur - list_h + 1
            self.stack[-1] = (title, items, cur, scroll)

        elif key in (curses.KEY_LEFT, 27):          # ← or Esc → back
            if len(self.stack) > 1:
                self.stack.pop()

        elif key in (*ENTER, curses.KEY_RIGHT, ord(' ')):
            if not items:
                return False
            it  = items[cur]
            sym = it.sym
            ok  = dep_ok(it, self.ctx)

            # Enter submenu: const always; bool only when value is y; others when dep_ok
            bool_on = sym.kind != 'bool' or self.values.get(sym.flat, '') == 'y'
            if it.kids and (sym.kind == 'const' or (ok and bool_on)) and key not in (ord(' '),):
                self.stack.append((it.label, it.kids, 0, 0))
            elif sym.kind == 'bool' and ok:
                v = self.values.get(sym.flat, '')
                self._set(sym, '' if v == 'y' else 'y')
            elif sym.kind in ('string', 'int') and ok:
                new = self._edit(sym)
                if new is not None:
                    msg = value_complaint(sym, self.ctx, new)
                    if msg:
                        self.status = msg
                        self.alert  = True
                    else:
                        self._set(sym, new)

        elif key == ord('?'):
            self._help()

        elif key in (ord('s'), ord('S')):
            self._save()

        elif key in (ord('q'), ord('Q')):
            if self.modified and self._confirm('Save before exit?'):
                self._save()
            return True

        return False

    def _set(self, sym, value):
        self.cfg.set(sym.flat, value)
        self.modified = True

    def _edit(self, sym):
        choices_raw = sym.fields.get('choices')
        if choices_raw is not None:
            choices = active_choices(choices_raw, self.ctx)
            return self._pick(sym.label, choices, self.values.get(sym.flat, ''))
        return self._readline(f'{sym.label}: ', self.values.get(sym.flat, ''))

    def _pick(self, title, choices, current):
        scr = self.scr
        rows, cols = scr.getmaxyx()
        cur = choices.index(current) if current in choices else 0
        while True:
            scr.erase()
            try: scr.addstr(0, 1, f'Select: {title}', curses.A_BOLD)
            except curses.error: pass
            for i, ch in enumerate(choices):
                mark = '*' if i == cur else ' '
                attr = curses.color_pair(C_SEL) if i == cur else 0
                try: scr.addstr(2 + i, 3, f'({mark}) {ch}'.ljust(cols - 4), attr)
                except curses.error: pass
            try: scr.addstr(rows - 1, 0,
                            ' [↑↓] Navigate  [Enter] Select  [Esc] Cancel'.ljust(cols),
                            curses.A_REVERSE)
            except curses.error: pass
            scr.refresh()
            k = scr.getch()
            if k == curses.KEY_UP   and cur > 0:               cur -= 1
            elif k == curses.KEY_DOWN and cur < len(choices)-1: cur += 1
            elif k in ENTER:                                    return choices[cur]
            elif k == 27:                                       return None

    def _readline(self, prompt, initial=''):
        scr = self.scr
        rows, cols = scr.getmaxyx()
        curses.curs_set(1)
        buf, pos = list(initial), len(initial)
        row = rows // 2
        while True:
            scr.move(row, 0)
            scr.clrtoeol()
            text = prompt + ''.join(buf)
            try: scr.addstr(row, 0, text[:cols - 1])
            except curses.error: pass
            scr.move(row, min(len(prompt) + pos, cols - 1))
            scr.refresh()
            k = scr.getch()
            if k in ENTER:
                curses.curs_set(0); return ''.join(buf)
            elif k == 27:
                curses.curs_set(0); return None
            elif k in (curses.KEY_BACKSPACE, 127, 8):
                if pos > 0: buf.pop(pos - 1); pos -= 1
            elif k == curses.KEY_LEFT  and pos > 0:       pos -= 1
            elif k == curses.KEY_RIGHT and pos < len(buf): pos += 1
            elif k == curses.KEY_HOME:                     pos = 0
            elif k == curses.KEY_END:                      pos = len(buf)
            elif 32 <= k < 127:
                buf.insert(pos, chr(k)); pos += 1

    def _confirm(self, msg):
        scr = self.scr
        rows, cols = scr.getmaxyx()
        try: scr.addstr(rows // 2, 0, f' {msg} [Y/n] '.ljust(cols), curses.A_REVERSE)
        except curses.error: pass
        scr.refresh()
        k = scr.getch()
        return k in (*ENTER, ord('y'), ord('Y'))

    def _help(self):
        _, items, cur, _ = self.stack[-1]
        if not items:
            return
        it  = items[cur]
        sym = it.sym
        scr = self.scr
        rows, cols = scr.getmaxyx()

        scr.erase()
        scr.attron(curses.color_pair(C_HDR) | curses.A_BOLD)
        try: scr.addstr(0, 0, f' Help: {it.label}'.ljust(cols))
        except curses.error: pass
        scr.attroff(curses.color_pair(C_HDR) | curses.A_BOLD)

        hdr = sym.fields.get('header')
        if hdr is None:
            hdr_name = '(none)'
        elif isinstance(hdr, str):
            hdr_name = hdr
        else:
            hdr_name = sym.flat

        info = [f'Symbol:  {sym.flat}', f'Type:    {sym.kind}',
                f'Header:  {hdr_name}']
        if 'align' in sym.fields:
            info.append(f'Align:   {eval_expr(sym.fields["align"], self.ctx):#x}')
        if 'base' in sym.fields:
            info.append(f'Base:    {sym.fields["base"]}')

        row = 1
        for line in info:
            if row >= rows - 1: break
            try: scr.addstr(row, 2, line[:cols - 2])
            except curses.error: pass
            row += 1

        if sym.kind != 'const' and row + 1 < rows - 1:
            row += 1  # blank separator
            prefix = 'Depends: '
            try: scr.addstr(row, 2, prefix)
            except curses.error: pass
            dep = sym.fields.get('depends')
            col = 2 + len(prefix)
            if dep is None:
                try: scr.addstr(row, col, '(none)')
                except curses.error: pass
            else:
                for text, attr in _colored_expr(dep, self.ctx):
                    if col + len(text) > cols - 1:
                        break
                    try: scr.addstr(row, col, text, attr)
                    except curses.error: pass
                    col += len(text)
            row += 1

        help_txt = sym.fields.get('help', '')
        if help_txt and row + 1 < rows - 1:
            row += 1  # blank separator
            for line in textwrap.wrap(help_txt, cols - 4):
                if row >= rows - 1:
                    break
                try: scr.addstr(row, 2, line)
                except curses.error: pass
                row += 1

        try: scr.addstr(rows - 1, 0, ' [Any key] Close'.ljust(cols), curses.A_REVERSE)
        except curses.error: pass
        scr.refresh()
        scr.getch()

    def _save(self):
        # A value that arrived broken in config.mk leaves the same way the
        # editor refuses it: named, not written.
        for sym in self.cfg.ordered:
            if is_tunable(sym) and sym.flat in self.values:
                msg = value_complaint(sym, self.ctx, self.values[sym.flat])
                if msg:
                    self.status = msg
                    self.alert  = True
                    return
        names = save(self.cfg, self.mk_path, self.h_path)
        self.modified = False
        self.status   = f'Saved {", ".join(names)}.' if names else 'Nothing to save.'


def run_menu(cfg, mk_path, h_path):
    # Deferred: every build runs this file, but hardly any run needs the TUI.
    global curses, textwrap, ENTER
    import curses, textwrap
    ENTER = (curses.KEY_ENTER, ord('\n'), ord('\r'))

    curses.wrapper(lambda scr: App(scr, cfg, mk_path, h_path).run())


def main():
    ap = argparse.ArgumentParser(
        prog='gconfig.py',
        description='Maintain the grinch build configuration.',
        epilog='Commands: ' + ', '.join(COMMANDS))
    ap.add_argument('command', choices=COMMANDS, metavar='COMMAND')
    ap.add_argument('--config-mk', type=Path, required=True)
    ap.add_argument('--config-h',  type=Path)
    ap.add_argument('--set', action='append', metavar='KEY=VALUE', default=[])
    args = ap.parse_args()

    cmdline = {}
    for kv in args.set:
        k, _, v = kv.partition('=')
        cmdline[k.strip()] = v.strip()

    cfg = load(args.config_mk, cmdline, defconfig=args.command == 'defconfig')

    if args.command == 'menuconfig':
        run_menu(cfg, args.config_mk, args.config_h)
        return

    check_values(cfg.ordered, cfg.values, cfg.ctx)
    changed = save(cfg, args.config_mk, args.config_h)
    if changed:
        print(' '.join(changed))


if __name__ == '__main__':
    main()
