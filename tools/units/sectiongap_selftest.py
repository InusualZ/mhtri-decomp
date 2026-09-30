#!/usr/bin/env python3
"""Fixtures-only selftest for tools/units/sectiongap.py.

    python tools/units/sectiongap_selftest.py
    python tools/units/sectiongap.py --selftest

No build, no `ninja` and no repository state: every object is an ELF32 big-endian image written by this
file, so the contract is pinned - how a `.rela<target>` section maps to the section it relocates, how a
symbol index resolves to a name, that metadata sections are excluded by default, and that a same-size
record whose relocations moved is reported with **both** offset sets (F41) while a short record is
reported with both sizes (F39). A section present on one side, a differing byte and a reloc name on one
side only are pinned too; `compare_objects` is checked to be silent on identical objects.
"""
from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import sectiongap as sg  # noqa: E402  (imported through the sys.path shim above)

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA = 1, 2, 3, 4
CHECK_COUNT = 0
FAILURES: list[str] = []


def _check(name: str, got, want) -> None:
    global CHECK_COUNT
    CHECK_COUNT += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


def _contains(name: str, needle: str, haystack: str) -> None:
    global CHECK_COUNT
    CHECK_COUNT += 1
    if needle not in haystack:
        FAILURES.append("%s: %r not found in %r" % (name, needle, haystack))


def build_elf(content, symbols, relocs=()) -> bytes:
    """A minimal ELF32 big-endian object.

    content : [(section name, bytes)]                PROGBITS sections, in shndx order
    symbols : [(name, shndx, size)]                  the null symbol is implicit at index 0, so these
                                                     start at index 1; `shndx` is a 1-based section index
    relocs  : [(target section name, offset, type, symbol index[, addend])]
    """
    n = len(content)
    target_names = []
    for entry in relocs:
        tgt = entry[0]
        if tgt not in target_names:
            target_names.append(tgt)
    shstr = bytearray(b"\0")
    shstr_off: dict[str, int] = {}

    def _shstr(text: str) -> int:
        if text not in shstr_off:
            shstr_off[text] = len(shstr)
            shstr.extend(text.encode("latin1") + b"\0")
        return shstr_off[text]

    # every section name has to be in `.shstrtab` before the section headers are built (the
    # `.shstrtab` entry itself is one of them), so register them all first
    for name, _data in content:
        _shstr(name)
    for extra in (".symtab", ".strtab", ".shstrtab"):
        _shstr(extra)
    for tgt in target_names:
        _shstr(".rela" + tgt)

    strtab = bytearray(b"\0")
    sym_name_off = []
    for name, _shndx, _size in symbols:
        sym_name_off.append(len(strtab))
        strtab.extend(name.encode("latin1") + b"\0")

    symtab = bytearray(b"\0" * 16)                       # null symbol
    for i, (_name, shndx, size) in enumerate(symbols):
        symtab.extend(struct.pack(">IIIBBH", sym_name_off[i], 0, size, 0x11, 0, shndx))

    symtab_idx, strtab_idx, shstr_idx = n + 1, n + 2, n + 3
    section_index = {name: i + 1 for i, (name, _data) in enumerate(content)}
    rela: dict[str, bytes] = {}
    for tgt in target_names:
        rows = bytearray()
        for entry in relocs:
            if entry[0] != tgt:
                continue
            _t, off, typ, sym = entry[:4]
            addend = entry[4] if len(entry) > 4 else 0
            rows.extend(struct.pack(">IIi", off, (sym << 8) | typ, addend))
        rela[tgt] = bytes(rows)

    entries = [(name, SHT_PROGBITS, data, 0, 0, 0, 4) for name, data in content]
    entries.append((".symtab", SHT_SYMTAB, bytes(symtab), strtab_idx, 0, 16, 4))
    entries.append((".strtab", SHT_STRTAB, bytes(strtab), 0, 0, 0, 1))
    entries.append((".shstrtab", SHT_STRTAB, bytes(shstr), 0, 0, 0, 1))
    for tgt in target_names:
        entries.append((".rela" + tgt, SHT_RELA, rela[tgt], symtab_idx, section_index[tgt], 12, 4))

    offset, blobs = 52, []
    for _name, _typ, data, _link, _info, _ent, _align in entries:
        offset += (-offset) % 4
        blobs.append(offset)
        offset += len(data)
    shoff = offset + (-offset) % 4

    header = (b"\x7fELF" + bytes([1, 2, 1, 0]) + b"\0" * 8
              + struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                            len(entries) + 1, shstr_idx))
    body = bytearray(header)
    body.extend(b"\0" * (shoff - len(body)))
    for blob, (_name, _typ, data, _link, _info, _ent, _align) in zip(blobs, entries):
        if len(body) < blob:
            body.extend(b"\0" * (blob - len(body)))
        body[blob:blob + len(data)] = data
    if len(body) < shoff:
        body.extend(b"\0" * (shoff - len(body)))
    body.extend(b"\0" * 40)                              # null section header
    for blob, (name, typ, data, link, info, entsize, align) in zip(blobs, entries):
        body.extend(struct.pack(">IIIIIIIIII", _shstr(name), typ, 0, 0, blob, len(data),
                                link, info, align, entsize))
    return bytes(body)


