#!/usr/bin/env python3
"""Deterministic self-test for the link-wide and byte-level checks in tools/units/flipcheck.py (the
`.comment` active-flags row-36 check, the extab/extabindex map-symbol link check, the section-byte count
and permutation naming, and the general undefined-reference check).

    python tools/units/flipcheck_selftest.py
    python tools/units/flipcheck.py --selftest

No build, no `ninja` and no repository state: every object is a fixture ELF32 big-endian image written by
this file, so the contract is pinned - how a `.comment` entry maps to an ELF symbol, that entries are paired
by name (the target and our object order their symbol tables differently), that only an *unreferenced*
symbol is a trim risk, and that metadata sections, 0-size labels and the reverse flag direction are ignored.
The map-symbol check is pinned on the same means: a `@etb_`/`@eti_` symbol only the target defines that
*another* linked object references (and no input, ours included, provides) is reported - now as an
informational note, because `tools/elf/objextab.py` names those symbols in the build (a current object
provides them, and then the check is silent).

The three classes a refusal has to tell apart are pinned here too, on fixtures: an object that exists but
emits none of the compared sections is *not* "no compiled object" (the wording is asserted verbatim, as is
the first-difference line the lanes parse); a same-size section whose symbols all carry their bytes at their
own address is named a permutation, and a size/layout/pad/single-symbol difference is not; and a referenced
name nothing a flip can use defines is reported while the pinned exemptions (defined here, a map row,
another provider, the target's own unresolved reference, the linker script's own symbols) stay silent.

The weaker layout class is pinned next to the strict one, because it is the shape the measured case has
(`Network/NetworkPat`: 11 of 12 symbols at a different address and 571 of 577 differing bytes outside the
symbols' own addresses, the rest inside three of them): a moved symbol carrying a word of its own is still
named, a mislaid minority with a residual elsewhere is not, and neither is a size/length/pad difference.
The byte extraction is pinned too - it reads the object itself, so it cannot skip in silence the way the old
objcopy path did when a worktree had no `MAIN/.pi` to extract into.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.units import flipcheck as fc

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4


def _align(n: int, a: int = 4) -> int:
    return (n + a - 1) // a * a


def build_obj(sections, symbols, relocs=(), flags=None, version=0x0E, with_comment=True) -> bytes:
    """A minimal ELF32 big-endian object with a `.comment` table.

    sections : [(name, data)]                       PROGBITS sections, in shndx order
    symbols  : [(name, size, section, info, value)] the null symbol is implicit at index 0; the optional
                                                    fifth element is `st_value` (the offset inside its
                                                    section; default 0), which the byte-order checks read
    relocs   : [(target_section, offset, name)]     become `.rela<target>` (SHT_RELA) sections
    flags    : {symbol_name: active_flags}           drives the `.comment` symbol table
    """
    syms = [(None, 0, None, 0, 0)] + [(s[0], s[1], s[2], s[3], s[4] if len(s) > 4 else 0) for s in symbols]
    comment = bytearray(b"CodeWarrior" + bytes([version]) + b"\0" * (fc.COMMENT_HEADER - 12))
    for name, _size, _section, _info, _value in syms:
        comment += struct.pack(">I", 4) + bytes([0, (flags or {}).get(name, 0) & 0xFF, 0, 0])

    rela_targets = []
    for target, _off, _name in relocs:
        if target not in rela_targets:
            rela_targets.append(target)
    all_sections = list(sections) + ([(".comment", bytes(comment))] if with_comment else [])
    sec_names = ([""] + [n for n, _ in all_sections] + [".rela" + t for t in rela_targets]
                 + [".symtab", ".strtab", ".shstrtab"])
    index = {n: i for i, n in enumerate(sec_names)}

    strtab = bytearray(b"\0")
    name_off = {None: 0}
    for name, _size, _section, _info, _value in syms:
        if name is not None:
            name_off[name] = len(strtab)
            strtab += name.encode() + b"\0"

    sym_index = {s[0]: i for i, s in enumerate(syms)}
    symtab = bytearray()
    for name, size, section, info, value in syms:
        symtab += struct.pack(">IIIBBH", name_off.get(name, 0), value, size, info, 0,
                              index.get(section, 0) if section else 0)

    rela_data = {t: bytearray() for t in rela_targets}
    for target, offset, name in relocs:
        rela_data[target] += struct.pack(">IIi", offset, (sym_index[name] << 8) | 0, 0)

    data = {n: d for n, d in all_sections}
    for t in rela_targets:
        data[".rela" + t] = bytes(rela_data[t])
    data[".symtab"] = bytes(symtab)
    data[".strtab"] = bytes(strtab)
    shstr = bytearray(b"\0")
    sh_name = {}
    for n in sec_names:
        sh_name[n] = len(shstr)
        shstr += n.encode() + b"\0"
    data[".shstrtab"] = bytes(shstr)

    placed = []
    off = 52
    for n in sec_names:
        if n == "":
            placed.append((n, 0, 0))
            continue
        off = _align(off)
        placed.append((n, off, len(data[n])))
        off += len(data[n])
    shoff = _align(off)
    buf = bytearray(shoff + 40 * len(sec_names))
    struct.pack_into(">4sBBBBB7s", buf, 0, b"\x7fELF", 1, 2, 1, 0, 0, b"\0" * 7)
    struct.pack_into(">HHIIIIIHHHHHH", buf, 0x10, 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                     len(sec_names), len(sec_names) - 1)
    for n, o, size in placed:
        if n:
            buf[o:o + size] = data[n]
    for i, (n, o, size) in enumerate(placed):
        if n == "":
            continue
        typ = SHT_RELA if n.startswith(".rela") else SHT_SYMTAB if n == ".symtab" else \
            SHT_STRTAB if n in (".strtab", ".shstrtab") else SHT_PROGBITS
        entsize = 12 if typ == SHT_RELA else 16 if typ == SHT_SYMTAB else 0
        link = index[".strtab"] if typ == SHT_SYMTAB else 0     # a symbol table names its string table (real ELF)
        struct.pack_into(">IIIIIIIIII", buf, shoff + i * 40, sh_name[n], typ, 0, 0, o, size, link, 0, 4, entsize)
    return bytes(buf)


# --- fixture helpers ---------------------------------------------------------------------------------

def write(tmp: str, name: str, obj: bytes) -> str:
    path = os.path.join(tmp, name)
    with open(path, "wb") as f:
        f.write(obj)
    return path


def target_with_flags(order, flags, version=0x0E):
    """An object whose symbols are `order` (name -> (size, section, info)) with `flags` in `.comment`."""
    symbols = [(n, s, sec, info) for n, (s, sec, info) in order.items()]
    return build_obj([(".text", b"\0" * 0x20), ("extabindex", b"\0" * 12)], symbols,
                     flags=flags, version=version)


# --- checks ------------------------------------------------------------------------------------------

FAILURES: list[str] = []


def expect(label: str, got, want) -> None:
    if got != want:
        FAILURES.append("%s\n      got:  %r\n      want: %r" % (label, got, want))
        print("FAIL  %s" % label)
    else:
        print("ok    %s" % label)


def selftest() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        # 1. `.comment` entries map to ELF symbols by index: name, size, section and active flag.
        tgt = write(tmp, "map_target.o", target_with_flags(
            {"fn_A": (0x20, ".text", 0x12), "fn_B": (0x10, ".text", 0x02), "@eti_A": (0xC, "extabindex", 0x03)},
            {"fn_A": 0x08}))
        entries = fc.comment_symbols(tgt)
        expect("comment_symbols entry count", len(entries), 4)
        expect("comment_symbols fn_A", (entries[1]["name"], entries[1]["size"], entries[1]["section"],
                                        entries[1]["active_flags"]), ("fn_A", 0x20, ".text", 0x08))
        expect("comment_symbols fn_B flag", entries[2]["active_flags"], 0x00)
        expect("comment_symbols @eti section", entries[3]["section"], "extabindex")
        expect("comment_symbols missing .comment", fc.comment_symbols(os.path.join(tmp, "nope.o")), None)

        # 2. Pairing is by name, not index: target [fn_A, fn_B] flags A, ours [fn_B, fn_A] flags B.
        #    An index-based compare would call both safe; the name compare must flag fn_A.
        ours_swapped = write(tmp, "swap_ours.o", target_with_flags(
            {"fn_B": (0x10, ".text", 0x02), "fn_A": (0x20, ".text", 0x12)}, {"fn_B": 0x08}))
        problems, checked, compared = fc.comment_trim_risks("U", tgt, ours_swapped, set())
        expect("name pairing: compared", compared, True)
        expect("name pairing: checked count", checked, 1)
        expect("name pairing: flags fn_A only",
               (len(problems), "fn_A" in problems[0], "fn_B" in problems[0]), (1, True, False))

        # 3. A code/data reference keeps the symbol; an extabindex reference does not.
        code = build_obj([(".text", b"\0" * 4), ("extabindex", b"\0" * 12)],
                         [("fn_A", 0x20, ".text", 0x12), ("fn_B", 0x20, ".text", 0x12)],
                         relocs=[(".text", 0, "fn_A"), ("extabindex", 0, "fn_B")])
        code_path = write(tmp, "code.o", code)
        expect("code_references_in excludes metadata", fc.code_references_in(code_path), {"fn_A"})
        refdir = os.path.join(tmp, "refs")
        os.makedirs(refdir)
        write(refdir, "code.o", code)
        refs, count = fc.code_reference_index([refdir])
        expect("code_reference_index reads the dir", (sorted(refs), count), (["fn_A"], 1))
        expect("referenced symbol is safe", fc.comment_trim_risks("U", tgt, ours_swapped, {"fn_A"})[0], [])

        # 4. An unreferenced, target-exported symbol our object leaves clear is the trim risk.
        ours_clear = write(tmp, "clear_ours.o", target_with_flags(
            {"fn_A": (0x20, ".text", 0x12)}, {}))
        problems, checked, compared = fc.comment_trim_risks("U", tgt, ours_clear, set())
        expect("trim risk reported", len(problems), 1)
        expect("trim risk names the symbol and size", ("fn_A" in problems[0], "0x20" in problems[0]), (True, True))
        expect("trim risk mentions the fix", "__declspec(export)" in problems[0], True)

        # 4b. `check` folds the row-36 risks into ONE summary line (count, first six names); `verbose` adds the
        #     per-symbol lines after it, each the `comment_trim_risks` wording.
        many = [("fn_%d" % i, 0x10 + i) for i in range(8)]
        summary = fc.row36_lines("U", many)
        expect("row 36: one line for eight risks", len(summary), 1)
        expect("row 36: the line names the count and the first six, then `...`",
               (summary[0].startswith("row 36: 8 function(s) force-active in retail .comment, not in ours: "
                                      "fn_0, fn_1, fn_2, fn_3, fn_4, fn_5, ... (first 6)"),
                "fn_6" in summary[0], "--verbose" in summary[0], "__declspec(export)" in summary[0]),
               (True, False, True, True))
        verbose = fc.row36_lines("U", many, verbose=True)
        expect("row 36 --verbose: the summary then one line per symbol",
               (len(verbose), verbose[0].split(" - ")[0] == summary[0].split(" - ")[0], verbose[1:]),
               (9, True, [fc.ROW36_SYMBOL_LINE % ("U", n, s) for n, s in many]))
        expect("row 36: six or fewer risks are all named, no `...`",
               ("..." in fc.row36_lines("U", many[:6])[0], "fn_5" in fc.row36_lines("U", many[:6])[0]), (False, True))
        expect("row 36: no risk, no line", fc.row36_lines("U", []), [])
        expect("comment_trim_risks keeps the per-symbol wording",
               problems, [fc.ROW36_SYMBOL_LINE % ("U", "fn_A", 0x20)])
        saved = (fc.MAIN, fc.SRC)
        fc.MAIN, fc.SRC = tmp, os.path.join(tmp, "build", "RMHE08", "src")
        try:
            for side, flags in (("obj", {n: 0x08 for n, _s in many}), ("src", {})):
                os.makedirs(os.path.join(tmp, "build", "RMHE08", side, "M"), exist_ok=True)
                write(os.path.join(tmp, "build", "RMHE08", side, "M"), "u.o",
                      build_obj([(".text", b"\0" * 0x100)], [(n, s, ".text", 0x12) for n, s in many], flags=flags))
            claim = {".text": (0x100, 2)}
            got = fc.check("M/u", claim, set())[0]
            expect("check: eight row-36 risks are one refusal line", (len(got), got[0].startswith("row 36: 8 ")),
                   (1, True))
            expect("check --verbose: summary plus eight", len(fc.check("M/u", claim, set(), verbose=True)[0]), 9)
        finally:
            fc.MAIN, fc.SRC = saved

        # 5. A 0-size label has nothing to trim.
        zero = write(tmp, "zero.o", target_with_flags({"fn_A": (0x0, ".text", 0x12)}, {"fn_A": 0x08}))
        expect("0-size symbol ignored", fc.comment_trim_risks("U", zero, ours_clear, set())[0], [])

        # 6. An extab/extabindex symbol is bookkeeping, not trimmable code.
        meta = write(tmp, "meta.o", target_with_flags({"@eti_A": (0xC, "extabindex", 0x03)}, {"@eti_A": 0x08}))
        expect("extabindex symbol ignored", fc.comment_trim_risks("U", meta, ours_clear, set())[0], [])

        # 6b. A `.ctors$10` / `.dtors$10` fragment word is a linker root, not trimmable code.
        ctor = write(tmp, "ctor.o", build_obj(
            [(".text", b"\0" * 4), (".ctors$10", b"\0" * 4)],
            [("__init_cpp_exceptions", 0x3C, ".text", 0x12),
             ("__init_cpp_exceptions_reference", 0x4, ".ctors$10", 0x11)],
            flags={"__init_cpp_exceptions_reference": 0x08}))
        expect("`.ctors$NN` fragment ignored",
               fc.comment_trim_risks("U", ctor, ours_clear, set())[0], [])

        # 6c. A `.ctors` relocation DOES root its target (a static constructor is a real reference).
        ctor_ref = write(tmp, "ctor_ref.o", build_obj(
            [(".text", b"\0" * 4), (".ctors$10", b"\0" * 4)],
            [("__init_cpp_exceptions", 0x3C, ".text", 0x12)],
            relocs=[(".ctors$10", 0, "__init_cpp_exceptions")]))
        expect("`.ctors` relocation roots its target",
               fc.code_references_in(ctor_ref), {"__init_cpp_exceptions"})

        # 6d. FORCEACTIVE roots are read from the linker script.
        lcf = write(tmp, "ldscript.lcf",
                    b"SECTIONS\n{\n}\n\nFORCEACTIVE\n{\n    __nw__FUl\n    hbm_InitGX__Fv\n}\n")
        expect("forced_active parses the block", fc.forced_active(lcf), {"__nw__FUl", "hbm_InitGX__Fv"})
        expect("forced_active without a script", fc.forced_active(os.path.join(tmp, "nope.lcf")), set())
        expect("entry symbols are roots", "__start" in fc.ENTRY_SYMBOLS, True)

        # 7. The reverse direction (ours exports, target does not) is not a trim risk.
        rev_tgt = write(tmp, "rev_tgt.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {}))
        rev_ours = write(tmp, "rev_ours.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {"fn_A": 0x08}))
        expect("reverse direction ignored", fc.comment_trim_risks("U", rev_tgt, rev_ours, set())[0], [])

        # 8. A `.comment` version byte difference (0x0e target vs 0x0f ours) is not a flag difference.
        ver_tgt = write(tmp, "ver_tgt.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {}, version=0x0E))
        ver_ours = write(tmp, "ver_ours.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {}, version=0x0F))
        expect("version byte ignored", fc.comment_trim_risks("U", ver_tgt, ver_ours, set())[0], [])

        # 9. A missing `.comment` on either side disables the check instead of guessing.
        no_comment = write(tmp, "no_comment.o", build_obj([(".text", b"\0" * 4)], [("fn_A", 0x20, ".text", 0x12)],
                                                         with_comment=False))
        expect("no .comment: skipped", fc.comment_trim_risks("U", tgt, no_comment, set()), ([], 0, False))

        # 10. A global binding is what the linker can use across objects; a local one cannot.
        expect("provides_global: global",
               fc.provides_global({"S": ("extabindex", 0x13)}, "S"), True)
        expect("provides_global: weak",
               fc.provides_global({"S": ("extabindex", 0x23)}, "S"), True)
        expect("provides_global: local",
               fc.provides_global({"S": ("extabindex", 0x03)}, "S"), False)
        expect("provides_global: absent", fc.provides_global({}, "S"), False)

        # 11. `link_inputs` reads the link edge and stops at the implicit dependencies (`|`).
        ninja = write(tmp, "build.ninja",
                      b"rule link\n  command = x\n\n"
                      b"build build\\RMHE08\\main.elf: link build\\RMHE08\\obj\\a.o $\n"
                      b"    build\\RMHE08\\src\\b.o | $\n"
                      b"    build\\RMHE08\\ldscript.lcf build\\compilers || post-compile\n")
        saved = fc.NINJA
        fc.NINJA = ninja
        try:
            got = fc.link_inputs()
        finally:
            fc.NINJA = saved
        expect("link_inputs parses the edge", got,
               [os.path.normpath("build\\RMHE08\\obj\\a.o".replace("\\", os.sep)),
                os.path.normpath("build\\RMHE08\\src\\b.o".replace("\\", os.sep))])
        expect("link_inputs stops at the implicit deps", [x for x in (got or []) if "ldscript" in x], [])
        fc.NINJA = os.path.join(tmp, "nope.ninja")
        try:
            expect("link_inputs without build.ninja", fc.link_inputs(), None)
        finally:
            fc.NINJA = saved

        # 12. The resfile-flip class: a map fragment only the target defines that another linked object
        #     references, and our object cannot provide, is reported - as a note, never a refusal.
        map_tgt = write(tmp, "map_tgt2.o", target_with_flags({"@eti_A": (0xC, "extabindex", 0x13)}, {}))
        base = dict(self_refs=set(), ref_count={"@eti_A": 1}, providers={})
        risks = fc.external_map_symbol_notes("U", map_tgt, ours_clear, **base)
        expect("map symbol risk reported", len(risks), 1)
        expect("map symbol risk names it", ("@eti_A" in risks[0], "undefined" in risks[0]), (True, True))
        expect("map symbol risk names the class", "rename" in risks[0], True)
        expect("map symbol risk is not a refusal", "Informational, not a refusal" in risks[0], True)
        expect("map symbol risk names the step", "objextab.py" in risks[0], True)

        # 13. Only a reference from another linked object counts; the target's own relocation does not.
        expect("self-reference is not external",
               fc.external_map_symbol_notes("U", map_tgt, ours_clear,
                                            {"@eti_A"}, {"@eti_A": 1}, {}), [])

        # 14. Our object (or another input) already providing it means the link still resolves.
        expect("our global definition is enough",
               fc.external_map_symbol_notes("U", map_tgt, map_tgt, **base), [])
        other = dict(base, providers={"@eti_A": {os.path.join(tmp, "other.o")}})
        expect("another provider is enough",
               fc.external_map_symbol_notes("U", map_tgt, ours_clear, **other), [])

        # 15. A source-level symbol is not a map fragment, even when externally referenced and absent.
        plain = write(tmp, "plain_tgt.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {}))
        expect("plain symbol ignored",
               fc.external_map_symbol_notes("U", plain, ours_clear, set(), {"fn_A": 1}, {}), [])

        # 16. A section-empty object is not a missing one. `sections()` returns {} for both, but the
        #     object exists and `ninja -n` answers "no work to do" (`NHTTP/NHTTP_os_RVL`: `.comment` and
        #     nothing else), so the refusal has to say what the claim needs instead of "compile it first".
        bodyless = write(tmp, "bodyless.o", build_obj([], []))
        bare_claim = {".text": (0x764, 2)}
        empty_msg = fc.missing_or_empty_object("NHTTP/NHTTP_os_RVL", bodyless, bare_claim)
        expect("section-empty object is not 'no compiled object'",
               [m.startswith("no compiled object") for m in empty_msg], [False])
        expect("section-empty object names the claim and what it does emit",
               ("0x764" in empty_msg[0], ".text" in empty_msg[0], ".comment" in empty_msg[0]),
               (True, True, True))
        expect("missing object keeps its exact wording",
               fc.missing_or_empty_object("U", os.path.join(tmp, "absent.o"), bare_claim),
               ["no compiled object (build/RMHE08/src/U.o) - compile it first"])

        # 17. The differing-byte count, and the permutation class: equal sizes, every symbol's bytes match
        #     at its own address, and the section still differs (`Network/NetworkPat`: 577 of 720 `.text`).
        fn_a, fn_b = b"\x11\x12\x13\x14", b"\x21\x22\x23\x24"
        perm_ours = write(tmp, "perm_ours.o", build_obj(
            [(".text", fn_b + fn_a)],
            [("fn_A", 4, ".text", 0x12, 4), ("fn_B", 4, ".text", 0x12, 0)]))
        perm_tgt = write(tmp, "perm_tgt.o", build_obj(
            [(".text", fn_a + fn_b)],
            [("fn_A", 4, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 4)]))
        lines = fc.section_byte_problems(".text", fn_b + fn_a, fn_a + fn_b, perm_ours, perm_tgt)
        expect("the first-difference line is unchanged", lines[0],
               ".text: bytes differ from the target object at +0x0 (ours 21, target 11) - "
               "the object is not the original's code")
        expect("the differing-byte count is printed", lines[1],
               ".text: 8 of 8 bytes differ from the target object")
        expect("the permutation is named", ("permutation" in lines[2], "2 symbol" in lines[2]), (True, True))

        # 17b. `.data` emission-order seams: the same symbols in another sequence is `order-only` and names the
        #      seams; the objects here are the fixture (a vtable+string TU pair laid out as one TU would).
        vt, st = b"\0" * 8 + b"\x80\x01\x00\x00", b"hello\0\0\0"
        seam_ours = write(tmp, "seam_ours.o", build_obj(
            [(".data", vt + st)], [("__vt__A", 12, ".data", 0x11, 0), ("@1", 8, ".data", 0x01, 12)]))
        seam_tgt = write(tmp, "seam_tgt.o", build_obj(
            [(".data", st + vt)], [("lbl_str", 8, ".data", 0x11, 0), ("__vt__A", 12, ".data", 0x11, 8)]))
        seams = [{"addr": 0x1008, "kind": "V->S"}, {"addr": 0x1100, "kind": "zigzag"}]
        gap_seams = [{"addr": 0x1008, "kind": "V->S", "latest": 0x1080, "width": 30, "tail": 0, "cut": 0x1008}]
        got = fc.data_seam_problems("U/u", ".data", seam_ours, seam_tgt, seams, (0x1000, 0x1200))
        expect("a reordered .data is order-only and names the seams",
               (len(got), "order-only: the unit spans several TUs; seams: at 0x00001008, at 0x00001100" in got[0]),
               (1, True))
        got = fc.data_seam_problems("U/u", ".data", seam_ours, seam_tgt, gap_seams, (0x1000, 0x1200))
        expect("a V->S gap says 'a boundary in [a, b)', not a position",
               (len(got), "seams: a boundary in [0x00001008, 0x00001080)" in got[0]), (1, True))
        expect("a range with no seam adds no line",
               fc.data_seam_problems("U/u", ".data", seam_ours, seam_tgt, seams, (0x2000, 0x2100)), [])
        expect("another section adds no line",
               fc.data_seam_problems("U/u", ".text", seam_ours, seam_tgt, seams, (0x1000, 0x1200)), [])
        # 17c. pool sharing: a differing `.sdata2` of a unit in a pool-sharing group is a partial pool of one TU
        pool_ours = write(tmp, "pool_ours.o", build_obj(
            [(".sdata2", b"\0" * 8)], [("@1", 4, ".sdata2", 0x01, 0), ("@2", 4, ".sdata2", 0x01, 4)]))
        pool_tgt = write(tmp, "pool_tgt.o", build_obj(
            [(".sdata2", b"\0" * 16)], [("lbl_a", 4, ".sdata2", 0x01, 0), ("lbl_b", 4, ".sdata2", 0x01, 4),
                                        ("lbl_c", 4, ".sdata2", 0x01, 8), ("lbl_d", 4, ".sdata2", 0x01, 12)]))
        fold = "candidate fold: U/u with V/v (3 shared pool literal(s), text adjacent, confidence high)"
        got = fc.pool_group_problems("U/u", {".sdata2": (16, 4)}, {".sdata2": (8, 4)}, pool_ours, pool_tgt, fold)
        expect("a short .sdata2 of a unit in a pool group is named a partial pool, with the fold",
               (len(got), "partial pool of a TU that spans several registered units" in got[0], fold in got[0]),
               (1, True, True))
        expect("a matching pool adds no line",
               fc.pool_group_problems("U/u", {".sdata2": (16, 4)}, {".sdata2": (16, 4)}, pool_tgt, pool_tgt, fold), [])
        expect("a section the unit does not claim adds no line",
               fc.pool_group_problems("U/u", {".data": (16, 4)}, {".sdata2": (8, 4)}, pool_ours, pool_tgt, fold), [])
        seam_other = write(tmp, "seam_other.o", build_obj(
            [(".data", st)], [("lbl_str", 8, ".data", 0x11, 0)]))
        got = fc.data_seam_problems("U/u", ".data", seam_other, seam_tgt, seams, (0x1000, 0x1200))
        expect("a .data that differs in more than order gets the multi-TU line, not order-only",
               (len(got), "order-only" in got[0], "spans at least 3 TUs" in got[0]), (1, False, True))
        expect("the strict permutation line is the strict one",
               "every one of the 2 symbol(s)" in lines[2], True)
        expect("the mislaid-layout line is not printed for a strict permutation",
               ["layout is a permutation" in line for line in lines], [False, False, False])
        expect("differing_bytes counts content and length",
               (fc.differing_bytes(b"\x01\x02", b"\x01\x05"), fc.differing_bytes(b"\x01", b"\x01\x02")),
               (1, 1))
        expect("section_symbols reads each symbol's own address",
               fc.section_symbols(perm_tgt, ".text"), {"fn_A": (0, 4), "fn_B": (4, 4)})
        expect("identical sections are not a permutation",
               fc.mislaid_layout(fn_a + fn_b, fn_a + fn_b, perm_ours, perm_tgt, ".text"), None)
        expect("sizes that differ are not a permutation",
               fc.mislaid_layout(fn_b + fn_a, fn_a, perm_ours, perm_tgt, ".text"), None)
        pad_ours = write(tmp, "pad_ours.o", build_obj(
            [(".text", fn_a + fn_b + b"\xAA\x00")],
            [("fn_A", 4, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 4)]))
        expect("unmoved symbols (a pad difference) are not a permutation",
               fc.mislaid_layout(fn_a + fn_b + b"\xAA\x00", fn_a + fn_b + b"\x00\x00",
                                 pad_ours, perm_tgt, ".text"), None)
        bad_ours = write(tmp, "bad_ours.o", build_obj(
            [(".text", fn_a + b"\x99\x22\x23\x24")],
            [("fn_A", 4, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 4)]))
        expect("a symbol whose own bytes differ is not a permutation",
               fc.mislaid_layout(fn_a + b"\x99\x22\x23\x24", fn_a + fn_b, bad_ours, perm_tgt, ".text"), None)
        one_ours = write(tmp, "one_ours.o", build_obj([(".text", fn_b)], [("fn_A", 4, ".text", 0x12, 0)]))
        expect("fewer than two shared symbols is not a permutation",
               fc.mislaid_layout(fn_b, fn_a, one_ours, perm_tgt, ".text"), None)

        # 17b. The mislaid-layout class the strict test cannot see: the section sizes agree and the symbols
        #      are at the addresses the source's definition order gave them, but one moved symbol carries a
        #      word of its own too (`Network/NetworkPat`: 11 of 12 symbols moved, 571 of 577 differing bytes
        #      outside the symbols' own addresses and six inside three of them - 99.7 % per symbol, which no
        #      per-symbol score can turn into an action).
        fn_a_stray = b"\x11\x99\x13\x14"
        mis_ours = write(tmp, "mis_ours.o", build_obj(
            [(".text", fn_b + fn_a_stray)],
            [("fn_A", 4, ".text", 0x12, 4), ("fn_B", 4, ".text", 0x12, 0)]))
        expect("the strict test does not name a moved symbol with a stray word",
               fc.mislaid_layout(fn_b + fn_a_stray, fn_a + fn_b, mis_ours, perm_tgt, ".text"), None)
        expect("mislaid_order returns the moved/comparison/outside/inside counts",
               fc.mislaid_order(fn_b + fn_a_stray, fn_a + fn_b, mis_ours, perm_tgt, ".text"), (2, 2, 7, 1))
        mis_lines = fc.section_byte_problems(".text", fn_b + fn_a_stray, fn_a + fn_b, mis_ours, perm_tgt)
        expect("the mislaid layout is named a permutation",
               (len(mis_lines), "the section's layout is a permutation" in mis_lines[2],
                "2 of the 2 symbol(s)" in mis_lines[2]), (3, True, True))
        expect("the mislaid line counts the bytes the layout does not explain",
               ("7 of the 8 differing bytes" in mis_lines[2], "only 1 differ inside them" in mis_lines[2]),
               (True, True))
        #     A mislaid *minority* is not the class: `ef/ef_effect` has two of its thirty-nine symbols
        #     mislaid (204 of 5564 bytes) with 129 bytes differing inside eight others, and reordering is
        #     not what that section needs.
        minor_ours = write(tmp, "minor_ours.o", build_obj(
            [(".text", fn_b + fn_a_stray + b"\x31" * 16)],
            [("fn_A", 4, ".text", 0x12, 4), ("fn_B", 4, ".text", 0x12, 0), ("fn_C", 16, ".text", 0x12, 8)]))
        minor_tgt = write(tmp, "minor_tgt.o", build_obj(
            [(".text", fn_a + fn_b + b"\x31" * 16)],
            [("fn_A", 4, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 4), ("fn_C", 16, ".text", 0x12, 8)]))
        expect("a mislaid minority with a residual elsewhere is not the class",
               fc.mislaid_order(fn_b + fn_a_stray + b"\x31" * 16, fn_a + fn_b + b"\x31" * 16,
                                minor_ours, minor_tgt, ".text"), None)
        expect("a residual in an unmoved symbol is not a mislaid layout",
               fc.mislaid_order(fn_a_stray + fn_b, fn_a + fn_b, write(tmp, "inplace_ours.o", build_obj(
                   [(".text", fn_a_stray + fn_b)],
                   [("fn_A", 4, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 4)])),
                   perm_tgt, ".text"), None)
        expect("identical sections are not a mislaid layout",
               fc.mislaid_order(fn_a + fn_b, fn_a + fn_b, perm_ours, perm_tgt, ".text"), None)
        expect("a length difference is not a mislaid layout",
               fc.mislaid_order(fn_b + fn_a_stray, fn_a, mis_ours, perm_tgt, ".text"), None)
        #     A symbol that changed *size* is what displaces the addresses after it - a byte residual, not a
        #     layout one - so it is refused outright rather than reasoned about.
        grow_ours = write(tmp, "grow_ours.o", build_obj(
            [(".text", fn_b + fn_a)],
            [("fn_A", 8, ".text", 0x12, 0), ("fn_B", 4, ".text", 0x12, 0)]))
        expect("a symbol whose size changed is not a mislaid layout",
               fc.mislaid_order(fn_b + fn_a, fn_a + fn_b, grow_ours, perm_tgt, ".text"), None)

        # 17c. The byte extraction reads the object itself (`objcompare.section_data`), not objcopy: the old
        #      objcopy path needed a scratch dir that a worktree lacked, `raw_section` returned None for
        #      both sides and every byte check above skipped without a word (`Network/NetworkPat` read READY).
        expect("a section's bytes are read without binutils", fc.raw_section(perm_ours, ".text"), fn_b + fn_a)
        expect("an absent section is None, not empty", fc.raw_section(perm_ours, ".data"), None)
        expect("a missing object is None", fc.raw_section(os.path.join(tmp, "absent.o"), ".text"), None)
        twice = write(tmp, "twice.o", build_obj([(".data", b"\x01" * 4), (".data", b"\x02" * 8)], []))
        expect("a repeated section name reads its last section (what objcopy left on top)",
               (fc.raw_section(twice, ".data"), fc.sections(twice).get(".data", (0,))[0]), (b"\x02" * 8, 8))

        # 18. The general relocation check: every name our object references must be defined by our object,
        #     a `symbols.txt` row, or a link input other than the target object (`Network/NetworkWiiMediator`
        #     is the measured refusal: four constructor names nothing else can supply).
        tgt_rel = os.path.join("build", "RMHE08", "obj", "U.o")
        other_rel = os.path.join("build", "RMHE08", "obj", "other.o")
        ref_ours = write(tmp, "ref_ours.o", build_obj(
            [(".text", b"\0" * 0x20)],
            [("fn_A", 0x20, ".text", 0x12, 0), ("defined_here", 4, ".text", 0x12, 0x1C),
             ("gone", 0, None, 0x10), ("target_only", 0, None, 0x10),
             ("provided_elsewhere", 0, None, 0x10), ("mapped_name", 0, None, 0x10),
             ("relay_only", 0, None, 0x10), ("already_referenced", 0, None, 0x10),
             ("misspelled", 0, None, 0x10)],
            relocs=[(".text", 0, "gone"), (".text", 4, "defined_here"),
                    (".text", 8, "target_only"), (".text", 0xC, "provided_elsewhere"),
                    (".text", 0x10, "mapped_name"), (".text", 0x14, "relay_only"),
                    (".text", 0x18, "already_referenced"), (".text", 0x1C, "misspelled")]))
        ref_tgt = write(tmp, "ref_tgt.o", build_obj(
            [(".text", b"\0" * 0x20)],
            [("fn_A", 0x20, ".text", 0x12, 0), ("target_only", 4, ".text", 0x12, 0x10),
             ("relay_only", 0, None, 0x10), ("misspelled__Fv", 0, None, 0x10)]))
        ctx = {"refs": {tgt_rel: {"relay_only", "misspelled__Fv"},
                        other_rel: {"already_referenced"}},
               "ref_count": {"gone": 0, "target_only": 0, "provided_elsewhere": 1,
                             "relay_only": 1, "misspelled__Fv": 1, "already_referenced": 1},
               "providers": {"target_only": {tgt_rel}, "provided_elsewhere": {other_rel}}}
        found = fc.undefined_reference_problems("U", tgt_rel, ref_tgt, ref_ours, ctx, {"mapped_name"})
        expect("undefined references are one line naming the names", len(found), 1)
        expect("the reported names are the undefined ones",
               ("gone" in found[0], "target_only" in found[0], "misspelled" in found[0]), (True, True, True))
        expect("a defined, mapped, provided or already-referenced name is silent",
               tuple(n in found[0] for n in ("defined_here", "provided_elsewhere", "mapped_name",
                                             "relay_only", "already_referenced")),
               (False, False, False, False, False))
        expect("the spelling hint names the target's variant", "misspelled__Fv" in found[0], True)
        expect("the refusal names the count and the undefined line",
               ("3 referenced symbol(s)" in found[0], "`undefined: 'gone'`" in found[0]), (True, True))

        # 19. The linker's own symbols are not a flip's to define: an lcf assignment (`_stack_addr`) and the
        #     EABI small-data bases (`_SDA_BASE_`, referenced by `Runtime.PPCEABI.H/__start`).
        lcf = write(tmp, "flipcheck.lcf", b"SECTIONS\n{\n    _stack_addr = 0x80004000;\n}\n")
        saved_lcf = fc.LDSCRIPT
        fc.LDSCRIPT = lcf
        try:
            lcf_ours = write(tmp, "lcf_ours.o", build_obj(
                [(".text", b"\0" * 8)],
                [("fn_A", 8, ".text", 0x12, 0), ("_stack_addr", 0, None, 0x10), ("_SDA_BASE_", 0, None, 0x10)],
                relocs=[(".text", 0, "_stack_addr"), (".text", 4, "_SDA_BASE_")]))
            no_providers = {"refs": {tgt_rel: set()}, "ref_count": {}, "providers": {}}
            expect("linker-provided names are not undefined references",
                   fc.undefined_reference_problems("U", tgt_rel, ref_tgt, lcf_ours, no_providers, set()), [])
        finally:
            fc.LDSCRIPT = saved_lcf
        expect("map_symbols reads the map's rows", fc.map_symbols(
            write(tmp, "symbols.txt", b"foo = .text:0x80004000; // type:function size:0x10\n"
                                       b"// comment\n\nbar = .text:0x80004010;\n")), {"foo", "bar"})
        expect("linker_assigned reads the script's assignments", fc.linker_assigned(lcf), {"_stack_addr"})
        expect("map_symbols without a file", fc.map_symbols(os.path.join(tmp, "nope.txt")), set())

    if FAILURES:
        print("\n%d check(s) FAILED" % len(FAILURES))
        return 1
    print("\nall checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
