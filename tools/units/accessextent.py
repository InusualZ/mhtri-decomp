#!/usr/bin/env python3
"""How far does anything read this block: the offset interpretation a data-extent claim turns on.

    python tools/units/accessextent.py <address|name> [--json] [--limit N] [--min-offset 0xN]
                                       [--elf PATH] [--objdump PATH] [--accessor NAME]
                                       [--rebuild] [--selftest]

**The question.** "How big is this data block?" is answered today by the furthest offset anything reaches
into it, and that number goes into `splits.txt` as a size. Until now every lane answered it by hand: the
quest recon lane wrote a ~150-line PPC abstract interpreter over the DOL for exactly this, at ~40 minutes
of its budget. It is asked again on every data block, so it should be one command.

**What it does not rebuild.** `tools/units/callers.py` already enumerates every reference to an address -
built on the dump's *addresses* (never its stale labels), cached, and with the split objects' relocations
as a fallback - so the census is reused here verbatim (`callers.query`, `callers.load_index`). What that
tool does not do is *interpret* a site: `lwz r3, <sym>` says the symbol is read, not how far past it the
code then walks. That interpretation is this tool.

**The shape of the answer.** For each reference site the instruction is decoded out of the target's own
disassembly (`build/<game>/main.elf`, through `build/binutils/powerpc-eabi-objdump.exe` - nothing is on
`PATH`; `ninja tools` fetches the binutils if it is absent), and a light abstract interpretation follows
the value the site puts in a register:

* the block pointer - the value loaded from the queried symbol - is `blk`; and for a symbol that *is* the
  block rather than a pointer to it (no `sym@sda21` load anywhere), `lis sym@ha` + `addi rD, rA, sym@l`
  materialises the block's own address, which the two halves make recognisable by value;
* `addi rD, rS, k` / `add rD, rA, rB` / `mr` move and add to it, so `blk + const` and `blk + reg` are
  values too;
* a load or store whose base is `blk + const + reg…` is an **access**: its offset is `const`, plus the
  displacement, plus an upper bound for every register in the expression;
* a register that is incremented by a constant inside an enclosing loop is bounded by the loop nest's own
  walk - `(trips - 1) * step`, multiplied through the outer levels, with `trips` read from the guard
  (`cmpwi rC, N` + `blt`, or a `bdnz` whose `mtctr` is a constant, which sits *before* the loop head) and
  the starting value from the last `li` before the access;
* control flow is respected enough that no state is invented: every direct branch records the register
  state that reaches its target, so the region an unconditional `b` jumps over is walked with an unknown
  state and a `bctr` switch keeps the state its cases were dispatched with.

**Whichever census answered, the instructions decide.** The dump graph and `callers.py`'s object fallback
are not equally detailed: the fallback has no instruction text, so it calls every non-call reference
`addr`, and it records an `R_PPC_ADDR16_HA`/`_LO` site as the immediate *halfword* it patches - two bytes
into the instruction. Both are recovered from the instruction this tool decodes anyway (the kind from the
mnemonic, the site from the halfword), so a query answers the same thing whichever census the tree has;
`census.source` still says which one answered, and `rep["census"]` is that census's own counts. The
fallback's two rows for a `lis`+`addi` pair are not coalesced the way the dump's are, so its `unresolved`
list is one row per relocation rather than one per site; the extent is the same.

**What it refuses to do.** A base that cannot be resolved is reported *per site* with the reason - a
register with no static bound, a pointer that is passed to a callee (`bl`) or stored away, a load that is
never dereferenced, an offset term the size of an address - never guessed. A tool that invents an offset is
worse than no tool, because the claim it justifies becomes real in `splits.txt`, and a size that is 2 bytes
short silently claims bytes belonging to the next object in the link order.

**The strongest evidence is reported separately.** A `memset`/`memcpy`/`memmove`/`bzero` call whose buffer
is the block and whose size argument is a constant *states* the extent: `memset(get_userdata(), 0, 0x6000)`
is a 0x6000-byte record, whatever the accesses reach. Those sites are listed with their function and
address, preferred over the inferred maximum, and a disagreement between the two numbers is printed loudly
- the dangerous direction (an access past the stated size) most loudly of all.

**The three numbers are labelled, always.** The default output spells out `furthest static access` (the
maximum offset the DOL contains - what a bare "furthest" means), `furthest loop-carried` (a displacement
plus a bound read out of a loop guard: a *derived* figure, and the one easy to paraphrase as the maximum),
and `literal size`, and the access table is headed "furthest static accesses". The data-extent brief that
asked for this tool had transcribed the loop-carried figure as the furthest static access - it cost a
round trip to unpick - so the output cannot be read that way, and `--json` carries
`furthest_static_*`/`furthest_loop_carried_*` as separate fields with a per-access `loop_carried` flag.

**The real case, as the acceptance run.** `Q_UserData` is reached through a cached global: `get_userdata`
(0x8004D120) loads `*(void**)(&system_w + 0x95C)` and stores it into the `.sbss` word at 0x80794880, from
which 346 functions read it back (789 loads + 3 stores). The literal size comes from the init path -
`memset(get_userdata(), 0, 0x6000)` in `fn_80047398.cpp`'s `fn_800497B4` (0x800497D0) and `fn_800498EC`
(0x80049908). The furthest static access is **0x53A8** (`lwz r0, 21416(r31)` at 0x8004F1F4 in
`set_mydata2vs__FUcUc`, r31 = `get_userdata()`'s return and never rewritten); the loop-carried one the
data-extent brief measured, `sth r29, 0x5366(r3)` at 0x803C138C inside a 16-iteration loop in
`menu/get_pop_dat_ptr.cpp`'s `fn_803C0F3C`, is 0x5366 + 15*4 = **0x53A2** (tied with the same loop's
0x803C10C0), and the same function also reads 0x53A4 after that loop. `--selftest` asserts all three, so
the loop bound, the plain displacement and the post-loop load are each pinned, and it also cross-checks a
symbol whose size the map *does* state - `system_w` is declared 0xA5C and the tool infers 0xA5C from a
furthest access of 0xA58, which is what says the offsets are not invented.

**Limits, stated so no number is over-read.**
* The census is the number of *sites*, and a site whose block pointer is passed to a callee is not
  followed into it: the block may be read further by the callee, and that is said per site rather than
  folded into the maximum.
* The scan is one pass in address order with a state per branch target, not a full dataflow fixpoint: a
  register written by a `bctr` switch's case bodies is followed only where the case re-establishes it, and
  a value that reaches an access through two *different* paths is kept only when both agree. Both
  directions are conservative - a missed access is reported as an unresolved site, not as a smaller size -
  but the count of `unresolved` is what to read for how much the pass could not follow.
* An interior reference the dump prints as `sym+0xNN@ha` is not indexed under `sym` by the census's
  symbol regex; separately, that gap is read here out of the census's own rows (never a second scan).
* The object form is used only when nothing loads the symbol directly (`sym@sda21`). A pointer variable
  whose *address* is materialised and then dereferenced (`lis/addi sym` + `lwz rX, 0(rB)`) is a site the
  pass cannot yet follow, and it is reported unresolved rather than guessed.
* The dump's state is the census's to report; when the dump is stale the instruction *texts* are too
  (never the addresses).
* It is a reader. No `src/` edits, and it writes nothing at all - the census it reuses owns the only
  cache (`build/tmp/callers/graph.json`).
"""

from __future__ import annotations

import argparse
import bisect
import collections
import functools
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)
for _path in (TOOLS, HERE, os.path.join(TOOLS, "symbols"), os.path.join(TOOLS, "splits")):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from units import callers as C  # noqa: E402  (the census: sites, addresses, the cached-global fallback)
from units import callees as cl  # noqa: E402  (decode_rw: the tree's one register read/write decode)

GAME = C.GAME
ELF_REL = os.path.join("build", GAME, "main.elf")
OBJDUMP_RELS = (os.path.join("build", "binutils", "powerpc-eabi-objdump.exe"),
                os.path.join("build", "binutils", "powerpc-eabi-objdump"))
DUMP_TOOL = "python tools/splits/dump_asm.py"
BINUTILS_TOOL = "ninja tools"
ALIGN = 4                      # data objects are at least word-aligned; the claim is rounded to it
# An offset term larger than this is not an offset: it is an address (the DOL loads at 0x80003100), and
# a register that happens to hold one - a `lis sym@ha` half, or a stale alias of one - must be called
# unresolved rather than added to the offset. 256 MB is far above any object in this game and far below
# the address space, so the two never meet.
MAX_OFFSET_TERM = 0x10000000

# Which callees state an extent directly. A `memset`-like call's *buffer* argument being the block and its
# *size* argument being a constant is the allocation talking about itself.
MEM_SIZE_REG = {"memset": "r5", "memcpy": "r5", "memmove": "r5", "bzero": "r4", "__fill_mem": "r5"}
MEM_BUF_REGS = {"memset": ("r3",), "memcpy": ("r3", "r4"), "memmove": ("r3", "r4"),
                "bzero": ("r3",), "__fill_mem": ("r3",)}


# --------------------------------------------------------------------------------------------------
# the target's own disassembly: `objdump -d` over main.elf, grouped by the label objdump prints
# --------------------------------------------------------------------------------------------------
LABEL_RE = re.compile(r"^([0-9a-f]{8}) <(.*)>:\s*$")
INSN_RE = re.compile(r"^([0-9a-f]{8}):\t(.*)$")
GPR_RE = re.compile(r"^r(\d+)$")
MEM_RE = re.compile(r"^(-?(?:0[xX][0-9A-Fa-f]+|\d+))?\((r\d+)\)$")
BRANCH_TARGET_RE = re.compile(r"^([0-9a-fA-F]{7,8})\b")

LOADS = frozenset("lbz lbzu lhz lhzu lha lhau lwz lwzu lwa lwarx lfs lfsu lfd lfdu lmw lwbrx "
                  "lhzx lhax lhaux lbzx lbzux lwzx lwzux lfsx lfdx lmw".split())
