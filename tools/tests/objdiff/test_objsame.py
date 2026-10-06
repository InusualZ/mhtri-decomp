"""objsame: two build trees' compiled objects compared section by section, names modulo `@N`/`$N` and a rename map;
unregistered objects skipped; `--base-tree` exports and builds a commit in a scratch directory (stub runner)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import types

from tools.lib import testing
from tools.lib.binary.build import ElfBuilder
from tools.lib.project import symbols as libsymbols
from tools.objdiff import objsame

TIER = "fixture"
OBJ = "build/RMHE08/src/"
CONFIGURE = ('cflags_base = ["-O4"]\nconfig.libs = [\n    {"lib": "A", "mw_version": "Wii/1.3", "cflags": cflags_base, '
             '"objects": [\n        Object(NonMatching, "A/a.c"),\n    ]},\n]\n')


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
        base, new = trees(tree, {"A/a.o": unit("@123"), "A/b.o": unit("@7@f@x"), "A/c.o": unit("__arraydtor$6556")},
                          {"A/a.o": unit("@125"), "A/b.o": unit("@9@f@x"), "A/c.o": unit("__arraydtor$6536")})
        r = objsame.run(base, new)
        c.check("renumbered pool labels and $N local counters: every object the same",
                (r["compared"], r["same"], r["differ"]), (3, 3, []))
        c.check("pool_free reads both counters", objsame.pool_free("__arraydtor$6556@12"), "__arraydtor$N@N")


def test_all_sections_string_table(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit("@99"), "A/b.o": unit("x$7")},
                          {"A/a.o": unit("@1000"), "A/b.o": unit("x$8")})
        r = objsame.run(base, new, all_sections=True)
        c.check("--all-sections: a renumbered label of another length is the same (.strtab normalised, "
                ".symtab name offsets ignored)", (r["same"], r["differ"]), (2, []))
        base2, new2 = trees(tree, {"A/c.o": unit(callee="aa")}, {"A/c.o": unit(callee="bb")})
        r = objsame.run(base2, new2, ["A/c"], all_sections=True)
        c.check("--all-sections: a real name change still differs", [d["unit"] for d in r["differ"]], ["A/c"])


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


def test_renames(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit(callee="fn_80001000", extra="fn_80002000"), "A/b.o": unit(),
                                 "A/c.o": unit(callee="fn_80001000")},
                          {"A/a.o": unit(callee="em_frame_check", extra="em_after"), "A/b.o": unit(),
                           "A/c.o": unit(callee="em_frame_check", text=b"\0\0\0\0")})
        r = objsame.run(base, new)
        c.check("without a rename map the renamed objects differ", sorted(d["unit"] for d in r["differ"]), ["A/a", "A/c"])
        ren = {"fn_80001000": "em_frame_check", "fn_80002000": "em_after", "unused_old": "unused_new"}
        r = objsame.run(base, new, rename=ren)
        c.check("with it: 1 identical, 1 differs only by renames (the renames it used, listed), 1 still differs",
                (r["same"], r["renamed"], [d["unit"] for d in r["differ"]]),
                (1, [{"unit": "A/a", "renames": ["fn_80001000 -> em_frame_check", "fn_80002000 -> em_after"]}],
                 ["A/c"]))
        c.check("a renamed-only object is not a failure", [row.name for row in objsame.verdict(r).rows], ["A/c"])
        m = tree.write("renames.txt", "# old new\nfn_1 new_1\nfn_2=new_2\n\n")
        c.check("a rename file: `old new` and `old=new` lines, comments skipped",
                objsame.parse_rename_map(str(m)), {"fn_1": "new_1", "fn_2": "new_2"})
        c.check("an inline map", objsame.parse_rename_map("a=b, c=d"), {"a": "b", "c": "d"})
        c.raises("a malformed entry is refused", ValueError, objsame.parse_rename_map, "a=b,c")


def test_rename_pairs(c):
    removed = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
               "fn_80002000 = .text:0x80002000; // type:function size:0x40",
               "lbl_80003000 = .data:0x80003000; // type:object size:0x4"]
    added = ["em_frame_check = .text:0x80001000; // type:function size:0x40",
             "moved_name = .text:0x80002004; // type:function size:0x3C",
             "a_obj = .data:0x80003000; // type:object size:0x2", "b_obj = .data:0x80003000; // type:object size:0x2"]
    c.check("one row renamed at an unchanged address is a rename; a moved row or a split place is not",
            libsymbols.rename_pairs(removed, added), {"fn_80001000": "em_frame_check"})


def test_renames_from_git(c):
    g = testing.GitFixture().init()
    try:
        g.commit({"config/RMHE08/symbols.txt": "fn_80001000 = .text:0x80001000; // type:function size:0x40\n"
                                                "keep_me = .text:0x80001040; // type:function size:0x20\n"}, "map")
        (g.root / "config/RMHE08/symbols.txt").write_text(
            "em_frame_check = .text:0x80001000; // type:function size:0x40\n"
            "keep_me = .text:0x80001040; // type:function size:0x20\n", encoding="utf-8", newline="\n")
        c.check("the symbols.txt diff against a ref, plus every stem the map names otherwise, is the rename map",
                objsame.renames_from_git(str(g.root), "main"),
                {"fn_80001000": "em_frame_check", "fn_80001040": "keep_me"})
        rows = [libsymbols.parse_line("x_data = .data:0x80003000; // type:object size:0x4"),
                libsymbols.parse_line("fn_80004000 = .text:0x80004000; // type:function size:0x4")]
        c.check("stem_renames: lbl_ for data, fn_ for functions, a generated name maps to nothing",
                libsymbols.stem_renames(rows), {"lbl_80003000": "x_data"})
    finally:
        g.cleanup()


def test_unregistered_objects_skipped(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit(), "A/old.o": unit()}, {"A/a.o": unit(), "A/old.o": unit(callee="x")})
        tree.write("base/configure.py", CONFIGURE)
        tree.write("new/configure.py", CONFIGURE)
        r = objsame.run(base, new)
        c.check("an object no Object(...) names is skipped on both sides, not compared",
                (r["compared"], r["same"], r["skipped_base"], r["skipped_tree"]), (1, 1, ["A/old"], ["A/old"]))
        r = objsame.run(base, new, registered_only=False)
        c.check("--all-objects compares it", [d["unit"] for d in r["differ"]], ["A/old"])


def test_base_tree(c):
    g = testing.GitFixture().init()
    try:
        g.commit({"configure.py": CONFIGURE, "src/A/a.c": "int a;\n"}, "base")
        sha = g.head()
        for rel in ("orig/RMHE08/sys/main.dol", "orig/RMHE08/files/mh3.sel", "build/tools/sjiswrap.exe", "build/tools/dtk.exe",
                    "build/compilers/x", "build/binutils/x"):
            p = g.root / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(b"x")
        calls = []

        def runner(argv, cwd):
            py = argv[0] == sys.executable
            calls.append((os.path.basename(argv[1]) if py else argv[0], argv[2:] if py else argv[1:], cwd))
            if argv[0] == "ninja":
                for t in argv[1:]:
                    if t.endswith(".o"):
                        p = os.path.join(cwd, t)
                        os.makedirs(os.path.dirname(p), exist_ok=True)
                        with open(p, "wb") as fh:
                            fh.write(unit())
            return types.SimpleNamespace(returncode=0, stdout="", stderr="")

        err = io.StringIO()
        d = objsame.build_base_tree(str(g.root), "main", ["A/*"], runner=runner, out=err)
        c.check("the base tree lives below build/tmp/objsame, keyed by the commit",
                os.path.relpath(d, g.root).replace("\\", "/"), "build/tmp/objsame/base-" + sha[:12])
        c.check("the commit's sources are exported, orig copied (not linked)",
                (os.path.isfile(os.path.join(d, "src/A/a.c")), os.path.isfile(os.path.join(d, "orig/RMHE08/sys/main.dol")),
                 os.path.islink(os.path.join(d, "orig"))), (True, True, False))
        c.check("configure points at the toolchain, then ninja builds the matching registered objects",
                [(name, args[:1]) for name, args, _cwd in calls], [("configure.py", ["--compilers"]),
                                                                  ("ninja", ["build" + os.sep + "RMHE08" + os.sep + "src"
                                                                             + os.sep + "A" + os.sep + "a.o"])])
        calls.clear()
        objsame.build_base_tree(str(g.root), "main", ["A/*"], runner=runner, out=err)
        c.check("a finished export is reused: no second configure, ninja only", [n for n, _a, _c in calls], ["ninja"])
        os.unlink(os.path.join(d, objsame.BASE_MARKER))
        with open(os.path.join(d, "stale.txt"), "w") as fh:
            fh.write("x")
        calls.clear()
        objsame.build_base_tree(str(g.root), "main", ["A/*"], runner=runner, out=err)
        c.check("an unfinished export (no marker) is wiped and redone", (os.path.exists(os.path.join(d, "stale.txt")),
                [n for n, _a, _c in calls]), (False, ["configure.py", "ninja"]))
    finally:
        g.cleanup()


def test_cli(c):
    with testing.FixtureTree() as tree:
        base, new = trees(tree, {"A/a.o": unit("@1")}, {"A/a.o": unit("@2")})
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = objsame.main([base, new])
        c.check("all the same: exit 0 and a count line", (code, out.getvalue().strip().splitlines()[-1]),
                (0, "objsame: 1 identical, 0 differ only by renames, 0 differ, 0 one-sided (of 1 compared; "
                    "0 unregistered object(s) skipped)"))
        tree.write("new/" + OBJ + "A/a.o", unit(callee="renamed"))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = objsame.main([base, new, "--rename-map", "callee=renamed"])
        c.check("only renames differ: exit 0, the unit listed RENAMED", (code, "RENAMED A/a  callee -> renamed"
                                                                         in out.getvalue()), (0, True))
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
        with contextlib.redirect_stderr(io.StringIO()):
            code = objsame.main([base, new, "--base-tree", "main"])
        c.check("BASE and --base-tree together cannot run", code, 2)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
