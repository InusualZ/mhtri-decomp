#!/usr/bin/env python3
"""Deterministic self-test for tools/units/linkorder.py.

    python tools/units/linkorder_selftest.py

No build, no `ninja` and no repository state: the fixture is a hand-built ELF32 big-endian image and the
DOL it should reproduce, both written by this file, so the contract is pinned - which sections become DOL
slots, how the slot size is derived, where the first divergence is reported, and who it is attributed to -
instead of being re-derived from whatever `build/RMHE08/main.elf` happens to contain today.

The real-data counterpart is the tool itself run against the green link, which reports MATCH.
"""
from __future__ import annotations

import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import linkorder as lo  # noqa: E402  (imported through the sys.path shim above)

ALIGN = 0x20
TEXT_ADDR = 0x80004000
CTORS_ADDR = 0x80005000
DATA_ADDR = 0x80005100
BSS_ADDR = 0x80006000


def align(value: int) -> int:
    return (value + ALIGN - 1) // ALIGN * ALIGN


def build_elf(content: list[dict], symbols: list[dict], entry: int) -> bytes:
    """A minimal valid ELF32 big-endian with one PT_LOAD per content section and a symtab."""
    names = [s["name"] for s in content] + [".symtab", ".strtab", ".shstrtab"]
    shstr, shstr_off = bytearray(b"\0"), {}
    for name in names:
        shstr_off[name] = len(shstr)
        shstr += name.encode() + b"\0"
    strtab, str_off = bytearray(b"\0"), {}
    for sym in symbols:
        str_off[sym["name"]] = len(strtab)
        strtab += sym["name"].encode() + b"\0"
    index = {s["name"]: i + 1 for i, s in enumerate(content)}
    symtab = bytearray(16)
    for sym in symbols:
        info = (sym.get("bind", 1) << 4) | sym.get("type", 2)
        symtab += struct.pack(">IIIBBH", str_off[sym["name"]], sym["value"], sym["size"], info, 0, index[sym["section"]])

    off = 52
    for section in content:
        if section["typ"] == 8:                      # NOBITS carries no file bytes
            section["offset"] = 0
            continue
        off = align(off)                             # file gaps are the DOL's padding, and they are zero
        section["offset"] = off
        off += len(section["data"])
    symtab_off, off = off, off + len(symtab)
    strtab_off, off = off, off + len(strtab)
    shstrtab_off, off = off, off + len(shstr)
    phoff, off = off, off + 32 * len(content)
    shoff, off = off, off + 40 * (len(content) + 4)
    buf = bytearray(off)

    struct.pack_into(">4sBBBBB7s", buf, 0, b"\x7fELF", 1, 2, 1, 0, 0, b"\0" * 7)
    struct.pack_into(">HHIIIIIHHHHHH", buf, 0x10, 2, 20, 1, entry, phoff, shoff, 0, 52, 32,
                     len(content), 40, len(content) + 4, len(content) + 3)
    for section in content:
        if section["typ"] != 8:
            buf[section["offset"]:section["offset"] + len(section["data"])] = section["data"]
    buf[symtab_off:symtab_off + len(symtab)] = symtab
    buf[strtab_off:strtab_off + len(strtab)] = strtab
    buf[shstrtab_off:shstrtab_off + len(shstr)] = shstr
    for i, section in enumerate(content):
        filesz = 0 if section["typ"] == 8 else section["size"]
        struct.pack_into(">8I", buf, phoff + i * 32, 1, section["offset"], section["addr"], 0,
                         filesz, section["size"], 5, section["align"])

    def shdr(i, name_off, typ, flags, addr, offset, size, link, info, align_, entsize):
        struct.pack_into(">10I", buf, shoff + i * 40, name_off, typ, flags, addr, offset, size, link, info, align_, entsize)

    shdr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    for i, section in enumerate(content):
        shdr(i + 1, shstr_off[section["name"]], section["typ"], section["flags"], section["addr"],
             section["offset"], section["size"], 0, 0, section["align"], 0)
    symtab_index = len(content) + 1
    shdr(symtab_index, shstr_off[".symtab"], 2, 0, 0, symtab_off, len(symtab), symtab_index + 1, 1, 4, 16)
    shdr(symtab_index + 1, shstr_off[".strtab"], 3, 0, 0, strtab_off, len(strtab), 0, 0, 1, 0)
    shdr(symtab_index + 2, shstr_off[".shstrtab"], 3, 0, 0, shstrtab_off, len(shstr), 0, 0, 1, 0)
    return bytes(buf)


