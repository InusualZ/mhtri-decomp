#!/usr/bin/env python3
"""Deterministic self-test for the flag inferencer in tools/flags/infer.py.

    python tools/flags/infer_selftest.py
    python tools/flags/infer.py --selftest

Two layers:

* pure unit tests of the instruction decoder, the cflags/pragma ground-truth parser and the
  `Insn` predicates - no files, no build;
* one synthetic ELF32 big-endian object per fingerprint, written here, so the contract is pinned
  without depending on `build/RMHE08/obj/` (which only exists after a `dol split`).

If the real split objects are present the test additionally asserts the known-case accuracy has no
confident miss - that is the property the tool exists to have.
"""
from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "flags") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "flags"))

import infer  # noqa: E402

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4


# --- instruction encoders (big-endian PPC words) ------------------------------------------------

def lis(rt_, imm=0):
    return (15 << 26) | (rt_ << 21) | (imm & 0xFFFF)


def addi(rt_, ra_, imm=0):
    return (14 << 26) | (rt_ << 21) | (ra_ << 16) | (imm & 0xFFFF)


def rlwinm(rs_, ra_, sh_, mb_, me_, rc=0):
    return (21 << 26) | (rs_ << 21) | (ra_ << 16) | (sh_ << 11) | (mb_ << 6) | (me_ << 1) | rc


def clrlwi(rs_, ra_, n, rc=0):
    return rlwinm(rs_, ra_, 0, n, 31, rc)


def srwi(rs_, ra_, n, rc=0):
    return rlwinm(rs_, ra_, 32 - n, n, 31, rc)


def stb(rs_, ra_, d=0):
    return (38 << 26) | (rs_ << 21) | (ra_ << 16) | (d & 0xFFFF)


def lwz(rt_, ra_, d=0):
    return (32 << 26) | (rt_ << 21) | (ra_ << 16) | (d & 0xFFFF)


def stmw(rs_, ra_, d=0):
    return (47 << 26) | (rs_ << 21) | (ra_ << 16) | (d & 0xFFFF)


def lmw(rt_, ra_, d=0):
    return (46 << 26) | (rt_ << 21) | (ra_ << 16) | (d & 0xFFFF)


def bl(disp=1):
    return (18 << 26) | (disp & 0x3FFFFFC) | 1


def fmadds(frt_, fra_, frb_, frc_, rc=0):
    return (59 << 26) | (frt_ << 21) | (fra_ << 16) | (frb_ << 11) | (frc_ << 6) | (29 << 1) | rc


def fmuls(frt_, fra_, frc_, rc=0):
    return (59 << 26) | (frt_ << 21) | (fra_ << 16) | (frc_ << 6) | (25 << 1) | rc


def fadds(frt_, fra_, frb_, rc=0):
    return (59 << 26) | (frt_ << 21) | (fra_ << 16) | (frb_ << 11) | (21 << 1) | rc


def psq_lx(frs_, ra_, rb_, xo=6):
    return (4 << 26) | (frs_ << 21) | (ra_ << 16) | (rb_ << 11) | (xo << 1)


# --- synthetic ELF ------------------------------------------------------------------------------

def _strtab(names):
    off, buf = {}, bytearray(b"\0")
    for n in names:
        if n and n not in off:
            off[n] = len(buf)
            buf += n.encode() + b"\0"
    return bytes(buf), off


