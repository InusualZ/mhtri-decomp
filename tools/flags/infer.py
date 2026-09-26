#!/usr/bin/env python3
"""Infer the compiler flags a target object was built with, from the object's own bytes.

    python tools/flags/infer.py <unit|object>          # one unit
    python tools/flags/infer.py --all                  # every registered unit
    python tools/flags/infer.py --accuracy             # + the known-case accuracy table
    python tools/flags/infer.py <unit> --json          # machine-readable

A *unit* is spelled as in `configure.py` (`Camellia/camellia.c`, `auto/80073398_fn_80073398.cpp`)
or as a path to a split target object (`build/RMHE08/obj/...o`).  The object is the *target* object
under `build/RMHE08/obj/` (the original split out of the DOL), never our `build/RMHE08/src/` build.

Why this exists
---------------
The matching playbook (`docs/matching.md`, rows 39-46) records eight levers that are derivable from
the target bytes alone - `-opt nopeephole`, `-fp_contract off`, `-pool off`, `-str ...` (not
`readonly`), `-use_lmw_stmw off`, `-func_align 4`, `-inline noauto`, the C++ front-end - and every one
of them was rediscovered independently by a worker who had to notice the same byte pattern.  This tool
applies those rules automatically and reports, per flag, the evidence and a confidence.  It is a
*pre-flight*, not an oracle: it reads one side of the diff (the target), so a flag whose effect is
source-dependent (the peephole pass, the optimizer level) can only be inferred one way, and the tool
says so instead of guessing.

Fingerprints implemented
------------------------
* record forms (`rlwinm.`/`clrlwi.`/`and.`/`add.`/`extsb.`/...) - only the peephole pass emits them,
  so their presence proves `-opt peephole` on; their absence proves nothing.
* fused multiply-adds (`fmadds`/`fmsubs`/...) vs an unfused `fmuls`+`fadds` chain - the contraction.
* pooled literal addressing: a single `lis` base register reused across several data symbols (pool on)
  vs one `lis`+`addi` pair per symbol (pool off).
* the string pool's section (`.rodata` = `-str ...,readonly`, `.data` = not readonly).
* the FPR/`GPR` save/restore shape: `stmw`/`lmw` (`-use_lmw_stmw on`) vs the EABI `_savegpr_*`/
  `_restgpr_*` helpers (off).
* function start alignment and inter-function `gap_*` padding: a start off a 16-byte boundary proves
  `-func_align 4`; every start 16-byte aligned *with* `gap_*` padding is only consistent with 16
  (`-O4,p` implies it, and a `#pragma function_align 16` restores it), because a `-func_align 4`
  unit can land all-16-aligned too, so that direction is a hint.
* a kept `bl` to a tiny function defined in the same object - consistent with `-inline noauto`, but a
  hint only: `-inline auto` is a heuristic, a source `#pragma dont_inline` also keeps the call, and in
  a multi-TU split object the callee may be a different original translation unit.
* `extab`/`extabindex` presence, `.ctors`/`.dtors` fragments, and the `.comment` byte (reported as
  evidence; the split objects' `.comment` is synthesized from `config.yml` and is uniform here).

Instruction decoding is done on the raw big-endian words, not through `objdump`: GNU objdump in the
pinned binutils mis-decodes the Gekko paired-single instructions (primary opcode 4) as VMX, which
would hide the indexed `psq_lx`/`psq_stx` epilogue that row 39 is about.

Nothing here compiles, links, re-splits or writes to the repository.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys

# --- ELF ---------------------------------------------------------------------------------------

# Relocation types used by this project's objects.
R_PPC_ADDR32 = 1
R_PPC_ADDR16_LO = 4
R_PPC_ADDR16_HA = 6
R_PPC_REL24 = 10
R_PPC_EMB_SDA21 = 109

CODE_SECTIONS = (".text", ".init")
DATA_SECTIONS = (".data", ".rodata", ".sdata", ".sdata2", ".bss", ".sbss")


class Elf:
    """A minimal big-endian ELF32 reader: sections, symbols and RELA relocations."""

    def __init__(self, path: str):
        self.path = path
        self.data = open(path, "rb").read()
        d = self.data
        if d[:4] != b"\x7fELF":
            raise ValueError("%s: not an ELF file" % path)
        if d[4] != 1 or d[5] != 2:
            raise ValueError("%s: expected 32-bit big-endian ELF" % path)
        self.is64 = False
        e_shoff, = struct.unpack_from(">I", d, 0x20)
        e_shentsize, = struct.unpack_from(">H", d, 0x2E)
        e_shnum, = struct.unpack_from(">H", d, 0x30)
        e_shstrndx, = struct.unpack_from(">H", d, 0x32)
        self.sections = []
        for i in range(e_shnum):
            off = e_shoff + i * e_shentsize
            name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from(
                ">IIIIIIIIII", d, off)
            self.sections.append(dict(
                name_off=name, typ=typ, flags=flags, addr=addr, offset=offset, size=size,
                link=link, info=info, align=align, entsize=entsize))
        shstr = self.sections[e_shstrndx]

        def name_of(o: int) -> str:
            end = d.index(b"\0", shstr["offset"] + o)
            return d[shstr["offset"] + o:end].decode("latin1")

        for h in self.sections:
            h["name"] = name_of(h["name_off"])
            h["bytes"] = d[h["offset"]:h["offset"] + h["size"]]
        self.symbols = self._symbols()

    def section(self, name: str):
        for h in self.sections:
            if h["name"] == name:
                return h
        return None

    def code_bytes(self) -> bytes:
        out = b""
        for h in self.sections:
            if h["name"] in CODE_SECTIONS:
                out += h["bytes"]
        return out

    def _symbols(self):
        for h in self.sections:
            if h["typ"] != 2:  # SHT_SYMTAB
                continue
            strtab = self.sections[h["link"]]

            def name_of(o: int) -> str:
                end = self.data.index(b"\0", strtab["offset"] + o)
                return self.data[strtab["offset"] + o:end].decode("latin1")

            out = []
            for i in range(h["size"] // 16):
                name, val, size, info, other, shndx = struct.unpack_from(
                    ">IIIBBH", self.data, h["offset"] + i * 16)
                out.append(dict(name=name_of(name), val=val, size=size, info=info,
                                bind=info >> 4, type=info & 0xF, shndx=shndx))
            return out
        return []

    def relocations(self):
        """{section_name: [(offset, symbol_name, type, addend), ...]} for every SHT_RELA."""
        out = {}
        for h in self.sections:
            if h["typ"] != 4:  # SHT_RELA
                continue
            target = self.sections[h["info"]]["name"]
            symtab = self.sections[h["link"]]
            strtab = self.sections[symtab["link"]]

            def sym_name(idx: int) -> str:
                if idx == 0:
                    return ""
                off = symtab["offset"] + idx * 16
                name_off, = struct.unpack_from(">I", self.data, off)
                end = self.data.index(b"\0", strtab["offset"] + name_off)
                return self.data[strtab["offset"] + name_off:end].decode("latin1")

            ents = []
            for i in range(h["size"] // 12):
                off, info, add = struct.unpack_from(">IIi", self.data, h["offset"] + i * 12)
                ents.append((off, sym_name(info >> 8), info & 0xFF, add))
            out.setdefault(target, []).extend(ents)
        return out

    def functions(self):
        """The object's functions: name, section-relative address, size, in address order."""
        out = []
        for s in self.symbols:
            if s["type"] != 2 or s["size"] == 0:  # STT_FUNC
                continue
            if not (0 < s["shndx"] < len(self.sections)):
                continue
            if self.sections[s["shndx"]]["name"] not in CODE_SECTIONS:
                continue
            if s["name"].startswith("gap_"):
                continue
            out.append(s)
        out.sort(key=lambda s: s["val"])
        return out


