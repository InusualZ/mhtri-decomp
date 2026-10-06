"""lanecheck on a temp git repo: a branch whose added comments mis-cite an owner, name a gone path and call owned data
unowned; an added empty stub and an unmarked non-dump name; the callee and flipcheck rules on fixed inputs; the CLI."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json

from tools.lib import testing
from tools.units import lanecheck as lc

TIER = "fixture"

SYMBOLS = ("fn_80001000 = .text:0x80001000; // type:function size:0x20\n"
           "named_ok = .text:0x80001020; // type:function size:0x20\n"
           "known_fn = .text:0x80001040; // type:function size:0x20\n"
           "tiny_fn = .text:0x80001060; // type:function size:0x4\n"
           "other_fn = .text:0x80002000; // type:function size:0x40\n"
           "data_x = .data:0x80003000; // type:object size:0x10\n"
           "unowned_y = .data:0x80004000; // type:object size:0x10\n")
SPLITS = ("Sections:\n\t.text type:code\n\t.data type:data\n\nmod/a.c:\n\t.text       start:0x80001000 end:0x80001080\n\n"
          "mod/b.c:\n\t.text       start:0x80002000 end:0x80002100\n\t.data       start:0x80003000 end:0x80003100\n")
CONFIGURE = ('cflags_base = ["-O4"]\nconfig.libs = [\n    {"lib": "mod", "mw_version": "Wii/1.3", "cflags": cflags_base, '
             '"objects": [\n        Object(NonMatching, "mod/a.c"),\n        Object(NonMatching, "mod/b.c"),\n    ]},\n]\n')
HEADER = ("/* mod/a.c - the fixture unit.\n"
          " * RANGE. .text 0x80001000-0x80001080 (4 functions).\n"
          " * NAMES. Map stems.\n"
          " * RESIDUALS. fn_80001000: register swap.\n"
          " */\n")
BASE_A = HEADER + "int fn_80001000(void) { return 1; }\nint named_ok(void) { return 2; }\n"
BRANCH_A = (HEADER + "int fn_80001000(void) { return 1; }\nint named_ok(void) { return 2; }\n"
            "/* `other_fn` is owned by `mod/a.c` and declared here. */\n"
            "/* the old home was `mod/gone.c`. */\n"
            "/* `data_x` has no registered owner; `unowned_y` has no registered owner. */\n"
            "/* `known_fn` is owned by `mod/a.c`. */\n"
            "/* `fn_80001000` (owned by `mod/a.c`), `other_fn` stays. */\n"
            "/* `data_x` is read here.  The edges are unclaimed. */\n"
            "void known_fn(void) { }\n"
            "void tiny_fn(void) { }\n")
DUMP = {0x80001040: [{"name": "zz_0001040_", "clean": "zz_0001040_", "placeholder": True}],
        0x80001020: [{"name": "named_ok", "clean": "named_ok", "placeholder": False}]}


def _repo():
    g = testing.GitFixture().init()
    g.commit({"configure.py": CONFIGURE, "config/RMHE08/symbols.txt": SYMBOLS, "config/RMHE08/splits.txt": SPLITS,
              "src/mod/a.c": BASE_A, "src/mod/b.c": "int other_fn(void) { return 0; }\n"}, "base")
    g.branch("work", checkout=True)
    g.commit({"src/mod/a.c": BRANCH_A}, "the lane's work")
    g.checkout("main")
    return g


def _by_class(res):
    out = {}
    for i in res["items"]:
        out.setdefault(i.cls, []).append((i.line, i.token))
    return out


def test_branch_checks(c):
    g = _repo()
    try:
        res = lc.run(str(g.root), "main", "work", flipcheck=False, dump_path=None, dump=DUMP)
        got = _by_class(res)
        c.check("the touched unit is the registered one whose source changed", res["units"], ["mod/a"])
        c.check("(a) a symbol a line says `mod/a.c` owns while splits.txt gives it to mod/b",
                got.get("owner-by-address"), [(8, "other_fn")])
        c.check("(a) a cited path that names no file", got.get("stale-path"), [(9, "mod/gone.c")])
        c.check("(f) owned data called unowned is named; truly unowned data and a phrase in another clause are not",
                got.get("unowned-claim"), [(10, "data_x")])
        c.check("(c) an added empty body for a 0x20-byte function is a stub; a 4-byte one is not",
                got.get("empty-stub"), [(14, "known_fn")])
        c.check("(e) an added name the dump does not carry at its address and NAMES does not mark",
                got.get("guess"), [(14, "known_fn"), (15, "tiny_fn")])
        c.check("objects missing and flipcheck off are reported as not checked, never as clean",
                sorted(s.split(":")[0].split(" ")[0] for s in res["skipped"]), ["flip-blocker", "wrong-callee"])
        g.checkout("work")
        (g.root / "src/mod/a.c").write_text(BRANCH_A.replace(" * NAMES. Map stems.\n",
                                                              " * NAMES. `known_fn` and `tiny_fn` are GUESS names (what they do).\n"),
                                             encoding="utf-8", newline="\n")
        res = lc.run(str(g.root), "main", None, flipcheck=False, dump_path=None, dump=DUMP)
        c.check("the working tree is judged too; a GUESS mark clears the guess finding",
                ("guess" in _by_class(res), _by_class(res).get("owner-by-address")), (False, [(8, "other_fn")]))
    finally:
        g.cleanup()


def test_callee_and_blocker_rules(c):
    header = HEADER.replace("register swap.", "register swap; `setColor` takes the wrong parameter type.").replace(
        "(4 functions).", "(4 functions); .sdata2 0x80005000-0x80005040.")       # RANGE names sections, not blockers
    diffs = [{"function": "f", "diffs": [
        {"kind": "callee", "ours": "em_after__Fv", "target": "em_frame__Fv", "offset": 0x10},
        {"kind": "mangling", "ours": "setColor__4CharFUl", "target": "setColor__4CharFP5Color", "offset": 4}]},
        {"function": "g", "diffs": [{"kind": "mangling", "ours": "setColor__4CharFUl",
                                     "target": "setColor__4CharFP5Color", "offset": 8}]}]
    items = lc.check_callees("src/mod/a.c", header, 1, diffs)
    c.check("(b) one item per function, only the differences the RESIDUALS text does not name",
            [(i.token, i.what.split(" - ")[0]) for i in items],
            [("f", "f: 1 relocation symbol(s) differ and are not in the header's RESIDUALS")])
    problems = ["splits.txt claims .data (0x228) but the object emits no such section - flipping drops 552 bytes",
                ".text: object is 0x7D0, splits.txt claims 0x14CC (-3324)",
                ".sdata2: object is 0x10, splits.txt claims 0x40 (-48)",
                "mod/a: 2 referenced symbol(s) are defined by nothing a flip can use - fn_80001000, zz_x - our object",
                ".text: bytes differ from the target object at +0x81 (ours fd, target dd)"]
    got = [i.token for i in lc.check_blockers("src/mod/a.c", header, 1, problems)]
    c.check("(d) a data section claimed-not-emitted or size gap the residuals do not name is a finding; a .text gap, "
            "byte differences and an undefined name the header names are not", got, [".data", ".sdata2"])
    c.check("the header section reader stops at the next label",
            lc.header_section(HEADER, "NAMES").strip(), "* NAMES. Map stems.")


def test_cli(c):
    g = _repo()
    try:
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = lc.TOOL.run(lc.main, ["--root", str(g.root), "--branch", "work", "--no-flipcheck", "--no-dump"],
                             parser=lc.build_parser())
        text = out.getvalue()
        c.check("findings exit 1, one `file:line | class | what | hint` line each",
                (rc, "src/mod/a.c:8 | owner-by-address | `other_fn`" in text), (1, True))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = lc.TOOL.run(lc.main, ["--root", str(g.root), "--branch", "work", "--no-flipcheck", "--no-dump",
                                       "--json"], parser=lc.build_parser())
        data = json.loads(out.getvalue())
        c.check("--json carries the rows with their hint and the skipped checks",
                (data["ok"], sorted({r["rule"] for r in data["rows"]}), bool(data["rows"][0]["hint"]),
                 len(data["skipped"])),
                (False, ["empty-stub", "owner-by-address", "stale-path", "unowned-claim"], True, 3))
        g.commit({"src/mod/b.c": "int other_fn(void) { return 0; } /* plain */\n"}, "clean change")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = lc.TOOL.run(lc.main, ["--root", str(g.root), "--base", "main~1", "--branch", "main",
                                       "--no-flipcheck", "--no-dump"], parser=lc.build_parser())
        c.check("a clean change exits 0 (advisory skips do not fail without --strict)", rc, 0)
        with contextlib.redirect_stdout(io.StringIO()):
            rc = lc.TOOL.run(lc.main, ["--root", str(g.root), "--base", "main~1", "--branch", "main",
                                       "--no-flipcheck", "--no-dump", "--strict"], parser=lc.build_parser())
        c.check("--strict fails a run whose checks could not all run", rc, 1)
    finally:
        g.cleanup()


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
