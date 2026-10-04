"""unionprose: the one union rule - code hunks union, prose hunks take the superset, mixed warns, no-superset blocks."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.tests.units import merge_fixtures as fx
from tools.units.merge import unionprose as up

TIER = "fixture"

ADDITIVE = ("Sections:\n"
            "anchor.cpp:\n"
            "\t.text       start:0x80000000 end:0x80000800\n"
            "<<<<<<< ours\n"
            "menu/main.cpp:\n"
            "\t.text       start:0x80001000 end:0x80002000\n"
            "||||||| base\n"
            "=======\n"
            "menu/branch.cpp:\n"
            "\t.text       start:0x80000800 end:0x80001000\n"
            ">>>>>>> theirs\n"
            "menu/tail.cpp:\n"
            "\t.text       start:0x80002000 end:0x80003000\n")
MIXED = ("int a;\n"
         "<<<<<<< ours\n"
         "/* main's paragraph. */\n"
         "int a_main;\n"
         "||||||| base\n"
         "int a_base;\n"
         "=======\n"
         "/* the branch's paragraph. */\n"
         "int a_lane;\n"
         ">>>>>>> theirs\n"
         "int z;\n")
MIXED_SUPERSET = ("<<<<<<< ours\n"
                  "/* main's paragraph. */\n"
                  "int a_main;\n"
                  "||||||| base\n"
                  "int a_base;\n"
                  "=======\n"
                  "/* main's paragraph. */\n"
                  "int a_main;\n"
                  "int a_branch_extra;\n"
                  ">>>>>>> theirs\n")
MIXED_INSERT = ("<<<<<<< ours\n"
                "/* main's note. */\n"
                "int a_main;\n"
                "=======\n"
                "/* the branch's note. */\n"
                "int a_lane;\n"
                ">>>>>>> theirs\n")
PROSE_NO_SUPERSET = ("<<<<<<< ours\n"
                     "/* main changed one word. */\n"
                     "||||||| base\n"
                     "/* the base paragraph. */\n"
                     "=======\n"
                     "/* the branch changed another. */\n"
                     ">>>>>>> theirs\n")


def test_additive_block_unions(c):
    merged, hunks, dec = up.union_markers(ADDITIVE, "config/RMHE08/splits.txt")
    c.check("an additive declaration block still unions both sides", merged,
            "Sections:\nanchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n"
            "menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n"
            "menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n"
            "menu/tail.cpp:\n\t.text       start:0x80002000 end:0x80003000\n")
    c.check("... one hunk, classified code", (hunks, [d["class"] for d in dec]), (1, ["code"]))
    c.check("... no side taken, no warning", ([d["took"] for d in dec], [d.get("warning") for d in dec]),
            ([None], [None]))
    c.check("... the union is ours-then-theirs, not a superset", merged.count("menu/branch.cpp"), 1)
    merged, conflicts, _ = up.union_markers("a\n<<<<<<< ours\nb\n||||||| base\nc\n=======\nd\ne\n>>>>>>> theirs\nf\n")
    c.check("a code conflict keeps ours and theirs and drops base", (merged, conflicts), ("a\nb\nd\ne\nf\n", 1))
    c.check("... and is idempotent for a line both sides added",
            up.union_markers("<<<<<<< o\nx\n=======\nx\n>>>>>>> t\n")[0], "x\n")
    c.check("no markers is no change, no hunks", up.union_markers("a\nb\n")[:2], ("a\nb\n", 0))


def test_real_prose_takes_the_superset(c):
    merged, hunks, dec = up.union_markers(fx.REAL_LOBBY_CONFLICT, "include/unsplit/lobby.h")
    c.check("the real prose rewrite is one prose hunk", (hunks, [d["class"] for d in dec]), (1, ["prose"]))
    c.check("... it takes the branch's superset copy, and says so",
            ([d["took"] for d in dec], all("superset" in d["why"] for d in dec)), (["theirs"], True))
    c.check("... the resolution IS the superset copy", merged, fx.REAL_LOBBY_SUPERSET)
    c.check("... no duplicated prose, no old generated name",
            (fx.nested_comment(merged), any(n in merged for n in fx.OLD_GENERATED_NAMES)), (False, False))
    c.check("... while the pre-fix union really duplicated it (the defect is real)",
            fx.nested_comment(fx.old_union(fx.REAL_LOBBY_CONFLICT)), True)
    merged, hunks, dec = up.union_markers(fx.REAL_FN_CONFLICT, "include/lobby/fn_801F3294.h")
    c.check("a hunk opening mid-comment is prose (the open block is carried in)",
            (hunks, [d["class"] for d in dec], [d["took"] for d in dec]), (1, ["prose"], ["theirs"]))
    c.check("... the resolution IS the superset copy", merged, fx.REAL_FN_SUPERSET)
    c.check("... no old file name reintroduced", any(n in merged for n in fx.OLD_FILES), False)


def test_mixed_and_blocked(c):
    merged, _h, dec = up.union_markers(MIXED, "include/mixed/thing.h")
    c.check("a mixed comment+code region with no superset unions as before", merged,
            "int a;\n/* main's paragraph. */\nint a_main;\n/* the branch's paragraph. */\nint a_lane;\nint z;\n")
    c.check("... classified mixed, no side taken, a warning naming the file",
            (dec[0]["class"], dec[0]["took"], "include/mixed/thing.h" in dec[0].get("warning", ""),
             "unioned as before" in dec[0].get("warning", "")), ("mixed", None, True, True))
    merged, _h, dec = up.union_markers(MIXED_SUPERSET, "include/mixed/thing.h")
    c.check("a mixed region with a superset prefers it, with no warning",
            ([d["took"] for d in dec], merged, [d.get("warning") for d in dec]),
            (["theirs"], "/* main's paragraph. */\nint a_main;\nint a_branch_extra;\n", [None]))
    dec = up.union_markers(MIXED_INSERT, "include/mixed/thing.h")[2]
    c.check("a mixed insertion (empty base) with no superset is reported too",
            (dec[0]["class"], dec[0]["took"], "warning" in dec[0]), ("mixed", None, True))
    dec = up.union_markers(PROSE_NO_SUPERSET, "include/else/thing.h")[2]
    c.check("a prose region with no superset is blocked, not unioned",
            (dec[0]["class"], dec[0].get("blocked"), dec[0]["took"]), ("prose", True, None))


def test_one_hunk_reader(c):
    two = ("x\n<<<<<<< a\no1\n||||||| b\n=======\nt1\n>>>>>>> c\ny\n"
           "<<<<<<< a\no2\n||||||| b\nold\n=======\nt2\n>>>>>>> c\n")
    segs = list(up.segments(two))
    hunks = [s for s in segs if isinstance(s, up.Hunk)]
    c.check("segments: plain lines and hunks in order", [s if isinstance(s, str) else "H" for s in segs],
            ["x\n", "H", "y\n", "H"])
    c.check("... each hunk carries its three sides", [(h.ours, h.base, h.theirs) for h in hunks],
            [(("o1\n",), (), ("t1\n",)), (("o2\n",), ("old\n",), ("t2\n",))])
    c.check("has_base_region: an empty base is a disjoint addition, a non-empty one an overlap",
            (up.has_base_region(two.split("y\n")[0]), up.has_base_region(two)), (False, True))
    blank_base = "<<<<<<< a\nx\n||||||| b\n\n   \n=======\ny\n>>>>>>> c\n"
    c.check("a base of blank lines only is no base region (both sides inserted at the anchor)",
            (up.has_base_region(blank_base), [s.has_base for s in up.segments(blank_base) if isinstance(s, up.Hunk)]),
            (False, [False]))
    c.check("markers_in finds a marker left in a line list",
            up.markers_in(["a", "<<<<<<< ours", "b", ">>>>>>>"]), [(2, "<<<<<<<"), (4, ">>>>>>>")])


def test_superset_rules(c):
    c.check("branch additions ignore blank lines", up.branch_only_additions(["a", ""], ["a", "", "b"]), ["b"])
    c.check("missing lines are reported in order", up.missing_from(["a"], ["a", "b", "c"]), ["b", "c"])
    cv = up.cover(["a"], ["a", "m"], ["a", "m", "t"])
    c.check("cover: each side's additions and who carries whose",
            (cv.ours_add, cv.theirs_add, cv.ours_has_theirs, cv.theirs_has_ours), (("m",), ("m", "t"), False, True))
    side, why = up.addadd_choice([], ["a", "b"], ["a", "b", "c"])
    c.check("add/add with no base: the superset side wins, explained", (side, "superset" in why and "branch" in why),
            ("theirs", True))
    c.check("... symmetrically", up.addadd_choice([], ["a", "b", "c"], ["a", "b"])[0], "ours")
    c.check("... an ambiguous pair is refused, naming what each side lacks",
            (up.addadd_choice([], ["a"], ["b"])[0], "neither copy is a superset" in up.addadd_choice([], ["a"], ["b"])[1]),
            (None, True))
    base = ["/* the band */", "int a;", "int b;"]
    c.check("with a pre-rename base only each side's own additions count",
            up.addadd_choice(base, base + ["main_only"], base + ["main_only", "branch_only"])[0], "theirs")
    c.check("... a copy that only deletes is not the superset", up.addadd_choice(base, base[:2], base + ["x"])[0],
            "theirs")
    c.check("... disjoint additions are refused", up.addadd_choice(base, base + ["x"], base + ["y"])[0], None)
    c.check("... a tie goes to the copy that kept every line, symmetrically",
            (up.addadd_choice(base, base + ["x"], base[:2] + ["x"])[0],
             up.addadd_choice(base, base[:2] + ["x"], base + ["x"])[0]), ("ours", "theirs"))
    c.check("... identical copies keep main's", up.addadd_choice(base, base, base)[0], "ours")
    c.check("prose_superset reads the same cover (a hunk-sized add/add)",
            up.prose_superset(["/* a */"], ["/* o */"], ["/* a */", "/* b */"])[0], "theirs")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
