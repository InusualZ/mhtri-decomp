"""mergebranch: bring `main` into a held branch and resolve by class - map rows, superset sources, add/add, the
resume guard, and the union by hunk class through the real `resolve` path, on throwaway repositories."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os
import subprocess
import tempfile

from tools.lib import testing
from tools.tests.units import merge_fixtures as mf
from tools.units.merge.mergebranch import (addadd_choice, apply_renames, blob, branch_only_additions, classification,
                                           cleanliness_blocker, conflicted, git, lines_of, map_symbols, merge_file,
                                           merge_in_progress, missing_from, read_lines, rename_pairs, resolve,
                                           stale_generated_names, stripped_code, sweep_band_header, text_ranges,
                                           union_markers, union_side)

TIER = "fixture"
mf.hermetic_git()
SCRATCH = tempfile.gettempdir()
nested_comment = mf.nested_comment
buggy_union = mf.old_union
real_lobby_base, real_lobby_main, real_lobby_branch = mf.REAL_LOBBY_BASE, mf.REAL_LOBBY_MAIN, mf.REAL_LOBBY_BRANCH
real_fn_base, real_fn_main, real_fn_branch = mf.REAL_FN_BASE, mf.REAL_FN_MAIN, mf.REAL_FN_BRANCH
old_names, old_files = mf.OLD_GENERATED_NAMES, mf.OLD_FILES


@contextlib.contextmanager
def fixture():
    """A throwaway repository plus the `qgit`/`put` helpers the fixtures below share."""
    with tempfile.TemporaryDirectory() as tmp:
        def qgit(*args: str, check: bool = True) -> str:
            p = subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                                "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                               text=True, encoding="utf-8", errors="replace")
            if check and p.returncode != 0:
                raise AssertionError("git %s failed: %s" % (" ".join(args), p.stderr))
            return p.stdout

        def put(rel: str, text: str) -> None:
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        yield tmp, qgit, put


def project(qgit, put, rows: list[str]) -> None:
    """The `main`/`lane` pair every fixture here starts from: both sides insert a row in the same place
    of `symbols.txt` and the lane edits `src/f.c`, so the merge has at least one real conflict."""
    qgit("init", "-q")
    qgit("checkout", "-q", "-b", "main")
    put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
    put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
    put("src/f.c", "int a;\nint pad1;\nint pad2;\nint b;\n")
    qgit("add", "-A")
    qgit("commit", "-q", "-m", "base")
    qgit("checkout", "-q", "-b", "lane")
    put("config/RMHE08/symbols.txt",
        "\n".join(rows[:2] + ["lane_renamed = .text:0x80001080; // type:function size:0x40"]) + "\n")
    qgit("commit", "-q", "-am", "the branch renames a map row")
    qgit("checkout", "-q", "main")
    put("config/RMHE08/symbols.txt",
        "\n".join(rows[:2] + ["main_added = .text:0x80001060; // type:function size:0x20", rows[2]]) + "\n")
    qgit("commit", "-q", "-am", "main adds a map row")
    qgit("checkout", "-q", "lane")


def run_quiet(root: str, dry_run: bool) -> tuple[int, str]:
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        code = resolve(root, "lane", dry_run=dry_run, as_json=False)
    return code, out.getvalue()


def e2e(src_same_line: bool) -> tuple[int, str, str, str]:
    with tempfile.TemporaryDirectory() as tmp:
        def qgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                           check=True)

        def put(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                "fn_80001080 = .text:0x80001080; // type:function size:0x40",
                "fn_800010C0 = .text:0x800010C0; // type:function size:0x40"]
        qgit("init", "-q")
        qgit("checkout", "-q", "-b", "main")
        put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
        put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
        put("src/f.c", "int a;\nint pad1;\nint pad2;\nint pad3;\nint b;\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "base")
        # the branch renames one map row and edits the first source line
        qgit("checkout", "-q", "-b", "lane")
        put("config/RMHE08/symbols.txt",
            "\n".join(rows[:2] + ["lane_renamed = .text:0x80001080; // type:function size:0x40", rows[3]]) + "\n")
        put("src/f.c", "int a_lane;\nint pad1;\nint pad2;\nint pad3;\nint b;\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "the branch's work")
        # main moves: a row inserted one line above the renamed one (so the hunks overlap), and
        # either the same source line (a real conflict) or a different one (a clean merge)
        qgit("checkout", "-q", "main")
        put("config/RMHE08/symbols.txt",
            "\n".join(rows[:2] + ["main_added = .text:0x80001060; // type:function size:0x20", rows[2],
                                  rows[3]]) + "\n")
        put("src/f.c", "int a_main;\nint pad1;\nint pad2;\nint pad3;\nint b;\n" if src_same_line
            else "int a;\nint pad1;\nint pad2;\nint pad3;\nint b_main;\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "main moved")
        qgit("checkout", "-q", "lane")
        code = resolve(tmp, "lane", dry_run=True, as_json=False)
        merged = "\n".join(read_lines(tmp, "config/RMHE08/symbols.txt") or [])
        source = "\n".join(read_lines(tmp, "src/f.c") or [])
        log = git(tmp, "log", "--oneline", "-3")
        return code, merged, source, log


def e2e_comment() -> tuple[int, str, str]:
    with tempfile.TemporaryDirectory() as tmp:
        def qgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                           check=True)

        def put(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                "fn_80001040 = .text:0x80001040; // type:function size:0x40"]
        body = "int a;\nint pad1;\nint b;\n"
        qgit("init", "-q")
        qgit("checkout", "-q", "-b", "main")
        put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
        put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
        put("src/f.c", "/* the unit note. */\n" + body)
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "base")
        qgit("checkout", "-q", "-b", "lane")
        put("src/f.c", "/* the branch's `.text`-only sentence. */\n" + body)
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "the branch rewrote the note")
        qgit("checkout", "-q", "main")
        put("src/f.c", "/* DATA CLAIMED - this unit's range is its own. */\n" + body)
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "main rewrote the note")
        qgit("checkout", "-q", "lane")
        code = resolve(tmp, "lane", dry_run=True, as_json=False)
        return code, "\n".join(read_lines(tmp, "src/f.c") or []), git(tmp, "log", "--oneline", "-2")


def test_pure_resolvers(c):
    base = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
            "fn_80010040 = .text:0x80010040; // type:function size:0x40",
            "old_name = .text:0x80010080; // type:function size:0x20"]
    branch = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
              "fn_80010040 = .text:0x80010040; // type:function size:0x40",
              "new_name = .text:0x80010080; // type:function size:0x20"]
    pairs = rename_pairs(base, branch)
    c.check("one rename pairs up", pairs, [(base[2], branch[2])])
    main_map = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
                "old_name = .text:0x80010080; // type:function size:0x20",
                "added_on_main = .text:0x800100C0; // type:function size:0x10"]
    applied, unapplied = apply_renames(main_map, pairs)
    c.check("the rename lands on main's map", "new_name = .text:0x80010080; // type:function size:0x20" in applied, True)
    c.check("... main's own new row is untouched", "added_on_main = .text:0x800100C0; // type:function size:0x10" in applied, True)
    c.check("... order is preserved (the renamed row did not move)",
          applied.index("new_name = .text:0x80010080; // type:function size:0x20") > applied.index(base[0]), True)
    c.check("nothing unapplied", unapplied, [])
    c.check("a pair whose old row is gone is reported, not guessed",
          apply_renames(main_map, [("gone = .text:0x1; // x", "fresh = .text:0x1; // x")])[1],
          ["gone = .text:0x1; // x"])
    c.check("an already-applied pair is a no-op", apply_renames(applied, pairs)[1], [])

    merged, conflicts, _decisions = union_markers("a\n<<<<<<< ours\nb\n||||||| base\nc\n=======\nd\ne\n>>>>>>> theirs\nf\n")
    c.check("the union counts the conflict", conflicts, 1)
    c.check("... keeps ours and theirs, drops base", merged, "a\nb\nd\ne\nf\n")
    c.check("... and is idempotent for a line both sides added",
          union_markers("<<<<<<< o\nx\n=======\nx\n>>>>>>> t\n")[0], "x\n")
    c.check("no markers is no change", union_markers("a\nb\n")[0], "a\nb\n")

    splits = ("menu/arena_result.cpp:\n\t.text       start:0x803B0F98 end:0x803B465C\n\n"
              "enemy/em_pop.cpp:\n\t.text       start:0x803B465C end:0x803B8000\n")
    ranges = text_ranges(splits)
    c.check("splits.txt text ranges parse", ranges,
          [(0x803B0F98, 0x803B465C, "menu/arena_result.cpp"), (0x803B465C, 0x803B8000, "enemy/em_pop.cpp")])
    header = ["/* band */",
              "s32 fn_803B1000(void);                  /* 0x803B1000 */",
              "void fn_803B521C(s32);                  /* 0x803B521C */",
              "void quest_arena_count_get(u16 index);  /* 0x803B68F0 */",
              "void unowned_thing(void);               /* 0x80700000 */",
              "struct ScreenGeomView { int x; };"]
    kept, dropped = sweep_band_header(header, ranges)
    c.check("a declaration inside a registered range is swept", len(dropped), 3)
    c.check("... and the owner is named", [o for _d, o in dropped],
          ["menu/arena_result.cpp", "enemy/em_pop.cpp", "enemy/em_pop.cpp"])
    c.check("a declaration outside every range stays", "void unowned_thing(void);               /* 0x80700000 */" in kept, True)
    c.check("... and so does a struct", "struct ScreenGeomView { int x; };" in kept, True)

    c.check("the map class", classification("config/RMHE08/symbols.txt"), "map")
    c.check("the source class", classification("src/menu/arena_result.cpp"), "source")
    c.check("the band class", classification("include/unsplit/menu.h"), "band")
    c.check("everything else is three-way", classification("configure.py"), "threeway")
    c.check("branch additions ignore blank lines", branch_only_additions(["a", ""], ["a", "", "b"]), ["b"])
    c.check("missing lines are reported in order", missing_from(["a"], ["a", "b", "c"]), ["b", "c"])

    # --- the stale-generated-name scan must read a *symbol reference*, not a unit file's path ---------
    # 2026-09-28: a comment naming `src/DWCi/fn_805113B0.c` was read as five stale symbols and blocked a
    # merge whose map row at that address is `DWCi_sendControlFrame` (the file name, not a map row).
    live, addresses = map_symbols("DWCi_sendControlFrame = .text:0x805113B0; // type:function size:0xB8\n"
                                  "some_real_name = .text:0x80004320; // type:function size:0x10\n")
    c.check("the map's names and addresses parse",
          ("DWCi_sendControlFrame" in live, addresses.get(0x805113B0)),
          (True, "DWCi_sendControlFrame"))
    c.check("a path in a comment is not a stale symbol",
          stale_generated_names("* see `src/DWCi/fn_805113B0.c` and this unit.", live, addresses), [])
    c.check("... the bare `/fn_XXXXXXXX.c` form too",
          stale_generated_names("from `DWCi/fn_805113B0.c`", live, addresses), [])
    c.check("... and an object-file mention (`fn_XXXXXXXX.o`)",
          stale_generated_names("in fn_805113B0.o's relocations", live, addresses), [])
    c.check("a genuinely stale CALL is still reported",
          stale_generated_names("void f(void) { fn_80004320(); }\n", live, addresses), ["fn_80004320"])
    c.check("... even when a comment also names its file",
          stale_generated_names("/* fn_80004320.c */\nvoid f(void) { fn_80004320(); }\n",
                                live, addresses), ["fn_80004320"])
    c.check("a name the map cannot resolve is not reported",
          stale_generated_names("fn_DEADBEEF()", live, addresses), [])
    c.check("a name still live in the map is not stale",
          stale_generated_names("void f(void) { fn_80004320(); }\n",
                                live | {"fn_80004320"}, addresses), [])
    c.check("the shared stripper compares code, not comment length",
          stripped_code("/* a longer rewritten paragraph */\nint a;\n") ==
          stripped_code("/* short */\nint a;\n"), True)
    c.check("... and a real code change is not hidden by it",
          stripped_code("int a_lane;\n") == stripped_code("int a_main;\n"), False)


def test_real_repository_fixture(c):
    # the failure mode this tool exists for, on a real repository: a conflicted file left as main's copy
    with tempfile.TemporaryDirectory() as tmp:
        def qgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)
        qgit("init", "-q")
        qgit("checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"))
        with open(os.path.join(tmp, "src", "f.txt"), "w", newline="\n") as fh:
            fh.write("line one\nline two\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "base")
        cut = git(tmp, "rev-parse", "HEAD").strip()
        qgit("checkout", "-q", "-b", "lane")
        with open(os.path.join(tmp, "src", "f.txt"), "w", newline="\n") as fh:
            fh.write("line one\nline two\nlane adds this\n")
        qgit("commit", "-q", "-am", "the branch's own work")
        branch_blob = blob(tmp, "HEAD", "src/f.txt")
        qgit("checkout", "-q", "main")
        with open(os.path.join(tmp, "src", "f.txt"), "w", newline="\n") as fh:
            fh.write("line one\nmain moved\nline two\n")
        qgit("commit", "-q", "-am", "main moved")
        qgit("checkout", "-q", "lane")
        c.check("the fixture's branch edit is a real addition",
              branch_only_additions(lines_of(blob(tmp, cut, "src/f.txt")), lines_of(branch_blob)), ["lane adds this"])
        c.check("... and main's copy lacks exactly that line",
              missing_from(lines_of(blob(tmp, "main", "src/f.txt")), ["lane adds this"]), ["lane adds this"])


def test_end_to_end(c):
    code, merged, source, _log = e2e(src_same_line=False)
    c.check("a clean source merge resolves", code, 0)
    c.check("... the branch's rename landed on main's map", "lane_renamed = .text:0x80001080" in merged, True)
    c.check("... and main's own new row survived it", "main_added = .text:0x80001060" in merged, True)
    c.check("... in address order (main's row is the lower one)",
          merged.index("main_added") < merged.index("lane_renamed"), True)
    c.check("... and main's source edit is in the source file too", "b_main" in source, True)

    code2, merged2, _source2, _log2 = e2e(src_same_line=True)
    c.check("a same-line source conflict BLOCKS rather than picking a side", code2, 1)
    c.check("... while the map conflict still resolved (the branch's rename was kept)",
          "lane_renamed = .text:0x80001080" in merged2, True)
    c.check("... and main's row was kept as well", "main_added = .text:0x80001060" in merged2, True)
    code3, source3, _log3 = e2e_comment()
    c.check("a comment-only source conflict resolves instead of blocking", code3, 0)
    c.check("... keeping main's comment block", "DATA CLAIMED" in source3, True)
    c.check("... and dropping the branch's obsolete paragraph", "`.text`-only sentence" not in source3, True)
    c.check("... while the code is untouched", "int a;\nint pad1;\nint b;" in source3, True)

    # the guard that must NOT be weakened: a full body rewrite on the same line still refuses
    code4, _merged4, _source4, _log4 = e2e(src_same_line=True)
    c.check("a real code conflict still blocks after the comment-only fix", code4, 1)


def test_addadd_choice(c):
    side, why = addadd_choice([], ["a", "b"], ["a", "b", "c"])
    c.check("an add/add pair with no base blob: the superset side wins", side, "theirs")
    c.check("... and the choice is explained (which side, and why)", "superset" in why and "branch" in why, True)
    c.check("... symmetrically (main's copy can be the superset too)",
          addadd_choice([], ["a", "b", "c"], ["a", "b"])[0], "ours")
    c.check("... and an ambiguous pair is refused, never guessed", addadd_choice([], ["a"], ["b"])[0], None)
    c.check("... naming what each side lacks",
          "neither copy is a superset" in addadd_choice([], ["a"], ["b"])[1], True)
    abase = ["/* the band */", "int a;", "int b;"]
    c.check("with a pre-rename base, only each side's own additions are compared",
          addadd_choice(abase, abase + ["main_only"], abase + ["main_only", "branch_only"])[0], "theirs")
    c.check("... so a copy that only *deletes* is not the superset",
          addadd_choice(abase, abase[:2], abase + ["branch_only"])[0], "theirs")
    c.check("... and disjoint additions are refused (no superset either way)",
          addadd_choice(abase, abase + ["x"], abase + ["y"])[0], None)
    c.check("... a tie is broken by the copy that kept every line (a deletion is not a replacement)",
          addadd_choice(abase, abase + ["x"], abase[:2] + ["x"])[0], "ours")
    c.check("... and that tie-break is symmetric too",
          addadd_choice(abase, abase[:2] + ["x"], abase + ["x"])[0], "theirs")
    c.check("... identical copies are not a decision at all (main's is kept)",
          addadd_choice(abase, abase, abase)[0], "ours")


def test_addadd_end_to_end(c):
    def e2e_addadd_no_base() -> tuple[int, str, str]:
        """Fixture (a): a path added on both sides with **no base blob at all** (no rename to point at),
        main's copy a strict subset of the branch's.  It used to raise before it could decide anything."""
        with fixture() as (tmp, qgit, put):
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("README", "the root\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("src/Pl/new.cpp", "one\ntwo\nthree\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch adds the file")
            qgit("checkout", "-q", "main")
            put("src/Pl/new.cpp", "one\ntwo\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main adds the same path")
            qgit("checkout", "-q", "lane")
            # prove the fixture is the class under test before the tool runs: unmerged, **no stage 1**
            qgit("merge", "--no-commit", "--no-ff", "main", check=False)
            stages = sorted(ln.split()[2] for ln in
                            git(tmp, "ls-files", "-u", "--", "src/Pl/new.cpp").splitlines() if ln.strip())
            c.check("the fixture is a real add/add: unmerged with no base stage", stages, ["2", "3"])
            qgit("merge", "--abort")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "src/Pl/new.cpp") or [])
            return code, report, merged

    code5, report5, merged5 = e2e_addadd_no_base()
    c.check("add/add with no base blob resolves instead of crashing", code5, 0)
    c.check("... taking the superset side's copy", merged5.strip(), "one\ntwo\nthree")
    c.check("... and reporting which side won, as add/add",
          ("add/add" in report5, "superset" in report5), (True, True))

    def e2e_addadd_rehome() -> tuple[int, str, str, bool]:
        """Fixture (a'), the lane's real case: the **same re-home on both sides**.  Main's rename is
        detected (its copy is nearly the old file), the branch's is not (its copy carries the whole naming
        pass, so git reads it as a *new* file) - which is what makes this add/add rather than a rename.
        The pre-rename path is the true three-way base, and the branch's copy is the superset."""
        with fixture() as (tmp, qgit, put):
            body = "".join("line %d\n" % i for i in range(20))
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("src/Pl/fn_8024F200.cpp", body)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            qgit("mv", "src/Pl/fn_8024F200.cpp", "src/Pl/pl_act_step.cpp")
            put("src/Pl/pl_act_step.cpp", "main_edit 0\n" + body[len("line 0\n"):]
                + "".join("lane_line_%d\n" % i for i in range(40)))
            qgit("commit", "-q", "-am", "the branch re-homes and adds")
            qgit("checkout", "-q", "main")
            qgit("mv", "src/Pl/fn_8024F200.cpp", "src/Pl/pl_act_step.cpp")
            put("src/Pl/pl_act_step.cpp", "main_edit 0\n" + body[len("line 0\n"):])
            qgit("commit", "-q", "-am", "main re-homes (the pure half)")
            qgit("checkout", "-q", "lane")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "src/Pl/pl_act_step.cpp") or [])
            resurrected = os.path.exists(os.path.join(tmp, "src/Pl/fn_8024F200.cpp"))
            return code, report, merged, resurrected

    code6, report6, merged6, resurrected6 = e2e_addadd_rehome()
    c.check("a re-home add/add (both sides, one side's rename undetected) resolves", code6, 0)
    c.check("... by taking the branch's copy (it carries main's edit)",
          ("main_edit 0" in merged6, "lane_line_0" in merged6), (True, True))
    c.check("... whole - never a union, never markers", "<<<<<<<" not in merged6, True)
    c.check("... against the pre-rename base git names for it",
          "pre-rename base is 'src/Pl/fn_8024F200.cpp'" in report6, True)
    c.check("... and the pre-rename path itself is not resurrected", resurrected6, False)

    def e2e_addadd_band() -> tuple[int, str, str]:
        """An add/add **band header**: taking a side whole still owes the rule-2 address sweep, so a
        declaration whose address a registered unit owns is moved out (and named), as in every other
        band resolution."""
        with fixture() as (tmp, qgit, put):
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
            put("README", "the root\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("include/unsplit/Pl.h", "/* the band */\n"
                "void fn_80001040(void);        /* 0x80001040 */\n"
                "void unowned_thing(void);      /* 0x80700000 */\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch adds the band header")
            qgit("checkout", "-q", "main")
            put("include/unsplit/Pl.h", "/* the band */\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main adds the same header, empty")
            qgit("checkout", "-q", "lane")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "include/unsplit/Pl.h") or [])
            return code, report, merged

    code7b, report7b, merged7b = e2e_addadd_band()
    c.check("an add/add band header resolves", code7b, 0)
    c.check("... keeping the branch's copy's unowned declaration", "unowned_thing" in merged7b, True)
    c.check("... and sweeping the one a registered unit owns", "fn_80001040" not in merged7b, True)
    c.check("... naming where it belongs", "belong elsewhere" in report7b and "src/f.c" in report7b, True)


