#!/usr/bin/env python3
"""Extract everything a split object already knows about a unit - one page, before any source is written.

docs/plan.md, "The binary is the ground truth, and compilation is lossy" (2026-09-23). This is a
*matching* decompilation, so the binary is the only authority - and compilation is lossy, so what
survives in the split object is *evidence about the source*: the original file name, the language, the
source line of every assert, the order of the literals, the switch structure, the unit's extent. The
method is to extract the maximum from the object first and fill only the blanks it cannot answer.

Six workers independently rediscovered the same peephole lever, one spent a round working out that its
unit's `__FILE__` string was `ef_cube.cpp`, another propagated "`Panic` is variadic" through 13 call
sites by hand. Every one of those traces was already in the object. This tool is the one page that
carries them, and `brief.py` embeds it so a worker's brief *is* the dossier plus the task.

What it reports, and what each trace is evidence for (the plan's table, mechanised):

| section | the trace | what it tells us |
| --- | --- | --- |
| source file & asserts | `__FILE__` strings in `.data`, read from the DOL | the original source name (`ef_line.cpp`), hence the module and language |
| symbols | mangled names in the object's symbol table | the language (C++ vs C) and the signature |
| panic line map | the `li r4, line` before each `Panic` call | the source line of each assert, in order |
| literals | the `.sdata2`/`.data` pool, in section order | the order the literals appear in the source |
| external references | relocations in `.text` | which named symbol each load/call refers to |
| jump tables | dense 4-byte `.data`/`.rodata` relocations | the switch structure, including the case count |
| section layout | the object's section headers | the exception/constructor structure and the unit's extent |
| blanks | what is *not* there | a name with no dump entry, a signature with no callers, a type with no size evidence |

The other oracle (`D:/WiiExperiment/DumpSymbols.zip`, `docs/memory-dump.md`) is consulted for names
only - never for codegen. A name it does not have is reported in the blanks section, not guessed.

    python tools/units/dossier.py <unit> [--json] [--out FILE] [--no-dump]
    python tools/units/dossier.py --selftest

Read-only: no `ninja`, no compile, no link, no write to any shared file.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for _path in (os.path.join(ROOT, "tools"), os.path.dirname(HERE), HERE):
    if _path not in sys.path:
        sys.path.insert(0, _path)

GAME = "RMHE08"
DOL_REL = os.path.join("orig", GAME, "sys", "main.dol")
# The shared runtime dump's symbol map is the cheap name oracle (docs/memory-dump.md). Overridable so
# the tool degrades to "no names" instead of failing on a machine that does not have it.
DUMP_ZIP = os.environ.get("MH3_DUMP_SYMBOLS", r"D:/WiiExperiment/DumpSymbols.zip")

# The relocation types MWCC emits in these objects. Type 109 (0x6D) is the EABI SDA21 form the
# compiler uses for small-data/pool accesses; the rest are the standard PPC EABI names.
RELOC_TYPES = {
    1: "R_PPC_ADDR32", 4: "R_PPC_ADDR16_LO", 5: "R_PPC_ADDR16_HI", 6: "R_PPC_ADDR16_HA",
    10: "R_PPC_REL24", 109: "R_PPC_EMB_SDA21",
}
POOL_SECTIONS = (".sdata2", ".sdata", ".rodata", ".data", ".bss", ".sbss", ".sbss2")
CODE_SECTIONS = (".text", ".init")
# MWCC's mangling: `__F...` (free function), `__Q<digits>...` (qualified/method), `__ct`/`__dt`. A C
# identifier never carries one, and the runtime helpers (`__register_fragment`, `_savegpr_15`) do not
# match, which is the whole point: the language comes from the symbol, not from our convenience.
MANGLED_RE = re.compile(r"__(?:Q\d|F[A-Za-z0-9]|ct|dt)")
PANIC_RE = re.compile(r"Panic|assert|Assert|__assert")
STRING_FILE_RE = re.compile(r"\.(?:cpp|cc|cxx|cp|c|hpp|hh|h)$")
FN_RE = re.compile(r"^(?:fn_|unk)")
LBL_HEX_RE = re.compile(r"_([0-9A-Fa-f]{6,8})$")


# ------------------------------------------------------------------------------------------------------------------
# pure readers (everything the selftest exercises directly)
# ------------------------------------------------------------------------------------------------------------------

def parse_elf(blob: bytes) -> tuple[list[dict], list[dict], list[dict]]:
    """(sections, symbols, relocations) of an ELF32 big-endian object - enough for MWCC objects.

    Relocations are SHT_RELA (type 4); `sh_info` is the section they apply to and `sh_link` the symbol
    table. Symbol rows keep their binding/type and the section index they live in, because "is this
    symbol defined here, and in which section" is what separates the unit's own symbols from its
    external references.
    """
    if blob[:4] != b"\x7fELF":
        raise ValueError("not an ELF object")
    (shoff,) = struct.unpack_from(">I", blob, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", blob, 0x2E)
    sections = []
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from(
            ">IIIIIIIIII", blob, o)
        sections.append(dict(name_off=name, typ=typ, flags=flags, addr=addr, off=off, size=size,
                             link=link, info=info, align=align, entsize=entsize,
                             data=blob[off:off + size]))
    shstr = sections[shstrndx]["data"]
    for s in sections:
        end = shstr.find(b"\0", s["name_off"])
        s["name"] = shstr[s["name_off"]:end].decode("latin-1")

    symbols = []
    symtab = next((s for s in sections if s["typ"] == 2), None)
    if symtab is not None:
        strtab = sections[symtab["link"]]["data"]
        for o in range(0, symtab["size"], symtab["entsize"] or 16):
            nm, val, size, info, _other, shndx = struct.unpack_from(">IIIBBH", symtab["data"], o)
            end = strtab.find(b"\0", nm)
            name = strtab[nm:end].decode("latin-1") if nm else ""
            symbols.append(dict(name=name, value=val, size=size, info=info, bind=info >> 4,
                                type=info & 0xF, shndx=shndx,
                                section=sections[shndx]["name"] if 0 < shndx < len(sections) else None))

    relocs = []
    for s in sections:
        if s["typ"] != 4:
            continue
        target = sections[s["info"]]["name"] if 0 <= s["info"] < len(sections) else None
        for o in range(0, s["size"], s["entsize"] or 12):
            roff, rinfo, radd = struct.unpack_from(">IIi", s["data"], o)
            si, rt = rinfo >> 8, rinfo & 0xFF
            relocs.append(dict(section=s["name"], target=target, offset=roff, type=rt,
                               type_name=RELOC_TYPES.get(rt, "type-%d" % rt),
                               symbol=symbols[si]["name"] if si < len(symbols) else None,
                               addend=radd))
    return sections, symbols, relocs


def cstring(data: bytes, off: int, limit: int = 256) -> str:
    """A NUL-terminated C string at `off`, or up to `limit` bytes when it is not terminated."""
    end = data.find(b"\0", off)
    if end < 0:
        end = min(len(data), off + limit)
    return data[off:end].decode("latin-1")


def is_mangled(name: str | None) -> bool:
    """Whether a symbol name carries MWCC's C++ mangling (`__F...`, `__Q<digits>...`, `__ct`/`__dt`)."""
    return bool(MANGLED_RE.search(name or ""))


