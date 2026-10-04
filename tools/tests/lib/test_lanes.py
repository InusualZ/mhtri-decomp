"""lib.lanes: one spelling for slugs/branches/rescue refs, the claim registry (cluster rows included), the session
signal, the launch line, the teardown step list and its junction-safe removal, rescue verdicts, the seeder's
deps-log rewrite and staleness guard, the slot sentinel, and the landing log - on throwaway trees only."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import subprocess
import tempfile
import time

from tools.lib import testing
from tools.lib.lanes import landlog, launch, naming, pool, registry, rescue, seed, sessions, teardown

TIER = "fixture"


def test_naming(c):
    c.check("a unit and its source spelling share one slug (the lock is not forked)",
            naming.slug("proposal/801679B0_fn_801679B0"), naming.slug("proposal/801679B0_fn_801679B0.cpp"))
    c.check("the slug is the cleaned basename plus 4 hex of the path", naming.slug("Pl/pl_act")[:7], "pl-act-")
    c.check("two units with one basename get two slugs", naming.slug("Pl/pl_act") == naming.slug("auto/pl_act"), False)
    c.check("the slug is capped", len(naming.slug("auto/" + "x" * 200)) <= naming.SLUG_MAX, True)
    c.check("branch, worktree and rescue ref follow the slug",
            (naming.branch_for("Pl/pl_act"), os.path.basename(naming.worktree_for("Pl/pl_act", "/r/mhtri-dtk")),
             naming.rescue_ref("Pl/pl_act.cpp")),
            ("worker/" + naming.slug("Pl/pl_act"), "mhtri-dtk.ws-" + naming.slug("Pl/pl_act"),
             "refs/rescue/" + naming.slug("Pl/pl_act")))
    c.check("slug_of_branch / lane_slug / rescue_ref_for_branch",
            (naming.slug_of_branch("worker/x-1"), naming.slug_of_branch(None), naming.lane_slug("experiment/wP"),
             naming.rescue_ref_for_branch("experiment/wP")), ("x-1", None, "wP", "refs/rescue/wP"))
    c.check("only the lane prefixes are lanes", [naming.is_lane_branch(b) for b in ("worker/x", "lane/y", "main")],
            [True, True, False])


def test_registry(c):
    with tempfile.TemporaryDirectory() as main:
        c.check("an absent registry is empty and readable", (registry.load(main), registry.read(main)), ({}, ({}, True)))
        registry.save(main, {"Gecko/G.cp": {"branch": "worker/g-1"},
                             "cluster/net": {"branch": "worker/net-2", "units": ["Net/a.cpp", "Net/b"]}})
        c.check("a record is found under either spelling", (registry.record(main, "Gecko/G")["branch"],
                registry.key_of(registry.load(main), "Gecko/G")), ("worker/g-1", "Gecko/G.cp"))
        c.check("a cluster row holds every unit it lists", registry.claimed_units(registry.load(main)),
                {"Gecko/G", "cluster/net", "Net/a", "Net/b"})
        c.check("record_holding finds the cluster's row for a member", registry.record_holding(main, "Net/a.cpp")["branch"],
                "worker/net-2")
        c.check("... and nothing for a stranger", registry.record_holding(main, "Net/z"), {})
        c.check("the outbox and notes follow the branch, the ack the unit",
                (os.path.basename(registry.outbox_path(main, "Gecko/G")), os.path.basename(registry.ack_path(main, "Gecko/G"))),
                ("g-1.json", naming.slug("Gecko/G") + ".json"))
        c.check("record_for_branch", registry.record_for_branch(registry.load(main), "worker/net-2")[0], "cluster/net")
        with open(registry.registry_path(main), "w") as fh:
            fh.write("{ not json")
        c.check("an unparseable registry is unreadable (fail closed), and loads as empty",
                (registry.read(main), registry.load(main)), (({}, False), {}))
        c.check("age_seconds parses its own stamp and refuses junk",
                (registry.age_seconds(registry.now()) is not None, registry.age_seconds("junk")), (True, None))


def test_branch_lock(c):
    g = testing.GitFixture().init()
    g.commit({"f.txt": "a\n"}, "base")
    main = str(g.root)
    g.branch("worker/x-1")
    c.check("the branch is the lock", (registry.branch_exists(main, "worker/x-1"), registry.worker_branches(main)),
            (True, {"worker/x-1"}))
    c.check("lock_held reads the branch", registry.lock_held(main, "x", {"worker/" + naming.slug("x")}), True)
    g.branch("feature", checkout=True)
    g.commit({"f.txt": "b\n"}, "work")
    g.checkout("main")
    c.check("an unmerged branch is ahead and not merged",
            (registry.commits_ahead(main, "feature"), registry.merged_into_main(main, "feature")), (1, False))
    g.git("cherry-pick", "feature")
    c.check("a cherry-picked branch counts as merged", registry.merged_into_main(main, "feature"), True)
    c.check("main_of resolves the tree a caller stands in", os.path.normcase(registry.main_of(main)),
            os.path.normcase(os.path.abspath(main)))


def test_sessions(c):
    with tempfile.TemporaryDirectory() as tmp:
        reg = os.path.join(tmp, sessions.SESSIONS_DIRNAME)
        os.makedirs(reg)
        gone = subprocess.Popen([sys.executable, "-c", "pass"])
        gone.wait()
        for name, pid, cwd in (("a", os.getpid(), os.path.join(tmp, "slot", "src")), ("b", gone.pid, tmp)):
            with open(os.path.join(reg, name + ".json"), "w") as fh:
                json.dump({"pid": pid, "sessionId": "sid-" + name, "cwd": cwd, "name": "lane " + name}, fh)
        with open(os.path.join(reg, "junk.json"), "w") as fh:
            fh.write("not json")
        runs = sessions.live_runs(reg)
        c.check("only a record whose pid is alive counts (a crash leaves a dead one)", [r["run_id"] for r in runs],
                ["sid-a"])
        c.check("a session inside a tree is in it", [r["run_id"] for r in sessions.runs_in(os.path.join(tmp, "slot"), reg)],
                ["sid-a"])
        c.check("... and not in a sibling", sessions.runs_in(os.path.join(tmp, "slot2"), reg), [])
        c.check("the registry is found under a config dir; none is no signal",
                (sessions.run_registries(tmp), sessions.run_registries(os.path.join(tmp, "nope"))), ([reg], []))
        c.check("a run is named by id and session", sessions.run_label(runs[0]), "sid-a (lane a)")


def test_launch(c):
    c.check("every kind maps to its profile; PROFILES is the table's values",
            ([launch.profile_for_kind(k) for k in ("unit", "fix", "tooling", "review")], launch.PROFILES),
            (["surveyor", "fixer", "worker", "codereviewer"], tuple(sorted(set(launch.KIND_PROFILE.values())))))
    err = c.raises("an unknown kind is refused", SystemExit, launch.profile_for_kind, "bogus")
    c.check("... listing every valid kind", all(k in str(err) for k in launch.KIND_PROFILE), True)
    block = launch.tree_block("C:/m", "C:/m.slot1")
    c.check("the tree block names the tree, the self-check, the release ban and do-not-land",
            ("C:/m.slot1" in block, "git rev-parse --show-toplevel" in block, "NEVER run `claims.py release`" in block,
             "**Do not land.**" in block), (True, True, True, True))


def test_teardown_steps(c):
    order = []
    steps = [teardown.step("one", lambda: order.append(1) or "noted"), teardown.skip("two", "nothing to do"),
             teardown.step("three", lambda: (_ for _ in ()).throw(SystemExit("refused: live"))),
             teardown.step("four", lambda: order.append(4))]
    c.check("a failure stops the plan", teardown.run(steps), False)
    c.check("... recording each step's status and why",
            [(s["status"], s["why"]) for s in steps],
            [("done", "noted"), ("skipped", "nothing to do"), ("failed", "refused: live"),
             ("skipped", "not attempted: three failed first")])
    c.check("... and nothing after it runs", order, [1])
    c.check("public() drops the callables", "action" in teardown.public(steps)[0], False)


def test_junction_safe_removal(c):
    with tempfile.TemporaryDirectory() as tmp:
        target = os.path.join(tmp, "keep")
        os.makedirs(target)
        with open(os.path.join(target, "data.txt"), "w") as fh:
            fh.write("ground truth\n")
        wt = os.path.join(tmp, "wt")
        os.makedirs(os.path.join(wt, "sub"))
        link = os.path.join(wt, "sub", "junc")
        if not teardown.make_junction(link, target):
            c.skip("junction removal", "no junction/symlink on this host")
            return
        c.check("a junction is a reparse point, a directory is not",
                (teardown.is_reparse_point(link), teardown.is_reparse_point(wt)), (True, False))
        c.check("unlink removes the link only", (teardown.unlink_reparse_points(wt), os.path.exists(link),
                open(os.path.join(target, "data.txt")).read()), ([link], False, "ground truth\n"))
    g = testing.GitFixture().init()
    g.commit({"f.txt": "x\n"}, "base")
    leftover = os.path.join(str(g.root) + "-left")
    os.makedirs(leftover)
    with open(os.path.join(leftover, "junk"), "w") as fh:
        fh.write("j")
    c.check("a directory git does not track is removed as a leftover",
            (teardown.remove_worktree_checked(str(g.root), leftover), os.path.exists(leftover)),
            ("removed the leftover directory (git no longer tracked it)", False))
    wt2 = g.worktree(str(g.root) + "-wt", "lane/x")
    c.check("a registered worktree is removed", (teardown.remove_worktree_checked(str(g.root), str(wt2)),
            os.path.exists(str(wt2))), ("removed", False))


def test_rescue_verdict(c):
    g = testing.GitFixture().init()
    base = g.commit({"configure.py": 'config.libs = [\n    Object(NonMatching, "a/a.cpp"),\n]\n',
                     "config/RMHE08/splits.txt": "a/a.cpp:\n\t.text       start:0x80000100 end:0x80000200\n",
                     "src/a/a.cpp": "int a;\n"}, "base")
    main = str(g.root)
    g.branch("b-red", checkout=True)
    red = g.commit({"src/a/a.cpp": "int a = 1;\n"}, "red")
    g.checkout("main")
    g.commit({"src/a/a.cpp": "int a = 1;\n"}, "land red")
    rescue.make(main, "refs/rescue/red", red)
    g.branch("b-unl", base, checkout=True)
    unl = g.commit({"configure.py": 'config.libs = [\n    Object(NonMatching, "a/a.cpp"),\n    Object(NonMatching, '
                                    '"u/u.cpp"),\n]\n', "src/u/u.cpp": "int u;\n"}, "unl")
    g.checkout("main")
    rescue.make(main, "refs/rescue/unl", unl)
    rows = {r["ref"]: r for r in rescue.audit(main)["refs"]}
    c.check("classify: redundant (touched path matches main) and unlanded (unit not on main)",
            (rows["refs/rescue/red"]["verdict"], rows["refs/rescue/unl"]["verdict"], rows["refs/rescue/unl"]["units"]),
            (rescue.VERDICT_REDUNDANT, rescue.VERDICT_UNLANDED, ["u/u"]))
    v = rescue.verdict(main, "refs/rescue/red")
    c.check("verdict prunes only a redundant ref and says so",
            (v["pruned"], v["line"].startswith("pruned refs/rescue/red"), registry.ref_exists(main, "refs/rescue/red")),
            (True, True, False))
    u = rescue.verdict(main, "refs/rescue/unl")
    c.check("an unlanded ref is kept and surfaced", (u["pruned"], "UNLANDED WORK" in u["line"],
            registry.ref_exists(main, "refs/rescue/unl")), (False, True, True))
    c.check("a ref that is not there is unknown, never an exception", rescue.verdict(main, "refs/rescue/none")["verdict"],
            rescue.VERDICT_UNKNOWN)


def test_seed(c):
    with tempfile.TemporaryDirectory() as tmp:
        main, wt = os.path.join(tmp, "m"), os.path.join(tmp, "w")
        root = os.path.abspath(main).replace("\\", "/").encode()
        log = seed.ninja_deps_serialize(4, [("path", root + b"/include/types.h"), ("path", b"src/a.cpp"),
                                            ("deps", 1, 12345, [0])])
        c.check("the deps log round-trips byte for byte", seed.ninja_deps_serialize(*seed.ninja_deps_parse(log)), log)
        moved = seed.ninja_deps_parse(seed.ninja_deps_rewrite(log, main, wt))[1]
        c.check("MAIN's paths move onto the worktree, relative ones stay",
                [r[1] for r in moved if r[0] == "path"],
                [os.path.abspath(wt).replace("\\", "/").encode() + b"/include/types.h", b"src/a.cpp"])
        c.check("a log that is not one is refused, not mangled", seed.ninja_deps_rewrite(b"junk", main, wt), None)
        for rel, t in (("config/RMHE08/splits.txt", 1_000_000), ("configure.py", 1_000_000),
                       ("build/RMHE08/config.json", 2_000_000), ("build.ninja", 2_000_000)):
            p = os.path.join(main, *rel.split("/"))
            os.makedirs(os.path.dirname(p), exist_ok=True)
            open(p, "w").close()
            os.utime(p, (t, t))
        c.check("a split newer than its inputs is current", seed.main_build_is_current(main), True)
        os.utime(os.path.join(main, "config", "RMHE08", "splits.txt"), (3_000_000, 3_000_000))
        c.check("... and a newer splits.txt makes it stale", seed.main_build_is_current(main), False)
        os.makedirs(os.path.join(main, "build", "tools"))
        open(os.path.join(main, "build", "tools", "dtk.exe"), "w").close()
        os.makedirs(wt)
        c.check("seeding copies the toolchain and says so", (seed.seed_worktree_build(main, wt).startswith("seeded"),
                os.path.isfile(os.path.join(wt, "build", "tools", "dtk.exe")),
                teardown.is_reparse_point(os.path.join(wt, "build", "tools"))), (True, True, False))
        c.check("a MAIN with nothing to seed is skipped", seed.seed_worktree_build(os.path.join(tmp, "x"), wt)
                .startswith("skipped"), True)


def test_pool_sentinel(c):
    with tempfile.TemporaryDirectory() as tmp:
        c.check("a slot is a stable sibling of MAIN", os.path.basename(pool.slot_dir(os.path.join(tmp, "mhtri-dtk"), 3)),
                "mhtri-dtk.slot3")
        slot = os.path.join(tmp, "slot")
        os.makedirs(slot)
        c.check("the sentinel is created once (O_EXCL)", (pool.mark_used(slot, "u", "worker/u-1", "w1"),
                pool.mark_used(slot, "v", "worker/v-1", "w2")), (True, False))
        info = pool.marker_info(slot)
        c.check("it records its owner first", (info["owner"], info["unit"], info["legacy"]), ("w1", "u", False))
        c.check("another owner's sentinel is a conflict, one's own is not",
                ("w1" in (pool.marker_claim_conflict(slot, "w2", 1) or ""), pool.marker_claim_conflict(slot, "w1", 1)),
                (True, None))
        pool.clear_marker(slot)
        with open(pool.marker_path(slot), "w") as fh:
            fh.write("123 auto/x worker/x\n")
        c.check("a legacy body names nobody (no conflict)", (pool.marker_info(slot)["legacy"],
                pool.marker_claim_conflict(slot, "w2")), (True, None))
        c.check("the owner label: worker, else branch slug, else unit",
                [pool.owner_label("u", "worker/u-9", "w"), pool.owner_label("u", "worker/u-9"), pool.owner_label("u", None)],
                ["w", "u-9", "u"])
        oid = "b" * 40
        c.check("merge-tree's tree OID is read; a conflict or junk is ambiguity",
                [pool.merge_tree_oid(0, oid + "\n"), pool.merge_tree_oid(1, oid), pool.merge_tree_oid(0, "CONFLICT")],
                [oid, None, None])
        c.check("no manifest and no slots: the pool is not enabled", pool.enabled(os.path.join(tmp, "mhtri-dtk")), False)


def test_landlog(c):
    with tempfile.TemporaryDirectory() as main:
        c.check("no log reads as nothing", landlog.read(main), ([], []))
        landlog.append(main, landlog.Attempt("worker/a-1", "landed", 61.0, units=("a/a",), commit="abc"))
        landlog.append(main, landlog.Attempt("worker/b-2", "refused", 12.5, refused_row="stylelint --diff adds none"))
        landlog.append(main, landlog.Attempt("worker/c-3", "conflict", 3.0, conflicts=("src/c.cpp", "configure.py")))
        landlog.append(main, landlog.Attempt("worker/d-4", "refused", 9.0, refused_row="stylelint --diff adds none"))
        with open(landlog.log_path(main), "a", encoding="utf-8") as fh:
            fh.write("not json\n")
        rows, bad = landlog.read(main)
        c.check("one JSON line per attempt, a bad line reported by number", (len(rows), bad), (4, [5]))
        c.check("each line is stamped and carries the schema", (bool(rows[0]["at"]), rows[0]["schema"]),
                (True, landlog.SCHEMA))
        s = landlog.summary(rows)
        c.check("the summary counts outcomes and ranks the refusing rows",
                (s["outcomes"], s["refused_rows"][0], s["landed_ratio"]),
                ({"landed": 1, "refused": 2, "conflict": 1, "error": 0}, ("stylelint --diff adds none", 2), 0.25))
        c.check("... the conflicted paths and the wall time", (s["conflicted_paths"][0][1], s["seconds_total"]), (1, 85.5))
        c.raises("an unknown outcome is refused", ValueError, landlog.Attempt, "b", "maybe", 1.0)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