# --- instruction decoding ----------------------------------------------------------------------

def op(w):  return w >> 26
def rt(w):  return (w >> 21) & 31
def ra(w):  return (w >> 16) & 31
def rb(w):  return (w >> 11) & 31
def frt(w): return (w >> 21) & 31
def fra(w): return (w >> 16) & 31
def frb(w): return (w >> 11) & 31
def frc(w): return (w >> 6) & 31
def xo5(w): return (w >> 1) & 31
def xo10(w): return (w >> 1) & 0x3FF
def si(w):  return struct.unpack(">h", struct.pack(">H", w & 0xFFFF))[0]
def sh(w):  return (w >> 11) & 31
def mb(w):  return (w >> 6) & 31
def me(w):  return (w >> 1) & 31


def rlwinm_alias(w):
    """The objdump spelling of an `rlwinm` (opcode 21) form, or 'rlwinm'.

    `clrlwi`/`clrrwi`/`srwi`/`slwi`/`extrwi` are aliases of single `rlwinm` instructions; the
    peephole's byte-extraction fold is `srwi`+`clrlwi` -> one `extrwi`, so telling the aliases apart
    is what makes the fold visible without objdump (which prints the aliases too).
    """
    if op(w) != 21:
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


# Narrowing stores: `clrlwi`/`clrrwi` directly before one of these is what the peephole folds away.
NARROW_STORE_OPS = {38, 39, 44, 45}   # stb, stbu, sth, sthu

# Primary opcodes whose bit 0 is the Rc (record) bit for every instruction in the family.
RECORD_OPCODES = {12, 13, 20, 21, 23, 31, 59, 63}
# Fused multiply-add XO values for the A-form float opcodes (59 single, 63 double).
FMA_XO = {28, 29, 30, 31}
FMUL_XO = {25}
FADD_XO = {21}
FSUB_XO = {20}
# Gekko paired-single indexed forms share primary opcode 4 with (later) VMX; the FPR epilogue's
# `li r0,N; psq_lx/psq_stx` uses XO 6.
PSQ_INDEXED_XO = 6


class Insn:
    __slots__ = ("addr", "word", "op", "reloc_sym", "reloc_type")

    def __init__(self, addr, word, reloc_sym="", reloc_type=-1):
        self.addr = addr
        self.word = word
        self.op = op(word)
        self.reloc_sym = reloc_sym
        self.reloc_type = reloc_type

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

    def is_clr_mask(self) -> bool:
        return rlwinm_alias(self.word) in ("clrlwi", "clrrwi")

    def is_shift(self) -> bool:
        return rlwinm_alias(self.word) in ("srwi", "slwi")

    def is_narrow_store(self) -> bool:
        return self.op in NARROW_STORE_OPS

    def is_branch_link(self) -> bool:
        return self.op == 18 and (self.word & 1) == 1

    def is_lis(self) -> bool:
        return self.op == 15 and ra(self.word) == 0

    def writes_gpr(self):
        """The GPR this instruction defines, or None (best effort; only the forms we decode)."""
        o = self.op
        if o in (14, 15, 24, 25, 26, 27, 28, 29, 32, 33, 34, 35, 36, 37, 38, 40, 42, 43):
            return rt(self.word)
        if o == 31:
            if xo10(self.word) in (266, 40, 444, 12, 124, 136, 792, 824, 536, 316, 8, 104, 235, 75, 83, 87, 467, 459, 491, 202, 210, 234, 266, 491):
                return rt(self.word)
        return None

    def reads_gpr(self):
        o = self.op
        if o in (14, 15, 24, 25, 26, 27, 28, 29, 32, 33, 34, 35, 36, 37, 38, 40, 42, 43):
            return ra(self.word)
        if o in (18, 16):
            return None
        if o == 31:
            return ra(self.word) if ra(self.word) else None
        return None


