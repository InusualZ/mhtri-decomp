"""PowerPC (Gekko/Broadway) instruction decode and the retail-text reference scanner, once.
Spec: docs/tools/spec/lib-ppc.md. CLI: none (library)."""
from __future__ import annotations

import bisect
import collections
import re
import struct
from dataclasses import dataclass, field

# --- opcode tables --------------------------------------------------------------------------------------------

#: integer loads that write rD (lwz/lwzu/lbz/lbzu/lha/lhau/lhz/lhzu)
LOADS_INT = (32, 33, 34, 35, 40, 41, 42, 43)
#: integer stores (stw/stwu/stb/stbu/sth/sthu/stmw)
STORES = (36, 37, 38, 39, 44, 45, 47)
#: floating-point loads and stores (lfs/lfsu/lfd/lfdu/stfs/stfsu/stfd/stfdu)
FP_MEM = (48, 49, 50, 51, 52, 53, 54, 55)
#: opcodes that READ memory through `rA + d` (lwz/lbz/lhz/lha/lfs/lfd, their update forms, lmw, psq_l/psq_lu)
LOAD_OPS = (32, 33, 34, 35, 40, 41, 42, 43, 46, 48, 49, 50, 51, 56, 57)
#: opcodes that WRITE memory through `rA + d` (stw/stb/sth, stmw, stfs/stfd, their update forms, psq_st/psq_stu)
STORE_OPS = (36, 37, 38, 39, 44, 45, 47, 52, 53, 54, 55, 60, 61)
#: the update-form accesses: they write the effective address back to rA
UPDATE_OPS = (33, 35, 37, 39, 41, 43, 45, 49, 51, 53, 55)
#: access width in bytes per D-form opcode (lmw/stmw report one word)
WIDTH = {32: 4, 33: 4, 34: 1, 35: 1, 36: 4, 37: 4, 38: 1, 39: 1, 40: 2, 41: 2, 42: 2, 43: 2, 44: 2, 45: 2,
         46: 4, 47: 4, 48: 4, 49: 4, 50: 8, 51: 8, 52: 4, 53: 4, 54: 8, 55: 8, 56: 8, 57: 8, 60: 8, 61: 8}
#: registers a call clobbers (r0, r3..r12): an address formed there does not survive a `bl`
VOLATILE = frozenset([0] + list(range(3, 13)))
#: opcode 31 forms that write rD (arithmetic with the OE bit masked off, indexed loads, mfcr/mfspr/mftb) and rA
#: (logical, shifts, extends)
X_ARITH = frozenset([266, 10, 138, 234, 202, 40, 8, 136, 232, 200, 104, 235, 75, 11, 491, 459])
X_LOADS = frozenset([23, 55, 87, 119, 279, 311, 343, 375, 20, 19, 83, 339, 371, 533])
X_LOGIC = frozenset([28, 60, 444, 412, 124, 476, 316, 284, 24, 536, 792, 824, 954, 922, 26])
#: primary opcodes whose bit 0 is the Rc (record) bit for every instruction of the family
RECORD_OPCODES = frozenset({12, 13, 20, 21, 23, 31, 59, 63})
#: A-form float XO values (opcodes 59 single, 63 double)
FMA_XO = frozenset({28, 29, 30, 31})
FMUL_XO = frozenset({25})
FADD_XO = frozenset({21})
FSUB_XO = frozenset({20})
#: Gekko paired-single indexed loads/stores (`psq_lx`/`psq_stx`) share opcode 4 under XO 6
PSQ_INDEXED_XO = 6
#: narrowing stores (stb, stbu, sth, sthu): a `clrlwi`/`clrrwi` right before one is what the peephole folds away
NARROW_STORE_OPS = frozenset({38, 39, 44, 45})
#: a `lis` value stays live this many instructions in a volatile register (`scan_refs`)
LIS_WINDOW = 200
BLR = 0x4E800020
NOP = 0x60000000


# --- fields ---------------------------------------------------------------------------------------------------

