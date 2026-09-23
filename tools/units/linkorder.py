#!/usr/bin/env python3
"""Link-order audit for flips (roadmap 7.19): does the linked ELF still reproduce the original DOL?

    python tools/units/linkorder.py                  # whole link: order, addresses, bytes, first divergence
    python tools/units/linkorder.py --unit <unit>    # one unit's claimed region (works before a flip too)
    python tools/units/linkorder.py --json           # the same report as data, for a batch gate

Why this exists
---------------
A flip substitutes our object for dtk's split object, so the only proof that the *link* is still the
original is `ninja build/RMHE08/ok`. When it goes red, `dtk dol diff` names a symbol and a count of
differing bytes - not the unit or the object responsible - and `build/RMHE08/main.MAP` cannot help: it is
a stale 2024 artefact, because dtk's link does not write one. So this tool reconstructs the DOL image
from the linked `build/RMHE08/main.elf` alone - section headers, PT_LOAD contents, entry point and the
NOBITS span - and compares it to `orig/RMHE08/sys/main.dol`. No relink, no `ninja`, nothing written.

What it checks
--------------
order     the ELF's code/data sections in address order against the DOL's text0..text6 / data0..data10
          slots: same count, same addresses, same sizes. A DOL slot is the section's size rounded up to
          0x20, the original's file-offset alignment (measured on the green link: 10/10 slots).
bytes     every slot, from the ELF's file bytes, against the DOL's - the whole `dtk dol diff`, bucketed
          per section, with the first differing byte and the count per section.
header    the reconstructed DOL header against the original's, field by field.
For the first divergence it prints both words, the original symbol (from `symbols.txt`) and the linked
symbol (from the ELF symtab), then the unit from `splits.txt` and the object `configure.py` links for it.

`--unit` scopes the same audit to the ranges one unit's `splits.txt` entry claims. That is the pre-flip
check: before a flip the region holds dtk's target object and matches, afterwards it holds ours. It also
reports whether a claimed range is a *fragment* of a shared `.ctors`/`.dtors` slot - the link-order case
roadmap 7.19 is about, which `flipcheck.py` cannot see - and it names the object the link actually used
(`src/` for a `Matching` unit, the target `obj/` for a `NonMatching` one). `extab`/`extabindex` fragments
are not flagged: `g3d/g3d_resanmamblight.c` is green with one. The content half of the proof (object vs
target object) stays `flipcheck.py`'s job; this tool owns the link shape.

`land.py` does not call this yet (it was built under a no-edit-other-tools constraint); the natural hook
is a `linkorder.py --json` step after `ok`, reading `ok` and the per-unit verdicts.

Read-only by construction: it opens the ELF, the DOL, `symbols.txt`, `splits.txt` and `configure.py`.
"""
from __future__ import annotations

import argparse
import bisect
import json
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = "RMHE08"
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))
if os.path.join(ROOT, "tools", "symbols") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "symbols"))

import symbolpreflight as preflight  # noqa: E402  (shares the symbols.txt / splits.txt / configure.py parsers)

ELF_PATH = os.path.join(ROOT, "build", GAME, "main.elf")
DOL_PATH = os.path.join(ROOT, "orig", GAME, "sys", "main.dol")
CONFIG_JSON = os.path.join(ROOT, "build", GAME, "config.json")
SRC_DIR = os.path.join(ROOT, "build", GAME, "src")
OBJ_DIR = os.path.join(ROOT, "build", GAME, "obj")

DOL_HEADER = 0x100
DOL_ALIGN = 0x20               # every DOL slot size / file offset is a multiple of 0x20 (measured, green link)
DOL_TEXT_SLOTS = 7
DOL_DATA_SLOTS = 11

SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHT_SYMTAB = 2
SHT_NOBITS = 8
STT_SECTION = 3
PT_LOAD = 1

# `.ctors`/`.dtors` are the link-order case (roadmap 7.19 / `.pi/notes/ctors-rule.md`): the linker builds
# their tables through a built-in path, not by concatenating fragments whose addresses the map pins. A unit
# whose claim is a proper subset of one of those slots is the one to watch. `extab`/`extabindex` are also
# concatenated but their fragments flip green (`g3d/g3d_resanmamblight.c`), so they are not flagged.
FRAGMENT_SECTIONS = (".ctors", ".dtors")
AUTO_RE = re.compile(r"^auto_\d+_([0-9A-Fa-f]{8})_(\w+)$")


# ---------------------------------------------------------------------------------------------------
# little readers: one ELF32 big-endian parser and one DOL header parser, both self-contained
# ---------------------------------------------------------------------------------------------------