def build_obj(text, functions, relocs=(), data=b""):
    """A minimal ELF32 big-endian relocatable object.

    text      : bytes of `.text`
    functions : [(name, offset, size)]          STT_FUNC symbols in `.text`
    relocs    : [(offset, symbol, type)]        `.rela.text` entries (symbol may be undefined)
    data      : bytes of `.rodata` (optional)
    """
    syms = [(None, 0, 0, 0)]
    for name, off, size in functions:
        syms.append((name, off, size, (2 << 4) | 2))            # global FUNC, shndx .text
    for _off, sym, _typ in relocs:
        if sym and all(s[0] != sym for s in syms):
            syms.append((sym, 0, 0, (2 << 4) | 0))              # global NOTYPE, undefined

    secs = [(".text", text, SHF_ALLOC | SHF_EXECINSTR)]
    if data:
        secs.append((".rodata", data, SHF_ALLOC))
    if relocs:
        secs.append((".rela.text", None, 0))
    secs += [(".symtab", None, 0), (".strtab", None, 0), (".shstrtab", None, 0)]

    sym_names = [s[0] or "" for s in syms]
    strtab, str_off = _strtab(sym_names)
    sym_index = {s[0]: i for i, s in enumerate(syms)}
    symtab = bytearray()
    for name, val, size, info in syms:
        symtab += struct.pack(">IIIBBH", str_off.get(name or "", 0), val, size, info, 0,
                              1 if (info >> 4 == 2) else 0)
    rela = bytearray()
    for off, sym, typ in relocs:
        rela += struct.pack(">IIi", off, (sym_index[sym] << 8) | typ, 0)

    shstr = bytearray(b"\0")
    sh_name = {}
    for name, _d, _f in secs:
        sh_name[name] = len(shstr)
        shstr += name.encode() + b"\0"

    payloads = []
    for name, data_, _flags in secs:
        if name == ".symtab":
            payloads.append(bytes(symtab))
        elif name == ".strtab":
            payloads.append(strtab)
        elif name == ".rela.text":
            payloads.append(bytes(rela))
        elif name == ".shstrtab":
            payloads.append(bytes(shstr))
        else:
            payloads.append(data_ or b"")

    ehsize, shentsize = 52, 40
    off = ehsize
    offsets = []
    for p in payloads:
        off = (off + 3) & ~3
        offsets.append(off)
        off += len(p)
    shoff = (off + 3) & ~3
    shnum = len(secs) + 1
    out = bytearray(shoff + shnum * shentsize)

    out[0:4] = b"\x7fELF"
    out[4], out[5], out[6] = 1, 2, 1                      # 32-bit, big-endian, version 1
    struct.pack_into(">HHIIII", out, 16, 1, 20, 1, 0, 0, shoff)   # type, machine, version, entry, phoff, shoff
    struct.pack_into(">HHHHHH", out, 40, ehsize, 0, 0, shentsize, shnum, len(secs))
    for i, p in enumerate(payloads):
        out[offsets[i]:offsets[i] + len(p)] = p
    for i, (name, _d, flags) in enumerate(secs, start=1):
        o = shoff + i * shentsize
        typ = {".symtab": SHT_SYMTAB, ".strtab": SHT_STRTAB,
               ".shstrtab": SHT_STRTAB, ".rela.text": SHT_RELA}.get(name, SHT_PROGBITS)
        link = info = 0
        if name == ".symtab":
            info = 1                                       # first global symbol index
        struct.pack_into(">IIIIIIIIII", out, o, sh_name[name], typ, flags, 0,
                         offsets[i - 1], len(payloads[i - 1]), link, info, 4, 0)
    # fix up the two link fields now that every section index is known
    shndx = {n: i for i, (n, _d, _f) in enumerate(secs, start=1)}
    for i, (name, _d, _f) in enumerate(secs, start=1):
        o = shoff + i * shentsize
        if name == ".symtab":
            struct.pack_into(">I", out, o + 24, shndx[".strtab"])
        if name == ".rela.text":
            struct.pack_into(">II", out, o + 24, shndx[".symtab"], shndx[".text"])
    return bytes(out)


# --- tests --------------------------------------------------------------------------------------

def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def test_decoder():
    check(infer.rlwinm_alias(clrlwi(3, 4, 24)) == "clrlwi", "clrlwi alias")
    check(infer.rlwinm_alias(srwi(3, 4, 8)) == "srwi", "srwi alias")
    check(infer.rlwinm_alias(rlwinm(3, 4, 0, 0, 31)) == "rlwinm", "plain rlwinm")
    check(infer.rlwinm_alias(clrlwi(3, 4, 24, rc=1)) == "clrlwi", "record clrlwi is still clrlwi")
    i = infer.Insn(0, clrlwi(3, 4, 24, rc=1))
    check(i.is_record() and i.is_clr_mask(), "record + clr mask")
    check(infer.Insn(0, srwi(3, 4, 8)).is_shift(), "shift predicate")
    check(infer.Insn(0, stb(4, 3)).is_narrow_store(), "narrow store predicate")
    check(infer.Insn(0, stmw(31, 1)).op == 47, "stmw opcode")
    check(infer.Insn(0, lmw(31, 1)).op == 46, "lmw opcode")
    check(infer.Insn(0, psq_lx(31, 1, 0)).is_psq_indexed(), "psq_lx predicate")
    check(infer.Insn(0, fmadds(1, 2, 3, 4)).is_fma(), "fmadds predicate")
    check(not infer.Insn(0, fmuls(1, 2, 3)).is_fma(), "fmuls is not an FMA")
    check(infer.Insn(0, fmuls(1, 2, 3)).is_fmul(), "fmuls predicate")
    check(infer.Insn(0, fadds(1, 2, 3)).is_faddsub(), "fadds predicate")
    check(infer.Insn(0, bl()).is_branch_link(), "bl predicate")
    check(infer.Insn(0, lis(3)).is_lis(), "lis predicate")


