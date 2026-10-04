"""lib.objcompare: sections, bytes, layout, symbols, relocations, undefined names, linkage, fingerprints."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os

from tools.lib import objcompare as oc
from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import SHN_ABS

TIER = "fixture"
FUNC = "func"


def extab(offsets, size=0x40):
    """An `extab` record of `size` bytes whose `__dl__FPv` relocations sit at `offsets` (the F41 shape)."""
    b = ElfBuilder().section("extab", b"\0" * size)
    b.symbol("__dl__FPv")
    for off in offsets:
        b.reloc("extab", off, "__dl__FPv", "R_PPC_ADDR32")
    return b.build()


def twin_data():
    """An object with TWO `.data` sections, each with its own `.rela.data` (dtk writes this): built as `.data` +
    `.datX` and renamed in the string table, since a name is the only thing that tells the two apart."""
    b = ElfBuilder().section(".data", b"\1" * 4).section(".datX", b"\2" * 8)
    b.symbol("first_ref").symbol("second_ref")
    b.reloc(".data", 0, "first_ref", "R_PPC_ADDR32").reloc(".datX", 0, "second_ref", "R_PPC_ADDR32")
    return b.build().replace(b".datX\0", b".data\0")


def test_sizes(c):
    c.check("size gaps: ours-extra and target-extra, metadata excluded",
            oc.size_gaps({".text": 92, ".comment": 124, ".data": 64}, {".text": 92, ".comment": 132, ".sdata2": 8}),
            ([(".sdata2", 8, 0)], [(".data", 64, 0)]))
    c.check("--all-sections keeps the metadata churn",
            oc.size_gaps({".comment": 124}, {".comment": 132}, all_sections=True), ([(".comment", 132, 124)], []))
    obj = ElfBuilder().section(".text", b"\0" * 8).nobits(".bss", 0x20).section(".data", b"").build()
    sizes = oc.section_sizes(obj)
    c.check("section_sizes omits empty sections and keeps NOBITS (tables included: size_gaps drops them)",
            ({k: v for k, v in sizes.items() if k in (".text", ".bss", ".data")}, ".symtab" in sizes),
            ({".text": 8, ".bss": 0x20}, True))
    sizes = oc.object_sizes(obj)
    c.check("object_sizes is objdump -h's view (no tables, no metadata)", sorted(sizes), [".bss", ".data", ".text"])
    c.check("... with the alignment exponent", sizes[".text"], (8, 2))
    c.check("object_sizes of a missing object is empty", oc.object_sizes("/no/such/object.o"), {})


def test_section_bytes(c):
    obj = ElfBuilder().section(".text", b"\x11\x22").nobits(".bss", 8).build()
    c.check("section_data reads the bytes", oc.section_data(obj, ".text"), b"\x11\x22")
    c.check("NOBITS reads as empty, the way objcopy extracts it", oc.section_data(obj, ".bss"), b"")
    c.check("an absent section is None", oc.section_data(obj, ".data"), None)
    twin = twin_data()
    c.check("a repeated name reads its LAST section, and object_sizes sizes that one",
            (oc.section_data(twin, ".data"), oc.object_sizes(twin)[".data"][0]), (b"\2" * 8, 8))
    c.check("first_difference / differing_bytes", (oc.first_difference(b"ab", b"ax"), oc.differing_bytes(b"ab", b"axyz")),
            (1, 3))
    c.check("an equal shared prefix has no first difference", oc.first_difference(b"ab", b"abc"), None)


def test_sections(c):
    moved = oc.sections(extab([0x2C, 0x34, 0x54]), extab([0x14, 0xAC, 0xB4]))
    c.check("F41: a same-size record whose relocations moved is one row", [g.section for g in moved], ["extab"])
    c.contains("... naming both offset sets", moved[0].why, "ours +0x14, +0xAC, +0xB4 (R_PPC_ADDR32); "
                                                             "target +0x2C, +0x34, +0x54 (R_PPC_ADDR32)")
    short = oc.sections(extab([0x2C], 0x80), extab([0x2C], 0x40))
    c.check("F39: a short record names both sizes", (short[0].ours, short[0].target), (0x40, 0x80))
    c.contains("... as target-extra", short[0].why, "target-extra 0x40 (64 B)")
    c.check("identical objects are silent", oc.sections(extab([4]), extab([4])), [])
    a = ElfBuilder().section(".text", b"\x01\x02\x03\x04").build()
    b = ElfBuilder().section(".text", b"\x01\x09\x03\x08").section(".data", b"\0" * 4).build()
    rows = oc.sections(a, b)
    c.check("a byte difference names the first offset and the count",
            rows[0].why, "bytes differ at +0x1 (ours 09, target 02) in 2 of 4 bytes")
    c.check("a section only ours has is ours-extra", rows[1].to_dict(),
            {"section": ".data", "ours": 4, "target": 0, "why": "ours-extra: the target has no such section (4 B)"})
    c.check("metadata is excluded unless asked",
            ".comment" in [g.section for g in oc.sections(a, ElfBuilder().section(".text", b"\x01\x02\x03\x04")
                                                          .comment().build(), all_sections=True)], True)
    c.check("a SectionGap is a Finding keyed by the unit", oc.sections(a, b)[1].finding("U/u").identity(),
            ("section-gap", "U/u", ".data", "ours-extra: the target has no such section (4 B)"))


def _layout(order, data, size=4):
    b = ElfBuilder().section(".text", data)
    for name, off in order:
        b.symbol(name, ".text", off, size, type=FUNC)
    return b.build()


def test_layout(c):
    fa, fb = b"\x11\x12\x13\x14", b"\x21\x22\x23\x24"
    ours = oc.section_symbols(_layout([("A", 4), ("B", 0)], fb + fa), ".text")
    theirs = oc.section_symbols(_layout([("A", 0), ("B", 4)], fa + fb), ".text")
    c.check("section_symbols maps each symbol to (offset, size)", ours, {"A": (4, 4), "B": (0, 4)})
    c.check("a permutation: every symbol at its own address, sizes agree", oc.mislaid_layout(fb + fa, fa + fb, ours,
                                                                                             theirs), (2, 8))
    c.check("an in-place residual is not a permutation", oc.mislaid_layout(fa + fa, fa + fb, theirs, theirs), None)
    lines = oc.section_byte_reasons(".text", fb + fa, fa + fb, ours, theirs)
    c.check("the refusal lines: first difference, count, permutation", [ln.split(" - ")[0] for ln in lines],
            [".text: bytes differ from the target object at +0x0 (ours 21, target 11)",
             ".text: 8 of 8 bytes differ from the target object", ".text: the section is a permutation"])
    stray = b"\x11\x12\x13\x99"
    big = b"\x31" * 4
    o2 = oc.section_symbols(_layout([("A", 4), ("B", 0), ("C", 8)], fb + stray + big), ".text")
    t2 = oc.section_symbols(_layout([("A", 0), ("B", 4), ("C", 8)], fa + fb + big), ".text")
    c.check("the weaker class: a moved symbol carrying a word of its own", oc.mislaid_order(fb + stray + big,
                                                                                         fa + fb + big, o2, t2),
            (2, 3, 7, 1))
    c.check("... is named by the reasons", "layout is a permutation" in
            oc.section_byte_reasons(".text", fb + stray + big, fa + fb + big, o2, t2)[2], True)


def test_symbols(c):
    T = {n: oc.SymbolSize(n, s, ".text") for n, s in (("big", 100), ("edge", 100), ("same", 64), ("gone", 32))}
    O = {n: oc.SymbolSize(n, s, ".text") for n, s in (("big", 40), ("edge", 52), ("same", 64), ("extra", 12))}
    rows = oc.symbols(T, O, 50.0)
    c.check("size-gap (strictly over the threshold), then missing, then extra",
            [(r.cls, r.name) for r in rows], [("size-gap", "big"), ("missing", "gone"), ("extra", "extra")])
    c.check("the gap row carries both sizes and the delta", (rows[0].target_size, rows[0].ours_size,
                                                             round(rows[0].delta, 6)), (100, 40, 0.6))
    c.check("the threshold is honoured", [r.name for r in oc.symbols(T, O, 45.0, "gap")], ["big", "edge"])
    c.check("a mode keeps only its class", [r.name for r in oc.symbols(T, O, mode="missing")], ["gone"])
    c.check("size_delta is symmetric and 1.0 with a zero side", (oc.size_delta(40, 100), oc.size_delta(0, 9)),
            (0.6, 1.0))
    obj = (ElfBuilder().section(".text", b"\0" * 8).section(".sdata2", b"\0" * 8)
           .symbol("f", ".text", 0, 8, type=FUNC).symbol("k", ".sdata2", 0, 8, type="object")
           .symbol("abs", None, 0, 4, shndx=SHN_ABS).symbol("ext").build())
    c.check("defined_symbols: by kind; ABS and undefined skipped", sorted(oc.defined_symbols(obj, {"code"})), ["f"])
    c.check("... the data kind", oc.defined_symbols(obj, oc.wanted_kinds("data"))["k"], oc.SymbolSize("k", 8, ".sdata2"))
    c.check("section kinds", [oc.section_kind(s) for s in ("extab", ".rela.text", ".text.hot", ".bss", ".comment")],
            ["other", "meta", "code", "data", "meta"])


def test_symbol_rows(c):
    target = ElfBuilder().section(".init", b"\xAA" * 8).symbol("pad_00_init", ".init", 0, 8).build()
    ours = (ElfBuilder().section(".init", b"\xAA" * 8).symbol("gTable", ".init", 0, 8, type="object")
            .symbol("label", ".init", 0, 0).build())
    row = oc.symbol_rows(target, ours)["pad_00_init"]
    c.check("a dtk-named row resolves by ADDRESS to the sized symbol there",
            (row["resolved_by"], row["candidate_name"], row["identical"]), ("address", "gTable", True))
    bad = ElfBuilder().section(".init", b"\xAB" * 8).symbol("gTable", ".init", 0, 8).build()
    c.check("... and still refuses different bytes", oc.symbol_rows(target, bad)["pad_00_init"]["identical"], False)
    c.check("symbol_locations keeps bytes per symbol", oc.symbol_locations(ours)["gTable"], (".init", 0, 8, b"\xAA" * 8))


def _calls(rows, extra_syms=()):
    b = ElfBuilder().section(".text", b"\0" * 0x20).symbol("f", ".text", 0, 0x10, type=FUNC)
    b.symbol("g", ".text", 0x10, 0x10, type=FUNC)
    for name in sorted({r[1] for r in rows} | set(extra_syms)):
        b.symbol(name)
    for off, name, typ, add in rows:
        b.reloc(".text", off, name, typ, add)
    return b.build()


def test_relocs(c):
    base = [(0x4, "callee_a", "R_PPC_REL24", 0), (0x14, "callee_b", "R_PPC_REL24", 0)]
    rows, err = oc.reloc_rows(_calls(base))
    c.check("reloc_rows: (offset, symbol, type, addend) per section", (rows, err),
            ({".text": [(4, "callee_a", 10, 0), (0x14, "callee_b", 10, 0)]}, None))
    c.check("a missing object is (None, why)", oc.reloc_rows("/no/such.o")[0], None)
    c.check("junk is (None, why)", oc.reloc_rows(b"not an elf at all, no")[1], "<bytes> is not an ELF object: not an ELF object")
    d = oc.reloc_classes([(0, "fn_A", 10, 0), (4, "x", 6, 0), (8, "cb", 6, 0), (12, "t", 1, 0)],
                         [(0, "fn_A", 10, 0), (4, "y", 6, 0), (8, "cb", 4, 0), (16, "o", 1, 0)])
    c.check("the four classes", (d["only_target"], d["only_ours"], d["different_symbol"], d["different_attr"]),
            ([(12, "t", 1, 0)], [(16, "o", 1, 0)], [(4, ("y", 6, 0), ("x", 6, 0))], [(8, ("cb", 4, 0), ("cb", 6, 0))]))
    c.check("identical lists are identical", oc.reloc_classes(base, base)["identical"], True)
    c.check("relocs() diffs every relocated section", oc.relocs(_calls(base), _calls(base))[".text"]["identical"], True)
    c.check("by_owner: identical", oc.by_owner(_calls(base), _calls(base)), (2, 2, []))
    wrong = [(0x4, "callee_x", "R_PPC_REL24", 0), base[1]]
    m, _t, lines = oc.by_owner(_calls(base), _calls(wrong))
    c.check("by_owner: a wrong callee is one line naming both", (m, len(lines), "callee_x vs callee_a" in lines[0]),
            (1, 1, True))
    slid = [(0x8, "callee_a", "R_PPC_REL24", 0), base[1]]
    m, _t, lines = oc.by_owner(_calls(base), _calls(slid))
    c.check("by_owner: a slid instruction is a non-failing note", (m, [ln.split()[0] for ln in lines]), (2, ["note"]))
    c.check("legacy names: six kinds named, others numeric", (oc.legacy_reloc_name(10), oc.legacy_reloc_name(11),
                                                              oc.legacy_reloc_name(3, "type-%d")),
            ("R_PPC_REL24", "R_PPC_11", "type-3"))


def test_reloc_facts(c):
    b = (ElfBuilder().section(".text", b"\0" * 8).section("extabindex", b"\0" * 12)
         .symbol("own", ".text", 0, 4, type=FUNC).symbol("called").symbol("covered")
         .reloc(".text", 0, "called", "R_PPC_REL24").reloc("extabindex", 0, "covered", "R_PPC_ADDR32"))
    facts = oc.reloc_facts(b.build())
    c.check("refs skip bookkeeping relocations", facts["refs"], {"called"})
    c.check("defined keeps (section, st_info)", facts["defined"], {"own": (".text", 0x12)})
    c.check("every relocation is listed with its target section", [(r["target"], r["symbol"]) for r in facts["relocs"]],
            [(".text", "called"), ("extabindex", "covered")])
    c.check("BOTH same-named relocation sections are read (the linker applies both)",
            oc.reloc_facts(twin_data())["refs"], {"first_ref", "second_ref"})
    c.check("an unreadable object is None", oc.reloc_facts(b"junk"), None)
    c.check("provides_global: global/weak yes, local no",
            [oc.provides_global(e) for e in ((".t", 0x12), (".t", 0x22), (".t", 0x02), None)],
            [True, True, False, False])


def test_undefined(c):
    ours = oc.reloc_facts(_calls([(0, "gone", 10, 0), (4, "mapped", 10, 0), (8, "elsewhere", 10, 0),
                                  (12, "relay", 10, 0), (16, "already", 10, 0), (20, "spelled", 10, 0),
                                  (24, "_stack_addr", 10, 0), (28, "f", 10, 0)]))
    target = oc.reloc_facts(_calls([(0, "other", 10, 0), (12, "relay", 10, 0), (20, "spelled__Fv", 10, 0)]))
    hits = oc.undefined(ours, target, map_set={"mapped"}, providers={"elsewhere": ["other.o"], "gone": ["T.o"]},
                        ref_count={"already": 1, "relay": 1}, target_rel="T.o", linker_set={"_stack_addr"})
    c.check("only the target providing it is a hit; defined here, mapped, provided elsewhere, the target's own "
            "reference, already referenced, linker-assigned: silent",
            [h[0] for h in hits], ["gone", "spelled"])
    c.check("the spelling hint: the target's name at the same relocation slot", dict(hits)["spelled"],
            ("spelled__Fv", "same offset"))
    stem = oc.spelling_hint(".text", 0x99, "spelled", target)
    c.check("... or the one name with the same linkage stem when the layouts differ", stem, ("spelled__Fv", "same stem"))
    c.check("external_candidates: first occurrence, not defined, not known",
            [n for _s, _o, n in oc.external_candidates(ours, {"mapped"})][:2], ["gone", "elsewhere"])


def test_link_index(c):
    with testing.FixtureTree() as tree:
        root = str(tree.root)
        tree.write("build.ninja", "rule link\n  command = x\n\nbuild build/RMHE08/main.elf: link a.o $\n"
                                  "    b.o missing.o | build/RMHE08/ldscript.lcf || post\n")
        c.check("link_inputs reads the edge and stops at the implicit deps",
                oc.link_inputs(os.path.join(root, "build.ninja")), ["a.o", "b.o", "missing.o"])
        c.check("no build file is None", oc.link_inputs(os.path.join(root, "nope.ninja")), None)
        tree.write("a.o", ElfBuilder().section(".text", b"\0" * 4).symbol("prov", ".text", 0, 4, type=FUNC)
                   .symbol("loc", ".text", 0, 4, bind="local").symbol("need").reloc(".text", 0, "need", 10).build())
        tree.write("b.o", ElfBuilder().section(".text", b"\0" * 4).symbol("need").reloc(".text", 0, "need", 10).build())
        index = oc.link_index(root)
        c.check("providers are global/weak definitions", index["providers"], {"prov": ["a.o"]})
        c.check("ref_count counts referencing inputs; refs per input (a missing one is empty)",
                (index["ref_count"], index["refs"]), ({"need": 2}, {"a.o": {"need"}, "b.o": {"need"}, "missing.o": set()}))
        cache = json.loads(tree.path(oc.LINK_INDEX_REL).read_text(encoding="utf-8"))
        c.check("the per-input facts are cached", sorted(cache["inputs"]), ["a.o", "b.o"])
        tree.write("x.lcf", "SECTIONS {\n    _stack_addr = 0x80004000;\n}\n")
        c.check("linker_assigned reads the script's assignments", oc.linker_assigned(str(tree.path("x.lcf"))),
                {"_stack_addr"})


def test_linkage(c):
    obj = (ElfBuilder().section(".text", b"\0" * 4).file_symbol("u.c").symbol("@176", ".text", bind="local")
           .symbol("main_fn", ".text", 0, 4, type=FUNC).symbol("drawSpr2TF").build())
    c.check("linkage_sets: global definitions and references; file, @labels and locals dropped",
            oc.linkage_sets(obj), ({"main_fn"}, {"drawSpr2TF"}))
    r = oc.linkage_audit(set(), {"drawSpr2TF"}, set(), {"drawSpr2TF__FUcP9fltSpr2TFUc"})
    c.check("a same-stem target spelling is a linkage row", r["linkage_undefined"],
            [{"our": "drawSpr2TF", "target": ["drawSpr2TF__FUcP9fltSpr2TFUc"]}])
    c.check("a target definition satisfies a reference", oc.linkage_audit(set(), {"h"}, {"h"}, set())["other_undefined"],
            [])
    c.check("a definition the target defines the same way is clean; a mangled one is a defined linkage row",
            (oc.linkage_audit({"x"}, set(), {"x"}, set())["other_defined"],
             oc.linkage_audit({"T"}, set(), {"T__Fv"}, set())["linkage_defined"]),
            ([], [{"our": "T", "target": ["T__Fv"]}]))


def test_fingerprints(c):
    def obj(name="callee", data=b"\x01\x02\x03\x04", sym="f"):
        return (ElfBuilder().section(".text", data).symbol(sym, ".text", 0, 4, type=FUNC).symbol(name)
                .reloc(".text", 0, name, 10).build())

    with testing.FixtureTree() as tree:
        p = {k: str(tree.write(k + ".o", v)) for k, v in
             {"a": obj(), "renamed": obj(sym="g"), "bytes": obj(data=b"\x09\x02\x03\x04")}.items()}
        p["raw"] = str(tree.write("raw.o", b"not an elf"))
        c.check("fingerprint: a rename is not drift", oc.fingerprint(p["a"]) == oc.fingerprint(p["renamed"]), True)
        c.check("fingerprint: a byte change is drift", oc.fingerprint(p["a"]) == oc.fingerprint(p["bytes"]), False)
        c.check("fingerprint: a non-ELF file falls back to its raw hash", oc.fingerprint(p["raw"])[:4], "raw:")
        syms = {"callee": {"section": ".text", "address": 0x100}, "callee2": {"section": ".text", "address": 0x100}}
        p["sweep"] = str(tree.write("sweep.o", obj(name="callee2")))
        a, b = oc.touch_fingerprint(p["a"], syms), oc.touch_fingerprint(p["sweep"], syms)
        c.check("touch: a rename sweep to the same address is the same object", oc.fingerprints_equal(a, b), True)
        moved = oc.touch_fingerprint(p["sweep"], {"callee2": {"section": ".text", "address": 0x200}})
        c.check("touch: an external target at another address is a change",
                oc.fingerprints_equal(oc.touch_fingerprint(p["a"], syms), moved), False)
        c.check("touch: a byte change is a change", oc.fingerprints_equal(a, oc.touch_fingerprint(p["bytes"], syms)),
                False)
        c.check("touch: unreadable is None", oc.touch_fingerprint(p["raw"]), None)


def _dtk_bss(sym="s", data=b"\x01\x02\x03\x04"):
    """An object with a `.bss` whose header offset is 0, as dtk writes it: its file slice is the ELF header, so
    a longer symbol name (a longer `.strtab`) moves the section-table offset inside that slice."""
    from tools.lib.binary.elf import Elf
    blob = bytearray(ElfBuilder().section(".data", data).nobits(".bss", 0x40)
                     .symbol(sym, ".data", 0, 4, type="object").symbol("b_" + sym, ".bss", 0, 4, type="object")
                     .build())
    bss = next(s for s in Elf.read(bytes(blob)).sections if s.name == ".bss")
    blob[bss.header + 16:bss.header + 20] = b"\0\0\0\0"              # sh_offset = 0
    return bytes(blob)


def test_fingerprint_nobits(c):
    with testing.FixtureTree() as tree:
        a = str(tree.write("a.o", _dtk_bss()))
        renamed = str(tree.write("r.o", _dtk_bss(sym="a_much_longer_symbol_name_that_grows_strtab")))
        edited = str(tree.write("d.o", _dtk_bss(data=b"\x09\x02\x03\x04")))
        c.check("fixture: the rename really moved the bytes a header-offset-0 .bss slices",
                open(a, "rb").read()[:0x40] != open(renamed, "rb").read()[:0x40], True)
        c.check("fingerprint: a .bss at offset 0 plus a .strtab length change is not drift",
                oc.fingerprint(a) == oc.fingerprint(renamed), True)
        c.check("fingerprint: a real .data change in the same object is drift",
                oc.fingerprint(a) == oc.fingerprint(edited), False)
        c.check("touch: the same rename keeps the touch body",
                oc.touch_fingerprint(a)["body"] == oc.touch_fingerprint(renamed)["body"], True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
