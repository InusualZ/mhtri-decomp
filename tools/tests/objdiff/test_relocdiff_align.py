"""relocdiff --values' default aligned pairing (`lib.objcompare.aligned_value_diffs`) on ElfBuilder objects: a code
shift that moves every constant is no value mismatch (offset pairing reads one per moved load), a wrong value is still
found shifted, in place, missing or extra, a rotation in unshifted code stays paired by place; also the CLI and flipcheck."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import struct

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import relocdiff
from tools.units import flipcheck

TIER = "fixture"

LFS = 0xC0220000      # lfs f1, 0(r2)    - a 4-byte FP access
ADDI = 0x38630000     # addi r3, r3, 0   - an address taken
NOP = 0x60000000
BLR = 0x4E800020


def func(loads: list, side: str, insn: int = LFS, typ: str = "R_PPC_EMB_SDA21", names: list | None = None,
         pool: bytes | None = None, sizes: list | None = None):
    """One function `f` (nops) with `insn` at each `(offset, value)` of `loads`, relocated to its own `.sdata2` word:
    the target's labels are `lbl_8079xxxx`, ours `@N` (or `names`); `pool`/`sizes` replace the f32 words."""
    end = max(off for off, _v in loads) + 8
    words = [NOP] * (end // 4)
    for off, _v in loads:
        words[off // 4] = insn
    words[-1] = BLR
    data = pool if pool is not None else b"".join(struct.pack(">f", v) for _off, v in loads)
    b = ElfBuilder().section(".text", b"".join(struct.pack(">I", w) for w in words)).section(".sdata2", data)
    b.symbol("f", ".text", 0, end, type="func")
    at = 0
    for i, (off, _v) in enumerate(loads):
        size = sizes[i] if sizes else 4
        name = names[i] if names else ("lbl_%08X" % (0x80796E00 + 4 * i) if side == "target" else "@%d" % (100 + i))
        if name not in (names or [])[:i]:
            b.symbol(name, ".sdata2", at, size, bind="local" if name.startswith("@") else "global", type="object")
        b.reloc(".text", off + (2 if typ == "R_PPC_ADDR16_LO" else 0), name, typ)
        at += size
    return b.build()


def pair(target_loads, ours_loads, **kw):
    return func(target_loads, "target", **kw), func(ours_loads, "ours", **kw)


SHIFT_T = [(0x10, 1.0), (0x14, 2.5), (0x18, 3.0)]
SHIFT_O = [(0x14, 1.0), (0x18, 2.5), (0x1C, 3.0)]          # one instruction inserted before the loads


def test_shifted_code(c):
    target, ours = pair(SHIFT_T, SHIFT_O)
    c.check("offset pairing reads the shift as two wrong values (the defect the alignment removes)",
            len(objcompare.value_diffs(target, ours, mode="offset")["differ"]), 2)
    rec = objcompare.value_diffs(target, ours)
    c.check("aligned (the default): the same values in the same order are no mismatch, all three read equal",
            (rec["differ"], rec["pairs"], rec["same"]), ([], 3, 3))
    saved = objcompare.ALIGN_MAX_CELLS
    objcompare.ALIGN_MAX_CELLS = 0
    try:
        rec = objcompare.value_diffs(target, ours)
    finally:
        objcompare.ALIGN_MAX_CELLS = saved
    c.check("past the matrix cap the difflib fallback aligns the shift too", rec["differ"], [])


def test_shifted_wrong_value(c):
    target, ours = pair(SHIFT_T, [(0x14, 1.0), (0x18, 2.0), (0x1C, 3.0)])
    rec = objcompare.value_diffs(target, ours)
    c.check("a wrong value inside a shift is one `different` row with both offsets and values",
            [(d["class"], d["target_offset"], d["ours_offset"], d["target_value"].split()[0],
              d["ours_value"].split()[0]) for d in rec["differ"]],
            [("different", 0x14, 0x18, "2.5", "2.0")])


def test_in_place(c):
    target, ours = pair([(0x10, 1.0), (0x14, 50.0), (0x18, 2.0)], [(0x10, 1.0), (0x14, 30.0), (0x18, 2.0)])
    rec = objcompare.value_diffs(target, ours)
    d = rec["differ"][0] if rec["differ"] else {}
    c.check("one wrong value at one place in a byte-identical body is `wrong`, `owner_equal` (the strongest class)",
            (len(rec["differ"]), d.get("class"), d.get("offset"), d.get("owner_equal")), (1, "wrong", 0x14, True))
    rot_t = [(0x10, 1.0), (0x14, 2.0), (0x18, 3.0), (0x1C, 4.0)]
    rot_o = [(0x10, 2.0), (0x14, 3.0), (0x18, 4.0), (0x1C, 1.0)]
    rec = objcompare.value_diffs(*pair(rot_t, rot_o))
    c.check("a rotation of constants in unshifted code stays paired by place: four `wrong` rows",
            [(d["class"], d["offset"]) for d in rec["differ"]],
            [("wrong", 0x10), ("wrong", 0x14), ("wrong", 0x18), ("wrong", 0x1C)])


def test_missing_extra_reordered(c):
    rec = objcompare.value_diffs(*pair([(0x10, 1.0), (0x14, 7.0), (0x18, 2.0)], [(0x14, 1.0), (0x18, 2.0)]))
    c.check("a value ours never loads is `missing`, ours side none",
            [(d["class"], d["target_offset"], d["ours"]) for d in rec["differ"]], [("missing", 0x14, None)])
    rec = objcompare.value_diffs(*pair([(0x10, 1.0), (0x14, 2.0)], [(0x14, 1.0), (0x18, 9.0), (0x1C, 2.0)]))
    c.check("a value retail never loads is `extra`, target side none",
            [(d["class"], d["ours_offset"], d["target"]) for d in rec["differ"]], [("extra", 0x18, None)])
    rec = objcompare.value_diffs(*pair([(0x10, 1.0), (0x14, 2.0)], [(0x18, 2.0), (0x1C, 1.0)]))
    c.check("two loads swapped across a shift are `reordered` (one row per side): listed, counted, not reported",
            (rec["differ"], rec["skipped"], len(rec["reordered"])), ([], {"reordered": 2}, 2))


def test_names_and_prefix(c):
    target = func([(0x10, 1.0)], "target", names=["kScale"])
    ours = func([(0x14, 2.0)], "ours", insn=ADDI, typ="R_PPC_ADDR16_LO", names=["kScale"])
    c.check("one symbol and addend reached by @sda21 and by lis/addi is one datum (placement, not value)",
            objcompare.value_diffs(target, ours)["differ"], [])
    text = b"abc\0def\0" + bytes(8)
    target = func([(0x10, 0)], "target", insn=ADDI, typ="R_PPC_ADDR16_LO", pool=text, sizes=[16])
    ours = func([(0x10, 0)], "ours", insn=ADDI, typ="R_PPC_ADDR16_LO", pool=b"abc\0", sizes=[4])
    c.check("an address-taken datum compares as far as the shorter symbol runs (a split label spanning more is no "
            "difference)", objcompare.value_diffs(target, ours)["differ"], [])
    ours = func([(0x10, 0)], "ours", insn=ADDI, typ="R_PPC_ADDR16_LO", pool=b"abd\0", sizes=[4])
    rec = objcompare.value_diffs(target, ours)
    c.check("... and a differing byte inside it is reported with its position",
            [(d["class"], d["first_difference"]) for d in rec["differ"]], [("wrong", 2)])


def test_cli_and_flipcheck(c):
    shifted_t, shifted_o = pair(SHIFT_T, SHIFT_O)
    wrong_t, wrong_o = pair(SHIFT_T, [(0x14, 1.0), (0x18, 2.0), (0x1C, 3.0)])
    with testing.FixtureTree() as tree:
        tree.write("src/mod/u.c", "int x;\n")
        saved = relocdiff._repo.repo_root
        relocdiff._repo.repo_root = lambda *a, **k: str(tree.root)
        try:
            runs = {}
            for label, (t, o), argv in (("default", (shifted_t, shifted_o), ["--values", "mod/u"]),
                                        ("offset", (shifted_t, shifted_o), ["--values", "--pairing", "offset", "mod/u"]),
                                        ("wrong", (wrong_t, wrong_o), ["--values", "mod/u"])):
                tree.write("build/RMHE08/obj/mod/u.o", t)
                tree.write("build/RMHE08/src/mod/u.o", o)
                out = io.StringIO()
                with contextlib.redirect_stdout(out):
                    rc = relocdiff.main(argv)
                runs[label] = (rc, out.getvalue())
        finally:
            relocdiff._repo.repo_root = saved
    c.check("the shift exits 0 aligned and 1 under --pairing offset; a shifted wrong value exits 1",
            (runs["default"][0], runs["offset"][0], runs["wrong"][0]), (0, 1, 1))
    c.contains("the summary names the pairing", runs["default"][1], "pairing align")
    c.contains("a shifted row prints both offsets and its tag", runs["wrong"][1],
               "mod/u  f+0x14 (ours +0x18)  R_PPC_EMB_SDA21  ours @101 = 2.0 (0x40000000) vs target lbl_80796E04 = 2.5 "
               "(0x40200000)  [shifted]")
    c.contains("the summary counts the classes", runs["wrong"][1], "pairing align (1 different)")
    with testing.FixtureTree() as tree:
        notes = {}
        flipcheck._MAP_ADDRESSES.clear()
        flipcheck.set_root(str(tree.root))
        try:
            for label, (t, o) in (("shifted", (shifted_t, shifted_o)), ("wrong", (wrong_t, wrong_o))):
                src = tree.write("build/RMHE08/src/mod/%s.o" % label, o)
                obj_path = tree.write("build/RMHE08/obj/mod/%s.o" % label, t)
                notes[label] = flipcheck.relocation_value_notes(str(src), str(obj_path))
        finally:
            flipcheck.set_root(str(pathlib.Path(flipcheck.__file__).resolve().parents[2]))
            flipcheck._MAP_ADDRESSES.clear()
    c.check("flipcheck's advisory row aligns too: no note for a pure shift", notes["shifted"], [])
    c.contains("... and one note naming both offsets for a shifted wrong value", (notes["wrong"] or [""])[0],
               "f+0x14 (ours +0x18) ours @101 = 2.0 (0x40000000), target lbl_80796E04 = 2.5 (0x40200000)")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