def language_of(name: str | None) -> str:
    return "C++" if is_mangled(name) else "C"


def decode_li(word: int) -> tuple[int, int] | None:
    """`(register, immediate)` when the word is `addi rD, 0, imm` (MWCC's `li`), else None."""
    if word >> 26 != 14 or ((word >> 16) & 0x1F) != 0:
        return None
    imm = word & 0xFFFF
    if imm >= 0x8000:
        imm -= 0x10000
    return (word >> 21) & 0x1F, imm


def panic_calls(text: bytes, base: int, relocs: list[dict], funcs: list[dict],
                window: int = 0x30) -> list[dict]:
    """One row per call to a panic/assert formatter, with the source line it passes in `r4`.

    `Panic(const char* file, int line, const char* fmt, ...)` and `OSPanic` agree on the argument
    registers, and MWCC materialises the line with `li r4, <line>` immediately before the call. The
    nearest such `li` is the assert's source line - which orders the source and shows how much is
    missing. Relocations in the preceding `window` bytes are the file and format-string operands.
    """
    rows = []
    for r in relocs:
        if not PANIC_RE.search(r.get("symbol") or ""):
            continue
        off = r["offset"]
        line = line_off = None
        for back in range(1, 17):
            o = off - back * 4
            if o < 0:
                break
            li = decode_li(struct.unpack_from(">I", text, o)[0])
            if li and li[0] == 4:
                line, line_off = li[1], o
                break
        refs = [x for x in relocs
                if x is not r and off - window <= x["offset"] < off
                and not PANIC_RE.search(x.get("symbol") or "")]
        func = next((f["name"] for f in funcs if f["value"] <= off < f["value"] + f["size"]), None)
        rows.append(dict(offset=off, address=base + off, line=line, line_offset=line_off,
                         function=func, symbol=r["symbol"], refs=refs))
    rows.sort(key=lambda row: row["offset"])
    return rows


def jump_tables(sections: list[dict], relocs: list[dict]) -> list[dict]:
    """Dense tables of 4-byte address relocations in a pool section - the switch structure.

    A jump table is a pool section whose entries are *all* 4-byte `ADDR32` relocations, one per slot
    with no gaps. The entry count is the switch's case count, and each entry names a code target.
    """
    out = []
    for s in sections:
        if s["name"] not in POOL_SECTIONS or not s["size"] or s["size"] % 4:
            continue
        rs = sorted((r for r in relocs if r["target"] == s["name"] and r["type"] == 1),
                    key=lambda r: r["offset"])
        if len(rs) != s["size"] // 4:
            continue
        if [r["offset"] for r in rs] != list(range(0, s["size"], 4)):
            continue
        out.append(dict(section=s["name"], size=s["size"], count=len(rs),
                        entries=[dict(offset=r["offset"], symbol=r["symbol"], addend=r["addend"])
                                 for r in rs]))
    return out


def pool_entries(section: dict, symbols: list[dict], relocs: list[dict]) -> list[dict]:
    """The literals of one owned pool section, in section order, with what references each.

    Boundaries come from the section's own local symbols (`@NNNN`), which MWCC emits per literal, and
    from any relocation that lands inside the section. Each entry carries its bytes, the local symbol
    name, and every relocation whose target symbol is that local symbol.
    """
    data, size = section["data"], section["size"]
    starts = {0}
    for sym in symbols:
        if sym["shndx"] and sym["section"] == section["name"]:
            starts.add(sym["value"])
    for r in relocs:
        if r["target"] == section["name"]:
            starts.add(r["offset"])
    starts = sorted(o for o in starts if 0 <= o < size)
    entries = []
    for i, off in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else size
        local = next((s["name"] for s in symbols if s["section"] == section["name"]
                      and s["value"] == off), None)
        refs = [r for r in relocs if r["target"] == section["name"] and r["offset"] == off]
        entries.append(dict(offset=off, size=end - off, bytes=data[off:end],
                            symbol=local, refs=refs))
    return entries