def build_dol(text: list[tuple[int, bytes]], data: list[tuple[int, bytes]],
              bss_addr: int, bss_size: int, entry: int) -> bytes:
    """The DOL the original build produced: header + slots, each slot the size rounded up to 0x20."""
    header = bytearray(0x100)
    cur = 0x100
    t_off, t_addr, t_size = [], [], []
    for addr, blob in text:
        size = align(len(blob))
        t_off.append(cur); t_addr.append(addr); t_size.append(size); cur += size
    d_off, d_addr, d_size = [], [], []
    for addr, blob in data:
        size = align(len(blob))
        d_off.append(cur); d_addr.append(addr); d_size.append(size); cur += size
    body = bytearray(cur)
    for i, (_addr, blob) in enumerate(text):
        body[t_off[i]:t_off[i] + len(blob)] = blob
    for i, (_addr, blob) in enumerate(data):
        body[d_off[i]:d_off[i] + len(blob)] = blob
    for i in range(7):
        if i < len(t_off):
            struct.pack_into(">I", header, i * 4, t_off[i])
            struct.pack_into(">I", header, 0x48 + i * 4, t_addr[i])
            struct.pack_into(">I", header, 0x90 + i * 4, t_size[i])
    for i in range(11):
        if i < len(d_off):
            struct.pack_into(">I", header, 0x1C + i * 4, d_off[i])
            struct.pack_into(">I", header, 0x64 + i * 4, d_addr[i])
            struct.pack_into(">I", header, 0xAC + i * 4, d_size[i])
    struct.pack_into(">I", header, 0xD8, bss_addr)
    struct.pack_into(">I", header, 0xDC, bss_size)
    struct.pack_into(">I", header, 0xE0, entry)
    body[:0x100] = header
    return bytes(body)


TEXT = bytes(range(0x40))
CTORS = struct.pack(">II", TEXT_ADDR, TEXT_ADDR + 0x10)
DATA = bytes(range(0x20))


def fixture() -> tuple[dict, bytes]:
    content = [
        {"name": ".text", "typ": 1, "flags": 0x6, "addr": TEXT_ADDR, "size": 0x40, "align": 4, "data": TEXT},
        {"name": ".ctors", "typ": 1, "flags": 0x2, "addr": CTORS_ADDR, "size": 0x8, "align": 4, "data": CTORS},
        {"name": ".data", "typ": 1, "flags": 0x3, "addr": DATA_ADDR, "size": 0x20, "align": 4, "data": DATA},
        {"name": ".bss", "typ": 8, "flags": 0x3, "addr": BSS_ADDR, "size": 0x20, "align": 4, "data": b""},
    ]
    symbols = [
        {"name": "fn_a", "value": TEXT_ADDR, "size": 0x10, "type": 2, "section": ".text"},
        {"name": "fn_b", "value": TEXT_ADDR + 0x10, "size": 0x30, "type": 2, "section": ".text"},
        {"name": "ref_a", "value": CTORS_ADDR, "size": 4, "type": 1, "section": ".ctors"},
        {"name": "ref_b", "value": CTORS_ADDR + 4, "size": 4, "type": 1, "section": ".ctors"},
        {"name": "obj_x", "value": DATA_ADDR, "size": 4, "type": 1, "section": ".data"},
    ]
    elf = lo.parse_elf(build_elf(content, symbols, TEXT_ADDR))
    dol = build_dol([(TEXT_ADDR, TEXT)], [(CTORS_ADDR, CTORS), (DATA_ADDR, DATA)], BSS_ADDR, 0x20, TEXT_ADDR)
    return elf, dol


SPLITS = [
    {"unit": "Mod/a.c", "ranges": [
        {"section": ".text", "start": TEXT_ADDR, "end": TEXT_ADDR + 0x40, "rename": None},
        {"section": ".ctors", "start": CTORS_ADDR, "end": CTORS_ADDR + 4, "rename": ".ctors$10"},
        {"section": ".bss", "start": BSS_ADDR, "end": BSS_ADDR + 0x20, "rename": None}]},
    {"unit": "Mod/b.c", "ranges": [
        {"section": ".ctors", "start": CTORS_ADDR + 4, "end": CTORS_ADDR + 8, "rename": None}]},
]
CONFIGURED = {"Mod/a.c": {"flag": "Matching"}, "Mod/b.c": {"flag": "NonMatching"}}
ORIG_SYMBOLS = {
    ".text": [{"name": "fn_a", "address": TEXT_ADDR, "size": 0x10},
              {"name": "fn_b", "address": TEXT_ADDR + 0x10, "size": 0x30}],
    ".ctors": [{"name": "ref_a", "address": CTORS_ADDR, "size": 4},
               {"name": "ref_b", "address": CTORS_ADDR + 4, "size": 4}],
}
AUTO = [{"addr": BSS_ADDR, "section": "bss", "name": "auto_00_80006000_bss",
         "object": "build/RMHE08/obj/auto_00_80006000_bss.o"}]


