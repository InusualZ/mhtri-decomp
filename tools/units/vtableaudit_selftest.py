#!/usr/bin/env python3
"""Self-test for `tools/units/vtableaudit.py`.

Four things here can silently make the sweep lie, so each gets its own block of checks:

* the **run rule** - `find_runs` must find maximal blocks of consecutive code pointers and must not call a
  single pointer a table (a run needs two entries), because one stray code address in a data section is an
  ordinary pointer;
* **address resolution** - a word's value comes from the relocation that sits on it (`section base + symbol
  + addend`), and a symbol that is *undefined* in the object has no address there at all: it is resolved
  through `symbols.txt` or the `_XXXXXXXX` spelling every `lbl_`/`fn_` name carries. Get this wrong and the
  sweep reports the wrong addresses, or nothing at all;
* the **verdict** - a run our object emits or references is fine, an owned run it neither emits nor
  references is the rule-10 violation, and a `.ctors`/`extab` run is **not** a vtable (`n/a`), because a run
  the linker/compiler puts in `.ctors` cannot be one;
* the **section comparison** - a non-`.text` section our object does not carry at all is the `missing` case
  the report exists for, and metadata (`.symtab`, `.rela*`, `.comment`, `.note.split`) is never compared.

The real `src/`, `configure.py`, `splits.txt`, `symbols.txt`, `build/` and DOL are never read or written:
every fixture lives in a temp directory, so the selftest is green on a tree with no build. The fixture tree
is hashed before and after the sweep, which is how "the tool only reads" is checked rather than promised.

    python tools/units/vtableaudit_selftest.py
    python tools/units/vtableaudit.py --selftest
"""

from __future__ import annotations

import hashlib
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vtableaudit as va  # noqa: E402

# info byte = (bind << 4) | type
GLOBAL_FUNC = (1 << 4) | 2                        # STB_GLOBAL, STT_FUNC
GLOBAL_OBJ = (1 << 4) | 1                        # STB_GLOBAL, STT_OBJECT
LOCAL_OBJ = (0 << 4) | 1
LOCAL_NOTYPE = (0 << 4) | 0
LOCAL_FILE = (0 << 4) | 4                        # STB_LOCAL, STT_FILE
SHN_UNDEF = 0
R_PPC_ADDR32 = 1

TEXT, DATA, RODATA = ".text", ".data", ".rodata"