def op(w: int) -> int: return w >> 26
def rt(w: int) -> int: return (w >> 21) & 31
def ra(w: int) -> int: return (w >> 16) & 31
def rb(w: int) -> int: return (w >> 11) & 31
def frc(w: int) -> int: return (w >> 6) & 31
def xo5(w: int) -> int: return (w >> 1) & 31
def xo10(w: int) -> int: return (w >> 1) & 0x3FF
def sh(w: int) -> int: return (w >> 11) & 31
def mb(w: int) -> int: return (w >> 6) & 31
def me(w: int) -> int: return (w >> 1) & 31
def ui(w: int) -> int: return w & 0xFFFF


def signed(value: int, bits: int) -> int:
    """`value` read as a `bits`-wide two's-complement number."""
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def si(w: int) -> int:
    """The signed 16-bit immediate / displacement."""
    return signed(w & 0xFFFF, 16)


def displacement(w: int) -> int:
    """The signed displacement of a D-form access: 12 bits for `psq_l`/`psq_lu`/`psq_st`/`psq_stu`, else 16."""
    if w >> 26 in (56, 57, 60, 61):
        return signed(w & 0xFFF, 12)
    return signed(w & 0xFFFF, 16)


def rlwinm_alias(w: int) -> str:
    """The objdump spelling of an `rlwinm` (opcode 21) form: `clrlwi`/`clrrwi`/`srwi`/`slwi`, else `rlwinm`."""
    if w >> 26 != 21:
        return "rlwinm"
    s, m, e = sh(w), mb(w), me(w)
    if s == 0 and e == 31 and m > 0:
        return "clrlwi"
    if s == 0 and m == 0 and e < 31:
        return "clrrwi"
    if e == 31 and m == 32 - s and s > 0:
        return "srwi"
    if m == 0 and s + e == 31 and s > 0:
        return "slwi"
    return "rlwinm"


def branch_target(address: int, word: int) -> int | None:
    """Where a relative `b`/`bl` (I-form, 26-bit LI) or `bc`/`bcl` (B-form, 16-bit BD) at `address` goes, or None
    for an absolute (`AA=1`) branch or a word that is not one: `NIA = CIA + EXTS(disp || 0b00)`."""
    o = word >> 26
    if word & 2 or o not in (16, 18):
        return None
    if o == 18:
        return (address + signed(word & 0x03FFFFFC, 26)) & 0xFFFFFFFF
    return (address + signed(word & 0xFFFC, 16)) & 0xFFFFFFFF


def decode_li(word: int) -> tuple[int, int] | None:
    """`(register, immediate)` when the word is `addi rD, 0, imm` (MWCC's `li`), else None."""
    if word >> 26 != 14 or ra(word) != 0:
        return None
    return rt(word), si(word)


def written_reg(w: int) -> int | None:
    """The general register an instruction WRITES, for the forms `scan_refs` does not track itself (None when it
    writes none or the form is not decoded): a `lis` value or a formed address in it is dead after the write."""
    o = w >> 26
    if o in (7, 8, 12, 13):
        return rt(w)
    if o in (20, 21, 23, 25, 26, 27, 28, 29):
        return ra(w)
    if o == 31:
        x = (w >> 1) & 0x3FF
        if (x & 0x1FF) in X_ARITH or x in X_LOADS:
            return rt(w)
        if x in X_LOGIC:
            return ra(w)
    return None


# --- the instruction value ------------------------------------------------------------------------------------

