#!/usr/bin/env python3
"""Deterministic self-test for the link-wide checks in tools/units/flipcheck.py (the `.comment`
active-flags row-36 check and the extab/extabindex map-symbol link check).

    python tools/units/flipcheck_selftest.py
    python tools/units/flipcheck.py --selftest

No build, no `ninja` and no repository state: every object is a fixture ELF32 big-endian image written by
this file, so the contract is pinned - how a `.comment` entry maps to an ELF symbol, that entries are paired
by name (the target and our object order their symbol tables differently), that only an *unreferenced*
symbol is a trim risk, and that metadata sections, 0-size labels and the reverse flag direction are ignored.
The map-symbol check is pinned on the same means: a `@etb_`/`@eti_` symbol only the target defines that
*another* linked object references (and no input, ours included, provides) is a link break.
"""
from __future__ import annotations

import os
import struct
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import flipcheck as fc  # noqa: E402  (imported through the sys.path shim above)

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_STRTAB = 3
SHT_RELA = 4


def _align(n: int, a: int = 4) -> int:
    return (n + a - 1) // a * a


def build_obj(sections, symbols, relocs=(), flags=None, version=0x0E, with_comment=True) -> bytes:
    """A minimal ELF32 big-endian object with a `.comment` table.

    sections : [(name, data)]                  PROGBITS sections, in shndx order
    symbols  : [(name, size, section, info)]   the null symbol is implicit at index 0
    relocs   : [(target_section, offset, name)] become `.rela<target>` (SHT_RELA) sections
    flags    : {symbol_name: active_flags}      drives the `.comment` symbol table
    """
    syms = [(None, 0, None, 0)] + list(symbols)
    comment = bytearray(b"CodeWarrior" + bytes([version]) + b"\0" * (fc.COMMENT_HEADER - 12))
    for name, _size, _section, _info in syms:
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
    for name, _size, _section, _info in syms:
        if name is not None:
            name_off[name] = len(strtab)
            strtab += name.encode() + b"\0"

    sym_index = {s[0]: i for i, s in enumerate(syms)}
    symtab = bytearray()
    for name, size, section, info in syms:
        symtab += struct.pack(">IIIBBH", name_off.get(name, 0), 0, size, info, 0,
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
        struct.pack_into(">IIIIIIIIII", buf, shoff + i * 40, sh_name[n], typ, 0, 0, o, size, 0, 0, 4, entsize)
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
        #     references, and our object cannot provide, is a link break of its own.
        map_tgt = write(tmp, "map_tgt2.o", target_with_flags({"@eti_A": (0xC, "extabindex", 0x13)}, {}))
        base = dict(self_refs=set(), ref_count={"@eti_A": 1}, providers={})
        risks = fc.external_map_symbol_risks("U", map_tgt, ours_clear, **base)
        expect("map symbol risk reported", len(risks), 1)
        expect("map symbol risk names it", ("@eti_A" in risks[0], "undefined" in risks[0]), (True, True))
        expect("map symbol risk names the class", "rename" in risks[0], True)

        # 13. Only a reference from another linked object counts; the target's own relocation does not.
        expect("self-reference is not external",
               fc.external_map_symbol_risks("U", map_tgt, ours_clear,
                                            {"@eti_A"}, {"@eti_A": 1}, {}), [])

        # 14. Our object (or another input) already providing it means the link still resolves.
        expect("our global definition is enough",
               fc.external_map_symbol_risks("U", map_tgt, map_tgt, **base), [])
        other = dict(base, providers={"@eti_A": {os.path.join(tmp, "other.o")}})
        expect("another provider is enough",
               fc.external_map_symbol_risks("U", map_tgt, ours_clear, **other), [])

        # 15. A source-level symbol is not a map fragment, even when externally referenced and absent.
        plain = write(tmp, "plain_tgt.o", target_with_flags({"fn_A": (0x20, ".text", 0x12)}, {}))
        expect("plain symbol ignored",
               fc.external_map_symbol_risks("U", plain, ours_clear, set(), {"fn_A": 1}, {}), [])

    if FAILURES:
        print("\n%d check(s) FAILED" % len(FAILURES))
        return 1
    print("\nall checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