# --- per-object fingerprints -------------------------------------------------------------------

class Fingerprint:
    """Every byte-derived observation about one target object."""

    def __init__(self, path: str):
        self.path = path
        self.elf = Elf(path)
        self.relocs = self.elf.relocations()
        self.insns = self._decode()
        self.funcs = self._functions_with_bodies()
        self.sections = {h["name"]: h["size"] for h in self.elf.sections}
        self.findings = []
        self.notes = []
        self._run()

    # -- decoding --------------------------------------------------------------
    def _decode(self):
        insns = []
        for h in self.elf.sections:
            if h["name"] not in CODE_SECTIONS:
                continue
            rel = {}
            for off, sym, typ, _add in self.relocs.get(h["name"], []):
                # a relocation's offset points at the immediate field, i.e. the instruction + 2;
                # fold it back to the instruction start.
                rel[off & ~3] = (sym, typ)
            for i in range(0, h["size"] - 3, 4):
                addr = h["offset"] + i
                word, = struct.unpack_from(">I", self.elf.data, addr)
                sym, typ = rel.get(i, ("", -1))
                insns.append(Insn(i, word, sym, typ))
        return insns

    def _functions_with_bodies(self):
        # Address-ordered (name, start, end) using the function symbols.
        out = []
        for f in self.elf.functions():
            out.append((f["name"], f["val"], f["val"] + f["size"]))
        return out

    def insns_in(self, start, end):
        return [i for i in self.insns if start <= i.addr < end]

    def _bl_targets(self):
        """Multi-set of relocation symbols called by a `bl` in this object."""
        return [i.reloc_sym for i in self.insns if i.is_branch_link() and i.reloc_sym]

    # -- fingerprints ----------------------------------------------------------
    def _run(self):
        self._fp_peephole()
        self._fp_fp_contract()
        self._fp_pool()
        self._fp_strings()
        self._fp_lmw_stmw()
        self._fp_align()
        self._fp_inline()
        self._fp_sections()
        self._fp_comment()

    def add(self, flag, value, confidence, evidence):
        self.findings.append(dict(flag=flag, value=value, confidence=confidence, evidence=evidence))

    # 1. peephole: record forms prove the pass ran on *some* function; a sequence the pass would
    # have folded proves it did not run on the function that kept it: a `clrlwi` before a narrowing
    # store, an unfused `srwi`+`clrlwi`, a `li r0,N; psq_lx/psq_stx` epilogue, and a kept
    # `lwz rX,disp(rB)` + `addi rB,rB,disp` (the pass folds that pair into `lwzu rX,disp(rB)`).
    # When both are present the TU carries a scoped `#pragma peephole off`, so the unit-level -opt
    # is not settled and the reading is only a hint.
    def _fp_peephole(self):
        rec = sum(1 for i in self.insns if i.is_record())
        idx_psq = sum(1 for i in self.insns if i.is_psq_indexed())
        self.rec_count = rec
        self.idx_psq_count = idx_psq
        clr_store = shift_mask = load_update = 0
        for _name, start, end in self.funcs:
            body = self.insns_in(start, end)
            for k in range(len(body) - 1):
                if body[k].is_clr_mask() and body[k + 1].is_narrow_store():
                    clr_store += 1
                if body[k].is_shift() and body[k + 1].is_clr_mask():
                    shift_mask += 1
            for k, ins in enumerate(body):
                if ins.op != 32:  # lwz
                    continue
                base, disp = ra(ins.word), si(ins.word)
                if base == 0 or disp == 0:
                    continue
                # the first instruction that writes the base is the candidate `addi`; the pass
                # would have rewritten the pair into `lwzu`, so a surviving pair means the pass was
                # off in this function.
                for later in body[k + 1:k + 7]:
                    w = later.writes_gpr()
                    if w is None or w != base:
                        continue
                    if later.op == 14 and rt(later.word) == base \
                            and si(later.word) == disp:
                        load_update += 1
                    break
        self.clr_store_count = clr_store
        self.shift_mask_count = shift_mask
        self.load_update_count = load_update
        folds = idx_psq + clr_store + shift_mask + load_update
        if rec:
            if folds:
                self.add("peephole", "mixed", "low",
                         "%d record form(s) prove the pass ran on some function(s), but %d "
                         "sequence(s) it would have folded are kept in others (%d `li r0,N; "
                         "psq_lx/psq_stx`, %d kept `clrlwi` before a narrowing store, %d unfused "
                         "`srwi`+`clrlwi`, %d kept `lwz`+`addi` load-update) - the TU carries a "
                         "scoped `#pragma peephole off`, so the unit-level -opt is not `on`"
                         % (rec, folds, idx_psq, clr_store, shift_mask, load_update))
            else:
                self.add("peephole", "on", "high",
                         "%d record-form instruction(s) (rlwinm./and./add./srwi./extsb.) - only the "
                         "peephole pass emits them" % rec)
        elif folds:
            self.add("peephole", "off", "medium",
                     "0 record forms, but %d instruction(s) the peephole pass would have folded: "
                     "%d `li r0,N; psq_lx/psq_stx` epilogue op(s), %d kept `clrlwi` before a "
                     "narrowing store, %d unfused `srwi`+`clrlwi`, %d kept `lwz`+`addi` load-update"
                     % (folds, idx_psq, clr_store, shift_mask, load_update))
        else:
            self.add("peephole", "unknown", "none",
                     "0 record-form instructions and no fold-shaped sequence - the peephole's folds "
                     "are source-dependent, so absence is not evidence of `off`")

    # 2. fp_contract: a fused FMA proves `on`; an unfused fmul->fadd chain proves `off`.
    def _fp_fp_contract(self):
        fma = sum(1 for i in self.insns if i.is_fma())
        pairs = 0
        for idx, i in enumerate(self.insns):
            if not i.is_fmul():
                continue
            dest = frt(i.word)
            for j in self.insns[idx + 1:idx + 5]:
                if j.op not in (59, 63) or j.is_fma():
                    break
                if j.is_faddsub() and dest in (fra(j.word), frb(j.word)):
                    pairs += 1
                    break
        self.fma_count = fma
        self.contract_pairs = pairs
        if fma and pairs:
            self.add("fp_contract", "mixed", "low",
                     "%d fused multiply-add(s) in some function(s) but %d unfused fmuls+fadds/fsubs "
                     "chain(s) elsewhere - a file-scoped `#pragma fp_contract off` pair (the chain "
                     "is not proof the source wrote `a*b+c`, but it is proof the pass did not run "
                     "everywhere), so the unit-level flag is not settled" % (fma, pairs))
        elif fma:
            self.add("fp_contract", "on", "high",
                     "%d fused multiply-add(s) (fmadds/fmsubs/fnmadds/fnmsubs) present" % fma)
        elif pairs:
            self.add("fp_contract", "off", "low",
                     "%d unfused fmuls+%s chain(s) with no fused form anywhere - suggestive, but "
                     "the chain is not proof the source wrote one a*b+c expression"
                     % (pairs, "fadds/fsubs"))
        else:
            self.add("fp_contract", "unknown", "none",
                     "no fused multiply-add and no unfused a*b+c chain - nothing to contract")

    # 3. pool: a base register reused across distinct data symbols (or addressed with a large plain
    # immediate) means the local literals were pooled; one `lis`+`addi` pair per symbol means not.
    def _fp_pool(self):
        defined = set()
        for s in self.elf.symbols:
            if 0 < s["shndx"] < len(self.elf.sections):
                if self.elf.sections[s["shndx"]]["name"] in DATA_SECTIONS \
                        and not s["name"].startswith("jumptable"):
                    defined.add(s["name"])
        shared = 0
        large_imm = 0
        per_symbol = set()
        for name, start, end in self.funcs:
            body = self.insns_in(start, end)
            for k, ins in enumerate(body):
                if not (ins.is_lis() and ins.reloc_type == R_PPC_ADDR16_HA):
                    continue
                base = rt(ins.word)
                sym = ins.reloc_sym
                for later in body[k + 1:]:
                    if later.writes_gpr() == base:
                        break
                    if later.reloc_type == R_PPC_ADDR16_LO and later.reloc_sym and later.reloc_sym != sym:
                        if later.reads_gpr() == base:
                            shared += 1
                        break
                    if later.op == 14 and ra(later.word) == base and later.reloc_type == -1 \
                            and abs(si(later.word)) >= 0x100:
                        large_imm += 1
                        break
                if sym in defined:
                    per_symbol.add(sym)
        self.pool_shared = shared
        self.pool_large_imm = large_imm
        self.pool_defined_syms = sorted(per_symbol)
        if shared or large_imm:
            self.add("pool", "on", "medium",
                     "shared literal base register (%d cross-symbol use(s), %d large plain-immediate "
                     "offset(s)) - the local literals were pooled" % (shared, large_imm))
        elif len(per_symbol) >= 2:
            self.add("pool", "off", "medium",
                     "%d distinct object-local data symbol(s) (%s) each addressed by its own "
                     "lis+addi pair, no shared base"
                     % (len(per_symbol), ", ".join(sorted(per_symbol)[:4])))
        elif per_symbol:
            self.add("pool", "unknown", "none",
                     "one object-local data symbol (%s) addressed per-symbol - too little to tell"
                     % ", ".join(sorted(per_symbol)))
        else:
            self.add("pool", "unknown", "none",
                     "no object-local literal base in this object - nothing to pool")

    # 4. strings: which section the pool landed in.
    def _fp_strings(self):
        def strings(secname):
            h = self.elf.section(secname)
            if not h:
                return []
            found, cur = [], bytearray()
            for b in h["bytes"]:
                if 0x20 <= b < 0x7F:
                    cur.append(b)
                else:
                    # a real string literal is NUL-terminated; binary data that merely looks
                    # printable (a constant table) is not
                    if len(cur) >= 4 and b == 0:
                        found.append(cur.decode("latin1"))
                    cur = bytearray()
            return found
        ro = strings(".rodata")
        da = strings(".data")
        # A real pool has several strings; one or two short NUL-terminated runs inside a constant
        # table are coincidence, so require at least three.
        if len(ro) < 3:
            ro = []
        if len(da) < 3:
            da = []
        self.strings_ro, self.strings_data = ro, da
        if da and not ro:
            self.add("str_readonly", "off", "medium",
                     "%d NUL-terminated string(s) in .data and none in .rodata - the pool is not "
                     "`readonly`" % len(da))
        elif ro and not da:
            self.add("str_readonly", "on", "low",
                     "%d NUL-terminated string(s) in .rodata and none in .data" % len(ro))
        elif ro and da:
            self.add("str_readonly", "mixed", "low",
                     "%d string(s) in .rodata and %d in .data" % (len(ro), len(da)))
        else:
            self.add("str_readonly", "unknown", "none",
                     "the unit's string pool is not in this object (it is a separate data split), "
                     "so -str readonly is not observable here")

    # 5. lmw/stmw vs the EABI save helpers.
    def _fp_lmw_stmw(self):
        stmw = sum(1 for i in self.insns if i.op in (46, 47))
        save = sum(1 for s in self._bl_targets() if s.startswith(("_savegpr_", "_restgpr_")))
        self.stmw_count, self.savegpr_count = stmw, save
        if stmw:
            self.add("lmw_stmw", "on", "high",
                     "%d stmw/lmw instruction(s) present" % stmw)
        elif save:
            self.add("lmw_stmw", "off", "high",
                     "no stmw/lmw; %d EABI _savegpr_*/_restgpr_* call(s) instead" % save)
        else:
            self.add("lmw_stmw", "unknown", "none",
                     "no stmw/lmw and no multi-register save at all - the unit never saves enough "
                     "registers to tell")

    # 6. func_align / -O4,p: 16-byte function starts (and gap padding) vs 4-byte packing.
    def _fp_align(self):
        starts = [f[1] for f in self.funcs]
        if len(starts) < 2:
            self.add("func_align", "unknown", "none",
                     "only %d function with a body - alignment is not observable" % len(starts))
            return
        misaligned = [s for s in starts if s % 16]
        gaps = [s["size"] for s in self.elf.symbols if s["name"].startswith("gap_") and s["size"]]
        if misaligned:
            self.add("func_align", "4", "high",
                     "%d of %d function(s) start off a 16-byte boundary" % (len(misaligned), len(starts)))
        elif gaps:
            # 16-aligned starts + padding is *consistent* with -func_align 16, but not proof of it:
            # a -func_align 4 unit can land every function on a 16-byte boundary with padding too
            # (DWCi/fn_805113B0.c and OS/FindContainHeap_.c document -func_align 4 and look exactly
            # like this), so this direction is only a hint.  A start *off* 16 is the proof (above).
            self.add("func_align", "16", "low",
                     "all %d function(s) start on a 16-byte boundary and %d gap_* padding "
                     "fragment(s) (%d bytes) pad them out - consistent with -func_align 16, but a "
                     "-func_align 4 unit also lands all-16-aligned with padding, so this is a hint"
                     % (len(starts), len(gaps), sum(gaps)))
            self.add("opt_level_hint", "-O4,p", "low",
                     "-O4,p implies -func_align 16; a -O3 build with an explicit -func_align 16 "
                     "looks the same")
        else:
            self.add("func_align", "16", "low",
                     "all %d function(s) start on a 16-byte boundary, but there is no gap_* "
                     "padding - could be coincidence (function sizes that are multiples of 16)"
                     % len(starts))

    # 7. inline: a kept call to a tiny function defined in the same object.  Bootstrap `.init`
    # (hand-written asm, no source inlining) is excluded.
    def _fp_inline(self):
        sizes = {f["name"]: f["size"] for f in self.elf.functions()}
        init_syms = {s["name"] for s in self.elf.symbols
                     if 0 < s["shndx"] < len(self.elf.sections)
                     and self.elf.sections[s["shndx"]]["name"] == ".init"}
        called = set(self._bl_targets())
        tiny = sorted(n for n in called if 0 < sizes.get(n, 0) <= 0x40 and n not in init_syms)
        self.tiny_callees = tiny
        if tiny:
            # A kept call is consistent with noauto, but not proof: -inline auto is a heuristic
            # (it may decline), a source `#pragma dont_inline` also keeps the call, and in these
            # maximal-run split objects the tiny callee may be a different original TU - a cross-TU
            # call is never inlined under either flag.  So this is a hint, not a confident claim.
            self.add("inline", "noauto", "low",
                     "kept bl to %d tiny same-object function(s) (%s) - consistent with -inline "
                     "noauto, but a kept call is not proof of it (`-inline auto` is a heuristic and "
                     "a `#pragma dont_inline` or a different original TU also keeps it)"
                     % (len(tiny), ", ".join("%s %dB" % (n, sizes[n]) for n in tiny[:3])))
        else:
            self.add("inline", "unknown", "none",
                     "no kept call to a small same-object function - auto and noauto are "
                     "indistinguishable here")

    # 8. sections: exceptions, ctors/dtors, data fragments, and the string base register.
    def _fp_sections(self):
        # a `@stringBase0`-style base symbol is the `-pool on` string-pooling signature: the pool
        # is addressed once and each string is an offset from it.
        bases = sorted({s for _o, s, _t, _a in
                        (e for ents in self.relocs.values() for e in ents)
                        if "stringBase" in s})
        self.string_bases = bases
        if bases:
            self.notes.append("pooled string base register(s): %s" % ", ".join(bases))
        extab = self.sections.get("extab", 0)
        extabindex = self.sections.get("extabindex", 0)
        self.extab, self.extabindex = extab, extabindex
        if extab or extabindex:
            self.notes.append("extab %d B / extabindex %d B present (the compiler emits these for "
                              "C++/framed code with -Cpp_exceptions off too, so they do not settle "
                              "the exception flag)" % (extab, extabindex))
        ctors = [h["name"] for h in self.elf.sections if h["name"].startswith(".ctors")]
        dtors = [h["name"] for h in self.elf.sections if h["name"].startswith(".dtors")]
        if ctors or dtors:
            self.notes.append("static init/term fragments: %s" % ", ".join(ctors + dtors))
        self.add("exceptions", "unknown", "none",
                 "extab/extabindex do not discriminate -Cpp_exceptions on/off in this project")

    # 9. comment byte.
    def _fp_comment(self):
        h = self.elf.section(".comment")
        self.comment = ""
        if h:
            # the useful part is the leading CodeWarrior magic + version bytes; dtk fills the rest
            # of the section from the string table.
            raw = h["bytes"][:24]
            self.comment = raw.split(b"\0")[0].decode("latin1", "replace")
            self.notes.append(".comment = %r + %s (the split objects' .comment is synthesized from "
                              "config.yml; it is uniform here and names no toolchain)"
                              % (self.comment, raw[11:13].hex()))