def dol_sections(blob: bytes) -> list[tuple[int, int, int]]:
    """`(address, size, file_offset)` for every DOL section with file bytes."""
    if len(blob) < 0x100:
        return []
    text_off = struct.unpack_from(">7I", blob, 0x00)
    data_off = struct.unpack_from(">11I", blob, 0x1C)
    text_addr = struct.unpack_from(">7I", blob, 0x48)
    data_addr = struct.unpack_from(">11I", blob, 0x64)
    text_size = struct.unpack_from(">7I", blob, 0x90)
    data_size = struct.unpack_from(">11I", blob, 0xAC)
    out = [(text_addr[i], text_size[i], text_off[i]) for i in range(7) if text_size[i]]
    out += [(data_addr[i], data_size[i], data_off[i]) for i in range(11) if data_size[i]]
    return out


def dol_bytes(blob: bytes, sections: list[tuple[int, int, int]], address: int, length: int):
    """The DOL's file bytes at `address`, or None when the address is not backed by file bytes."""
    for start, size, offset in sections:
        if start <= address and address + length <= start + size:
            begin = offset + (address - start)
            return blob[begin:begin + length]
    return None


def dump_names(path: str = DUMP_ZIP) -> dict:
    """`{address: name}` from the shared dump's Dolphin map - the cheap name oracle.

    Lines look like `RSOStaticLocateObject 804d9ec4 f` or `kbd_open(unsigned 80040798 f`: the name is
    the first token (demangled arguments stripped), the address is the one 8-hex token. `zz_<addr>_`
    is the dumper's own placeholder for an unnamed function and is treated as no name at all.
    """
    if not os.path.exists(path):
        return {}
    key = (path, os.path.getmtime(path))
    cached = _DUMP_CACHE.get(key)
    if cached is not None:
        return cached
    out: dict = {}
    try:
        with zipfile.ZipFile(path) as z:
            member = next((n for n in z.namelist() if n.endswith(".map")), None)
            if member is None:
                return {}
            for line in z.read(member).decode("latin-1").splitlines():
                parts = line.split()
                if not parts:
                    continue
                addr = next((int(p, 16) for p in parts if re.fullmatch(r"[0-9a-fA-F]{8}", p)), None)
                if addr is None:
                    continue
                name = parts[0].split("(")[0]
                if name.startswith("zz_"):
                    continue
                out.setdefault(addr, name)
    except (zipfile.BadZipFile, OSError):
        return {}
    _DUMP_CACHE.clear()
    _DUMP_CACHE[key] = out
    return out


_DUMP_CACHE: dict = {}
_DOL_CACHE: dict = {}
_MAPLOOKUP_CACHE: dict = {}


def looks_like_string(data: bytes) -> bool:
    """Whether the bytes are a NUL-terminated printable C string - the `.data` string-pool shape.

    The map's `data:string` annotation is the primary signal, but `brief.map_rows` keeps only the
    `type:object` field, so this is the fallback that stops a string pool rendering as raw hex.
    """
    end = data.find(b"\0") if data else -1
    if end < 3:
        return False
    return all(32 <= b < 127 or b in (9, 10, 13) for b in data[:end])


def name_address(name: str) -> int | None:
    """The DOL address a `lbl_XXXXXXXX`/`fn_XXXXXXXX` name encodes, or None."""
    m = LBL_HEX_RE.search(name or "")
    return int(m.group(1), 16) if m else None


def literal_value(data: bytes, kind: str) -> str:
    """A readable rendering of a pool literal - float, double, pointer, or C string."""
    if kind == "string" or (not kind and looks_like_string(data)):
        return json.dumps(cstring(data, 0, 64))
    if len(data) == 4:
        (u32,) = struct.unpack(">I", data)
        (f32,) = struct.unpack(">f", data)
        if f32 == f32 and abs(f32) not in (float("inf"),) and 1e-30 < abs(f32) < 1e30:
            return "%g (0x%08X)" % (f32, u32)
        return "0x%08X" % u32
    if len(data) == 8:
        (f64,) = struct.unpack(">d", data)
        (u64,) = struct.unpack(">Q", data)
        return "%g (0x%016X)" % (f64, u64) if abs(f64) < 1e300 else "0x%016X" % u64
    return data.hex()


# ------------------------------------------------------------------------------------------------------------------
# the analysis
# ------------------------------------------------------------------------------------------------------------------

def map_lookup(main: str) -> dict:
    """`{name: map row}` for the whole symbol map, through `brief.py`'s in-process parser and cache.

    The dict is cached per (path, mtime): `brief.py --pool` builds a dossier per unit in one process,
    and rebuilding a 65k-entry dict for every one of them is pure waste.
    """
    try:
        from units import brief  # lazy: brief imports this module at load time
    except ImportError:  # running dossier.py as a plain script
        import brief  # type: ignore
    path = os.path.join(main, "config", GAME, "symbols.txt") if main else None
    key = (path, os.path.getmtime(path) if path and os.path.exists(path) else None)
    cached = _MAPLOOKUP_CACHE.get(key)
    if cached is not None:
        return cached
    out = {r["name"]: r for r in (brief.map_rows(main) or [])}
    _MAPLOOKUP_CACHE.clear()
    _MAPLOOKUP_CACHE[key] = out
    return out