def test_ground_truth_parser():
    cf = infer.parse_cflags(["-O4,p", "-inline auto", "-str reuse", "-str reuse,pool,readonly",
                             "-fp_contract on"])
    check(cf["str"] == "reuse,pool,readonly", "last -str wins")
    check(cf["inline"] == "auto", "inline parsed")
    check(cf["fp_contract"] == "on", "fp_contract parsed")
    cf = infer.parse_cflags(["-O3", "-opt nopeephole", "-opt nopeephole,level=4"])
    check("nopeephole" in cf["opt"] and cf["opt"]["level"] == "4", "merged -opt sub-options")

    exp = infer.expected_flags(["-opt nopeephole", "-fp_contract on"], {"peephole": ["off"]}, True)
    check(exp["peephole"] == "off" and exp["fp_contract"] == "on", "documented cflags + pragma")
    exp = infer.expected_flags(["-func_align 4"], {"function_align": ["16"]}, True)
    check(exp["func_align"] == "16", "a source function_align pragma beats -func_align 4")
    exp = infer.expected_flags(["-func_align 4"], {}, True)
    check(exp["func_align"] == "4", "no pragma -> the cflags group")
    exp = infer.expected_flags(["-O3"], {"peephole": ["off", "reset", "on"]}, False)
    check(exp["peephole"] == "off", "a scoped off pragma is a record even with a later reset")
    check("fp_contract" not in exp, "an auto unit with no pragma records no fp_contract")