# --- inference rendering -----------------------------------------------------------------------

CONF_ORDER = {"high": 0, "medium": 1, "low": 2, "none": 3}


def render(fp: Fingerprint) -> str:
    lines = ["%s" % fp.path]
    for f in sorted(fp.findings, key=lambda x: CONF_ORDER[x["confidence"]]):
        conf = {"high": "high", "medium": "med", "low": "low", "none": " -- "}[f["confidence"]]
        lines.append("  %-14s %-8s [%s] %s" % (f["flag"], f["value"], conf, f["evidence"]))
    for n in fp.notes:
        lines.append("  note: %s" % n)
    return "\n".join(lines)


# --- unit resolution + ground truth ------------------------------------------------------------

def repo_root() -> str:
    d = os.path.dirname(os.path.abspath(__file__))
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        p = os.path.dirname(d)
        if p == d:
            raise SystemExit("repo root (configure.py) not found")
        d = p


def obj_path_for(unit_name: str, root: str) -> str:
    """`auto/x.c` -> build/RMHE08/obj/auto/x.o;  `main.cpp` -> build/RMHE08/obj/main.o."""
    stem = os.path.splitext(unit_name)[0]
    return os.path.join(root, "build", "RMHE08", "obj", stem + ".o")


def registered_units(root: str):
    """(lib, unit_name, obj_path, cflags) for every Object() in configure.py, without running it."""
    src = open(os.path.join(root, "configure.py")).read()
    src = src.split("if args.mode ==")[0]
    ns = {"__name__": "configure_prefix"}
    if root not in sys.path:
        sys.path.insert(0, root)
    sys.argv = ["configure.py"]
    exec(compile(src, "configure.py", "exec"), ns)  # read-only: the build-generating tail is cut
    out = []
    for lib in ns["config"].libs:
        for o in lib["objects"]:
            # Object() keeps its options in `.options`; a per-object `cflags=` override (Pl/pl_skill,
            # Network/fn_803D3CE8, Network/fn_8041A87C) must beat the library group.
            opts = getattr(o, "options", {}) or {}
            override = opts.get("cflags")
            cflags = list(override or lib["cflags"])
            out.append((lib["lib"], o.name, obj_path_for(o.name, root), cflags))
    return out