def dol_blob(path: str):
    """The original DOL's bytes, cached per (path, mtime) - it is read once per process, not per unit."""
    if not path or not os.path.exists(path):
        return None
    key = (path, os.path.getmtime(path))
    cached = _DOL_CACHE.get(key)
    if cached is not None:
        return cached
    blob = open(path, "rb").read()
    _DOL_CACHE.clear()
    _DOL_CACHE[key] = blob
    return blob


def _resolve_addr(name: str, section: str | None, value: int, ranges: dict, by_name: dict) -> int | None:
    """The DOL address of a symbol: its section base + value when it is defined here, else the map."""
    if section and section in ranges:
        return ranges[section][0] + value
    row = by_name.get(name)
    if row:
        return row["address"]
    return name_address(name)


def build(main: str, unit: str, target: str, ranges: dict | None = None,
          map_symbols: list[dict] | None = None, dump: bool = True) -> dict:
    """The whole dossier for one unit, as a dict.

    `ranges` is `brief.splits_range`'s `{section: (start, end, size)}` and gives every section's DOL
    base address; `map_symbols` is the unit's map rows. Both are optional so the tool can also run
    from a bare target object path.
    """
    ranges = ranges or {}
    d = dict(unit=unit, target=target, error=None, sections=[], symbols=[], externals=[],
             panic=[], jump_tables=[], pool=[], literals=[], blanks=[], source_file=None,
             dol_available=False, dump_available=False)
    if not target or not os.path.exists(target):
        d["error"] = "target object not found: %s" % target
        return d
    blob = open(target, "rb").read()
    sections, symbols, relocs = parse_elf(blob)
    d["sections"] = [dict(name=s["name"], typ=s["typ"], size=s["size"], addr=s["addr"],
                          align=s["align"], flags=s["flags"],
                          dol_addr=ranges.get(s["name"], (None,))[0])
                     for s in sections if s["name"]]
    by_name = map_lookup(main) if main else {}
    defined = {s["name"]: s for s in symbols if s["shndx"]}

    # the DOL is the linked image the split object came from: strings and pool values are read there
    dol_blob_data, dol_segs = b"", []
    dol_path = os.path.join(main, DOL_REL) if main else None
    loaded = dol_blob(dol_path)
    if loaded is not None:
        dol_blob_data = loaded
        dol_segs = dol_sections(dol_blob_data)
        d["dol_available"] = True
    names = dump_names() if dump else {}
    d["dump_available"] = bool(names)

    def read_dol(address: int, length: int):
        return dol_bytes(dol_blob_data, dol_segs, address, length) if dol_segs else None

    def dump_name(address):
        return names.get(address) if address is not None else None

    # --- owned symbols: the unit's own object symbols, plus its map rows -------------------------------
    seen = set()
    for sym in symbols:
        if not sym["shndx"] or not sym["name"]:
            continue
        if sym["type"] not in (0, 1, 2):  # notype / object / function
            continue
        addr = _resolve_addr(sym["name"], sym["section"], sym["value"], ranges, by_name)
        row = by_name.get(sym["name"])
        d["symbols"].append(dict(name=sym["name"], address=addr, size=sym["size"] or (row or {}).get("size", 0),
                                 section=sym["section"], defined=True, language=language_of(sym["name"]),
                                 mangled=is_mangled(sym["name"]), dump_name=dump_name(addr),
                                 kind="func" if sym["type"] == 2 else "data"))
        seen.add(sym["name"])
    for row in (map_symbols or []):
        if row["name"] in seen or not row["name"]:
            continue
        addr = row["address"]
        d["symbols"].append(dict(name=row["name"], address=addr, size=row.get("size", 0),
                                 section=row.get("section"), defined=True,
                                 language=language_of(row["name"]), mangled=is_mangled(row["name"]),
                                 dump_name=dump_name(addr), kind=row.get("type", "data")))
        seen.add(row["name"])
    d["symbols"].sort(key=lambda s: (s["address"] is None, s["address"] or 0, s["name"]))

    # --- external references: every undefined reloc target ---------------------------------------------
    ext: dict = {}
    for r in relocs:
        name = r.get("symbol")
        if not name or name in defined:
            continue
        row = by_name.get(name)
        section = (row or {}).get("section")
        addr = _resolve_addr(name, None, 0, ranges, by_name)
        kind = "call" if r["type"] == 10 else ("pool" if r["type"] == 109 else "data")
        e = ext.setdefault(name, dict(name=name, address=addr, section=section, kind=kind,
                                      refs=[], language=language_of(name), dump_name=dump_name(addr)))
        e["refs"].append(r)
    d["externals"] = sorted(ext.values(), key=lambda e: (e["address"] is None, e["address"] or 0, e["name"]))

    # --- source file and asserts ------------------------------------------------------------------------
    file_refs = [e for e in d["externals"] if e["section"] == ".data" and e["address"] is not None]
    for e in file_refs:
        row = by_name.get(e["name"]) or {}
        size = row.get("size", 0x40)
        data = read_dol(e["address"], max(size, 8))
        e["string"] = cstring(data, 0) if data else None
    sources = [e for e in file_refs if e.get("string") and STRING_FILE_RE.search(e["string"])]
    d["source_strings"] = [dict(name=e["name"], address=e["address"], text=e["string"])
                           for e in sources]
    if sources:
        d["source_file"] = sorted({e["string"] for e in sources})[0]

    # --- panic line map ---------------------------------------------------------------------------------
    text_sec = next((s for s in sections if s["name"] in CODE_SECTIONS), None)
    if text_sec is not None:
        base = ranges.get(text_sec["name"], (0, 0, 0))[0]
        funcs = [dict(name=s["name"], value=s["value"], size=s["size"])
                 for s in symbols if s["type"] == 2 and s["shndx"] and s["section"] in CODE_SECTIONS]
        text_relocs = [r for r in relocs if r["target"] == text_sec["name"]]
        calls = panic_calls(text_sec["data"], base, text_relocs, funcs)
        for call in calls:
            strings = []
            for ref in call["refs"]:
                row = by_name.get(ref["symbol"]) or {}
                addr = _resolve_addr(ref["symbol"], None, 0, ranges, by_name)
                data = read_dol(addr, max(row.get("size", 0x40), 8)) if addr is not None else None
                strings.append(dict(name=ref["symbol"], address=addr,
                                    text=cstring(data, 0, 80) if data else None))
            call["strings"] = strings
            call["file"] = next((s["text"] for s in strings
                                 if s["text"] and STRING_FILE_RE.search(s["text"])), None)
            call["message"] = next((s["text"] for s in strings
                                    if s["text"] and not STRING_FILE_RE.search(s["text"])), None)
        d["panic"] = calls

    # --- literals in pool order, and the external pool the object references -----------------------------
    # A section that is a dense address table is the switch structure, not a literal pool: report it in
    # the jump-tables section instead of duplicating every entry here.
    d["jump_tables"] = jump_tables(sections, relocs)
    table_sections = {t["section"] for t in d["jump_tables"]}
    for s in sections:
        if s["name"] not in POOL_SECTIONS or not s["size"] or s["name"] in table_sections:
            continue
        entries = pool_entries(s, symbols, relocs)
        rows = []
        for e in entries:
            refs = []
            for r in e["refs"]:
                refs.append(dict(symbol=r["symbol"], addend=r["addend"], type=r["type_name"]))
            row = by_name.get(e["symbol"] or "")
            kind = "string" if (row or {}).get("type") == "string" else ""
            rows.append(dict(offset=e["offset"], size=e["size"], bytes=e["bytes"].hex(),
                             symbol=e["symbol"], refs=refs, value=literal_value(e["bytes"], kind)))
        d["pool"].append(dict(section=s["name"], size=s["size"], entries=rows))

    # external pool literals: the `.sdata2` labels a `.text` load refers to, in relocation order
    lit = []
    for e in d["externals"]:
        if e["section"] not in (".sdata2", ".sdata", ".rodata", ".data") or e["address"] is None:
            continue
        row = by_name.get(e["name"]) or {}
        size = row.get("size", 4)
        data = read_dol(e["address"], max(size, 4))
        if data is None:
            continue
        kind = "string" if row.get("type") == "string" else ""
        lit.append(dict(name=e["name"], address=e["address"], section=e["section"],
                        value=literal_value(data, kind), kind=e["kind"],
                        refs=sorted({r["offset"] for r in e["refs"]})))
    d["literals"] = lit

    # --- blanks: what the object does not answer ---------------------------------------------------------
    referenced = {r.get("symbol") for r in relocs}
    referenced |= {e["symbol"] for t in d["jump_tables"] for e in t["entries"]}
    no_name = [s["name"] for s in d["symbols"]
               if s["defined"] and FN_RE.match(s["name"]) and not s["dump_name"]]
    no_caller = [s["name"] for s in d["symbols"]
                 if s["defined"] and s["kind"] == "func" and s["name"] not in referenced
                 and not s["dump_name"]]
    ext_unknown = [e["name"] for e in d["externals"]
                   if FN_RE.match(e["name"]) and not e["dump_name"] and e["kind"] == "call"]
    unreadable = [e["name"] for e in d["externals"]
                  if e["section"] == ".data" and e["address"] is not None and not e.get("string")]
    blanks = []
    if not dump:
        blanks.append("Dump names were not consulted (`--no-dump`), so an unnamed `fn_*` may still have a "
                      "name in the shared map.")
    elif not d["dump_available"]:
        blanks.append("The shared dump map is not available (`%s`), so no external name can be checked." % DUMP_ZIP)
    if not d["dol_available"]:
        blanks.append("The original DOL is not readable, so string/pool *contents* are unknown; only "
                      "their addresses are evidence.")
    if no_name:
        blanks.append("Names the binary does not settle (still `fn_*`/`unk*` and absent from the dump): "
                      + ", ".join("`%s`" % n for n in no_name[:12]) + ("" if len(no_name) <= 12 else " (+%d)" % (len(no_name) - 12)))
    if ext_unknown:
        blanks.append("Callees whose name the binary does not settle: "
                      + ", ".join("`%s`" % n for n in ext_unknown[:12]) + ("" if len(ext_unknown) <= 12 else " (+%d)" % (len(ext_unknown) - 12)))
    if no_caller:
        blanks.append("Functions with no caller in this object and no dump name - nothing pins their "
                      "signature: "
                      + ", ".join("`%s`" % n for n in no_caller[:12]) + ("" if len(no_caller) <= 12 else " (+%d)" % (len(no_caller) - 12)))
    blanks.append("Types have no size evidence here: an MWCC object carries no DWARF, so a struct's "
                  "size and fields must be read from the field offsets in `.text` (or the dump's "
                  "`get_struct_layout`), never assumed.")
    if not any(p["entries"] for p in d["pool"]) and not d["jump_tables"]:
        blanks.append("The unit owns no pool section (`.sdata2`/`.sdata`/`.data`): its literals live in "
                      "a shared or another unit's pool, so only the reference order below is evidence, "
                      "not the pool's own order.")
    if unreadable:
        blanks.append("String references the DOL could not resolve: "
                      + ", ".join("`%s`" % n for n in unreadable[:8]))
    d["blanks"] = blanks
    return d