# --------------------------------------------------------------------------------------------------
# a minimal ELF32 big-endian object with the sections, symbols and relocations the reader needs
# --------------------------------------------------------------------------------------------------
def build_object(symbols, content, relocs=()) -> bytes:
    """An ELF32 BE object.

    `content` is `[(section_name, sh_type, bytes)]` in section-index order (index 0 is the null header);
    `symbols` is `[(name, value, size, info, shndx)]` with the null row first; `relocs` is
    `[(section_name, offset, symbol_index, type, addend)]`. This is the shape `elfsect.sections` reads and
    `vtableaudit.read_object` expects - including a `.symtab` whose `sh_link` points at `.strtab`, which is
    what makes a relocation's symbol index resolvable.
    """
    strtab, offsets = b"\x00", []
    for name, *_ in symbols:
        offsets.append(len(strtab))
        strtab += name.encode("latin-1") + b"\x00"
    symtab = b"".join(struct.pack(">IIIBBH", off if name else 0, value, size, info, 0, shndx)
                      for (name, value, size, info, shndx), off in zip(symbols, offsets))
    grouped = {}
    for section, offset, sym_index, typ, addend in relocs:
        grouped.setdefault(section, []).append((offset, sym_index, typ, addend))
    sections = [(name, typ, data, 0, 0) for name, typ, data in content]
    symtab_index = 1 + len(sections) + len(grouped)
    strtab_index = symtab_index + 1
    for name, _typ, _data in content:
        rows = grouped.get(name)
        if rows:
            body = b"".join(struct.pack(">IIi", off, (si << 8) | typ, add)
                            for off, si, typ, add in sorted(rows))
            sections.append((".rela" + name, 4, body, symtab_index, 12))
    sections.append((".symtab", 2, symtab, strtab_index, 16))
    sections.append((".strtab", 3, strtab, 0, 0))
    shstr, name_off = b"\x00", []
    for name, *_ in sections:
        name_off.append(len(shstr))
        shstr += name.encode("latin-1") + b"\x00"
    shstr_index = len(sections) + 1
    name_off.append(len(shstr))
    shstr += b".shstrtab\x00"
    body, body_off = b"", []
    for _n, _t, data, _l, _e in sections:
        body_off.append(len(body))
        body += data
    shstr_off = len(body)
    body += shstr
    shentsize, ehsize = 40, 52
    shnum = len(sections) + 2
    shoff = ehsize + len(body)
    ehdr = (b"\x7fELF\x01\x02\x01" + b"\x00" * 9 +
            struct.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, ehsize, 0, 0,
                        shentsize, shnum, shstr_index))
    shdrs = struct.pack(">10I", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    for i, (_name, typ, data, link, entsize) in enumerate(sections):
        shdrs += struct.pack(">10I", name_off[i], typ, 0, 0, ehsize + body_off[i], len(data),
                             link, 0, 4, entsize)
    shdrs += struct.pack(">10I", name_off[len(sections)], 3, 0, 0, ehsize + shstr_off, len(shstr),
                         0, 0, 1, 0)
    return ehdr + body + shdrs


def fixture_object(text_size, data_size, data_relocs, extra=()):
    """A split-looking object: `.text` of `text_size` zero bytes, `.data` of `data_size`, plus `extra`.

    `data_relocs` are `(offset, symbol_index)` R_PPC_ADDR32 sites, so a "table" is built exactly the way the
    splitter builds one - a relocation per entry, the bytes left at zero. `extra` is extra content sections.
    """
    symbols = [("", 0, 0, 0, 0),
               ("unit.cpp", 0, 0, LOCAL_FILE, 0xFFF1),
               ("fn_80004000", 0, 16, GLOBAL_FUNC, 1),
               ("fn_80004010", 0x10, 16, GLOBAL_FUNC, 1),
               ("tbl_80050000", 0, 16, LOCAL_OBJ, 2)]
    content = [(TEXT, 1, b"\x00" * text_size), (DATA, 1, b"\x00" * data_size)]
    content += list(extra)
    relocs = [(DATA, off, sym, R_PPC_ADDR32, 0) for off, sym in data_relocs]
    return build_object(symbols, content, relocs), symbols


def write_elf(path, blob) -> str:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(blob)
    return path


# --------------------------------------------------------------------------------------------------
# the fixture tree
# --------------------------------------------------------------------------------------------------
SPLITS = """Sections:
\t.text       type:code align:32
\t.rodata     type:rodata align:32
\t.data       type:data align:32

t/unit.cpp:
\t.text       start:0x80004000 end:0x80004100
\t.data       start:0x80050000 end:0x80050010

t/gone.cpp:
\t.text       start:0x80004200 end:0x80004300
\t.data       start:0x80050100 end:0x80050108

t/ref.cpp:
\t.text       start:0x80004400 end:0x80004600
\t.data       start:0x80050200 end:0x80050208

t/init.cpp:
\t.text       start:0x80004800 end:0x80004900
\t.ctors      start:0x8056F2C0 end:0x8056F2C8 rename:.ctors$10
"""

SYMBOLS = """fn_80004000 = .text:0x80004000; // type:function size:0x10
fn_80004010 = .text:0x80004010; // type:function size:0x10
tbl_80050000 = .data:0x80050000; // type:object size:0x10 scope:local
lbl_80050200 = .data:0x80050200; // type:object size:0x8 scope:local
UnitVTable = .data:0x80050000; // type:object size:0x10 scope:global
FarVTable = .data:0x80600000; // type:object size:0x8 scope:global
"""

# The definition index's input: a struct whose member at +0x00 is a pointer to a table of function pointers.
# The member is deliberately spelled `pVtbl` - not `vtable`/`vtbl` - so the selftest proves the scan finds
# an assignment through the DEFINITION and not through a hard-coded name.
UNIT_HEADER = """typedef struct Vtbl {
    void* rtti_00;
    void* rtti_04;
    void (*slot_08)(void* self);
    void (*slot_0C)(void* self);
} Vtbl;

typedef struct Unit {
    Vtbl* pVtbl;                 /* +0x00 */
    unsigned int field;          /* +0x04 */
} Unit;
"""

CONFIGURE = """config.libs = [
    Object(NonMatching, "t/unit.cpp"),
    Object(NonMatching, "t/gone.cpp"),
    Object(NonMatching, "t/ref.cpp"),
    Object(NonMatching, "t/init.cpp"),
]
"""

# The source fixtures: one legal rule-10 Case 2 reference (outside every registered range), one reference
# to another unit's range, and one to the file's own range - the last two named with the `_VTable`
# spelling that the old `lbl_XXXXXXXX`-only scan could not see, which is the blind spot this selftest
# exists for. Two lines must NOT match.
UNIT_SRC = """void f(void) {
    self->pVtbl = &FarVTable;             /* no registered range owns this - rule 10 Case 2 */
    self->vtable = lbl_80050200;          /* t/ref.cpp's range - that unit's table */
    self->pVtbl = &UnitVTable;            /* this file's OWN .data - the forbidden shape */
    vtable_size = 4;                      /* not a reference */
}
"""
REF_SRC = """void g(void) {
    s->vtable = (const Vtbl*)lbl_80050200;   /* this file's own range - the forbidden shape */
}
"""


def dol_header() -> bytes:
    """A 0x100-byte DOL header with one text section at 0x80004000..0x80004100 (its own `.text`)."""
    hdr = bytearray(0x100)
    struct.pack_into(">I", hdr, 0x00, 0x100)          # text[0] file offset
    struct.pack_into(">I", hdr, 0x48, 0x80004000)     # text[0] address
    struct.pack_into(">I", hdr, 0x90, 0x100)          # text[0] size
    return bytes(hdr)


def table_dol() -> bytes:
    """A DOL with a `.text` at 0x80004000 and a `.data` at 0x80050000 holding two of its code pointers.

    The structural ownership test reads these bytes, so it needs a DOL that really carries both sections.
    """
    hdr = bytearray(0x100)
    struct.pack_into(">I", hdr, 0x00, 0x100)          # text[0] file offset
    struct.pack_into(">I", hdr, 0x1C, 0x200)          # data[0] file offset
    struct.pack_into(">I", hdr, 0x48, 0x80004000)     # text[0] address
    struct.pack_into(">I", hdr, 0x64, 0x80050000)     # data[0] address
    struct.pack_into(">I", hdr, 0x90, 0x100)          # text[0] size
    struct.pack_into(">I", hdr, 0xAC, 8)              # data[0] size
    return bytes(hdr) + b"\x00" * 0x100 + struct.pack(">II", 0x80004000, 0x80004010)


def make_tree(root: str) -> dict:
    """Write the whole fixture tree and return its expected object paths."""
    paths = {
        "splits": os.path.join(root, "config", "RMHE08", "splits.txt"),
        "symbols": os.path.join(root, "config", "RMHE08", "symbols.txt"),
        "dol": os.path.join(root, "orig", "RMHE08", "sys", "main.dol"),
        "configure": os.path.join(root, "configure.py"),
        "unit_src": os.path.join(root, "src", "t", "unit.cpp"),
        "unit_hdr": os.path.join(root, "src", "t", "unit.h"),
        "ref_src": os.path.join(root, "src", "t", "ref.cpp"),
    }
    for key in ("splits", "symbols", "dol", "configure", "unit_src", "unit_hdr", "ref_src"):
        os.makedirs(os.path.dirname(paths[key]), exist_ok=True)
    with open(paths["splits"], "w", encoding="utf-8") as fh:
        fh.write(SPLITS)
    with open(paths["symbols"], "w", encoding="utf-8") as fh:
        fh.write(SYMBOLS)
    with open(paths["configure"], "w", encoding="utf-8") as fh:
        fh.write(CONFIGURE)
    with open(paths["dol"], "wb") as fh:
        fh.write(dol_header())
    with open(paths["unit_src"], "w", encoding="utf-8") as fh:
        fh.write(UNIT_SRC)
    with open(paths["unit_hdr"], "w", encoding="utf-8") as fh:
        fh.write(UNIT_HEADER)
    with open(paths["ref_src"], "w", encoding="utf-8") as fh:
        fh.write(REF_SRC)

    def obj_path(unit, side):
        return os.path.join(root, "build", "RMHE08", side, unit + ".o")

    def both(unit, target_blob, our_blob):
        write_elf(obj_path(unit, "obj"), target_blob)
        write_elf(obj_path(unit, "src"), our_blob)

    # t/unit.cpp: a three-entry table at 0x80050000 (a real run of >= 2 words), emitted by our object too.
    table, _ = fixture_object(0x100, 16, [(0, 2), (4, 3), (8, 2)])
    both("t/unit", table, table)

    # t/gone.cpp: the target owns a two-entry table our object does not emit at all (no `.data`).
    gone_target, _ = fixture_object(0x100, 8, [(0, 2), (4, 3)])
    gone_our = build_object(
        [("", 0, 0, 0, 0),
         ("fn_80004000", 0, 16, GLOBAL_FUNC, 1)],
        [(TEXT, 1, b"\x00" * 0x100)])
    both("t/gone", gone_target, gone_our)

    # t/ref.cpp: our `.data` is short (0 words of it are code) and a `.text` relocation references the
    # abandoned table's address - the "owned but referenced" case - plus a `.rodata` the target lacks.
    ref_target, _ = fixture_object(0x100, 8, [(0, 2), (4, 3)])
    ref_our = build_object(
        [("", 0, 0, 0, 0),
         ("fn_80004000", 0, 16, GLOBAL_FUNC, 1),
         ("lbl_80050200", 0, 0, LOCAL_NOTYPE, SHN_UNDEF)],
        [(TEXT, 1, b"\x00" * 0x200), (DATA, 1, b"\x00" * 4), (RODATA, 1, b"\x00" * 16)],
        [(TEXT, 0, 2, R_PPC_ADDR32, 0)])
    both("t/ref", ref_target, ref_our)

    # t/init.cpp: the split renames its own `.ctors` range (dtk's `.ctors$10`), and the run lives in the
    # renamed object section - the case that made `Runtime.PPCEABI.H/__init_cpp_exceptions.cpp` look like a
    # unit with a missing target section.
    init_target = build_object(
        [("", 0, 0, 0, 0),
         ("fn_80004000", 0, 16, GLOBAL_FUNC, 1),
         ("fn_80004010", 0x10, 16, GLOBAL_FUNC, 1)],
        [(TEXT, 1, b"\x00" * 0x100), (".ctors$10", 1, b"\x00" * 8)],
        [(".ctors$10", 0, 1, R_PPC_ADDR32, 0), (".ctors$10", 4, 2, R_PPC_ADDR32, 0)])
    both("t/init", init_target, init_target)
    return paths


def tree_digest(root: str) -> dict:
    """`{relative path: sha1}` for every file the fixture tree holds - the read-only check's witness."""
    out = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = sorted(dirnames)
        for fn in sorted(filenames):
            p = os.path.join(dirpath, fn)
            with open(p, "rb") as fh:
                out[os.path.relpath(p, root).replace("\\", "/")] = hashlib.sha1(fh.read()).hexdigest()
    return out


# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # -- the parsers ----------------------------------------------------------------------------
    splits = va.parse_splits(SPLITS)
    check("splits: four units", sorted(splits),
          ["t/gone.cpp", "t/init.cpp", "t/ref.cpp", "t/unit.cpp"])
    check("splits: a unit's ranges", [(r["section"], r["start"], r["end"], r["object"])
                                      for r in splits["t/unit.cpp"]],
          [(TEXT, 0x80004000, 0x80004100, TEXT), (DATA, 0x80050000, 0x80050010, DATA)])
    check("splits: the Sections: header is not a unit", "Sections" in splits, False)
    renamed = va.parse_splits("u.c:\n\t.ctors\tstart:0x8056F2C0 end:0x8056F2C4 rename:.ctors$10\n")
    check("splits: a renamed section keeps both spellings",
          [(r["section"], r["object"]) for r in renamed["u.c"]], [(".ctors", ".ctors$10")])
    symbols = va.parse_symbols(SYMBOLS)
    check("symbols: name -> (section, address, size)",
          symbols["lbl_80050200"], (DATA, 0x80050200, 8))
    check("symbols: comments do not create rows", len(symbols), 6)
    check("dol: text ranges from the header", va.dol_text_ranges(dol_header()),
          [(0x80004000, 0x80004100)])
    check("dol: a runt is not a DOL", va.dol_text_ranges(b"\x7fELF" + b"\x00" * 8), [])

    # -- the range helpers ----------------------------------------------------------------------
    check("merge_ranges coalesces touching ranges",
          va.merge_ranges([(10, 20), (20, 30), (40, 50), (5, 8)]), [(5, 8), (10, 30), (40, 50)])
    check("in_ranges is half-open", (va.in_ranges([(10, 20)], 10), va.in_ranges([(10, 20)], 19),
                                     va.in_ranges([(10, 20)], 20)), (True, True, False))
    check("in_ranges on an empty list", va.in_ranges([], 0x80000000), False)

    # -- the run rule ---------------------------------------------------------------------------
    check("a single code pointer is not a table", va.find_runs([0, 1, 0]), [])
    check("two consecutive is a table", va.find_runs([0, 1, 1, 0]), [(1, 2)])
    check("maximal block, not overlapping windows", va.find_runs([0, 1, 1, 1, 0, 1, 1]), [(1, 3), (5, 2)])
    check("an all-code section is one run", va.find_runs([1, 1, 1, 1]), [(0, 4)])
    check("min_words is honoured", va.find_runs([0, 1, 1, 0], min_words=3), [])

    # -- address resolution ---------------------------------------------------------------------
    base = {TEXT: 0x80004000, DATA: 0x80050000}
    order = ["", TEXT, DATA]
    defined = {"name": "fn_80004010", "value": 0x10, "shndx": 1}
    check("a defined symbol is its section base plus its offset",
          va.resolve_symbol(defined, order, base, {}), (0x80004010, TEXT))
    undef = {"name": "tbl_80050000", "value": 0, "shndx": 0}
    check("an undefined symbol resolves through symbols.txt",
          va.resolve_symbol(undef, order, base, symbols), (0x80050000, DATA))
    check("an undefined symbol falls back to its _XXXXXXXX spelling",
          va.resolve_symbol({"name": "lbl_80050200", "value": 0, "shndx": 0}, order, base, {}),
          (0x80050200, None))
    check("an unresolved undefined symbol is None",
          va.resolve_symbol({"name": "__dl__FPv", "value": 0, "shndx": 0}, order, base, {}),
          (None, None))
    check("an SHN_ABS symbol is its own value",
          va.resolve_symbol({"name": "abs", "value": 0x1234, "shndx": 0xFFF1}, order, base, {}),
          (0x1234, None))
    check("a symbol in an unowned section is None",
          va.resolve_symbol({"name": "x", "value": 0, "shndx": 2}, order, {TEXT: 0x80004000}, {}),
          (None, None))

    blob, _ = fixture_object(0x100, 8, [(0, 2)])
    with tempfile.TemporaryDirectory() as tmp:
        obj = va.read_object(write_elf(os.path.join(tmp, "one.o"), blob))
        check("the reader is None on a file that is not there",
              va.read_object(os.path.join(tmp, "definitely-not-here.o")), None)
        write_elf(os.path.join(tmp, "junk.o"), b"not an ELF\n")
        check("the reader is None on junk, not an exception",
              va.read_object(os.path.join(tmp, "junk.o")), None)
    check("the reader sees both content sections",
          sorted(k for k in obj["sections"] if k in (TEXT, DATA)), [".data", ".text"])
    check("the reader groups relocations by section", sorted(obj["relocs"]), [DATA])
    check("the reader keeps the undefined rows (index-addressed)",
          [s["name"] for s in obj["symbols"]][:3], ["", "unit.cpp", "fn_80004000"])
    check("a word with a relocation reads as a reloc",
          va.read_word(obj, DATA, 0, base, symbols), ("reloc", 0x80004000, TEXT))
    check("a word without one reads its own bytes",
          va.read_word(obj, DATA, 4, base, symbols), ("raw", 0, None))
    check("past the section is None", va.read_word(obj, DATA, 8, base, symbols), ("raw", None, None))
    check("is_code_pointer: a code symbol is decisive",
          va.is_code_pointer("reloc", 0x60000000, TEXT, []), True)
    check("is_code_pointer: a raw address in .text counts",
          va.is_code_pointer("raw", 0x80004004, None, [(0x80004000, 0x80004100)]), True)
    check("is_code_pointer: an address outside .text does not",
          va.is_code_pointer("raw", 0x80050000, None, [(0x80004000, 0x80004100)]), False)
    check("is_code_pointer: an unresolvable word does not",
          va.is_code_pointer("reloc", None, None, [(0x80004000, 0x80004100)]), False)

    # -- the verdict and the section comparison -------------------------------------------------
    check("classify_reference: this unit's range is `own`",
          va.classify_reference(0x80050000, [(0x80050000, 0x80050010)], [(0x80050000, 0x80050010)]),
          "own")
    check("classify_reference: another unit's range is `foreign`",
          va.classify_reference(0x80050000, [(0x80050100, 0x80050108)], [(0x80050000, 0x80050010)]),
          "foreign")
    check("classify_reference: no registered range is `external`",
          va.classify_reference(0x80600000, [(0x80050000, 0x80050010)], [(0x80050000, 0x80050010)]),
          "external")
    check("section_diff: a section we do not carry is `missing`",
          va.section_diff({TEXT: 0x10}, {TEXT: 0x10, DATA: 8}),
          [{"section": DATA, "ours": 0, "target": 8, "kind": "missing"}])
    check("section_diff: a section the target lacks is `extra`",
          va.section_diff({RODATA: 4}, {})[0]["kind"], "extra")
    check("section_diff: shorter / longer",
          [va.section_diff({DATA: 4}, {DATA: 8})[0]["kind"],
           va.section_diff({DATA: 12}, {DATA: 8})[0]["kind"]], ["short", "long"])
    check("section_diff: .text and metadata are never compared",
          va.section_diff({TEXT: 1, ".symtab": 2, ".rela.text": 3, ".comment": 4,
                           ".note.split": 5, DATA: 4},
                          {TEXT: 9, ".symtab": 7, ".rela.text": 8, ".comment": 6,
                           ".note.split": 5, DATA: 8}),
          [{"section": DATA, "ours": 4, "target": 8, "kind": "short"}])
    check("symbol_at: an exact name",
          va.symbol_at(symbols, DATA, 0x80050200), "lbl_80050200")
    check("symbol_at: the nearest symbol at or below",
          va.symbol_at(symbols, DATA, 0x80050004), "tbl_80050000+0x4")
    check("symbol_at: nothing in that section",
          va.symbol_at(symbols, RODATA, 0x80060000), None)
    check("the scan takes a plain, a cast and an `&` spelling, and filters by the member's DEFINITION",
          [(h["field"], h["symbol"]) for h in va.scan_text_assignments(
              "  a->vtable = lbl_80050000;\n  b->vtable = (const Vtbl*)lbl_80050200;\n"
              "  c->pVtbl = &SomeVTable;\n  d->vtable_slot = lbl_80050200;\n  vtable_size = 4;\n",
              {"pVtbl": {"pointee": "Vtbl"}})],
          [("vtable", "lbl_80050000"), ("vtable", "lbl_80050200"), ("pVtbl", "SomeVTable")])
    check("the scan does not read `==` as an assignment",
          va.scan_text_assignments("  a->vtable == 0;\n  b->vtable != 0;\n", {}), [])
    check("the scan does not read an assignment QUOTED IN A COMMENT as one",
          va.scan_text_assignments("/* `self->vtable = &OwnVTable;` was the defect */\n"
                                   "// self->vtable = &OwnVTable;\n", {}), [])
    check("... and it does not read one inside a string literal either",
          va.scan_text_assignments('  log("s->vtable = &OwnVTable;");\n', {}), [])

    # -- the definition index (a +0x00 fn-table pointer is a class with inheritance) ---------------
    defs = va.type_definitions({"t/unit.h": UNIT_HEADER})
    check("the definition index reads a function-pointer member",
          [x[1] for x in defs["Vtbl"]], ["rtti_00", "rtti_04", "slot_08", "slot_0C"])
    check("a +0x00 pointer to a function-pointer table is found, whatever the member is called",
          {k: v["pointee"] for k, v in va.fn_table_fields(defs).items()}, {"pVtbl": "Vtbl"})
    check("a linked-list `next` to a struct that merely holds a callback is NOT a vtable field",
          va.fn_table_fields(va.type_definitions({"c.c": "typedef struct Chain { struct Chain* next;\n"
                                                  "  void (*dtor)(void*);\n  void* object; } Chain;\n"
                                                  "typedef struct Top { Chain* next; } Top;\n"})),
          {})
    check("a `pad`/`rtti`-only filler does not disqualify a vtable",
          va.fn_table_fields(va.type_definitions({"h.h": "typedef struct V { void* rtti_00;\n"
                                                  "  void* rtti_04; u8 pad08[0x20];\n"
                                                  "  void (*a)(void); void (*b)(void); } V;\n"
                                                  "typedef struct T { V* p; } T;\n"})),
          {"p": {"pointee": "V", "file": "h.h", "type": "T"}})

    # -- the structural ownership test ------------------------------------------------------------
    check("a table whose entries are this unit's code is structurally the unit's own vtable",
          va.table_belongs_to_unit(table_dol(), (DATA, 0x80050000, 8), [(0x80004000, 0x80004100)]),
          True)
    check("... and a table pointing elsewhere is not",
          va.table_belongs_to_unit(table_dol(), (DATA, 0x80050000, 8), [(0x80006000, 0x80006100)]),
          False)
    check("an address the DOL does not cover is not an owned table",
          va.table_belongs_to_unit(table_dol(), (DATA, 0x80070000, 8), [(0x80004000, 0x80004100)]),
          False)

    # -- violation_keys: the `--diff` comparison's unit -------------------------------------------------
    check("violation_keys names a run and an own-range assignment distinctly",
          va.violation_keys({"violations": [{"unit": "u.cpp", "section": ".data",
                                              "address": 0x80050000, "words": 2}],
                             "references": [{"file": "src/u.cpp", "line": 3,
                                             "symbol": "X", "kind": "own", "unit": "u"},
                                            {"file": "src/u.cpp", "line": 4,
                                             "symbol": "Y", "kind": "external", "unit": "u"}]}),
          ["ref:src/u.cpp:3:X", "run:.data:80050000"])

    # -- a rename must produce delta 0 (the incident `--diff` exists for) ---------------------------
    # plan §12 re-homes a placeholder-path unit routinely (`fn_80429B94.cpp` ->
    # `Network/network_pat_control.cpp`), and `--diff` reported "17 before, 17 after, 7 added" for a pure
    # rename because the run key carried the unit name.  The run key is the range now, which a rename keeps
    # (an address is unique in the DOL); the `ref:` key's file is translated through the rename map.
    run_before = {"violations": [{"unit": "fn_80429B94.cpp", "section": ".data",
                                  "address": 0x806038E8, "words": 5}],
                  "references": [{"file": "src/fn_80429B94.cpp", "line": 9, "symbol": "lbl_80594A18",
                                  "kind": "own", "unit": "fn_80429B94.cpp"}]}
    run_after = {"violations": [{"unit": "Network/network_pat_control.cpp", "section": ".data",
                                 "address": 0x806038E8, "words": 5}],
                 "references": [{"file": "src/Network/network_pat_control.cpp", "line": 9,
                                 "symbol": "lbl_80594A18", "kind": "own",
                                 "unit": "Network/network_pat_control.cpp"}]}
    rename = {"fn_80429B94.cpp": "Network/network_pat_control.cpp",
              "src/fn_80429B94.cpp": "src/Network/network_pat_control.cpp"}
    check("a renamed unit's runs and own-range refs measure delta 0",
          sorted(set(va.violation_keys(run_after)) - set(va.violation_keys(run_before, rename))), [])
    check("... because the run key is the range, not the unit",
          va.violation_keys(run_before, rename), va.violation_keys(run_after))
    check("... and a `ref:` without the translation is the one that would read as an addition",
          sorted(set(va.violation_keys(run_after)) - set(va.violation_keys(run_before))),
          ["ref:src/Network/network_pat_control.cpp:9:lbl_80594A18"])

    # -- rename_map: the ref path -> the path the working tree now spells ---------------------------
    with tempfile.TemporaryDirectory() as tmp:
        def rmg(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)
        rmg("init", "-q")
        os.makedirs(os.path.join(tmp, "src", "old"))
        with open(os.path.join(tmp, "src", "old", "unit.cpp"), "w", encoding="utf-8") as fh:
            fh.write("int x;\n")
        rmg("add", "-A")
        rmg("commit", "-q", "-m", "base")
        base_ref = subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()
        os.makedirs(os.path.join(tmp, "src", "new"))
        os.rename(os.path.join(tmp, "src", "old", "unit.cpp"),
                  os.path.join(tmp, "src", "new", "unit.cpp"))
        rmg("add", "-A")
        rmg("commit", "-q", "-m", "re-home the unit")
        check("rename_map reads a committed `git mv` (what `--diff <base>` sees)",
              va.rename_map(tmp, base_ref), {"src/old/unit.cpp": "src/new/unit.cpp"})
        check("... and an unchanged tree maps nothing",
              va.rename_map(tmp, "HEAD"), {})

    # -- `--at`: one vtable out of the DOL, with each target's owner ---------------------------------
    # The census a lane hand-built twice (114 slots, 62 wrong) - the mode exists so it is never built by
    # hand again.  The fixture's DOL is the table one, so the slots are real DOL words.
    with tempfile.TemporaryDirectory() as tmp:
        make_tree(tmp)
        with open(os.path.join(tmp, "orig", "RMHE08", "sys", "main.dol"), "wb") as fh:
            fh.write(table_dol())
        at_tree = va.load_tree(tmp)
        slots = va.vtable_slots(at_tree, 0x80050000)
        check("--at reads the table's slots out of the DOL",
              [(s["address"], s["target"]) for s in slots],
              [(0x80050000, 0x80004000), (0x80050004, 0x80004010)])
        check("... with each target's OWNER, by address not by name",
              [s["owner"] for s in slots], ["t/unit.cpp", "t/unit.cpp"])
        check("... and the symbol the map names at it",
              [s["symbol"] for s in slots], ["fn_80004000", "fn_80004010"])
        check("... stopping at the data section's end (a non-code word ends the table)", len(slots), 2)
        check("--at names the reference object's relocation for each slot (dossier.parse_elf)",
              va.reference_slots(tmp, at_tree, 0x80050000, len(slots)),
              {0: "fn_80004000", 1: "fn_80004010"})
        check("a lone code word is not a table", va.vtable_slots(at_tree, 0x80050004), [])
        check("a word outside the DOL's sections is not a table", va.vtable_slots(at_tree, 0x80060000), [])
        tool = os.path.join(os.path.dirname(os.path.abspath(__file__)), "vtableaudit.py")
        p = subprocess.run([sys.executable, tool, "--main", tmp, "--at", "0x80050000"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("--at on the CLI lists the slots and exits 0",
              (p.returncode, "+0x000" in p.stdout and "t/unit.cpp" in p.stdout), (0, True))
        p = subprocess.run([sys.executable, tool, "--main", tmp, "--at", "0x80060000"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("--at with no table there exits 2 and says so",
              (p.returncode, "no vtable there" in p.stdout), (2, True))

    # -- end to end, over the fixture tree ------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        make_tree(tmp)
        before = tree_digest(tmp)
        s = va.sweep(tmp)

        check("sweep: four registered units, all built", (s["units_total"], s["units_built"]), (4, 4))
        check("sweep: the DOL supplied the .text ranges", s["text_source"], "dol")

        # (a) the fabricated run is found, with the right shape
        unit_runs = [r for r in s["runs"] if r["unit"] == "t/unit.cpp"]
        check("sweep: the fabricated run is one run", len(unit_runs), 1)
        run = unit_runs[0]
        check("sweep: the run's address, length and kind",
              (run["section"], run["address"], run["words"], run["kind"]),
              (".data", 0x80050000, 3, "table"))
        check("sweep: the run is named from symbols.txt", run["symbol"], "tbl_80050000")
        check("sweep: the run's entries resolve to .text", run["targets"],
              [0x80004000, 0x80004010, 0x80004000])
        check("sweep: an emitted run is not a violation", (run["verdict"], run["emitted"]),
              ("emitted", True))
        check("sweep: and its values match", run["values_match"], True)

        # (b) the owned-but-unemitted run IS the violation
        gone = [r for r in s["runs"] if r["unit"] == "t/gone.cpp"][0]
        check("sweep: an owned run our object does not emit is a violation",
              (gone["verdict"], gone["emitted"], gone["referenced"]), ("violation", False, False))
        check("sweep: it is named in the violation list",
              [(v["unit"], v["section"], v["address"], v["words"]) for v in s["violations"]],
              [("t/gone.cpp", ".data", 0x80050100, 2)])
        check("sweep: the run verdicts are counted",
              (s["run_verdicts"]["emitted"], s["run_verdicts"]["referenced"],
               s["run_verdicts"]["violation"]), (1, 1, 1))
        check("sweep: the run kinds are counted",
              (s["run_kinds"]["table"], s["run_kinds"]["initializer"], s["run_kinds"]["exception"]),
              (3, 1, 0))

        # a run our object references (but does not emit) is legal
        ref = [r for r in s["runs"] if r["unit"] == "t/ref.cpp"][0]
        check("sweep: a referenced run is not a violation",
              (ref["verdict"], ref["emitted"], ref["referenced"]), ("referenced", False, True))

        # initializer/exception runs are not vtables: they are the two kinds exempt from the emit rule
        check("only .ctors/.dtors/extab/extabindex runs are exempt from the emit rule",
              sorted(va.RUN_KIND), [".ctors", ".dtors", "extab", "extabindex"])
        init_rec = [r for r in s["records"] if r["unit"] == "t/init.cpp"][0]
        check("a renamed split section is read by its object name (`rename:.ctors$10`)",
              [(r["section"], r["kind"], r["words"], r["verdict"]) for r in init_rec["runs"]],
              [(".ctors$10", "initializer", 2, "n/a")])
        check("and no registered range is left without a target section",
              (init_rec["missing"], s["absent_target_sections"]), ([], []))

        # (c) section completeness
        kinds = {(d["unit"], d["section"]): d["kind"] for d in s["sections"]}
        check("sweep: our absent .data is reported as `missing`",
              kinds.get(("t/gone.cpp", ".data")), "missing")
        check("sweep: a shorter .data is reported as `short`",
              kinds.get(("t/ref.cpp", ".data")), "short")
        check("sweep: a section only we carry is reported as `extra`",
              kinds.get(("t/ref.cpp", ".rodata")), "extra")
        check("sweep: .text is never in the section report",
              [k for k in kinds if k[1] == ".text"], [])
        check("sweep: the section kinds are counted", s["section_kinds"]["missing"], 1)

        # (b) the source references, classified, with the legal case NOT reported
        check("sweep: four assignments found, any symbol spelling or member name",
              len(s["references"]), 4)
        check("sweep: one legal Case 2, one foreign, two own-range",
              s["reference_kinds"], {"external": 1, "foreign": 1, "own": 2, "unresolved": 0})
        by_kind = {r["kind"]: r for r in s["references"]}
        check("sweep: the external one names its file and line",
              (by_kind["external"]["file"], by_kind["external"]["line"]),
              ("src/t/unit.cpp", 2))
        check("sweep: the own-range one is the file's own .data",
              (by_kind["own"]["symbol"], by_kind["own"]["address"]),
              ("UnitVTable", 0x80050000))
        check("sweep: a `_VTable`-named table is found by ownership, not by spelling",
              sorted(r["symbol"] for r in s["references"] if r["symbol"].endswith("VTable")),
              ["FarVTable", "UnitVTable"])
        check("sweep: the `pVtbl` member was matched through the definition index",
              [r["field"] for r in s["references"] if r["symbol"] in ("UnitVTable", "FarVTable")],
              ["pVtbl", "pVtbl"])
        check("sweep: `vtable_size = 4;` is not a reference",
              [r for r in s["references"] if r["line"] == 5], [])
        check("sweep: the +0x00 fn-table pointer fields are reported",
              {k: v["pointee"] for k, v in s["fn_table_fields"].items()}, {"pVtbl": "Vtbl"})

        # the tool only reads: every fixture file is byte-identical afterwards
        check("sweep: nothing in the tree was written", tree_digest(tmp), before)

        # a tree with no DOL falls back to the registered code ranges
        os.remove(os.path.join(tmp, "orig", "RMHE08", "sys", "main.dol"))
        fallback = va.load_tree(tmp)
        check("no DOL: the registered code ranges are the fallback",
              (fallback["text_source"], fallback["text_ranges"]),
              ("splits", [(0x80004000, 0x80004100), (0x80004200, 0x80004300),
                          (0x80004400, 0x80004600), (0x80004800, 0x80004900)]))

        # --unit filters both the unit list and the source references
        one = va.sweep(tmp, only="t/gone")
        check("--unit selects one unit", [r["unit"] for r in one["records"]], ["t/gone.cpp"])
        check("--unit filters the source references",
              [r["file"] for r in one["references"]], [])
        check("--unit still finds that unit's violation", len(one["violations"]), 1)

        # an unbuilt unit is named, never silently passed
        os.remove(os.path.join(tmp, "build", "RMHE08", "src", "t", "gone.o"))
        s2 = va.sweep(tmp)
        check("a missing object is unbuilt, not clean",
              (s2["units_built"], [r["unit"] for r in s2["unbuilt"]]), (3, ["t/gone.cpp"]))
        check("the unbuilt unit names the missing path",
              s2["unbuilt"][0]["missing"], ["build/RMHE08/src/t/gone.o"])

    # -- (d) the .data emission order ----------------------------------------------------------
    def order_obj(names):
        """An object whose `.data` symbols are `names` at 0x20-byte strides, in the given order."""
        syms = [("", 0, 0, 0, 0)] + [(n, 0x20 * i, 0x20, LOCAL_OBJ, 2) for i, n in enumerate(names)]
        blob = build_object(syms, [(TEXT, 1, b"\x00" * 16), (DATA, 1, b"\x00" * (0x20 * len(names)))])
        with tempfile.TemporaryDirectory() as t:
            return va.read_object(write_elf(os.path.join(t, "o.o"), blob))

    classes = ["A", "B", "C"]
    check("vtable_class: plain", va.vtable_class("__vt__1A"), "A")
    check("vtable_class: qualified is unresolved", va.vtable_class("__vt__Q23ns1A"), None)
    good = va.emission_order(order_obj(["@1", "@2", "__vt__1C", "__vt__1B", "__vt__1A"]), classes)
    check("order: strings then reverse vtables is clean", (good["vtables"], good["findings"]), (3, []))
    tail = va.emission_order(order_obj(["@1", "__vt__1B", "@2", "__vt__1A", "@stringBase0", "@77", "@STRING@g__1AFv"]), classes)
    check("order: @NNN strings after a vtable are an inline tail, not a finding", tail["findings"], [])
    late = va.emission_order(order_obj(["@1", "__vt__1B", "glob", "__vt__1A", "@9"]), classes)
    check("order: an initialised global after a vtable is a finding (a tail string next to it is not)",
          [(f["kind"], f["symbol"], f["offset"]) for f in late["findings"]],
          [("vtable-before-data", "glob", 0x40)])
    check("is_string_literal: @NNN and @stringBaseN only",
          [va.is_string_literal(n) for n in ("@12", "@stringBase0", "@STRING@f__Fv", "@etb_80001000", "lbl_1", "@")],
          [True, True, True, False, False, False])
    up = va.emission_order(order_obj(["@1", "__vt__1A", "__vt__1B"]), classes)
    check("order: vtables in class order (ascending) is a finding",
          [(f["kind"], f["symbol"], f["offset"]) for f in up["findings"]], [("vtable-order", "__vt__1B", 0x40)])
    skip = va.emission_order(order_obj(["@1", "__vt__1A", "__vt__1Z"]), classes)
    check("order: an unresolved class is skipped and counted", (skip["unresolved"], skip["findings"]),
          (1, []))
    check("order: no .data section is clean", va.emission_order({"order": [], "symbols": []}, classes)["findings"], [])
    files = {"u.cpp": '#include "h.h"\nclass C : public B { virtual void f(); };\n',
             "h.h": 'class A;\nstruct A { virtual void f(); };\n#include "g.h"\n// class Z {\n',
             "g.h": "class B : A { };\n"}
    order = va.class_order_from_texts(lambda n, inc: (n, files[n]) if n in files else None, "u.cpp")
    check("class order: includes at their position, forward decls and comments ignored", order, ["A", "B", "C"])

    print("vtableaudit selftest: %d checks, %d failed" % (checks, len(fails)))
    for f in fails:
        print("  FAIL %s" % f)
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(selftest())
