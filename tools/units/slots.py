#!/usr/bin/env python3
"""The slot pool's CLI over `lib.lanes.pool`: reusable lane directories at stable paths, a slot holds no branch.
Spec: docs/tools/spec/slots.md. CLI: slots.py init | acquire <unit> | spawn --kind K [--slot N] | release | reclaim |
collect [--slot N|--path P] [--release] | status [--json] | refresh N [--force] | verify | shadow <slot> <dir> | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import io
import json
import os
import shutil
import subprocess
import tarfile
import time

from tools.lib import proc as _proc
from tools.lib import testing
from tools.lib.git import Git
from tools.lib.lanes import launch, naming, pool, seed, sessions
from tools.lib.lanes.registry import main_of as lane_registry_main_of
from tools.lib.lanes.launch import KIND_PROFILE, PROFILES, profile_for_kind, tree_block as _tree_block  # noqa: F401
from tools.lib.lanes.pool import (DEFAULT_COUNT, LOCK_SUBDIR, SLOT_KEEP, SLOT_MARKER, SLOT_SUFFIX,  # noqa: F401
                                  acquire, all_slots, branch_fully_applied, capacity_error, claim_currency,
                                  clean_slot, clear_lock, clear_marker, compile_outputs, enabled, free_count,
                                  free_slots, git, lock_path, lock_stale, locks_dir, mark_used, marker_claim_conflict,
                                  marker_info, marker_owner, marker_path, marker_present, marker_text, merge_tree_of,
                                  ninja_pending, orphaned_commits, owner_label, pool_manifest, pool_size, preview,
                                  read_lock, reclaim_line, reclaim_slot, reclaim_verdict, refresh_slot, refs_containing,
                                  registry_claims_by_slot, release, release_blockers, release_refusal,
                                  report_matches, reset_slot, slot_attached_branch, slot_count, slot_dir, slot_dirty,
                                  slot_head, slot_of_path, slot_state, unlanded_reason, verify, write_lock)
from tools.lib.lanes.sessions import (SESSIONS_DIRNAME, config_dir, live_runs, pid_alive,  # noqa: F401
                                      run_label, run_registries)

_proc.install_spawn_retry()  # a launch Windows refuses transiently (WinError 5) is retried

_merge_tree_oid = pool.merge_tree_oid
_resolve_slot = pool.resolve_slot
_pick_free = pool.pick_free
_announce_reclaim = pool.announce_reclaim
_manifest_outputs = pool.manifest_outputs
_same_tree = sessions.same_tree


def runs_in_slot(slot: str, registry: str | None = None, config: str | None = None) -> list[dict]:
    """Every live session whose cwd resolves into `slot`."""
    return sessions.runs_in(slot, registry, config)


def _claims():
    """`claims` imported late (it imports nothing of this module; the selftest's claim-path seam uses it)."""
    from tools.units import claims
    return claims


def currency_lines(cur: dict) -> list[str]:
    """The handover proof in the lane's own words: report bytes, the object count, the pending steps - with the
    "proven current" header printed only when the check passed, the doubt named otherwise."""
    objects = "%d/%d" % (cur["objects_present"], cur["objects_checked"])
    report = "byte-identical to MAIN's" if cur["report_matches"] else "DIFFERS from MAIN's"
    if cur["pending"] is None:
        pending = "unknown (%s)" % (cur["pending_note"] or "ninja could not run")
    elif cur["pending"] == 0:
        pending = "0"
    else:
        pending = "%d still pending%s" % (cur["pending"], " (%s)" % cur["pending_note"] if cur["pending_note"] else "")
    measured = ("`report.json` is %s; the official scorer's compile outputs are complete (%s objects "
                "present); `ninja -n` pending steps: %s." % (report, objects, pending))
    doubts = []
    if not cur["report_matches"]:
        doubts.append("`report.json` differs from MAIN's")
    if not cur["compile_outputs"]:
        doubts.append("the official scorer's compile outputs are incomplete")
    if cur["pending"] is None:
        doubts.append("ninja could not count the pending steps")
    elif cur["pending"]:
        doubts.append("%d ninja step(s) are still pending" % cur["pending"])
    if cur.get("completed") is False:
        doubts.append("the ninja run that was meant to finish them failed")
    if cur.get("ok"):
        lines = ["**Your build tree is proven current at handover.** " + measured]
    else:
        lines = ["**Build tree currency: NOT PROVEN at handover.** " + measured +
                 " Doubt: %s." % "; ".join(doubts or ["the currency check did not pass"])]
    if cur["completed"]:
        lines.append("The pending steps were run **in this slot** at acquire time; nothing outside it was touched.")
    return lines


# --- init -------------------------------------------------------------------------------------------------------

def init(main: str, count: int = DEFAULT_COUNT, force: bool = False, seed_trees: bool = True) -> dict:
    """Create the pool: `count` detached slots beside MAIN, each seeded once. Idempotent; `force` re-creates
    them and refuses while any slot holds live work (a branch, a run, a claim, a sentinel)."""
    if force:
        live = [s for s in all_slots(main) if s.get("attached") or s.get("live_claim") or s.get("run") or s.get("marked")]
        if live:
            raise SystemExit("REFUSED init --force: %d slot(s) still hold live work - the pool is the "
                             "campaign's concurrency cap, not a scratch file; release or reclaim each slot "
                             "first:\n%s" % (len(live), "\n".join("  slot %d: %s" % (s["slot"], s["why"]) for s in live)))
    os.makedirs(locks_dir(main), exist_ok=True)
    with open(os.path.join(locks_dir(main), "pool.json"), "w", encoding="utf-8") as fh:
        json.dump({"count": count, "created_at": time.strftime("%Y-%m-%dT%H:%M:%S")}, fh, indent=1, sort_keys=True)
    tip = git(["rev-parse", "HEAD"], main)
    created, present, notes = [], [], []
    for n in range(1, count + 1):
        d = slot_dir(main, n)
        if os.path.exists(os.path.join(d, ".git")) and not force:
            present.append(n)
        else:
            if os.path.isdir(d):
                subprocess.run(["git", "worktree", "remove", "--force", "--force", d], cwd=main,
                               capture_output=True, text=True, encoding="utf-8", errors="replace")
            git(["worktree", "prune"], main)
            git(["worktree", "add", "--detach", d, tip], main)
            created.append(n)
        if seed_trees:
            notes.append((n, seed.seed_worktree_build(main, d, copy_orig=True, overwrite=False)))
    return {"main": main, "tip": tip, "count": count, "created": created, "present": present,
            "notes": notes, "slots": [slot_dir(main, n) for n in range(1, count + 1)]}


# --- shadow -----------------------------------------------------------------------------------------------------

def _rmtree_force(path: str) -> None:
    """`shutil.rmtree` that also clears git's read-only objects (Windows denies unlink on them)."""
    def _onexc(func, p, exc):
        os.chmod(p, 0o700)
        func(p)

    try:
        shutil.rmtree(path, onexc=_onexc)
    except TypeError:
        shutil.rmtree(path, onerror=lambda f, p, e: (os.chmod(p, 0o700), f(p)))


def shadow(main: str, slot: int, dest: str, seed_build: bool = False, force: bool = False) -> dict:
    """Materialise a lane-shaped tree INSIDE a slot (`git archive HEAD` + its own `git init`, nothing registered
    in MAIN). Refuses a destination outside the slot (resolved paths), a non-empty one, and a slot that is not
    free (unless `force`)."""
    d = os.path.abspath(slot_dir(main, slot))
    if not os.path.exists(os.path.join(d, ".git")):
        raise SystemExit("REFUSED shadow: slot %d is not a worktree (%s)" % (slot, d))
    dest_abs = os.path.abspath(dest if os.path.isabs(dest) else os.path.join(d, dest))
    real_d = os.path.normcase(os.path.realpath(d))
    real_dest = os.path.normcase(os.path.realpath(dest_abs))
    if real_dest == real_d or not real_dest.startswith(real_d + os.sep):
        raise SystemExit("REFUSED shadow: %s is not INSIDE slot %d (%s) - a shadow lives in its slot, "
                         "never in MAIN or a sibling slot" % (dest_abs, slot, d))
    state = slot_state(main, slot)
    if not state["free"] and not force:
        raise SystemExit("REFUSED shadow: slot %d is not free (%s) - a shadow inside a lane's tree shows up "
                         "in ITS `git status` and refuses the lane's landing; pass --force only for a slot "
                         "you hold" % (slot, state["why"]))
    if os.path.exists(dest_abs):
        if os.listdir(dest_abs) and not force:
            raise SystemExit("REFUSED shadow: %s exists and is not empty (--force to replace it)" % dest_abs)
        if force:
            _rmtree_force(dest_abs)
    p = subprocess.run(["git", "-C", d, "archive", "--format=tar", "HEAD"], capture_output=True)
    if p.returncode != 0:
        raise SystemExit("REFUSED shadow: cannot read slot %d's tree: %s"
                         % (slot, (p.stderr or b"").decode("utf-8", "replace").strip()))
    os.makedirs(dest_abs, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(p.stdout)) as tf:
        try:
            tf.extractall(dest_abs, filter="data")
        except TypeError:
            tf.extractall(dest_abs)
        files = sum(1 for m in tf.getmembers() if m.isfile())
    seeded: list[str] = []
    if seed_build:
        for rel in (seed.RMHE08_REL, os.path.join("build", "tools"), os.path.join("build", "compilers"),
                    os.path.join("build", "binutils"), seed.ORIG_REL, os.path.join("tools", "m2c")):
            src, dst = os.path.join(d, rel), os.path.join(dest_abs, rel)
            if os.path.exists(dst):
                continue
            if os.path.isdir(src):
                shutil.copytree(src, dst)
                seeded.append(rel.replace(os.sep, "/"))
            elif os.path.isfile(src):
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
                seeded.append(rel.replace(os.sep, "/"))
    git(["init", "-q", "-b", "main"], dest_abs)
    git(["add", "-A"], dest_abs)
    git(["-c", "user.email=shadow@example.invalid", "-c", "user.name=shadow", "-c", "commit.gpgsign=false",
         "commit", "-q", "-m", "shadow of slot %d" % slot], dest_abs)
    return {"slot": slot, "dir": dest_abs, "files": files, "seeded": seeded, "head": git(["rev-parse", "HEAD"], dest_abs)}


# --- collect ----------------------------------------------------------------------------------------------------

#: The evidence a lane leaves under its own `.pi/` (copied out before a release `clean -ffdx`es it).
EVIDENCE = (("outbox", ".json"), ("notes", ".md"))


def _merge_data_requests(main: str, slot_root: str) -> int:
    """Merge a lane's `.pi/data-requests.json` into MAIN's deduplicated register -> rows added or changed."""
    from tools.units import dataqueue as dq
    theirs = dq.load_requests(slot_root)
    if not theirs:
        return 0
    merged, changed = dq.load_requests(main), 0
    for row in theirs:
        merged, did = dq.merge_request(merged, row)
        changed += 1 if did else 0
    if changed:
        dst = os.path.join(main, dq.REQUESTS_REL)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        dq.write_requests(dst, dq.render_requests(merged))
    return changed


def collect(main: str, slot: int | None = None, path: str | None = None, release_after: bool = False,
            force: bool = False, registry: str | None = None) -> dict:
    """Copy a finished lane's `.pi/outbox/*.json` and `.pi/notes/*.md` into MAIN (keeping MAIN's copy when at
    least as new), merge its data requests, report its branch, commits and dirt; release only when nothing is
    unlanded (or `force`). `path` may also name a plain worktree of the repository (a lane launched in its own
    `git worktree`, not a slot): its evidence is collected the same way, and it is never released here (its
    teardown is the harness's `git worktree remove`), so `release_after` is refused for it."""
    n = slot if slot is not None else slot_of_path(main, path)
    if n is None:
        d = plain_worktree(main, path)
        if d is None:
            raise SystemExit("REFUSED collect: no slot or worktree of this repository resolves from %r - pass "
                             "--slot N, a slot's path or a `git worktree list` path" % (path,))
        if release_after:
            raise SystemExit("REFUSED collect --release: %s is a plain worktree, not a slot - collect without "
                             "--release; its teardown is `git worktree remove`" % d)
    else:
        d = slot_dir(main, n)
        if not os.path.isdir(d):
            raise SystemExit("REFUSED collect: slot %d does not exist (%s)" % (n, d))
    branch = slot_attached_branch(d)
    copied, kept = [], []
    for sub, ext in EVIDENCE:
        src_dir = os.path.join(d, ".pi", sub)
        if not os.path.isdir(src_dir):
            continue
        for name in sorted(os.listdir(src_dir)):
            src = os.path.join(src_dir, name)
            if not name.endswith(ext) or not os.path.isfile(src):
                continue
            dst = os.path.join(main, ".pi", sub, name)
            if os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
                kept.append("%s/%s" % (sub, name))
                continue
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)
            copied.append("%s/%s" % (sub, name))
    requests_merged = _merge_data_requests(main, d)
    commits = []
    if branch:
        head = git(["rev-parse", "HEAD"], main)
        commits = [ln for ln in git(["log", "--format=%h %s", "%s..%s" % (head, branch)], main).splitlines() if ln]
    reason = pool.unlanded_reason_at(main, d)
    out = {"slot": n, "dir": d, "branch": branch, "slug": naming.slug_of_branch(branch) if branch else None,
           "copied": copied, "kept": kept, "requests_merged": requests_merged, "commits": commits,
           "dirty": slot_dirty(d), "unlanded": reason, "released": False}
    if release_after:
        if reason and not force:
            out["kept_because"] = reason
        else:
            out["release"] = release(main, slot=n, branch=branch, registry=registry, force=force)
            out["released"] = True
    return out


