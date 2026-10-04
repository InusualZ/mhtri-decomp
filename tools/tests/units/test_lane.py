"""lane: rescue before delete, an idempotent teardown, the non-lane refusal, a missing rescue ref reported as
missing, and the `orig/` guard that aborts a removal which damaged MAIN - on a throwaway repository."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import hashlib
import os

from tools.lib import testing
from tools.lib.lanes import teardown as td
from tools.units import lane

TIER = "fixture"


def _repo():
    g = testing.GitFixture().init()
    g.commit({"a.txt": "a\n"}, "base")
    g.branch("experiment/lane-x", checkout=True)
    g.commit({"b.txt": "b\n"}, "lane work")
    g.checkout("main")
    return g


def test_slug(c):
    c.check("slug strips the lane prefix", lane.slug("experiment/wP-promote"), "wP-promote")
    c.check("slug keeps a worker branch readable", lane.slug("worker/800a99b4-fn-800a99b4-0403"),
            "800a99b4-fn-800a99b4-0403")
    c.check("slug leaves an unprefixed name alone", lane.slug("wip"), "wip")


def test_rescue(c):
    g = _repo()
    tmp = str(g.root)
    c.check("unlanded finds the lane's commit", len(lane.unlanded("experiment/lane-x", repo=tmp)), 1)
    c.check("unlanded is empty for main itself", lane.unlanded("main", repo=tmp), [])
    c.raises("teardown refuses a non-lane branch", SystemExit, lane.teardown, "main", repo=tmp)
    r = lane.rescue("experiment/lane-x", repo=tmp)
    c.check("rescue sets the ref", r["action"], "set")
    c.check("the ref names the lane's tip", g.head("refs/rescue/lane-x"), g.head("experiment/lane-x"))
    c.check("a second rescue is idempotent", lane.rescue("experiment/lane-x", repo=tmp)["action"], "set")
    g.git("branch", "-D", "experiment/lane-x")
    c.check("the rescue ref survives the branch deletion", bool(g.head("refs/rescue/lane-x")), True)
    g.git("branch", "recovered", "refs/rescue/lane-x")
    c.check("recovery is one command and holds the work", len(lane.unlanded("recovered", repo=tmp)), 1)
    g.branch("experiment/lane-y", checkout=True)
    g.commit({"c.txt": "c\n"}, "lane y")
    g.checkout("main")
    rows = {row["branch"]: row for row in lane.lanes(repo=tmp)}
    y = rows.get("experiment/lane-y") or {}
    c.check("lanes reports a missing rescue ref as missing", (y.get("unlanded"), y.get("rescue")), (1, False))


def test_teardown(c):
    g = _repo()
    tmp = str(g.root)
    dol = os.path.join(tmp, "orig", "RMHE08", "sys", "main.dol")
    sel = os.path.join(tmp, "orig", "RMHE08", "files", "mh3.sel")
    os.makedirs(os.path.dirname(dol))
    os.makedirs(os.path.dirname(sel))
    with open(dol, "w") as fh:
        fh.write("the original\n")
    with open(sel, "w") as fh:
        fh.write("the selfile\n")
    h = hashlib.sha1(open(dol, "rb").read()).hexdigest().upper()
    hs = hashlib.sha1(open(sel, "rb").read()).hexdigest().upper()
    g.commit({"configure.py": "# fixture\n",
              "config/RMHE08/config.yml": "object: orig/RMHE08/sys/main.dol\nhash: %s\nselfile: "
                                          "orig/RMHE08/files/mh3.sel\nselfile_hash: %s\n" % (h, hs)},
             "config and orig")
    wt = g.worktree(str(g.root) + "-wt", "experiment/lane-teardown")
    td.make_junction(os.path.join(str(wt), "orig_junc"), os.path.join(tmp, "orig"))
    out = lane.teardown("experiment/lane-teardown", force=True, repo=tmp)
    c.check("teardown removes the worktree", (out["worktree"] is not None, os.path.exists(str(wt))), (True, False))
    c.check("teardown deletes the branch", g.run("rev-parse", "--verify", "--quiet",
                                                 "refs/heads/experiment/lane-teardown").returncode != 0, True)
    c.check("teardown leaves MAIN's orig/ intact", td.verify_orig(tmp), [])
    c.check("a second teardown is a clean no-op",
            lane.teardown("experiment/lane-teardown", force=True, repo=tmp)["action"], "nothing to tear down")
    wt2 = g.worktree(str(g.root) + "-wt2", "experiment/lane-hazard")

    def damaging(path, repo=None):
        links = td.remove_worktree(path, repo)
        with open(dol, "w") as fh:
            fh.write("damaged\n")
        return links

    c.raises("teardown aborts when MAIN's orig/ changes", SystemExit, lane.teardown, "experiment/lane-hazard",
             force=True, repo=tmp, remove=damaging)
    c.check("... and keeps the branch for the next attempt",
            g.run("rev-parse", "--verify", "--quiet", "refs/heads/experiment/lane-hazard").returncode, 0)
    c.check("... the hazard's worktree was the one removed", os.path.exists(str(wt2)), False)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
