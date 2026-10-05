"""land: the gate's tests, re-homed from `land.py --selftest` (WP4) - the guard, staging, the rows' decisions,
the real `land`/`land --branch` on fixture repos, the resolver and the CLI wiring."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import shutil
import subprocess
import tempfile
import unittest.mock as mock

import tools.units.claims as claims
import tools.units.dataclosure as dg
import tools.units.merge.unionguard as ug
import tools.units.verifyunit as vu
from tools.lib import testing
from tools.units.landing import api as L

TIER = "fixture"

testing.isolate_live_state()   # no real ~/.claude/sessions: a live lane must not change the verdict


# branch_commits() counts `main..branch`, not `<batch base>..branch`: a worker's branch is cut when the
# unit is claimed, so in a multi-batch round it predates the base the orchestrator later records. Real
# temp repos, because the check is entirely about git reachability. The unit is spelled extensionless and
# then called with the extension: the branch must be found under either spelling (the 2026-09-23 fix).
unit = "Pl/pl_act"
branch = claims.branch_for(unit)

def repo_git(path, *args):
    p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                        "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
    return p.stdout.strip()

def repo_commit(path, msg):
    with open(os.path.join(path, "f.txt"), "a", encoding="utf-8") as fh:
        fh.write(msg + "\n")
    repo_git(path, "add", "-A")
    repo_git(path, "commit", "-q", "-m", msg)


def fake_verify_with(write, gate_code=0, gate_problems=()):
    """A `verify` stand-in: `write(main)` is "the build", then the gate's verdict and its problems."""

    def fake_verify(main, units, base, dry_run, no_build, allow_regression=None, check_outbox=True,
                    release_claims=True, problems=None, branch=None, no_selftests=False, warnings=None):
        write(main)
        if problems is not None:
            problems.extend(gate_problems)
        L.write_land_message(main, "land: batch\n\nledger: (fixture)\n")
        return gate_code

    return fake_verify

def land_fixture(tmp, verify_stand_in, stage_scratch=False):
    """A repo on `main` with one batch file changed; run the real `land` under the stand-in."""
    repo_git(tmp, "init", "-q")
    repo_git(tmp, "checkout", "-q", "-b", "main")
    os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
    with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
        fh.write("base\n")
    repo_git(tmp, "add", "-A")
    repo_git(tmp, "commit", "-q", "-m", "base")
    base_sha = repo_git(tmp, "rev-parse", "HEAD")
    with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
        fh.write("the batch\n")
    if stage_scratch:
        # exactly what the landing flow's own `git add -A` was measured doing: the scratch in the index
        with open(os.path.join(tmp, "d910.json"), "w", encoding="utf-8") as fh:
            fh.write("staged by another step\n")
        repo_git(tmp, "add", "-A")
    buf, err = io.StringIO(), io.StringIO()
    with mock.patch.object(L.gate, "verify", verify_stand_in), \
            contextlib.redirect_stdout(buf), contextlib.redirect_stderr(err):
        code = L.land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                    release_claims=False, subject="x")
    return code, buf.getvalue(), err.getvalue(), base_sha

def write_scratch(main):
    with open(os.path.join(main, "d910.json"), "w", encoding="utf-8") as fh:
        fh.write("during the build\n")

def write_foreign(main):
    # outside the batch's expected set, and the file the gate exists to protect: a foreign SOURCE edit may
    # live under `src/`/`include/`/`configure.py` (those are the batch's own paths by design) - what this
    # guard refuses is work the batch could not have produced, e.g. the ground truth.
    os.makedirs(os.path.join(main, "config", "RMHE08"), exist_ok=True)
    with open(os.path.join(main, "config", "RMHE08", "build.sha1"), "w", encoding="utf-8") as fh:
        fh.write("someone else's edit\n")

def write_foreign_root(main):
    with open(os.path.join(main, "NOTES.md"), "w", encoding="utf-8") as fh:
        fh.write("a foreign file the batch did not receive\n")

def write_both(main):
    write_scratch(main)
    write_foreign(main)


def already_applied_repo(tmp):
    repo_git(tmp, "init", "-q")
    repo_git(tmp, "checkout", "-q", "-b", "main")
    os.makedirs(os.path.join(tmp, "src", "Pl"), exist_ok=True)
    with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
        fh.write("base\n")
    with open(os.path.join(tmp, ".gitignore"), "w", encoding="utf-8") as fh:
        fh.write(".pi/\n")
    repo_git(tmp, "add", "-A")
    repo_git(tmp, "commit", "-q", "-m", "base")
    with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
        fh.write("the batch, applied before record-base\n")
    L.record_base(tmp)          # the ordering mistake: the batch is already in the tree
    return repo_git(tmp, "rev-parse", "HEAD")