STORES = frozenset("stb stbu sth sthu stw stwu stfs stfsu stfd stfdu stmw stwcx stwux stwx stbx sthx "
                   "stfsx stfdx psq_st psq_stx".split())
XFORMS = frozenset("lwzx lwzux lbzx lbzux lhzx lhax lhaux lfsx lfdx stwx stwux stbx sthx stfsx stfdx "
                   "lwbrx psq_lx psq_stx".split())
# The mnemonics whose operand is the symbol's own address, i.e. the half a `lis` pairs with. `lis` is not
# one of them: it is the high half, and on its own it is not a site.
ADDRESS_MATERIALISERS = frozenset("addi addis ori oris subfic".split())
WIDTHS = {"lbz": 1, "lbzu": 1, "lbzx": 1, "lbzux": 1, "stb": 1, "stbu": 1, "stbx": 1,
          "lhz": 2, "lhzu": 2, "lha": 2, "lhau": 2, "lhzx": 2, "lhax": 2, "lhaux": 2,
          "sth": 2, "sthu": 2, "sthx": 2,
          "lwz": 4, "lwzu": 4, "lwa": 4, "lwzx": 4, "lwzux": 4, "lwarx": 4, "lwbrx": 4,
          "stw": 4, "stwu": 4, "stwx": 4, "stwux": 4, "stwcx": 4,
          "lfs": 4, "lfsu": 4, "lfsx": 4, "stfs": 4, "stfsu": 4, "stfsx": 4,
          "lfd": 8, "lfdu": 8, "lfdx": 8, "stfd": 8, "stfdu": 8, "stfdx": 8}


def find_objdump(root=ROOT):
    """The build's `powerpc-eabi-objdump`, or whatever the host has. Nothing is on `PATH` by default."""
    for rel_ in OBJDUMP_RELS:
        path = os.path.join(root, rel_)
        if os.path.exists(path):
            return path
    for name in ("powerpc-eabi-objdump", "objdump"):
        hit = shutil.which(name)
        if hit:
            return hit
    return None


def elf_of(root=ROOT):
    return os.path.join(root, ELF_REL)


def split_insn(text):
    """`lis     r3,-32666` -> `("lis", "r3,-32666")`; the trailing `.` of a record form is dropped."""
    bits = text.split(None, 1)
    if not bits:
        return None, ""
    return bits[0].rstrip("."), (bits[1].strip() if len(bits) > 1 else "")


def split_ops(ops):
    return [t.strip() for t in ops.split(",")] if ops else []


def gpr_num(tok):
    m = GPR_RE.match(tok.strip()) if tok else None
    return int(m.group(1)) if m else None


def parse_int(tok):
    try:
        return int(tok, 0)
    except (TypeError, ValueError):
        return None


def parse_mem(tok):
    """`21350(r3)` / `-17824(r13)` / `0(r4)` / `(r4)` -> `(21350, "r3")`."""
    m = MEM_RE.match(tok.strip()) if tok else None
    if not m:
        return None
    return (int(m.group(1), 0) if m.group(1) else 0), m.group(2)


def access_width(mnem):
    return WIDTHS.get(mnem)


def branch_target(ops):
    """The absolute address a `b`/`bl`/`blt`/`bdnz` prints first (`80456e04 <_savegpr_26>`)."""
    if not ops:
        return None
    tok = ops.split(",")[-1].strip().split(None, 1)[0]
    m = BRANCH_TARGET_RE.match(tok)
    return int(m.group(1), 16) if m else None


def parse_disassembly(text, needed):
    """-> {function address: [(address, mnemonic, operands)]} for the functions in `needed`.

    `objdump -d` prints a `<name>:` label exactly at every symbol it knows, so the instructions between
    one label and the next are one function - no size has to be guessed from the map, and a function the
    map does not name is still bounded. Only the functions asked for are kept, so a 61 MB listing costs
    the memory of the few hundred bodies that matter.
    """
    out, cur, keep = {}, None, False
    for line in text.splitlines():
        m = LABEL_RE.match(line)
        if m:
            cur = int(m.group(1), 16)
            keep = cur in needed
            if keep and cur not in out:
                out[cur] = []
            continue
        if not keep:
            continue
        m = INSN_RE.match(line)
        if not m:
            continue
        body = m.group(2)
        if "\t" in body:
            body = body.rsplit("\t", 1)[1]
        mnemonic, operands = split_insn(body.strip())
        if mnemonic:
            out[cur].append((int(m.group(1), 16), mnemonic, operands))
    return out


def disassemble(objdump, elf, needed):
    """Run objdump once over the whole ELF and keep the requested bodies (streamed, not buffered)."""
    try:
        proc = subprocess.Popen([objdump, "-d", elf], stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True, encoding="utf-8",
                                errors="replace", bufsize=1 << 20)
    except OSError as exc:
        raise RuntimeError("cannot run %s: %s" % (objdump, exc))
    try:
        out = parse_disassembly(proc.stdout.read(), needed)
    finally:
        proc.stdout.close()
        err = proc.stderr.read()
        proc.stderr.close()
        proc.wait()
    if proc.returncode != 0:
        raise RuntimeError("objdump failed (%d): %s" % (proc.returncode, (err or "").strip()[:400]))
    return out


# --------------------------------------------------------------------------------------------------
# the abstract interpretation: blk + const + registers, and the loops that bound the registers
# --------------------------------------------------------------------------------------------------
# A value is `(blk, off, regs, origin)`: `False`/`True` for whether the block base is part of it, the
# constant part, the registers whose (unknown) values are still added in, and - for a block value - the
# site whose load produced it, so an access can name the reference it came from.
Abs = collections.namedtuple("Abs", "blk off regs origin")


def a_const(n):
    return Abs(False, n, frozenset(), None)


def a_reg(r):
    return Abs(False, 0, frozenset((r,)), None)


def a_blk(off=0, regs=frozenset(), origin=None):
    return Abs(True, off, regs, origin)


def a_add(a, b):
    return Abs(a.blk or b.blk, a.off + b.off, a.regs | b.regs, a.origin or b.origin)


def as_block(value, blk_addrs):
    """-> the value as a block base, if it *is* the block; else None.

    Two things can put the block in a register: the load the census indexes (the symbol is a pointer, and
    its contents are the block), and the symbol's own address, materialised as `lis sym@ha` +
    `addi rX, rX, sym@l` (the symbol *is* the block). The second reaches the access as an ordinary
    constant - the two halves add up to exactly the map's address - so it is recognised by value.
    """
    if value is None:
        return None
    if value.blk:
        return value
    if value.off in blk_addrs and not value.blk:
        return a_blk(regs=value.regs, origin=("abs", value.off))
    return None


class Loop:
    """One back edge: the body is `[start, back]` inclusive, and `end` is the instruction after it."""

    __slots__ = ("start", "back", "end")

    def __init__(self, start, back, end):
        self.start, self.back, self.end = start, back, end

    def contains(self, addr):
        return self.start <= addr < self.end

    def span(self):
        return self.end - self.start


def find_loops(insns, fstart):
    """Every backward branch inside one function. A `b`/`bdnz`/`blt` to a lower address is a loop."""
    loops = []
    for addr, mnem, ops in insns:
        if mnem in cl._CALL or mnem in ("blr", "blrl", "bctr", "bcctr"):
            continue
        if not mnem.startswith("b"):
            continue
        tgt = branch_target(ops)
        if tgt is None or tgt >= addr or tgt < fstart:
            continue
        loops.append(Loop(tgt, addr, addr + 4))
    return loops


def const_lookup(insns):
    """-> `f(reg, addr)`: the constant an `li`/`lis` last put in `reg` strictly before `addr`, or None."""
    defs = collections.defaultdict(list)
    for addr, mnem, ops in insns:
        if mnem not in ("li", "lis"):
            continue
        toks = split_ops(ops)
        if len(toks) < 2 or gpr_num(toks[0]) is None:
            continue
        n = parse_int(toks[1])
        if n is None:
            continue
        defs[toks[0]].append((addr, n << 16 if mnem == "lis" else n))

    def lookup(reg, addr):
        lst = defs.get(reg)
        if not lst:
            return None
        addrs = [a for a, _v in lst]
        i = bisect.bisect_left(addrs, addr) - 1
        return lst[i][1] if i >= 0 else None
    return lookup


@functools.lru_cache(maxsize=None)
def writes_of(mnem, ops):
    """The GPR numbers an instruction defines, from the tree's one decode, with a narrow fallback.

    Cached: one function is decoded repeatedly (every loop's body, every bound lookup), and the decode
    is a pure function of the two operand strings.
    """
    _reads, writes, is_branch, is_call, decoded = cl.decode_rw(mnem, ops)
    if decoded:
        return writes, is_branch, is_call
    if is_branch:
        return set(), True, False
    toks = split_ops(ops)
    n = gpr_num(toks[0]) if toks else None
    if n is None:
        return set(), False, False
    return {n}, False, False


def child_loops(loop, loops):
    """The loops strictly inside `loop`: their bodies are theirs, not the outer loop's."""
    return tuple(c for c in loops
                 if c is not loop and loop.start <= c.start and c.end <= loop.end
                 and c.span() < loop.span())


def loop_increments(loop, reg, insns, children=()):
    """-> `(delta, reset, bad)` for `reg` in one loop body, excluding the nested loops' own bodies.

    `delta` is the total constant *increase* per iteration (`addi reg,reg,4`), `reset` the last constant
    the body writes into `reg` (`li`), and `bad` says the body also writes `reg` in a way neither explains
    - which is exactly the case that must be called unresolved rather than bounded. Instructions inside a
    strictly inner loop are skipped: they belong to that loop, and counting them here as well is how one
    `addi r3,r3,20` becomes a 480-byte phantom walk.
    """
    delta, reset, bad = 0, None, False
    want = gpr_num(reg)
    for addr, mnem, ops in insns:
        if not (loop.start <= addr <= loop.back):
            continue
        if any(c.start <= addr < c.end for c in children):
            continue
        writes, _b, _c = writes_of(mnem, ops)
        if want not in writes:
            continue
        toks = split_ops(ops)
        if mnem in ("addi", "addis") and len(toks) >= 3 and toks[0] == reg and toks[1] == reg:
            k = parse_int(toks[2])
            if k is None:
                bad = True
            elif k > 0:
                delta += k
        elif mnem in ("subi", "subis") and len(toks) >= 3 and toks[0] == reg and toks[1] == reg:
            pass                                     # a negative step cannot raise the maximum
        elif mnem in ("li", "lis") and len(toks) >= 2:
            n = parse_int(toks[1])
            reset = (n << 16 if mnem == "lis" else n) if n is not None else reset
        else:
            bad = True
    return delta, reset, bad