def resolve_object(spec: str, root: str) -> str:
    if os.path.exists(spec):
        return spec
    p = os.path.join(root, spec)
    if os.path.exists(p):
        return p
    p = obj_path_for(spec, root)
    if os.path.exists(p):
        return p
    raise SystemExit("no object for %r (looked for %s)" % (spec, p))


# -- ground truth for the accuracy table --------------------------------------------------------

PRAGMA_RE = re.compile(r"^\s*#\s*pragma\s+([A-Za-z_]+)\s+(.*?)\s*(?:/\*.*)?$")


def source_pragmas(unit_name: str, root: str):
    """The real (non-comment) pragmas in a unit's source, as {name: [values in order]}.

    A list, not a single value, because a unit may carry a scoped `#pragma peephole off` /
    `#pragma peephole reset` pair - the campaign records the lever as "needed" from the `off`, even
    though the last pragma in the file is a reset.
    """
    stem = os.path.splitext(unit_name)[0]
    for ext in (".c", ".cpp", ".cp", ".cc", ".cxx"):
        p = os.path.join(root, "src", stem + ext)
        if os.path.exists(p):
            break
    else:
        return {}
    out = {}
    for line in open(p, errors="replace"):
        # strip block/line comments so prose that mentions a pragma is not read as one
        line = re.sub(r"/\*.*?\*/", " ", line)
        line = line.split("//")[0]
        m = PRAGMA_RE.match(line)
        if m:
            out.setdefault(m.group(1), []).append(m.group(2).strip())
    return out