# outbox_units() must look where brief.py wrote: the claim's branch minus worker/, not slug(unit)
entry = {"unit": "Pl/pl_act", "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 50.0,
         "symbols": [{"name": "fn_1", "percent": 50.0}], "residual": "none",
         "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}


def _write_tree(tmp, files):
    for rel, text in files.items():
        path = os.path.join(tmp, *rel.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    repo_git(tmp, "add", "-A")
    repo_git(tmp, "commit", "-q", "-m", "write tree")

band_fixture = {
    "config/RMHE08/symbols.txt": (
        "fn_8019E9AC = .text:0x801993E0; // type:function size:0x10\n"
        "em_act_ck__FP11_ENEMY_WORKUcUc = .text:0x801993E4; // type:function size:0x8\n"
        "fn_ALREADY = .text:0x80010000; // type:function size:0x10\n"
        "fn_WIDENED = .text:0x80010014; // type:function size:0x8\n"
        "fn_OTHER = .text:0x80020000; // type:function size:0x8\n"),
    "config/RMHE08/splits.txt": (
        "Sections:\n\t.text       type:code align:32\n\n"
        "existing/unit.cpp:\n\t.text       start:0x80010000 end:0x80010010\n"),
    "configure.py": "config.libs = [\n]\n",
    "include/unsplit/enemy.h": (
        "#ifndef B1\n#define B1\n"
        "void fn_8019E9AC(void);\n"
        "void fn_OTHER(void);\n"
        "void fn_ALREADY(void);\n"
        "#endif\n"),
    "include/unsplit/widen.h": "#ifndef B2\n#define B2\nvoid fn_WIDENED(void);\n#endif\n",
    # a C++ callee declared by its clean spelling while symbols.txt holds the mangling
    "include/unsplit/cxx.h": "#ifndef B4\n#define B4\nu32 em_act_ck(struct _ENEMY_WORK*, u8);\n#endif\n",
}


SECTION = "Sections:\n\t.text       type:code align:32\n\n"
ANCHOR_SPLIT = "anchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n"
TAIL_SPLIT = "tail.cpp:\n\t.text       start:0x80010000 end:0x80011000\n"
CONF_HEAD = 'config.libs = [\n    {\n        "lib": "menu",\n        "objects": [\n'
CONF_ANCHOR = '            Object(NonMatching, "anchor.cpp"),\n'
CONF_TAIL = '            Object(NonMatching, "tail.cpp"),\n'
CONF_FOOT = '        ],\n    },\n]\n'

def _registration_files(extra_splits="", extra_conf="", anchor_split=ANCHOR_SPLIT,
                        anchor_conf=CONF_ANCHOR):
    return {
        "config/RMHE08/splits.txt": SECTION + anchor_split + extra_splits + "\n" + TAIL_SPLIT,
        "configure.py": CONF_HEAD + anchor_conf + extra_conf + CONF_TAIL + CONF_FOOT,
        "src/anchor.cpp": "int a(void) { return 0; }\n",
        "src/tail.cpp": "int t(void) { return 0; }\n",
        ".gitignore": ".pi/\n",
    }

def _resolve_fixture(tmp, branch_splits="", main_splits="", branch_conf="", main_conf="",
                     branch_anchor_conf=CONF_ANCHOR, main_anchor_conf=CONF_ANCHOR):
    """`main` and `worker/x` both append a registration at one anchor; -> the merge-base sha."""
    repo_git(tmp, "init", "-q")
    repo_git(tmp, "checkout", "-q", "-b", "main")
    _write_tree(tmp, _registration_files())
    base = repo_git(tmp, "rev-parse", "HEAD")
    repo_git(tmp, "checkout", "-q", "-b", "worker/x")
    _write_tree(tmp, _registration_files(branch_splits, branch_conf, ANCHOR_SPLIT, branch_anchor_conf))
    repo_git(tmp, "checkout", "-q", "main")
    _write_tree(tmp, _registration_files(main_splits, main_conf, ANCHOR_SPLIT, main_anchor_conf))
    return base

def _conflicted_worktree(tmp, branch="worker/x", name="scratch"):
    """A scratch worktree on `branch`, with `git merge main` left mid-conflict (ours=branch)."""
    wt = os.path.join(tmp, name)
    repo_git(tmp, "worktree", "add", "-b", name, wt, branch)
    subprocess.run(["git", "merge", "--no-commit", "main"], cwd=wt,
                   capture_output=True, text=True, encoding="utf-8", errors="replace")
    return wt


def _land_verify_ok(main, units, base, dry_run, no_build, allow_regression=None,
                    check_outbox=True, release_claims=True, problems=None, branch=None,
                    no_selftests=False, warnings=None):
    L.write_land_message(main, "land: %s\n\nledger: (fixture)\n" % ",".join(units))
    return 0


def test_batch_paths_stage_and_subject(c):
    """The batch-path guard, unit renames, batch paths vs units, conflict markers, staging, the decision, messages."""
    check = c.check
    check("inside the batch: a unit source", L.outside_batch(["src/Pl/pl_act.cpp"]), [])
    check("inside the batch: splits.txt", L.outside_batch(["config/RMHE08/splits.txt"]), [])
    check("outside the batch: orig", L.outside_batch(["orig/RMHE08/sys/main.dol"]), ["orig/RMHE08/sys/main.dol"])
    check("outside the batch: build", L.outside_batch(["build/RMHE08/main.dol"]), ["build/RMHE08/main.dol"])
    check("outside the batch: ground truth", L.outside_batch(["config/RMHE08/build.sha1"]),
          ["config/RMHE08/build.sha1"])
    check("outside the batch: the campaign state files", L.outside_batch([".pi/claims.json"]), [".pi/claims.json"])

    # the flip-readiness row's input: which units configure.py turns Matching (2026-09-29). Real temporary repo.
    with testing.temp_dir() as tmp:
        def _f(*a):
            subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *a], cwd=tmp, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        def _conf(*rows):
            with open(os.path.join(tmp, "configure.py"), "w", newline="") as fh:
                fh.write("".join('    Object(%s, "%s"),' % r + chr(10) for r in rows))
        _f("init", "-q")
        _conf(("NonMatching", "Pl/pl_act.cpp"), ("Matching", "OS/a.c"))
        _f("add", "-A")
        _f("commit", "-q", "-m", "base")
        check("flipped_units: nothing flipped", L.flipped_units(tmp), [])
        _conf(("Matching", "Pl/pl_act.cpp"), ("Matching", "OS/a.c"))
        check("flipped_units: names only the unit that gained Matching", L.flipped_units(tmp), ["Pl/pl_act"])
        _conf(("NonMatching", "Pl/pl_act.cpp"), ("Matching", "OS/a.c"), ("Matching", "New/b.cpp"))
        check("flipped_units: a new Matching unit counts", L.flipped_units(tmp), ["New/b"])
    check("flipcheck_problems: no units, no work", L.flipcheck_problems(".", []), [])
    L.set_unit_renames(["Network/fn_803D3CE8=Network/NetworkSessionManager", "bad"])
    check("unit renames: the declared pair is recorded, junk ignored",
          L.state.UNIT_RENAMES, {"Network/fn_803D3CE8": "Network/NetworkSessionManager"})
    check("unit renames: a snapshot key follows the new name",
          L.rename_snapshot_keys({"Network/fn_803D3CE8": {"x": 1}, "OS/a": {"y": 2}}),
          {"Network/NetworkSessionManager": {"x": 1}, "OS/a": {"y": 2}})
    L.set_unit_renames(["E/a=E/c", "E/b=E/c", "E/gone="])
    check("unit renames: several OLDs share one NEW, OLD= is kept as a drop",
          L.state.UNIT_RENAMES, {"E/a": "E/c", "E/b": "E/c", "E/gone": ""})
    check("unit renames: a fold merges its OLDs' entries under NEW (refs unioned), OLD= drops the entry",
          L.rename_snapshot_keys({"E/a": {"source": "1", "refs": ["x"]}, "E/b": {"source": "2", "refs": ["y"]},
                                "E/c": {"source": "3", "refs": ["z"]}, "E/gone": {"refs": ["q"]}}),
          {"E/c": {"source": "3", "refs": ["x", "y", "z"]}})
    L.set_unit_renames(["E/donor=E/x", "E/donor=E/y", "E/solo=E/z"])
    check("unit renames: one OLD onto several NEWs is copied to each; only single-target OLDs reach the data row",
          (L.state.UNIT_RENAMES, L.rename_snapshot_keys({"E/donor": {"refs": ["d"]}, "E/x": {"refs": ["a"]}})),
          ({"E/solo": "E/z"}, {"E/x": {"refs": ["a", "d"]}, "E/y": {"refs": ["d"]}}))
    L.set_unit_renames(["E/d=E/x"], ["E/d", "E/x"])
    check("unit renames: a donor that survives the batch keeps its own entry beside the copy",
          L.rename_snapshot_keys({"E/d": {"refs": ["d"]}}), {"E/x": {"refs": ["d"]}, "E/d": {"refs": ["d"]}})
    L.set_unit_renames(None)
    check("unit renames: none declared leaves the snapshot alone",
          L.rename_snapshot_keys({"OS/a": {"y": 2}}), {"OS/a": {"y": 2}})

    # a batch that DELETES an extension-less file: after the apply the tree no longer has it, so only the
    # BASE's tree can say it was a path (`tools/git/hooks/post-commit`, 2026-09-28). Real temporary repo.
    with testing.temp_dir() as tmp:
        def _g(*a):
            subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *a], cwd=tmp, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        _g("init", "-q")
        os.makedirs(os.path.join(tmp, "tools", "git", "hooks"))
        open(os.path.join(tmp, "tools", "git", "hooks", "post-commit"), "w").write("#!/bin/sh\n")
        open(os.path.join(tmp, "tools", "git", "hooks", "pre-commit"), "w").write("#!/bin/sh\n")
        _g("add", "-A")
        _g("commit", "-q", "-m", "base")
        base = L.git(["rev-parse", "HEAD"], tmp).strip()
        os.remove(os.path.join(tmp, "tools", "git", "hooks", "post-commit"))
        hook = "tools/git/hooks/post-commit"
        check("a deleted extension-less file is gone from the tree", os.path.exists(os.path.join(tmp, hook)), False)
        check("... and without the base it is misread as a unit (the bug)", L.is_batch_path(tmp, hook), False)
        check("... with the base it is a batch path", L.is_batch_path(tmp, hook, base), True)
        check("unit_rows drops the deleted hook given the base",
              L.unit_rows(tmp, [hook, "Pl/pl_act"], base), ["Pl/pl_act"])
        check("a unit name that is not a path is still a unit at the base",
              L.is_batch_path(tmp, "menu/arena_result", base), False)
        check("an extension-less file that exists is still a path",
              L.is_batch_path(tmp, "tools/git/hooks/pre-commit", base), True)
        check("a directory at the base is a path", L.is_batch_path(tmp, "tools/git", base), True)
        check("an unresolvable base changes nothing", L.is_batch_path(tmp, hook, "0" * 40), False)

    # ALLOWED_FILES: the root documents a docs/tooling batch edits are allowed, an unknown root file is not
    check("inside the batch: README.md", L.outside_batch(["README.md"]), [])
    check("inside the batch: LICENSE", L.outside_batch(["LICENSE"]), [])
    check("inside the batch: .gitattributes and .flake8", L.outside_batch([".gitattributes", ".flake8"]), [])
    check("inside the batch: the CI example", L.outside_batch([".github.example/workflows/build.yml"]), [])
    check("outside the batch: an unknown root file", L.outside_batch(["notes.txt"]), ["notes.txt"])
    check("outside the batch: .gitmodules", L.outside_batch([".gitmodules"]), [".gitmodules"])

    # a `--units` entry that names a repo PATH is not a unit: it has no `Object(...)` line, no splits.txt
    # block and no `build/RMHE08/src/<unit>.o` target, so the unit-shaped rows must skip it. The extension
    # signal alone missed the extension-less ones - a batch naming `.gitignore` or `LICENSE` was refused by
    # the registration row with a message about a source file registered in name only (2026-09-27).
    with testing.temp_dir() as tmp:
        open(os.path.join(tmp, ".gitignore"), "w").write("build/\n")
        open(os.path.join(tmp, "LICENSE"), "w").write("CC0\n")
        open(os.path.join(tmp, "Makefile"), "w").write("all:\n")
        os.makedirs(os.path.join(tmp, "docs"))
        check("a hidden file is a batch path, not a unit", L.is_batch_path(tmp, ".gitignore"), True)
        check("an extension-less file at the root is a batch path", L.is_batch_path(tmp, "LICENSE"), True)
        check("a Makefile is a batch path", L.is_batch_path(tmp, "Makefile"), True)
        check("a directory the batch stages is a batch path", L.is_batch_path(tmp, "docs"), True)
        check("a tool script is a batch path", L.is_batch_path(tmp, "tools/units/land.py"), True)
        check("a unit is not a path - the bare name is not a file in the tree",
              L.is_batch_path(tmp, "menu/arena_result"), False)
        check("... with or without the `src/` prefix",
              L.is_batch_path(tmp, "src/menu/arena_result"), False)
        check("... and a deeper unit path is still a unit",
              L.is_batch_path(tmp, "Network/initNetworkSessionStable"), False)
        os.makedirs(os.path.join(tmp, "docs", "matching"))
        open(os.path.join(tmp, "docs", "matching", "043-demo.cpp"), "w").write("int f();")
        os.makedirs(os.path.join(tmp, "src", "Network"))
        open(os.path.join(tmp, "src", "Network", "unit_x.cpp"), "w").write("int g();")
        check("a demo .cpp outside src/ (its extension stripped by norm_unit) is a batch path",
              L.is_batch_path(tmp, "docs/matching/043-demo"), True)
        check("... a unit whose source is under src/ is still a unit", L.is_batch_path(tmp, "Network/unit_x"), False)
        check("... including the src/ spelling", L.is_batch_path(tmp, "src/Network/unit_x"), False)

        # a committed conflict marker: the build only reports it as a syntax error, so it is worth a row of
        # its own. `=======` alone is a banner, not evidence.
        open(os.path.join(tmp, "conflicted.c"), "w").write(
            "int a;\n<<<<<<< HEAD\nint b;\n=======\nint c;\n>>>>>>> other\n")
        open(os.path.join(tmp, "banner.c"), "w").write("/* ======= */\nint d;\n")
        open(os.path.join(tmp, "clean.c"), "w").write("int e;\n")
        check("a conflict marker is found, with its line and spelling",
              L.conflict_marker_files(tmp, ["conflicted.c"]),
              [("conflicted.c", 2, "<<<<<<<"), ("conflicted.c", 6, ">>>>>>>")])
        check("a banner of `=======` alone is not a marker", L.conflict_marker_files(tmp, ["banner.c"]), [])
        check("a clean file has none", L.conflict_marker_files(tmp, ["clean.c"]), [])
        check("a path that is not a file is skipped, not an error",
              L.conflict_marker_files(tmp, ["gone.c", "docs"]), [])
        check("... and the scan covers the batch's files together",
              len(L.conflict_marker_files(tmp, ["clean.c", "conflicted.c", "banner.c"])), 2)

    # the pre-flight names the likely cause of a foreign path whose name looks like lane scratch (the
    # `.tmp-mwcc/upstream` incident); an ordinary foreign file gets no invented cause
    check("preflight: `.tmp-*` names the mis-launch cause",
          "cwd set to MAIN" in (L.likely_cause(".tmp-mwcc/upstream/x.c") or ""), True)
    check("preflight: `.ws-*` also names lane scratch",
          "cwd set to MAIN" in (L.likely_cause(".ws-foo/f.txt") or ""), True)
    check("preflight: a bare `upstream` names a leftover clone",
          "cloned upstream" in (L.likely_cause("upstream/x.c") or ""), True)
    check("preflight: an ordinary foreign file gets no invented cause", L.likely_cause("NOTES.md"), None)

    # `land` stages the batch's own files only: a tracked change is the batch, but an untracked file that is
    # not a named unit's source is another stream's in-flight work (the round's `tools/units/playbook.py`)
    rows = [(" M", "src/Pl/pl_act.cpp"), ("??", "src/Pl/pl_act.cpp"), ("??", "tools/units/playbook.py"),
            ("??", "tools/units/land.py"), ("??", "include/Foo.h"), ("??", "src/Other/other.cpp"),
            (" M", "configure.py"), ("??", "build/RMHE08/main.dol")]
    check("a tracked batch file is staged", "src/Pl/pl_act.cpp" in L.land_stageable(["Pl/pl_act"], rows), True)
    check("an untracked unit source is staged", "src/Pl/pl_act.cpp" in L.land_stageable(["Pl/pl_act"], rows), True)
    check("another worker's untracked tool is not",
          "tools/units/playbook.py" in L.land_stageable(["Pl/pl_act"], rows), False)
    check("an untracked source or header is staged",
          ("include/Foo.h" in L.land_stageable(["Pl/pl_act"], rows)
           and "src/Other/other.cpp" in L.land_stageable(["Pl/pl_act"], rows)), True)
    check("a unit named by its own path is staged",
          "tools/units/land.py" in L.land_stageable(["tools/units/land.py"], rows), True)
    check("a shared-file edit is staged", "configure.py" in L.land_stageable(["Pl/pl_act"], rows), True)
    check("build output is never staged", "build/RMHE08/main.dol" in L.land_stageable(["Pl/pl_act"], rows), False)
    # the incident: a *tracked* `tools/` change is another stream's work unless the batch names it
    check("another worker's tracked tool edit is not staged",
          "tools/units/langcheck.py" in L.land_stageable(["Pl/pl_act"], [(" M", "tools/units/langcheck.py")]), False)
    check("a tool the batch names is still staged",
          "tools/units/land.py" in L.land_stageable(["tools/units/land.py"], [(" M", "tools/units/land.py")]), True)

    # the 2026-09-24 hazard: a path that was already dirty when the batch base was recorded is another
    # stream's work, not batch material, even though it sits inside the allowed set - a prepared `docs/plan.md`
    # rode `85ddd7b6` and a `src/RSO/runtime.c` header rode `890631e8`. The snapshot comes from `record_base`.
    foreign_rows = [(" M", "docs/plan.md"), (" M", "src/RSO/runtime.c"), (" M", "src/Pl/pl_act.cpp"),
                    (" M", "configure.py")]
    dirty_at_base = {"docs/plan.md", "src/RSO/runtime.c"}
    check("a path dirty at the batch base is not staged",
          "docs/plan.md" in L.land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("... and neither is a foreign unit header",
          "src/RSO/runtime.c" in L.land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("a path the batch names is staged even if dirty at the base",
          "src/Pl/pl_act.cpp" in L.land_stageable(["Pl/pl_act"], foreign_rows, {"src/Pl/pl_act.cpp"}), True)
    check("a path clean at the base is still batch material",
          "configure.py" in L.land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), True)
    check("without the base snapshot the old sweep is reproduced (the failure mode)",
          "docs/plan.md" in L.land_stageable(["Pl/pl_act"], foreign_rows), True)

    # the one-command path: the gate's verdict is the decision, so a red gate can never reach `git commit`
    check("a failed gate refuses the commit", L.land_decision(False, ["src/Pl/pl_act.cpp"]),
          ("refuse", "the gate failed - nothing staged or committed"))
    check("a green gate with nothing to stage refuses", L.land_decision(True, []),
          ("refuse", "the gate passed but no batch path is stageable - nothing to commit"))
    check("a green gate with a batch commits", L.land_decision(True, ["src/Pl/pl_act.cpp"]),
          ("commit", ""))

    # the 2026-09-24 incident: a `--message "$(cat /tmp/msg1.txt)"` whose file lived at a different `/tmp`
    # expanded to the empty string, `if subject:` dropped the override, and the batch landed under the gate's
    # fallback subject instead of the one the worker wrote (`71c244f9`). An empty or blank argument is refused
    # before the gate runs; omitting --message still asks for the gate's default subject; a real one lands.
    check("no --message uses the gate's default subject", L.message_error(None), None)
    empty_err = L.message_error("")
    check("an empty --message is refused", empty_err is not None, True)
    check("... and the refusal names --message", "--message" in (empty_err or ""), True)
    check("... and names it empty", "is empty" in (empty_err or ""), True)
    blank_err = L.message_error("  \t\n ")
    check("a whitespace-only --message is refused", blank_err is not None, True)
    check("... and the refusal names --message", "--message" in (blank_err or ""), True)
    check("... and names it whitespace-only", "whitespace-only" in (blank_err or ""), True)
    check("a real --message is accepted", L.message_error("ef: land fn_800FAE08 (41/41 symbols)"), None)
    check("a real --message replaces the gate subject and keeps its body",
          L.message_body_with_subject("land: Pl/pl_act\n\nledger: closed 1 -> 2\n",
                                    "ef: land fn_800FAE08 (41/41 symbols)"),
          "ef: land fn_800FAE08 (41/41 symbols)\n\nledger: closed 1 -> 2\n")

    # 4d. the gate's own subject row (CLAUDE.md's commit convention). `land_subject` composes the subject
    # every landing is written under, and the row runs `commitlint.py` over it - the tool, never a second copy
    # of the rules. The demonstration uses the REAL tool against a fixture tree (`--root` pins the member set
    # to the fixture, not this checkout), so its verdict is the convention's, not a stub's restatement.
    check("a unit under src/ composes a game/<module> subject",
          L.land_subject(["Network/network_transport"]), "game/network: land Network/network_transport")
    check("subject: a batch of configure.py alone is config/flags, not a game member",
          L.land_subject(["configure.py"]), "config/flags: land configure.py")
    check("a tool path composes the grouping category",
          L.land_subject(["tools/units/land.py"]), "tools/units: land tools/units/land.py")
    check("no units still yields a conventional subject",
          L.land_subject([]), "repo/batch: land a batch")

    with testing.temp_dir() as lint_root:
        for rel in ("tools/units/land.py", "tools/units/stylelint.py", "tools/git/commitlint.py"):
            path = os.path.join(lint_root, *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "w", encoding="utf-8").close()
        cl_tool = os.path.join(L.SELF_REPO, "tools", "git", "commitlint.py")
        check("the real commitlint.py exists for the row to call", os.path.exists(cl_tool), True)

        # a good composed subject passes: `tools/land` is a member now (the script is `tools/units/land.py`)
        good_ok, good_detail = L.subject_lint(lint_root, "tools/land: land tools/units/land.py", tool=cl_tool)
        check("the row PASSES a good composed subject", good_ok, True)
        check("... and a pass carries no detail", good_detail, "")

        # ... and a bad one is refused. `land_subject` reads the first segment after `tools/`, so a unit
        # under an invented grouping composes a category no member matches.
        bad_subject = L.land_subject(["tools/land2/tool.py"])
        check("a unit under an invented grouping composes a bad subject",
              bad_subject, "tools/land2: land tools/land2/tool.py")
        bad_ok, bad_detail = L.subject_lint(lint_root, bad_subject, tool=cl_tool)
        check("the row REFUSES a bad composed subject", bad_ok, False)
        check("... and the refusal carries commitlint's own finding, not a restatement",
              "not a known `tools` member" in bad_detail, True)

        # exit 2 is "nothing checked" - the row treats it as a failure, never a pass
        class _NothingChecked:
            returncode, stdout, stderr = 2, "commitlint: nothing was checked", ""
        stub_ok, stub_detail = L.subject_lint(lint_root, "tools/land: x", tool=cl_tool,
                                            runner=lambda args: _NothingChecked())
        check("the row treats exit 2 (nothing checked) as a failure", stub_ok, False)
        check("... and says the lint did not run", "nothing was checked" in stub_detail, True)

    # the warning that keeps a foreign staged edit visible: `land` leaves it alone and names it
    check("the foreign-index warning names the path",
          L.foreign_warning(["tools/units/langcheck.py"]),
          "WARNING: the index holds 1 path outside this batch - left staged, not committed: "
          "tools/units/langcheck.py")
    check("... and pluralises two", L.foreign_warning(["a", "b"]).count("paths"), 1)


def test_row_details(c):
    """The suite row's detail, tool scratch, the compile gate, registration, drift and the re-measure."""
    check = c.check
    # the gate's "all tool selftests pass" row: `tools/selftest.py --json` is parsed so the refusal NAMES the
    # failing tool - `command_detail`'s head would show the table header, not the reason.
    def _stp(code, stdout):
        return subprocess.CompletedProcess(["selftest"], code, stdout, "")

    check("a green selftest row is silent", L.selftest_detail(_stp(0, "")), "")
    _red = json.dumps({"failures": [{"name": "tools/flags/infer"}], "stale_parks": [],
                       "tree_clean": True})
    check("a red selftest row names the failing tool",
          "tools/flags/infer" in L.selftest_detail(_stp(1, _red)), True)
    _stale = json.dumps({"failures": [], "stale_parks": ["tools/units/wtsafe"], "tree_clean": True})
    check("... and a stale park", "stale park: tools/units/wtsafe" in L.selftest_detail(_stp(1, _stale)), True)
    _dirty = json.dumps({"failures": [], "stale_parks": [], "tree_clean": False,
                         "tree_offenders": ["?? tools/x"]})
    check("... and a selftest that changed the tree",
          "changed the tree" in L.selftest_detail(_stp(1, _dirty)), True)
    check("unparseable output falls back to the head",
          "boom" in L.selftest_detail(_stp(1, "boom")), True)
    # the refusal also carries the failing tool's last lines, and a green run still names its flakes
    _tail = json.dumps({"failures": [{"name": "tools/units/claims", "status": "fail", "returncode": 1,
                                      "tail": "Traceback\nPermissionError: [WinError 5]"}],
                        "flaky": [{"name": "tools/agents/ideas", "first_status": "fail",
                                   "first_failure": "FAIL ids raced"}], "tree_clean": True})
    _blk = L.selftest_tails(_stp(1, _tail))
    check("selftest_tails: the failing tool's last lines", "PermissionError: [WinError 5]" in _blk, True)
    check("... named with its tool and exit", "FAILED: tools/units/claims (fail, exit 1)" in _blk, True)
    check("... and the flake is a WARNING with its first failure",
          "WARNING selftest FLAKY: tools/agents/ideas" in _blk and "FAIL ids raced" in _blk, True)
    check("selftest_tails: nothing for output that is not the JSON", L.selftest_tails(_stp(1, "boom")), "")

    # the 2026-09-25 `d910.json` defect. An objdiff `diff` dump from the repo root is outside the guard's
    # allowed set, so `outside_batch` still classifies it - the tolerance is a deliberate carve-out *at the
    # tolerance site*, never a loosening of the guard - and the carve-out is narrow: the `d`/`t` + digits +
    # `.json` shape in the repo root only. A `d`-shaped name elsewhere, or any other path, is still refused.
    check("scratch: an objdiff dump is scratch", L.is_scratch("d910.json"), True)
    check("scratch: the target-side dump too", L.is_scratch("t910.json"), True)
    check("scratch: a single digit", L.is_scratch("d0.json"), True)
    check("scratch: uppercase is not the measured shape", L.is_scratch("D910.json"), False)
    check("scratch: not a suffix of another name", L.is_scratch("d910.json.bak"), False)
    check("scratch: not a letter-only name", L.is_scratch("dx.json"), False)
    check("scratch: no letter", L.is_scratch("910.json"), False)
    check("scratch: no dot", L.is_scratch("d.json"), False)
    check("scratch: not under a subdirectory", L.is_scratch("src/d910.json"), False)
    check("scratch: a source file is not scratch", L.is_scratch("src/Pl/pl_act.cpp"), False)
    check("scratch: the shared file is not scratch", L.is_scratch("configure.py"), False)
    check("scratch: the guard still classifies it outside the batch",
          L.outside_batch(["d910.json"]), ["d910.json"])
    check("scratch: it is named in the note", "d910.json" in L.scratch_note(["d910.json"]), True)
    check("scratch: the note says it is not a refusal",
          "never a refusal" in L.scratch_note(["d910.json"]), True)
    check("scratch: the note pluralises", L.scratch_note(["d910.json", "t910.json"]).count("paths"), 1)
    check("scratch: the subset keeps order",
          L.scratch_paths(["d910.json", "src/a.c", "t12.json"]), ["d910.json", "t12.json"])
    # the crossing point itself: the gate must never stage a path it did not receive from the batch
    check("scratch: a scratch path is never stageable",
          "d910.json" in L.land_stageable(["Pl/pl_act"], [("??", "d910.json")]), False)
    check("scratch: the batch's own file in the same rows still is",
          L.land_stageable(["Pl/pl_act"], [("??", "d910.json"), (" M", "src/Pl/pl_act.cpp")]),
          ["src/Pl/pl_act.cpp"])
    check("scratch: a staged scratch path is not stageable either",
          "d910.json" in L.land_stageable(["Pl/pl_act"], [("A ", "d910.json")]), False)

    # the compile gate (2026-09-26): `ninja build/RMHE08/ok` is structurally blind to a `NonMatching` unit's
    # object (it is never linked), so a unit that does not compile reached `main` twice in one session with
    # `ok` green. The check is scoped by object target: only a `FAILED:` output that is one of the batch's own
    # `build/RMHE08/src/<unit>.o` targets refuses. A foreign dirty object failing passes and is named.
    check("compile: a unit's object target is build/RMHE08/src/<unit>.o",
          L.compile_targets(["Pl/fn_80273B14"]), ["build/RMHE08/src/Pl/fn_80273B14.o"])
    check("compile: a unit spelled with its extension still yields one target",
          L.compile_targets(["Pl/fn_80273B14.cpp"]), ["build/RMHE08/src/Pl/fn_80273B14.o"])
    check("compile: duplicates collapse", L.compile_targets(["Pl/pl_act", "Pl/pl_act.cpp"]),
          ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: no units yields no targets", L.compile_targets([]), [])
    check("compile: FAILED outputs are read from ninja's stderr", L.failed_compile_outputs(
        "FAILED: build/RMHE08/src/Pl/pl_act.o\nrun 1\nFAILED: build/RMHE08/src/RSO/runtime.o\nrun 2"),
        ["build/RMHE08/src/Pl/pl_act.o", "build/RMHE08/src/RSO/runtime.o"])
    check("compile: a target failed twice is named once", L.failed_compile_outputs(
        "FAILED: build/RMHE08/src/Pl/pl_act.o\nFAILED: build/RMHE08/src/Pl/pl_act.o"),
        ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: a Windows path separator is normalised", L.failed_compile_outputs(
        "FAILED: build\\RMHE08\\src\\Pl\\pl_act.o"), ["build/RMHE08/src/Pl/pl_act.o"])
    check("compile: a batch unit's own failure is the batch's", L.batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/Pl/pl_act.o"), (["Pl/pl_act"], []))
    check("compile: a foreign dirty object's failure is NOT the batch's", L.batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/RSO/runtime.o"),
        ([], ["build/RMHE08/src/RSO/runtime.o"]))
    check("compile: a mixed run names both, refuses on the batch's unit", L.batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/RSO/runtime.o\nFAILED: build/RMHE08/src/Pl/pl_act.o"),
        (["Pl/pl_act"], ["build/RMHE08/src/RSO/runtime.o"]))
    check("compile: a target that only shares a prefix is foreign", L.batch_compile_failures(
        ["Pl/pl_act"], "FAILED: build/RMHE08/src/Pl/pl_act2.o"),
        ([], ["build/RMHE08/src/Pl/pl_act2.o"]))

    class FakeProc:
        """A `subprocess.CompletedProcess`-shaped stand-in for the selftest's fake ninja."""

        def __init__(self, code, out="", err=""):
            self.returncode, self.stdout, self.stderr = code, out, err

    with testing.temp_dir() as tmp:
        open(os.path.join(tmp, "build.ninja"), "w").close()
        calls = []

        def fake_runner(args):
            calls.append(args)
            return FakeProc(1, "", "FAILED: build/RMHE08/src/Pl/pl_act.o\n(10248) does not match")

        ok, detail = L.compile_check(tmp, ["Pl/pl_act"], runner=fake_runner)
        check("compile gate: a batch unit that does not compile is REFUSED", ok, False)
        check("... and the batch unit is named", "Pl/pl_act" in detail, True)
        check("... and it ran one `ninja -k 0`", calls, [["ninja", "-k", "0"]])

        ok, detail = L.compile_check(
            tmp, ["Pl/pl_act"],
            runner=lambda args: FakeProc(1, "FAILED: build/RMHE08/src/RSO/runtime.o\n"))
        check("compile gate: it PASSES when only FOREIGN dirty work is broken", ok, True)
        check("... and the foreign failure is named, not counted", "foreign" in detail, True)
        check("... and it never reads as the batch's", "Pl/pl_act" in detail, False)

        ok, detail = L.compile_check(tmp, ["Pl/pl_act"], runner=lambda args: FakeProc(0))
        check("compile gate: a clean compile passes", ok, True)

        ok, detail = L.compile_check(
            tmp, ["Pl/pl_act"], runner=lambda args: FakeProc(2, "ninja: error: unknown target\n"))
        check("compile gate: an unattributable ninja failure is not read as a pass", ok, False)
        check("... and the reason is in the detail", "unknown target" in detail, True)

    with testing.temp_dir() as no_build:
        ok, detail = L.compile_check(no_build, ["Pl/pl_act"], runner=lambda args: FakeProc(1, "x"))
        check("compile gate: without build.ninja the configure.py gate owns it", ok, True)

    # the registration gate (2026-09-26): a source file committed without its `configure.py` line and
    # `splits.txt` block is registered in name only and absent from the build; `ok` stays green because a
    # `NonMatching` object is never linked, and the compile gate cannot see it because there is no
    # `build/RMHE08/src/<unit>.o` target to scope to. `vu.registration_problems` owns the three axes.
    check("registration: a unit in all three places passes",
          vu.registration_problems(
              ["Pl/pl_act"],
              'Object(NonMatching, "Pl/pl_act.cpp")\n',
              "Sections:\nPl/pl_act.cpp:\n\t\t.text start:0x1 end:0x2\n",
              "build build\\RMHE08\\src\\Pl\\pl_act.o: mwcc_sjis\n"),
          [])
    check("registration: a source-only unit REFUSES",
          vu.registration_problems(["Pl/pl_act"], "", "Sections:\n", "build a.o: rule\n") != [],
          True)
    check("registration: it names all three axes",
          len(vu.registration_problems(["Pl/pl_act"], "", "Sections:\n", "build a.o: rule\n")), 3)
    check("registration: an Object line with no splits block still refuses",
          vu.registration_problems(["Pl/pl_act"],
                                   'Object(NonMatching, "Pl/pl_act.cpp")\n',
                                   "Sections:\n",
                                   "build build\\RMHE08\\src\\Pl\\pl_act.o: mwcc_sjis\n") != [],
          True)
    check("registration: a unit missing from a stale build.ninja still refuses",
          vu.registration_problems(["Pl/pl_act"],
                                   'Object(NonMatching, "Pl/pl_act.cpp")\n',
                                   "Sections:\nPl/pl_act.cpp:\n\t\t.text start:0x1 end:0x2\n",
                                   "") != [],
          True)

    # the split-target drift (the merger's strongest form): target objects come from the DOL split, so a
    # unit the batch does not name whose object moved is a `splits.txt` change that re-ranged a neighbour.
    check("drift: an unchanged tree has no drift",
          vu.target_drift_problems({"a": "1", "b": "2"}, {"a": "1", "b": "2"}, []), [])
    check("drift: the batch's own unit may move",
          vu.target_drift_problems({"a": "1"}, {"a": "2"}, ["a"]), [])
    check("drift: a re-ranged neighbour REFUSES",
          any("re-ranged" in p
              for p in vu.target_drift_problems({"a": "1", "n": "2"}, {"a": "1", "n": "3"}, ["a"])),
          True)

    # the independent per-symbol re-measure: a `fuzzy_match_percent`-absent function is 0%, not 100%, and
    # the unit arithmetic that proves the reading must refuse when the two disagree (SKILL 5.2/5.3).
    check("re-measure: an absent fuzzy key is 0%, so the arithmetic reproduces",
          vu.arithmetic_crosscheck(
              {"total_code": 200, "fuzzy_match_percent": 50.0},
              {"a": {"size": "100", "fuzzy_match_percent": 100.0}, "b": {"size": "100"}})[0],
          True)
    check("re-measure: a unit fuzzy that only reproduces if absent=100 REFUSES",
          vu.arithmetic_crosscheck(
              {"total_code": 200, "fuzzy_match_percent": 100.0},
              {"a": {"size": "100", "fuzzy_match_percent": 100.0}, "b": {"size": "100"}})[0],
          False)
    check("re-measure: a 100% claim whose bytes differ REFUSES",
          any("not identical" in p for p in vu.symbol_problems(
              {"a": {"fuzzy_match_percent": 100.0}}, {"a": {"fuzzy_match_percent": 100.0}},
              {"a": {"target_size": 16, "candidate_size": 16, "in_target": True,
                     "in_candidate": True, "identical": False}})[0]),
          True)
    check("re-measure: a report not reproducible from a fresh generate REFUSES",
          any("not reproducible" in p for p in vu.symbol_problems(
              {"a": {"fuzzy_match_percent": 100.0}}, {"a": {"fuzzy_match_percent": 50.0}},
              {"a": {"target_size": 16, "candidate_size": 16, "in_target": True,
                     "in_candidate": True, "identical": True}})[0]),
          True)


def test_message_file_and_regression(c):
    """The message file, regression_rows and the per-symbol rule."""
    check = c.check
    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, ".git"), exist_ok=True)
        stale = L.write_land_message(tmp, "land: old batch\n")
        check("a green gate writes the message", os.path.exists(stale), True)
        check("a failed gate removes it", L.clear_land_message(tmp), stale)
        check("... and it is gone", os.path.exists(stale), False)
        check("clearing a missing message is a no-op", L.clear_land_message(tmp), None)

    with testing.temp_dir() as tmp:
        # regression_rows() reads a report_changes.json fixture
        fixture = os.path.join(tmp, "changes.json")
        json.dump({"units": [
            {"name": "src/Pl/pl_act.cpp", "measures": {"matched_code": {"old": 100, "new": 90}}},
            {"name": "src/Pl/pl_skill.cpp", "measures": {"matched_code": {"old": 90, "new": 90}}},
            {"name": "main/auto_eft004_set_pl__FP4_PLWUcfffUl_text", "measures": {"matched_code": {"old": 10, "new": 1}}},
        ]}, open(fixture, "w"))
        rows = L.regression_rows(fixture)
        check("a regression is caught", len(rows), 1)
        check("the auto_ scaffold is ignored", any("auto_" in r[0] for r in rows), False)
        check("a registered auto/* unit is still tracked",
              L.regression_rows(fixture) != [] and all("main/auto/" not in r[0] for r in rows), True)
        check("a flat measure is not a regression", any(r[1] == "matched_code" and r[2] == 90 for r in rows), False)
        check("a missing changes file is not a crash", L.regression_rows(os.path.join(tmp, "nope.json")), [])

    # the per-SYMBOL regression rule (2026-09-25). A unit's `fuzzy` is an average over its symbols, so an
    # already-registered unit that is EXTENDED - a widened splits range, a head joined to its tail - falls in
    # average as weaker new bodies join it. The old gate called that a regression and refused two legitimate
    # extensions in one day (g3d_resshp: `no symbol or unit regressed: ... unit fuzzy 99.96 -> 99.26`). The
    # rule is now symbol by symbol: only a symbol the previous report HELD can regress, and a symbol it did
    # not hold is NEW and never refuses a batch.
    def old_unit_avg_regression(before, after):
        """The pre-2026-09-25 rule, kept as the oracle the fix must beat: any unit average or symbol drop."""
        rows = []
        for unit, av in after.items():
            pv = before.get(unit)
            if not pv:
                continue
            af, bf = pv.get("fuzzy"), av.get("fuzzy")
            if isinstance(af, (int, float)) and isinstance(bf, (int, float)) and bf < af - 1e-9:
                rows.append((unit, "unit fuzzy", af, bf))
            for sym, bp in (pv.get("symbols") or {}).items():
                ap = (av.get("symbols") or {}).get(sym)
                if isinstance(ap, (int, float)) and ap < bp - 1e-9:
                    rows.append((unit, sym, bp, ap))
        return rows

    # the shape the gate refused: the 21-symbol head at 99.96 % widened to the 58-function TU at 99.26 %,
    # every head symbol re-measured unchanged and 37 weaker bodies added (fn_8009A1E0 among them at 70.28).
    head = {"main/g3d/g3d_resshp": {"fuzzy": 99.95968, "matched_code": 964,
                                    "symbols": {"fn_80099724": 98.5714}}}
    extended = {"main/g3d/g3d_resshp": {"fuzzy": 99.26, "matched_code": 2108,
                                        "symbols": {"fn_80099724": 98.5714, "fn_8009A1E0": 70.28,
                                                    "fn_8009A244": 55.0, "fn_8009A2D0": 31.5}}}
    check("a unit average that fell only because it grew is not a regression",
          L.report_regressions(head, extended, []), ([], []))
    check("... but the old unit-average rule did fire on it",
          old_unit_avg_regression(head, extended) != [], True)
    check("... and unit_grew reads the new symbols as growth",
          L.unit_grew(head["main/g3d/g3d_resshp"], extended["main/g3d/g3d_resshp"]), True)

    # an existing symbol's own score dropping must still refuse, naming the symbol and both numbers
    regressed = {"main/g3d/g3d_resshp": {"fuzzy": 98.5, "matched_code": 2108,
                                         "symbols": {"fn_80099724": 91.25, "fn_8009A1E0": 70.28}}}
    check("an existing symbol that dropped is a regression",
          L.report_regressions(extended, regressed, []),
          ([("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 91.25)], []))
    check("... and the new symbol beside it is never named",
          L.report_regressions(extended, regressed, [])[0][0][1], "fn_80099724")
    # growth plus a real drop is still a refusal: the symbol row wins, the average row does not double it
    grew_and_dropped = {"main/g3d/g3d_resshp": {"fuzzy": 95.0, "matched_code": 2400,
                                                "symbols": {"fn_80099724": 90.0, "fn_NEW": 20.0}}}
    check("an extension that ALSO regressed a held symbol still refuses, once, on the symbol",
          L.report_regressions(extended, grew_and_dropped, []),
          ([("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 90.0)], []))

    # a unit that did NOT grow and whose average fell (matched bytes lost, no held symbol dropped) still
    # refuses at the unit level - the one place a per-symbol row cannot name the loss
    shrunk = {"main/g3d/g3d_resshp": {"fuzzy": 90.0, "matched_code": 900,
                                      "symbols": {"fn_80099724": 98.5714}}}
    check("a unit that shrank and lost its average still refuses",
          L.report_regressions(head, shrunk, []),
          ([("main/g3d/g3d_resshp", "unit fuzzy", 99.95968, 90.0)], []))
    check("... and unit_grew is false for it",
          L.unit_grew(head["main/g3d/g3d_resshp"], shrunk["main/g3d/g3d_resshp"]), False)

    # the happy paths: unchanged measurements, and a unit the previous report never held, are not regressions
    check("an unchanged unit is not a regression", L.report_regressions(head, head, []), ([], []))
    check("a brand-new unit is not a regression", L.report_regressions({}, extended, []), ([], []))
    check("an empty snapshot on both sides is not a regression", L.report_regressions({}, {}, []), ([], []))

    # --allow-regression still authorises exactly the unit it names, and nothing else
    check("an authorised drop leaves the unauthorised list empty",
          L.report_regressions(extended, regressed, ["g3d_resshp"])[0], [])
    check("... and is reported as authorised",
          L.report_regressions(extended, regressed, ["g3d_resshp"])[1],
          [("main/g3d/g3d_resshp", "fn_80099724", 98.5714, 91.25)])
    check("an allowance that does not name the unit does not authorise",
          len(L.report_regressions(extended, regressed, ["SomeOther/file.cpp"])[0]), 1)
    check("... and nothing is authorised by it",
          L.report_regressions(extended, regressed, ["SomeOther/file.cpp"])[1], [])


def test_pathspec_commit(c):
    """The pathspec commit leaves another stream's staged edit alone."""
    check = c.check
    # `land` commits with a pathspec. `git commit` with none takes the whole index, which is how another
    # stream's staged `tools/units/langcheck.py` landed under two unrelated unit commits on 2026-09-23
    # (`85f3d4b5`, `d50fdd32`). The proof is a real repo, run through the real `land_stageable` and the real
    # commit helper: the batch's paths are committed, the foreign edit is not, and it is still staged
    # afterwards. A rename's deletion is part of the batch, so both of its paths go in the pathspec.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        for name in ("src/batch.c", "src/old.c", "tools/units/langcheck.py"):
            os.makedirs(os.path.dirname(os.path.join(tmp, name)), exist_ok=True)
            with open(os.path.join(tmp, name), "w", encoding="utf-8") as fh:
                fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src/batch.c"), "w", encoding="utf-8") as fh:
            fh.write("the batch\n")
        repo_git(tmp, "mv", "src/old.c", "src/new.c")   # a rename: its deletion belongs to the batch too
        with open(os.path.join(tmp, "tools/units/langcheck.py"), "w", encoding="utf-8") as fh:
            fh.write("another stream\n")
        repo_git(tmp, "add", "tools/units/langcheck.py")  # foreign, staged by another worker
        rows = L.changed_status(tmp)
        check("a rename reports both of its paths",
              ("src/old.c" in [p for _c, p in rows] and "src/new.c" in [p for _c, p in rows]), True)
        stageable = L.land_stageable(["Pl/pl_act"], rows)
        check("the tracked tool edit is not part of the batch", "tools/units/langcheck.py" in stageable, False)
        check("the foreign staged edit is named", L.staged_elsewhere(tmp, stageable), ["tools/units/langcheck.py"])
        L.stage_batch(tmp, stageable)
        msg_file = os.path.join(tmp, "msg.txt")
        with open(msg_file, "w", encoding="utf-8") as fh:
            fh.write("land: the batch\n")
        p = L.commit_pathspec(tmp, msg_file, stageable)
        check("the pathspec commit succeeds", p.returncode, 0)
        check("the batch's file is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("the rename's new path is committed",
              L.run(["git", "cat-file", "-e", "HEAD:src/new.c"], tmp).returncode, 0)
        check("the rename's deletion is committed",
              L.run(["git", "cat-file", "-e", "HEAD:src/old.c"], tmp).returncode != 0, True)
        check("the foreign edit is NOT committed",
              repo_git(tmp, "show", "HEAD:tools/units/langcheck.py"), "base")
        check("... and is still staged", repo_git(tmp, "diff", "--cached", "--name-only"),
              "tools/units/langcheck.py")
        check("... and still uncommitted", repo_git(tmp, "status", "--porcelain").startswith("M"), True)


def test_rule_rows(c):
    """Row 8's deletion, rule 10 add-only, the data-closure allowance, rule 12."""
    check = c.check
    # gate row 8 (the `rule 7 deferred` escape's growth) is deleted (2026-10-05): rule 7 is the lint's (row 5)
    check("rule7: no gate row judges the `rule 7 deferred` escape any more",
          (hasattr(L, "rule7_defer_growth"), hasattr(L, "rule7_row"),
           any(getattr(r, "__name__", "") == "rule7_row" for r in L.PRE_BUILD)), (False, False, False))

    # --- rule 10: the row is ADD-only, like the lint's `--diff` ------------------------------------
    # The keys are `vtableaudit.violation_rows`'s rename-stable shape: a run by its range (an address is
    # unique in the DOL and a re-home keeps it), a `ref:` by the path the tree now spells.
    existing = {"run:.data:805D4D38": {"unit": "ai/fn_802CC794",
                                       "where": "ai/fn_802CC794.cpp .data"},
                "ref:src/old/unit.cpp:9:OldVTable": {"unit": "old/unit",
                                                      "where": "src/old/unit.cpp:9 assigns OldVTable"}}
    same = dict(existing, **{"ref:src/new/unit.cpp:3:NewVTable":
                             {"unit": "new/unit", "where": "src/new/unit.cpp:3 assigns NewVTable"}})
    check("rule10: an unchanged set (existing violations grandfathered) adds nothing",
          L.rule10_growth(existing, existing, ["new/unit"]), ([], []))
    added, touched = L.rule10_growth(existing, same, ["new/unit"])
    check("rule10: a batch that ADDS a violation is refused",
          added, ["ref:src/new/unit.cpp:3:NewVTable"])
    check("... and the row prints the batch units' violations even when it passes",
          touched, ["src/new/unit.cpp:3 assigns NewVTable"])
    check("rule10: a violation that disappears is not an addition",
          L.rule10_growth(same, existing, ["new/unit"]), ([], []))
    check("rule10: a batch touching a file that already has one passes",
          L.rule10_growth(existing, existing, ["ai/fn_802CC794"]),
          ([], ["ai/fn_802CC794.cpp .data"]))

    # --- rule 10 identity pairing (2026-10-05): the row decides with `vtableaudit.diff_rows`, so a `run:` key that
    # moved because the run's first word stopped resolving is SHIFTED, never added; a real new run still refuses.
    def run_row(addr, words, unit="sound/sound_obj", section=".data"):
        return {"unit": unit, "kind": "run", "section": section, "address": addr, "words": words,
                "where": "%s %s 0x%08X" % (unit, section, addr)}
    base_runs = {"run:.data:80598038": run_row(0x80598038, 6),
                 "ref:src/sound/sound_obj.cpp:767:lbl_80597DF8": {"unit": "sound/sound_obj", "kind": "ref",
                                                                 "where": "x"}}
    moved = {"run:.data:8059803C": run_row(0x8059803C, 5),
             "ref:src/sound/sound_obj.cpp:767:lbl_80597DF8": base_runs["ref:src/sound/sound_obj.cpp:767:lbl_80597DF8"]}
    check("rule10 pairing: a run whose start moved by a word inside the old run is SHIFTED, not added",
          L.rule10_growth(base_runs, moved, [])[0], [])
    fresh = dict(base_runs, **{"run:.data:805B2328": run_row(0x805B2328, 4)})
    check("rule10 pairing: a genuinely new run (no removed run overlaps it) is still refused",
          L.rule10_growth(base_runs, fresh, [])[0], ["run:.data:805B2328"])
    grown = {"run:.data:805FB808": run_row(0x805FB808, 630)}
    check("rule10 pairing: a run that GREW over a removed run (the 850127ccb recut: 630 words over 7) is an addition",
          L.rule10_growth({"run:.data:805FBD68": run_row(0x805FBD68, 7)}, grown, [])[0], ["run:.data:805FB808"])
    beside = dict(base_runs, **{"run:.data:8059803C": run_row(0x8059803C, 5)})
    check("rule10 pairing: a new run overlapping a run that is STILL there is an addition (only a removed run pairs)",
          L.rule10_growth(base_runs, beside, [])[0], ["run:.data:8059803C"])
    other_sec = {"run:.rodata:8059803C": run_row(0x8059803C, 5, section=".rodata")}
    check("rule10 pairing: an overlap in another section does not pair",
          L.rule10_growth({"run:.data:80598038": run_row(0x80598038, 6)}, other_sec, [])[0],
          ["run:.rodata:8059803C"])
    renum = {"run:.data:80598038": run_row(0x80598038, 6),
             "ref:src/sound/sound_obj.cpp:770:lbl_80597DF8": {"unit": "sound/sound_obj", "kind": "ref", "where": "y"}}
    check("rule10 pairing: `ref:` keys stay a set difference (a moved line is an addition, as before)",
          L.rule10_growth(base_runs, renum, [])[0], ["ref:src/sound/sound_obj.cpp:770:lbl_80597DF8"])
    import tools.units.vtableaudit as _vta
    with mock.patch.object(_vta, "sweep", lambda main, text_ref=None: {}), \
            mock.patch.object(_vta, "violation_rows", lambda s, rename=None: {"run:.data:80598038": run_row(0x80598038, 6,
                                                                                                    unit="sound/x.cpp")}):
        snap = L.rule10_violations(".")
    check("rule10 pairing: the gate's snapshot keeps the span `diff_rows` pairs on (section, address, words)",
          {k: (v["unit"], v.get("section"), v.get("address"), v.get("words")) for k, v in snap.items()},
          {"run:.data:80598038": ("sound/x", ".data", 0x80598038, 6)})
    two_new = dict(moved, **{"run:.data:80598050": run_row(0x80598050, 2)})
    check("rule10 pairing: one removed run pairs with ONE added run - the second overlapping addition still refuses",
          L.rule10_growth(base_runs, two_new, [])[0], ["run:.data:80598050"])

    # --- the data-closure row: ADD-only over (unit, orphan address) pairs; datagap's own selftest has the
    # end-to-end fixtures (real objects), this pins the allowance plumbing and the row's decision shape.
    rec = {"unit": "new/unit", "name": "lbl_80500040", "section": ".rodata", "address": 0x80500040, "sites": 2}
    key = dg.orphan_key("new/unit", ".rodata", 0x80500040)
    check("orphan: a new (unit, orphan) pair is refused",
          len(dg.orphan_verdict([], {key: rec}, [])["added"]), 1)
    check("orphan: the same pair at the base is pre-existing debt, reported not refused",
          dg.orphan_verdict([key], {key: rec}, [])["added"], [])
    L.set_allow_orphan([" 0x80500040 ", "", None])
    check("orphan: set_allow_orphan trims and drops blanks", L.state.ALLOW_ORPHAN, ["0x80500040"])
    check("orphan: a recorded allowance excuses exactly that address",
          dg.orphan_verdict([], {key: rec}, [], L.state.ALLOW_ORPHAN)["added"], [])
    L.set_allow_orphan(["0x80500044"])
    check("orphan: an allowance that matches nothing keeps the refusal",
          len(dg.orphan_verdict([], {key: rec}, [], L.state.ALLOW_ORPHAN)["added"]), 1)
    L.set_allow_orphan([])
    check("orphan: a shrunk claim is a refusal with no pair involved",
          len(dg.orphan_verdict([], {}, [(".rodata", 0x80500008, 0x8050000C)])["added"]), 1)

    # --- rule 12: the style-lint row's allowance, applied to stylelint's own --diff JSON -----------
    # Rule 12 is refused inside the style lint, so `--allow-rule12 <token>` is applied to that row's
    # `added`/`detail` delta. The token is the at-fault symbol name `--list-added` prints.
    added12 = [{"rule": 12, "file": "src/Pl/pl_act_step.cpp", "added": 1, "before": 0, "after": 1}]
    detail12 = [{"rule": 12, "file": "src/Pl/pl_act_step.cpp", "line": 9,
                 "token": "pl_frame_window_44", "detail": "unowned data"}]
    check("rule12: an addition with no allowance is refused",
          L.rule12_verdict(added12, detail12, []), (False, [], ["pl_frame_window_44"], []))
    check("rule12: the named token is excused",
          L.rule12_verdict(added12, detail12, ["pl_frame_window_44"]),
          (True, ["pl_frame_window_44"], [], []))
    check("rule12: an allowance that matches nothing keeps the refusal",
          L.rule12_verdict(added12, detail12, ["some_other_symbol"]),
          (False, [], ["pl_frame_window_44"], []))
    check("rule12: a count with fewer named tokens refuses",
          L.rule12_verdict([{"rule": 12, "file": "a.cpp", "added": 2, "before": 0, "after": 2}],
                                detail12, ["pl_frame_window_44"])[0], False)
    check("rule12: another rule's addition is never excusable",
          L.rule12_verdict([{"rule": 2, "file": "a.cpp", "added": 1, "before": 0, "after": 1}],
                                detail12, ["pl_frame_window_44"])[0], False)
    check("rule12: a clean delta has nothing to excuse", L.rule12_verdict([], [], []),
          (True, [], [], []))
    L.set_allow_rule12([" pl_frame_window_44 ", "", None])
    check("rule12: set_allow_rule12 trims and drops blanks", L.state.ALLOW_RULE12,
          ["pl_frame_window_44"])
    L.set_allow_rule12([])


def test_land_flow_refusals(c):
    """The real `land` under a stand-in gate: pre-flight, scratch, foreign paths, named and kinded refusals."""
    check = c.check
    with testing.temp_dir() as tmp:
        # the pre-flight: a foreign path ALREADY in the tree (not during the build) is named - with a likely
        # cause when it looks like lane scratch - and refused BEFORE the expensive gate runs.
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "batch.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        os.makedirs(os.path.join(tmp, ".tmp-mwcc", "upstream"), exist_ok=True)
        with open(os.path.join(tmp, ".tmp-mwcc", "upstream", "x.c"), "w", encoding="utf-8") as fh:
            fh.write("a clone inside MAIN\n")
        called = []

        def recording_verify(*_a, **_k):
            called.append(True)
            return 0

        buf, err = io.StringIO(), io.StringIO()
        with mock.patch.object(L.gate, "verify", recording_verify), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(err):
            code = L.land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x")
        check("preflight: a foreign path before the build refuses early", code, 1)
        check("... and the expensive gate never ran", called, [])
        check("... the answer line is a REFUSED", buf.getvalue().startswith("REFUSED"), True)
        check("... the pre-flight names the planted path", ".tmp-mwcc/upstream/x.c" in err.getvalue(), True)
        check("... and the likely cause (a lane launched in MAIN)", "cwd set to MAIN" in err.getvalue(), True)
        check("... and nothing was committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "base")

    with testing.temp_dir() as tmp:
        # (a) scratch appears during the build: the batch lands, the scratch is never staged or committed, and
        # the caller's dump is left in the tree - the tolerance is named, not silent
        code, out, err, _base = land_fixture(tmp, fake_verify_with(write_scratch))
        check("scratch during the build: the landing is not refused", code, 0)
        check("... the answer line says LANDED", L.answer_line(out).startswith("LANDED"), True)
        check("... the batch is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("... the scratch is NOT committed",
              L.run(["git", "cat-file", "-e", "HEAD:d910.json"], tmp).returncode != 0, True)
        check("... nothing is left staged", repo_git(tmp, "diff", "--cached", "--name-only"), "")
        check("... and the caller's dump is left alone",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "during the build\n")
        check("... the gate log names it", "d910.json" in err, True)
        check("... and says the tolerance is not a refusal", "never a refusal" in err, True)

    with testing.temp_dir() as tmp:
        # (b) the index already holds the scratch (the landing flow's `git add -A` did it): `land` de-indexes
        # it, so the commit can never carry a path the batch did not receive
        code, out, err, _base = land_fixture(tmp, fake_verify_with(lambda main: None), stage_scratch=True)
        check("a staged scratch does not block the landing", code, 0)
        check("... the answer line says LANDED", L.answer_line(out).startswith("LANDED"), True)
        check("... the batch is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("... the staged scratch is de-indexed and never committed",
              L.run(["git", "cat-file", "-e", "HEAD:d910.json"], tmp).returncode != 0, True)
        check("... and the index is left otherwise empty",
              repo_git(tmp, "diff", "--cached", "--name-only"), "")
        check("... the file itself survives",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "staged by another step\n")
        check("... and the de-indexing is named", "removed from the index" in err, True)

    with testing.temp_dir() as tmp:
        # a `git reset` that fails must be REPORTED, never claimed as removed: the guarantee is "the batch can
        # never carry a path it did not receive", and a silent failure would leave that guarantee a fiction
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        with open(os.path.join(tmp, "d910.json"), "w", encoding="utf-8") as fh:
            fh.write("scratch\n")
        repo_git(tmp, "add", "-A")
        real_run = L.run

        def failing_reset(args, cwd):
            if args[:2] == ["git", "reset"]:
                return subprocess.CompletedProcess(args, 1, "", "index.lock exists")
            return real_run(args, cwd)

        with mock.patch.object(L.tree, "run", failing_reset):
            note = L.tolerate_scratch(tmp, ["d910.json"])
        check("a failed unstage warns", "WARNING" in note, True)
        check("... and names the path", "d910.json" in note, True)
        check("... and does not claim it was removed", "removed from the index" in note, False)
        check("... the path really is still staged",
              repo_git(tmp, "diff", "--cached", "--name-only"), "d910.json")
        # and the successful path: `git reset` drops the entry and leaves the file in the tree
        check("a successful unstage reports what it removed",
              L.tolerate_scratch(tmp, ["d910.json"]), L.scratch_note(["d910.json"])
              + " (a staged copy was removed from the index)")
        check("... and the file survives",
              open(os.path.join(tmp, "d910.json"), encoding="utf-8").read(), "scratch\n")
        check("... and is no longer staged", repo_git(tmp, "diff", "--cached", "--name-only"), "")

    with testing.temp_dir() as tmp:
        # (c) a foreign path outside the batch that appears during the build is refused loudly - the tolerance is
        # tool scratch only, and it does not swallow the refusal when scratch appears beside it
        code, out, _err, base_sha = land_fixture(tmp, fake_verify_with(write_foreign_root))
        check("a foreign root file during the build is refused", code, 1)
        check("... the refusal names it", "appeared during the build: NOTES.md" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
        check("... and its message is cleared", os.path.exists(L.land_message_path(tmp)), False)

    with testing.temp_dir() as tmp:
        code, out, _err, base_sha = land_fixture(tmp, fake_verify_with(write_foreign))
        check("a foreign ground-truth edit is refused", code, 1)
        check("... the refusal names it",
              "appeared during the build: config/RMHE08/build.sha1" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)

    with testing.temp_dir() as tmp:
        code, out, err, base_sha = land_fixture(tmp, fake_verify_with(write_both))
        check("foreign + scratch together is still refused", code, 1)
        check("... and the foreign path is the one named",
              "appeared during the build: config/RMHE08/build.sha1" in out, True)
        check("... the refused batch is not committed", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
        check("... and the tolerated scratch is still named", "d910.json" in err, True)

    with testing.temp_dir() as tmp:
        # (d) a failing gate NAMES itself: the 2026-09-25 `proposal/800916FC` refusal printed only "the gate
        # failed - nothing staged or committed" while its lint row showed stylelint's trailing legend, so the
        # reader hunted for a defect that was not there. Both halves are asserted here - the check's name, what
        # it printed, and that a passing gate lands.
        stylelint_row = "style lint (§6.5) adds no violation"
        failing = [("ground truth (build.sha1 == the DOL's hash)", True, "", ""),
                   (stylelint_row, False,
                    "exit 1: the batch adds 3 section 6.5 violation(s) over 2 changed file(s):; "
                    "+3 rule 2  src/ef/eft029.cpp  (0 -> 3)", "")]
        named = L.failing_checks(failing)
        check("a failing gate produces one line per failed check", len(named), 1)
        check("... it names the check", named[0].startswith(stylelint_row + " [GATE]:"), True)
        check("... and what the check printed", "+3 rule 2 src/ef/eft029.cpp" in named[0], True)
        check("... a passing check is not reported", "ground truth" in named[0], False)
        check("... a gate failure carries its KIND", "[GATE]" in named[0], True)
        check("... and its remedy", "remedy:" in named[0], True)
        green = [("ground truth (build.sha1 == the DOL's hash)", True, "", "")]
        check("an all-green gate reports nothing", L.failing_checks(green), [])
        check("a bare failed gate keeps the old wording",
              L.land_decision(False, ["src/Pl/pl_act.cpp"]),
              ("refuse", "the gate failed - nothing staged or committed"))
        named_reason = L.land_decision(False, ["src/Pl/pl_act.cpp"], named,
                                     L.kinds_from_failures(named))[1]
        check("a failed gate refuses and names the check",
              L.land_decision(False, ["src/Pl/pl_act.cpp"], named)[0], "refuse")
        check("... the refusal carries the check's name", stylelint_row in named_reason, True)
        check("... and what it printed", "+3 rule 2" in named_reason, True)
        check("... and names the kind it failed as", "the gate failed" in named_reason, True)
        check("... and the per-check line carries [GATE]", "[GATE]" in named_reason, True)
        check("kinds_from_failures reads the gate tag", L.kinds_from_failures(named), {L.KIND_GATE})
        check("a full set of passing gates lands",
              L.land_decision(True, ["src/Pl/pl_act.cpp"], L.failing_checks(green)), ("commit", ""))
        code, out, _err, _base = land_fixture(tmp, fake_verify_with(lambda main: None, gate_code=1,
                                                                  gate_problems=named))
        check("a red gate never reaches git commit", code, 1)
        check("... the answer line is a refusal", out.startswith("REFUSED"), True)
        check("... and it names the failing check", stylelint_row in out, True)
        check("... and what the check printed", "+3 rule 2" in out, True)
        check("... and the batch file is still uncommitted",
              repo_git(tmp, "status", "--porcelain").startswith("M"), True)

    # a BOOKKEEPING failure (the batch is fine, the landing's own state is stale) must read differently:
    # its remedy is printed, and the refusal never says "the gate failed", so a reader does not treat a
    # released branch or a stale base as a defective batch (2026-09-26 case (a)).
    bk_row = ("every unit's branch carries its work as commits", False,
              "no commits of its own on the branch", "", L.KIND_BOOKKEEPING,
              "restore the branch from refs/rescue/<slug>, then re-run")
    bk_named = L.failing_checks([bk_row])
    check("a bookkeeping failure names its KIND", "[BOOKKEEPING]" in bk_named[0], True)
    check("... and prints its remedy", "restore the branch from refs/rescue" in bk_named[0], True)
    check("... and does not call the batch bad", "the batch itself is bad" in bk_named[0], False)
    check("kinds_from_failures reads the bookkeeping tag", L.kinds_from_failures(bk_named), {L.KIND_BOOKKEEPING})
    check("... and both tags together", L.kinds_from_failures(named + bk_named), {L.KIND_GATE, L.KIND_BOOKKEEPING})
    bk_reason = L.land_decision(False, ["src/Pl/pl_act.cpp"], bk_named,
                              L.kinds_from_failures(bk_named))[1]
    check("a bookkeeping refusal still refuses", bk_reason.startswith("BOOKKEEPING refusal"), True)
    check("... says the batch itself passed", "the batch itself passed" in bk_reason, True)
    check("... never says the gate failed", "the gate failed" in bk_reason, False)
    check("... and names the failing check and its remedy",
          "every unit's branch carries its work as commits" in bk_reason and "remedy:" in bk_reason, True)
    mixed_reason = L.land_decision(False, ["src/Pl/pl_act.cpp"], named + bk_named,
                                 L.kinds_from_failures(named + bk_named))[1]
    check("a mixed refusal names both kinds",
          "GATE" in mixed_reason and "BOOKKEEPING" in mixed_reason, True)
    check("failure_summary names the gate kind for a gate-only failure",
          "(GATE:" in L.failure_summary(failing), True)
    check("... and never says 'the gate failed' for a bookkeeping-only failure",
          "the gate failed" in L.failure_summary([bk_row]), False)
    check("... and a bookkeeping-only summary says the batch passed",
          "the batch itself passed" in L.failure_summary([bk_row]), True)


def test_rescued_branch_and_already_applied(c):
    """Cases (a) and (b) end to end."""
    check = c.check
    # case (a) end to end: a `--force` release deleted `worker/<label>` and parked its only copy of the work
    # at refs/rescue/<slug>. The gate cares that the work exists as commits, so it restores the branch from
    # the rescue ref itself (`restore_rescued_branch`) instead of refusing (2026-09-26). A real temp repo,
    # because the whole point is git reachability, then the real `verify` on top.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src", "Pl"), exist_ok=True)
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, ".gitignore"), "w", encoding="utf-8") as fh:
            fh.write(".pi/\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        repo_git(tmp, "branch", branch)
        repo_git(tmp, "checkout", "-q", branch)
        with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "w", encoding="utf-8") as fh:
            fh.write("the worker's work\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "the worker's own work")
        rescue = claims.rescue_ref_name(unit)
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", branch))
        repo_git(tmp, "checkout", "-q", "main")
        repo_git(tmp, "branch", "-D", branch)          # the `release --force` shape
        check("case (a): the released branch is really gone", claims.branch_exists(tmp, branch), False)
        check("case (a): its work is preserved at the rescue ref", claims.rescue_exists(tmp, unit), rescue)
        check("case (a): the rescue ref is accepted as the branch's work", L.branch_problems(tmp, [unit]), [])
        restored = L.restore_rescued_branch(tmp, unit)
        check("case (a): the landing path restores the branch from the rescue ref", restored, branch)
        check("... the branch exists again", claims.branch_exists(tmp, branch), True)
        check("... and carries its work as commits", L.branch_commits(tmp, unit) > 0, True)
        check("... so branch_problems is still empty", L.branch_problems(tmp, [unit]), [])
        check("restoring an existing branch is a no-op", L.restore_rescued_branch(tmp, unit), None)
        # the real `verify`, dry-run: it restores the branch during its own check and reports the gate green
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        with open(claims.outbox_path(tmp, unit), "w", encoding="utf-8") as fh:
            json.dump({"unit": unit, "worker": "a", "finished_at": "2026-01-01T00:00:00",
                       "unit_percent": 50.0, "symbols": [{"name": "fn_1", "percent": 50.0}],
                       "residual": "none", "measured_with": "recompile.py", "config_requests": [],
                       "flags_probed": [], "blockers": []}, fh)
        repo_git(tmp, "branch", "-D", branch)           # back to the released shape for the verify run
        check("case (a): the branch is gone again", claims.branch_exists(tmp, branch), False)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = L.verify(tmp, [unit], repo_git(tmp, "rev-parse", "HEAD"), dry_run=True, no_build=True)
        check("case (a): the real verify passes on the rescued branch", code, 0)
        check("... and --dry-run touched nothing (no branch created)",
              claims.branch_exists(tmp, branch), False)
        # the out-parameter plumbing that the classification depends on: before the fix, reusing the
        # `problems` name for the outbox results rebound it locally and NO failed check reached the caller's
        # list (so `land`'s refusal could not name anything). A real verify must populate it.
        problems = []
        repo_git(tmp, "update-ref", "-d", rescue)      # no rescue ref: the branch is genuinely gone
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            code = L.verify(tmp, [unit], repo_git(tmp, "rev-parse", "HEAD"), dry_run=True, no_build=True,
                          check_outbox=True, problems=problems)
        check("case (a): a failed check reaches verify's problems out-param", code, 1)
        check("... and it carries the BOOKKEEPING tag",
              any("[BOOKKEEPING]" in p for p in problems), True)
        check("... naming the branch check",
              any(p.startswith("every unit's branch carries its work as commits") for p in problems), True)
        # the outbox row and `land --branch`: a pilot/worktree branch has no per-unit outbox and is not asked
        # for one; a `worker/*` claim branch with no outbox is WARNED, never refused (2026-10-05: the outbox is a
        # bookkeeping record; the branch-commits row stays the "the work exists" refusal)
        os.remove(claims.outbox_path(tmp, unit))
        outbox_rows, outbox_warned, codes = {}, {}, {}
        for name in ("pilot/net-l3", "worker/pl-act-zz99"):
            repo_git(tmp, "checkout", "-q", "-b", name)
            with open(os.path.join(tmp, "src", "Pl", "pl_act.c"), "a", encoding="utf-8") as fh:
                fh.write(name + "\n")
            repo_git(tmp, "commit", "-q", "-am", name)
            repo_git(tmp, "checkout", "-q", "main")
            seen, warned = [], []
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                codes[name] = L.verify(tmp, [unit], repo_git(tmp, "rev-parse", "HEAD"), dry_run=True, no_build=True,
                                       problems=seen, branch=name, warnings=warned)
            outbox_rows[name] = any(p.startswith("every unit's outbox validates") for p in seen)
            outbox_warned[name] = any(w.startswith("every unit's outbox validates: ") for w in warned)
        check("outbox: a non-claim branch with no outbox passes the row and warns nothing",
              (outbox_rows["pilot/net-l3"], outbox_warned["pilot/net-l3"]), (False, False))
        check("outbox: a worker/* branch with no outbox is WARNED (the warnings out-param), not refused",
              (outbox_rows["worker/pl-act-zz99"], outbox_warned["worker/pl-act-zz99"], codes["worker/pl-act-zz99"]),
              (False, True, 0))
        check("outbox: the skip note names the branch", "pilot/x" in (L.outbox_skip_note("pilot/x") or ""), True)
        check("outbox: no --branch is strict", L.outbox_skip_note(None), None)

    # case (b) end to end: the batch was applied to the working tree *before* `record-base` ran, so the base's
    # dirty snapshot recorded the batch's own edits as foreign and `land_stageable` had nothing to stage. The
    # old `land` refused "the gate passed but no batch path is stageable" straight after "READY: every check
    # passed" and the batch was committed by hand. The new `land` names the ordering, and --already-applied
    # stages the paths instead (2026-09-26).
    check("case (b): an applied-before-base batch is detected",
          L.looks_already_applied([(" M", "src/batch.c")], {"src/batch.c"}), True)
    check("case (b): a clean batch is not", L.looks_already_applied([(" M", "src/batch.c")], set()), False)
    check("case (b): a batch with no changed path is not", L.looks_already_applied([], {"src/batch.c"}), False)
    check("case (b): a path outside the batch is not the signature",
          L.looks_already_applied([("??", "orig/RMHE08/sys/main.dol")], {"orig/RMHE08/sys/main.dol"}), False)


    with testing.temp_dir() as tmp:
        base_sha = already_applied_repo(tmp)
        check("case (b): record-base snapshotted the applied path as dirty",
              "src/Pl/pl_act.c" in L.base_dirty_paths(tmp), True)
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", fake_verify_with(lambda main: None)), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = L.land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x")
        check("case (b): without --already-applied land refuses", code, 1)
        check("... and says plainly the batch is already applied", "already applied" in buf.getvalue(), True)
        check("... and never commits", repo_git(tmp, "rev-parse", "HEAD"), base_sha)
    with testing.temp_dir() as tmp:
        base_sha = already_applied_repo(tmp)
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", fake_verify_with(lambda main: None)), \
                contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
            code = L.land(tmp, ["Pl/pl_act"], None, no_build=True, check_outbox=False,
                        release_claims=False, subject="x", already_applied=True)
        check("case (b): --already-applied lands the batch (no manual commit)", code, 0)
        check("... the answer line says LANDED", L.answer_line(buf.getvalue()).startswith("LANDED"), True)
        check("... the batch file is committed", repo_git(tmp, "show", "HEAD:src/Pl/pl_act.c"),
              "the batch, applied before record-base")
        check("... and the base commit is the commit before it", repo_git(tmp, "rev-parse", "HEAD~1"), base_sha)


def test_command_detail(c):
    """The head of a linter's output, the FAILED target of ninja's."""
    check = c.check
    # `command_detail` is the detail a FAILED lint row carries: stylelint prints its findings FIRST and its
    # "not enforced: ..." legend LAST, so the old `output[-300:]` showed the legend and hid the violation.
    stylelint_out = ("stylelint: the batch adds 3 section 6.5 violation(s) over 2 changed file(s):\n"
                     "  +3 rule 2  src/ef/eft029.cpp  (0 -> 3)\n"
                     "  not enforced: rule 7 under src/auto/ (temporary grandfather: legacy scaffolding\n"
                     "       with bodies, until the auto/ migration lands)\n"
                     "  not enforced: rule 7 for a file with no bodies yet (a stub has nothing to name)\n")
    detail = L.command_detail(subprocess.CompletedProcess([], 1, stdout=stylelint_out, stderr=""))
    check("a failed lint row carries the violation", "+3 rule 2  src/ef/eft029.cpp" in detail, True)
    check("... and its exit code", "exit 1" in detail, True)
    check("... not just the legend", "temporary grandfather" in detail, False)
    check("a clean lint row says so",
          L.command_detail(subprocess.CompletedProcess([], 0, stdout="ok\n", stderr="")), "exit 0: ok")
    check("an empty output is not a crash",
          L.command_detail(subprocess.CompletedProcess([], 3, stdout="", stderr="")), "no output (exit 3)")

    # A failing `ninja` reported only `ninja: build stopped: subcommand failed.` for a whole session of gate
    # runs (2026-09-26, `ef/fn_8030681C` refused four times). Its head is `[N/M]` progress and its tail is
    # that summary, so neither end names the reason - the `FAILED: <target>` line in the middle does. The
    # detail must therefore prefer the failed outputs over either end of the output.
    ninja_out = ("[91/240] cxx src/ef/fn_8030681C.cpp\n"
                 "FAILED: build/RMHE08/src/ef/fn_8030681C.o\n"
                 "src/ef/fn_8030681C.cpp(12): error: identifier \"EftWork\" is undefined\n"
                 "ninja: build stopped: subcommand failed.\n")
    ninja_detail = L.command_detail(subprocess.CompletedProcess([], 1, stdout=ninja_out, stderr=""))
    check("a failed ninja row names the FAILED target",
          "FAILED: build/RMHE08/src/ef/fn_8030681C.o" in ninja_detail, True)
    check("... not the progress line", "[91/240]" in ninja_detail, False)
    check("... and not just 'build stopped'", ninja_detail.rstrip().endswith("subcommand failed."), False)


def test_record_base_and_branch_commits(c):
    """The dirty snapshot, branch_commits, rescue refs."""
    check = c.check
    # the snapshot the guard reads: `record_base` must capture what was dirty when it ran, so a path the
    # batch edits afterwards is batch material and one that was dirty before it is foreign. CLAUDE.md is an
    # ordinary tracked file here: an edit to it is foreign when dirty at the base and stageable when named.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("foreign\n")
        data = L.record_base(tmp)
        check("record_base snapshots the dirty set", data.get("dirty_at_base"), ["src/a.c"])
        check("the snapshot is read back", L.base_dirty_paths(tmp), {"src/a.c"})
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\nedited\n")
        check("a CLAUDE.md edit dirty at the base is foreign, like any file",
              "CLAUDE.md" in (L.record_base(tmp).get("dirty_at_base") or []), True)
        check("... and a CLAUDE.md edit the batch names is stageable with no special casing",
              "CLAUDE.md" in L.land_stageable(["CLAUDE.md"], L.changed_status(tmp), L.base_dirty_paths(tmp)), True)
        check("... and one the batch does not name, dirty at the base, is left alone",
              "CLAUDE.md" in L.land_stageable(["src/a.c"], L.changed_status(tmp), L.base_dirty_paths(tmp)), False)
        check("... and the clean-tree gate treats it as dirt", "CLAUDE.md" in (L.require_clean_tree(tmp) or ""), True)

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # the worker branch is cut here
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")  # committed on the branch
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch with commits ahead of main passes", L.branch_commits(tmp, unit) > 0, True)
        check("the .cpp spelling finds the same branch", L.branch_commits(tmp, unit + ".cpp"),
              L.branch_commits(tmp, unit))

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut, but nothing committed on it
        check("a branch with no commits ahead of main fails", L.branch_commits(tmp, unit), 0)

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut before the batch base
        repo_commit(tmp, "the batch base")        # main moves on while the worker works
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch cut before the batch base still passes with commits ahead",
              L.branch_commits(tmp, unit) > 0, True)

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("a missing worker branch fails", L.branch_commits(tmp, unit), 0)

    # a `--force` release deletes the branch and parks the work at refs/rescue/<slug>. The gate's need is that
    # the work exists as commits, so a rescue ref carrying commits is ACCEPTED as the branch's work (2026-09-26
    # case (a)); a rescue ref with no commits of its own, or none at all, is still reported.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        rescue = claims.rescue_ref_name(unit)
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", "HEAD"))
        problems = L.branch_problems(tmp, [unit])
        check("a rescue ref with no commits of its own is reported", len(problems), 1)
        check("... and names the rescue ref", rescue in problems[0], True)
        check("... and names the missing branch", claims.branch_for(unit) in problems[0], True)
        # a rescue ref that carries the worker's commits IS the branch's work
        repo_git(tmp, "checkout", "-q", "-b", "tmpwork")
        repo_commit(tmp, "the worker's own work")
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", "tmpwork"))
        repo_git(tmp, "checkout", "-q", "main")
        repo_git(tmp, "branch", "-D", "tmpwork")
        check("a rescue ref carrying commits is accepted", L.branch_problems(tmp, [unit]), [])
        check("... and restore_rescued_branch puts the real branch back",
              L.restore_rescued_branch(tmp, unit), claims.branch_for(unit))
        check("... which then carries the work", L.branch_commits(tmp, unit) > 0, True)
        check("a missing branch with no rescue ref is still reported", len(L.branch_problems(tmp, ["Nope/none"])), 1)


def test_outbox_and_release(c):
    """Outbox lookup and validation, the release plan, the summary."""
    check = c.check
    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act") + "-dd6e"}})
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"), "w"))
        check("a branch-derived outbox is found", L.outbox_units(tmp, ["Pl/pl_act"]), (["Pl/pl_act"], []))
        # the same entry under the unit-path slug is not what brief.py wrote, so the gate must not accept it
        os.remove(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"))
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + ".json"), "w"))
        ok_units, problems = L.outbox_units(tmp, ["Pl/pl_act"])
        check("an outbox under the unit-path slug is not found", ok_units, [])
        check("and is reported as missing", "no outbox" in (problems[0] if problems else ""), True)

    # the case that failed: a registry keyed by the extensionless name, the gate asked for the .c spelling.
    # `land.py verify --units Camellia/camellia.c` must read the same outbox as `Camellia/camellia`.
    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Camellia/camellia": {"branch": "worker/camellia-67ed"}})
        json.dump(dict(entry, unit="Camellia/camellia"),
                  open(os.path.join(tmp, ".pi", "outbox", "camellia-67ed.json"), "w"))
        check("the extensionless spelling finds the outbox", L.outbox_units(tmp, ["Camellia/camellia"])[1], [])
        check("the .c spelling finds the same outbox", L.outbox_units(tmp, ["Camellia/camellia.c"])[1], [])
        check("both spellings resolve to one unit",
              L.outbox_units(tmp, ["Camellia/camellia"])[0] == L.outbox_units(tmp, ["Camellia/camellia.c"])[0], True)
        check("the outbox path ignores the spelling",
              claims.outbox_path(tmp, "Camellia/camellia.c"), claims.outbox_path(tmp, "Camellia/camellia"))

    # a batch whose `--units` names a HEADER: the path carries a file extension, so `is_batch_path` marks it and
    # `unit_rows` drops it from every unit-shaped row. The outbox row used to run over the raw `--units` list,
    # so the header was validated as a unit and demanded `residual`/`flags_probed` - a BOOKKEEPING refusal while
    # every real gate row passed (2026-09-28, landed with `--no-outbox`). The fixture gives the header a
    # non-unit outbox deliberately: if the row ever runs on it again, this check fails.
    with testing.temp_dir() as tmp:
        os.makedirs(os.path.join(tmp, "include", "Network"))
        open(os.path.join(tmp, "include", "Network", "network_state.h"), "w").write("/* h */\n")
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        header = "include/Network/network_state.h"
        json.dump({"unit": header, "worker": "a", "finished_at": "2026-01-01T00:00:00",
                   "unit_percent": 1.0, "symbols": [{"name": "x", "percent": 1.0}],
                   "measured_with": "n/a", "config_requests": [], "blockers": []},
                  open(claims.outbox_path(tmp, header), "w"))
        json.dump(entry, open(claims.outbox_path(tmp, "Pl/pl_act"), "w"))
        check("a header is a batch path, not a unit", L.is_batch_path(tmp, header), True)
        _ok, raw_problems = L.outbox_units(tmp, [header, "Pl/pl_act"])
        check("validating the raw list treats the header as a unit (the bug)", bool(raw_problems), True)
        check("unit_rows drops the header from the unit-shaped rows", L.unit_rows(tmp, [header, "Pl/pl_act"]),
              ["Pl/pl_act"])
        check("the outbox row then validates only the unit",
              L.outbox_units(tmp, L.unit_rows(tmp, [header, "Pl/pl_act"])), (["Pl/pl_act"], []))

    # a multi-unit batch: one outbox names both units' symbols. Validating it against one unit at a time made
    # every symbol of the other unit "not owned", so the batch could only land with --no-outbox (which turns
    # the outbox check off entirely). The row now reads the batch's whole owned set.
    with testing.temp_dir() as tmp:
        cfg = os.path.join(tmp, "config", "RMHE08")
        os.makedirs(cfg)
        with open(os.path.join(cfg, "splits.txt"), "w", encoding="utf-8") as fh:
            fh.write("A/a.c:\n    .text start:0x100 end:0x200\nB/b.c:\n    .text start:0x200 end:0x300\n")
        with open(os.path.join(cfg, "symbols.txt"), "w", encoding="utf-8") as fh:
            fh.write("fn_a = .text:0x100; // type:function size:0x10\n"
                     "fn_b = .text:0x200; // type:function size:0x10\n")
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        branch = "worker/two-units-abcd"
        obx = os.path.join(tmp, ".pi", "outbox", claims.slug_of_branch(branch) + ".json")
        json.dump(dict(entry, unit="A/a.c + B/b.c",
                       symbols=[{"name": "fn_a", "percent": 50.0}, {"name": "fn_b", "percent": 50.0}]),
                  open(obx, "w"))
        check("a multi-unit outbox validates against the batch's whole owned set",
              L.outbox_units(tmp, ["A/a", "B/b"], branch=branch), (["A/a", "B/b"], []))
        # typo/stale detection stays: a symbol no batch unit owns is still refused
        json.dump(dict(entry, unit="A/a.c + B/b.c", symbols=[{"name": "fn_zzz", "percent": 50.0}]),
                  open(obx, "w"))
        check("... but a symbol no batch unit owns is still refused",
              bool(L.outbox_units(tmp, ["A/a", "B/b"], branch=branch)[1]), True)

    # the teardown step (owner's rule): a green gate releases the batch's claims, a failed one leaves them
    green = [("ground truth", True, "", ""), ("ok", True, "", "")]
    red = [("ground truth", True, "", ""), ("ok was recreated by THIS run", False, "stale stamp", "")]
    check("a green gate releases the batch's claims", L.release_plan(green, ["Pl/pl_act"], True), ["Pl/pl_act"])
    check("a failed gate leaves the claims alone", L.release_plan(red, ["Pl/pl_act"], True), [])
    check("--no-release turns the teardown off", L.release_plan(green, ["Pl/pl_act"], False), [])
    check("an empty batch releases nothing", L.release_plan(green, [], True), [])
    # the 2026-09-23 bug: `--no-worker-units` skipped the outbox check *and* the release, so teardowns stopped
    # for a dozen landings. The two opt-outs are independent now: skipping the outbox check must not skip this.
    check("release runs when the outbox check is off",
          L.release_plan(green, ["Pl/pl_act"], True, check_outbox=False), ["Pl/pl_act"])
    check("... and is unchanged by it", L.release_plan(green, ["Pl/pl_act"], True, check_outbox=True),
          L.release_plan(green, ["Pl/pl_act"], True, check_outbox=False))
    check("--no-release still stops it with the outbox check off",
          L.release_plan(green, ["Pl/pl_act"], False, check_outbox=False), [])
    check("summary delta", L.summary({"closed": 284, "matched": 217}, {"closed": 290, "matched": 223}),
          "closed 284 -> 290, matched 217 -> 223")
    check("summary tolerates a missing side", L.summary({}, {}), "(ledger numbers unavailable)")


def test_branch_guards_and_cli(c):
    """The branch guards and record-base/verify through main()."""
    check = c.check
    # the branch guard: `land` runs on `main`, never on a worker's branch. The incident (2026-09-24): a worker
    # told to "branch and commit there" ran `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout,
    # so MAIN's HEAD left `main` and the next 14 landings went onto that branch while the `main` ref sat at
    # `e3ade082`. the landing path keys off `main`, so a stale `main` does not fail - it
    # silently changes what its diff means. The gate must refuse before any check runs.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("landing on main passes the branch guard", L.branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = L.branch_error(tmp)
        check("a land off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the refusal path itself: exit 1, no gate, and the stale message is cleared like every other refusal
        L.write_land_message(tmp, "land: stale batch\n")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = L.land(tmp, ["Pl/pl_act"], None, no_build=True, subject="x")
        check("a land off main refuses with exit 1", code, 1)
        check("... the refusal names the branch", "throwaway-check" in buf.getvalue(), True)
        check("... and the stale message is cleared", os.path.exists(L.land_message_path(tmp)), False)

    # `record-base` and `verify` are the landing path's two other manual entry points. They read MAIN's tree,
    # but MAIN is resolved from wherever the caller stands (`rc.main_root`), so both must refuse a caller that
    # is not on `main` - a worker's worktree is on the claim's branch, not main, and a MAIN left on a throwaway
    # branch would record the wrong HEAD. `caller_branch_error` is `branch_error` asked about the caller's tree;
    # the entry points themselves are exercised through `main()` with the worktree root pointed at a temp repo.
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("record-base/verify on main pass the caller guard", L.caller_branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = L.caller_branch_error(tmp)
        check("a record-base/verify off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the normal path: `record-base` from a tree on `main` still records that HEAD
        repo_git(tmp, "checkout", "-q", "main")
        with mock.patch.object(L.common, "worktree_root", return_value=tmp), \
                mock.patch.object(sys, "argv", ["land.py", "record-base", "--json"]):
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                code = L.main()
        check("record-base on main still runs", code, 0)
        check("... and records the base it read", L.read_base(tmp).get("base"), repo_git(tmp, "rev-parse", "HEAD"))
        # both entry points refuse a caller off `main`, before reading or writing anything
        repo_git(tmp, "checkout", "-q", "throwaway-check")
        for cmd in ("record-base", "verify"):
            with mock.patch.object(L.common, "worktree_root", return_value=tmp), \
                    mock.patch.object(sys, "argv", ["land.py", cmd]):
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf):
                    code = L.main()
            check("land.py %s off main refuses" % cmd, code, 1)
            check("... names the branch it found", "throwaway-check" in buf.getvalue(), True)


def test_band_boundary(c):
    """Rule 2 at the registration boundary."""
    check = c.check
    # --- rule 2 at the registration boundary (Backlog #1): a range this batch registers makes the
    # symbols inside it owned, so a declaration of one of them still in include/unsplit/<band>.h is now a
    # rule-2 violation - and a candidate `(10505) illegal overloading`. The old stylelint `--diff` could
    # not see it: the band header is not a changed file, so it is not in the diff's file set at all. The
    # checks below are the guard's own bar - delete `band_ownership_warnings` (or the range diff) and they
    # go red on the fixture, because the warning has to FIRE for the checks to pass.
    check("range diff: an unchanged range adds nothing",
          L.added_split_ranges([("a", ".text", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1100)]), [])
    check("range diff: a widened range contributes only the new strip",
          L.added_split_ranges([("a", ".text", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1200)]),
          [("a", ".text", 0x1100, 0x1200)])
    check("range diff: a brand-new unit contributes its whole block",
          L.added_split_ranges([("a", ".text", 0x1000, 0x1100)],
                             [("a", ".text", 0x1000, 0x1100), ("b", ".text", 0x2000, 0x2100)]),
          [("b", ".text", 0x2000, 0x2100)])
    check("range diff: another section is not coverage",
          L.added_split_ranges([("a", ".sdata", 0x1000, 0x1100)], [("a", ".text", 0x1000, 0x1100)]),
          [("a", ".text", 0x1000, 0x1100)])
    check("range diff: two old spans leave the hole between them",
          L.added_split_ranges([("a", ".text", 0x1000, 0x1100), ("b", ".text", 0x1200, 0x1300)],
                             [("c", ".text", 0x1000, 0x1300)]),
          [("c", ".text", 0x1100, 0x1200)])


    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, band_fixture)
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        # the batch: register a new unit's range and WIDEN an existing one; add a band header that declares
        # a symbol an already-registered unit owns. It does NOT touch enemy.h or widen.h.
        _write_tree(tmp, {
            "config/RMHE08/splits.txt": (
                "Sections:\n\t.text       type:code align:32\n\n"
                "existing/unit.cpp:\n\t.text       start:0x80010000 end:0x80010020\n\n"
                "new/fn_801993E0.cpp:\n\t.text       start:0x801993E0 end:0x801993F0\n"),
            "configure.py": ("config.libs = [\n    Object(NonMatching, \"new/fn_801993E0.cpp\"),\n]\n"),
            "include/unsplit/band_added.h": "#ifndef B3\n#define B3\nvoid fn_ALREADY(void);\n#endif\n",
        })
        warns = L.band_ownership_warnings(tmp, base_sha)
        blob = "\n".join(warns)
        check("band: a newly-registered range that the band still declares warns",
              any("fn_8019E9AC" in w and "enemy.h" in w and "src/new/fn_801993E0.cpp" in w
                  for w in warns), True)
        check("band: a WIDENED range that the band declares warns too",
              any("fn_WIDENED" in w and "widen.h" in w and "src/existing/unit.cpp" in w
                  for w in warns), True)
        check("band: a declaration the batch ADDS of an already-owned symbol warns",
              any("fn_ALREADY" in w and "band_added.h" in w and "src/existing/unit.cpp" in w
                  for w in warns), True)
        check("band: a clean C++ spelling of a newly-owned mangled symbol warns",
              any("em_act_ck" in w and "cxx.h" in w
                  and "em_act_ck__FP11_ENEMY_WORKUcUc" in w for w in warns), True)
        check("band: exactly the four findings fire", len(warns), 4)
        # the 2026-09-26 main breakage: the rule-2 instruction to move the declaration broke `main` when it
        # was followed blindly (`756023c4e` moved `fn_80335CE8` into an owner header of a different arity and
        # three call sites lost their declaration). Every rule-2 warning now carries the call-site caveat.
        check("band: the newly-registered warning tells the reader to check the call sites first",
              any("fn_8019E9AC" in w and "after checking every call site" in w
                  and "changes its arity if the owner's prototype differs" in w for w in warns), True)
        check("band: ... and so does the added-declaration warning",
              any("fn_ALREADY" in w and "after checking every call site" in w for w in warns), True)
        check("band: ... and the C++ spelling warning",
              any("em_act_ck" in w and "after checking every call site" in w for w in warns), True)
        # a pre-existing band declaration of an already-owned symbol is NOT this batch's defect: a clean
        # batch must not be spammed with the band's whole backlog.
        check("band: a pre-existing owned declaration does not warn",
              any("enemy.h" in w and "fn_ALREADY" in w for w in warns), False)
        check("band: an unowned symbol in the band is left alone", "fn_OTHER" in blob, False)
        # the guard's own failure mode: with no base there is no diff to reason about, so it is silent
        check("band: no base is silent", L.band_ownership_warnings(tmp, None), [])

    with testing.temp_dir() as tmp:
        # a CLEAN batch - source only, no registration edit - must say nothing
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, dict(band_fixture, **{"src/new/fn_801993E0.cpp": "int f(void) { return 0; }\n"}))
        base_sha = repo_git(tmp, "rev-parse", "HEAD")
        _write_tree(tmp, {"src/new/fn_801993E0.cpp": "int f(void) { return 1; }\n"})
        check("band: a source-only batch is silent", L.band_ownership_warnings(tmp, base_sha), [])


def test_resolver(c):
    """The registration append-conflict resolver."""
    check = c.check

    with testing.temp_dir() as tmp:
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = L.resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("resolve: the append conflict is union-resolved", result.get("ok"), True)
        text = open(os.path.join(wt, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("resolve: both bands are kept",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("resolve: main's own unit is not dropped", "anchor.cpp" in text, True)
        check("resolve: the branch's block is first (address order)",
              text.index("menu/branch.cpp") < text.index("menu/main.cpp"), True)
        check("resolve: no conflict marker survives",
              "<<<<<<<" in text or ">>>>>>>" in text, False)
        check("resolve: exactly the two scoped paths are staged",
              sorted(p for p in repo_git(wt, "diff", "--cached", "--name-only").splitlines() if p),
              ["config/RMHE08/splits.txt", "configure.py"])
        check("resolve: no unmerged path is left", ug.unmerged(wt), {})
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with testing.temp_dir() as tmp:
        # An UNSAFE union: both sides changed the same existing line - unionguard must refuse, and the
        # tree must keep its markers for a hand resolution.
        base = _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = L.resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("unsafe union: land refuses", result.get("ok"), False)
        check("... and names unionguard", "unionguard refused" in result.get("reason", ""), True)
        check("... naming the overlap reason", "same region" in result.get("reason", ""), True)
        conf = open(os.path.join(wt, "configure.py"), encoding="utf-8").read()
        check("... leaving the conflict for a hand resolution", "<<<<<<<" in conf, True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with testing.temp_dir() as tmp:
        # The invariant unionguard CANNOT see: both sides append disjointly (empty base), but their brand
        # bands claim the *same* address range. The union is textually safe and would overlap - the
        # assertion is what refuses it, and nothing is written.
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/dup.cpp:\n\t.text       start:0x80000900 end:0x80000940\n",
            main_splits="menu/dup2.cpp:\n\t.text       start:0x80000920 end:0x80000960\n",
            branch_conf='            Object(NonMatching, "menu/dup.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/dup2.cpp"),\n')
        wt = _conflicted_worktree(tmp)
        result = L.resolve_conflicts(wt, tmp, "worker/x", base=base)
        check("invariant: a textually-safe but overlapping union is refused",
              result.get("ok"), False)
        check("... the reason names the overlap", "overlapping" in result.get("reason", ""), True)
        check("... and carries the violation list", bool(result.get("violations")), True)
        text = open(os.path.join(wt, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("... nothing was written (markers still present)", "<<<<<<<" in text, True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with testing.temp_dir() as tmp:
        # A conflict outside the scope (a header) is a content conflict, never a union.
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"include/shared.h": "int shared = 0;\n", "src/anchor.cpp": "int a;\n"})
        repo_git(tmp, "checkout", "-q", "-b", "worker/x")
        _write_tree(tmp, {"include/shared.h": "int shared = 1;\n"})
        repo_git(tmp, "checkout", "-q", "main")
        _write_tree(tmp, {"include/shared.h": "int shared = 2;\n"})
        wt = _conflicted_worktree(tmp)
        result = L.resolve_conflicts(wt, tmp, "worker/x", base=repo_git(tmp, "merge-base", "main", "worker/x"))
        check("scope: a header conflict is refused", result.get("ok"), False)
        check("... naming it as outside the registration scope",
              "outside the registration scope" in result.get("reason", ""), True)
        check("... and the header is left alone",
              "<<<<<<<" in open(os.path.join(wt, "include", "shared.h"), encoding="utf-8").read(), True)
        repo_git(tmp, "worktree", "remove", "--force", wt)
        repo_git(tmp, "branch", "-D", "scratch")

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n"})
        result = L.resolve_conflicts(tmp, tmp, "main", base=None)
        check("MAIN is refused as a resolution tree", result.get("ok"), False)
        check("... naming MAIN", "inside MAIN" in result.get("reason", ""), True)

    with testing.temp_dir() as tmp:
        # scratch_resolve: the merger lane's operation - a temp worktree, `git merge main`, resolve,
        # commit - with MAIN's HEAD provably untouched.
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        result = L.scratch_resolve(tmp, "worker/x")
        check("scratch: the branch resolves in a scratch worktree", result.get("ok"), True)
        check("... a scratch branch is named", bool(result.get("scratch_branch")), True)
        check("... MAIN's HEAD is untouched", repo_git(tmp, "rev-parse", "HEAD"), main_head)
        text = open(os.path.join(result["worktree"], "config", "RMHE08", "splits.txt"),
                    encoding="utf-8").read()
        check("... the merge commit carries both bands",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("... the merge is committed on the scratch branch",
              repo_git(result["worktree"], "rev-parse", "HEAD") != main_head, True)
        repo_git(tmp, "worktree", "remove", "--force", result["worktree"])
        repo_git(tmp, "branch", "-D", result["scratch_branch"])

    with testing.temp_dir() as tmp:
        _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        result = L.scratch_resolve(tmp, "worker/x")
        check("scratch: an unsafe merge is refused", result.get("ok"), False)
        check("... and the scratch worktree is removed",
              repo_git(tmp, "worktree", "list", "--porcelain").count("worktree "), 1)


def test_land_branch(c):
    """`land --branch` end to end."""
    check = c.check
    # --- the one-command landing (`land --branch`) -------------------------------------------------
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n", "CLAUDE.md": "base\n"})
        check("clean-tree: a committed tree is clean", L.require_clean_tree(tmp), None)
        open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8").write("dirty\n")
        dirty = L.require_clean_tree(tmp)
        check("clean-tree: a tracked edit is refused", dirty is not None, True)
        check("... naming the path", "src/a.cpp" in (dirty or ""), True)
        check("... and the exact clean command", "stash push --include-untracked" in (dirty or ""), True)
        repo_git(tmp, "checkout", "--", "src/a.cpp")
        with open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8") as fh:
            fh.write("real dirt\n")
        message = L.require_clean_tree(tmp) or ""
        check("clean-tree: the dirt is refused", "main's tree is not clean: M src/a.cpp" in message, True)
        check("... and the stash command names it", "stash push --include-untracked -- src/a.cpp" in message,
              True)
        repo_git(tmp, "checkout", "--", "src/a.cpp")


    with testing.temp_dir() as tmp:
        base = _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        check("units-from-branch: reads the branch's registration",
              L.units_from_branch(tmp, "worker/x", base), ["menu/branch"])
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = L.land_branch(tmp, "worker/x", no_build=True, check_outbox=False,
                               release_claims=False, subject="selftest")
        check("land --branch: lands the branch in one command", code, 0)
        check("... the answer line says LANDED", L.answer_line(buf.getvalue()).startswith("LANDED"), True)
        check("... main advanced past the base", repo_git(tmp, "rev-parse", "HEAD") != main_head, True)
        text = open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), encoding="utf-8").read()
        check("... both bands are in the committed file",
              "menu/branch.cpp" in text and "menu/main.cpp" in text, True)
        check("... and the tree is clean afterwards", L.require_clean_tree(tmp), None)
        check("... and a conflict-free landing leaves no land/* ref",
              repo_git(tmp, "for-each-ref", "refs/heads/land/"), "")

    with testing.temp_dir() as tmp:
        # A landing that goes through the conflict-resolution path: `scratch_resolve` parked the union on
        # a `land/resolve-*` helper (and its scratch worktree), the caller fast-forwarded the branch onto
        # it, and the landing deletes the now-redundant helper - visibly.
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        resolved = L.scratch_resolve(tmp, "worker/x")
        helper = resolved["scratch_branch"]
        check("resolve helper: the union is parked on a land/resolve-* ref",
              repo_git(tmp, "for-each-ref", "--format=%(refname)", "refs/heads/land/").endswith(helper),
              True)
        repo_git(tmp, "branch", "-f", "worker/x", helper)      # the caller's documented fast-forward
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf), \
                contextlib.redirect_stderr(buf):
            code = L.land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False,
                               subject="selftest helper")
        check("resolve helper: the branch lands", code, 0)
        check("resolve helper: the landing leaves no land/* ref behind",
              repo_git(tmp, "for-each-ref", "refs/heads/land/"), "")
        check("resolve helper: the deletion is visible in the landing output",
              "resolve helper refs/heads/%s deleted" % helper in buf.getvalue(), True)

    with testing.temp_dir() as tmp:
        # The helper carries a hand fix the branch never took: deleting it would drop the only copy, so
        # the landing must refuse loudly and leave it alone.
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        resolved = L.scratch_resolve(tmp, "worker/x")
        helper = resolved["scratch_branch"]
        _write_tree(resolved["worktree"], {"src/hand_fix.cpp": "int hand_fix;\n"})
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf), \
                contextlib.redirect_stderr(buf):
            code = L.land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False,
                               subject="selftest helper fix")
        check("resolve helper fix: the branch still lands", code, 0)
        check("resolve helper fix: the helper is NOT deleted",
              repo_git(tmp, "for-each-ref", "--format=%(refname)", "refs/heads/land/").endswith(helper),
              True)
        check("resolve helper fix: the refusal is named",
              "REFUSING to delete resolve helper" in buf.getvalue(), True)
        repo_git(tmp, "worktree", "remove", "--force", resolved["worktree"])
        repo_git(tmp, "branch", "-D", helper)

    with testing.temp_dir() as tmp:
        _resolve_fixture(
            tmp,
            branch_anchor_conf='            Object(NonMatching, "anchor_branch.cpp"),\n',
            main_anchor_conf='            Object(Matching, "anchor.cpp"),\n')
        main_head = repo_git(tmp, "rev-parse", "HEAD")
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = L.land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False)
        check("land --branch: an unresolvable conflict is refused", code, 1)
        check("... the answer line says REFUSED", buf.getvalue().startswith("REFUSED"), True)
        check("... naming unionguard", "unionguard refused" in buf.getvalue(), True)
        check("... and the tree is left clean (the apply was undone)", L.require_clean_tree(tmp), None)
        check("... with main unchanged", repo_git(tmp, "rev-parse", "HEAD"), main_head)

    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        _write_tree(tmp, {"src/a.cpp": "int a;\n"})
        repo_git(tmp, "branch", "worker/x")
        open(os.path.join(tmp, "src", "a.cpp"), "w", encoding="utf-8").write("dirty\n")
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf):
            code = L.land_branch(tmp, "worker/x", no_build=True, check_outbox=False, release_claims=False)
        check("land --branch: a dirty tree is refused before anything", code, 1)
        check("... the refusal names the clean command", "stash push --include-untracked" in buf.getvalue(), True)
        check("... and main did not move", repo_git(tmp, "rev-parse", "HEAD"), repo_git(tmp, "rev-parse", "main"))

    with testing.temp_dir() as tmp:
        # A unit renamed at registration: the outbox keeps the pre-registration branch slug. The
        # branch-derived lookup finds it; the unit-derived one does not - and says --no-outbox is the
        # remedy.
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        renamed = "worker/old-proposal-name-abcd"
        slug = claims.slug_of_branch(renamed)
        json.dump(dict(entry, unit="hud/fn_80334568"),
                  open(os.path.join(tmp, ".pi", "outbox", slug + ".json"), "w"))
        ok, problems = L.outbox_units(tmp, ["hud/fn_80334568"])
        check("renamed unit: the unit-derived outbox path misses", ok, [])
        check("... and the problem names --no-outbox", "--no-outbox" in (problems[0] if problems else ""), True)
        ok, problems = L.outbox_units(tmp, ["hud/fn_80334568"], branch=renamed)
        check("renamed unit: the branch-derived path finds the outbox", ok, ["hud/fn_80334568"])
        check("... with no problems", problems, [])
        check("renamed unit: the claim key is read from the branch",
              L.claim_unit_for_branch(tmp, renamed), None)   # no registry entry: None, not a wrong key

    with testing.temp_dir() as tmp:
        # the CLI wiring for the one command, through `main()` with MAIN pointed at the fixture
        _resolve_fixture(
            tmp,
            branch_splits="menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
            main_splits="menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n",
            branch_conf='            Object(NonMatching, "menu/branch.cpp"),\n',
            main_conf='            Object(NonMatching, "menu/main.cpp"),\n')
        with mock.patch.object(L.common, "worktree_root", return_value=tmp), \
                mock.patch.object(L.common, "main_root", return_value=tmp), \
                mock.patch.object(L.gate, "verify", _land_verify_ok), \
                mock.patch.object(sys, "argv", ["land.py", "land", "--branch", "worker/x",
                                                "--no-build", "--no-outbox", "--no-release"]):
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                code = L.main()
        check("land --branch runs through main()", code, 0)
        check("... and prints the one answer line", L.answer_line(buf.getvalue()).startswith("LANDED"), True)
        check("... with the short LEDGER line directly above it",
              [ln.split(" ")[0] for ln in buf.getvalue().splitlines() if ln.strip()][-2:], ["LEDGER:", "LANDED"])


CFG_BASE = ("object: orig/RMHE08/sys/main.dol\nfill_gaps: true\nblock_relocations:\n"
            "- target: extabindex:0x80020000\n  end: extabindex:0x80020010\n")
CFG_BLOCK = CFG_BASE + ("# the updateSession error code is a constant\n- source: .text:0x803D7564\n"
                        "  end: .text:0x803D756C\n")


def _config_branch(tmp, branch_files):
    """`main` with a registration and a config.yml; `worker/x` registers a unit and writes `branch_files`."""
    repo_git(tmp, "init", "-q")
    repo_git(tmp, "checkout", "-q", "-b", "main")
    _write_tree(tmp, dict(_registration_files(), **{"config/RMHE08/config.yml": CFG_BASE,
                                                    "config/RMHE08/build.sha1": "BF48  build/RMHE08/main.dol\n"}))
    repo_git(tmp, "checkout", "-q", "-b", "worker/x")
    files = _registration_files("menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n",
                                '            Object(NonMatching, "menu/branch.cpp"),\n')
    files["src/menu/branch.cpp"] = "int b(void) { return 1; }\n"
    files.update(branch_files)
    _write_tree(tmp, files)
    repo_git(tmp, "checkout", "-q", "main")


def test_land_branch_config_relocations(c):
    """A `block_relocations`-only config.yml change rides a `land --branch` batch (the hook's own rule decides);
    any other key, and build.sha1, still refuse at pre-flight with main unchanged (the L2 round-2 refusal)."""
    check = c.check
    with testing.temp_dir() as tmp:
        _config_branch(tmp, {"config/RMHE08/config.yml": CFG_BLOCK})
        head = repo_git(tmp, "rev-parse", "HEAD")
        buf = io.StringIO()
        with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(buf), \
                contextlib.redirect_stderr(io.StringIO()):
            code = L.land_branch(tmp, "worker/x", units=["menu/branch"], no_build=True, check_outbox=False,
                                 release_claims=False, subject="selftest")
        check("config: a block_relocations-only change lands with the unit batch", (code, L.answer_line(
            buf.getvalue()).split(" ")[0]), (0, "LANDED"))
        check("... in the one commit, with the unit source",
              sorted(repo_git(tmp, "diff", "--name-only", head, "HEAD").splitlines()),
              ["config/RMHE08/config.yml", "config/RMHE08/splits.txt", "configure.py", "src/menu/branch.cpp"])
        check("... and the tree is clean afterwards", L.require_clean_tree(tmp), None)
        check("outside_batch admits config.yml by content only when MAIN is named",
              (L.outside_batch(["config/RMHE08/config.yml"]), L.outside_batch(["config/RMHE08/config.yml"], main=tmp)),
              (["config/RMHE08/config.yml"], []))
    for label, files, cause in (
            ("a frozen key (fill_gaps)", {"config/RMHE08/config.yml": CFG_BLOCK.replace("fill_gaps: true",
                                                                                        "fill_gaps: false")},
             "`fill_gaps`"),
            ("build.sha1", {"config/RMHE08/config.yml": CFG_BLOCK,
                            "config/RMHE08/build.sha1": "0000  build/RMHE08/main.dol\n"}, None)):
        with testing.temp_dir() as tmp:
            _config_branch(tmp, files)
            head = repo_git(tmp, "rev-parse", "HEAD")
            out, err = io.StringIO(), io.StringIO()
            with mock.patch.object(L.gate, "verify", _land_verify_ok), contextlib.redirect_stdout(out), \
                    contextlib.redirect_stderr(err):
                code = L.land_branch(tmp, "worker/x", units=["menu/branch"], no_build=True, check_outbox=False,
                                     release_claims=False, subject="selftest")
            check("config: %s still refuses at pre-flight" % label,
                  (code, "pre-flight" in (L.answer_line(out.getvalue()) + err.getvalue()).lower()
                   or "outside the batch" in out.getvalue()), (1, True))
            check("... main did not move and the apply was undone",
                  (repo_git(tmp, "rev-parse", "HEAD"), L.require_clean_tree(tmp)), (head, None))
            if cause:
                check("... and the pre-flight names the frozen key", cause in err.getvalue(), True)


def test_ledger_line(c):
    """The short ledger line and the answer line."""
    check = c.check
    # the short ledger line: the delta alone (no `partial`, no unit list), so a cut LANDED line loses nothing
    _b = {"covered": 10, "closed": 4, "partial": 2, "matched": 30, "bytes": 1000}
    _a = {"covered": 12, "closed": 5, "partial": 3, "matched": 35, "bytes": 1600, "total_code": 9}
    check("ledger_line: covered/closed/matched/bytes deltas", L.ledger_line(_b, _a),
          "LEDGER: covered 10 -> 12, closed 4 -> 5, matched 30 -> 35, bytes 1000 -> 1600")
    check("ledger_line: no recorded base is said plainly", L.ledger_line({}, _a), "LEDGER: (ledger numbers unavailable)")
    check("ledger_line: short enough to survive a cut", len(L.ledger_line(_b, _a)) < 100, True)
    check("answer_line: the last non-empty line", L.answer_line("LEDGER: x\nLANDED abc u | y\n\n"), "LANDED abc u | y")


def test_f34_and_integrate(c):
    """The gate's own decode and the integrate forward."""
    check = c.check
    # --- F34: the gate's own decode, and the lint that keeps it that way ---------------------------------
    # `land.run` used `text=True` with no codec, so git's UTF-8 stdout was decoded with the host's locale
    # codec (`cp1252` here) while a file was compared against it read as UTF-8.  One em dash in CLAUDE.md's
    # prose made the two spellings differ, and the landing gate refused *every* landing with "main's tree is
    # not clean: M CLAUDE.md" - a false refusal.  The fixture is that em dash at byte level: a check that
    # decodes through the locale is exactly the failure it is here to catch, so it must not itself be
    # host-dependent.
    dash = "\u2014"
    probe = L.run([sys.executable, "-c", "import sys; sys.stdout.buffer.write(bytes.fromhex('%s'))"
                 % dash.encode("utf-8").hex()], tempfile.gettempdir())
    check("F34: `run` decodes a subprocess' UTF-8 stdout as UTF-8", probe.stdout, dash)
    with testing.temp_dir() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        prose = "prose with an em dash %s in it\n" % dash
        _write_tree(tmp, {"CLAUDE.md": prose})
        head_text = L.run(["git", "show", "HEAD:CLAUDE.md"], tmp).stdout
        head_bytes = subprocess.run(["git", "show", "HEAD:CLAUDE.md"], cwd=tmp,
                                    capture_output=True).stdout
        check("F34: `run` returns HEAD's em dash, not its cp1252 spelling", dash in head_text, True)
        check("... where a locale decode of those same bytes would not have matched",
              dash in head_bytes.decode("cp1252", "replace"), False)
        check("F34: an unchanged non-ASCII CLAUDE.md is clean", L.require_clean_tree(tmp), None)
        with open(os.path.join(tmp, "CLAUDE.md"), "w", encoding="utf-8", newline="") as fh:
            fh.write("prose with an em dash %s and an EDIT\n" % dash)
        check("... while a real edit is dirt, naming CLAUDE.md", "CLAUDE.md" in (L.require_clean_tree(tmp) or ""), True)

    check("integrate: `land.py integrate ARGS` runs integrate.py with ARGS unchanged",
          L.integrate_command(["--dry-run", "--lane", "x"])[1:], [L.INTEGRATE, "--dry-run", "--lane", "x"])
    check("integrate: the forward target exists", os.path.isfile(L.INTEGRATE), True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