@dataclass(frozen=True, slots=True)
class Insn:
    """One decoded instruction word at `address`. `op` is stored (the hot field); every other field is a property
    of the word, so building one per word stays cheap."""
    address: int
    word: int
    op: int = field(init=False, repr=False, compare=False)

    def __post_init__(self) -> None:
        object.__setattr__(self, "op", self.word >> 26)

    @property
    def rt(self) -> int: return (self.word >> 21) & 31
    @property
    def rs(self) -> int: return (self.word >> 21) & 31
    @property
    def ra(self) -> int: return (self.word >> 16) & 31
    @property
    def rb(self) -> int: return (self.word >> 11) & 31
    @property
    def si(self) -> int: return si(self.word)
    @property
    def ui(self) -> int: return self.word & 0xFFFF
    @property
    def d(self) -> int: return displacement(self.word)
    @property
    def sh(self) -> int: return sh(self.word)
    @property
    def mb(self) -> int: return mb(self.word)
    @property
    def me(self) -> int: return me(self.word)

    def is_record(self) -> bool:
        return self.op in RECORD_OPCODES and (self.word & 1) == 1

    def is_fma(self) -> bool:
        return self.op in (59, 63) and xo5(self.word) in FMA_XO

    def is_fmul(self) -> bool:
        return self.op in (59, 63) and xo5(self.word) in FMUL_XO

    def is_faddsub(self) -> bool:
        return self.op in (59, 63) and xo5(self.word) in (FADD_XO | FSUB_XO)

    def is_psq_indexed(self) -> bool:
        return self.op == 4 and xo5(self.word) == PSQ_INDEXED_XO

    def rlwinm_alias(self) -> str:
        return rlwinm_alias(self.word)

    def is_clr_mask(self) -> bool:
        return rlwinm_alias(self.word) in ("clrlwi", "clrrwi")

    def is_shift(self) -> bool:
        return rlwinm_alias(self.word) in ("srwi", "slwi")

    def is_narrow_store(self) -> bool:
        return self.op in NARROW_STORE_OPS

    def is_branch(self) -> bool:
        return self.op in (16, 18, 19)

    def is_branch_link(self) -> bool:
        """A relative-or-absolute `bl` (I-form with LK); the only shape the infer fingerprints count."""
        return self.op == 18 and (self.word & 1) == 1

    def is_call(self) -> bool:
        """Any branch with LK set (`bl`, `bcl`, `bctrl`, `blrl`)."""
        return self.op in (16, 18, 19) and (self.word & 1) == 1

    @property
    def target(self) -> int | None:
        return branch_target(self.address, self.word)

    def is_lis(self) -> bool:
        return self.op == 15 and self.ra == 0

    def is_load(self) -> bool:
        return self.op in LOAD_OPS

    def is_store(self) -> bool:
        return self.op in STORE_OPS

    def d_form(self) -> bool:
        """A `rA + d` memory access (the opcodes `LOAD_OPS`/`STORE_OPS` name)."""
        return self.op in LOAD_OPS or self.op in STORE_OPS

    @property
    def width(self) -> int | None:
        return WIDTH.get(self.op)


def decode(word: int, address: int = 0) -> Insn:
    """The `Insn` for one big-endian instruction word."""
    return Insn(address, word)


def words(code: bytes) -> tuple[int, ...]:
    """The big-endian 32-bit words of `code` (a trailing partial word is dropped)."""
    n = len(code) // 4
    return struct.unpack(">%dI" % n, code[:n * 4])


# --- function-shape predicates (phantom) ----------------------------------------------------------------------

def looks_like_prologue(word: int | None) -> bool:
    """`stwu r1,-N(r1)` / `mflr rD` / `stmw` - the shapes a real function entry starts with."""
    if word is None:
        return False
    if word & 0xFFFF0000 == 0x94210000:
        return True
    if word >> 26 == 31 and (word >> 16) & 0x1F == 8 and word & 0x7FF == 0x2A6:
        return True
    return word >> 26 == 47


def is_dead_epilogue(code: bytes | None) -> bool:
    """A phantom's own shape: the bytes are exactly one `blr`."""
    return bool(code) and len(code) == 4 and struct.unpack(">I", code)[0] == BLR


# --- the reference scanners -----------------------------------------------------------------------------------

def find_sda_bases(code_words) -> tuple[int | None, int | None]:
    """`(r13, r2)`: the `lis rN` / `ori|addi rN, rN` pair the start-up code (`__init_registers`) forms.
    `code_words` is `[(address, word), ...]`."""
    out: dict[int, int | None] = {13: None, 2: None}
    lis: dict[int, int] = {}
    for _addr, w in code_words:
        o = w >> 26
        d, a = (w >> 21) & 31, (w >> 16) & 31
        if o == 15 and a == 0 and d in out:
            lis[d] = (w & 0xFFFF) << 16
        elif o == 24 and d in lis and a == d and out[d] is None:
            out[d] = lis[d] | (w & 0xFFFF)
        elif o == 14 and a in lis and d == a and out[d] is None:
            out[d] = (lis[d] + si(w)) & 0xFFFFFFFF
    return out[13], out[2]