def _write(tmp: str, name: str, blob: bytes) -> str:
    path = os.path.join(tmp, name)
    with open(path, "wb") as fh:
        fh.write(blob)
    return path


def _obj(extab_offsets, extab_size=8, text=b"\x01\x02\x03\x04"):
    """A two-section fixture object: `extab` (with `__dl__FPv` relocations) and `.text` (`fn_A`)."""
    relocs = [("extab", off, 1, 1) for off in extab_offsets]
    return build_elf([("extab", b"\0" * extab_size), (".text", text)],
                     [("__dl__FPv", 0, 0), ("fn_A", 2, len(text))], relocs)


def selftest() -> int:
    import tempfile

    with tempfile.TemporaryDirectory() as tmp:
        # --- read_object: `.rela<target>` -> target section, symbol name resolution, metadata skip -----
        target = sg.read_object(_write(tmp, "target.o", _obj([0x2c, 0x34, 0x54])))
        _check("a `.relaextab` section is read as `extab`'s relocations", sorted(target["relocs"]), ["extab"])
        _check("the symbol index resolves to its name",
               target["relocs"]["extab"], [(0x2C, 1, "__dl__FPv"), (0x34, 1, "__dl__FPv"),
                                            (0x54, 1, "__dl__FPv")])
        _check("content sections are kept, metadata is not",
               sorted(target["sections"]), [".text", "extab"])
        _check("the target's section order is the header order", target["order"], ["extab", ".text"])
        _check("a section's size and bytes survive the read", target["sections"][".text"],
               {"size": 4, "data": b"\x01\x02\x03\x04"})
        all_target = sg.read_object(_write(tmp, "all.o", _obj([0x2c])), all_sections=True)
        _contains("--all-sections adds the symbol/string tables", ".symtab", all_target["sections"])

        # --- F41: the right size, the relocations at the wrong offsets ---------------------------------
        ours = sg.read_object(_write(tmp, "ours.o", _obj([0x14, 0xac, 0xb4])))
        rows = sg.compare_objects(ours, target)
        _check("a same-size, moved-relocation record is one row", [r["section"] for r in rows], ["extab"])
        _check("the row carries both sizes", (rows[0]["ours"], rows[0]["target"]), (8, 8))
        _contains("the reason names our offsets", "+0x14, +0xAC, +0xB4", rows[0]["why"])
        _contains("the reason names the target's offsets", "+0x2C, +0x34, +0x54", rows[0]["why"])
        _contains("the reason names the relocated symbol", "`__dl__FPv`", rows[0]["why"])
        _contains("the reason calls it a relocation difference", "relocations for", rows[0]["why"])

        # --- pool sharing: a differing literal pool of a unit in a pool group says it is a partial pool ---
        pool_rows = [{"section": ".sdata2", "ours": 8, "target": 16, "why": "target-extra 0x8 (8 B)"},
                     {"section": "extab", "ours": 8, "target": 8, "why": "bytes differ"}]
        fold = "candidate fold: A/a with B/b (3 shared pool literal(s), text adjacent, confidence high)"
        noted = sg.add_pool_notes([dict(r) for r in pool_rows], fold)
        _contains("a differing .sdata2 row names the partial pool and the fold", "partial pool of a TU", noted[0]["why"])
        _contains("... with the fold line", fold, noted[0]["why"])
        _check("another section's row is untouched", noted[1]["why"], "bytes differ")
        _check("no fold line, no note", sg.add_pool_notes([dict(r) for r in pool_rows], None), pool_rows)

        # --- F39: the short record, both sizes in one row ----------------------------------------------
        short = {"sections": {"extab": {"size": 0x2E4, "data": b"\0" * 0x2E4}},
                 "order": ["extab"], "relocs": {"extab": []}}
        full = {"sections": {"extab": {"size": 0x4EC, "data": b"\0" * 0x4EC}},
                "order": ["extab"], "relocs": {"extab": []}}
        rows = sg.compare_objects(short, full)
        _check("a short record is one row", len(rows), 1)
        _check("the short record names both sizes", (rows[0]["ours"], rows[0]["target"]), (0x2E4, 0x4EC))
        _contains("the short record is target-extra", "target-extra 0x208 (520 B)", rows[0]["why"])

        # --- a clean pair is silent --------------------------------------------------------------------
        _check("identical objects are silent", sg.compare_objects(ours, ours), [])

        # --- the smaller differences a lane still needs to see ------------------------------------------
        left = {"sections": {"extab": {"size": 8, "data": b"ABCDEFGH"}},
                "order": ["extab"], "relocs": {"extab": []}}
        right = {"sections": {"extab": {"size": 8, "data": b"ABCXEFGH"}},
                 "order": ["extab"], "relocs": {"extab": []}}
        why = sg.compare_objects(left, right)[0]["why"]
        _contains("a byte difference quotes the offset and both bytes", "bytes differ at +0x3 (ours 44, target 58)", why)
        _contains("a byte difference counts the differing bytes", "1 of 8 bytes", why)

        only_ours = {"sections": {"extab": {"size": 4, "data": b"\0" * 4}},
                     "order": ["extab"], "relocs": {"extab": []}}
        _contains("an ours-only section is ours-extra",
                  "ours-extra",
                  sg.compare_objects(only_ours, {"sections": {}, "order": [], "relocs": {}})[0]["why"])
        _contains("a target-only section is missing",
                  "missing from our object",
                  sg.compare_objects({"sections": {}, "order": [], "relocs": {}}, only_ours)[0]["why"])

        # a reloc name on one side only is named, not counted as a move
        mine = {"sections": {"extab": {"size": 8, "data": b"\0" * 8}}, "order": ["extab"],
                "relocs": {"extab": [(0, 1, "mine_only")]}}
        theirs = {"sections": {"extab": {"size": 8, "data": b"\0" * 8}}, "order": ["extab"],
                  "relocs": {"extab": [(4, 1, "target_only")]}}
        why = sg.compare_objects(mine, theirs)[0]["why"]
        _contains("a reloc name only ours has is named", "ours relocates `mine_only` at +0x0", why)
        _contains("a reloc name only the target has is named", "the target relocates `target_only` at +0x4", why)

        # reloc type is carried next to the offset, so a 6/4 pair reads apart
        typed = sg.reloc_reasons([(0x66, 6, "cb"), (0x6A, 4, "cb")], [(0x66, 6, "cb"), (0x6A, 4, "cb")])
        _check("identical typed relocations are silent", typed, [])
        typed = " ".join(sg.reloc_reasons([(0x66, 6, "cb")], [(0x66, 4, "cb")]))
        _contains("a changed reloc type is named", "R_PPC_ADDR16_HA", typed)
        _contains("... with the other side's type", "R_PPC_ADDR16_LO", typed)

        # the cap keeps a section with hundreds of moved relocations to one line
        many_mine = [(0x10 * i, 1, "fn_%d" % i) for i in range(20)]
        many_theirs = [(0x10 * i + 4, 1, "fn_%d" % i) for i in range(20)]
        capped = sg.reloc_reasons(many_mine, many_theirs)
        _check("a many-symbol diff is capped", capped[-1], "... and 14 more relocation difference(s)")
        _check("the cap keeps the requested head", len(capped), sg.MAX_RELOC_DIFFS + 1)

    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECK_COUNT)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
