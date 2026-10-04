"""unionresolve: the landing path's union (the one rule) and the four invariants `check_union` asserts."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os
import tempfile

from tools.lib import testing
from tools.tests.units import merge_fixtures as fx
from tools.units.merge import unionresolve as ur

TIER = "fixture"


def test_the_union(c):
    merged, hunks = ur.union_text("head\n<<<<<<< ours\nmain block\n=======\nbranch block\n>>>>>>> theirs\ntail\n")
    c.check("a conflict unions to ours-then-theirs, counted", (merged, hunks), ("head\nmain block\nbranch block\ntail\n", 1))
    c.check("... leaving no marker", "<<<<<<<" in merged or ">>>>>>>" in merged, False)
    c.check("a clean file is passed through with no hunk", ur.union_text("no conflict here\n"), ("no conflict here\n", 0))
    c.check("a --diff3 conflict drops the base section",
            ur.union_text("a\n<<<<<<< ours\no\n||||||| base\nstale\n=======\nt\n>>>>>>> theirs\nz\n")[0], "a\no\nt\nz\n")
    additive = ("Sections:\nanchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n"
                "<<<<<<< ours\nmenu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n"
                "||||||| base\n=======\nmenu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n"
                ">>>>>>> theirs\nmenu/tail.cpp:\n\t.text       start:0x80002000 end:0x80003000\n")
    merged, hunks, dec = ur.union_text_full(additive, "config/RMHE08/splits.txt")
    c.check("the registration class still unions, main's block first",
            ("menu/main.cpp" in merged, "menu/branch.cpp" in merged,
             merged.index("menu/main.cpp") < merged.index("menu/branch.cpp")), (True, True, True))
    c.check("... one code hunk, no side taken, no warning",
            (hunks, [d["class"] for d in dec], [d["took"] for d in dec], [d.get("warning") for d in dec]),
            (1, ["code"], [None], [None]))


def test_real_prose_conflicts(c):
    for path, conflict, superset in (("include/unsplit/lobby.h", fx.REAL_LOBBY_CONFLICT, fx.REAL_LOBBY_SUPERSET),
                                     ("include/lobby/fn_801F3294.h", fx.REAL_FN_CONFLICT, fx.REAL_FN_SUPERSET)):
        merged, hunks, dec = ur.union_text_full(conflict, path)
        c.check("%s: every hunk is prose and takes the branch's superset" % path,
                ([d["class"] for d in dec], [d["took"] for d in dec]), (["prose"] * hunks, ["theirs"] * hunks))
        c.check("%s: the resolution equals the superset region" % path, merged, superset)
        c.check("%s: no nested `/*`, no old generated name, no old file name" % path,
                (fx.nested_comment(merged), any(n in merged for n in fx.OLD_GENERATED_NAMES + fx.OLD_FILES)),
                (False, False))


def test_union_file_refuses_a_blocked_hunk(c):
    prose_no_sup = ("<<<<<<< ours\n/* main changed one word. */\n||||||| base\n/* the base paragraph. */\n"
                    "=======\n/* the branch changed another. */\n>>>>>>> theirs\n")
    dec = ur.union_text_full(prose_no_sup, "include/else/thing.h")[2]
    c.check("a prose hunk with no superset is blocked", (dec[0]["class"], dec[0].get("blocked")), ("prose", True))
    with tempfile.TemporaryDirectory() as tmp:
        victim = os.path.join(tmp, "thing.h")
        with open(victim, "w", encoding="utf-8", newline="") as fh:
            fh.write(prose_no_sup)
        n, out = testing.capture(ur.union_file, victim)
        c.check("... union_file resolves nothing and says REFUSED", (n, "REFUSED" in out), (0, True))
        with open(victim, encoding="utf-8", newline="") as fh:
            c.check("... and leaves the conflict intact", fh.read(), prose_no_sup)
        good = os.path.join(tmp, "reg.txt")
        with open(good, "w", encoding="utf-8", newline="") as fh:
            fh.write("a\n<<<<<<< ours\nm\n=======\nb\n>>>>>>> theirs\n")
        with contextlib.redirect_stdout(io.StringIO()):
            n = ur.union_file(good)
        with open(good, encoding="utf-8", newline="") as fh:
            c.check("union_file unions a named file in place", (n, fh.read()), (1, "a\nm\nb\n"))


def test_invariants(c):
    splits = ("Sections:\n\t.text       type:code align:32\n\n"
              "a.cpp:\n\t.text       start:0x1000 end:0x1100\n\n"
              "a.cpp:\n\t.text       start:0x2000 end:0x2100\n")
    c.check("a duplicated unit key is found", ur.duplicate_unit_keys(ur.split_units(splits)), ["a.cpp"])
    conf = 'config.libs = [\n    Object(NonMatching, "a.cpp"),\n    Object(NonMatching, "a.cpp"),\n]\n'
    c.check("a duplicated Object() line is found", ur.duplicate_objects(ur.configure_objects(conf)),
            ['Object(NonMatching, "a.cpp")'])
    c.check("object_names reads the registered unit", ur.object_names(conf), ["a.cpp", "a.cpp"])
    rows = [("a.cpp", ".text", 0x1000, 0x1100), ("b.cpp", ".text", 0x10F0, 0x1200)]
    c.check("an overlapping .text range is found, naming both units",
            [o[1:3] for o in ur.overlapping_ranges(rows)], [("a.cpp", 0x1000)])
    c.check("touching ranges are the seam, not an overlap",
            ur.overlapping_ranges([("a.cpp", ".text", 0x1000, 0x1100), ("b.cpp", ".text", 0x1100, 0x1200)]), [])
    c.check("a .data overlap is out of the named sections",
            ur.overlapping_ranges([("a.cpp", ".data", 0x1000, 0x1100), ("b.cpp", ".data", 0x1050, 0x1200)]), [])

    main_splits = ("Sections:\n\t.text       type:code align:32\n\n"
                   "anchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n\n"
                   "main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n")
    merged = main_splits.replace("main.cpp:\n", "branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n\nmain.cpp:\n")
    main_conf = 'config.libs = [\n    Object(NonMatching, "main.cpp"),\n]\n'
    merged_conf = 'config.libs = [\n    Object(NonMatching, "branch.cpp"),\n    Object(NonMatching, "main.cpp"),\n]\n'
    c.check("a sound union has no problems", ur.check_union(main_splits, merged, main_conf, merged_conf), [])
    dup = merged + "\nmain.cpp:\n\t.text       start:0x80003000 end:0x80004000\n"
    drop = main_splits.replace("main.cpp:\n", "")
    ov = merged.replace("start:0x80000800 end:0x80001000", "start:0x80000400 end:0x80001800")
    drop_obj = 'config.libs = [\n    Object(NonMatching, "branch.cpp"),\n]\n'
    c.check("each of the four breakages is caught",
            [any(w in p for p in ur.check_union(main_splits, s, main_conf, conf_))
             for s, conf_, w in ((dup, merged_conf, "duplicate unit key"),
                                 (drop, merged_conf, "missing from the merged splits.txt"),
                                 (ov, merged_conf, "overlapping"),
                                 (merged, drop_obj, "missing from the merged configure.py"))], [True] * 4)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