def materialisations(code: bytes, start: int, window: int = 16) -> list[tuple[int, int]]:
    """`[(site, value)]` for every `lis rD, hi` + `addi|ori rD, rD, lo` pair at most `window` bytes apart.

    The narrow idiom (same register, short distance) the phantom scan counts as "an address taken"; the
    register-tracking reader with r13/r2 bases and loads is `scan_refs`."""
    out = []
    lis: dict[int, tuple[int, int]] = {}
    for i, w in enumerate(words(code)):
        address = start + i * 4
        o = w >> 26
        if o == 15:
            lis[(w >> 21) & 0x1F] = (address, w & 0xFFFF)
        elif o in (14, 24):
            d, a = (w >> 21) & 0x1F, (w >> 16) & 0x1F
            hit = lis.get(d)
            if hit and a == d and address - hit[0] <= window:
                value = hit[1] << 16
                value = value + (w & 0xFFFF) if o == 14 else value | (w & 0xFFFF)
                out.append((address, value))
        if lis:
            lis = {r: v for r, v in lis.items() if address - v[0] <= window}
    return out


def scan_refs(code, start, sda13, sda2, is_data, fn_starts=(), loads=None, stores=None, passes=None):
    """`{target_address: [site, ...]}` for every absolute/small-data address a function materialises or accesses.

    `code` is the big-endian bytes of a code range starting at `start`; `is_data(addr)` says whether a computed
    address is worth recording; a `lis` value is forgotten at a function start (`fn_starts`).  When `loads` is a
    dict, it receives `{address: [site, ...]}` for the accesses that READ the address (a load through r13/r2, a
    `lis` + load, a load through a register an `addi`/`ori` formed the address into); the `addi`/`ori` that only
    forms the address is a reference and NOT a read.  `stores` receives the accesses that WRITE it; `passes`
    receives `{address: [(site, register, callee)]}` for every `bl` made while a register of r3..r10 holds an
    address an `addi`/`ori` formed (`callee` the branch target, None for an indirect call).  Rules: spec.
    """
    refs = collections.defaultdict(list)
    ws = words(code)
    fs = set(fn_starts)
    lis = {}
    areg = {}                                          # register -> (address formed by addi/ori, index)

    def live_lis(r, i):
        e = lis.get(r)
        return e is not None and (r >= 14 or i - e[1] <= LIS_WINDOW)

    for i, w in enumerate(ws):
        site = start + i * 4
        if site in fs:
            lis.clear()
            areg.clear()
        o = w >> 26
        if o == 18 or o == 19:                         # b / bl / blr / bctr / bctrl / bclr...: control leaves
            if w & 1:                                  # a call clobbers the volatile registers
                if passes is not None:
                    callee = None
                    if o == 18 and not w & 2:
                        callee = (site + signed(w & 0x03FFFFFC, 26)) & 0xFFFFFFFF
                    for r in range(3, 11):
                        if r in areg:
                            passes.setdefault(areg[r][0], []).append((site, r, callee))
                for r in VOLATILE:
                    areg.pop(r, None)
                    lis.pop(r, None)
            elif o == 18 or (w >> 21) & 31 == 20:      # an unconditional jump or return ends the path
                if passes is not None and o == 18 and not w & 2:      # a tail call passes what the registers hold
                    callee = (site + signed(w & 0x03FFFFFC, 26)) & 0xFFFFFFFF
                    if callee in fs:
                        for r in range(3, 11):
                            if r in areg:
                                passes.setdefault(areg[r][0], []).append((site, r, callee))
                areg.clear()
            continue
        if o == 15:                                    # addis rD,rA,SIMM  (lis when rA == 0)
            d, a = (w >> 21) & 31, (w >> 16) & 31
            areg.pop(d, None)
            if a == 0:
                lis[d] = ((w & 0xFFFF) << 16, i)
            else:
                lis.pop(d, None)
            continue
        if o == 24:                                    # ori rA,rS,UI : base is rS, result goes to rA
            s, a = (w >> 21) & 31, (w >> 16) & 31
            areg.pop(a, None)
            if live_lis(s, i):
                t = lis[s][0] | (w & 0xFFFF)
                if is_data(t):
                    refs[t].append(site)
                areg[a] = (t, i)
            if a != s:
                lis.pop(a, None)
            continue
        if o == 14 or 32 <= o <= 57 or o in (60, 61):
            d, a = (w >> 21) & 31, (w >> 16) & 31
            simm = displacement(w)
            t = None
            if a == 13 and sda13 is not None:
                t = (sda13 + simm) & 0xFFFFFFFF
            elif a == 2 and sda2 is not None:
                t = (sda2 + simm) & 0xFFFFFFFF
            elif a != 0 and live_lis(a, i):
                t = (lis[a][0] + simm) & 0xFFFFFFFF
            if t is not None and is_data(t):
                refs[t].append(site)
                if loads is not None and o in LOAD_OPS:
                    loads.setdefault(t, []).append(site)
                if stores is not None and o in STORE_OPS:
                    stores.setdefault(t, []).append(site)
            elif o != 14 and a in areg and (loads is not None or stores is not None):
                ta = (areg[a][0] + simm) & 0xFFFFFFFF
                if is_data(ta):
                    if loads is not None and o in LOAD_OPS:
                        loads.setdefault(ta, []).append(site)
                    if stores is not None and o in STORE_OPS:
                        stores.setdefault(ta, []).append(site)
            if o in UPDATE_OPS and a in lis and t is not None:
                lis[a] = (t, i)                        # the update form leaves rA = the effective address
            if o == 14:
                if d != a:
                    areg.pop(d, None)
                if t is not None and is_data(t):
                    areg[d] = (t, i)
                else:
                    areg.pop(d, None)
            elif o in LOADS_INT:
                areg.pop(d, None)
            if o == 14 or o in LOADS_INT:
                lis.pop(d, None)
            elif o == 46:                              # lmw rD: rD..r31 written
                for r in range(d, 32):
                    lis.pop(r, None)
                    areg.pop(r, None)
            continue
        if o == 31 and (w >> 1) & 0x3FF == 444 and (w >> 21) & 31 == (w >> 11) & 31:    # mr rA,rS: rA takes rS
            s, a = (w >> 21) & 31, (w >> 16) & 31
            e, f = lis.get(s), areg.get(s)
            lis.pop(a, None)
            areg.pop(a, None)
            if e is not None:
                lis[a] = e
            if f is not None:
                areg[a] = f
            continue
        d = written_reg(w)
        if d is not None:
            lis.pop(d, None)
            areg.pop(d, None)
    return refs


