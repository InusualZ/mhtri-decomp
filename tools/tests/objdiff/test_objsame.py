"""objsame: two build trees' compiled objects compared section by section, relocation names modulo `@N`."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.objdiff import objsame

TIER = "fixture"
OBJ = "build/RMHE08/src/"


def unit(pool="@123", text=b"\x48\x00\x00\x01", callee="callee", data=b"\x3f\x80\x00\x00", extra=None):
    """A compiled object: `.text` calling `callee` and loading a `.sdata2` pool constant named `pool`."""
    b = (ElfBuilder().section(".text", text).section(".sdata2", data)
         .symbol("f", ".text", 0, 4, type="func").symbol(pool, ".sdata2", 0, 4, bind="local")
         .symbol(callee).reloc(".text", 0, callee, "R_PPC_REL24").reloc(".text", 2, pool, "R_PPC_EMB_SDA21"))
    if extra:
        b.symbol(extra, ".text", 0, 4, type="func")
    return b.build()


def trees(tree, base_objs, new_objs):
    for name, blob in base_objs.items():
        tree.write("base/" + OBJ + name, blob)
    for name, blob in new_objs.items():
        tree.write("new/" + OBJ + name, blob)
    return str(tree.path("base")), str(tree.path("new"))


def test_pool_numbering_is_not_a_difference(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit("@123"), "A/b.o": unit("@7@f@x")},
                          {"A/a.o": unit("@125"), "A/b.o": unit("@9@f@x")})
        r = objsame.run(base, new)
        c.check("renumbered pool labels: every object the same", (r["compared"], r["same"], r["differ"]), (2, 2, []))


def test_real_changes_are_named(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree,
                          {"A/a.o": unit(), "A/b.o": unit(), "A/c.o": unit(), "A/d.o": unit(), "A/gone.o": unit()},
                          {"A/a.o": unit(text=b"\x48\x00\x00\x05"), "A/b.o": unit(callee="other"),
                           "A/c.o": unit(data=b"\x40\x00\x00\x00"), "A/d.o": unit(extra="g"), "A/new.o": unit()})
        r = objsame.run(base, new)
        why = {d["unit"]: d["reasons"] for d in r["differ"]}
        c.check("a .text byte, a relocation name, a .sdata2 constant and a new symbol each differ",
                sorted(why), ["A/a", "A/b", "A/c", "A/d"])
        c.check("... the byte change names the section and offset", why["A/a"], [".text bytes differ at +0x3 (1 byte(s))"])
        c.check("... the relocation change names both spellings", why["A/b"], [".rela.text: +0x0 callee -> +0x0 other"])
        c.check("... a new defined symbol is a symbol-table difference", why["A/d"], ["symbols: -[] +[g]"])
        c.check("one-sided objects are reported, not compared", (r["only_base"], r["only_tree"]), (["A/gone"], ["A/new"]))
        c.check("--unit narrows both sides", objsame.run(base, new, ["A/c"])["compared"], 1)


def test_cli(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit("@1")}, {"A/a.o": unit("@2")})
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = objsame.main([base, new])
        c.check("all the same: exit 0 and a count line", (code, out.getvalue().strip().splitlines()[-1]),
                (0, "objsame: 1 of 1 object(s) the same modulo @N pool numbering; 0 differ, 0 one-sided"))
        tree.write("new/" + OBJ + "A/a.o", unit(text=b"\0\0\0\0"))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = objsame.main([base, new, "--json"])
        payload = json.loads(out.getvalue())
        c.check("a difference: exit 1, the lib.findings schema naming the unit",
                (code, payload["tool"], payload["ok"], [r["name"] for r in payload["rows"]]), (1, "objsame", False, ["A/a"]))
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            code = objsame.main([base, str(tree.path("missing"))])
        c.check("a tree with no build: exit 2, nothing built", (code, "builds nothing" in err.getvalue()), (2, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