def parse_cflags(cflags):
    """The effective value of the flags this tool infers, last occurrence winning."""
    c = " ".join(cflags)
    out = {}

    def last(pattern):
        m = None
        for m in re.finditer(pattern, c):
            pass
        return m

    m = last(r"-str\s+([^ ]+)")
    if m:
        out["str"] = m.group(1)
    m = last(r"-inline\s+(\w+)")
    if m:
        out["inline"] = m.group(1)
    m = last(r"-use_lmw_stmw\s+(\w+)")
    if m:
        out["lmw_stmw"] = m.group(1)
    m = last(r"-func_align\s+(\d+)")
    if m:
        out["func_align"] = m.group(1)
    m = last(r"-fp_contract\s+(\w+)")
    if m:
        out["fp_contract"] = m.group(1)
    m = last(r"-pool\s+(\w+)")
    if m:
        out["pool"] = m.group(1)
    m = last(r"-Cpp_exceptions\s+(\w+)")
    if m:
        out["exceptions"] = m.group(1)
    # merge every -opt's sub-options, last wins
    opts = {}
    for m in re.finditer(r"-opt\s+([^ ]+)", c):
        for part in m.group(1).split(","):
            if "=" in part:
                k, v = part.split("=", 1)
                opts[k] = v
            else:
                opts[part] = "on"
    out["opt"] = opts
    return out