def scan_calls(code, start, fn_starts) -> list[tuple[int, int]]:
    """`[(site, target)]` for every relative `b`/`bl` whose target is a function start other than the start of the
    function holding the site (a branch to its own start is a loop, not a call)."""
    out = []
    fs = sorted(set(fn_starts))
    fset = set(fs)
    for i, w in enumerate(words(code)):
        if w >> 26 != 18 or w & 2:
            continue
        site = start + i * 4
        t = (site + signed(w & 0x03FFFFFC, 26)) & 0xFFFFFFFF
        if t in fset:
            k = bisect.bisect_right(fs, site) - 1
            if k < 0 or fs[k] != t:
                out.append((site, t))
    return out


# --- the disassembly-text register decode ---------------------------------------------------------------------

_GPR = re.compile(r"^r(\d+)$")
_GPR_IN = re.compile(r"r(\d+)\s*\)")
#: mnemonic groups for `decode_rw`: the subset MWCC emits around a call site; anything else is "undecoded"
_DEST_FIRST = frozenset((
    "add addc adde addme addze subf subfc subfe subfme subfze mullw mulhw mulhwu divw divwu neg "
    "slw srw sraw srawi and or xor nor nand eqv orc andc extsb extsh extsw cntlzw popcntb "
    "li lis addi addis subi subis mulli subfic addic "
    "rlwinm rlwimi rlwnm slwi srwi clrlwi clrrwi extlwi extrwi rotlwi rotrwi "
    "mr ori oris xori xoris andi andis "
    "mflr mfctr mfcr mfspr mfmsr mftb"
).split())
_LOADS = frozenset("lbz lbzu lhz lhzu lha lhau lwz lwzu lwa lwarx lfs lfsu lfd lfdu lmw".split())
_STORES = frozenset(
    "stb stbu sth sthu stw stwu stfs stfsu stfd stfdu stmw stwcx stwx stwux stbx sthx".split())