def test_resume_guard(c):
    def e2e_resume(stray: bool) -> dict:
        """Fixture (b): a **mid-merge** tree.  A `--dry-run` leaves exactly what a crash leaves - MERGE_HEAD,
        the state file, the resolved paths written, nothing committed - so the second invocation is the
        tool's own documented re-run.  With `stray`, an edit the merge does not own is made first: that is
        still a dirty tree and must still refuse."""
        with fixture() as (tmp, qgit, put):
            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                    "fn_80001080 = .text:0x80001080; // type:function size:0x40"]
            project(qgit, put, rows)
            code1, first = run_quiet(tmp, dry_run=True)
            crash_state = {"merge_head": merge_in_progress(tmp),
                           "clean": cleanliness_blocker(tmp, True, conflicted(tmp), False)}
            put("scratch.txt", "the lane's own notes\n")          # untracked: not the merge's business
            if stray:
                put("src/f.c", "int a;\nint lane_touched_this;\nint pad2;\nint b;\n")
            code2, second = run_quiet(tmp, dry_run=False)
            out = {"code1": code1, "code2": code2, "first": first, "second": second,
                   "merged": "\n".join(read_lines(tmp, "config/RMHE08/symbols.txt") or []),
                   "still_merging": merge_in_progress(tmp),
                   "scratch_tracked": bool(git(tmp, "ls-files", "scratch.txt").strip()),
                   "scratch_left": os.path.isfile(os.path.join(tmp, "scratch.txt")),
                   "committed": git(tmp, "log", "--oneline", "-2").splitlines()}
            out.update(crash_state)
            return out

    r = e2e_resume(stray=False)
    c.check("the dry run that leaves the mid-merge tree resolved", r["code1"], 0)
    c.check("the fixture leaves a merge in progress, as a crash does", r["merge_head"], True)
    c.check("a tree dirty only because a merge is in progress is not a dirty tree", r["clean"], None)
    c.check("the second invocation RESUMES instead of refusing the re-run", r["code2"], 0)
    c.check("... and says so", "resuming" in r["second"], True)
    c.check("... finishing the merge", r["still_merging"], False)
    c.check("... with both sides of the map kept",
          ("lane_renamed" in r["merged"], "main_added" in r["merged"]), (True, True))
    c.check("... and an untracked file is NOT swept into the merge commit", r["scratch_tracked"], False)
    c.check("... while it is still in the worktree", r["scratch_left"], True)

    s = e2e_resume(stray=True)
    c.check("an edit the merge does not own still refuses a resume", s["code2"], 1)
    c.check("... naming the path and the reason",
          ("outside it" in s["second"], "src/f.c" in s["second"]), (True, True))
    c.check("... and committing nothing", s["still_merging"], True)

    def e2e_foreign_merge() -> tuple[int, str]:
        """A merge this tool did **not** start has no recorded conflicted list to drive, and is not
        resumable: it is refused by that name (`git merge --abort` is the way out)."""
        with fixture() as (tmp, qgit, put):
            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                    "fn_80001080 = .text:0x80001080; // type:function size:0x40"]
            project(qgit, put, rows)
            subprocess.run(["git", "merge", "--no-commit", "--no-ff", "main"], cwd=tmp,
                           capture_output=True)
            return run_quiet(tmp, dry_run=False)

    code7, report7 = e2e_foreign_merge()
    c.check("a merge this tool did not start is refused, not resumed blindly", code7, 1)
    c.check("... by name", "no state for it" in report7, True)