def expected_flags(cflags, pragmas, documented):
    """The documented value of each lever for a unit, or absent when the campaign has no record.

    For a non-`auto` library the `configure.py` cflags group is the record.  For an `auto` unit the
    cflags group is a bulk-attribution guess, so only a source pragma (docs/matching.md rows 39-40)
    is treated as known.
    """
    cf = parse_cflags(cflags)
    exp = {}

    def pragma(name, value):
        return value in pragmas.get(name, [])

    # peephole: only an explicit -opt peephole/nopeephole (or a pragma) is a record.
    if "nopeephole" in cf["opt"]:
        exp["peephole"] = "off"
    elif "peephole" in cf["opt"]:
        exp["peephole"] = "on"
    if pragma("peephole", "off"):
        exp["peephole"] = "off"
    elif pragma("peephole", "on"):
        exp["peephole"] = "on"

    # A source `#pragma function_align N` is a per-unit record, exactly like a peephole/fp_contract
    # pragma: it overrides the cflags group (AX/AXFXReverbHi.c and EXI/ProbeBarnacle.c restore
    # -O4,p's 16 under the OS group that says -func_align 4).
    if pragma("function_align", "16"):
        exp["func_align"] = "16"
    elif pragma("function_align", "4"):
        exp["func_align"] = "4"

    if not documented:
        # fp_contract: a pragma is the only record for an auto unit.
        if pragma("fp_contract", "off"):
            exp["fp_contract"] = "off"
        elif pragma("fp_contract", "on"):
            exp["fp_contract"] = "on"
        return exp

    if cf.get("fp_contract"):
        exp["fp_contract"] = cf["fp_contract"]
    if pragma("fp_contract", "off"):
        exp["fp_contract"] = "off"
    elif pragma("fp_contract", "on"):
        exp["fp_contract"] = "on"
    exp["pool"] = cf.get("pool", "on")
    if "str" in cf:
        exp["str_readonly"] = "on" if "readonly" in cf["str"] else "off"
    if cf.get("lmw_stmw"):
        exp["lmw_stmw"] = cf["lmw_stmw"]
    if cf.get("inline"):
        exp["inline"] = cf["inline"]
    if "func_align" not in exp:  # a source `#pragma function_align N` wins over the cflags group
        if cf.get("func_align"):
            exp["func_align"] = cf["func_align"]
        elif cf["opt"].get("level") in ("0", "1", "2", "3") or re.search(r"-O[0-3]", " ".join(cflags)):
            # the project's documented -O3 units (Camellia, Pl, main) pack on 4-byte boundaries
            exp["func_align"] = "4"
    return exp


def _matches(lever, want, have):
    if lever == "inline":
        if want == "auto":
            return have == "unknown"  # an abstention is the correct answer under -inline auto
        return want == have
    return want == have


def accuracy_report(root: str):
    """Score the inferencer against the campaign's documented cases."""
    rows = []
    for lib, name, obj, cflags in registered_units(root):
        if not os.path.exists(obj):
            continue
        pragmas = source_pragmas(name, root)
        documented = lib != "auto"
        exp = expected_flags(cflags, pragmas, documented)
        fp = Fingerprint(obj)
        got = {}
        conf = {}
        for f in fp.findings:
            got[f["flag"]] = f["value"]
            conf[f["flag"]] = f["confidence"]
        rows.append(dict(lib=lib, name=name, documented=documented, expected=exp,
                         got=got, conf=conf, fp=fp))

    levers = ["peephole", "fp_contract", "pool", "str_readonly", "lmw_stmw", "inline",
              "func_align"]
    summary = {}
    for lever in levers:
        s = dict(claims=0, correct=0, hints=0, hint_correct=0, abstain=0, misses=[], hints_miss=[])
        for r in rows:
            if lever not in r["expected"]:
                continue
            want = r["expected"][lever]
            have = r["got"].get(lever, "unknown")
            if have == "unknown":
                s["abstain"] += 1
                continue
            ok = _matches(lever, want, have)
            if r["conf"][lever] in ("high", "medium"):
                s["claims"] += 1
                s["correct"] += ok
                if not ok:
                    s["misses"].append((r["name"], want, have))
            else:
                s["hints"] += 1
                s["hint_correct"] += ok
                if not ok:
                    s["hints_miss"].append((r["name"], want, have))
        summary[lever] = s
    return rows, summary


