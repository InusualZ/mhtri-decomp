"""relocdiff --callees (`lib.objcompare.callee_diffs`) on ElfBuilder objects: a wrong callee, a mangling, a C linkage,
a moved relocation cancelled by name, pool/jump-table labels dropped, a stub and a function only one side defines skipped."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import names, objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import relocdiff

TIER = "fixture"


def obj(calls: dict, sizes: dict | None = None):
    """An object whose `.text` defines each function of `calls` (in order, 0x40 bytes unless `sizes` says) with its
    relocations `[(offset, symbol, type)]` (undefined symbols declared once)."""
    sizes = sizes or {}
    b, at, seen = ElfBuilder(), 0, set()
    total = sum(sizes.get(f, 0x40) for f in calls)
    b.section(".text", b"\0" * total).section(".sdata2", b"\0" * 8).section(".data", b"\0" * 8)
    for fn, rels in calls.items():
        size = sizes.get(fn, 0x40)
        b.symbol(fn, ".text", at, size, type="func")
        for off, sym, typ in rels:
            if sym not in seen and sym not in calls:
                seen.add(sym)
                if sym.startswith("@"):
                    b.symbol(sym, ".sdata2", 0, 4, bind="local")
                elif sym.startswith(("lbl_", "jumptable_")):
                    b.symbol(sym, ".data", 0, 4)
                else:
                    b.symbol(sym)
            b.reloc(".text", at + off, sym, typ)
        at += size
    return b.build()


TARGET = obj({
    "f_wrong": [(0x10, "em_frame_check__FP4WorkUs", "R_PPC_REL24"), (0x20, "other", "R_PPC_REL24")],
    "f_mangle": [(0x08, "setColor__4CharFUlP5Color", "R_PPC_REL24")],
    "f_linkage": [(0x08, "get_work__FUc", "R_PPC_REL24")],
    "f_moved": [(0x08, "a_call", "R_PPC_REL24"), (0x10, "b_call", "R_PPC_REL24")],
    "f_pool": [(0x0a, "lbl_80796EE0", "R_PPC_EMB_SDA21"), (0x12, "jumptable_805A20E0", "R_PPC_ADDR16_HA")],
    "f_stub": [(0x00, "x_call", "R_PPC_REL24")],
    "f_only_target": [(0x00, "y_call", "R_PPC_REL24")],
    "f_clean": [(0x04, "z_call", "R_PPC_REL24")],
})
OURS = obj({
    "f_wrong": [(0x10, "em_after_frame_check__FP4WorkUs", "R_PPC_REL24"), (0x20, "other", "R_PPC_REL24")],
    "f_mangle": [(0x08, "setColor__4CharFUl5Color", "R_PPC_REL24")],
    "f_linkage": [(0x08, "get_work", "R_PPC_REL24")],
    "f_moved": [(0x0c, "b_call", "R_PPC_REL24"), (0x14, "a_call", "R_PPC_REL24")],
    "f_pool": [(0x0a, "@2033", "R_PPC_EMB_SDA21"), (0x12, "@1502", "R_PPC_ADDR16_HA")],
    "f_stub": [],
    "f_clean": [(0x08, "z_call", "R_PPC_REL24")],
}, sizes={"f_stub": 4})


def test_callee_diffs(c):
    got = {f["function"]: [(d["kind"], d["ours"], d["target"], d["offset"]) for d in f["diffs"]]
           for f in objcompare.callee_diffs(TARGET, OURS)}
    c.check("a wrong callee is named with both spellings and the offset in the function", got.get("f_wrong"),
            [("callee", "em_after_frame_check__FP4WorkUs", "em_frame_check__FP4WorkUs", 0x10)])
    c.check("the same owner stem with two argument lists is a mangling", got.get("f_mangle"),
            [("mangling", "setColor__4CharFUl5Color", "setColor__4CharFUlP5Color", 0x08)])
    c.check("the same stem once in C is a linkage difference", got.get("f_linkage"),
            [("linkage", "get_work", "get_work__FUc", 0x08)])
    c.check("relocations that only moved cancel by name (and a shifted clean call is no difference)",
            ("f_moved" in got, "f_clean" in got), (False, False))
    c.check("our @N pool/jump-table labels cancel the target's lbl_/jumptable_ of the same type", "f_pool" in got, False)
    c.check("a stub (<= 8 bytes) and a function ours does not define are not judged",
            ("f_stub" in got, "f_only_target" in got), (False, False))
    c.check("exactly the three real differences", sorted(got), ["f_linkage", "f_mangle", "f_wrong"])
    c.check("owner_stem keeps the class qualifier", names.owner_stem("setColor__4CharFUlP5Color"), "setColor__4Char")


def test_unpaired(c):
    t = obj({"f": [(0x08, "lbl_80592F68", "R_PPC_ADDR16_HA"), (0x10, "extra_t", "R_PPC_REL24")]})
    o = obj({"f": [(0x08, "lbl_80592EF0", "R_PPC_ADDR16_HA")]})
    got = [(d["kind"], d["ours"], d["target"]) for d in objcompare.callee_diffs(t, o)[0]["diffs"]]
    c.check("two named data labels are a difference (not pool noise); a leftover is `missing`",
            got, [("callee", "lbl_80592EF0", "lbl_80592F68"), ("missing", None, "extra_t")])
    t2 = obj({"f": [(0x08, "lbl_80796F00", "R_PPC_EMB_SDA21")]})
    o2 = obj({"f": [(0x08, "@12", "R_PPC_ADDR16_LO")]})
    c.check("a pool label of another relocation type does not cancel", len(objcompare.callee_diffs(t2, o2)), 1)


def test_cli(c):
    with testing.FixtureTree() as tree:
        tree.write("build/RMHE08/obj/mod/u.o", TARGET)
        tree.write("build/RMHE08/src/mod/u.o", OURS)
        tree.write("src/mod/u.c", "int x;\n")
        saved = relocdiff._repo.repo_root
        relocdiff._repo.repo_root = lambda *a, **k: str(tree.root)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                rc = relocdiff.main(["--callees", "mod/u", "--json", "-"])
            text = out.getvalue()
        finally:
            relocdiff._repo.repo_root = saved
        lines = [ln for ln in text.splitlines() if ln.startswith("mod/u  ")]
        c.check("one line per function with a difference, exit 1", (rc, len(lines)), (1, 3))
        c.contains("the line names kind, both spellings and the offset", text,
                   "mod/u  f_wrong  callee: ours em_after_frame_check__FP4WorkUs vs target em_frame_check__FP4WorkUs @+0x10")
        payload = json.loads(text[text.index("["):])
        c.check("--json - prints the record", [f["function"] for f in payload[0]["functions"]],
                ["f_wrong", "f_mangle", "f_linkage"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