def test_union_by_hunk_class(c):
    # (a) an ADDITIVE DECLARATION BLOCK still unions exactly as before - merges depend on this.
    additive = ("void aaa(void);\n"
                "<<<<<<< ours\n"
                "void fn_80002000(void);   /* 0x80002000 */\n"
                "||||||| base\n"
                "=======\n"
                "void fn_80002040(void);   /* 0x80002040 */\n"
                ">>>>>>> theirs\n"
                "void zzz(void);\n")
    add_merged, add_hunks, add_dec = union_markers(additive, "include/unsplit/band.h")
    c.check("an additive declaration block still unions both sides", add_merged,
          "void aaa(void);\n"
          "void fn_80002000(void);   /* 0x80002000 */\n"
          "void fn_80002040(void);   /* 0x80002040 */\n"
          "void zzz(void);\n")
    c.check("... one hunk, classified code", (add_hunks, [d["class"] for d in add_dec]), (1, ["code"]))
    c.check("... no side taken, no warning",
          ([d["took"] for d in add_dec], [d.get("warning") for d in add_dec]), ([None], [None]))


def test_real_prose_conflicts(c):
    for real_path, rbase, rmain, rbranch in (
            ("include/unsplit/lobby.h", real_lobby_base, real_lobby_main, real_lobby_branch),
            ("include/lobby/fn_801F3294.h", real_fn_base, real_fn_main, real_fn_branch)):
        rtext, _rrc = merge_file(SCRATCH, rmain.encode(), rbase.encode(), rbranch.encode())
        rmerged, rhunks, rdec = union_markers(rtext, real_path)
        c.check("%s: the rewrite is prose hunks" % real_path,
              [d["class"] for d in rdec], ["prose"] * rhunks)
        c.check("%s: every prose hunk takes the branch's superset copy" % real_path,
              [d["took"] for d in rdec], ["theirs"] * rhunks)
        c.check("%s: ... and says so, naming the branch as the superset" % real_path,
              all("superset" in d["why"] and "branch" in d["why"] for d in rdec), True)
        c.check("%s: the resolution IS the branch's copy, not a union" % real_path, rmerged, rbranch)
        c.check("%s: NO duplicated prose (no nested `/*`)" % real_path, nested_comment(rmerged), False)
        c.check("%s: no old generated name from the unioned side" % real_path,
              any(n in rmerged for n in old_names), False)
        c.check("%s: no old unit file name reintroduced" % real_path,
              any(n in rmerged for n in old_files), False)
        c.check("%s: the plain union would have duplicated the prose (the defect is real)" % real_path,
              buggy_union(rtext) != rmerged, True)
    c.check("the lobby.h union duplication is the nested-`/*` signature",
          nested_comment(buggy_union(merge_file(SCRATCH, real_lobby_main.encode(), real_lobby_base.encode(),
                                                real_lobby_branch.encode())[0])), True)

    # the same two conflicts through the real `resolve` path, at their real paths (band and three-way)
    def e2e_real_prose() -> tuple[int, str, str, str]:
        with fixture() as (tmp, qgit, put):
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("include/unsplit/lobby.h", real_lobby_base)
            put("include/lobby/fn_801F3294.h", real_fn_base)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("include/unsplit/lobby.h", real_lobby_branch)
            put("include/lobby/fn_801F3294.h", real_fn_branch)
            qgit("commit", "-q", "-am", "the branch's rename sweep")
            qgit("checkout", "-q", "main")
            put("include/unsplit/lobby.h", real_lobby_main)
            put("include/lobby/fn_801F3294.h", real_fn_main)
            qgit("commit", "-q", "-am", "main's rename")
            qgit("checkout", "-q", "lane")
            code, report = run_quiet(tmp, dry_run=True)
            lobby = "\n".join(read_lines(tmp, "include/unsplit/lobby.h") or [])
            fnh = "\n".join(read_lines(tmp, "include/lobby/fn_801F3294.h") or [])
            return code, report, lobby, fnh

    rcode, rreport, rlobby, rfn = e2e_real_prose()
    c.check("the real prose conflicts resolve through `resolve` (band + three-way)", rcode, 0)
    c.check("... include/unsplit/lobby.h is the branch's superset copy", rlobby, real_lobby_branch)
    c.check("... include/lobby/fn_801F3294.h is the branch's superset copy", rfn, real_fn_branch)
    c.check("... the report names the side taken and why",
          rreport.count("the branch's copy is the superset") >= 2, True)
    c.check("... and names both paths",
          ("include/unsplit/lobby.h" in rreport, "include/lobby/fn_801F3294.h" in rreport), (True, True))
    c.check("... no nested `/*` survived in lobby.h", nested_comment(rlobby), False)
    c.check("... no old generated name or file name reintroduced",
          any(n in rlobby or n in rfn for n in old_names + old_files), False)