def corrupt(dol: bytes, offset: int, value: int = 0xAA) -> bytes:
    bad = bytearray(dol)
    bad[offset] = value
    return bytes(bad)


def rows():
    elf, dol = fixture()
    image = lo.dol_image(elf)

    yield "elf entry", elf["entry"], TEXT_ADDR
    yield "section names", [s["name"] for s in elf["sections"]], ["", ".text", ".ctors", ".data", ".bss", ".symtab", ".strtab", ".shstrtab"]
    yield "symbol lookup", lo.symbol_at(elf, TEXT_ADDR + 0x10)["name"], "fn_b"
    yield "content sections", [s["name"] for s in lo.content_sections(elf)], [".text", ".ctors", ".data"]
    yield "nobits sections", [s["name"] for s in lo.nobits_sections(elf)], [".bss"]
    yield "slot sizes", [(s["name"], s["slot_size"]) for s in image["slots"]], [(".text", 0x40), (".ctors", 0x20), (".data", 0x20)]
    yield "bss span", (image["bss_addr"], image["bss_size"]), (BSS_ADDR, 0x20)
    yield "reconstruction equals the original DOL", image["image"] == dol, True

    report = lo.audit_link(elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "green audit ok", report["ok"], True
    yield "green audit has no divergence", report["first"], None
    yield "green audit order ok", report["order"]["ok"], True
    yield "green audit row count", len(report["sections"]), 3
    yield "row sizes", [(r["name"], r["dol_size"], r["elf_size"]) for r in report["sections"]], \
        [(".text", 0x40, 0x40), (".ctors", 0x20, 0x8), (".data", 0x20, 0x20)]
    yield "header ok", report["header"]["ok"], True

    # .ctors is the second data slot; its file offset is header + .text slot = 0x100 + 0x40
    bad = corrupt(dol, 0x140)
    report = lo.audit_link(elf, bad, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "corrupt .ctors detected", report["ok"], False
    yield "corrupt .ctors is the first divergence", (report["first"]["section"], report["first"]["addr"]), (".ctors", CTORS_ADDR)
    yield "corrupt .ctors diff count", report["first"]["diff_count"], 1
    yield "corrupt .ctors owner unit", report["first"]["unit"], "Mod/a.c"
    yield "corrupt .ctors owner object", report["first"]["object"], "build/RMHE08/src/Mod/a.c.o"
    yield "corrupt .ctors linked symbol", report["first"]["linked_symbol"], "ref_a"

    # .data is the third data slot: header + .text + .ctors + .data slots = 0x100 + 0x40 + 0x20
    bad = corrupt(dol, 0x164)
    report = lo.audit_link(elf, bad, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "corrupt .data address", report["first"]["addr"], DATA_ADDR + 4
    yield "corrupt .data is unclaimed", (report["first"]["unit"], report["first"]["object"]), (None, None)

    # a DOL whose .ctors address moved: order, not bytes
    moved = build_dol([(TEXT_ADDR, TEXT)], [(CTORS_ADDR + 0x20, CTORS), (DATA_ADDR, DATA)], BSS_ADDR, 0x20, TEXT_ADDR)
    report = lo.audit_link(elf, moved, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "moved slot breaks order", report["order"]["ok"], False
    yield "moved slot row is not address-ok", report["sections"][1]["addr_ok"], False

    # an extra code section the DOL has no slot for
    content = [
        {"name": ".text", "typ": 1, "flags": 0x6, "addr": TEXT_ADDR, "size": 0x40, "align": 4, "data": TEXT},
        {"name": ".text2", "typ": 1, "flags": 0x6, "addr": TEXT_ADDR + 0x40, "size": 0x20, "align": 4, "data": DATA},
        {"name": ".ctors", "typ": 1, "flags": 0x2, "addr": CTORS_ADDR, "size": 0x8, "align": 4, "data": CTORS},
        {"name": ".data", "typ": 1, "flags": 0x3, "addr": DATA_ADDR, "size": 0x20, "align": 4, "data": DATA},
        {"name": ".bss", "typ": 8, "flags": 0x3, "addr": BSS_ADDR, "size": 0x20, "align": 4, "data": b""},
    ]
    symbols = [{"name": "fn_a", "value": TEXT_ADDR, "size": 0x10, "type": 2, "section": ".text"}]
    extra_elf = lo.parse_elf(build_elf(content, symbols, TEXT_ADDR))
    report = lo.audit_link(extra_elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "section the DOL has no slot for", [r["name"] for r in report["sections"]
                                              if r["group"] == "text" and r["dol_size"] == 0], [".text2"]
    yield "extra code section breaks the audit", report["ok"], False

    # more than seven code sections: the ones past the DOL's last text slot land in `extra`
    many = [{"name": ".t%d" % i, "typ": 1, "flags": 0x6, "addr": TEXT_ADDR + i * 0x20, "size": 0x20,
              "align": 4, "data": bytes(0x20)} for i in range(8)]
    many.append({"name": ".bss", "typ": 8, "flags": 0x3, "addr": BSS_ADDR, "size": 0x20, "align": 4, "data": b""})
    many_elf = lo.parse_elf(build_elf(many, [{"name": "fn_a", "value": TEXT_ADDR, "size": 0x10, "type": 2,
                                             "section": ".t0"}], TEXT_ADDR))
    report = lo.audit_link(many_elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "sections past the seven text slots", [e["name"] for e in report["extra"]], [".t7"]

    # a corrupted DOL header field
    bad = bytearray(dol)
    struct.pack_into(">I", bad, 0xE0, 0x80009999)
    report = lo.audit_link(elf, bytes(bad), SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, "elf", "dol")
    yield "header divergence is named", (report["first"]["where"], report["first"]["field"]), ("header", "entry point")
    yield "header divergence breaks the audit", report["ok"], False

    # duplicate claims across units
    dups = lo.duplicate_claims(SPLITS + [{"unit": "Mod/c.c", "ranges": [
        {"section": ".ctors", "start": CTORS_ADDR, "end": CTORS_ADDR + 4, "rename": None}]}])
    yield "duplicate claim found", [(d["section"], d["units"]) for d in dups], [(".ctors", ["Mod/a.c", "Mod/c.c"])]

    # --unit: a fragment of a concatenated slot is the order-sensitive case
    unit = lo.unit_report("Mod/a.c", elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, image)
    yield "unit ok on the green link", unit["ok"], True
    yield "unit claims three ranges", len(unit["ranges"]), 3
    yield "unit .text is not a fragment", unit["ranges"][0]["fragment"], False
    yield "unit .ctors is a fragment", (unit["ranges"][1]["fragment"], unit["ranges"][1]["order_sensitive"]), (True, True)
    yield "unit reports the other claimant", unit["ranges"][1]["others"], ["Mod/b.c"]
    yield "unit bss range has no bytes to compare", (unit["ranges"][2]["bss"], unit["ranges"][2]["bytes_ok"]), (True, None)
    yield "unit is order-sensitive", unit["order_sensitive"], True
    yield "unit linked object is the src object", unit["linked_object"], "build/RMHE08/src/Mod/a.c.o"
    yield "unit symbols pair by name", unit["symbols_ok"], True

    bad = corrupt(dol, 0x140)
    unit = lo.unit_report("Mod/a.c", elf, bad, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, image)
    yield "unit sees the corrupt region", unit["ok"], False

    # a NonMatching unit links dtk's target object, not ours
    unit_b = lo.unit_report("Mod/b.c", elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, image)
    yield "NonMatching unit links the target object", unit_b["linked_object"], "build/RMHE08/obj/Mod/b.c.o"
    yield "NonMatching unit is also order-sensitive", unit_b["order_sensitive"], True

    # an unknown unit is refused, not silently green
    missing = lo.unit_report("Nope/nope.c", elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO, image)
    yield "unknown unit is not ok", missing["ok"], False

    # a map name the link does not carry is called out (playbook row 31)
    renamed = dict(ORIG_SYMBOLS)
    renamed[".text"] = [{"name": "old_name", "address": TEXT_ADDR, "size": 0x10}]
    unit = lo.unit_report("Mod/a.c", elf, dol, SPLITS, CONFIGURED, renamed, AUTO, image)
    yield "unpaired map name is reported", unit["symbols_ok"], False

    # rendering says which way it went
    yield "render match", "RESULT MATCH" in lo.render(lo.audit_link(elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO), []), True
    yield "render diverge", "RESULT DIVERGE" in lo.render(lo.audit_link(elf, corrupt(dol, 0x140), SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO), []), True
    # a report whose staleness check ran but found nothing must still render (the wrapper dict is
    # truthy either way, so testing it instead of the inner flag made every run raise KeyError)
    _fresh = lo.audit_link(elf, dol, SPLITS, CONFIGURED, ORIG_SYMBOLS, AUTO)
    _fresh["stale"] = {"stale": False, "newest_object": "build/RMHE08/src/x.o"}
    yield "render not-stale", "RESULT MATCH" in lo.render(_fresh, []), True
    _old = dict(_fresh)
    _old["stale"] = {"stale": True, "elf_time": 1.0, "newest_time": 2.0,
                     "newest_object": "build/RMHE08/src/x.o", "detail": "x.o is newer than main.elf"}
    yield "render stale warns", "WARN  x.o is newer than main.elf" in lo.render(_old, []), True


def main() -> int:
    failures = 0
    for item in rows():
        label, got, want = item
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