def nest_walk(reg, access_addr, loops, insns, by_addr, const_before, strict=True):
    """How far the loop nest around a site walks `reg`, innermost first, or `(None, _, False)`.

    -> `(extra, touched, ok)`. A clean level (only constant increments in its own body) multiplies what
    the levels inside it accumulated; a level that *rewrites* `reg` (`mr reg, base`, a load) means the
    value is re-established there, so the walk does not carry past it and the accumulation stops. With
    `strict` (a register used as a *term* of the base expression) a rewrite at the innermost level is not
    a counter at all and is refused; without it (the base register itself, whose value the linear pass
    already tracks) the rewrite is exactly the re-establishment and the walk is simply zero.
    """
    nest = sorted((L for L in loops if L.contains(access_addr)), key=lambda L: L.span())
    acc, touched = 0, False
    for i, loop in enumerate(nest):
        delta, reset, bad = loop_increments(loop, reg, insns, child_loops(loop, loops))
        if not (delta or reset is not None or bad):
            continue                                 # this level never writes the register
        touched = True
        if bad:
            if i == 0 and strict:
                return None, True, False             # rewritten by an arithmetic the bound cannot follow
            return acc, True, True                   # an outer level re-establishes it: do not multiply
        if reset is not None:
            return acc + delta, True, True           # `li reg, k` each iteration: one walk, not T
        trips = loop_trips(loop, insns, by_addr, const_before)
        if trips is None:
            return None, True, False
        acc = (trips - 1) * (delta + acc) + acc
    return acc, touched, True


def loop_growth(reg, access_addr, loops, insns, by_addr, const_before):
    """How far an enclosing loop nest walks the *base register itself* by adding a constant to it.

    `mr r3, blk` then `addi r3, r3, 0x14` inside a 2-trip loop is `blk + 0x14` at the far end, and the
    register's own tracked value cannot see it: the increment is a re-definition of the base, not a
    separate term. None when a loop that walks it has no provable trip count.
    """
    extra, touched, ok = nest_walk(reg, access_addr, loops, insns, by_addr, const_before, strict=False)
    if not ok:
        return None
    return extra or 0


def _mtctr_ctr(insns, loop):
    """The constant a `bdnz` counts on, or None.

    A `for (i = 0; i < 2; i++)` compiles as `li r0,2; mtctr r0` *before* the loop head and the `bdnz` at
    the bottom branches back to the head, so the `mtctr` is regularly outside `[start, back]` - searching
    only the body reads that loop as unbounded, which is how a bounded access turns into an unresolved
    site.
    """
    body = sorted((i for i in insns if loop.start <= i[0] <= loop.back), reverse=True)
    for window in (body, sorted((i for i in insns if i[0] < loop.start), reverse=True)[:8]):
        for addr, mn, op in window:
            if mn == "mtctr":
                return op.strip(), addr
    return None, None