def test_mixed_and_blocked(c):
    # (c) a MIXED region (comment lines and code lines): no superset here, so it keeps the old union and
    # REPORTS it - a warning naming the file, never a silent duplication.
    mixed = ("int a;\n"
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
    mix_merged, mix_hunks, mix_dec = union_markers(mixed, "include/mixed/thing.h")
    c.check("a mixed comment+code region still unions as before", mix_merged,
          "int a;\n/* main's paragraph. */\nint a_main;\n/* the branch's paragraph. */\nint a_lane;\nint z;\n")
    c.check("... classified mixed", [d["class"] for d in mix_dec], ["mixed"])
    c.check("... takes no side when there is no superset", [d["took"] for d in mix_dec], [None])
    c.check("... and WARNS, naming the file",
          ("include/mixed/thing.h" in (mix_dec[0].get("warning") or ""),
           "unioned as before" in (mix_dec[0].get("warning") or "")), (True, True))

    # a MIXED region WITH a superset: the superset is preferred and no warning is emitted
    mixed_sup = ("<<<<<<< ours\n"
                 "/* main's paragraph. */\n"
                 "int a_main;\n"
                 "||||||| base\n"
                 "int a_base;\n"
                 "=======\n"
                 "/* main's paragraph. */\n"
                 "int a_main;\n"
                 "int a_branch_extra;\n"
                 ">>>>>>> theirs\n")
    ms_dec = union_markers(mixed_sup, "include/mixed/thing.h")[2]
    c.check("a mixed region with a superset prefers it", [d["took"] for d in ms_dec], ["theirs"])
    c.check("... with no warning (the superset is sound)", [d.get("warning") for d in ms_dec], [None])

    # a mixed region with an EMPTY base (both sides inserted) is still reported, not unioned silently
    mixed_add = ("<<<<<<< ours\n"
                 "/* main's note. */\n"
                 "int a_main;\n"
                 "=======\n"
                 "/* the branch's note. */\n"
                 "int a_lane;\n"
                 ">>>>>>> theirs\n")
    ma_dec = union_markers(mixed_add, "include/mixed/thing.h")[2]
    c.check("a mixed insertion with no superset is reported too",
          (ma_dec[0]["class"], ma_dec[0]["took"], "warning" in ma_dec[0]), ("mixed", None, True))

    # a PROSE region with no superset is refused, never unioned (the operator resolves it by hand)
    prose_no_sup = ("<<<<<<< ours\n"
                    "/* main changed one word. */\n"
                    "||||||| base\n"
                    "/* the base paragraph. */\n"
                    "=======\n"
                    "/* the branch changed another. */\n"
                    ">>>>>>> theirs\n")
    pns_dec = union_markers(prose_no_sup, "include/else/thing.h")[2]
    c.check("a prose region with no superset is blocked, not unioned",
          (pns_dec[0]["class"], pns_dec[0].get("blocked"), pns_dec[0]["took"]), ("prose", True, None))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