def _u16(data: bytes, off: int) -> int:
    return struct.unpack_from(">H", data, off)[0]


def _u32(data: bytes, off: int) -> int:
    return struct.unpack_from(">I", data, off)[0]


def _cstr(data: bytes, off: int) -> str:
    if off < 0 or off >= len(data):
        return ""
    end = data.find(b"\0", off)
    return data[off:end if end >= 0 else len(data)].decode("latin1")


def align_up(value: int, align: int) -> int:
    return (value + align - 1) // align * align


def parse_elf(data: bytes) -> dict:
    """ELF32 big-endian: sections, PT_LOAD segments, the symtab and e_entry. Works for objects too."""
    if len(data) < 0x34 or data[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    if data[4] != 1 or data[5] != 2:
        raise ValueError("expected a 32-bit big-endian ELF (class %d, data %d)" % (data[4], data[5]))
    entry = _u32(data, 0x18)
    phoff, shoff = _u32(data, 0x1C), _u32(data, 0x20)
    phentsize, phnum = _u16(data, 0x2A), _u16(data, 0x2C)
    shentsize, shnum, shstrndx = _u16(data, 0x2E), _u16(data, 0x30), _u16(data, 0x32)

    sections = []
    for i in range(shnum):
        off = shoff + i * shentsize
        name_off, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from(">10I", data, off)
        sections.append({"index": i, "name_off": name_off, "typ": typ, "flags": flags, "addr": addr,
                         "offset": offset, "size": size, "link": link, "info": info, "align": align,
                         "entsize": entsize, "name": ""})
    if 0 <= shstrndx < len(sections):
        base = sections[shstrndx]["offset"]
        for section in sections:
            section["name"] = _cstr(data, base + section["name_off"])

    segments = []
    for i in range(phnum):
        off = phoff + i * phentsize
        typ, offset, vaddr, _paddr, filesz, memsz, flags, align = struct.unpack_from(">8I", data, off)
        segments.append({"typ": typ, "offset": offset, "vaddr": vaddr, "filesz": filesz,
                         "memsz": memsz, "flags": flags, "align": align})

    symbols = []
    for section in sections:
        if section["typ"] != SHT_SYMTAB or section["link"] >= len(sections):
            continue
        strtab = sections[section["link"]]
        base, entsize = strtab["offset"], section["entsize"] or 16
        for j in range(section["size"] // entsize):
            name_off, value, size, info, _other, shndx = struct.unpack_from(">IIIBBH", data, section["offset"] + j * entsize)
            stype = info & 0xF
            name = _cstr(data, base + name_off)
            if not name or stype == STT_SECTION:
                continue
            symbols.append({"name": name, "value": value, "size": size, "type": stype,
                            "bind": info >> 4, "shndx": shndx})
    symbols.sort(key=lambda s: (s["value"], s["size"]))
    return {"data": data, "entry": entry, "sections": sections, "segments": segments, "symbols": symbols,
            "sym_values": [s["value"] for s in symbols]}


def parse_dol(data: bytes) -> dict:
    if len(data) < DOL_HEADER:
        raise ValueError("not a DOL (shorter than its 0x100 header)")
    return {
        "text_offsets": list(struct.unpack_from(">7I", data, 0x00)),
        "data_offsets": list(struct.unpack_from(">11I", data, 0x1C)),
        "text_addrs": list(struct.unpack_from(">7I", data, 0x48)),
        "data_addrs": list(struct.unpack_from(">11I", data, 0x64)),
        "text_sizes": list(struct.unpack_from(">7I", data, 0x90)),
        "data_sizes": list(struct.unpack_from(">11I", data, 0xAC)),
        "bss_addr": _u32(data, 0xD8),
        "bss_size": _u32(data, 0xDC),
        "entry": _u32(data, 0xE0),
    }


def content_sections(elf: dict, require_load: bool = True) -> list[dict]:
    """Allocatable, non-NOBITS, non-empty sections - the ones a DOL slot is made of."""
    loads = [s for s in elf["segments"] if s["typ"] == PT_LOAD]

    def loaded(section: dict) -> bool:
        if not require_load:
            return True
        return any(seg["vaddr"] <= section["addr"] and section["addr"] + section["size"] <= seg["vaddr"] + seg["memsz"]
                   for seg in loads)

    return [s for s in elf["sections"]
            if s["typ"] != SHT_NOBITS and (s["flags"] & SHF_ALLOC) and s["size"] > 0 and loaded(s)]


def nobits_sections(elf: dict) -> list[dict]:
    loads = [s for s in elf["segments"] if s["typ"] == PT_LOAD]
    return [s for s in elf["sections"]
            if s["typ"] == SHT_NOBITS and (s["flags"] & SHF_ALLOC) and s["size"] > 0
            and any(seg["vaddr"] <= s["addr"] and s["addr"] + s["size"] <= seg["vaddr"] + seg["memsz"] for seg in loads)]


def dol_image(elf: dict) -> dict:
    """Reconstruct the DOL the linked ELF would produce: header + slots + the full image bytes.

    The slot size is the section size rounded up to 0x20 and the slot bytes are read from the ELF's own
    file offsets, so the inter-section file gaps (the original's padding) come along exactly as
    `dtk elf2dol` copies them. That is what makes the byte comparison the whole `dtk dol diff`.
    """
    content = content_sections(elf)
    code = sorted([s for s in content if s["flags"] & SHF_EXECINSTR], key=lambda s: s["addr"])
    data = sorted([s for s in content if not (s["flags"] & SHF_EXECINSTR)], key=lambda s: s["addr"])
    nobits = nobits_sections(elf)
    bss_addr = min((s["addr"] for s in nobits), default=0)
    bss_end = max((s["addr"] + s["size"] for s in nobits), default=0)

    slots, cur = [], DOL_HEADER
    for group, sections, limit in (("text", code, DOL_TEXT_SLOTS), ("data", data, DOL_DATA_SLOTS)):
        for i, section in enumerate(sections[:limit]):
            slot_size = align_up(section["size"], DOL_ALIGN)
            slots.append({"group": group, "index": i, "name": section["name"], "addr": section["addr"],
                          "size": section["size"], "slot_size": slot_size, "offset": cur,
                          "file_offset": section["offset"], "section_index": section["index"]})
            cur += slot_size
    extra = [{"group": "text", "name": s["name"], "addr": s["addr"], "size": s["size"]}
             for s in code[DOL_TEXT_SLOTS:]] + \
            [{"group": "data", "name": s["name"], "addr": s["addr"], "size": s["size"]}
             for s in data[DOL_DATA_SLOTS:]]

    header = bytearray(DOL_HEADER)
    for slot in slots:
        base = slot["index"] * 4
        if slot["group"] == "text":
            struct.pack_into(">I", header, base, slot["offset"])
            struct.pack_into(">I", header, 0x48 + base, slot["addr"])
            struct.pack_into(">I", header, 0x90 + base, slot["slot_size"])
        else:
            struct.pack_into(">I", header, 0x1C + base, slot["offset"])
            struct.pack_into(">I", header, 0x64 + base, slot["addr"])
            struct.pack_into(">I", header, 0xAC + base, slot["slot_size"])
    struct.pack_into(">I", header, 0xD8, bss_addr)
    struct.pack_into(">I", header, 0xDC, bss_end - bss_addr)
    struct.pack_into(">I", header, 0xE0, elf["entry"])

    image = bytearray(cur)
    image[:DOL_HEADER] = header
    data_bytes = elf["data"]
    for slot in slots:
        chunk = data_bytes[slot["file_offset"]:slot["file_offset"] + slot["slot_size"]]
        image[slot["offset"]:slot["offset"] + len(chunk)] = chunk

    return {"slots": slots, "extra": extra, "header": bytes(header), "image": bytes(image),
            "bss_addr": bss_addr, "bss_size": bss_end - bss_addr, "entry": elf["entry"]}


# ---------------------------------------------------------------------------------------------------
# address attribution
# ---------------------------------------------------------------------------------------------------

def section_at(elf: dict, addr: int) -> dict | None:
    for section in elf["sections"]:
        if (section["flags"] & SHF_ALLOC) and section["size"] > 0 and section["addr"] <= addr < section["addr"] + section["size"]:
            return section
    return None


def symbol_at(elf: dict, addr: int) -> dict | None:
    i = bisect.bisect_right(elf["sym_values"], addr) - 1
    if i < 0:
        return None
    return elf["symbols"][i]


def read_addr(elf: dict, addr: int, size: int) -> bytes | None:
    section = section_at(elf, addr)
    if section is None or section["typ"] == SHT_NOBITS:
        return None
    if addr + size > section["addr"] + section["size"]:
        return None
    off = section["offset"] + (addr - section["addr"])
    return elf["data"][off:off + size]


def dol_slot_at(dol: dict, addr: int) -> dict | None:
    for group, addrs, sizes, offsets in (("text", dol["text_addrs"], dol["text_sizes"], dol["text_offsets"]),
                                         ("data", dol["data_addrs"], dol["data_sizes"], dol["data_offsets"])):
        for i, (a, size, off) in enumerate(zip(addrs, sizes, offsets)):
            if size and a <= addr < a + size:
                return {"group": group, "index": i, "addr": a, "size": size, "offset": off}
    return None


def dol_read(data: bytes, dol: dict, addr: int, size: int) -> bytes | None:
    slot = dol_slot_at(dol, addr)
    if slot is None or addr + size > slot["addr"] + slot["size"]:
        return None
    off = slot["offset"] + (addr - slot["addr"])
    return data[off:off + size]


def covering_any(splits: list[dict], addr: int) -> dict | None:
    for block in splits:
        for rng in block["ranges"]:
            if rng["start"] <= addr < rng["end"]:
                return {"unit": block["unit"], **rng}
    return None


def orig_symbol_at(by_section: dict, section: str, addr: int) -> dict | None:
    entries = by_section.get(section) or []
    best = None
    for entry in entries:
        if entry["address"] <= addr:
            best = entry
        else:
            break
    return best


def load_auto_units() -> list[dict]:
    """dtk's autogenerated split objects, keyed by the address in their name (build/RMHE08/config.json)."""
    if not os.path.exists(CONFIG_JSON):
        return []
    try:
        units = json.load(open(CONFIG_JSON, "r", encoding="utf-8")).get("units", [])
    except (ValueError, OSError):
        return []
    out = []
    for unit in units:
        m = AUTO_RE.match(unit.get("name", ""))
        if m:
            out.append({"addr": int(m.group(1), 16), "section": m.group(2), "name": unit["name"],
                        "object": unit.get("object", "")})
    out.sort(key=lambda u: u["addr"])
    return out


def nearest_auto(auto_units: list[dict], addr: int) -> dict | None:
    best = None
    for unit in auto_units:
        if unit["addr"] <= addr:
            best = unit
        else:
            break
    return best


def describe(addr: int, elf: dict, splits: list[dict], configured: dict, orig_symbols: dict,
             auto_units: list[dict]) -> dict:
    """Everything the report needs to name who put this byte here."""
    section = section_at(elf, addr)
    section_name = section["name"] if section else None
    linked = symbol_at(elf, addr)
    owner = preflight.covering(splits, section_name, addr) if section_name else None
    if owner is None:
        owner = covering_any(splits, addr)
    unit = owner["unit"] if owner else None
    info = configured.get(unit) or {}
    if unit:
        obj = "build/%s/%s/%s.o" % (GAME, "src" if info.get("flag") == "Matching" else "obj", unit)
    else:
        auto = nearest_auto(auto_units, addr)
        obj = auto["object"] if auto else None
    original = orig_symbol_at(orig_symbols, section_name or "", addr)
    return {
        "addr": addr, "section": section_name,
        "linked_symbol": linked["name"] if linked else None,
        "linked_symbol_value": linked["value"] if linked else None,
        "orig_symbol": original["name"] if original else None,
        "unit": unit, "flag": info.get("flag"), "object": obj,
        "auto": None if unit else (nearest_auto(auto_units, addr) or {}).get("name"),
    }


# ---------------------------------------------------------------------------------------------------
# the audit
# ---------------------------------------------------------------------------------------------------

def _header_field(off: int) -> str:
    if off < 0x1C:
        return "text%d offset" % (off // 4)
    if off < 0x48:
        return "data%d offset" % ((off - 0x1C) // 4)
    if off < 0x64:
        return "text%d address" % ((off - 0x48) // 4)
    if off < 0x90:
        return "data%d address" % ((off - 0x64) // 4)
    if off < 0xAC:
        return "text%d size" % ((off - 0x90) // 4)
    if off < 0xD8:
        return "data%d size" % ((off - 0xAC) // 4)
    if off < 0xDC:
        return "bss address"
    if off < 0xE0:
        return "bss size"
    if off < 0xE4:
        return "entry point"
    return "header padding"


def _row(group: str, index: int, elf_section: dict | None, dol_addrs: list[int], dol_sizes: list[int],
         dol_offsets: list[int], image: dict, dol_data: bytes) -> dict:
    dol_addr = dol_addrs[index] if index < len(dol_addrs) else 0
    dol_size = dol_sizes[index] if index < len(dol_sizes) else 0
    dol_off = dol_offsets[index] if index < len(dol_offsets) else 0
    row = {"group": group, "index": index,
           "name": elf_section["name"] if elf_section else "?",
           "elf_addr": elf_section["addr"] if elf_section else 0,
           "elf_size": elf_section["size"] if elf_section else 0,
           "dol_addr": dol_addr, "dol_size": dol_size, "slot_size": 0,
           "addr_ok": False, "size_ok": False, "bytes_ok": False, "diff_count": 0, "first_diff": None}
    if elf_section is not None:
        row["slot_size"] = align_up(elf_section["size"], DOL_ALIGN)
    slot = next((s for s in image["slots"] if s["group"] == group and s["index"] == index), None)
    if elf_section is not None:
        row["addr_ok"] = elf_section["addr"] == dol_addr
        row["size_ok"] = row["slot_size"] == dol_size
    if slot is None or not dol_size:
        # a section the DOL has no slot for, or an empty DOL slot: the only good case is both absent
        row["bytes_ok"] = slot is None and dol_size == 0
        return row
    mine = image["image"][slot["offset"]:slot["offset"] + slot["slot_size"]]
    theirs = dol_data[dol_off:dol_off + dol_size]
    n = min(len(mine), len(theirs))
    first = next((i for i in range(n) if mine[i] != theirs[i]), None)
    row["first_diff"] = first
    row["diff_count"] = sum(1 for i in range(n) if mine[i] != theirs[i]) + abs(len(mine) - len(theirs))
    row["bytes_ok"] = row["diff_count"] == 0 and len(mine) == len(theirs)
    return row


def audit_link(elf: dict, dol_data: bytes, splits: list[dict], configured: dict, orig_symbols: dict,
               auto_units: list[dict], elf_path: str = "", dol_path: str = "") -> dict:
    dol = parse_dol(dol_data)
    image = dol_image(elf)
    content = content_sections(elf)
    code = sorted([s for s in content if s["flags"] & SHF_EXECINSTR], key=lambda s: s["addr"])
    data = sorted([s for s in content if not (s["flags"] & SHF_EXECINSTR)], key=lambda s: s["addr"])

    rows = []
    for i in range(max(len(code), DOL_TEXT_SLOTS)):
        rows.append(_row("text", i, code[i] if i < len(code) else None,
                         dol["text_addrs"], dol["text_sizes"], dol["text_offsets"], image, dol_data))
    for i in range(max(len(data), DOL_DATA_SLOTS)):
        rows.append(_row("data", i, data[i] if i < len(data) else None,
                         dol["data_addrs"], dol["data_sizes"], dol["data_offsets"], image, dol_data))
    # empty DOL slots with no section on either side are not rows worth printing
    rows = [r for r in rows if not (r["name"] == "?" and r["dol_size"] == 0)]

    header = image["header"]
    original_header = dol_data[:DOL_HEADER]
    header_diff = next((i for i in range(min(len(header), len(original_header))) if header[i] != original_header[i]), None)
    header_ok = header_diff is None and len(header) == len(original_header)

    bss_ok = image["bss_addr"] == dol["bss_addr"] and image["bss_size"] == dol["bss_size"]
    entry_ok = image["entry"] == dol["entry"]

    order_ok = all(r["addr_ok"] for r in rows) and not image["extra"]
    sections_ok = all(r["bytes_ok"] for r in rows)
    ok = bool(order_ok and sections_ok and header_ok and bss_ok and entry_ok and not image["extra"])

    report = {
        "elf": elf_path, "dol": dol_path, "ok": ok,
        "elf_size": len(elf["data"]), "dol_size": len(dol_data),
        "entry": {"elf": image["entry"], "dol": dol["entry"], "ok": entry_ok},
        "bss": {"elf_addr": image["bss_addr"], "elf_size": image["bss_size"],
                "dol_addr": dol["bss_addr"], "dol_size": dol["bss_size"], "ok": bss_ok},
        "header": {"ok": header_ok, "first_diff": header_diff,
                   "field": _header_field(header_diff) if header_diff is not None else None},
        "image_size": len(image["image"]),
        "order": {"ok": order_ok,
                  "text": [{"name": r["name"], "addr": r["elf_addr"], "dol_addr": r["dol_addr"]} for r in rows if r["group"] == "text"],
                  "data": [{"name": r["name"], "addr": r["elf_addr"], "dol_addr": r["dol_addr"]} for r in rows if r["group"] == "data"]},
        "sections": rows,
        "extra": image["extra"],
        "diffs_total": sum(r["diff_count"] for r in rows)
                       + sum(1 for i in range(min(len(header), len(original_header))) if header[i] != original_header[i]),
        "first": None,
        "duplicate_claims": duplicate_claims(splits),
    }

    # the first divergence in DOL file order, named and attributed
    if not header_ok and header_diff is not None:
        report["first"] = {"where": "header", "file_offset": header_diff, "field": _header_field(header_diff),
                           "linked": original_header[header_diff:header_diff + 4].hex(),
                           "original": header[header_diff:header_diff + 4].hex(),
                           "addr": None}
    else:
        candidates = [r for r in rows if not r["bytes_ok"]]
        candidates.sort(key=lambda r: next(s["offset"] for s in image["slots"]
                                           if s["group"] == r["group"] and s["index"] == r["index"]))
        for row in candidates:
            slot = next(s for s in image["slots"] if s["group"] == row["group"] and s["index"] == row["index"])
            at = row["first_diff"] or 0
            addr = slot["addr"] + at
            dol_off = (dol["text_offsets"] if row["group"] == "text" else dol["data_offsets"])[row["index"]]
            word_l = image["image"][slot["offset"] + at:slot["offset"] + at + 4]
            word_o = dol_data[dol_off + at:dol_off + at + 4]
            info = describe(addr, elf, splits, configured, orig_symbols, auto_units)
            report["first"] = {"where": "section", "group": row["group"], "index": row["index"],
                               "section": row["name"], "file_offset": dol_off + at, "offset_in_slot": at,
                               "addr": addr, "linked_word": word_l.hex(), "orig_word": word_o.hex(),
                               "linked_byte": word_l[0] if word_l else None, "orig_byte": word_o[0] if word_o else None,
                               "diff_count": row["diff_count"], **info}
            break
    return report


def duplicate_claims(splits: list[dict]) -> list[dict]:
    """Ranges two units both claim - a static link hazard the byte diff may not show yet."""
    spans = []
    for block in splits:
        for rng in block["ranges"]:
            spans.append((rng["section"], rng["start"], rng["end"], block["unit"]))
    spans.sort()
    out = []
    for i in range(len(spans)):
        for j in range(i + 1, len(spans)):
            a, b = spans[i], spans[j]
            if a[0] != b[0] or b[1] >= a[2]:
                break
            if a[3] != b[3]:
                out.append({"section": a[0], "start": max(a[1], b[1]), "end": min(a[2], b[2]),
                            "units": sorted({a[3], b[3]})})
    return out


# ---------------------------------------------------------------------------------------------------
# --unit: the unit's claimed region, the slot it lands in, and the object the link used
# ---------------------------------------------------------------------------------------------------

def unit_report(unit: str, elf: dict, dol_data: bytes, splits: list[dict], configured: dict,
                orig_symbols: dict, auto_units: list[dict], image: dict) -> dict:
    block = next((b for b in splits if b["unit"] == unit), None)
    info = configured.get(unit) or {}
    flag = info.get("flag")
    src_obj = "build/%s/src/%s.o" % (GAME, unit)
    tgt_obj = "build/%s/obj/%s.o" % (GAME, unit)
    linked_obj = src_obj if flag == "Matching" else tgt_obj
    report = {"unit": unit, "flag": flag, "linked_object": linked_obj, "src_object": src_obj,
              "target_object": tgt_obj, "ranges": [], "notes": [], "ok": True, "order_sensitive": False}
    if block is None:
        report["ok"] = False
        report["notes"].append("no splits.txt entry: nothing claims this unit's addresses")
        return report

    for rng in block["ranges"]:
        section, start, end = rng["section"], rng["start"], rng["end"]
        size = end - start
        slot = dol_slot_at(parse_dol(dol_data), start)
        in_bss = slot is None and image["bss_addr"] <= start < image["bss_addr"] + image["bss_size"]
        elf_bytes = read_addr(elf, start, size)
        dol_bytes = dol_read(dol_data, parse_dol(dol_data), start, size)
        row = {"section": section, "start": start, "end": end, "size": size,
               "slot": None, "slot_size": 0, "fragment": False, "order_sensitive": False,
               "bss": in_bss, "bytes_ok": None, "first_diff": None, "others": []}
        if slot is not None:
            row["slot"] = "%s%d (%s)" % (slot["group"], slot["index"], section)
            row["slot_size"] = slot["size"]
            # `.text`/`.sdata` are concatenated too, but their symbols carry pinned addresses, so only the
            # compiler-generated ctor/dtor tables are the link-order case.
            row["fragment"] = size < slot["size"] and section in FRAGMENT_SECTIONS
            row["order_sensitive"] = row["fragment"]
            row["others"] = sorted({b["unit"] for b in splits for r in b["ranges"]
                                    if r["section"] == section and r["start"] < slot["addr"] + slot["size"]
                                    and r["end"] > slot["addr"] and b["unit"] != unit})
        if elf_bytes is not None and dol_bytes is not None:
            n = min(len(elf_bytes), len(dol_bytes))
            first = next((i for i in range(n) if elf_bytes[i] != dol_bytes[i]), None)
            row["first_diff"] = first
            row["bytes_ok"] = first is None and len(elf_bytes) == len(dol_bytes)
            if not row["bytes_ok"]:
                report["ok"] = False
        if row["order_sensitive"]:
            report["order_sensitive"] = True
            report["notes"].append("%s: fragment of a shared %s slot (%d other claimed unit(s)) - the 7.19 "
                                   "link-order case" % (section, section, len(row["others"])))
        report["ranges"].append(row)

    # the unit's symbols must be where the map says, or objdiff pairs nothing (playbook row 31)
    placed = []
    for rng in block["ranges"]:
        for entry in orig_symbols.get(rng["section"], []):
            if not (rng["start"] <= entry["address"] < rng["end"]):
                continue
            if entry["size"] <= 0 or entry["name"].startswith("@"):
                continue     # labels (`_ctors`, `lbl_*`) share the address; `@etb_*` are dtk's exception names
            linked = symbol_at(elf, entry["address"])
            placed.append({"name": entry["name"], "addr": entry["address"],
                           "linked": linked["name"] if linked else None,
                           "ok": linked is not None and linked["name"] == entry["name"]})
    report["symbols"] = placed
    report["symbols_ok"] = all(p["ok"] for p in placed)
    if not report["symbols_ok"]:
        report["notes"].append("%d symbol(s) do not pair by name in the link" % sum(1 for p in placed if not p["ok"]))
    return report


# ---------------------------------------------------------------------------------------------------
# loading the repository state and rendering
# ---------------------------------------------------------------------------------------------------

def load_state() -> tuple[list[dict], dict, dict, dict, list[dict]]:
    splits = preflight.load_splits()
    configured, _libs = preflight.load_configure()
    by_name, by_section, _by_address = preflight.load_symbols()
    return splits, configured, by_name, by_section, load_auto_units()


def staleness(elf_path: str) -> dict | None:
    """`main.elf` older than the newest object means the link predates the sources - do not believe it."""
    if not os.path.exists(elf_path):
        return None
    newest, newest_path = 0.0, ""
    for root, _dirs, files in os.walk(SRC_DIR):
        for name in files:
            if name.endswith(".o"):
                path = os.path.join(root, name)
                if os.path.getmtime(path) > newest:
                    newest, newest_path = os.path.getmtime(path), path
    if not newest:
        return None
    elf_time = os.path.getmtime(elf_path)
    if elf_time < newest:
        return {"stale": True, "elf_time": elf_time, "newest_object": newest_path, "newest_time": newest,
                "detail": "%s is newer than %s - relink before believing this audit" % (newest_path, elf_path)}
    return {"stale": False, "newest_object": newest_path}


def fmt_addr(value: int | None) -> str:
    return "0x%08X" % value if value is not None else "-"


def render(report: dict, units: list[dict]) -> str:
    out = []
    out.append("link audit  %s" % (report["elf"] or "main.elf"))
    out.append("        vs  %s" % (report["dol"] or "main.dol"))
    out.append("entry %s  bss %s+0x%X  image %d bytes (DOL %d)"
               % (fmt_addr(report["entry"]["elf"]), fmt_addr(report["bss"]["elf_addr"]),
                  report["bss"]["elf_size"], report["image_size"], report["dol_size"]))
    # the wrapper dict is present whenever the check ran, so test the inner flag, not the dict
    stale = report.get("stale") or {}
    if stale.get("stale"):
        out.append("WARN  %s" % stale["detail"])
    text = " ".join(r["name"] for r in report["sections"] if r["group"] == "text")
    data = " ".join(r["name"] for r in report["sections"] if r["group"] == "data")
    out.append("order %s text %s | data %s"
               % ("OK" if report["order"]["ok"] else "DIFF", text, data))
    out.append("")
    out.append("%-4s %-5s %-14s %-10s %-10s %-9s %-9s %s" %
               ("grp", "slot", "section", "dol addr", "elf addr", "dol size", "elf size", "bytes"))
    for row in report["sections"]:
        flag = "OK" if row["bytes_ok"] else ("DIFF %d" % row["diff_count"])
        mark = "" if row["addr_ok"] else "  ADDR"
        out.append("%-4s %-5s %-14s %-10s %-10s 0x%-7X 0x%-7X %s%s" %
                   (row["group"], "%s%d" % (row["group"], row["index"]), row["name"],
                    fmt_addr(row["dol_addr"]), fmt_addr(row["elf_addr"]),
                    row["dol_size"], row["elf_size"], flag, mark))
    for extra in report["extra"]:
        out.append("EXTRA %s %s %s (0x%X) has no DOL slot" % (extra["group"], extra["name"],
                                                               fmt_addr(extra["addr"]), extra["size"]))
    if not report["header"]["ok"]:
        out.append("header DIFF at +0x%X (%s)" % (report["header"]["first_diff"], report["header"]["field"]))
    if report["duplicate_claims"]:
        for dup in report["duplicate_claims"]:
            out.append("DUPLICATE %s %s-0x%X claimed by %s"
                       % (dup["section"], fmt_addr(dup["start"]), dup["end"], " and ".join(dup["units"])))

    first = report["first"]
    if first is None:
        out.append("")
        out.append("RESULT MATCH: the linked ELF reproduces the original DOL byte for byte "
                   "(%d sections, %d bytes)" % (len(report["sections"]), report["dol_size"]))
    elif first["where"] == "header":
        out.append("")
        out.append("RESULT DIVERGE: DOL header field %s differs (linked %s, original %s)"
                   % (first["field"], first["linked"], first["original"]))
    else:
        out.append("")
        out.append("RESULT DIVERGE: %d differing byte(s), first at DOL +0x%X (%s/%s%d @ %s)"
                   % (first["diff_count"], first["file_offset"], first["section"], first["group"],
                      first["index"], fmt_addr(first["addr"])))
        out.append("  linked  : %s  %s" % (first["linked_word"], first["linked_symbol"] or "-"))
        out.append("  original: %s  %s" % (first["orig_word"], first["orig_symbol"] or "-"))
        owner = first["unit"] or (first["auto"] or "unclaimed")
        out.append("  owner   : %s%s  %s" % (owner, " [%s]" % first["flag"] if first["flag"] else "",
                                             first["object"] or ""))

    for unit in units:
        out.append("")
        out.append("unit %s  [%s]  linked object %s" % (unit["unit"], unit["flag"] or "unregistered",
                                                        unit["linked_object"]))
        if not unit["ranges"]:
            for note in unit["notes"]:
                out.append("  ! %s" % note)
            continue
        out.append("  %-10s %-22s %-12s %-6s %-6s %s" %
                   ("section", "claim", "slot", "bytes", "shared", "note"))
        for row in unit["ranges"]:
            claim = "%s-0x%X" % (fmt_addr(row["start"]), row["end"])
            shared = "%d/%d" % (row["size"], row["slot_size"]) if row["fragment"] else "no"
            if row["bss"]:
                state = "bss"
            elif row["bytes_ok"] is None:
                state = "?"
            else:
                state = "OK" if row["bytes_ok"] else "DIFF"
            note = "link-order" if row["order_sensitive"] else ""
            out.append("  %-10s %-22s %-12s %-6s %-6s %s"
                       % (row["section"], claim, row["slot"] or ("bss" if row["bss"] else "-"),
                          state, shared, note))
        for note in unit["notes"]:
            out.append("  ! %s" % note)
        if not unit["symbols_ok"]:
            for placed in unit["symbols"]:
                if not placed["ok"]:
                    out.append("  symbol %s at %s: link has %s"
                               % (placed["name"], fmt_addr(placed["addr"]), placed["linked"] or "nothing"))
        verdict = "LINK OK" if unit["ok"] else "DIFFERS"
        if unit["ok"] and unit["order_sensitive"]:
            verdict += " (link-order fragment)"
        out.append("  verdict: %s" % verdict)
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Audit the link: reconstruct the DOL from build/RMHE08/main.elf and compare it to the "
                    "original, per section, naming the first divergence and the unit responsible.")
    ap.add_argument("--unit", action="append", default=[], metavar="UNIT",
                    help="scope the audit to one unit's splits.txt claim (repeatable)")
    ap.add_argument("--elf", default=ELF_PATH)
    ap.add_argument("--dol", default=DOL_PATH)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--no-stale-check", action="store_true", help="skip the main.elf-vs-objects mtime check")
    args = ap.parse_args()

    if not os.path.exists(args.elf):
        print("no linked ELF at %s - run ninja first" % args.elf, file=sys.stderr)
        return 2
    if not os.path.exists(args.dol):
        print("no original DOL at %s" % args.dol, file=sys.stderr)
        return 2

    elf = parse_elf(open(args.elf, "rb").read())
    dol_data = open(args.dol, "rb").read()
    splits, configured, _by_name, by_section, auto_units = load_state()
    report = audit_link(elf, dol_data, splits, configured, by_section, auto_units, args.elf, args.dol)
    if not args.no_stale_check:
        report["stale"] = staleness(args.elf)

    image = dol_image(elf)
    units = [unit_report(name, elf, dol_data, splits, configured, by_section, auto_units, image)
             for name in args.unit]

    if args.json:
        print(json.dumps({"link": report, "units": units}, indent=2))
    else:
        print(render(report, units))

    bad = (not report["ok"]) or any(not u["ok"] for u in units)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