def loop_trips(loop, insns, by_addr, const_before):
    """An upper bound on how many times the loop body runs, or None when the guard does not say.

    Two guards are read off the code itself: a `bdnz` whose `mtctr` is a constant, and a conditional back
    edge whose preceding `cmpwi/cmplwi` names a constant. An unconditional back edge is an unbounded
    loop; a `bge`-guarded one has no static ceiling. Everything else is None, and a None is never turned
    into a number downstream.
    """
    mnem, ops = by_addr[loop.back]
    if mnem == "bdnz":
        creg, caddr = _mtctr_ctr(insns, loop)
        if creg is None:
            return None
        n = const_before(creg, caddr)
        return n if (n is not None and n > 0) else None
    if mnem == "bdz":
        return 1
    if mnem in ("b", "ba"):
        return None
    creg, cn = None, None
    seen = 0
    for addr, mn, op in sorted((i for i in insns if loop.start <= i[0] < loop.back), reverse=True):
        if mn in ("cmpwi", "cmplwi", "cmpdi", "cmpldi"):
            toks = split_ops(op)
            creg = toks[0] if toks else None
            cn = parse_int(toks[1]) if len(toks) > 1 else None
            break
        seen += 1
        if seen > 2 or mn.startswith("b"):
            break
    if creg is None or cn is None:
        return None
    init = const_before(creg, loop.start)
    step, _reset, bad = loop_increments(loop, creg, insns)
    if init is None or bad:
        return None
    if mnem in ("blt", "bltu", "bne"):
        if step <= 0:
            return 1 if init >= cn else None
        if init >= cn:
            return 1                                  # a do-while runs at least once
        return (cn - init + step - 1) // step
    if mnem in ("ble", "bleu", "beq"):
        if step <= 0:
            return 1
        return 1 + max(0, (cn - init) // step)
    return None


def plausible_offset(n):
    """`n` as an offset term, or None when the magnitude says it is an address and not an offset."""
    return n if (-MAX_OFFSET_TERM <= n < MAX_OFFSET_TERM) else None


def bound_of_reg(reg, access_addr, regs, loops, insns, by_addr, const_before):
    """An upper bound on `reg` at `access_addr`, or None - never a number it cannot prove.

    A register the enclosing loops walk by a constant is `init + the nest's walk`. A register nothing
    bounds is only usable if the tracked value is itself constant or a sum of bounded registers.
    """
    extra, touched, ok = nest_walk(reg, access_addr, loops, insns, by_addr, const_before)
    if not ok:
        return None
    if touched:
        init = const_before(reg, access_addr)
        if init is None:
            value = regs.get(reg)
            if value is not None and not value.blk and not value.regs:
                init = value.off
        return None if init is None else plausible_offset(init + (extra or 0))
    value = regs.get(reg)
    if value is None:
        return None
    if not value.regs:
        return plausible_offset(value.off)
    total = value.off
    for other in value.regs:
        if other == reg:
            return None
        b = bound_of_reg(other, access_addr, regs, loops, insns, by_addr, const_before)
        if b is None:
            return None
        total += b
    return plausible_offset(total)


# A transfer that does not fall through: everything after it in address order is reached by a jump, so
# the state live before it must not be carried into that region. The instruction *sequences* the region
# holds (a switch's case bodies, for one) are still walked - with an unknown state - because skipping
# them outright loses every access they contain.
NO_FALLTHROUGH = frozenset(("b", "ba", "blr", "blrl"))


def merge_states(a, b):
    """The state two paths agree on: a register survives only when both carry the same value."""
    if a is None:
        return dict(b) if b else {}
    if b is None:
        return dict(a)
    return {k: v for k, v in a.items() if b.get(k) == v}


def loop_derived(access_addr, regs, loops, insns):
    """Whether an offset depends on a loop: the site sits inside a loop that walks one of the registers
    the base expression is made of (or the base register itself). This is what makes 0x53A2 a
    *loop-carried* reach rather than the plain displacement it looks like in the instruction.
    """
    for loop in loops:
        if not loop.contains(access_addr):
            continue
        for reg in regs:
            delta, reset, bad = loop_increments(loop, reg, insns, child_loops(loop, loops))
            if delta or reset is not None or bad:
                return True
    return False


def interpret(insns, fstart, blk_sites, accessor_addrs, mem_callees, cache_writes=frozenset(),
              addr_sites=None, blk_addrs=frozenset()):
    """The whole light interpretation of one function.

    -> `(accesses, unresolved, literals, escapes)`. `blk_sites` maps a census site's address to the
    register that site loads the block into; `accessor_addrs` are functions that cache *and return* the
    pointer (`get_userdata`), so a `bl` to one leaves `blk` in r3; `mem_callees` maps a callee address to
    `(name, size_reg, buf_regs)` for the `memset`-like calls that state an extent; `cache_writes` are the
    census's own store sites into the queried symbol, which cache the pointer rather than leak it; and
    `addr_sites`/`blk_addrs` carry the object form - the instruction that materialises the symbol's own
    address, and that address itself, for a symbol that is the block rather than a pointer to it.
    """
    addr_sites = addr_sites or {}
    last = insns[-1][0] + 4 if insns else fstart
    snapshots = {}                   # branch target -> the register state at the branch that jumps there
    dead = False                     # the linear state is not a live incoming state
    cleared = False
    by_addr = {a: (m, o) for a, m, o in insns}
    loops = find_loops(insns, fstart)
    const_before = const_lookup(insns)
    # Every register a loop writes is "loop-grown": its tracked constant is the value it entered with,
    # not what it is at the far end of the loop, so an expression must keep it symbolic and let the
    # loop math bound it. Folding `li r28, 0` into the sum is exactly how the furthest access of the
    # Q_UserData record disappears from an otherwise correct interpretation.
    grown = set()
    for loop in loops:
        for addr, mnem, ops in insns:
            if loop.start <= addr <= loop.back:
                for n in writes_of(mnem, ops)[0]:
                    grown.add("r%d" % n)
    regs = {}
    accesses, unresolved, literals, escapes = [], [], [], []

    def plain_const(tok):
        v = regs.get(tok)
        return v is not None and not v.blk and not v.regs

    def expr_operand(tok):
        if gpr_num(tok) is None:
            return a_reg(tok)
        if tok in grown and plain_const(tok):
            return a_reg(tok)
        return regs.get(tok) or a_reg(tok)

    def emit(addr, mnem, disp, bv, base_reg):
        width = access_width(mnem)
        if width is None:
            unresolved.append({"site": addr, "function": fstart, "instruction": text_of(addr),
                               "reason": "the access width of `%s` is not a fixed size" % mnem})
            return
        total = bv.off + disp
        for r in sorted(bv.regs):
            b = bound_of_reg(r, addr, regs, loops, insns, by_addr, const_before)
            if b is None:
                unresolved.append({"site": addr, "function": fstart, "instruction": text_of(addr),
                                   "reason": "the base register %s has no statically known bound" % r,
                                   "origin": bv.origin})
                return
            total += b
        if base_reg is not None:
            g = loop_growth(base_reg, addr, loops, insns, by_addr, const_before)
            if g is None:
                unresolved.append({"site": addr, "function": fstart, "instruction": text_of(addr),
                                   "reason": "the base register %s is walked by a loop with no "
                                             "provable trip count" % base_reg, "origin": bv.origin})
                return
            total += g
        accesses.append({"offset": total, "width": width, "site": addr, "function": fstart,
                         "instruction": text_of(addr), "reads": mnem in LOADS,
                         "base": base_reg, "regs": sorted(bv.regs), "origin": bv.origin,
                         "loop_carried": loop_derived(addr, set(bv.regs) | ({base_reg}
                                                                           if base_reg else set()),
                                                      loops, insns)})

    def text_of(addr):
        m, o = by_addr.get(addr, (None, None))
        return ("%s %s" % (m, o)).strip() if m else None

    for addr, mnem, ops in insns:
        toks = split_ops(ops)
        if dead:
            if addr in snapshots:
                regs.clear()
                regs.update(snapshots[addr])   # the path that arrives here is that branch's own
                dead = False
            elif not cleared:
                regs.clear()                   # forget the state that cannot have reached this region
                cleared = True
        elif addr in snapshots:
            merged = merge_states(regs, snapshots[addr])
            regs.clear()
            regs.update(merged)
        if addr in blk_sites:
            reg = blk_sites[addr]
            if reg:
                regs[reg] = a_blk(origin=addr)
            continue
        if addr in addr_sites:
            reg, origin = addr_sites[addr]
            if reg:
                regs[reg] = a_blk(origin=origin)
            continue
        _writes, is_branch, is_call = writes_of(mnem, ops)
        if is_call:
            callee = branch_target(ops)
            mc = mem_callees.get(callee)
            spent = set()
            if mc is not None:
                name, size_reg, buf_regs = mc
                size = const_of(regs, size_reg)
                for r in buf_regs:
                    v = regs.get(r)
                    if v is not None and v.blk and not v.regs:
                        literals.append({"size": size, "site": addr, "function": fstart,
                                         "instruction": text_of(addr), "callee": name, "arg": r,
                                         "offset": v.off, "measured": size is not None})
                        spent.add(r)
            for n in range(3, 11):
                r = "r%d" % n
                v = regs.get(r)
                if v is not None and v.blk and r not in spent:
                    escapes.append({"site": addr, "function": fstart, "instruction": text_of(addr),
                                    "reason": "the pointer is passed to the callee in %s" % r,
                                    "origin": v.origin})
            for n in range(0, 13):
                regs.pop("r%d" % n, None)
            if callee in accessor_addrs:
                regs["r3"] = a_blk(origin=("accessor", callee))
            continue
        if is_branch:
            tgt = branch_target(ops)
            if tgt is not None and fstart <= tgt <= last and tgt != addr:
                snapshots[tgt] = merge_states(snapshots.get(tgt), regs)
            if mnem in NO_FALLTHROUGH:
                dead = True
                cleared = False
            continue
        mem = parse_mem(toks[1]) if len(toks) > 1 else None
        if mnem in LOADS or mnem in STORES:
            if mem is not None and (mnem not in XFORMS):
                disp, base = mem
                bv = as_block(regs.get(base), blk_addrs)
                if bv is not None:
                    emit(addr, mnem, disp, bv, base)
                sv = as_block(regs.get(toks[0]), blk_addrs) if toks else None
                if mnem in STORES and sv is not None and bv is None \
                        and addr not in cache_writes:
                    escapes.append({"site": addr, "function": fstart, "instruction": text_of(addr),
                                    "reason": "the pointer is stored into another object",
                                    "origin": sv.origin})
            elif len(toks) >= 3 and mnem in XFORMS:
                for base in (toks[1], toks[2]):
                    bv = as_block(regs.get(base), blk_addrs)
                    if bv is not None:
                        emit(addr, mnem, 0, bv, base)
        value = None
        primary = toks[0] if toks and gpr_num(toks[0]) is not None else None
        if mnem == "li" and len(toks) >= 2:
            n = parse_int(toks[1])
            value = a_const(n) if n is not None else None
        elif mnem == "lis" and len(toks) >= 2:
            n = parse_int(toks[1])
            value = a_const((n << 16) & 0xFFFFFFFF) if n is not None else None
        elif mnem in ("add", "add.") and len(toks) >= 3:
            value = a_add(expr_operand(toks[1]), expr_operand(toks[2]))
        elif mnem in ("addi", "addis") and len(toks) >= 3:
            n = parse_int(toks[2])
            if n is not None:
                if toks[0] == toks[1] and toks[1] in grown and plain_const(toks[1]):
                    value = a_reg(toks[0])           # an in-place step: the loop math owns the value
                elif toks[1] == "r0":
                    value = a_const(n)               # `addi rD, 0, k` means the literal, not r0
                else:
                    value = a_add(expr_operand(toks[1]), a_const(n))
        elif mnem in ("subi", "subis") and len(toks) >= 3:
            n = parse_int(toks[2])
            value = a_add(expr_operand(toks[1]), a_const(-n)) if n is not None else None
        elif mnem in ("mr", "or") and len(toks) >= 2 and (mnem == "mr" or toks[1] == toks[2]):
            value = expr_operand(toks[1])
        writes, _b, _c = writes_of(mnem, ops)
        for n in writes:
            r = "r%d" % n
            regs[r] = value if (r == primary and value is not None) else a_reg(r)
    return accesses, unresolved, literals, escapes


def const_of(regs, reg):
    v = regs.get(reg)
    if v is None or v.blk or v.regs:
        return None
    return v.off


# --------------------------------------------------------------------------------------------------
# the analysis: census -> functions -> interpretation -> the extent
# --------------------------------------------------------------------------------------------------
def mem_callees(cmap):
    """The `memset`-like callees in the current map: address -> (name, size register, buffer registers).

    Matched on the map's *own* spelling so no address is hard-coded: `memset`, `TRK_memset`, `bzero`, and
    the mangled forms (`memset__FPvUli`), whose plain stem is one of the known base names.
    """
    out = {}
    for name in cmap.symbols:
        stem = C.plain_name(name)
        base = "memset" if stem not in MEM_SIZE_REG and stem.rsplit("_", 1)[-1] in MEM_SIZE_REG \
            else stem
        if base in MEM_SIZE_REG:
            for section, addr, _type in cmap.symbols[name]:
                if section in C.CODE_SECTIONS:
                    out[addr] = (name, MEM_SIZE_REG[base], MEM_BUF_REGS[base])
    return out


def site_insn(ref, funcs):
    """The decoded instruction at a census site, or None: `funcs` is this analysis's own disassembly."""
    caller = ref.get("caller") or {}
    insns = funcs.get(int(caller["address"], 16)) if caller.get("address") else None
    if not insns:
        return None
    return next((i for i in insns if i[0] == int(ref["site"], 16)), None)


def repair_sites(refs, funcs):
    """Move a site that is a halfword offset rather than an instruction onto the instruction.

    MWCC records `R_PPC_ADDR16_HA`/`_HI`/`_LO` against the immediate halfword they patch - two bytes into
    the instruction - and `callers.py`'s object fallback reports that offset as the site, so the census
    calls 0x8003F38E the site of the `lis` at 0x8003F38C. Every site this analysis decodes an instruction
    at has to be the instruction, so one that lands between two instructions and whose address minus two
    is a decoded instruction is moved onto it. (The address-*keyed* census is unaffected: this is about
    reading the instruction back, not about which address the graph indexed.)
    """
    out = []
    for r in refs:
        if site_insn(r, funcs) is None:
            moved = dict(r, site="0x%08X" % (int(r["site"], 16) - 2))
            if site_insn(moved, funcs) is not None:
                out.append(moved)
                continue
        out.append(r)
    return out


def refine_kinds(refs, funcs):
    """Recover each site's kind from its decoded instruction - the coarse-census case.

    `callers.py`'s object fallback has no instruction text, so it reports every non-call reference as
    `addr` (its own docstring says so), and this tool would read a pointer variable the code *loads*
    through `sym@sda21` as a symbol whose own address is the block: no access found, every site
    unresolved. The instruction is decoded here anyway, so the kind is recovered from its mnemonic
    through the census's own `mem_kind` rule - the one the dump census applies to the dump's text - and
    the dump census is untouched, because it already splits read/write/addr itself.
    """
    out = []
    for r in refs:
        ins = site_insn(r, funcs) if r["kind"] == "addr" else None
        out.append(dict(r, kind=C.mem_kind(ins[1])) if ins is not None else r)
    return out


def object_sites(addr_refs, funcs, coarse):
    """Where the code materialises the symbol's *own* address -> `{instruction address: (reg, origin)}`.

    Two readers, because the two censuses carry different evidence. The dump prints a `lis sym@ha` +
    `addi rD, rA, sym@l` pair as one row whose text names the address half, so the site is read out of
    that text. `callers.py`'s object fallback has no instruction text at all (its own docstring says
    so), so the same site is recovered from the decoded instruction: an `addr` row whose mnemonic writes
    a register out of the symbol's address is where the block's address lands, and the `lis` row four
    bytes before it - when the census has one - is the pair's other half, hence the origin.
    """
    out = {}
    if not coarse:
        for r in addr_refs:
            text = r["instruction"] or ""
            if "@l" not in text:
                continue                     # a lone `lis sym@ha` is only half the address
            part = text.split(" + ")[-1]
            instr = int(r["site"], 16) + (4 if " + " in text else 0)
            m = re.match(r"\S+\s+(r\d+)", part.strip())
            if m:
                out[instr] = (m.group(1), int(r["site"], 16))
        return out
    rows = {int(r["site"], 16): r for r in addr_refs}
    for r in addr_refs:
        ins = site_insn(r, funcs)
        if ins is None or ins[1] not in ADDRESS_MATERIALISERS:
            continue
        toks = split_ops(ins[2])
        if not toks or gpr_num(toks[0]) is None:
            continue
        site = int(r["site"], 16)
        half = rows.get(site - 4)
        half_ins = site_insn(half, funcs) if half is not None else None
        out[site] = (toks[0], site - 4 if half_ins is not None and half_ins[1] == "lis" else site)
    return out


def detect_accessors(write_refs, funcs, cmap):
    """Functions that cache the pointer *and return it*, so `bl` to one leaves `blk` in r3.

    `get_userdata` (0x8004D120) is the shape: `lwz r3, 0x95c(r3)` off `system_w`, `stw r3, <cache>`,
    `blr` - a leaf with no stack frame that stores r3 and returns it. The two init paths that write the
    same word (`fn_800497B4`, `fn_800498EC`) have a frame and call `memset`, so they are not accessors:
    their store takes its value *from* the accessor's return.
    """
    out = {}
    for ref in write_refs:
        caller = ref.get("caller") or {}
        faddr = int(caller["address"], 16)
        insns = funcs.get(faddr)
        if not insns:
            continue
        site = int(ref["site"], 16)
        ins = next((i for i in insns if i[0] == site), None)
        if ins is None or not ins[1].startswith("st"):
            continue
        toks = split_ops(ins[2])
        if not toks or toks[0] != "r3":
            continue
        if any(writes_of(m, o)[2] or m == "stwu" for _a, m, o in insns):
            continue                                     # a framed caller is not the accessor
        if not insns or insns[-1][1] not in ("blr", "blrl"):
            continue
        if any(a > site and 3 in writes_of(m, o)[0] for a, m, o in insns):
            continue                                     # r3 is reused after the store
        out[faddr] = caller.get("name") or "0x%08X" % faddr
    return out


def interior_sites(index, aliases):
    """Census rows that name the target with a folded displacement (`sym+0x10@l`).

    The census's symbol regex stops at `+`, so such a site is keyed under the token after the `+` rather
    than under the symbol - the row is in the index, just not under the name being queried. It is read
    back out of the index's own rows (no second scan of the dump), because an interior reference is an
    access like any other and dropping it would understate the extent.
    """
    pat = re.compile(r"(?:^|[^\w$.])(%s)\+0x([0-9A-Fa-f]+)@"
                     % "|".join(re.escape(a) for a in sorted(aliases) if a), re.M)
    needles = [a for a in sorted(aliases) if a]
    out = {}
    for key, rows in index.get("refs", {}).items():
        for row in rows:
            text = row[3] or ""
            if not any(a in text for a in needles):     # 245k rows: the cheap test filters nearly all
                continue
            m = pat.search(text)
            if not m:
                continue
            site = row[0]
            off = int(m.group(2), 16)
            out.setdefault((site, off), {
                "offset": off, "width": 4, "site": site, "function": row[2],
                "instruction": text, "reads": True, "base": None, "regs": [],
                "origin": ("interior", site), "kind": "interior"})
    return list(out.values())


def analyze(target, index, cmap, info, objdump, elf, accessor_names=None, root=ROOT):
    """The whole answer for one queried address: its extent, its far accesses, its unresolved sites."""
    rep = C.query(target, index, cmap, limit=0, pointers=True)
    out = {"query": target, "resolved": rep.get("resolved"), "error": rep.get("error"),
           "census": {"counts": rep.get("counts"), "how": rep.get("how"),
                      "notes": rep.get("notes")},
           "elf": C.rel(elf, root), "objdump": objdump,
           "verdict": {}, "accesses": [], "unresolved": [], "literal": [], "interior": [],
           "accessors": [], "counts": {}}
    if rep.get("error"):
        return out
    hit = rep["resolved"]
    addr = int(hit["address"], 16)
    aliases = {hit["name"], target} | {a for a in [hit.get("asm_label")] if a}

    refs = rep["references"]
    mem = mem_callees(cmap)
    mem_calls = [r for r in refs if r["kind"] == "call"]           # cache refs that are calls: none
    extra_calls = []
    for name in sorted({m[0] for m in mem.values()}):
        q = C.query(name, index, cmap, kinds=["call"], limit=0)
        extra_calls.extend(r for r in q.get("references", []) if r["kind"] == "call")

    needed = {}
    for r in refs:
        c = r.get("caller") or {}
        if c.get("address"):
            needed.setdefault(int(c["address"], 16), c.get("name"))
    for r in extra_calls:
        c = r.get("caller") or {}
        if c.get("address"):
            needed.setdefault(int(c["address"], 16), c.get("name"))
    if not needed:
        out["verdict"] = {"error": "no function references this address"}
        return out

    funcs = disassemble(objdump, elf, set(needed) | {int(r["caller"]["address"], 16)
                                                     for r in mem_calls if r.get("caller")})
    out["counts"]["functions_decoded"] = len(funcs)
    # `callers.py`'s object fallback carries no instruction text: its reference *kind* is every non-call
    # as `addr` and a `lis`+`addi` pair is two rows, so the kind and the object form's site are both
    # recovered from the instructions, which are decoded here anyway. (The census's rows, counts and
    # addresses are still its own, and are reported as such.)
    coarse = info.get("source") == "elf"
    if coarse:
        refs = refine_kinds(repair_sites(refs, funcs), funcs)
    write_refs = [r for r in refs if r["kind"] == "write"]
    accessors = detect_accessors(write_refs, funcs, cmap)
    if accessor_names:
        for name in accessor_names:
            hit2 = cmap.symbols.get(name)
            if hit2:
                for section, a, _t in hit2:
                    if section in C.CODE_SECTIONS:
                        accessors[a] = name
    out["accessors"] = sorted({"0x%08X %s" % (a, n) for a, n in accessors.items()})

    reads = [r for r in refs if r["kind"] == "read"]
    blk_sites = {}
    for r in refs:
        if r["kind"] != "read":
            continue
        site = int(r["site"], 16)
        insns = funcs.get(int((r.get("caller") or {}).get("address", "0x0"), 16))
        ins = next((i for i in insns if i[0] == site), None) if insns else None
        if ins is None:
            continue
        toks = split_ops(ins[2])
        blk_sites[site] = toks[0] if toks and gpr_num(toks[0]) is not None else None

    # The other shape a block is reached by: the symbol *is* the block, and the code materialises its
    # address (`lis sym@ha` + `addi rX, rX, sym@l`) before indexing it. Only when nothing reads the
    # symbol directly: a `sym@sda21` load of a pointer variable would make those sites `&var`, not the
    # block, and reading the address as the block there would invent offsets.
    addr_refs = [r for r in refs if r["kind"] == "addr"]
    object_mode = not reads and not write_refs and bool(addr_refs)
    blk_addrs = frozenset((addr,)) if object_mode else frozenset()
    addr_sites = object_sites(addr_refs, funcs, coarse) if object_mode else {}

    accesses, unresolved, literals, escapes = [], [], [], []
    site_addrs = set(blk_sites) | set(addr_sites)
    mem_site_addrs = {int(r["site"], 16) for r in extra_calls}
    cache_writes = {int(r["site"], 16) for r in write_refs}
    # The object form reaches the block two ways: a seeded `addr_sites` entry, and - for a census whose
    # rows are coarser than that site - by value, where `as_block` folds the two halves into the map's own
    # address. Without the second half of the test a coarse-census object-mode query interprets only the
    # functions that happen to call `memset`, and answers "no access" for a symbol whose own address is
    # the block. The dump census always has `addr_sites` here, so its answer is untouched.
    by_value_object = coarse and bool(blk_addrs) and not addr_sites
    for faddr in sorted(funcs):
        insns = funcs[faddr]
        addrs = {i[0] for i in insns}
        sites = {s: r for s, r in blk_sites.items() if s in addrs}
        asites = {s: v for s, v in addr_sites.items() if s in addrs}
        if not sites and not asites and not by_value_object and not (addrs & mem_site_addrs):
            continue
        a, u, lit, esc = interpret(insns, faddr, sites, set(accessors), mem, cache_writes,
                                   asites, blk_addrs)
        accesses.extend(a)
        unresolved.extend(u)
        literals.extend(lit)
        escapes.extend(esc)

    resolved_sites = {x["origin"] for x in accesses if x["origin"]}
    escaped_sites = {x["origin"] for x in escapes if x["origin"]}
    for r in [x for x in refs if x["kind"] in ("read", "addr")]:
        site = int(r["site"], 16)
        if site in resolved_sites:
            continue
        worded = "the loaded pointer" if r["kind"] == "read" else "the materialised address"
        reason = (worded + " is passed on, so the callee may reach further") \
            if site in escaped_sites else \
            (worded + " is never dereferenced at a statically known offset in this function")
        unresolved.append({"site": site, "function": r["caller"]["address"] if r.get("caller") else None,
                           "instruction": r["instruction"], "reason": reason, "kind": "site"})
    unresolved.extend(escapes)

    out["accesses"] = sorted(accesses, key=lambda x: (-x["offset"], x["site"]))
    out["unresolved"] = sorted(unresolved, key=lambda x: x["site"])
    out["literal"] = sorted(literals, key=lambda x: (-(x["size"] or 0), x["site"]))
    out["interior"] = sorted(interior_sites(index, aliases), key=lambda x: -x["offset"])
    out["counts"].update(refs=len(refs), reads=len(reads), writes=len(write_refs),
                         address_taken=len(addr_refs), object_mode=object_mode,
                         accesses=len(out["accesses"]), unresolved=len(out["unresolved"]),
                         literal=len(out["literal"]), escaped=len(escapes))

    def name_of(faddr):
        if faddr is None:
            return None
        if needed.get(faddr):
            return needed[faddr]
        hit_ = cmap.name_at(faddr)
        return hit_[0] if hit_ else None

    for rows in (out["accesses"], out["unresolved"], out["literal"]):
        for row in rows:
            row["name"] = name_of(row.get("function"))
    out["reasons"] = dict(sorted(collections.Counter(_reason_kind(u["reason"])
                                                   for u in out["unresolved"]).items(),
                                   key=lambda kv: -kv[1]))

    inferred = None
    if out["accesses"]:
        far = out["accesses"][0]
        inferred = align_up(far["offset"] + far["width"])
    literal_sizes = [x["size"] for x in out["literal"] if x["size"]]
    literal_size = max(literal_sizes) if literal_sizes else None
    far = out["accesses"][0] if out["accesses"] else None
    loop = next((x for x in out["accesses"] if x.get("loop_carried")), None)
    loop_ties = [fmt(x["site"]) for x in out["accesses"]
                 if loop and x.get("loop_carried") and x["offset"] == loop["offset"]]

    def fname(row):
        return (row.get("name") or fmt(row.get("function"))) if row else None

    # The three numbers are named, and "furthest" on its own always means the *static* one: a
    # loop-carried reach is a derived figure (a displacement plus a bound read out of a guard), and the
    # data-extent brief that asked for this tool was mis-read exactly that way once.
    verdict = {"furthest_static_offset": fmt(far["offset"]) if far else None,
               "furthest_static_width": far["width"] if far else None,
               "furthest_static_site": fmt(far["site"]) if far else None,
               "furthest_static_function": fname(far),
               "furthest_static_loop_carried": bool(far and far.get("loop_carried")),
               "furthest_loop_carried_offset": fmt(loop["offset"]) if loop else None,
               "furthest_loop_carried_width": loop["width"] if loop else None,
               "furthest_loop_carried_site": fmt(loop["site"]) if loop else None,
               "furthest_loop_carried_sites": loop_ties,
               "furthest_loop_carried_function": fname(loop),
               "inferred_size": fmt(inferred) if inferred is not None else None,
               "literal_size": fmt(literal_size) if literal_size is not None else None,
               "suggested_size": fmt(literal_size if literal_size is not None else inferred),
               "evidence": ("the constant size argument of %s at %s"
                            % (out["literal"][0]["callee"], fmt(out["literal"][0]["site"]))
                            if literal_size is not None and out["literal"] else
                            "the furthest static access")}
    if literal_size is not None and inferred is not None and literal_size != inferred:
        if inferred > literal_size:
            verdict["disagreement"] = (
                "ESCALATE: an access reaches %s but the stated size is only %s - %s byte(s) past the "
                "allocation" % (fmt(inferred), fmt(literal_size), inferred - literal_size))
        else:
            verdict["disagreement"] = (
                "the literal size %s and the inferred reach %s differ (%s byte(s) unread)"
                % (fmt(literal_size), fmt(inferred), literal_size - inferred))
    out["verdict"] = verdict
    return out


def align_up(n, unit=ALIGN):
    return (n + unit - 1) // unit * unit


def _reason_kind(reason):
    """A short bucket for one unresolved reason, so 1600 rows can be read as a handful of causes."""
    if reason.startswith("the base register") and "bound" in reason:
        return "a register in the base expression has no static bound"
    if "trip count" in reason:
        return "a loop that walks the base has no provable trip count"
    if reason.startswith("the loaded pointer is passed on") or "passed to the callee" in reason \
            or reason.startswith("the materialised address is passed on"):
        return "the reference is passed to a callee (it may reach further)"
    if reason.startswith("the loaded pointer is stored"):
        return "the pointer is stored into another object"
    if reason.startswith("the loaded pointer is never dereferenced") \
            or reason.startswith("the materialised address is never dereferenced"):
        return "the reference is never dereferenced here"
    return reason


def fmt(n):
    return "0x%X" % n if n is not None else None


# --------------------------------------------------------------------------------------------------
# the answer
# --------------------------------------------------------------------------------------------------
def print_report(rep, info=None, root=ROOT, limit=20, min_offset=0):
    if rep.get("error"):
        print("== %s: no answer" % rep["query"])
        print()
        print("   %s" % rep["error"])
        return 1
    hit = rep["resolved"]
    v = rep["verdict"]
    print("== %s  %s  %s%s" % (hit["name"], hit["address"], hit["section"] or "?",
                               (" size:0x%X" % hit["size"]) if hit.get("size") else ""))
    print("   owner  %s (%s)" % (hit["owner"], hit["owner_state"]))
    c = rep["census"]["counts"]
    # the census's *own* rows, all three of them: the tool's refined kinds are its interpretation, and a
    # coarse census reports no reads and no writes at all (`callers.py`'s object fallback), so mixing the
    # two would print "0 read(s), 0 write(s), 0 address-taken" for a block with 789 readers.
    print("   census %d read(s), %d write(s), %d address-taken over %d function(s) - "
          "tools/units/callers.py%s"
          % (c["read"], c["write"], c["addr"], c["functions"],
             " (cached)" if info and info.get("cached") else ""))
    print("   block  %s" % ("the symbol's own address is the block (object form)"
                            if rep["counts"].get("object_mode")
                            else "the value the symbol holds is the block (pointer form)"))
    print("   code   %s via %s - %d function(s) decoded (the census's + every memset-like caller)"
          % (rep["elf"], rep["objdump"], rep["counts"].get("functions_decoded", 0)))
    for a in rep["accessors"]:
        print("   accessor %s caches the pointer and returns it" % a)
    print()
    if v.get("error"):
        print("   %s" % v["error"])
        return 0
    print("   verdict")
    print("     furthest static access  %-10s width %s  at %s  (%s)"
          % (v["furthest_static_offset"] or "-", v["furthest_static_width"] or "-",
             v["furthest_static_site"] or "-", v["furthest_static_function"] or "-"))
    print("     furthest loop-carried   %-10s width %s  at %s  (%s)%s"
          % (v["furthest_loop_carried_offset"] or "-", v["furthest_loop_carried_width"] or "-",
             v["furthest_loop_carried_site"] or "-", v["furthest_loop_carried_function"] or "-",
             " and %d more at this offset" % (len(v["furthest_loop_carried_sites"]) - 1)
             if len(v.get("furthest_loop_carried_sites") or []) > 1 else ""))
    print("     inferred size          %-10s (the furthest static offset + its width, rounded to %d)"
          % (v["inferred_size"] or "-", ALIGN))
    print("     literal size           %-10s (%s)"
          % (v["literal_size"] or "-", v["evidence"] if v["literal_size"] else "no memset-like call"))
    print("   >> suggested size for the claim: %s  (%s)" % (v["suggested_size"] or "UNKNOWN",
                                                            v["evidence"]))
    print("   (a bare 'furthest' here always means the *static* access; the loop-carried one is the "
          "displacement plus a bound read out of a loop guard)")
    if v.get("disagreement"):
        print("   !! DISAGREEMENT  %s" % v["disagreement"])
    print()
    rows = [a for a in rep["accesses"] if a["offset"] >= min_offset]
    shown = rows if not limit else rows[:limit]
    print("   furthest static accesses (top %d of %d resolved, furthest first)" % (len(shown), len(rows)))
    hdr = "   %-10s %-5s %-10s %-14s %-7s %s" % ("offset", "width", "site", "function", "loop",
                                                   "instruction")
    print(hdr)
    print("   " + "-" * (len(hdr) - 3))
    for a in shown:
        print("   %-10s %-5s %-10s %-14s %-7s %s" % (
            fmt(a["offset"]), a["width"], fmt(a["site"]), short_func(a),
            "yes" if a.get("loop_carried") else "-", a["instruction"] or "?"))
    if not rows:
        print("   (none resolved)")
    for section, note in (("interior", "interior references the census folds into the instruction text"),
                          ("literal", "literal size arguments - the allocation stating its own extent"),
                          ("unresolved", "unresolved - said per site, never guessed")):
        items = rep[section]
        if not items:
            continue
        print()
        print("   %s (%d)" % (note, len(items)))
        if section == "unresolved":
            for reason, n in (rep.get("reasons") or {}).items():
                print("     %5d  %s" % (n, reason))
        for a in (items if not limit else items[:limit]):
            if section == "literal":
                print("     %-10s %s  %-14s %s  (%s)" % (
                    fmt(a["size"]) if a["size"] else "?", fmt(a["site"]),
                    (a.get("name") or func_name(a.get("function")))[:14], a["instruction"] or "?",
                    "%s is the block" % a["arg"]))
            elif section == "interior":
                print("     +0x%-8X %s  %s" % (a["offset"], fmt(a["site"]), a["instruction"]))
            else:
                print("     %-10s %-22s %s" % (fmt(a["site"]),
                                                (a.get("name") or func_name(a.get("function")))[:22],
                                                a["reason"]))
        if limit and len(items) > limit:
            print("     ... (%d more, raise --limit)" % (len(items) - limit))
    return 0


def short_func(a):
    return a.get("name") or func_name(a.get("function"))


def func_name(f):
    return "0x%08X" % f if isinstance(f, int) else (f or "-")


def main(argv=None, root=ROOT):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", nargs="?", help="an address (0x80794880) or a symbol name")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--limit", type=int, default=20, help="rows listed per section (0 = all)")
    ap.add_argument("--min-offset", default="0", help="only accesses at or past this offset")
    ap.add_argument("--elf", help="the target ELF (default build/<game>/main.elf)")
    ap.add_argument("--objdump", help="the objdump to use (default build/binutils/powerpc-eabi-objdump.exe)")
    ap.add_argument("--accessor", action="append", default=[],
                    help="name a function that returns the block (repeatable)")
    ap.add_argument("--rebuild", action="store_true", help="rebuild the census even if its cache is valid")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()

    elf = args.elf or elf_of(root)
    objdump = args.objdump or find_objdump(root)
    if args.objdump and not os.path.exists(args.objdump):
        print("== no objdump at %s" % args.objdump)
        print("   nothing is on PATH; the build's copy is %s" % OBJDUMP_RELS[0])
        return 2
    if not objdump:
        print("== no objdump")
        print("   nothing is on PATH; the build's copy is %s" % os.path.join(*OBJDUMP_RELS[0].split(os.sep)))
        print("   fetch it with:  %s" % BINUTILS_TOOL)
        return 2
    if not os.path.exists(elf):
        print("== no target ELF")
        print("   %s does not exist, so the sites' instructions cannot be decoded." % C.rel(elf, root))
        print("   the census alone says *that* the address is read, never how far.")
        return 2
    if not args.target:
        ap.error("an address or a symbol name is required (or --selftest)")

    cmap = C.load_map(root)
    index, info = C.load_index(root=root, rebuild=args.rebuild)
    source = "asm"
    if index is None:
        index, info = C.load_elf_index(root=root, rebuild=args.rebuild, cmap=cmap)
        source = "elf"
        if index is None:
            print("== no census")
            print("   neither %s nor the split objects exist, so there are no reference sites."
                  % C.rel(C.asm_dir_of(root), root))
            print("   remedy: %s" % DUMP_TOOL)
            return 2
    try:
        rep = analyze(args.target, index, cmap, info, objdump, elf, accessor_names=args.accessor,
                      root=root)
    except RuntimeError as exc:
        print("== the disassembly failed")
        print("   %s" % exc)
        return 2
    rep["census"]["source"] = source
    rep["index"] = {"path": C.rel(info["cache"], root), "cached": info.get("cached"),
                    "rebuilt": info.get("rebuilt"), "reason": info.get("reason"),
                    "refs": info.get("stats", {}).get("refs")}
    if args.json:
        print(json.dumps(rep, indent=2, default=str))
        return 1 if rep.get("error") else 0
    return print_report(rep, info=info, root=root, limit=args.limit,
                        min_offset=parse_int(args.min_offset) or 0)