def plain_worktree(main: str, path: str | None) -> str | None:
    """`path` as one of the repository's own worktrees other than MAIN (`git worktree list`), or None."""
    if not path:
        return None
    want = os.path.normcase(os.path.realpath(path))
    if want == os.path.normcase(os.path.realpath(main)):
        return None
    for wt in Git(main).worktree_list():
        if os.path.normcase(os.path.realpath(wt.path)) == want:
            return os.path.realpath(path)
    return None


def collect_lines(out: dict) -> list[str]:
    where = "slot %d" % out["slot"] if out["slot"] is not None else "worktree %s" % out["dir"]
    lines = ["%s  %s" % (where, out["branch"] or "(detached)")]
    lines.append("  evidence copied to MAIN/.pi: %s" % (", ".join(out["copied"]) or "none"))
    if out.get("requests_merged"):
        lines.append("  data-claim requests merged into MAIN/.pi/data-requests.json: %d" % out["requests_merged"])
    if out["kept"]:
        lines.append("  already in MAIN and at least as new: %s" % ", ".join(out["kept"]))
    lines.append("  commits main does not have: %d%s" % (len(out["commits"]),
                 "".join("\n    " + c for c in out["commits"][:10])))
    lines.append("  release: %s" % ("DONE" if out["released"] else
                 ("kept - %s" % out["unlanded"] if out["unlanded"] else "safe (nothing unlanded)")))
    return lines


# --- spawn ------------------------------------------------------------------------------------------------------

def tree_block(main: str, path: str) -> str:
    return _tree_block(main, path)


def spawn_units(main: str, task: str | None, units: list[str] | None) -> tuple[list[str], list[str]]:
    """The unit set a spawned lane holds: `units` when given, else the task's `Units:` line, each resolved to a
    registered unit (`launch.resolve_units` over `splits.txt`) -> `(units, unresolved names)`."""
    from tools.lib.project import Splits
    names = list(units) if units else launch.task_units(task or "")
    if not names:
        return [], []
    splits = os.path.join(main, "config", "RMHE08", "splits.txt")
    registered = Splits.read(splits).units if os.path.exists(splits) else []
    return launch.resolve_units(names, registered)


def spawn(main: str, kind: str, slot: int | None = None, unit: str | None = None,
          task: str | None = None, task_file: str | None = None, worker: str | None = None,
          force: bool = False, units: list[str] | None = None) -> dict:
    """Take a slot (by number, or the first free one - named) for a lane of `kind` and return the paste-ready
    launch: the headless `claude --agent <profile>` line run in the slot, then the "your tree" block and the
    claim-time currency proof. The kind, profile and the lane's unit set (`units`, else the task's `Units:`
    line - `claims.py list --json` reads it back as `units`) are recorded on the slot's lock."""
    profile = profile_for_kind(kind)
    if task is None and task_file:
        try:
            with open(task_file, encoding="utf-8") as fh:
                task = fh.read().strip()
        except OSError as exc:
            raise SystemExit("REFUSED spawn: cannot read --task-file %s: %s" % (task_file, exc))
    held, unresolved = spawn_units(main, task, units)
    unit = unit or "lane/%s-%s" % (kind, time.strftime("%Y%m%d-%H%M%S"))
    info = acquire(main, unit, worker=worker or os.environ.get("USERNAME") or os.environ.get("USER") or "unknown",
                   slot=slot, force=force)
    path = info["dir"]
    lock = read_lock(main, info["slot"])
    if lock:
        lock["kind"] = kind
        lock["agent"] = profile
        if held:
            lock["units"] = held
        write_lock(main, info["slot"], lock)
    tail = ("End your turn with your report: your final message is the result the orchestrator receives. "
            "If you need a ruling, end the turn with the request - the orchestrator resumes this session.")
    if task:
        task_text = "%s\n\n%s" % (task, tail)
    else:
        task_text = ("No task text was given (`--task-file` was not passed). Replace this placeholder with "
                     "the lane's task. " + tail)
    call = launch.lane_call(profile, path, task_text, name="%s-slot%d" % (profile, info["slot"]),
                            main=main, key="slot%d" % info["slot"])
    if lock:
        lock["session_id"] = call["session_id"]
        write_lock(main, info["slot"], lock)
    block = "\n\n".join([tree_block(main, path)] + currency_lines(info["currency"]))
    return {"slot": info["slot"], "path": path, "agent": profile, "kind": kind,
            "branch": info["branch"], "unit": unit, "units": held, "unresolved_units": unresolved,
            "sessionId": call["session_id"], "spawnLine": "%s\n\n%s" % (call["call"], block)}


# --- status -----------------------------------------------------------------------------------------------------

def status(main: str, registry: str | None = None) -> list[dict]:
    """Every slot's row, plus its build tree's verdict, its HEAD and any live run in it."""
    rows = []
    live = live_runs(registry)
    for s in all_slots(main, runs=live):
        v = verify(main, s["slot"]) if s["exists"] else {"ok": False, "reasons": ["missing"]}
        rows.append({**s, "build_ok": v["ok"], "build_reasons": v["reasons"],
                     "head": slot_head(s["dir"]) if s["exists"] else None})
    return rows


# --- selftest -----------------------------------------------------------------------------------

