"""undefrefs --census --linkage (relocaudit folded in): the linkage rule, the classifier, the reader, the sweep."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os

from tools.lib import objcompare, testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.binary.elf import SHN_ABS
from tools.units import undefrefs

TIER = "fixture"


def fixture_object(undefined, defined=()) -> bytes:
    """An object carrying a `STT_FILE` row, an `@176` compiler label and a local `...data.0` row (the reader's
    exclusions), `defined` global functions in `.text` and `undefined` global references."""
    b = ElfBuilder().section(".text", b"\x10\x00\x00\x00" * 4)
    b.symbol("unit.c", None, bind="local", type="file", shndx=SHN_ABS)
    b.symbol("@176", ".text", bind="local")
    b.symbol("...data.0", ".text", size=4, bind="local")
    for name in defined:
        b.symbol(name, ".text", size=16, type="func")
    for name in undefined:
        b.symbol(name)
    return b.build()


def test_linkage_rule(c):
    stem = undefrefs.linkage_stem
    c.check("stem strips a C++ argument list", stem("drawSpr2TF__FUcP9fltSpr2TFUc"), "drawSpr2TF")
    c.check("stem strips a class-qualified argument list", stem("Panic__Q24nw4r2dbFPCciPCce"), "Panic")
    c.check("stem strips a long EF spelling", stem("get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3"),
            "get_joint_wpos_em")
    for plain in ("fn_80059550", "lbl_8058B290", "__start", "_savegpr_20", "__init_data", "@176"):
        c.check("not a mangling: %s" % plain, stem(plain), plain)
    for helper in ("_savegpr_20", "_restgpr_17", "_savefpr_31", "_restfpr_14"):
        c.check("compiler helper: %s" % helper, undefrefs.mismatch_kind(helper), "compiler-helper")
    for extra in ("GXSetTexCoordGen2", "fn_80501EE0", "lbl_80629B90"):
        c.check("not a compiler helper: %s" % extra, undefrefs.mismatch_kind(extra), "extra")


def test_classifier(c):
    audit = objcompare.linkage_audit
    r = audit(set(), {"a"}, set(), {"a", "b"})
    c.check("a subset of the target's references is clean", [r[k] for k in undefrefs.LINKAGE_KEYS], [[], [], [], []])
    r = audit(set(), {"drawSpr2TF"}, set(), {"drawSpr2TF__FUcP9fltSpr2TFUc"})
    c.check("plain where the target is mangled is a linkage row", r["linkage_undefined"],
            [{"our": "drawSpr2TF", "target": ["drawSpr2TF__FUcP9fltSpr2TFUc"]}])
    c.check("... and not an other row", r["other_undefined"], [])
    r = audit(set(), {"fn_80043EA8__FP4Vec3"}, set(), {"fn_80043EA8"})
    c.check("mangled where the target is plain is a linkage row", r["linkage_undefined"],
            [{"our": "fn_80043EA8__FP4Vec3", "target": ["fn_80043EA8"]}])
    r = audit(set(), {"helper"}, {"helper"}, set())
    c.check("a target definition satisfies our reference", (r["linkage_undefined"], r["other_undefined"]), ([], []))
    r = audit(set(), {"mystery_fn"}, set(), {"other_fn"})
    c.check("a name the target never mentions is an other row", (r["other_undefined"], r["linkage_undefined"]),
            ([{"our": "mystery_fn", "target": []}], []))
    r = audit({"TPLtexLoad"}, set(), {"TPLtexLoad__FPvP9_tex_info"}, set())
    c.check("a definition with the wrong linkage is a defined linkage row", r["linkage_defined"],
            [{"our": "TPLtexLoad", "target": ["TPLtexLoad__FPvP9_tex_info"]}])
    c.check("a definition the target never names is an other defined row",
            audit({"brand_new"}, set(), set(), set())["other_defined"], [{"our": "brand_new", "target": []}])


def test_reader(c):
    with testing.FixtureTree() as tree:
        p = tree.write("one.o", fixture_object(["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"], ["main_fn"]))
        defined, undefined = objcompare.linkage_sets(str(p))
        c.check("the reader keeps global definitions", sorted(defined), ["main_fn"])
        c.check("the reader reports undefined names and drops STT_FILE, @NNN and locals", sorted(undefined),
                ["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"])
        c.check("None on a missing file", objcompare.linkage_sets(str(tree.path("nope.o"))), None)
        c.check("None on junk", objcompare.linkage_sets(str(tree.write("junk.o", b"not an ELF\n"))), None)
        c.check("None on a truncated object",
                objcompare.linkage_sets(str(tree.write("trunc.o", p.read_bytes()[:60]))), None)


def test_sweep(c):
    with testing.FixtureTree() as tree:
        root = str(tree.root)
        tree.write("configure.py", 'config.libs = [\n    Object(NonMatching, "unit.c"),\n]\n')
        tree.write("build/RMHE08/src/unit.o", fixture_object(["drawSpr2TF", "get_ScreenSize__FP8_MH_VEC2"],
                                                             ["main_fn"]))
        tree.write("build/RMHE08/obj/unit.o", fixture_object(["drawSpr2TF__FUcP9fltSpr2TFUc",
                                                              "get_ScreenSize__FP8_MH_VEC2"], ["main_fn", "helper"]))
        s = undefrefs.linkage_sweep(root, with_decls=False)
        c.check("the sweep sees the one registered unit", s["units_total"], 1)
        rec = s["suspects"][0] if s["suspects"] else {"linkage_undefined": [], "other_undefined": []}
        c.check("the wrong-linkage fixture is the one suspect, as a linkage row",
                (len(s["suspects"]), len(rec["linkage_undefined"]), len(rec["other_undefined"])), (1, 1, 0))
        c.check("the linkage row names both spellings", rec["linkage_undefined"][:1],
                [{"our": "drawSpr2TF", "target": ["drawSpr2TF__FUcP9fltSpr2TFUc"], "kind": "linkage"}])
        c.check("the counts", (s["clean"], s["undefined_suspects"]), (0, 1))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = undefrefs.main(["--census", "--linkage", "--main", root, "--no-decls", "--json"])
        c.check("the CLI runs the linkage census (exit 0, the same record)",
                (rc, json.loads(out.getvalue())["undefined_suspects"]), (0, 1))
        tree.write("build/RMHE08/src/unit.o", fixture_object(["drawSpr2TF__FUcP9fltSpr2TFUc",
                                                             "get_ScreenSize__FP8_MH_VEC2"], ["main_fn"]))
        s = undefrefs.linkage_sweep(root, with_decls=False)
        c.check("the repaired fixture is clean", (s["clean"], len(s["suspects"])), (1, 0))
        os.remove(tree.path("build/RMHE08/src/unit.o"))
        s = undefrefs.linkage_sweep(root, with_decls=False)
        c.check("a missing object is unbuilt, not clean, and named",
                (s["units_built"], s["clean"], [r["missing"] for r in s["unbuilt"]]),
                (0, 0, [["build/RMHE08/src/unit.o"]]))
        tree.write("src/unit.c", "void f(void) {}\n")
        tree.write("include/owner.h", 'extern "C" void drawSpr2TF(int a);\n')
        decls = undefrefs.declarations_for(undefrefs.declaration_index(root), "drawSpr2TF__FUcP9fltSpr2TFUc")
        c.check("the declaration lookup uses the linkage stem", [(d["file"], d["line"]) for d in decls],
                [("include/owner.h", 1)])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