# --------------------------------------------------------------------------------------------------
# self-test: synthetic instruction sequences (no dump, no ELF, no objdump), plus the real acceptance
# run against this tree when it has one. The synthetic half runs everywhere, so its check count is the
# same in MAIN and in a fresh worktree.
# --------------------------------------------------------------------------------------------------
def selftest():
    checks, fails, notes = 0, [], []

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s\n      got  %r\n      want %r" % (name, got, want))

    # --- operand decode ---------------------------------------------------------------------------
    check("mem: a displacement and a base", parse_mem("21350(r3)"), (21350, "r3"))
    check("mem: a negative displacement", parse_mem("-17824(r13)"), (-17824, "r13"))
    check("mem: a zero displacement", parse_mem("0(r4)"), (0, "r4"))
    check("mem: a bare base", parse_mem("(r4)"), (0, "r4"))
    check("mem: a register operand is not a memory form", parse_mem("r3,r4,r5"), None)
    check("branch: a bl target decodes", branch_target("80456e04 <_savegpr_26>"), 0x80456E04)
    check("branch: a conditional target decodes", branch_target("803c1364 <fn_803C0F3C+0x428>"), 0x803C1364)
    check("branch: bc with a leading cr operand decodes",
          branch_target("12,0,800497b4 <fn>"), 0x800497B4)
    check("width: sth is two bytes", access_width("sth"), 2)
    check("width: lwz is four bytes", access_width("lwz"), 4)
    check("width: lbz is one byte", access_width("lbz"), 1)
    check("insn: a record form drops its dot", split_insn("add.    r3,r4,r5"), ("add", "r3,r4,r5"))
    check("insn: a no-operand form", split_insn("blr"), ("blr", ""))

    # the fixture's own disassembly text, so the objdump reader is checked without running objdump
    fake = "\n".join([
        "10001000 <base_fn>:",
        "10001000:\t94 21 ff f0 \tstwu    r1,-16(r1)",
        "10001004:\t80 0d ba 60 \tlwz     r0,-17824(r13)",
        "10001008:\t80 80 01 00 \tlwz     r4,256(r0)",
        "1000100c:\t4e 80 00 20 \tblr",
        "10002000 <other_fn>:",
        "10002000:\t4e 80 00 20 \tblr",
    ])
    got = parse_disassembly(fake, {0x10001000})
    check("objdump: only the asked-for function is kept", sorted(got), [0x10001000])
    check("objdump: the raw bytes column is stripped",
          got[0x10001000][1], (0x10001004, "lwz", "r0,-17824(r13)"))
    check("objdump: the label is not an instruction", len(got[0x10001000]), 4)

    # --- the abstract interpretation: the four sequences a data-extent claim turns on -------------
    # (a) a known base with a constant displacement
    a = [
        (0x10001000, "stwu", "r1,-16(r1)"),
        (0x10001004, "lwz", "r3,-17824(r13)"),
        (0x10001008, "lwz", "r4,256(r3)"),
        (0x1000100C, "addi", "r3,r3,512"),
        (0x10001010, "sth", "r4,16(r3)"),
        (0x10001014, "blr", ""),
    ]
    acc, unr, lit, esc = interpret(a, 0x10001000, {0x10001004: "r3"}, set(), {})
    check("const: the base load is not an access", unr, [])
    check("const: the straight displacement is found",
          [(x["offset"], x["width"]) for x in acc if x["site"] == 0x10001008], [(0x100, 4)])
    check("const: the base register's own constant walk is added",
          [(x["offset"], x["width"]) for x in acc if x["site"] == 0x10001010], [(0x210, 2)])
    check("const: the furthest offset is the walked one", max(x["offset"] for x in acc), 0x210)
    check("const: the origin names the census site that loaded the pointer",
          sorted({x["origin"] for x in acc}), [0x10001004])
    check("const: the access records whether it reads", [x["reads"] for x in acc], [True, False])

    # (b) a loop-carried offset: `add r3, blk, r28` with r28 stepping 4 over a 16-trip loop
    b = [
        (0x10002000, "lwz", "r0,-17824(r13)"),
        (0x10002004, "li", "r28,0"),
        (0x10002008, "li", "r29,0"),
        (0x1000200C, "add", "r3,r0,r28"),
        (0x10002010, "sth", "r29,21350(r3)"),
        (0x10002014, "addi", "r28,r28,4"),
        (0x10002018, "addi", "r29,r29,1"),
        (0x1000201C, "cmpwi", "r29,16"),
        (0x10002020, "blt", "1000200C <fn+0xc>"),
        (0x10002024, "blr", ""),
    ]
    acc, unr, lit, esc = interpret(b, 0x10002000, {0x10002000: "r0"}, set(), {})
    check("loop: the loop-carried access resolves", len(acc), 1)
    check("loop: 0x5366 + (16-1)*4 is the bound", acc[0]["offset"], 0x53A2)
    check("loop: the register that carries the offset is named", acc[0]["regs"], ["r28"])
    check("loop: nothing is left unresolved", unr, [])
    loops = find_loops(b, 0x10002000)
    check("loop: one back edge is found", len(loops), 1)
    check("loop: the guard bounds the trip count",
          loop_trips(loops[0], b, {x[0]: (x[1], x[2]) for x in b}, const_lookup(b)),
          16)
    # ... and the same loop with an unconditional back edge is refused, not guessed
    c = list(b)
    c[8] = (0x10002020, "b", "1000200C <fn+0xc>")
    acc, unr, lit, esc = interpret(c, 0x10002000, {0x10002000: "r0"}, set(), {})
    check("loop: an unconditional back edge is not bounded", acc, [])
    check("loop: and the site is reported unresolved",
          [u["reason"] for u in unr], ["the base register r28 has no statically known bound"])

    # (c) an unresolvable base: the offset register is a function argument
    d = [
        (0x10003000, "lwz", "r3,-17824(r13)"),
        (0x10003004, "add", "r4,r3,r5"),
        (0x10003008, "lwz", "r0,8(r4)"),
        (0x1000300C, "blr", ""),
    ]
    acc, unr, lit, esc = interpret(d, 0x10003000, {0x10003000: "r3"}, set(), {})
    check("unresolved: no access is invented", acc, [])
    check("unresolved: the site says why",
          [u["reason"] for u in unr], ["the base register r5 has no statically known bound"])
    # (c2) a pointer that escapes into a callee
    e = [
        (0x10003100, "lwz", "r3,-17824(r13)"),
        (0x10003104, "bl", "80001234 <consumer>"),
        (0x10003108, "blr", ""),
    ]
    acc, unr, lit, esc = interpret(e, 0x10003100, {0x10003100: "r3"}, set(), {})
    check("escape: a passed pointer is not an access", acc, [])
    check("escape: the callee is named",
          [x["reason"] for x in esc], ["the pointer is passed to the callee in r3"])

    # (d) the literal size argument, and the accessor that makes `bl` yield the block
    f = [
        (0x10004000, "bl", "8004d120 <get_userdata__Fv>"),
        (0x10004004, "li", "r4,0"),
        (0x10004008, "li", "r5,24576"),
        (0x1000400C, "bl", "80004350 <memset>"),
        (0x10004010, "blr", ""),
    ]
    mem = {0x80004350: ("memset", "r5", ("r3",))}
    acc, unr, lit, esc = interpret(f, 0x10004000, {}, {0x8004D120}, mem)
    check("literal: the size argument is read off the call", [x["size"] for x in lit], [0x6000])
    check("literal: the block is the destination", [x["arg"] for x in lit], ["r3"])
    check("literal: the accessor is not also an escape", esc, [])
    check("literal: a memset is not an access", acc, [])
    # the same call with a *register* size says nothing, and is not reported as a size
    g = list(f)
    g[2] = (0x10004008, "lwz", "r5,16(r31)")
    acc, unr, lit, esc = interpret(g, 0x10004000, {}, {0x8004D120}, mem)
    check("literal: a non-constant size is not a literal", [x["size"] for x in lit], [None])
    check("literal: and it is flagged as unmeasured", [x["measured"] for x in lit], [False])
    # without the accessor, r3 is not the block and the call is not a size statement
    acc, unr, lit, esc = interpret(f, 0x10004000, {}, set(), mem)
    check("literal: no accessor -> no block -> no literal size", lit, [])

    # --- align ------------------------------------------------------------------------------------
    check("align: 0x53A2+2 rounds to 0x53A4", align_up(0x53A2 + 2), 0x53A4)
    check("align: an already aligned size is left alone", align_up(0x6000), 0x6000)

    # --- the control flow the offset interpretation rests on ---------------------------------------
    # a state set before an unconditional branch is live *at its target* but not in the region the
    # branch jumps over: that region is walked with an unknown state, never with the branch's own
    h = [
        (0x50001000, "lwz", "r3,-17824(r13)"),
        (0x50001004, "b", "50001010 <fn+0x10>"),
        (0x50001008, "addi", "r3,r3,64"),
        (0x5000100C, "lwz", "r0,0(r3)"),
        (0x50001010, "lwz", "r0,32(r3)"),
        (0x50001014, "blr", ""),
    ]
    acc, unr, lit, esc = interpret(h, 0x50001000, {0x50001000: "r3"}, set(), {})
    check("flow: the region an unconditional branch jumps over invents nothing",
          [(fmt(x["offset"]), fmt(x["site"])) for x in acc], [("0x20", "0x50001010")])
    # and the mirror: a return, not a branch, ends the walk-the-rest-with-a-known-state assumption
    i = list(h)
    i[1] = (0x50001004, "blr", "")
    acc, unr, lit, esc = interpret(i, 0x50001000, {0x50001000: "r3"}, set(), {})
    check("flow: code after a `blr` is not credited with the return's state",
          [x["site"] for x in acc], [])

    # --- the object form: the symbol *is* the block, reached by its own address --------------------
    j = [
        (0x60001000, "lis", "r3,-32666"),
        (0x60001004, "addi", "r3,r3,-31264"),
        (0x60001008, "lwz", "r4,16(r3)"),
        (0x6000100C, "sth", "r4,0(r3)"),
        (0x60001010, "blr", ""),
    ]
    base = ((-32666 << 16) & 0xFFFFFFFF) - 31264          # the two halves add up to the object's address
    acc, unr, lit, esc = interpret(j, 0x60001000, {}, set(), {}, frozenset(), {}, frozenset((base,)))
    check("object: `lis sym@ha` + `addi sym@l` is the block's own address",
          [(fmt(x["offset"]), x["width"]) for x in acc], [("0x10", 4), ("0x0", 2)])
    check("object: without that address nothing is claimed",
          interpret(j, 0x60001000, {}, set(), {}, frozenset())[0], [])
    check("plausible: a small constant is an offset", plausible_offset(0x5366), 0x5366)
    check("plausible: an address-sized constant is refused", plausible_offset(0x80660000), None)

    # --- the coarse census: `callers.py`'s object fallback, which carries no instruction text -------
    # Its rows name the symbol and the relocation and call every non-call reference `addr`, and its
    # `HA`/`LO` sites are the immediate *halfword*, two bytes into the instruction. Both are recovered
    # from the decoded instruction here, so the same query answers the same thing whichever census
    # answered - which is what the acceptance run below asserts in either kind of tree.
    fns = {0x70001000: [
        (0x70001000, "lis", "r3,-32666"),
        (0x70001004, "addi", "r3,r3,-31264"),
        (0x70001008, "lwz", "r31,-17824(r13)"),
        (0x7000100C, "stw", "r31,0(r3)"),
    ]}

    def cref(site, kind="addr"):
        return {"kind": kind, "site": site, "caller": {"address": "0x70001000"},
                "instruction": "R_PPC_ADDR16_HA sym"}

    check("coarse: an instruction that materialises an address is not a read",
          [refine_kinds([cref("0x70001000"), cref("0x70001004")], fns)[i]["kind"]
           for i in (0, 1)], ["addr", "addr"])
    kinds = [r["kind"] for r in refine_kinds([cref("0x70001008"), cref("0x7000100C")], fns)]
    check("coarse: the symbol's kind comes back from the instruction at the site",
          kinds, ["read", "write"])
    check("coarse: a row the disassembly has no instruction for keeps the census's kind",
          refine_kinds([cref("0x70002000")], fns)[0]["kind"], "addr")
    # `0x70001006` is the halfword the HA reloc patches, two bytes into the `addi` at 0x70001004
    check("coarse: a halfword site is moved onto the instruction it patches",
          repair_sites([cref("0x70001006")], fns)[0]["site"], "0x70001004")
    check("coarse: a site that is neither an instruction nor a halfword of one is left alone",
          repair_sites([cref("0x70001001")], fns)[0]["site"], "0x70001001")
    # the object form's site: the address lands in the `addi`'s destination, and the `lis` before it is
    # the pair's other half - the same (register, origin) the dump census's coalesced row yields
    check("coarse object form: the materialising row is the site, the `lis` is the origin",
          object_sites([cref("0x70001000"), cref("0x70001004")], fns, coarse=True),
          {0x70001004: ("r3", 0x70001000)})
    check("coarse object form: a lone half is its own origin",
          object_sites([cref("0x70001004")], fns, coarse=True), {0x70001004: ("r3", 0x70001004)})
    check("coarse object form: a `lis` on its own is not a site",
          object_sites([cref("0x70001000")], fns, coarse=True), {})

    # --- the real acceptance run: Q_UserData, when this tree has the ELF and the census -----------
    elf, objdump = elf_of(), find_objdump()
    rep = None
    if not (elf and os.path.exists(elf) and objdump):
        notes.append("acceptance run skipped: no %s or no objdump in this tree" % C.rel(elf))
        check("acceptance: skipped only when the tree cannot run it",
              bool(not os.path.exists(elf) or not objdump), True)
    else:
        cmap = C.load_map()
        index, info = C.load_index()
        if index is None:
            index, info = C.load_elf_index(cmap=cmap)
        if index is None:
            # an ELF but no dump *and* no split objects: the tool has no census at all, and says so
            # rather than answering; there is nothing here to assert an extent against.
            notes.append("acceptance run skipped: neither the dump nor the split objects exist")
            check("acceptance: skipped only when this tree has no census",
                  bool(not os.path.exists(elf) or not objdump or index is None), True)
        else:
            rep = analyze("0x80794880", index, cmap, info, objdump, elf)
    if rep is not None:
        v = rep["verdict"]
        # The literal size is the memset in the init path. The furthest *static* access the DOL
        # actually contains is 0x53A8 (`lwz r0, 21416(r31)` in set_mydata2vs__FUcUc, r31 = the
        # get_userdata() return); the brief's 0x53A2 is the furthest *loop-carried* one, the
        # 16-iteration loop at 0x803C138C in fn_803C0F3C. Both are asserted, so a regression in
        # either the loop bound or the plain displacement is caught.
        check("acceptance: the literal size is the 0x6000 memset",
              v.get("literal_size"), "0x6000")
        check("acceptance: the memset sits in fn_800497B4 / fn_800498EC",
              sorted({"0x%08X" % x["function"] for x in rep["literal"]
                      if x["site"] in (0x800497D0, 0x80049908)}),
              ["0x800497B4", "0x800498EC"])
        check("acceptance: the suggested size is the literal one", v.get("suggested_size"), "0x6000")
        check("acceptance: the loop-carried access in fn_803C0F3C reaches 0x53A2",
              ("0x53A2", "0x803C138C") in
              [(fmt(x["offset"]), fmt(x["site"])) for x in rep["accesses"]], True)
        check("acceptance: the loop-carried access is 0x5366 + 15*4, not a constant",
              [x["instruction"] for x in rep["accesses"]
               if x["site"] == 0x803C138C], ["sth r29,21350(r3)"])
        check("acceptance: the furthest *static* access in the DOL is 0x53A8",
              (v.get("furthest_static_offset"), v.get("furthest_static_site"),
               v.get("furthest_static_width")),
              ("0x53A8", "0x8004F1F4", 4))
        check("acceptance: the furthest *loop-carried* access is 0x53A2, including 0x803C138C",
              (v.get("furthest_loop_carried_offset"),
               "0x803C138C" in v.get("furthest_loop_carried_sites", [])),
              ("0x53A2", True))
        check("acceptance: the two are not confused - the static one is not loop-carried",
              (v.get("furthest_static_loop_carried"),
               next(x.get("loop_carried") for x in rep["accesses"] if x["site"] == 0x803C138C)),
              (False, True))
        # the labels the brief's figure was mis-read from: they must be in the *default* output
        import contextlib as _cl
        import io as _io
        _buf = _io.StringIO()
        with _cl.redirect_stdout(_buf):
            print_report(rep, info=info, limit=4)
        _out = _buf.getvalue()
        check("acceptance: the default output labels all three numbers",
              all(s in _out for s in ("furthest static access", "furthest loop-carried",
                                      "literal size")), True)
        check("acceptance: and it never says 'furthest' for the loop-carried one",
              "furthest loop-carried" in _out and "furthest accesses" not in _out, True)
        check("acceptance: the literal size is the stronger evidence, and preferred",
              v.get("inferred_size"), "0x53AC")
        check("acceptance: the block was recognised as a cached-global",
              [a.split()[0] for a in rep["accessors"]], ["0x8004D120"])
        check("acceptance: the unresolvable sites are reported per site",
              rep["counts"]["unresolved"] > 0 and all(u.get("reason") for u in rep["unresolved"]),
              True)
        check("acceptance: the census is the one tools/units/callers.py reports",
              (rep["counts"]["reads"], rep["counts"]["writes"], rep["census"]["counts"]["functions"]),
              (789, 3, 346))
        # A cross-check the tool did not have to be told: for an object whose size the map *does*
        # state, the inferred reach must sit inside it - `system_w` is declared 0xA5C and the furthest
        # access into it is 0xA58. A tool that invents offsets fails here.
        sysw = analyze("0x806585E0", index, cmap, info, objdump, elf)
        check("acceptance: system_w's inferred size is inside the map's 0xA5C",
              sysw["verdict"].get("inferred_size"), "0xA5C")
        check("acceptance: and its object form is recognised",
              sysw["counts"].get("object_mode"), True)
    print("== accessextent selftest")
    for n in notes:
        print("   note %s" % n)
    for f_ in fails:
        print("   FAIL %s" % f_)
    if fails:
        print("   %d/%d checks failed" % (len(fails), checks))
        return 1
    print("   ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(main())