_XLOADS = frozenset("lwzx lwzux lbzx lbzux lhzx lhax lhaux lfsx lfdx".split())
_XSTORES = frozenset("stwx stwux stbx sthx stfsx stfdx".split())
_CMP = frozenset("cmpwi cmpdi cmplwi cmpldi cmpw cmpd cmplw cmpld".split())
_MT = frozenset("mtlr mtctr mtcrf mtspr mtmsr".split())
BRANCH_MNEMONICS = frozenset((
    "b ba bl bla bc bca bcl bcla beq bne blt bgt ble bge bso bns bdnz bdz blr blrl bctr bctrl "
    "bcctr bclr beqlr bnelr bltlr bgtlr blelr bgelr bsolr bnsr bdnzlr bdzlr blrl"
).split())
CALL_MNEMONICS = frozenset("bl bla bcl bcla bctrl".split())
_INERT = frozenset(("nop", "sc", "sync", "isync", "eieio", "dcbf", "dcbst", "dcbt", "dccci", "icbi", "twi", "tw",
                    "crxor", "cror", "crand", "crnand", "crnor", "creqv", "crandc", "crorc", "mcrf", "crset",
                    "crclr", "crmove", "crnot"))


def _gpr(token: str) -> int | None:
    m = _GPR.match(token.strip())
    return int(m.group(1)) if m else None


def _gpr_in(token: str) -> int | None:
    m = _GPR_IN.search(token)
    return int(m.group(1)) if m else None


def decode_rw(mnemonic: str, operands: str):
    """`(reads, writes, is_branch, is_call, decoded)` for one disassembled instruction (GPR numbers).

    `decoded` is False outside the curated subset, so a caller reports `?` instead of inventing an answer."""
    toks = [t.strip() for t in operands.split(",")] if operands else []
    reads: set[int] = set()
    writes: set[int] = set()
    if mnemonic in BRANCH_MNEMONICS:
        return reads, writes, True, mnemonic in CALL_MNEMONICS, True
    if mnemonic in ("li", "lis"):
        d = _gpr(toks[0]) if toks else None
        if d is not None:
            writes.add(d)
        return reads, writes, False, False, True
    if mnemonic in _DEST_FIRST:
        for t in toks:
            g = _gpr(t)
            if g is not None:
                if not writes:
                    writes.add(g)
                else:
                    reads.add(g)
        return reads, writes, False, False, True
    if mnemonic in _LOADS or mnemonic in _STORES:
        if toks:
            g = _gpr(toks[0])
            if g is not None:
                (writes if mnemonic in _LOADS else reads).add(g)
            base = _gpr_in(toks[1]) if len(toks) > 1 else None
            if base is not None and base != 0:
                reads.add(base)
            if mnemonic == "lmw" and g is not None:
                writes.update(range(g, 32))
            if mnemonic == "stmw":
                reads.update(range(g, 32))
        return reads, writes, False, False, True
    if mnemonic in _XLOADS or mnemonic in _XSTORES:
        if toks:
            g = _gpr(toks[0])
            if g is not None:
                (writes if mnemonic in _XLOADS else reads).add(g)
            for t in toks[1:]:
                g = _gpr(t)
                if g is not None and g != 0:
                    reads.add(g)
        return reads, writes, False, False, True
    if mnemonic in _CMP or mnemonic in _MT:
        for t in toks:
            g = _gpr(t)
            if g is not None:
                reads.add(g)
        return reads, writes, False, False, True
    if mnemonic.startswith("f"):
        return reads, writes, False, False, True       # every float form touches FPRs/CR only
    if mnemonic.startswith(("ps_", "psq_")):
        for t in toks:                                  # a GPR operand of a paired-single form is an address
            for g in (_gpr(t), _gpr_in(t)):
                if g is not None and g != 0:
                    reads.add(g)
        return reads, writes, False, False, True
    if mnemonic in _INERT:
        return reads, writes, False, False, True
    return reads, writes, False, False, False