def test_synthetic_fingerprints():
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        def write(name, code):
            p = os.path.join(td, name)
            with open(p, "wb") as f:
                f.write(code)
            return p

        # peephole off: a kept clrlwi before a narrowing store + an unfused srwi+clrlwi
        text = b"".join(struct.pack(">I", w) for w in
                        [clrlwi(3, 4, 24), stb(4, 3), srwi(3, 5, 8), clrlwi(5, 6, 16)])
        p = write("peep.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["peephole"][0] == "off" and got["peephole"][1] == "medium",
              "kept fold -> peephole off, medium: %r" % (got["peephole"],))

        # peephole on: a record form
        text = b"".join(struct.pack(">I", w) for w in [clrlwi(3, 4, 24, rc=1)])
        p = write("rec.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: f["value"] for f in fp.findings}
        check(got["peephole"] == "on", "record form -> peephole on")

        # fp_contract on: a fused FMA
        text = b"".join(struct.pack(">I", w) for w in [fmadds(1, 2, 3, 4)])
        p = write("fma.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["fp_contract"][0] == "on" and got["fp_contract"][1] == "high", "FMA -> on")

        # fp_contract off is only a low-confidence hint
        text = b"".join(struct.pack(">I", w) for w in [fmuls(0, 0, 1), fadds(2, 0, 3)])
        p = write("chain.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["fp_contract"][0] == "off" and got["fp_contract"][1] == "low",
              "unfused chain -> low hint")

        # peephole mixed: a record form plus a kept `lwz`+`addi` load-update pair (the pass folds
        # it into `lwzu`) - a scoped `#pragma peephole off`.  Must be a hint, not a confident `on`.
        text = b"".join(struct.pack(">I", w) for w in
                        [clrlwi(3, 4, 24, rc=1), lwz(12, 3, 0x10), addi(3, 3, 0x10)])
        p = write("mixed_peep.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["peephole"] == ("mixed", "low"),
              "record form + kept load-update -> mixed hint: %r" % (got["peephole"],))

        # fp_contract mixed: a fused FMA in one function, an unfused chain in another - a scoped
        # `#pragma fp_contract off`.  Hint only.
        text = b"".join(struct.pack(">I", w) for w in
                        [fmadds(1, 2, 3, 4), fmuls(0, 0, 1), fadds(2, 0, 3)])
        p = write("mixed_fpc.o", build_obj(text, [("f", 0, len(text))]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["fp_contract"] == ("mixed", "low"),
              "FMA + unfused chain -> mixed hint: %r" % (got["fp_contract"],))

        # func_align 16 with gap padding is only a hint: a -func_align 4 unit can look the same.
        text = b"".join(struct.pack(">I", w) for w in [0] * 5)
        p = write("align16.o", build_obj(text, [("a", 0, 4), ("gap_x", 4, 12), ("b", 16, 4)]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["func_align"] == ("16", "low"),
              "16-aligned starts + padding -> low hint: %r" % (got["func_align"],))

        # inline noauto is a hint: a kept bl to a tiny same-object function is not proof (auto is a
        # heuristic, and a cross-TU callee in a multi-TU split object is never inlined either way).
        text = b"".join(struct.pack(">I", w) for w in [bl(4), 0])
        p = write("tinycall.o", build_obj(text, [("caller", 0, 4), ("tiny", 4, 4)],
                                          relocs=[(0, "tiny", 10)]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: (f["value"], f["confidence"]) for f in fp.findings}
        check(got["inline"] == ("noauto", "low"),
              "kept tiny same-object call -> low hint: %r" % (got["inline"],))

        # lmw_stmw off: an EABI save helper call
        text = b"".join(struct.pack(">I", w) for w in [bl(4)])
        p = write("save.o", build_obj(text, [("f", 0, len(text))], relocs=[(0, "_savegpr_14", 10)]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: f["value"] for f in fp.findings}
        check(got["lmw_stmw"] == "off", "EABI save helper -> lmw_stmw off")

        # func_align 4: two functions, the second off a 16-byte boundary
        text = b"".join(struct.pack(">I", w) for w in [0] * 3)
        p = write("align.o", build_obj(text, [("a", 0, 4), ("b", 12, 4)]))
        fp = infer.Fingerprint(p)
        got = {f["flag"]: f["value"] for f in fp.findings}
        check(got["func_align"] == "4", "misaligned second function -> func_align 4")

        # pool off: two object-local arrays each with their own lis+addi pair
        text = b"".join(struct.pack(">I", w) for w in
                        [lis(3), addi(4, 3), lis(5), addi(6, 5)])
        p = write("pool.o", build_obj(text, [("f", 0, len(text))],
                                      relocs=[(0, "t1", 6), (4, "t1", 4), (8, "t2", 6), (12, "t2", 4)]))
        # t1/t2 are undefined here, so the detector should abstain rather than claim `off`
        fp = infer.Fingerprint(p)
        got = {f["flag"]: f["value"] for f in fp.findings}
        check(got["pool"] == "unknown", "external data symbols are not pool evidence")


def test_known_objects():
    """If the split objects exist, the fingerprints must agree with the campaign's records."""
    obj = os.path.join(ROOT, "build", "RMHE08", "obj")
    cam = os.path.join(obj, "Camellia", "camellia.o")
    rso = os.path.join(obj, "RSO", "runtime.o")
    if not (os.path.exists(cam) and os.path.exists(rso)):
        print("  (skipped: build/RMHE08/obj not present)")
        return
    got = {f["flag"]: f["value"] for f in infer.Fingerprint(cam).findings}
    check(got["pool"] == "off", "Camellia -pool off")
    check(got["lmw_stmw"] == "off", "Camellia -use_lmw_stmw off")
    check(got["func_align"] == "4", "Camellia -func_align 4")
    check(got["peephole"] == "off", "Camellia -opt nopeephole")
    got = {f["flag"]: f["value"] for f in infer.Fingerprint(rso).findings}
    check(got["peephole"] == "on", "RSO keeps the peephole")
    check(got["lmw_stmw"] == "off", "RSO -use_lmw_stmw off")
    check(got["func_align"] == "4", "RSO -func_align 4")


def test_no_confident_miss():
    obj = os.path.join(ROOT, "build", "RMHE08", "obj")
    if not os.path.isdir(obj):
        print("  (skipped: build/RMHE08/obj not present)")
        return
    _rows, summary = infer.accuracy_report(ROOT)
    bad = {lever: s["misses"] for lever, s in summary.items() if s["misses"]}
    check(not bad, "confident claims must not miss a documented case: %r" % bad)


def main():
    tests = [test_decoder, test_ground_truth_parser, test_synthetic_fingerprints,
             test_known_objects, test_no_confident_miss]
    for t in tests:
        t()
        print("ok  %s" % t.__name__)
    print("all %d checks passed" % len(tests))
    return 0


if __name__ == "__main__":
    sys.exit(main())