def selftest() -> int:
    testing.isolate_live_state()   # no real ~/.claude/sessions: a live lane must not change the verdict
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def _raises(fn) -> bool:
        try:
            fn()
            return False
        except SystemExit:
            return True

    import contextlib
    import shutil
    import tempfile
    claims = _claims()

    # --- the ninja seam: deterministic and hermetic, and it exercises the pending>0 -> complete -> 0 path --
    saved_ninja = pool.NINJA_RUNNER

    def fake_ninja(pending=()):
        """A ninja runner: `-n` reports the next count in `pending` (then 0); a real run succeeds.

        `pending` is a queue of *pending-step counts*: `[3]` means the first `ninja -n` saw 3 steps and,
        once `ninja` ran, the re-count saw 0 - the exact fresh-slot shape.  `["error"]` / `["missing"]`
        simulate ninja failing to answer.
        """
        counts = list(pending) if pending else [0]
        calls = []

        class Done:
            pass

        def run(argv, cwd=None, **kw):
            calls.append(list(argv))
            p = Done()
            p.stdout, p.stderr = "", ""
            if list(argv) == ["ninja", "-n"]:
                n = counts.pop(0) if counts else 0
                if n == "error":
                    p.returncode, p.stderr = 1, "ninja: fatal: build.ninja, line 1: unknown rule"
                elif n == "missing":
                    raise FileNotFoundError("ninja")
                else:
                    p.returncode = 0
                    p.stdout = ("".join("[%d/%d] step\n" % (i + 1, n) for i in range(n))
                                if n else "ninja: no work to do.\n")
            else:
                p.returncode, p.stdout, p.stderr = 0, "", ""
            return p

        run.calls = calls
        return run

    # every acquire/spawn below reads the currency seam; a hermetic 0-pending answer stands in for ninja
    pool.NINJA_RUNNER = fake_ninja()

    # the counting itself, before any fixture: dry-run lines -> the pending count; failure/absence -> unknown
    check("ninja -n output is counted", ninja_pending("/nonexistent", runner=fake_ninja([3]))[0], 3)
    check("... and 'no work to do' is 0", ninja_pending("/nonexistent", runner=fake_ninja([0]))[0], 0)
    check("... a failing ninja is UNKNOWN, not 0", ninja_pending("/nonexistent", runner=fake_ninja(["error"]))[0],
          None)
    check("... and it says why", "ninja -n failed" in ninja_pending("/nonexistent",
                                                                     runner=fake_ninja(["error"]))[1], True)
    check("... an absent ninja is UNKNOWN, not 0", ninja_pending("/nonexistent",
                                                                runner=fake_ninja(["missing"]))[0], None)

    def g(path, *args, check=True):
        p = subprocess.run(["git", "-c", "user.email=t@example.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if check and p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def commit(path, msg):
        open(os.path.join(path, "f.txt"), "a", encoding="utf-8").write(msg + "\n")
        g(path, "add", "-A")
        g(path, "commit", "-q", "-m", msg)

    def fixture(tmp, count=2):
        """A real repo with a real (small) warm build tree, so init/acquire/release exercise git."""
        repo = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(os.path.join(repo, "src", "auto"))
        os.makedirs(os.path.join(repo, "config", "RMHE08"))
        os.makedirs(os.path.join(repo, "tools", "m2c"))
        os.makedirs(os.path.join(repo, "orig", "RMHE08", "sys"))
        os.makedirs(os.path.join(repo, "orig", "RMHE08", "files"))
        os.makedirs(os.path.join(repo, "build", "RMHE08", "obj", "auto"))
        os.makedirs(os.path.join(repo, "build", "compilers"))
        os.makedirs(os.path.join(repo, "build", "binutils"))
        os.makedirs(os.path.join(repo, "build", "tools"))
        open(os.path.join(repo, "configure.py"), "w").write("config.libs = []\n")
        # CLAUDE.md is committed with a real **em dash** in its prose: a decode that used the host locale codec
        # (`cp1252` here) would call a clean slot dirty (F34's failure, one file over).  Written as UTF-8
        # explicitly - the fixture must not inherit the trap it guards against.
        with open(os.path.join(repo, "CLAUDE.md"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write("# repo notes\n\nprose with an em dash \u2014 in it\n")
        open(os.path.join(repo, ".gitignore"), "w").write(
            "build/\norig/\n.ninja_*\nbuild.ninja\nobjdiff.json\ncompile_commands.json\n__pycache__/\n.used\n")
        open(os.path.join(repo, "src", "auto", "stub.c"), "w").write("/* header only */\n")
        open(os.path.join(repo, "tools", "m2c", "m2c.py"), "w").write("# m2c\n")
        open(os.path.join(repo, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/stub.c:\n\t.text  start:0x80100000 end:0x80100100\n")
        open(os.path.join(repo, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_80100000 = .text:0x80100000; // type:function size:0x40\n")
        open(os.path.join(repo, "orig", "RMHE08", "sys", "main.dol"), "wb").write(b"dol\n")
        open(os.path.join(repo, "orig", "RMHE08", "files", "mh3.sel"), "wb").write(b"sel\n")
        open(os.path.join(repo, "build", "tools", "dtk.exe"), "wb").write(b"dtk\n")
        open(os.path.join(repo, "build", "compilers", "mwcc.exe"), "wb").write(b"cc\n")
        open(os.path.join(repo, "build", "binutils", "as.exe"), "wb").write(b"as\n")
        open(os.path.join(repo, "build", "RMHE08", "obj", "auto", "stub.o"), "wb").write(b"OBJ-V1\n")
        os.makedirs(os.path.join(repo, "build", "RMHE08", "src", "auto"))
        open(os.path.join(repo, "build", "RMHE08", "src", "auto", "stub.o"), "wb").write(b"SRC-OBJ-V1\n")
        open(os.path.join(repo, "build", "RMHE08", "config.json"), "w").write('{"libs": []}\n')
        open(os.path.join(repo, "build", "RMHE08", "report.json"), "w").write('{"units": {"auto/stub": 1.0}}\n')
        open(os.path.join(repo, "build.ninja"), "w").write("# ninja\n")
        open(os.path.join(repo, ".ninja_deps"), "wb").write(seed.ninja_deps_serialize(4, [
            ("path", os.path.abspath(repo).replace("\\", "/").encode() + b"/include/types.h")]))
        open(os.path.join(repo, ".ninja_log"), "w").write("# ninja log v5\n")
        # The manifest the official scorer opens: `target_path` is the split object, `base_path` the src
        # compile output.  A real one (both objects exist) so `compile_outputs` has something to assert.
        open(os.path.join(repo, "objdiff.json"), "w").write(json.dumps({"units": [
            {"name": "main/auto/stub", "target_path": "build/RMHE08/obj/auto/stub.o",
             "base_path": "build/RMHE08/src/auto/stub.o"}]}, indent=1) + "\n")
        open(os.path.join(repo, "compile_commands.json"), "w").write("[]\n")
        g(repo, "init", "-q")
        g(repo, "checkout", "-q", "-b", "main")
        g(repo, "add", "-A")
        g(repo, "commit", "-q", "-m", "base")
        # mtimes: the split is newer than its inputs, the manifest newer than configure (the guard's rule)
        for p, t in (("config/RMHE08/symbols.txt", 1_000_000), ("config/RMHE08/splits.txt", 1_000_000),
                     ("orig/RMHE08/sys/main.dol", 1_000_000), ("orig/RMHE08/files/mh3.sel", 1_000_000),
                     ("configure.py", 1_000_000), ("build/RMHE08/config.json", 2_000_000),
                     ("build/RMHE08/obj/auto/stub.o", 2_000_000),
                     ("build/RMHE08/src/auto/stub.o", 2_000_000), ("build/RMHE08/report.json", 2_000_000),
                     ("build.ninja", 2_000_000)):
            os.utime(os.path.join(repo, p), (t, t))
        return repo

    # the path rule: siblings of MAIN, stable across calls
    check("slot dir is a sibling of main", os.path.basename(slot_dir("/tmp/mhtri-dtk", 3)), "mhtri-dtk.slot3")
    check("slot path is stable", slot_dir("/tmp/mhtri-dtk", 3), slot_dir("/tmp/mhtri-dtk", 3))
    check("a tree with no pool is not enabled", enabled("/tmp/nonexistent-mhtri-dtk"), False)

    with testing.temp_dir() as tmp:
        repo = fixture(tmp, count=2)
        out = init(repo, count=2)
        check("init creates the requested slots", out["created"], [1, 2])
        check("init seeded them", all("build/RMHE08" in note for _, note in out["notes"]), True)
        check("the pool is now enabled", enabled(repo), True)
        check("a slot is a detached worktree", slot_attached_branch(slot_dir(repo, 1)), None)
        check("the slot has the warm build tree",
              os.path.exists(os.path.join(slot_dir(repo, 1), "build", "RMHE08", "report.json")), True)
        check("the slot has the original payload",
              os.path.exists(os.path.join(slot_dir(repo, 1), "orig", "RMHE08", "sys", "main.dol")), True)
        check("the slot's deps log points at the slot, not MAIN",
              os.path.abspath(repo).replace("\\", "/").encode() + b"/include/types.h"
              not in open(os.path.join(slot_dir(repo, 1), ".ninja_deps"), "rb").read(), True)
        check("a freshly seeded slot verifies current", verify(repo, 1)["ok"], True)

        # (0) THE COMPILE-OUTPUT SET: `config.json`'s mtime and `report.json`'s bytes can both pass while
        # the objects the report was computed from are gone - and that tree cannot run the official scorer.
        # `objdiff.json` names the scorer's inputs (`target_path` = the split object, `base_path` = the
        # `src/` compile output); `verify` asserts every one of them is on disk.
        v0 = verify(repo, 1)
        check("verify asserts the scorer's compile outputs", v0["compile_outputs"], True)
        check("... counting the split and the src object", v0["compile_outputs_checked"], 2)
        gone = os.path.join(slot_dir(repo, 1), "build", "RMHE08", "src", "auto", "stub.o")
        os.remove(gone)
        v0 = verify(repo, 1)
        check("a slot missing a src/ compile output is REFUSED", v0["ok"], False)
        check("... naming the missing object",
              any("compile outputs" in r and "src" in r for r in v0["reasons"]), True)
        check("... while report.json still matches MAIN (the old check would have passed it)",
              v0["report_matches"], True)
        check("... and the missing count is reported", v0["compile_outputs_missing"], 1)
        # a re-seed refills it from MAIN (the arbiter) - verify is green again, so this is repair, not a wedge
        seed.seed_worktree_build(repo, slot_dir(repo, 1), copy_orig=True, overwrite=False)
        check("re-seeding refills the dropped output, and verify is green", verify(repo, 1)["ok"], True)
        # the slot's own manifest is a doubt too: the scorer reads the slot's copy, not MAIN's
        mf = os.path.join(slot_dir(repo, 1), "objdiff.json")
        os.remove(mf)
        v0 = verify(repo, 1)
        check("a slot missing its objdiff.json is refused even with every object present", v0["ok"], False)
        check("... naming the manifest",
              any("objdiff.json" in r for r in v0["reasons"]), True)
        seed.seed_worktree_build(repo, slot_dir(repo, 1), copy_orig=True, overwrite=False)
        check("... and re-seeding restores it", verify(repo, 1)["ok"], True)
        # `refresh N` (the registry's slot-build refresh): a current slot is left alone, a stale one is repaired by
        # copying only what differs, and the verdict after is what it reports
        check("refresh leaves a current slot alone", refresh_slot(repo, 1, runs=[])["refreshed"], None)
        open(os.path.join(slot_dir(repo, 1), "build", "RMHE08", "report.json"), "w").write('{"units": {}}\n')
        rf = refresh_slot(repo, 1, runs=[])
        check("refresh repairs a slot whose report differs from MAIN's",
              (rf["before"]["ok"], bool(rf["refreshed"]), rf["after"]["ok"]), (False, True, True))
        live = [{"cwd": slot_dir(repo, 1), "run_id": "r-live", "session": "a lane"}]
        check("a run record in the slot reads LIVE", slot_state(repo, 1, runs=live)["state"], "LIVE")
        try:
            refresh_slot(repo, 1, runs=live)
            refused_live = "not refused"
        except SystemExit as exc:
            refused_live = "RUNNING Claude session r-live" in str(exc)
        check("refresh refuses a slot a running session works in", refused_live, True)
        # claim-time currency: the pending steps a fresh slot ALWAYS has by construction are run here, then
        # re-counted - so the number the lane is shown is what is left, not what was
        nrun = fake_ninja([3])
        cur = claim_currency(repo, 1, complete=True, runner=nrun)
        check("claim_currency runs the pending ninja steps", ["ninja"] in nrun.calls, True)
        check("... and re-counts to 0", cur["pending"], 0)
        check("... recording that it completed them", cur["completed"], True)
        check("... and re-verifying the tree it just touched", cur["verify"]["ok"], True)
        check("... while proving the report and the object set",
              (cur["report_matches"], cur["compile_outputs"], cur["objects_present"]), (True, True, 2))
        # count-only leaves the work to the caller and says it did not do it
        cur2 = claim_currency(repo, 1, complete=False, runner=fake_ninja([2]))
        check("claim_currency(count-only) reports the pending count", cur2["pending"], 2)
        check("... and does not claim to have completed them", cur2["completed"], None)
        # the handover line carries the proof; an unknown is never rounded down to a confident 0
        proof = "\n".join(currency_lines(cur))
        check("the handover proof names report.json", "byte-identical to MAIN's" in proof, True)
        check("... the object count", "(2/2 objects present)" in proof, True)
        check("... and 0 pending steps", "pending steps: 0" in proof, True)
        unknown = "\n".join(currency_lines({"report_matches": True, "compile_outputs": True,
                                            "objects_present": 2, "objects_checked": 2,
                                            "pending": None, "pending_note": "ninja unavailable",
                                            "completed": None}))
        check("an unknown pending count is reported as unknown, not 0",
              "pending steps: unknown" in unknown, True)
        # `ok` is the field a future caller trusts, and the UNKNOWN pending count must not pass it:
        # `not None` was True, so the currency check passed on the one reading that proves nothing
        check("pending work is not `ok`", cur2["ok"], False)
        cur3 = claim_currency(repo, 1, complete=True, runner=fake_ninja(["error"]))
        check("... and neither is an UNKNOWN pending count", cur3["ok"], False)
        check("... reported as None, never rounded to 0", cur3["pending"], None)
        doubtful = "\n".join(currency_lines(cur3))
        check("... so the handover block never claims currency",
              "proven current at handover" in doubtful, False)
        check("... naming the doubt and the measured values instead",
              "NOT PROVEN" in doubtful and "pending steps: unknown" in doubtful, True)
        check("... while the proven case still says it", "proven current at handover" in proof, True)
        # idempotent
        again = init(repo, count=2)
        check("init is idempotent", (again["created"], sorted(again["present"])), ([], [1, 2]))

        # THE CAP IS pool.json's COUNT, the authority - a broken slot must not drop a LIVE one from the
        # enumeration (slot 2's missing worktree used to shrink a pool of 6 to `slots 1..5`, hiding slot 6).
        check("pool_size reads the manifest's count", pool_size(repo), 2)
        moved = slot_dir(repo, 2) + ".moved-for-test"
        os.rename(slot_dir(repo, 2), moved)
        try:
            rows2 = all_slots(repo)
            check("... every slot in the manifest is still enumerated when one has no worktree",
                  [r["slot"] for r in rows2], [1, 2])
            check("... and the broken one says `no worktree`", rows2[1]["state"], "no worktree")
            check("... so a live slot is never hidden behind a broken one", rows2[0]["exists"], True)
        finally:
            os.rename(moved, slot_dir(repo, 2))

        # (0b) A SHADOW: a lane-shaped tree INSIDE the slot, for fixture tests.  `git worktree add` is the
        # obvious way and the wrong one - it writes metadata into MAIN's `.git/worktrees`, outside the slot.
        wts_before = [line for line in g(repo, "worktree", "list", "--porcelain").splitlines()
                      if line.startswith("worktree ")]
        shd = shadow(repo, 1, "fixture")
        check("shadow materialises INSIDE the slot", os.path.normcase(shd["dir"]),
              os.path.normcase(os.path.join(slot_dir(repo, 1), "fixture")))
        check("... as a lane-shaped tree (configure.py + src/)",
              os.path.exists(os.path.join(shd["dir"], "configure.py"))
              and os.path.isdir(os.path.join(shd["dir"], "src")), True)
        check("... with its OWN repository inside the slot", os.path.isdir(os.path.join(shd["dir"], ".git")),
              True)
        check("... resolving like a lane (rev-parse --show-toplevel)",
              os.path.normcase(g(shd["dir"], "rev-parse", "--show-toplevel")),
              os.path.normcase(shd["dir"]))
        check("... and MAIN registered NO new worktree (nothing outside the slot was touched)",
              [line for line in g(repo, "worktree", "list", "--porcelain").splitlines()
               if line.startswith("worktree ")], wts_before)
        check("shadow refuses a destination OUTSIDE the slot",
              _raises(lambda: shadow(repo, 1, os.path.join(tmp, "outside"))), True)
        check("shadow refuses a non-empty destination", _raises(lambda: shadow(repo, 1, "fixture")), True)
        check("... unless --force replaces it", shadow(repo, 1, "fixture", force=True)["files"] > 0, True)
        _rmtree_force(os.path.join(slot_dir(repo, 1), "fixture"))
        # the containment test compares RESOLVED paths: a lexical prefix test refuses a destination reached
        # through a symlink (and cannot see one that walks out).  Windows needs a privilege for `symlink`,
        # so this row is skipped where the platform refuses it.
        link = os.path.join(tmp, "slot-link")
        try:
            os.symlink(slot_dir(repo, 1), link, target_is_directory=True)
        except (OSError, NotImplementedError, AttributeError):
            link = None
        if link:
            check("shadow accepts a destination inside the slot through a symlinked path",
                  shadow(repo, 1, os.path.join(link, "fixture"))["files"] > 0, True)
            _rmtree_force(os.path.join(slot_dir(repo, 1), "fixture"))

        # (1) THE GOOD CASE: acquire -> branch off main's tip, report matches, no src diff, clean status
        tip = g(repo, "rev-parse", "HEAD")
        info = acquire(repo, "auto/stub", worker="w-good")
        d1 = info["dir"]
        check("acquire picks a free slot", info["slot"], 1)
        check("acquire cuts the branch off main's tip", g(d1, "rev-parse", "HEAD"), tip)
        check("acquire is on the claim's branch", slot_attached_branch(d1), claims.branch_for("auto/stub"))
        check("the slot is locked", read_lock(repo, 1).get("unit"), "auto/stub")
        check("the handed tree verifies current", info["verify"]["ok"], True)
        check("the slot's report matches MAIN exactly", report_matches(repo, d1), True)
        check("acquire finishes the claim-time pending work", info["currency"]["pending"], 0)
        check("... and records the currency proof on the lock",
              (read_lock(repo, 1).get("pending"), read_lock(repo, 1).get("compile_outputs")), (0, True))
        check("no src diff against main", g(d1, "diff", "--name-only", "main", "--", "src"), "")
        check("git status in the slot is clean", g(d1, "status", "--porcelain"), "")
        check("acquire creates the `.used` marker", marker_present(d1), True)
        check("... and the marker is gitignored (the tree stays clean)",
              g(d1, "status", "--porcelain", "-uall"), "")
        # A SHADOW INTO A LANE'S SLOT IS REFUSED: nothing consulted the `.used` marker, so a fixture could
        # be written into a live lane's tree - that lane's `git status` then shows an untracked directory
        # and its landing refuses on a dirty tree.  `--force` is the deliberate override.
        try:
            shadow(repo, 1, "fixture")
            check("shadow refuses a slot a lane holds", "no error", "SystemExit")
        except SystemExit as exc:
            check("shadow refuses a slot a lane holds", "not free" in str(exc), True)
            check("... naming the claim that holds it",
                  "auto/stub" in str(exc) and "w-good" in str(exc), True)
        check("... while --force overrides the occupancy check",
              shadow(repo, 1, "forced", force=True)["files"] > 0, True)
        _rmtree_force(os.path.join(d1, "forced"))
        check("... leaving the lane's tree clean again", g(d1, "status", "--porcelain", "-uall"), "")

        # (2) THE HOSTILE CASE: poison the build tree, then release and re-acquire - it must repair, not lie
        open(os.path.join(d1, "build", "RMHE08", "report.json"), "w").write('{"units": {}}\n')
        open(os.path.join(d1, "build", "RMHE08", "config.json"), "w").write('{"libs": ["OLD"]}\n')
        os.utime(os.path.join(d1, "build", "RMHE08", "config.json"), (500_000, 500_000))
        poisoned = verify(repo, 1)
        check("a poisoned slot is detected as stale", poisoned["ok"], False)
        check("... naming the stale report", any("report.json" in r for r in poisoned["reasons"]), True)
        check("... and the stale split", any("split is stale" in r for r in poisoned["reasons"]), True)
        # acquire refuses on a *live* lock, so release the claim first (as the landing would), then poison
        rel = release(repo, slot=1, unit="auto/stub", rescue=False)
        check("release detaches the slot", slot_attached_branch(d1), None)
        check("release deletes the branch", claims.branch_exists(repo, claims.branch_for("auto/stub")), False)
        check("release clears the lock", read_lock(repo, 1), {})
        check("release removes the `.used` marker", marker_present(d1), False)
        check("release refreshes the warm tree", report_matches(repo, d1), True)
        # poison again, this time while the slot is free, so acquire itself must repair it
        open(os.path.join(d1, "build", "RMHE08", "report.json"), "w").write('{"units": {"stale": true}}\n')
        os.utime(os.path.join(d1, "build", "RMHE08", "config.json"), (500_000, 500_000))
        check("the free slot is now poisoned", verify(repo, 1)["ok"], False)
        good = acquire(repo, "auto/stub-2", worker="w-repair")
        check("acquire detected the poison and re-seeded", good["slot"], 1)
        check("... and refreshed (not handed the lie)", bool(good["refreshed"]), True)
        check("... whatever it hands over is current", good["verify"]["ok"], True)
        check("... the report matches MAIN after the repair", report_matches(repo, d1), True)
        check("... and the poisoned config.json is gone",
              open(os.path.join(d1, "build", "RMHE08", "config.json")).read(), '{"libs": []}\n')
        release(repo, slot=1, unit="auto/stub-2", rescue=False)

        # the marker is atomic: two racing acquires cannot both claim one slot
        check("the marker is created atomically", mark_used(d1, "u", "b"), True)
        check("... a second, racing mark on the same slot is refused", mark_used(d1, "u2", "b2"), False)
        clear_marker(d1)
        check("... and once cleared the slot can be marked again", mark_used(d1, "u3", "b3"), True)
        clear_marker(d1)

        # (3) THE BRANCH INVARIANT: fresh branch off main's tip each time; a held branch is refused
        first = acquire(repo, "auto/stub-a")
        d = first["dir"]
        check("first acquire is at main's tip", g(d, "rev-parse", "HEAD"), tip)
        # a live claim on the slot: acquiring again must not silently reuse it
        try:
            acquire(repo, "auto/stub-b", slot=first["slot"])
            check("a locked slot is refused", "no error", "SystemExit")
        except SystemExit as exc:
            check("a locked slot is refused", "in use" in str(exc) or "REFUSED" in str(exc), True)
        release(repo, slot=first["slot"], unit="auto/stub-a", rescue=False)
        commit(repo, "main moves")
        tip2 = g(repo, "rev-parse", "HEAD")
        second = acquire(repo, "auto/stub-b", slot=first["slot"])
        check("the second acquire cuts a branch off the NEW tip", g(d, "rev-parse", "HEAD"), tip2)
        check("... and it is a different, fresh branch", second["branch"], claims.branch_for("auto/stub-b"))
        check("... while the released branch is gone",
              claims.branch_exists(repo, claims.branch_for("auto/stub-a")), False)
        # Give the held branch a commit main does not have, so it is genuinely UNLANDED (tree differs) - the
        # shape the refusal is for; an empty branch's tree equals main's and is now reclaimable instead.
        commit(d, "unlanded work on the held branch")
        # a slot holding an unlanded branch (lock lost, branch left attached) is surfaced loudly
        clear_lock(repo, first["slot"])          # simulate a crash between detach and unlock
        clear_marker(d)                          # ... and losing the marker too: the HOSTILE case
        check("HOSTILE: a checked-out branch with NO marker and NO lock still reports in use",
              slot_state(repo, first["slot"])["free"], False)
        check("... the worktree branch, not any file, is what decides",
              slot_state(repo, first["slot"])["attached"], claims.branch_for("auto/stub-b"))
        check("... `status` agrees it is not free", status(repo)[0]["free"], False)
        check("... and it is excluded from the free count", free_count(repo), 1)
        # the search must SKIP it and fall through to the next genuinely free slot
        skipped = acquire(repo, "auto/stub-skip")
        check("acquire skips the occupied slot and falls through to the next free one", skipped["slot"], 2)
        release(repo, slot=2, unit="auto/stub-skip", rescue=False)
        try:
            acquire(repo, "auto/stub-c", slot=first["slot"])
            check("a slot holding an unlanded branch is refused", "no error", "SystemExit")
        except SystemExit as exc:
            check("a slot holding an unlanded branch is refused", "never a branch" in str(exc), True)
        # a *stale lock* (branch gone) is reclaimable, not a wedge
        release(repo, slot=first["slot"], branch=claims.branch_for("auto/stub-b"), rescue=False)
        write_lock(repo, first["slot"], {"slot": first["slot"], "unit": "ghost/unit",
                                         "branch": "worker/ghost-gone", "worker": "dead"})
        check("a lock whose branch is gone is stale", lock_stale(repo, first["slot"]), True)
        revived = acquire(repo, "auto/stub-c", slot=first["slot"])
        check("a stale lock is reclaimed, not a wedge", revived["reclaimed"] is not None, True)
        check("... and the slot is now locked by the new claim", read_lock(repo, first["slot"])["unit"],
              "auto/stub-c")
        release(repo, slot=first["slot"], unit="auto/stub-c", rescue=False)
        # a crash leaves a marker on a DETACHED worktree: reclaimable, never a permanent wedge
        mark_used(d, "ghost", "ghost-gone")
        check("a `.used` marker on a detached worktree is stale",
              slot_state(repo, first["slot"])["marker_stale"], True)
        check("... and the slot reads free", slot_state(repo, first["slot"])["free"], True)
        reclaimed2 = acquire(repo, "auto/reclaim", slot=first["slot"])
        check("... acquire reclaims it and hands it out",
              (marker_present(d), reclaimed2["slot"]), (True, first["slot"]))
        release(repo, slot=first["slot"], unit="auto/reclaim", rescue=False)

        # (4) THE CAP: fill every slot and the free count is zero
        acquire(repo, "auto/cap-1", slot=1)
        acquire(repo, "auto/cap-2", slot=2)
        check("no slot is free at the cap", free_slots(repo), [])
        check("capacity_error names the cap", "concurrency cap" in (capacity_error(repo) or ""), True)
        try:
            acquire(repo, "auto/cap-3")
            check("acquire refuses past the cap", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire refuses past the cap", "concurrency cap" in str(exc), True)
        release(repo, slot=1, unit="auto/cap-1", rescue=False)
        release(repo, slot=2, unit="auto/cap-2", rescue=False)
        check("releasing frees a slot", len(free_slots(repo)), 2)

        # a slot reset discards scratch and stale source edits, keeps the warm trees - but ONLY when the
        # release is deliberate: the dirty tree is exactly what fails the release closed (E2).
        info2 = acquire(repo, "auto/dirty", slot=1)
        dd = info2["dir"]
        open(os.path.join(dd, "src", "auto", "stub.c"), "w").write("/* dirty edit */\n")
        open(os.path.join(dd, "scratch.txt"), "w").write("junk\n")
        os.makedirs(os.path.join(dd, "out", "scratch"), exist_ok=True)
        open(os.path.join(dd, "out", "scratch", "junk.bin"), "wb").write(b"x")
        try:
            release(repo, slot=1, unit="auto/dirty", rescue=False)
            check("a dirty tree refuses the release", "no error", "SystemExit")
        except SystemExit as exc:
            check("a dirty tree refuses the release", "REFUSED release slot 1" in str(exc), True)
            check("... naming the tracked edit it would discard", "src/auto/stub.c" in str(exc), True)
            check("... and the untracked scratch", "scratch.txt" in str(exc), True)
            check("... and offering the deliberate override", "--force" in str(exc), True)
        check("... and nothing was touched by the refusal",
              "dirty edit" in open(os.path.join(dd, "src", "auto", "stub.c")).read(), True)
        forced = release(repo, slot=1, unit="auto/dirty", rescue=False, force=True)
        check("--force releases anyway", forced["detached"], True)
        check("... and records what it overrode",
              any("dirty" in line for line in forced["overridden"]), True)
        check("a dirty src/ is reset to main", "dirty edit" not in open(
            os.path.join(dd, "src", "auto", "stub.c")).read(), True)
        check("stray scratch is discarded", os.path.exists(os.path.join(dd, "scratch.txt")), False)
        check("stray dirs are discarded", os.path.exists(os.path.join(dd, "out")), False)
        check("the warm build tree survives the reset",
              os.path.exists(os.path.join(dd, "build", "RMHE08", "report.json")), True)
        check("the orig payload survives the reset",
              os.path.exists(os.path.join(dd, "orig", "RMHE08", "sys", "main.dol")), True)
        check("the m2c submodule survives the reset",
              os.path.exists(os.path.join(dd, "tools", "m2c", "m2c.py")), True)
        check("the slot is detached and free after release",
              (slot_attached_branch(dd), read_lock(repo, 1)), (None, {}))
        check("... and its `.used` marker is gone", marker_present(dd), False)

        # integration: the *claim path* takes a slot and its release returns it (claims.py's own selftest
        # covers the classic path; this is the seam between the two tools)
        claims.save_registry(repo, {})
        info3 = claims.claim("auto/slotted", repo, "w-int", False, cwd=repo)
        check("claims.claim takes a slot when a pool exists", info3.get("slot") in (1, 2), True)
        check("... the worktree is the slot dir", info3["worktree"], slot_dir(repo, info3["slot"]))
        check("... the registry records the slot",
              claims.load_registry(repo)["auto/slotted"].get("slot"), info3["slot"])
        rel3 = claims.release("auto/slotted", repo, force=False, dry_run=False)
        check("claims.release returns the slot, not a worktree removal", rel3.get("complete"), True)
        check("... the slot directory persists", os.path.isdir(slot_dir(repo, info3["slot"])), True)
        check("... and it is detached and unlocked",
              (slot_attached_branch(slot_dir(repo, info3["slot"])), read_lock(repo, info3["slot"])),
              (None, {}))
        check("... the branch is deleted by the return",
              claims.branch_exists(repo, claims.branch_for("auto/slotted")), False)

        # status reports every slot
        rows = status(repo)
        check("status lists every slot", [r["slot"] for r in rows], [1, 2])
        check("status reports the build tree current on a clean pool", all(r["build_ok"] for r in rows), True)
        check("status keeps `used` and the worktree branch as separate readings",
              all("used" in r and "attached" in r and "marked" in r for r in rows), True)

        # --- E1: the `.used` sentinel records its OWNER, and `status` shows it -------------------------
        check("the owner label is the worker label when there is one",
              owner_label("auto/own", "worker/own-9f2a", "w-own"), "w-own")
        check("... else the branch slug (stable across rounds)",
              owner_label("auto/own", "worker/own-9f2a"), "own-9f2a")
        check("... else the unit", owner_label("auto/own", None), "auto/own")
        own = acquire(repo, "auto/own", worker="w-own")
        d_own = own["dir"]
        sentinel = marker_info(d_own)
        check("the sentinel records the owner label", sentinel.get("owner"), "w-own")
        check("... the unit", sentinel.get("unit"), "auto/own")
        check("... the branch", sentinel.get("branch"), claims.branch_for("auto/own"))
        check("... and the pid that wrote it", str(sentinel.get("pid") or "").isdigit(), True)
        check("... as key=value with the owner first",
              open(marker_path(d_own), encoding="utf-8").read().splitlines()[0], "owner=w-own")
        check("the sentinel is not reported as legacy", sentinel.get("legacy"), False)
        check("marker_owner reads it back", marker_owner(d_own), "w-own")
        check("slot_state reports the owner", slot_state(repo, own["slot"])["owner"], "w-own")
        own_row = next(r for r in status(repo) if r["slot"] == own["slot"])
        check("`status` reports the owner", own_row["owner"], "w-own")
        check("... and shows no live run when the registry names none here", own_row["run"], None)
        # a legacy positional body still parses, and is explicitly *not* an owner
        with open(marker_path(d_own), "w", encoding="utf-8") as fh:
            fh.write("%d auto/own worker/own-9f2a\n" % os.getpid())
        legacy = marker_info(d_own)
        check("a legacy positional body reads as legacy", legacy.get("legacy"), True)
        check("... with its unit parsed best-effort", legacy.get("unit"), "auto/own")
        check("... and no owner, so no conflict", marker_claim_conflict(d_own, "x", 1), None)
        check("an ownerless sentinel is not a conflict either",
              marker_info(d_own) and marker_claim_conflict(d_own, "someone-else", own["slot"]), None)
        clear_marker(d_own)
        mark_used(d_own, "auto/own", claims.branch_for("auto/own"), "")
        check("... a marker written with no owner label names nobody",
              marker_claim_conflict(d_own, "someone-else", own["slot"]), None)
        clear_marker(d_own)
        mark_used(d_own, "auto/own", claims.branch_for("auto/own"), "w-own")
        release(repo, slot=own["slot"], unit="auto/own")

        # --- E1/E3: a sentinel that names ANOTHER claim is refused at launch -------------------------
        free = acquire(repo, "auto/free", slot=1)
        d_free = free["dir"]
        release(repo, slot=1, unit="auto/free")            # detached, unlocked, marker gone: acquirable
        check("the slot is genuinely free before the foreign marker",
              slot_state(repo, 1)["free"], True)
        check("a foreign marker can be planted on a free slot",
              mark_used(d_free, "auto/other", "worker/other-9f2a", "w-other"), True)
        conflict = marker_claim_conflict(d_free, "auto/mine", 1)
        check("the sentinel is read back as the other claim's", marker_owner(d_free), "w-other")
        check("... and `slot_state` says so", slot_state(repo, 1)["owner"], "w-other")
        check("... including that it is a stale remnant", slot_state(repo, 1)["marker_stale"], True)
        check("the conflict names the owner", "w-other" in (conflict or ""), True)
        check("... and the override to use", "--force" in (conflict or ""), True)
        try:
            acquire(repo, "auto/mine")
            check("acquire REFUSES a sentinel owned by another claim", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire REFUSES a sentinel owned by another claim", "w-other" in str(exc), True)
            check("... and says why (HEAD detached mid-run)", "detached mid-run" in str(exc), True)
        check("... the refusal touched nothing (the other claim's marker survives",
              marker_owner(d_free), "w-other")
        taken = acquire(repo, "auto/mine", force=True)
        check("--force takes the slot deliberately", taken["slot"], 1)
        check("... and the sentinel is rewritten with the new owner",
              marker_owner(d_free) != "w-other", True)
        check("... naming the new claim", marker_owner(d_free), owner_label("auto/mine", taken["branch"]))
        release(repo, slot=1, unit="auto/mine")

        # --- E2/E3: the run registry is the live-lane signal -----------------------------------------
        live_slot = acquire(repo, "auto/live", slot=1)
        d_live = live_slot["dir"]
        cfg_home = os.path.join(tmp, "claude-config")
        reg_root = os.path.join(cfg_home, SESSIONS_DIRNAME)
        os.makedirs(reg_root)
        check("the harness' session registry is discovered under the config dir",
              run_registries(cfg_home), [reg_root])
        check("... and a config dir with no registry yields no signal", run_registries(tmp), [])
        run_id = "11111111-2222-3333-4444-555555555555"
        gone = subprocess.Popen([sys.executable, "-c", "pass"])
        gone.wait()
        check("a finished process is not alive", pid_alive(gone.pid), False)
        check("this process is alive", pid_alive(os.getpid()), True)

        def write_run(state="running", cwd=None):
            # `finished` is a session whose process has exited: the record stays, the pid is dead
            with open(os.path.join(reg_root, "%s.json" % run_id), "w", encoding="utf-8") as fh:
                json.dump({"pid": os.getpid() if state == "running" else gone.pid, "sessionId": run_id,
                           "cwd": cwd or d_live, "name": "worker: fix the thing", "kind": "headless",
                           "status": "busy"}, fh)

        write_run()
        check("a RUNNING record is a live run", [r["run_id"] for r in live_runs(reg_root)], [run_id])
        check("... discovered through the temp dir too",
              [r["run_id"] for r in live_runs(config=cfg_home)], [run_id])
        check("... carrying its cwd and session name",
              (live_runs(reg_root)[0]["cwd"], live_runs(reg_root)[0]["session"]),
              (d_live, "worker: fix the thing"))
        check("a run whose cwd is the slot is IN the slot",
              [r["run_id"] for r in runs_in_slot(d_live, reg_root)], [run_id])
        check("... and one working in another slot is not",
              runs_in_slot(slot_dir(repo, 2), reg_root), [])
        write_run(cwd=os.path.join(d_live, "src"))
        check("... a lane launched in a SUBdirectory is in the slot too",
              [r["run_id"] for r in runs_in_slot(d_live, reg_root)], [run_id])
        write_run()
        live_row = next(r for r in status(repo, registry=reg_root) if r["slot"] == 1)
        check("`status` reports the live run", (live_row["run"] or {}).get("run_id"), run_id)
        check("... and says a run holds the slot", "RUNNING Claude session" in live_row["why"], True)
        try:
            release(repo, slot=1, unit="auto/live", registry=reg_root)
            check("release REFUSES while a run is working in the slot", "no error", "SystemExit")
        except SystemExit as exc:
            check("release REFUSES while a run is working in the slot",
                  "REFUSED release slot 1" in str(exc), True)
            check("... NAMING the run", run_id in str(exc), True)
            check("... and its session", "worker: fix the thing" in str(exc), True)
            check("... and saying a lock cannot see a lane", "a lock cannot see a lane" in str(exc), True)
            check("... and offering --force", "--force" in str(exc), True)
        check("... and HEAD was not detached by the refusal",
              slot_attached_branch(d_live), claims.branch_for("auto/live"))
        # the session's process exits (a crash leaves the record behind): the record is dead, the release proceeds
        write_run(state="finished")
        check("a record whose process has exited retires it", live_runs(reg_root), [])
        check("the finished run is no longer in the slot", runs_in_slot(d_live, reg_root), [])
        write_run()
        forced_live = release(repo, slot=1, unit="auto/live", registry=reg_root, force=True)
        check("--force releases under a live run", forced_live["detached"], True)
        check("... recording the live run it overrode",
              any(run_id in line for line in forced_live["overridden"]), True)
        check("... and naming the lane in the recorded reason",
              any("still working" in line for line in forced_live["overridden"]), True)

        # --- E2/E3: commits no branch reaches refuse the release --------------------------------------
        held = acquire(repo, "auto/orphan", slot=1)
        d_orph = held["dir"]
        commit(d_orph, "a commit on the claim's branch")
        check("a commit the claim's branch reaches is not orphaned", orphaned_commits(d_orph), [])
        check("... so the release proceeds", release_blockers(repo, 1, d_orph), [])
        g(d_orph, "checkout", "-q", "--detach")          # the 2026-09-28 shape: detached, still working
        detached_head = slot_head(d_orph)
        commit(d_orph, "a commit no branch reaches")
        check("a detached commit is orphaned", len(orphaned_commits(d_orph)), 1)
        try:
            release(repo, slot=1, unit="auto/orphan")
            check("release REFUSES to orphan commits", "no error", "SystemExit")
        except SystemExit as exc:
            check("release REFUSES to orphan commits", "no branch reaches" in str(exc), True)
            check("... naming the commit", orphaned_commits(d_orph)[0][:8] in str(exc), True)
        check("... and the refusal left HEAD where the lane put it",
              slot_head(d_orph) != detached_head, True)
        check("... i.e. still detached on the orphan", slot_attached_branch(d_orph), None)
        orphan_forced = release(repo, slot=1, unit="auto/orphan", force=True)
        check("--force releases an orphaned HEAD", orphan_forced["detached"], True)
        check("... recording what it overrode",
              any("no branch reaches" in line for line in orphan_forced["overridden"]), True)

        # --- CLAUDE.md is an ordinary tracked file: an edit is dirt, and a reset puts the committed text back ----
        blocky = acquire(repo, "auto/blocky", slot=1)
        d_blocky = blocky["dir"]
        check("a freshly acquired slot has a clean tree", slot_dirty(d_blocky), [])
        with open(os.path.join(d_blocky, "CLAUDE.md"), "a", encoding="utf-8", newline="") as fh:
            fh.write("an edit \u2014\n")
        check("a CLAUDE.md edit IS dirt, with no special casing", any("CLAUDE.md" in r for r in slot_dirty(d_blocky)), True)
        release(repo, slot=1, unit="auto/blocky", force=True)
        check("... and the reset puts the committed CLAUDE.md back",
              "an edit" in open(os.path.join(d_blocky, "CLAUDE.md"), encoding="utf-8").read(), False)

        # --- E2/E3: the SEAM - the claim path's teardown is the release that caused the incident --------
        # 2026-09-28's release was `claims.py release`, not a bare `slots.py release`, so the guard has to
        # hold through that path too: the slot step fails, the slot is left exactly where the live lane put
        # it, and `--force` is what tears it down.
        claims.save_registry(repo, {})
        seam = claims.claim("auto/seam", repo, "w-seam", False, cwd=repo)
        d_seam = seam["worktree"]
        check("claims.claim took a slot for the seam fixture", seam.get("slot") in (1, 2), True)
        write_run(cwd=d_seam)
        rel_seam = claims.release("auto/seam", repo, force=False, dry_run=False, run_registry=reg_root)
        check("claims.release refuses while a run works in the slot", rel_seam["complete"], False)
        step_text = json.dumps([(s["label"], s["status"], s["why"]) for s in rel_seam["steps"]])
        check("... naming the run in the failed step", run_id in step_text, True)
        check("... and saying the run is still working in it", "still working" in step_text, True)
        check("... and the teardown stops before it clears the claim (registry entry kept)",
              any(s["label"].startswith("registry entry") and s["status"] == "skipped"
                  and "did not complete" in s["why"] for s in rel_seam["steps"]), True)
        check("... and the slot is still on its branch, exactly as the lane left it",
              slot_attached_branch(d_seam), claims.branch_for("auto/seam"))
        check("... so the claim's branch survives",
              claims.branch_exists(repo, claims.branch_for("auto/seam")), True)
        rel_forced = claims.release("auto/seam", repo, force=True, dry_run=False, run_registry=reg_root)
        check("claims.release --force tears it down anyway", rel_forced["complete"], True)
        check("... clearing the claim as well",
              any(s["label"].startswith("registry entry") and s["status"] == "done"
                  for s in rel_forced["steps"]), True)
        check("... and the slot is detached and unlocked",
              (slot_attached_branch(d_seam), read_lock(repo, seam["slot"])), (None, {}))

        # --- SPAWN: the slot is an explicit launch parameter and the profile follows the kind ----------
        # (the defect: the launcher left both choices to the lane, and three tooling lanes were told
        # `decompiler` - unit policy they could never satisfy. `spawn` makes both explicit.)
        check("the kind mapping is the whole table", KIND_PROFILE,
              {"unit": "surveyor", "fix": "fixer", "merge": "merger", "tooling": "worker",
               "docs": "worker", "review": "codereviewer", "scout": "scout", "plan": "planner"})
        check("PROFILES is the table's values, not a second copy", PROFILES,
              tuple(sorted(set(KIND_PROFILE.values()))))
        for _kind, _profile in sorted(KIND_PROFILE.items()):
            check("kind %s -> %s" % (_kind, _profile), profile_for_kind(_kind), _profile)
        try:
            profile_for_kind("bogus")
            check("an unknown kind is refused (never guessed)", "no error", "SystemExit")
        except SystemExit as exc:
            check("an unknown kind is refused (never guessed)", "unknown kind" in str(exc), True)
            check("... listing every valid kind", all(k in str(exc) for k in KIND_PROFILE), True)
            # the same refusal is raised from `claims.py claim --kind` and `queue.py next --kind`, neither of
            # which spawns anything, so it must not say it is a spawn refusal
            check("... and not claiming to be a spawn", "REFUSED spawn" in str(exc), False)

        # (a) BY NUMBER: takes THAT slot, and the line's cwd is it and its agent is the mapped profile
        sp = spawn(repo, "tooling", slot=1, unit="lane/spawn-tooling", task="Wire up the tool.")
        check("spawn takes the slot BY NUMBER", sp["slot"], 1)
        check("... cwd is that slot", sp["path"].replace("\\", "/"),
              slot_dir(repo, 1).replace("\\", "/"))
        check("... the kind maps to `worker`", sp["agent"], "worker")
        check("... the slot is on the claim's branch", slot_attached_branch(slot_dir(repo, 1)), sp["branch"])
        check("... the line is a headless claude call with that agent, run in the slot",
              sp["spawnLine"].startswith("cd %s && claude --agent worker "
                                          % slot_dir(repo, 1).replace("\\", "/")), True)
        check("... carrying a session id the orchestrator can resume",
              "--session-id %s" % sp["sessionId"] in sp["spawnLine"], True)
        check("... and carries task/slot/path/agent/kind/spawnLine",
              all(k in sp for k in ("slot", "path", "agent", "kind", "spawnLine")), True)
        check("... the block carries the `rev-parse --show-toplevel` self-check",
              "git rev-parse --show-toplevel" in sp["spawnLine"], True)
        check("... and the STOP-and-report rule", "STOP and report it" in sp["spawnLine"], True)
        check("... and the write-nothing-outside rule",
              "nothing" in sp["spawnLine"].lower() and "outside it" in sp["spawnLine"].lower(), True)
        check("... and the claims.py release prohibition", "claims.py release" in sp["spawnLine"], True)
        check("... and the do-not-land rule", "Do not land" in sp["spawnLine"], True)
        check("... and the claim-time currency proof the lane can see",
              "proven current at handover" in sp["spawnLine"]
              and "pending steps: 0" in sp["spawnLine"], True)
        check("... the kind/profile are recorded in the slot's lock",
              (read_lock(repo, 1).get("kind"), read_lock(repo, 1).get("agent")), ("tooling", "worker"))

        # (b) BY NUMBER, occupied: a slot holding an unlanded branch is refused
        try:
            spawn(repo, "unit", slot=1, unit="lane/spawn-intruder", task="x")
            check("spawn BY NUMBER refuses a slot holding an unlanded branch", "no error", "SystemExit")
        except SystemExit as exc:
            check("spawn BY NUMBER refuses a slot holding an unlanded branch",
                  "never a branch" in str(exc), True)
        check("... and the held branch is untouched", slot_attached_branch(slot_dir(repo, 1)), sp["branch"])
        plain_row = next(r for r in claims.claims_view(repo) if r["branch"] == sp["branch"])
        check("a spawned lane with no unit set lists under its lock's name, with no `units` key",
              (plain_row["unit"], plain_row.get("registered"), "units" in plain_row),
              ("lane/spawn-tooling", False, False))
        release(repo, slot=1, unit="lane/spawn-tooling", rescue=False)

        # (a2) the lane's UNIT SET: the task's `Units:` line is recorded on the lock and `claims.py list` reads
        # it back (the network pilot: a spawned lane listed as `(unregistered)`, so integrate saw no owner)
        sp_u = spawn(repo, "fix", slot=1, unit="lane/spawn-units",
                     task="Lane L9.\nUnits: Net/alpha (Matching), Net/beta, Net/alpha.\nOwned headers: x.h")
        check("spawn records the task's Units: line on the lock", read_lock(repo, 1).get("units"),
              ["Net/alpha", "Net/beta"])
        check("... and returns it", (sp_u["units"], sp_u["unresolved_units"]), (["Net/alpha", "Net/beta"], []))
        lane_row = next(r for r in claims.claims_view(repo) if r["branch"] == sp_u["branch"])
        check("claims list shows the lane by name with its units, not as (unregistered)",
              (lane_row["unit"], lane_row.get("registered"), lane_row.get("units"), lane_row.get("kind")),
              ("lane/spawn-units", False, ["Net/alpha", "Net/beta"], "fix"))
        check("... and the registry teardown sweep skips it (no registry row to release)",
              [s["why"] for s in claims.release_merged(repo, dry_run=True)["skipped"]
               if s["branch"] == sp_u["branch"]], ["no registry entry"])
        release(repo, slot=1, unit="lane/spawn-units", rescue=False)
        sp_x = spawn(repo, "fix", slot=1, unit="lane/spawn-explicit", task="Units: Net/ignored",
                     units=["Net/gamma"])
        check("an explicit --units wins over the task's line", read_lock(repo, 1).get("units"), ["Net/gamma"])
        release(repo, slot=1, unit="lane/spawn-explicit", rescue=False)
        check("... and the release clears the unit set with the lock", read_lock(repo, 1), {})
        del sp_x

        # (c) an unknown kind is refused before any slot is touched
        try:
            spawn(repo, "not-a-kind", unit="lane/spawn-bad")
            check("spawn refuses an unknown kind", "no error", "SystemExit")
        except SystemExit as exc:
            check("spawn refuses an unknown kind", "unknown kind" in str(exc), True)
        check("... without taking a slot", free_count(repo), 2)

        # (d) NO --slot: skips the occupied slot and takes the FIRST genuinely free one, naming it
        hold = spawn(repo, "tooling", slot=1, unit="lane/spawn-hold", task="hold")
        freed = spawn(repo, "docs", unit="lane/spawn-first-free", task="write the doc")
        check("spawn with no --slot takes the first genuinely free slot", freed["slot"], 2)
        check("... naming it", freed["path"].replace("\\", "/"), slot_dir(repo, 2).replace("\\", "/"))
        check("... and `docs` maps to the `worker` fallback", freed["agent"], "worker")
        release(repo, slot=2, unit="lane/spawn-first-free", rescue=False)
        unit_sp = spawn(repo, "unit", unit="lane/spawn-unit", task="Reconstruct the unit.")
        check("... and `unit` maps to `surveyor`", unit_sp["agent"], "surveyor")
        check("... on the first free slot again", unit_sp["slot"], 2)
        release(repo, slot=2, unit="lane/spawn-unit", rescue=False)
        release(repo, slot=1, unit="lane/spawn-hold", rescue=False)

        # (e) --task-file: the printed line carries the file's text
        tf = os.path.join(tmp, "spawn-task.txt")
        with open(tf, "w", encoding="utf-8") as fh:
            fh.write("Read this and do X.")
        tfs = spawn(repo, "fix", unit="lane/spawn-taskfile", task_file=tf)
        check("spawn reads the task text from --task-file", "Read this and do X." in tfs["spawnLine"], True)
        check("... and `fix` maps to `fixer`", tfs["agent"], "fixer")
        release(repo, slot=tfs["slot"], unit="lane/spawn-taskfile", rescue=False)
        check("the pool is left as it was found (both slots free)", free_count(repo), 2)

        # --- RECLAIM: a slot holding a LANDED branch is bookkeeping, not work in progress ----------------
        # The refusal "a slot holds a directory, never a branch" is load-bearing for an *unlanded* branch,
        # but after a landing the branch's content is in main and detaching/deleting it by hand cost a round
        # trip every time.  These fixtures pin both directions, the rescue-first ordering, and fail-closed.

        def land_into_main(branch, rel="f.txt", msg="land the branch"):
            """Simulate the GATE landing (`land.py land --branch`): apply the branch's content onto main as a
            NEW commit, so the branch stays *ahead of main by its own commit* while its TREE now equals
            main's - exactly the shape the merge-tree equality must still call applied, because it compares
            trees and not commits.

            A pathspec on the commit keeps unrelated untracked state (the claims registry) out of main, so
            the only tree difference between main and the branch is the change itself.
            """
            g(repo, "cherry-pick", "--no-commit", branch)
            g(repo, "commit", "-q", "-m", msg, "--", rel)

        # (e1) the merge-tree read itself: a conflict or unreadable output is ambiguity, never "applied"
        _oid40 = "a" * 40
        check("merge-tree: a clean run yields its tree OID", _merge_tree_oid(0, _oid40 + "\n"), _oid40)
        check("... a conflict (nonzero exit) is NOT a tree", _merge_tree_oid(1, _oid40 + "\n"), None)
        check("... an unparseable first line is ambiguity", _merge_tree_oid(0, "CONFLICT (content)\n"), None)
        check("... empty output is ambiguity", _merge_tree_oid(0, ""), None)
        check("... and a merge-tree that cannot resolve the branch is unknown",
              branch_fully_applied(repo, "worker/ghost-does-not-exist"), None)

        # (c) a genuinely FREE slot is untouched: reclaim refuses and writes nothing
        rescue_before = g(repo, "for-each-ref", "--format=%(refname)", "refs/rescue/")
        free_v = reclaim_slot(repo, 1)
        check("reclaim leaves a genuinely free slot untouched", free_v["reclaimed"], False)
        check("... saying it holds no branch", "no branch" in free_v["reason"], True)
        check("... and parking no rescue ref",
              g(repo, "for-each-ref", "--format=%(refname)", "refs/rescue/"), rescue_before)
        check("... which the read-only verdict agrees with", reclaim_verdict(repo, 1)["reclaimable"], False)

        # (a)+(d) a slot holding a LANDED branch is reclaimed and taken, rescue ref FIRST
        land = acquire(repo, "auto/landed", slot=1)
        d_land = land["dir"]
        lbranch = land["branch"]
        commit(d_land, "a landed change")
        land_into_main(lbranch, msg="land: a landed change")
        clear_lock(repo, 1)                      # the claim was released; the branch/worktree remain
        ltip = g(repo, "rev-parse", lbranch)
        check("the branch's content is now fully in main (the free test)",
              branch_fully_applied(repo, lbranch), True)
        # THE GATE-ROUTE PIN: the branch is still ahead of main by its own commit (a commit-based test would
        # call it unlanded), yet its TREE equals main's - which is why the one merge-tree rule is enough.
        check("... the GATE-landed branch is still ahead of main by its own commits",
              claims.commits_ahead(repo, lbranch) >= 1, True)
        check("... and merge-tree really equals main^{tree}", merge_tree_of(repo, lbranch),
              g(repo, "rev-parse", "main^{tree}"))
        check("the read-only verdict confirms the landed branch", reclaim_verdict(repo, 1)["reclaimable"], True)
        pv = preview(repo, "auto/after-land", slot=1)
        check("a dry run predicts the reclaim without doing it",
              (pv.get("reclaim") or {}).get("branch"), lbranch)
        check("... and touches nothing (the branch is still attached)",
              slot_attached_branch(d_land), lbranch)
        check("... so the landed branch is still there", claims.branch_exists(repo, lbranch), True)
        taken = acquire(repo, "auto/after-land", slot=1)
        check("acquire RECLAIMS a slot holding a landed branch and takes it", taken["slot"], 1)
        check("... the slot now holds the NEW claim's branch", slot_attached_branch(d_land), taken["branch"])
        rec = taken.get("landed_reclaim") or {}
        check("... reporting the reclaim", rec.get("reclaimed"), True)
        lref = claims.rescue_ref_name("auto/landed")
        check("... the rescue ref exists", claims.rescue_exists(repo, "auto/landed"), lref)
        check("... pointing at the branch tip", g(repo, "rev-parse", lref), ltip)
        check("... and the landed branch is deleted", claims.branch_exists(repo, lbranch), False)
        lsteps = [s["step"] for s in rec.get("steps", [])]
        check("... the rescue ref is parked BEFORE the detach and the delete",
              lsteps.index("rescue") < lsteps.index("detach") < lsteps.index("delete-branch"), True)
        check("... and the one line says what it did and why", "RECLAIMED" in (rec.get("line") or ""), True)
        release(repo, slot=1, unit="auto/after-land", rescue=False)

        # (a2) the named action `slots.py reclaim` does the same thing deliberately, for one slot
        act = acquire(repo, "auto/act", slot=1)
        d_act = act["dir"]
        commit(d_act, "action landed")
        land_into_main(act["branch"], msg="land: action landed")
        clear_lock(repo, 1)
        atip = g(repo, "rev-parse", act["branch"])
        check("the action's branch is fully applied", branch_fully_applied(repo, act["branch"]), True)
        aout = reclaim_slot(repo, 1)
        check("the explicit reclaim action reclaims a landed slot", aout["reclaimed"], True)
        check("... freeing the slot for the next acquire", slot_state(repo, 1)["free"], True)
        check("... parking the rescue ref at the branch tip",
              g(repo, "rev-parse", claims.rescue_ref_name("auto/act")), atip)
        check("... and deleting the branch", claims.branch_exists(repo, act["branch"]), False)

        # (b) an UNLANDED branch is still refused, with today's message, verbatim
        unl = acquire(repo, "auto/unlanded", slot=1)
        d_unl = unl["dir"]
        commit(d_unl, "definitely not landed")
        clear_lock(repo, 1)
        check("the unlanded branch is NOT applied", branch_fully_applied(repo, unl["branch"]), False)
        v_unl = reclaim_slot(repo, 1)
        check("reclaim REFUSES an unlanded branch", v_unl["reclaimed"], False)
        check("... with the test's own verdict", v_unl["applied"], False)
        try:
            acquire(repo, "auto/unlanded-2", slot=1)
            check("acquire still refuses an unlanded branch", "no error", "SystemExit")
        except SystemExit as exc:
            check("acquire still refuses an unlanded branch", "it still has branch" in str(exc), True)
            check("... keeping the rule verbatim",
                  "a slot holds a directory, never a branch" in str(exc), True)
        check("... and the unlanded branch is untouched", slot_attached_branch(d_unl), unl["branch"])
        check("... with no rescue ref parked for it", claims.rescue_exists(repo, "auto/unlanded"), None)
        check("... and `status` names the missing record",
              "missing record" in slot_state(repo, 1)["record_note"], True)
        # a record that names a branch the slot has left is STALE, and `status` says so with the remedy
        write_lock(repo, 1, {"slot": 1, "unit": "ghost/unit", "branch": "worker/ghost-gone",
                             "base": "0" * 40})
        check("... a stale record is named", "stale record" in slot_state(repo, 1)["record_note"], True)
        check("... with its remedy in `why`", "acquire --slot 1" in slot_state(repo, 1)["why"], True)
        clear_lock(repo, 1)
        release(repo, slot=1, unit="auto/unlanded", rescue=False)

        # (b2) a MISSING `.used` marker is a crash remnant, not a claim: a LANDED branch is still reclaimable
        nm = acquire(repo, "auto/nomark-land", slot=1)
        commit(nm["dir"], "nomark landed")
        land_into_main(nm["branch"], msg="land: nomark landed")
        clear_lock(repo, 1)
        clear_marker(nm["dir"])                  # crash remnant: branch attached, marker MISSING
        check("a MISSING-marker slot still shows its branch attached",
              slot_state(repo, 1)["attached"], nm["branch"])
        check("... with the marker gone", marker_present(nm["dir"]), False)
        check("... and the branch proven applied", branch_fully_applied(repo, nm["branch"]), True)
        nm_taken = acquire(repo, "auto/nomark-after", slot=1)
        check("acquire RECLAIMS a MISSING-marker slot whose branch is applied", nm_taken["slot"], 1)
        check("... and takes it", slot_attached_branch(nm["dir"]), nm_taken["branch"])
        check("... with the rescue ref parked", claims.rescue_exists(repo, "auto/nomark-land"),
              claims.rescue_ref_name("auto/nomark-land"))
        check("... and the old branch deleted", claims.branch_exists(repo, nm["branch"]), False)
        release(repo, slot=1, unit="auto/nomark-after", rescue=False)

        # (b3) ... but the same MISSING marker next to an UNLANDED branch still refuses
        nm2 = acquire(repo, "auto/nomark-unl", slot=1)
        commit(nm2["dir"], "nomark unlanded")
        clear_lock(repo, 1)
        clear_marker(nm2["dir"])
        nm2_v = reclaim_slot(repo, 1)
        check("a MISSING-marker slot holding an UNLANDED branch still refuses", nm2_v["reclaimed"], False)
        check("... with the test's verdict", nm2_v["applied"], False)
        check("... and the branch untouched", slot_attached_branch(nm2["dir"]), nm2["branch"])
        release(repo, slot=1, unit="auto/nomark-unl", rescue=False)

        # (c) DEBRIS: no branch checked out, a `.used` sentinel, and NO registry row - a crash remnant
        db = acquire(repo, "auto/debris", slot=1)
        db_branch = db["branch"]
        clear_lock(repo, 1)
        g(db["dir"], "checkout", "-q", "--detach", g(repo, "rev-parse", "HEAD"))   # at main's tip
        g(repo, "branch", "-D", db_branch)      # "no branch": the ref is gone, the sentinel survives
        check("a detached sentinel with no claim is named `debris`", slot_state(repo, 1)["state"], "debris")
        check("... and reads free so it can never wedge the pool", slot_state(repo, 1)["free"], True)
        db_v = reclaim_verdict(repo, 1)
        check("... and the verdict says reclaimable debris",
              (db_v["reclaimable"], db_v["kind"]), (True, "debris"))
        db_out = reclaim_slot(repo, 1)
        check("reclaim clears the debris sentinel", (db_out["reclaimed"], db_out["kind"]), (True, "debris"))
        check("... with no branch to rescue", db_out["rescue_ref"], None)
        check("... and the sentinel is gone", marker_present(db["dir"]), False)
        check("... naming the state in its one line", "debris" in (db_out["line"] or ""), True)

        # (d) a sentinel WITH a live claim is refused - someone's work is in it
        cl = acquire(repo, "auto/claimed", slot=1)
        check("a live lock makes the slot `claimed`", slot_state(repo, 1)["state"], "claimed")
        cl_v = reclaim_slot(repo, 1)
        check("reclaim REFUSES a slot a live claim holds", cl_v["reclaimed"], False)
        check("... naming the live claim", "live claim" in cl_v["reason"], True)
        check("... and leaving the branch attached", slot_attached_branch(cl["dir"]), cl["branch"])
        release(repo, slot=1, unit="auto/claimed", rescue=False)

        # (d2) SLOT 2'S REAL SHAPE (measured 2026-09-28): marker present, branch checked out and alive, its
        # worktree missing from `git worktree list`, and the ONLY evidence of life a REGISTRY row whose
        # progress advances.  The registry row - the same liveness record `claims.expire`/`claim_status` read
        # - must refuse, even with the lock cleared.  ("The label is not the measurement.")
        l2 = acquire(repo, "auto/live2", slot=1)
        claims.save_registry(repo, {"network/dwci-band": {
            "branch": l2["branch"], "slot": 1, "worker": "loop-6", "worktree": l2["dir"],
            "claimed_at": "2026-09-28T18:00:00"}})
        clear_lock(repo, 1)                      # no owner string on the lock: the registry row is the evidence
        check("a registry-held slot reads `claimed`", slot_state(repo, 1)["state"], "claimed")
        check("... and is NOT free", slot_state(repo, 1)["free"], False)
        l2_v = reclaim_slot(repo, 1)
        check("reclaim REFUSES a registry-held slot (progress moves = life)", l2_v["reclaimed"], False)
        check("... naming the live claim", "live claim" in l2_v["reason"], True)
        try:
            acquire(repo, "auto/live2b", slot=1)
            check("... and acquire refuses it too", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and acquire refuses it too", "it still has branch" in str(exc), True)
        claims.save_registry(repo, {})
        release(repo, slot=1, unit="auto/live2", branch=l2["branch"], rescue=False)

        # (d3) the MIRROR: a registry row with NO sentinel still counts as in use (never hand a live branch
        # to a new lane)
        mir = acquire(repo, "auto/mirror", slot=1)
        claims.save_registry(repo, {"x/y": {"branch": mir["branch"], "slot": 1, "worker": "w"}})
        clear_lock(repo, 1)
        clear_marker(mir["dir"])
        g(mir["dir"], "checkout", "-q", "--detach", g(repo, "rev-parse", "HEAD"))
        check("a registry row with no sentinel still reads `claimed`", slot_state(repo, 1)["state"], "claimed")
        check("... and is NOT free", slot_state(repo, 1)["free"], False)
        check("... so reclaim refuses", reclaim_slot(repo, 1)["reclaimed"], False)
        claims.save_registry(repo, {})
        release(repo, slot=1, branch=mir["branch"], rescue=False)

        # (e-reg) fail closed: an UNREADABLE claim registry refuses rather than guesses
        os.makedirs(os.path.join(repo, ".pi"), exist_ok=True)
        with open(os.path.join(repo, ".pi", "claims.json"), "w", encoding="utf-8") as fh:
            fh.write("{ this is not json")
        bad = reclaim_slot(repo, 1)
        check("an unreadable claim registry refuses the reclaim", bad["reclaimed"], False)
        check("... naming the registry", "cannot be read" in bad["reason"], True)
        try:
            acquire(repo, "auto/noreg", slot=1)
            check("... and no slot is handed out", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and no slot is handed out", "cannot be read" in str(exc), True)
        claims.save_registry(repo, {})
        check("... restored, the pool is usable again", slot_state(repo, 1)["free"], True)

        # (pool) a bulk `init --force` REFUSES while any slot holds live work - the pool is the campaign's
        # concurrency cap, not a scratch file
        keep = acquire(repo, "auto/keep", slot=1)
        try:
            init(repo, count=2, force=True)
            check("init --force refuses while a slot holds live work", "no error", "SystemExit")
        except SystemExit as exc:
            check("init --force refuses while a slot holds live work", "REFUSED init --force" in str(exc), True)
            check("... naming the slot", "auto/keep" in str(exc), True)
        check("... and the live slot is untouched", slot_attached_branch(keep["dir"]), keep["branch"])
        release(repo, slot=1, unit="auto/keep", rescue=False)
        check("... while a drained pool may be forced", init(repo, count=2, force=True)["created"], [1, 2])

        # (e2) fail-closed for real: a CONFLICTING branch is an ambiguity, so nothing is reclaimed
        conf = acquire(repo, "auto/conflict", slot=1)
        d_conf = conf["dir"]
        with open(os.path.join(d_conf, "f.txt"), "w", encoding="utf-8") as fh:
            fh.write("branch side\n")
        g(d_conf, "add", "-A")
        g(d_conf, "commit", "-q", "-m", "branch side")
        with open(os.path.join(repo, "f.txt"), "w", encoding="utf-8") as fh:
            fh.write("main side\n")
        g(repo, "commit", "-q", "-m", "main side", "--", "f.txt")
        clear_lock(repo, 1)
        check("a conflicting branch cannot be tested - ambiguity, not 'applied'",
              branch_fully_applied(repo, conf["branch"]), None)
        conf_v = reclaim_slot(repo, 1)
        check("... so reclaim fails closed and does not act", conf_v["reclaimed"], False)
        check("... naming the doubt", "failing closed" in conf_v["reason"], True)
        check("... and the conflicting branch is left attached", slot_attached_branch(d_conf),
              conf["branch"])
        try:
            acquire(repo, "auto/conflict-2", slot=1)
            check("... and acquire keeps its refusal for it", "no error", "SystemExit")
        except SystemExit as exc:
            check("... and acquire keeps its refusal for it", "never a branch" in str(exc), True)
        release(repo, slot=1, unit="auto/conflict", rescue=False)

        # (e3) the live-lane guard is not bypassed: a landed branch with a RUNNING lane in the slot refuses
        live_land = acquire(repo, "auto/live-land", slot=1)
        d_ll = live_land["dir"]
        commit(d_ll, "landed under a live lane")
        land_into_main(live_land["branch"], msg="land: under a live lane")
        clear_lock(repo, 1)                      # the claim is gone; the only life left is the RUNNING run
        check("the live-lane branch is fully applied", branch_fully_applied(repo, live_land["branch"]), True)
        reg_root2 = os.path.join(tmp, "claude-reclaim", SESSIONS_DIRNAME)
        os.makedirs(reg_root2, exist_ok=True)
        run_id2 = "99999999-8888-7777-6666-555555555555"
        rec2 = os.path.join(reg_root2, "%s.json" % run_id2)
        with open(rec2, "w", encoding="utf-8") as fh:
            json.dump({"pid": os.getpid(), "sessionId": run_id2, "cwd": d_ll, "kind": "headless"}, fh)
        live_v = reclaim_slot(repo, 1, registry=reg_root2)
        check("reclaim refuses a landed branch while a RUNNING lane is in the slot",
              live_v["reclaimed"], False)
        check("... naming the live run", run_id2 in live_v["reason"], True)
        check("... and leaving the branch attached", slot_attached_branch(d_ll), live_land["branch"])
        with open(rec2, "w", encoding="utf-8") as fh:
            json.dump({"pid": gone.pid, "sessionId": run_id2, "cwd": d_ll, "kind": "headless"}, fh)
        live_done = reclaim_slot(repo, 1, registry=reg_root2)
        check("... and reclaims once the run is finished", live_done["reclaimed"], True)
        check("... leaving the pool free", free_count(repo), 2)

        # (f) the exact line `spawn` prints when it reclaims, and its stdout artifact stays clean
        sl = acquire(repo, "auto/spawn-land", slot=1)
        commit(sl["dir"], "spawn landed")
        land_into_main(sl["branch"], msg="land: spawn landed")
        clear_lock(repo, 1)
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            sp_rec = spawn(repo, "tooling", slot=1, unit="lane/spawn-reclaim", task="x")
        check("spawn reclaims a landed slot on the way in", sp_rec["slot"], 1)
        check("... and prints the one RECLAIMED line",
              "RECLAIMED" in buf.getvalue() and sl["branch"] in buf.getvalue(), True)
        check("... leaving the paste-ready stdout line untouched",
              sp_rec["spawnLine"].startswith("cd %s && claude --agent worker "
                                              % slot_dir(repo, 1).replace("\\", "/")), True)
        release(repo, slot=1, unit="lane/spawn-reclaim", rescue=False)
        check("... and the pool is left as it was found", free_count(repo), 2)

        # (g) collect: evidence out of the slot, the unlanded verdict, and a release that never eats work
        with open(os.path.join(repo, ".git", "info", "exclude"), "a", encoding="utf-8") as fh:
            fh.write("\n.pi/\n")               # the real repo ignores `.pi/`; the fixture must too
        cs = acquire(repo, "lane/collect-a", slot=1)
        d_c = cs["dir"]
        os.makedirs(os.path.join(d_c, ".pi", "outbox"), exist_ok=True)
        os.makedirs(os.path.join(d_c, ".pi", "notes"), exist_ok=True)
        with open(os.path.join(d_c, ".pi", "outbox", "collect-a.json"), "w") as fh:
            fh.write("{}")
        with open(os.path.join(d_c, ".pi", "notes", "collect-a.md"), "w") as fh:
            fh.write("note")
        with open(os.path.join(d_c, ".pi", "notes", "ignored.txt"), "w") as fh:
            fh.write("not evidence")
        col = collect(repo, path=d_c)
        check("collect resolves the slot from a path", col["slot"], 1)
        check("... copies the outbox and notes evidence",
              sorted(col["copied"]), ["notes/collect-a.md", "outbox/collect-a.json"])
        check("... into MAIN's .pi", os.path.exists(os.path.join(repo, ".pi", "outbox", "collect-a.json")), True)
        check("... and only evidence files", os.path.exists(os.path.join(repo, ".pi", "notes", "ignored.txt")), False)
        check("a clean slot with no commits holds nothing unlanded", col["unlanded"], None)
        again = collect(repo, slot=1)
        check("a second collect keeps what MAIN already has (as new)",
              (again["copied"], sorted(again["kept"])), ([], ["notes/collect-a.md", "outbox/collect-a.json"]))
        commit(d_c, "collect: unlanded content")
        held = collect(repo, slot=1, release_after=True)
        check("collect reports the commit main does not have", len(held["commits"]), 1)
        check("... calls the slot unlanded", "holds commits main does not have" in (held["unlanded"] or ""), True)
        check("... and --release REFUSES over it (kept, not released)",
              (held["released"], "kept_because" in held), (False, True))
        check("... leaving the branch attached", slot_attached_branch(d_c), cs["branch"])
        land_into_main(cs["branch"], msg="land: collect-a")
        landed = collect(repo, slot=1, release_after=True)
        check("once landed, --release frees the slot", landed["released"], True)
        check("... and the slot is free again", free_count(repo), 2)
        req_row = {"unit": "Lane/unit", "section": ".data", "start": 0x1000, "end": 0x1010, "size": 16,
                   "evidence": "e", "unblocks": "u"}
        other_row = dict(req_row, unit="Other/unit", start=0x2000, end=0x2010)
        from tools.units import dataqueue as _dq
        _dq.write_requests(os.path.join(repo, _dq.REQUESTS_REL), _dq.render_requests([other_row]))
        cs2 = acquire(repo, "lane/collect-b", slot=1)
        _dq.write_requests(os.path.join(cs2["dir"], _dq.REQUESTS_REL), _dq.render_requests([req_row]))
        col2 = collect(repo, slot=1)
        check("collect merges a lane's data requests into MAIN's register", col2["requests_merged"], 1)
        check("... keeping MAIN's other rows",
              sorted(r["unit"] for r in _dq.load_requests(repo)), ["Lane/unit", "Other/unit"])
        check("... and a second collect adds nothing", collect(repo, slot=1)["requests_merged"], 0)
        release(repo, slot=1, unit="lane/collect-b", rescue=False)
        check("an unknown path is refused", _raises(lambda: collect(repo, path=os.path.join(tmp, "nowhere"))), True)
        # (g2) a plain worktree (a lane launched in its own `git worktree`, not a slot) is collected too
        plain = os.path.join(tmp, "plain-lane")
        git(["worktree", "add", "-q", "-b", "lane/plain", plain], repo)
        os.makedirs(os.path.join(plain, ".pi", "notes"), exist_ok=True)
        os.makedirs(os.path.join(plain, ".pi", "outbox"), exist_ok=True)
        with open(os.path.join(plain, ".pi", "notes", "plain-lane.md"), "w") as fh:
            fh.write("note")
        with open(os.path.join(plain, ".pi", "outbox", "plain-lane.json"), "w") as fh:
            fh.write("{}")
        commit(plain, "plain: unlanded content")
        pc = collect(repo, path=plain)
        check("a plain worktree's evidence is collected into MAIN's .pi (no slot)",
              (pc["slot"], sorted(pc["copied"]), os.path.exists(os.path.join(repo, ".pi", "notes", "plain-lane.md"))),
              (None, ["notes/plain-lane.md", "outbox/plain-lane.json"], True))
        check("... with its branch, its commit main does not have and the unlanded verdict",
              (pc["branch"], len(pc["commits"]), "holds commits main does not have" in (pc["unlanded"] or "")),
              ("lane/plain", 1, True))
        check("... and the report names the worktree", collect_lines(pc)[0].startswith("worktree "), True)
        check("--release is refused for a plain worktree (its teardown is not a slot release)",
              (_raises(lambda: collect(repo, path=plain, release_after=True)), os.path.isdir(plain)), (True, True))
        check("MAIN itself is not a lane to collect", _raises(lambda: collect(repo, path=repo)), True)
        git(["worktree", "remove", "--force", plain], repo)

    pool.NINJA_RUNNER = saved_ninja
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


# --- CLI -----------------------------------------------------------------------------------------

def _main_root() -> str:
    return lane_registry_main_of()


def main() -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--main", default=None, help="MAIN's path (default: the first worktree git lists)")
    sub = ap.add_subparsers(dest="cmd")
    i = sub.add_parser("init", help="create the pool of reusable slots")
    i.add_argument("--count", type=int, default=DEFAULT_COUNT)
    i.add_argument("--force", action="store_true", help="remove and re-create existing slots")
    i.add_argument("--json", action="store_true")
    a = sub.add_parser("acquire", help="take a slot: reset, cut a fresh branch off main, verify, lock")
    a.add_argument("unit")
    a.add_argument("--slot", type=int, default=None)
    a.add_argument("--worker", default=None)
    a.add_argument("--branch", default=None)
    a.add_argument("--force", action="store_true",
                   help="take a slot whose `.used` sentinel names a claim whose owner is gone")
    a.add_argument("--dry-run", action="store_true")
    a.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="return a slot to main's tip (keeps the warm trees)")
    r.add_argument("--slot", type=int, default=None)
    r.add_argument("--unit", default=None)
    r.add_argument("--branch", default=None)
    r.add_argument("--keep-branch", action="store_true", help="do not delete the claim's branch")
    r.add_argument("--force", action="store_true",
                   help="release even though a run is live in the slot / its tree is dirty / its HEAD "
                        "holds commits no branch reaches")
    r.add_argument("--dry-run", action="store_true")
    r.add_argument("--json", action="store_true")
    rc = sub.add_parser("reclaim", help="release a slot whose checked-out branch is ALREADY fully applied "
                                         "to main, or whose `.used` sentinel is debris with no claim (an "
                                         "unlanded branch keeps today's refusal)")
    rc.add_argument("--slot", type=int, default=None)
    rc.add_argument("--unit", default=None)
    rc.add_argument("--branch", default=None)
    rc.add_argument("--json", action="store_true")
    sp = sub.add_parser("spawn", help="take a slot for a lane of a kind and print the paste-ready launch")
    sp.add_argument("--kind", required=True,
                    help="the kind of work -> agent profile: " + ", ".join(KIND_PROFILE))
    sp.add_argument("--slot", type=int, default=None,
                    help="take this slot by number (default: the first genuinely free slot)")
    sp.add_argument("--unit", default=None,
                    help="the claim/branch name (default: lane/<kind>-<timestamp>)")
    sp.add_argument("--task-file", default=None, help="read the lane's task text from this file")
    sp.add_argument("--units", default=None,
                    help="the units the lane holds, comma-separated (default: the task's `Units:` line); "
                         "recorded on the slot lock, read back by `claims.py list --json` as `units`")
    sp.add_argument("--worker", default=None)
    sp.add_argument("--force", action="store_true",
                    help="take a slot whose `.used` sentinel names a claim whose owner is gone")
    sp.add_argument("--json", action="store_true")
    co = sub.add_parser("collect", help="copy a finished lane's evidence out of its slot into MAIN/.pi and "
                                         "report what the slot holds (--release frees it when nothing is unlanded)")
    co.add_argument("--slot", type=int, default=None)
    co.add_argument("--path", default=None, help="a slot's path, as a subagent's result reports it")
    co.add_argument("--release", action="store_true", help="release the slot after collecting, if nothing is unlanded")
    co.add_argument("--force", action="store_true", help="with --release: release even over unlanded work")
    co.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="every slot, its lock and whether its build tree is current")
    s.add_argument("--json", action="store_true")
    rf = sub.add_parser("refresh", help="bring one slot's build tree current from MAIN, copying only what differs")
    rf.add_argument("slot", type=int)
    rf.add_argument("--force", action="store_true",
                    help="re-seed even when the slot verifies, or when a running session works in it")
    rf.add_argument("--json", action="store_true")
    v = sub.add_parser("verify", help="validate each slot's build tree against MAIN's current map/DOL")
    v.add_argument("--slot", type=int, default=None)
    v.add_argument("--json", action="store_true")
    sh = sub.add_parser("shadow", help="materialise a lane-shaped tree INSIDE a slot (for fixture tests)")
    sh.add_argument("slot", type=int)
    sh.add_argument("dir", help="where inside the slot: relative to the slot, or an absolute path inside it")
    sh.add_argument("--seed-build", action="store_true",
                    help="also copy the warm build/input trees, so the shadow can compile and score")
    sh.add_argument("--force", action="store_true", help="replace a non-empty destination directory")
    sh.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0
    main_wt = args.main or _main_root()

    if args.cmd == "init":
        out = init(main_wt, args.count, args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("pool of %d slot(s) under %s" % (out["count"], os.path.dirname(out["slots"][0])))
        print("  created: %s" % (out["created"] or "none"))
        print("  present: %s" % (out["present"] or "none"))
        for n, note in out["notes"]:
            print("  slot %d: %s" % (n, note))
        return 0
    if args.cmd == "collect":
        out = collect(main_wt, args.slot, args.path, args.release, args.force)
        if args.json:
            print(json.dumps(out, indent=2))
        else:
            print("\n".join(collect_lines(out)))
        return 0
    if args.cmd == "acquire":
        if args.dry_run:
            out = preview(main_wt, args.unit, args.branch, args.slot, args.worker, args.force)
            print("would acquire slot %d for %s (owner %s): %s"
                  % (out["slot"], args.unit, out["owner"], out["command"]))
            if out.get("reclaim"):
                print("  ... after RECLAIMING it - its branch %r is already fully applied to main"
                      % out["reclaim"]["branch"])
            return 0
        info = acquire(main_wt, args.unit, args.branch, args.worker, args.slot, args.force)
        if args.json:
            print(json.dumps(info, indent=2))
            return 0
        print("acquired slot %d for %s\n  dir     %s\n  branch  %s\n  base    %s\n  verify  %s"
              % (info["slot"], info["unit"], info["dir"], info["branch"], info["base"],
                 "current" if info["verify"]["ok"] else "; ".join(info["verify"]["reasons"])))
        if info.get("refreshed"):
            print("  refreshed %s" % info["refreshed"])
        for line in currency_lines(info["currency"]):
            print("  " + line.replace("**", ""))
        print("  owner   %s (in `.used`)" % marker_owner(info["dir"]))
        if (info.get("landed_reclaim") or {}).get("reclaimed"):
            print("  reclaimed a landed branch first: %s" % info["landed_reclaim"]["line"])
        return 0
    if args.cmd == "release":
        out = release(main_wt, args.slot, args.unit, args.branch, delete_branch=not args.keep_branch,
                      dry_run=args.dry_run, force=args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        verb = "would release" if args.dry_run else "released"
        print("%s slot %d (%s)%s%s" % (verb, out["slot"], out["dir"],
                                       "" if out["branch_deleted"] or not out["branch"] else
                                       " - branch %s kept" % out["branch"],
                                       "" if not out["refreshed"] else "\n  %s" % out["refreshed"]))
        if out.get("overridden"):
            print("  --force overrode:\n  - %s" % "\n  - ".join(out["overridden"]))
        return 0
    if args.cmd == "reclaim":
        n, lock = _resolve_slot(main_wt, args.slot, args.unit, args.branch)
        out = reclaim_slot(main_wt, n, branch=args.branch or lock.get("branch"),
                           unit=args.unit or lock.get("unit"))
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if not out["reclaimed"]:
            raise SystemExit("REFUSED reclaim slot %d: %s\n  the refusal is load-bearing: a slot holds a "
                             "directory, never a branch, and only a branch whose content is PROVEN in main "
                             "is bookkeeping to reclaim (land it, or `release --slot %d`)."
                             % (n, out["reason"], n))
        print(out["line"])
        return 0
    if args.cmd == "spawn":
        out = spawn(main_wt, args.kind, slot=args.slot, unit=args.unit, task_file=args.task_file,
                    worker=args.worker, force=args.force,
                    units=[u.strip() for u in (args.units or "").split(",") if u.strip()] or None)
        if args.json:
            # exactly the fields a caller needs; `spawnLine` is the whole paste-ready text (line + block)
            print(json.dumps({k: out[k] for k in ("slot", "path", "agent", "kind", "units", "spawnLine")},
                             indent=2))
            return 0
        if out["unresolved_units"]:
            print("spawn: NOT recorded (no single registered unit): %s" % ", ".join(out["unresolved_units"]),
                  file=sys.stderr)
        # stdout is exactly the paste-ready artifact (the claude launch line + the block); the header names the
        # slot that was taken on stderr, so a caller can capture stdout verbatim.
        print("spawn slot %d (%s) for kind %s -> agent %s\n  path: %s\n  holds: %s"
              % (out["slot"], out["branch"], out["kind"], out["agent"], out["path"],
                 ", ".join(out["units"]) or "no unit set (pass --units or a task `Units:` line)"),
              file=sys.stderr)
        print(out["spawnLine"])
        return 0
    if args.cmd == "shadow":
        out = shadow(main_wt, args.slot, args.dir, seed_build=args.seed_build, force=args.force)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("shadowed slot %d -> %s (%d file(s)%s)"
              % (out["slot"], out["dir"], out["files"],
                 ", seeded " + ", ".join(out["seeded"]) if out["seeded"] else ""))
        print("  head %s (its OWN repository, inside the slot; nothing outside it was touched)" % out["head"])
        return 0
    if args.cmd == "status":
        rows = status(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no slot pool - run `python tools/units/slots.py init`")
            return 0
        print("%-5s %-9s %-16s %-20s %-20s %-7s %-9s %s"
              % ("slot", "state", "owner", "unit", "branch", ".used", "run", "build tree"))
        for row in rows:
            claim = row.get("claim") or {}
            lock = row.get("lock") or {}
            # one word for WHY it is not free, so a silent cap becomes a diagnosis (`branch`/`claimed`/`debris`)
            state = row.get("state") or ("free" if row["free"] else "used")
            marker = "stale" if row.get("marker_stale") else ("yes" if row["marked"] else "-")
            run = (row["run"]["run_id"][:8] if row.get("run") else "-")
            print("%-5d %-9s %-16s %-20s %-20s %-7s %-9s %s" % (
                row["slot"], state, (row.get("owner") or "-")[:16],
                (claim.get("unit") or lock.get("unit") or "-")[:20],
                (row["attached"] or "detached")[:20], marker, run,
                "current" if row["build_ok"] else "; ".join(row["build_reasons"])))
        # every slot in the manifest gets a reason line when it is not simply free, so a silent cap is a
        # diagnosis: `no worktree`, `branch`, `claimed`, `debris`, with a bad record named where there is one
        for row in rows:
            if not row["free"] or row.get("state") == "no worktree" or row.get("record_note"):
                print("  slot %d: %s" % (row["slot"], row["why"]))
        return 0
    if args.cmd == "refresh":
        out = refresh_slot(main_wt, args.slot, force=args.force)
        if args.json:
            print(json.dumps(out, indent=2))
        elif out["refreshed"] is None:
            print("slot %d: current - nothing to refresh (--force re-seeds anyway)" % args.slot)
        else:
            print("slot %d: %s in %.1f s" % (args.slot, out["refreshed"], out["seconds"]))
            print("slot %d: %s" % (args.slot, "current" if out["after"]["ok"] else "; ".join(out["after"]["reasons"])))
        return 0 if out["after"]["ok"] else 1
    if args.cmd == "verify":
        rows = all_slots(main_wt)
        nums = [args.slot] if args.slot else [r["slot"] for r in rows]
        bad = 0
        out = []
        for n in nums:
            v = verify(main_wt, n)
            out.append(v)
            if not v["ok"]:
                bad += 1
            if not args.json:
                print("slot %d: %s" % (n, "current" if v["ok"] else "; ".join(v["reasons"])))
        if args.json:
            print(json.dumps(out, indent=2))
        return 1 if bad else 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