# ------------------------------------------------------------------------------------------------------------------
# rendering
# ------------------------------------------------------------------------------------------------------------------

def _hx(value):
    return "0x%X" % value if value is not None else "?"


def _cell(text):
    """A markdown table cell: a newline in a panic string must not break the row."""
    if text is None:
        return ""
    return text.replace("\r", "").replace("\n", "\\n").replace("|", "\\|")


def render(d: dict, title: bool = False) -> str:
    """The dossier as markdown. `title=True` adds the standalone `# Dossier:` heading."""
    out = []
    if title:
        out.append("# Dossier: %s" % d["unit"])
        out.append("")
    out.append("### The binary dossier (what the split object already knows)")
    out.append("")
    out.append("> `%s` - the traces the object carries about the source. The binary is the ground "
               "truth and compilation is lossy, so read these before writing a line." % d["target"])
    out.append("")
    if d.get("error"):
        out.append("**Cannot read the object:** %s" % d["error"])
        return "\n".join(out).rstrip() + "\n"

    # 1 source file & asserts
    out.append("**Source file.** " + ("`%s`" % _cell(d["source_file"]) if d["source_file"]
                                      else "no `__FILE__` string is referenced by this unit"))
    if d["source_file"]:
        lang = "C++" if d["source_file"].endswith((".cpp", ".cc", ".cxx", ".cp")) else "C"
        out.append(" - the original translation unit is a `%s` file, so the language is **%s** and "
                   "objdiff pairs by a mangled (or `extern \"C\"`) name." % (_cell(d["source_file"]), lang))
    for s in d.get("source_strings") or []:
        out.append("Its symbol is `%s` @ %s (`\"%s\"`)." % (s["name"], _hx(s["address"]), _cell(s["text"])))
    asserts = [p for p in d["panic"] if p["line"] is not None]
    if asserts:
        lines = ", ".join(str(p["line"]) for p in asserts)
        out.append("")
        out.append("**Panic line map** (in order): %s - these are the original source lines, so they "
                   "order the file and show how much is missing." % lines)
    out.append("")

    # 2 symbols
    out.append("**Owned symbols** (defined in the object / the map's rows for the unit).")
    out.append("")
    out.append("| symbol | address | size | section | language | dump name (candidate) |")
    out.append("| --- | --- | --- | --- | --- | --- |")
    for s in d["symbols"][:60]:
        out.append("| `%s` | %s | %s | `%s` | %s | %s |"
                   % (s["name"], _hx(s["address"]), _hx(s["size"]), s["section"] or "?", s["language"],
                      "`%s`" % _cell(s["dump_name"]) if s["dump_name"] else "-"))
    if len(d["symbols"]) > 60:
        out.append("| ... | | | | | (%d more) |" % (len(d["symbols"]) - 60))
    out.append("")
    ext_calls = [e for e in d["externals"] if e["kind"] == "call"]
    if ext_calls:
        out.append("**Referenced callees** (undefined reloc targets; a dump name is a *candidate* - "
                   "docs/memory-dump.md says trust it only when it reads like an SDK identifier and "
                   "the signature agrees; a `fn_*` with none is a blank).")
        out.append("")
        out.append("| callee | address | section | language | dump name (candidate) | call sites |")
        out.append("| --- | --- | --- | --- | --- | --- |")
        for e in ext_calls[:40]:
            out.append("| `%s` | %s | `%s` | %s | %s | %s |"
                       % (e["name"], _hx(e["address"]), e["section"] or "?", e["language"],
                          "`%s`" % _cell(e["dump_name"]) if e["dump_name"] else "-",
                          ", ".join("%s" % _hx(r["offset"]) for r in e["refs"][:6])))
        out.append("")

    # 3 panic detail
    if d["panic"]:
        out.append("**Panic call sites.**")
        out.append("")
        out.append("| line | function | call | file | message |")
        out.append("| --- | --- | --- | --- | --- |")
        for p in d["panic"]:
            out.append("| %s | `%s` | %s | %s | %s |"
                       % (p["line"] if p["line"] is not None else "?",
                          p["function"] or "?", _hx(p["address"]),
                          "`%s`" % _cell(p["file"]) if p["file"] else "-",
                          "`%s`" % _cell(p["message"]) if p["message"] else "-"))
        out.append("")

    # 4 literals in pool order
    owned_pool = [p for p in d["pool"] if p["entries"]]
    if owned_pool:
        out.append("**Literals in pool order** (owned sections, in section order).")
        out.append("")
        out.append("| section | offset | bytes | value | local | referenced by |")
        out.append("| --- | --- | --- | --- | --- | --- |")
        for p in owned_pool:
            for e in p["entries"]:
                refs = ", ".join("%s %s" % (r["type"], r["symbol"]) for r in e["refs"]) or "-"
                out.append("| `%s` | 0x%X | `%s` | %s | %s | %s |"
                           % (p["section"], e["offset"], e["bytes"], e["value"],
                              "`%s`" % e["symbol"] if e["symbol"] else "-", refs))
        out.append("")
    if d["literals"]:
        out.append("**External pool literals** (referenced from `.text`, in relocation order).")
        out.append("")
        out.append("| symbol | address | section | value | refs |")
        out.append("| --- | --- | --- | --- | --- |")
        for e in d["literals"]:
            out.append("| `%s` | %s | `%s` | %s | %s |"
                       % (e["name"], _hx(e["address"]), e["section"], e["value"],
                          ", ".join(_hx(o) for o in e["refs"][:8])))
        out.append("")

    # 5 jump tables
    if d["jump_tables"]:
        out.append("**Jump tables** (switch structure).")
        out.append("")
        for t in d["jump_tables"]:
            out.append("- `%s` 0x%X - **%d entries**: %s"
                       % (t["section"], t["size"], t["count"],
                          ", ".join("%s+%d" % (e["symbol"], e["addend"]) for e in t["entries"][:16])))
        out.append("")

    # 6 section layout
    out.append("**Section layout.**")
    out.append("")
    out.append("| section | type | size | DOL base |")
    out.append("| --- | --- | --- | --- |")
    for s in d["sections"]:
        out.append("| `%s` | %d | 0x%X | %s |"
                   % (s["name"], s["typ"], s["size"], _hx(s.get("dol_addr"))))
    out.append("")

    # 7 blanks
    out.append("**Blanks - what the binary does not answer.**")
    out.append("")
    for b in d["blanks"]:
        out.append("- " + b)
    return "\n".join(out).rstrip() + "\n"


