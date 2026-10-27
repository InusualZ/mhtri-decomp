"""relocdiff --values (`lib.objcompare.value_diffs`) on ElfBuilder objects: a relocation pair whose two targets differ
only by name (`@N` vs `lbl_X`) is read on both sides - the object's own section bytes, or the DOL at the map's address
for an undefined name - and reported when the BYTES differ, which no instruction score can see. Also: the access width
and the datum size, masked relocated words, a diverged instruction, a stub, and flipcheck's advisory row."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import struct

from tools.lib import objcompare, testing
from tools.lib.binary.build import DolBuilder, ElfBuilder
from tools.lib.binary.dol import Dol
from tools.objdiff import relocdiff
from tools.units import flipcheck

TIER = "fixture"

LFS = 0xC0220000      # lfs f1, 0(r2)    - a 4-byte FP access (shown as a number)
LFD = 0xC8220000      # lfd f1, 0(r2)    - an 8-byte FP access
LWZ = 0x80620000      # lwz r3, 0(r2)    - a 4-byte integer access
ADDI = 0x38630000     # addi r3, r3, 0   - an address taken: the datum's size is read
BLR = 0x4E800020
NOP = struct.pack(">I", 0x60000000)
MIN_BODY = 16


def f32(x: float) -> bytes:
    return struct.pack(">f", x)


def obj(words: list[int], pool: bytes, labels: list, relocs: list, func_size: int | None = None,
        data: bytes | None = None, data_relocs: list = ()):
    """One function `f` whose `.text` is `words`; `.sdata2` holds `pool` with `labels` `[(name, offset, size)]`
    (`size` None: an undefined name); `.text` is padded with nops to `MIN_BODY` bytes; `relocs` are `[(text offset, name, type)]`. `data`/`data_relocs` add a `.data`
    section with relocations `[(offset, name)]` (ADDR32)."""
    text = b"".join(struct.pack(">I", w) for w in words)
    text += NOP * max(0, (MIN_BODY - len(text)) // 4)     # longer than a stub (`objcompare.STUB_MAX_BYTES`)
    b = ElfBuilder().section(".text", text).section(".sdata2", pool)
    if data is not None:
        b.section(".data", data)
    b.symbol("f", ".text", 0, func_size or len(text), type="func")
    for name, off, size in labels:
        if size is None:
            b.symbol(name)
        else:
            sec = ".data" if name.startswith("tbl") else ".sdata2"
            b.symbol(name, sec, off, size, bind="local" if name.startswith("@") else "global", type="object")
    for off, name, typ in relocs:
        b.reloc(".text", off, name, typ)
    for off, name in data_relocs:
        b.reloc(".data", off, name, "R_PPC_ADDR32")
    return b.build()


def pool_pair(ours_word: bytes, target_word: bytes, insn: int = LFS, typ: str = "R_PPC_EMB_SDA21"):
    """`(target, ours)`: one access at +0x0 of `insn`, ours naming `@12`, the target `lbl_80796EE0`."""
    target = obj([insn, BLR], target_word, [("lbl_80796EE0", 0, len(target_word))], [(0x0, "lbl_80796EE0", typ)])
    ours = obj([insn, BLR], ours_word, [("@12", 0, len(ours_word))], [(0x0, "@12", typ)])
    return target, ours


def test_value_width(c):
    c.check("lfs reads 4 bytes shown as a single", objcompare.value_width(LFS, [0]), (4, "f32"))
    c.check("lfd reads 8 bytes shown as a double", objcompare.value_width(LFD, [0]), (8, "f64"))
    c.check("lwz reads 4 bytes as an integer", objcompare.value_width(LWZ, [64]), (4, "int"))
    c.check("addi reads the smaller stated datum size", objcompare.value_width(ADDI, [16, 12]), (12, "bytes"))
    c.check("addi with no stated size reads the default word", objcompare.value_width(ADDI, [0, 0]),
            (objcompare.VALUE_DEFAULT_WIDTH, "bytes"))
    c.check("a huge datum is capped", objcompare.value_width(None, [4096])[0], objcompare.VALUE_MAX_WIDTH)


def test_pool_word(c):
    target, ours = pool_pair(f32(500.0), f32(500.0))
    rec = objcompare.value_diffs(target, ours)
    c.check("the same pool value under two names is no difference (1 pair read, equal)",
            (rec["pairs"], rec["same"], rec["differ"]), (1, 1, []))
    target, ours = pool_pair(f32(0.7), f32(500.0))
    rec = objcompare.value_diffs(target, ours)
    d = rec["differ"][0] if rec["differ"] else {}
    c.check("another pool value is reported with both names, values and the offset",
            (d.get("ours"), d.get("target"), d.get("offset"), d.get("type"), d.get("kind")),
            ("@12", "lbl_80796EE0", 0, "R_PPC_EMB_SDA21", "f32"))
    c.contains("the value is spelled as a number and its hex", d.get("ours_value", ""), "0.699999988079071 (0x3f333333)")
    c.contains("the target's value is spelled too", d.get("target_value", ""), "500.0 (0x43fa0000)")
    c.check("the bodies are byte-identical apart from the relocation (the case no score sees)",
            d.get("owner_equal"), True)
    target, ours = pool_pair(f32(1.0) + f32(0.0), f32(1.0) + f32(2.0), insn=LFD)
    c.check("an lfd compares all 8 bytes", len(objcompare.value_diffs(target, ours)["differ"]), 1)
    target, ours = pool_pair(f32(0.7), f32(500.0), typ="R_PPC_ADDR16_HA")
    c.check("an @ha (the high half) is not read: its @l partner is", objcompare.value_diffs(target, ours)["pairs"], 0)


def test_same_name_and_instruction(c):
    target = obj([LFS, BLR], f32(1.0), [("kOne", 0, 4)], [(0x0, "kOne", "R_PPC_EMB_SDA21")])
    ours = obj([LFS, BLR], f32(2.0), [("kOne", 0, 4)], [(0x0, "kOne", "R_PPC_EMB_SDA21")])
    c.check("a pair under one name is not this question (the section bytes are)",
            objcompare.value_diffs(target, ours)["pairs"], 0)
    target = obj([LFS, BLR], f32(1.0), [("lbl_80796EE0", 0, 4)], [(0x0, "lbl_80796EE0", "R_PPC_EMB_SDA21")])
    ours = obj([0xC0420000, BLR], f32(2.0), [("@12", 0, 4)], [(0x0, "@12", "R_PPC_EMB_SDA21")])
    rec = objcompare.value_diffs(target, ours)
    c.check("another instruction at the offset (lfs f2 vs f1) is a diverged body, not a value: skipped",
            (rec["differ"], rec["skipped"]), ([], {"other-instruction": 1}))
    target = obj([0, LFS, BLR], f32(1.0), [("lbl_80796EE0", 0, 4)], [(0x4, "lbl_80796EE0", "R_PPC_EMB_SDA21")])
    ours = obj([LFS, 0, BLR], f32(2.0), [("@12", 0, 4)], [(0x0, "@12", "R_PPC_EMB_SDA21")])
    c.check("a relocation at another offset does not pair", objcompare.value_diffs(target, ours)["pairs"], 0)


def test_stub_and_bss(c):
    target = obj([LFS, BLR], f32(1.0), [("lbl_80796EE0", 0, 4)], [(0x0, "lbl_80796EE0", "R_PPC_EMB_SDA21")])
    ours = obj([LFS, BLR], f32(2.0), [("@12", 0, 4)], [(0x0, "@12", "R_PPC_EMB_SDA21")], func_size=8)
    c.check("a stub ours has not written (<= 8 bytes) is not judged", objcompare.value_diffs(target, ours)["pairs"],
            0)
    b = ElfBuilder().section(".text", struct.pack(">III", LWZ, BLR, BLR)).nobits(".sbss", 8)
    b.symbol("f", ".text", 0, 12, type="func").symbol("@3", ".sbss", 0, 4, bind="local", type="object")
    b.reloc(".text", 0, "@3", "R_PPC_EMB_SDA21")
    ours = b.build()
    b = ElfBuilder().section(".text", struct.pack(">III", LWZ, BLR, BLR)).nobits(".sbss", 8)
    b.symbol("f", ".text", 0, 12, type="func").symbol("lbl_80800000", ".sbss", 4, 4, type="object")
    b.reloc(".text", 0, "lbl_80800000", "R_PPC_EMB_SDA21")
    c.check("an uninitialised target has no value: counted as bss",
            objcompare.value_diffs(b.build(), ours)["skipped"], {"bss": 1})


def test_undefined_through_the_dol(c):
    """ours loads another unit's pool word (an undefined `lbl_`), the target its own: the DOL holds ours'."""
    dol = Dol.read(DolBuilder().text(0x80004000, b"\0" * 0x20).data(0x80796E00, f32(0.0) * 4 + f32(1200.0)).build())
    addresses = {"lbl_80796E10": 0x80796E10, "lbl_80796E00": 0x80796E00}
    target = obj([LFS, BLR], f32(1200.0), [("lbl_80796EE0", 0, 4)], [(0x0, "lbl_80796EE0", "R_PPC_EMB_SDA21")])
    ours = obj([LFS, BLR], b"", [("lbl_80796E00", 0, None)], [(0x0, "lbl_80796E00", "R_PPC_EMB_SDA21")])
    rec = objcompare.value_diffs(target, ours, dol, addresses)
    c.check("an undefined name is read from the DOL at the map's address: 0.0 vs 1200.0 is reported",
            [(d["ours"], d["ours_value"].split()[0], d["target_value"].split()[0]) for d in rec["differ"]],
            [("lbl_80796E00", "0.0", "1200.0")])
    ours = obj([LFS, BLR], b"", [("lbl_80796E10", 0, None)], [(0x0, "lbl_80796E10", "R_PPC_EMB_SDA21")])
    c.check("the right word under another unit's name is equal", objcompare.value_diffs(target, ours, dol,
                                                                                         addresses)["differ"], [])
    c.check("without the DOL (or the map row) it is counted unresolved, never a false equal",
            objcompare.value_diffs(target, ours)["skipped"], {"unresolved": 1})


def test_address_and_relocated(c):
    target = obj([ADDI, BLR], b"g3d_resmat_ac.h\0", [("lbl_805911C8", 0, 16)],
                 [(0x2, "lbl_805911C8", "R_PPC_ADDR16_LO")])
    ours = obj([ADDI, BLR], b"g3d_resmat.cpp\0\0", [("@1100", 0, 15)], [(0x2, "@1100", "R_PPC_ADDR16_LO")])
    rec = objcompare.value_diffs(target, ours)
    d = rec["differ"][0] if rec["differ"] else {}
    c.check("an address-taken string reads the smaller datum and is quoted, with the first differing byte",
            (d.get("ours_value"), d.get("target_value"), d.get("kind"), d.get("first_difference")),
            ('"g3d_resmat.cpp"', '"g3d_resmat_ac.h"', "bytes", 0xA))
    table_t = struct.pack(">II", 0x80284100, 0x80284108)
    target = obj([ADDI, BLR], b"", [("tbl_t", 0, 8)], [(0x2, "tbl_t", "R_PPC_ADDR16_LO")], data=table_t,
                 data_relocs=[(0, "f"), (4, "f")])
    ours = obj([ADDI, BLR], b"", [("tbl_o", 0, 8)], [(0x2, "tbl_o", "R_PPC_ADDR16_LO")], data=bytes(8),
               data_relocs=[(0, "f"), (4, "f")])
    rec = objcompare.value_diffs(target, ours)
    c.check("a jump table made of relocated words (zero in ours, linked addresses in the split object) is skipped",
            (rec["differ"], rec["skipped"]), ([], {"relocated": 1}))


def test_cli_and_flipcheck(c):
    target, ours = pool_pair(f32(0.7), f32(500.0))
    with testing.FixtureTree() as tree:
        tree.write("build/RMHE08/obj/mod/u.o", target)
        tree.write("build/RMHE08/src/mod/u.o", ours)
        tree.write("src/mod/u.c", "int x;\n")
        saved = relocdiff._repo.repo_root
        relocdiff._repo.repo_root = lambda *a, **k: str(tree.root)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                rc = relocdiff.main(["--values", "mod/u", "--json", "-"])
            text = out.getvalue()
            same_t, same_o = pool_pair(f32(1.0), f32(1.0))
            tree.write("build/RMHE08/obj/mod/u.o", same_t)
            tree.write("build/RMHE08/src/mod/u.o", same_o)
            with contextlib.redirect_stdout(io.StringIO()):
                rc_clean = relocdiff.main(["--values", "mod/u"])
        finally:
            relocdiff._repo.repo_root = saved
        c.check("exit 1 on a value mismatch, 0 when the values agree", (rc, rc_clean), (1, 0))
        c.contains("one line names unit, owner+offset, type, both names and values", text,
                   "mod/u  f+0x0  R_PPC_EMB_SDA21  ours @12 = 0.699999988079071 (0x3f333333) vs target "
                   "lbl_80796EE0 = 500.0 (0x43fa0000)  [bytes equal]")
        c.contains("the summary line counts mismatches and pairs read", text,
                   "relocdiff --values: 1 value mismatch(es) in 1 of 1 unit(s); 1 renamed pair(s) read, 0 equal")
        payload = json.loads(text[text.index("\n[") + 1:])
        c.check("--json - prints the record", [d["owner"] for d in payload[0]["differ"]], ["f"])
    with testing.FixtureTree() as tree:
        src = tree.write("build/RMHE08/src/mod/u.o", ours)
        obj_path = tree.write("build/RMHE08/obj/mod/u.o", target)
        flipcheck._MAP_ADDRESSES.clear()
        flipcheck.set_root(str(tree.root))
        try:
            notes = flipcheck.relocation_value_notes(str(src), str(obj_path))
        finally:
            flipcheck.set_root(str(pathlib.Path(flipcheck.__file__).resolve().parents[2]))
            flipcheck._MAP_ADDRESSES.clear()
        c.check("flipcheck's row is one advisory note (never a problem)",
                [n.startswith(flipcheck.ADVISORY) for n in notes], [True])
        c.contains("the note names the owner, offset and both values", notes[0] if notes else "",
                   "f+0x0 ours @12 = 0.699999988079071 (0x3f333333), target lbl_80796EE0 = 500.0 (0x43fa0000)")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
