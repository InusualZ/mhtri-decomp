"""wtsafe: a junction is unlinked, never walked; `git worktree remove --force` DOES destroy a junction's target
(the 2026-09-24 hazard) and `remove_worktree` stops it; MAIN's `orig/` matches the hashes `config.yml` pins."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import subprocess
import tempfile

from tools.lib import testing
from tools.units import wtsafe

TIER = "smoke"   # the last test reads MAIN's orig/ (skipped when absent); the rest work in temp dirs


def _git(args, cwd):
    subprocess.run(["git", "-c", "user.email=a@b", "-c", "user.name=c", "-c", "init.defaultBranch=main", *args],
                   cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")


def test_unlink_keeps_the_target(c):
    with tempfile.TemporaryDirectory() as tmp:
        plain = os.path.join(tmp, "plain")
        os.makedirs(os.path.join(plain, "sub"))
        c.check("a real directory is not a reparse point", wtsafe.is_reparse_point(plain), False)
        target = os.path.join(tmp, "keep")
        os.makedirs(target)
        precious = os.path.join(target, "data.txt")
        with open(precious, "w") as fh:
            fh.write("ground truth\n")
        wt = os.path.join(tmp, "unlink_me")
        os.makedirs(wt)
        link = os.path.join(wt, "junc")
        if not wtsafe._make_junction(link, target):
            c.skip("junction round trip", "no junction/symlink available on this host")
            return
        c.check("a junction IS a reparse point", wtsafe.is_reparse_point(link), True)
        c.check("unlink_reparse_points removes it", wtsafe.unlink_reparse_points(wt), [link])
        c.check("... the link is gone, the target and its contents stay",
                (os.path.exists(link), os.path.isdir(target), open(precious).read()), (False, True, "ground truth\n"))


def test_git_hazard_and_the_fix(c):
    if os.name != "nt":
        c.skip("the git junction hazard", "measured on Windows junctions only")
        return
    with tempfile.TemporaryDirectory() as tmp:
        main, target = os.path.join(tmp, "m"), os.path.join(tmp, "target")
        os.makedirs(main)
        os.makedirs(target)
        precious = os.path.join(target, "data.txt")
        _git(["init", "-q", "."], main)
        with open(os.path.join(main, "f"), "w") as fh:
            fh.write("hi\n")
        _git(["add", "f"], main)
        _git(["commit", "-qm", "init"], main)
        out = []
        for name, protected in (("hazard", False), ("protected", True)):
            with open(precious, "w") as fh:
                fh.write("ground truth\n")
            wt = os.path.join(tmp, "wt_" + name)
            _git(["worktree", "add", "-q", wt], main)
            if not wtsafe._make_junction(os.path.join(wt, "junc"), target):
                c.skip("the git junction hazard", "mklink /J unavailable")
                return
            if protected:
                wtsafe.remove_worktree(wt, main)
            else:
                _git(["worktree", "remove", "--force", wt], main)
            out.append(os.path.exists(precious))
            _git(["worktree", "prune"], main)
        c.check("git worktree remove DOES destroy a junction's target (the 2026-09-24 hazard)", out[0], False)
        c.check("remove_worktree() stops it (target intact)", out[1], True)


def test_main_orig_matches_the_pins(c):
    main = wtsafe.main_worktree()
    if not os.path.exists(os.path.join(main, wtsafe.CONFIG)):
        c.skip("orig/ pins", "no config.yml under %s" % main)
        return
    pins = wtsafe.ground_truth(main)
    c.check("ground_truth reads both pins", sorted(pins), ["orig/RMHE08/files/mh3.sel", "orig/RMHE08/sys/main.dol"])
    if not all(os.path.exists(os.path.join(main, p)) for p in pins):
        c.skip("orig/ hashes", "orig/ is absent in %s" % main)
        return
    c.check("MAIN's orig/ matches config.yml's pinned hashes", wtsafe.verify_orig(main), [])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