# ------------------------------------------------------------------------------------------------------------------
# selftest
# ------------------------------------------------------------------------------------------------------------------

def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # mangling/language: the language comes from the symbol
    check("a mangled method is C++", language_of("Panic__Q24nw4r2dbFPCciPCce"), "C++")
    check("a mangled free function is C++", language_of("foo__FPci"), "C++")
    check("a constructor is C++", language_of("Vec__ctFv"), "C++")
    check("a plain C function is C", language_of("RSONotifyPreRSOLink"), "C")
    check("a runtime helper is not mangled", is_mangled("__register_fragment"), False)
    check("a gpr helper is not mangled", is_mangled("_savegpr_21"), False)
    check("an @ local is not mangled", is_mangled("@1845"), False)

    # cstring
    check("cstring stops at NUL", cstring(b"ef_line.cpp\0NW4R", 0), "ef_line.cpp")
    check("cstring at an offset", cstring(b"ef_line.cpp\0NW4R", 12), "NW4R")
    check("cstring without a terminator is bounded", cstring(b"abcdef", 0, 3), "abc")

    # li decode
    check("li r4, 42 decodes", decode_li((14 << 26) | (4 << 21) | 42), (4, 42))
    check("a negative li decodes", decode_li((14 << 26) | (4 << 21) | 0xFFF0), (4, -16))
    check("addi with a base is not li", decode_li((14 << 26) | (4 << 21) | (3 << 16) | 42), None)
    check("a non-addi is not li", decode_li(0x48000000), None)

    # panic_calls: a synthetic `.text` with three asserts, lines 42/43/44
    def li(rt, imm):
        return (14 << 26) | (rt << 21) | (imm & 0xFFFF)

    text = b"".join(struct.pack(">I", w) for w in [
        li(3, 0),            # 0x00 lis r3 (file)
        0,                   # 0x04
        li(4, 42),           # 0x08
        li(5, 0),            # 0x0C
        0x48000001,          # 0x10 bl Panic
        li(4, 43),           # 0x14
        0x48000001,          # 0x18 bl Panic
        li(4, 44),           # 0x1C
        0x48000001,          # 0x20 bl Panic
        0x4E800020,          # 0x24 blr
    ])
    relocs = [dict(offset=0x10, symbol="Panic__Q24nw4r2dbFPCciPCce", type=10),
              dict(offset=0x18, symbol="Panic__Q24nw4r2dbFPCciPCce", type=10),
              dict(offset=0x20, symbol="Panic__Q24nw4r2dbFPCciPCce", type=10)]
    funcs = [dict(name="fn_1", value=0, size=0x28)]
    calls = panic_calls(text, 0x80000000, relocs, funcs)
    check("three panic calls", len(calls), 3)
    check("the lines are in order", [c["line"] for c in calls], [42, 43, 44])
    check("a call site carries its address", calls[0]["address"], 0x80000010)
    check("a call site carries its function", calls[0]["function"], "fn_1")

    # jump_tables: a dense 4-entry table, and a sparse section that is not a table
    data = dict(name=".data", size=16, data=b"\0" * 16)
    rs = [dict(target=".data", offset=i * 4, type=1, symbol="f", addend=i) for i in range(4)]
    tables = jump_tables([data], rs)
    check("a dense table is found", len(tables), 1)
    check("the entry count is the case count", tables[0]["count"], 4)
    check("the entry addends survive", [e["addend"] for e in tables[0]["entries"]], [0, 1, 2, 3])
    check("a sparse section is not a table", jump_tables([data], rs[:2]), [])

    # pool_entries: boundaries from local symbols
    sec = dict(name=".sdata2", size=8, data=struct.pack(">ff", 0.0, 1.0))
    syms = [dict(name="@153", value=0, shndx=1, section=".sdata2"),
            dict(name="@154", value=4, shndx=1, section=".sdata2")]
    entries = pool_entries(sec, syms, [])
    check("two pool entries", len(entries), 2)
    check("entry sizes", [e["size"] for e in entries], [4, 4])
    check("entry values", [literal_value(e["bytes"], "") for e in entries], ["0x00000000", "1 (0x3F800000)"])

    # DOL section parsing and byte lookup
    blob = bytearray(0x200)
    struct.pack_into(">7I", blob, 0x00, 0x100, 0, 0, 0, 0, 0, 0)      # text0 offset
    struct.pack_into(">7I", blob, 0x48, 0x80004000, 0, 0, 0, 0, 0, 0)  # text0 addr
    struct.pack_into(">7I", blob, 0x90, 4, 0, 0, 0, 0, 0, 0)          # text0 size
    blob[0x100:0x104] = b"\xde\xad\xbe\xef"
    segs = dol_sections(bytes(blob))
    check("the DOL's sections parse", segs, [(0x80004000, 4, 0x100)])
    check("the DOL's bytes read", dol_bytes(bytes(blob), segs, 0x80004000, 4), b"\xde\xad\xbe\xef")
    check("an address off the image is None", dol_bytes(bytes(blob), segs, 0x90000000, 4), None)

    # name_address
    check("a lbl name encodes its address", name_address("lbl_80594EE0"), 0x80594EE0)
    check("a fn name encodes its address", name_address("fn_800CCFB0"), 0x800CCFB0)
    check("a plain name has no address", name_address("RSOLink"), None)

    # dump_names parsing: name / demangled args / zz_ placeholder, from a fixture zip
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        zpath = os.path.join(tmp, "DumpSymbols.zip")
        with zipfile.ZipFile(zpath, "w") as z:
            z.writestr("Dump_Loading85.raw.map",
                       "RSOStaticLocateObject 804d9ec4 f\n"
                       "kbd_open(unsigned 80040798 f\n"
                       "zz_0040598_ 80040598 f\n"
                       "_restgpr_14 80000200 f\n")
        names = dump_names(zpath)
        check("a real dump name is kept", names.get(0x804D9EC4), "RSOStaticLocateObject")
        check("demangled args are stripped", names.get(0x80040798), "kbd_open")
        check("a zz_ placeholder is no name", 0x80040598 in names, False)
        check("a runtime helper name is kept", names.get(0x80000200), "_restgpr_14")

    # the whole pipeline against a real object, when the build tree is present
    real = os.path.join(ROOT, "build", "RMHE08", "obj", "auto", "800CCFB0_fn_800CCFB0.o")
    if os.path.exists(real):
        d = build(ROOT, "auto/800CCFB0_fn_800CCFB0", real, dump=False)
        check("the real object parses", d["error"], None)
        check("the file name is recovered from the DOL", d["source_file"], "ef_line.cpp")
        check("the panic lines are 42/43/44", [p["line"] for p in d["panic"]], [42, 43, 44])
        check("a mangled C++ symbol is recognised",
              any(s["name"].startswith("Panic__") and s["language"] == "C++" for s in d["externals"]), True)
        check("the section layout has .text and extab",
              {".text", "extab", "extabindex"} <= {s["name"] for s in d["sections"]}, True)
        check("the external literals carry their values",
              any(e["name"] == "lbl_807962E8" and e["value"] == "0x00000000" for e in d["literals"]), True)
        check("a unit with an external pool has the pool blank",
              any("owns no pool section" in b for b in d["blanks"]), True)
        text = render(d, title=True)
        check("the render names the source file", "`ef_line.cpp`" in text, True)
        check("the render carries the panic lines", "42, 43, 44" in text, True)
        check("the render has a blanks section", "Blanks - what the binary does not answer" in text, True)
        # the brief integration: a worker's brief *is* the dossier plus the task (plan 7.3)
        try:
            from units import brief as brief_mod
        except ImportError:
            import brief as brief_mod  # type: ignore
        b = brief_mod.build(ROOT, ROOT, "auto/800CCFB0_fn_800CCFB0", None)
        btext = brief_mod.render(ROOT, b, None)
        check("the brief carries the dossier block", "The binary dossier" in btext, True)
        check("the brief's dossier names the source file", "`ef_line.cpp`" in btext, True)
        check("the dossier rides part 2, before the unit header",
              btext.index("The binary dossier") < btext.index("## 3 · What is already known"), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# ------------------------------------------------------------------------------------------------------------------
# CLI
# ------------------------------------------------------------------------------------------------------------------

def resolve_unit(main: str, unit: str) -> tuple[str, str, dict, list]:
    """(source name, target object, splits ranges, map rows) for a unit - no build, no ninja.

    The CLI uses this instead of `brief.build` so that `dossier.py <unit>` never touches `ninja` (not even
    the read-only `ninja -t commands` the brief needs for its flags) and works before the unit is built.
    """
    try:
        from units import brief  # lazy: brief imports this module at load time
    except ImportError:
        import brief  # type: ignore
    unit = unit.strip("/")
    name = brief.source_name(unit, main)
    ranges = brief.splits_range(main, unit)
    text = ranges.get(".text")
    syms = brief.symbols_in_range(main, text[0], text[1]) if text else []
    target = os.path.join(main, "build", GAME, "obj", *name.split("/"))
    target = os.path.splitext(target)[0] + ".o"
    return name, target, ranges, syms


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--out", default=None, help="write the markdown here (default: stdout)")
    ap.add_argument("--json", action="store_true", help="print the dossier data instead of markdown")
    ap.add_argument("--no-dump", action="store_true", help="do not consult the shared dump map")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.unit:
        ap.print_help()
        return 0

    from units import recompile as rc  # type: ignore
    main_root = rc.main_root(rc.worktree_root())
    _name, target, ranges, syms = resolve_unit(main_root, args.unit)
    d = build(main_root, args.unit, target, ranges=ranges, map_symbols=syms, dump=not args.no_dump)
    if args.json:
        print(json.dumps(d, indent=2, default=str))
        return 0
    text = render(d, title=True)
    if args.out:
        os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
        open(args.out, "w", encoding="utf-8", newline="\n").write(text)
        print("wrote %s (%d lines)" % (args.out, text.count("\n")))
    else:
        print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