def print_accuracy(root: str):
    rows, summary = accuracy_report(root)
    n = len(rows)
    known = sum(1 for r in rows if r["expected"])
    print("\n=== known-case accuracy (%d units with a documented lever of %d registered) ==="
          % (known, n))
    print("%-14s %8s %9s %8s %9s %8s   confident misses"
          % ("lever", "claims", "correct", "hints", "hint ok", "abstain"))
    for lever, s in summary.items():
        pct = ("%.0f%%" % (100.0 * s["correct"] / s["claims"])) if s["claims"] else "-"
        hpct = ("%.0f%%" % (100.0 * s["hint_correct"] / s["hints"])) if s["hints"] else "-"
        miss = ", ".join("%s(want %s got %s)" % m for m in s["misses"][:4])
        print("%-14s %8d %9s %8d %9s %8d   %s"
              % (lever, s["claims"], pct, s["hints"], hpct, s["abstain"], miss))
    print("\nconfident = high/medium confidence claims; hints = low confidence, not scored as")
    print("confident because the fingerprint is source-dependent.  Names of the known cases follow.")

    print("\n%-42s %-11s %-11s %-11s %-11s %-11s"
          % ("unit", "peephole", "fp_contract", "lmw_stmw", "inline", "func_align"))
    for r in rows:
        if not r["expected"]:
            continue
        def cell(lever):
            want = r["expected"].get(lever)
            have = r["got"].get(lever, "unknown")
            if want is None:
                return "-"
            if have == "unknown":
                return "%s/?" % want
            mark = "" if _matches(lever, want, have) else "*"
            return "%s/%s%s" % (want, have, mark)
        print("%-42s %-11s %-11s %-11s %-11s %-11s"
              % (r["name"], cell("peephole"), cell("fp_contract"), cell("lmw_stmw"),
                 cell("inline"), cell("func_align")))
    print("\ncell = documented/inferred ('?' = the tool abstained, '*' = a confident miss;"
          " a '*' on a low-confidence hint is not a confident miss)")
    inferable = sum(1 for r in rows if any(v != "unknown" for v in r["got"].values()))
    print("\nunits with at least one inferred lever: %d/%d" % (inferable, n))


def print_markdown(root: str):
    """A compact, reproducible per-unit run + the accuracy summary, as markdown."""
    rows, summary = accuracy_report(root)
    levers = ["peephole", "fp_contract", "pool", "str_readonly", "lmw_stmw", "inline",
              "func_align"]
    print("# Flag inference run\n")
    print("Generated by `python tools/flags/infer.py --markdown` over every object registered in")
    print("`configure.py`.  `?` = the tool abstained (no evidence); a flag name is the inferred")
    print("value.  The confidence is in `--all`; every `off`/`on` below is a confident claim except")
    print("`fp_contract off` (low) and `func_align 16` without gap padding (low).\n")
    print("| unit | " + " | ".join(levers) + " |")
    print("|" + "---|" * (len(levers) + 1))
    for r in sorted(rows, key=lambda r: r["name"]):
        cells = [r["got"].get(l, "?") for l in levers]
        print("| %s | %s |" % (r["name"], " | ".join(cells)))
    print()
    print("## Aggregate (of %d units)\n" % len(rows))
    for lever in levers:
        counts = {}
        for r in rows:
            v = r["got"].get(lever, "unknown")
            counts[v] = counts.get(v, 0) + 1
        print("- `%s`: %s" % (lever, ", ".join("%s %d" % (k, v) for k, v in sorted(counts.items()))))
    print()
    print("## Known-case accuracy\n")
    print("| lever | confident claims | correct | low-confidence hints | hint ok | abstained |")
    print("|---|---|---|---|---|---|")
    for lever in levers:
        s = summary[lever]
        cp = ("%.0f%%" % (100.0 * s["correct"] / s["claims"])) if s["claims"] else "-"
        hp = ("%.0f%%" % (100.0 * s["hint_correct"] / s["hints"])) if s["hints"] else "-"
        print("| %s | %d | %s | %d | %s | %d |"
              % (lever, s["claims"], cp, s["hints"], hp, s["abstain"]))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", nargs="?", help="unit spec or path to a target object")
    ap.add_argument("--all", action="store_true", help="infer for every registered unit")
    ap.add_argument("--accuracy", action="store_true", help="print the known-case accuracy table")
    ap.add_argument("--markdown", action="store_true", help="print the run + accuracy as markdown")
    ap.add_argument("--selftest", action="store_true", help="run tools/flags/infer_selftest.py")
    ap.add_argument("--json", action="store_true", help="emit JSON")
    args = ap.parse_args(argv)
    root = repo_root()

    if args.selftest:
        sys.path.insert(0, os.path.join(root, "tools", "flags"))
        import infer_selftest
        return infer_selftest.main()

    if args.all:
        for lib, name, obj, _cf in registered_units(root):
            if not os.path.exists(obj):
                continue
            fp = Fingerprint(obj)
            if args.json:
                print(json.dumps(dict(name=name, lib=lib, findings=fp.findings, notes=fp.notes)))
            else:
                print(render(fp))
                print()
    elif args.unit:
        fp = Fingerprint(resolve_object(args.unit, root))
        if args.json:
            print(json.dumps(dict(path=fp.path, findings=fp.findings, notes=fp.notes)))
        else:
            print(render(fp))

    if args.accuracy:
        print_accuracy(root)

    if args.markdown:
        print_markdown(root)

    if not args.unit and not args.all and not args.accuracy and not args.markdown and not args.selftest:
        ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
