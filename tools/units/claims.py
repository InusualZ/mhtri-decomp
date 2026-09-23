"""Claim a unit for a worker: one git worktree, one branch, and the branch *is* the lock.

docs/plan.md 7.2. With up to four externally spawned worker agents, "who owns this unit" cannot live in
anyone's memory, and a lock file has to be trusted. Git already has an atomic one: creating a branch either
succeeds or fails, and two worktrees cannot share a branch name. So a claim is

    git worktree add -b worker/<slug> <sibling>.ws-<slug> <main's HEAD>

and the unit-to-worker mapping, the batch base sha and the timestamps live in `MAIN/.pi/claims.json` - a
convenience registry on top of git, never the source of truth (git is: `list` reconstructs from it when the
registry is missing).

    python tools/units/claims.py claim <unit> [--worker NAME] [--dry-run] [--json]
    python tools/units/claims.py list [--json]
    python tools/units/claims.py release <unit> [--force] [--dry-run]
    python tools/units/claims.py expire [--minutes N] [--apply]
    python tools/units/claims.py --selftest

`<unit>` is the path from the repository root (`Pl/pl_act`, `auto/80040598_fn_80040598`).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import recompile as rc  # noqa: E402

SLUG_MAX = 40
BRANCH_PREFIX = "worker/"


def git(args: list[str], cwd: str, check: bool = True) -> str:
    out = subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, errors="replace")
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, out.stderr.strip()))
    return out.stdout


def slug(unit: str) -> str:
    """A stable, filesystem- and git-safe name for a unit path, unique by a short hash of the full path."""
    # the unit's basename is what a reader recognises; the hash of the full path keeps it unique,
    # since two units can share a basename (Pl/pl_act vs auto/pl_act)
    stem = os.path.splitext(os.path.basename(unit.strip("/")))[0]
    cleaned = re.sub(r"[^A-Za-z0-9]+", "-", stem).strip("-").lower()
    digest = hashlib.sha1(unit.strip("/").encode("utf-8")).hexdigest()[:4]
    room = SLUG_MAX - len(digest) - 1
    return "%s-%s" % (cleaned[:room].strip("-"), digest)


def branch_for(unit: str) -> str:
    return BRANCH_PREFIX + slug(unit)


def worktree_for(unit: str, main: str) -> str:
    """A sibling of MAIN, following the repository's existing `<repo>.ws-<stream>` convention."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s.ws-%s" % (os.path.basename(main), slug(unit)))


def registry_path(main: str) -> str:
    return os.path.join(main, ".pi", "claims.json")


def load_registry(main: str) -> dict:
    path = registry_path(main)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def save_registry(main: str, data: dict) -> None:
    os.makedirs(os.path.dirname(registry_path(main)), exist_ok=True)
    with open(registry_path(main), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)


def branch_exists(main: str, branch: str) -> bool:
    out = subprocess.run(["git", "show-ref", "--verify", "--quiet", "refs/heads/%s" % branch],
                         cwd=main, capture_output=True)
    return out.returncode == 0


def merged_into_main(main: str, branch: str) -> bool:
    """True when every commit of `branch` is already reachable from main - merged *or* cherry-picked."""
    out = subprocess.run(["git", "cherry", "main", branch], cwd=main, capture_output=True, text=True)
    if out.returncode != 0:
        return False
    return not [l for l in out.stdout.splitlines() if l.startswith("+")]


def outbox_path(main: str, unit: str) -> str:
    return os.path.join(main, ".pi", "outbox", slug(unit) + ".json")


def claims_view(main: str) -> list[dict]:
    """Every worker claim git knows about, enriched with the registry and what the worker left behind."""
    registry = load_registry(main)
    rows: list[dict] = []
    for entry in rc.main_worktree_list(main):
        branch = entry.get("branch") or ""
        if not branch.startswith(BRANCH_PREFIX):
            continue
        unit = next((u for u, v in registry.items() if v.get("branch") == branch), None)
        record = registry.get(unit or "", {})
        rows.append({
            "unit": unit or "(unregistered)",
            "branch": branch,
            "worktree": entry["path"],
            "worker": record.get("worker"),
            "claimed_at": record.get("claimed_at"),
            "base": record.get("base"),
            "merged": merged_into_main(main, branch),
            "outbox": bool(unit and os.path.exists(outbox_path(main, unit))),
            "exists": os.path.isdir(entry["path"]),
        })
    extra = [{"unit": u, **v, "worktree": worktree_for(u, main), "merged": merged_into_main(main, v["branch"]),
              "outbox": os.path.exists(outbox_path(main, u)), "exists": os.path.isdir(worktree_for(u, main)),
              "branch": v["branch"]}
             for u, v in registry.items() if not branch_exists(main, v.get("branch", ""))]
    return sorted(rows + extra, key=lambda r: r["unit"])


def claim(unit: str, main: str, worker: str | None, dry_run: bool) -> dict:
    unit = unit.strip("/")
    branch = branch_for(unit)
    path = worktree_for(unit, main)
    if branch_exists(main, branch):
        raise SystemExit("REFUSED: branch %s already exists - the unit is claimed (or was never released).\n"
                         "  see: python tools/units/claims.py list" % branch)
    if os.path.exists(path):
        raise SystemExit("REFUSED: %s already exists" % path)
    base = git(["rev-parse", "HEAD"], main).strip()
    cmd = ["worktree", "add", "-b", branch, path, base]
    if dry_run:
        return {"unit": unit, "branch": branch, "worktree": path, "base": base,
                "command": "git " + " ".join(cmd), "dry_run": True}
    git(cmd, main)
    # the worker writes its outbox and notes here (docs/plan.md 5.3); create them with the claim so the
    # paths in the brief exist before the worker tries to write to them
    for sub in ("outbox", "notes"):
        os.makedirs(os.path.join(main, ".pi", sub), exist_ok=True)
    registry = load_registry(main)
    registry[unit] = {"branch": branch, "worktree": path, "worker": worker or os.environ.get("USERNAME")
                      or os.environ.get("USER") or "unknown", "base": base,
                      "claimed_at": time.strftime("%Y-%m-%dT%H:%M:%S")}
    save_registry(main, registry)
    return {"unit": unit, "branch": branch, "worktree": path, "base": base}


def release(unit: str, main: str, force: bool, dry_run: bool) -> dict:
    unit = unit.strip("/")
    registry = load_registry(main)
    record = registry.get(unit, {})
    branch = record.get("branch") or branch_for(unit)
    path = record.get("worktree") or worktree_for(unit, main)
    outbox = os.path.exists(outbox_path(main, unit))
    merged = merged_into_main(main, branch) if branch_exists(main, branch) else True
    if not force and not (merged or outbox):
        raise SystemExit(
            "REFUSED: %s's branch %s is neither merged into main nor has an outbox entry at\n  %s\n"
            "  releasing it would drop work with no record. Finish the handoff, or pass --force."
            % (unit, branch, outbox_path(main, unit)))
    steps = []
    if os.path.isdir(path):
        steps.append(["worktree", "remove", "--force", path])
    if branch_exists(main, branch):
        steps.append(["branch", "-D", branch])
    steps.append(["worktree", "prune"])
    if dry_run:
        return {"unit": unit, "steps": ["git " + " ".join(s) for s in steps], "dry_run": True,
                "merged": merged, "outbox": outbox}
    for step in steps:
        git(step, main)
    registry.pop(unit, None)
    save_registry(main, registry)
    return {"unit": unit, "branch": branch, "worktree": path, "merged": merged, "outbox": outbox}


def expire(main: str, minutes: int, apply: bool) -> list[dict]:
    now = time.time()
    stale = []
    for row in claims_view(main):
        if not row.get("claimed_at") or row["outbox"]:
            continue
        try:
            age = (now - time.mktime(time.strptime(row["claimed_at"], "%Y-%m-%dT%H:%M:%S"))) / 60.0
        except ValueError:
            continue
        if age >= minutes:
            row["age_minutes"] = round(age, 1)
            stale.append(row)
    if apply:
        for row in stale:
            release(row["unit"], main, force=True, dry_run=False)
    return stale


def selftest() -> int:
    fails = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("slug: simple", slug("Pl/pl_act").startswith("pl-act-"), True)
    check("slug: separators collapse", slug("Pl/pl_act")[:7], "pl-act-")
    check("slug: basename only", slug("Pl/pl_act").startswith("pl-act"), True)
    check("slug: stable", slug("Pl/pl_act"), slug("Pl/pl_act"))
    check("slug: unique per unit", slug("Pl/pl_act") == slug("Pl/pl_skill"), False)
    check("slug: sanitised", bool(re.fullmatch(r"[a-z0-9-]+", slug("auto/80040598_fn_80040598"))), True)
    check("slug: length cap", len(slug("auto/" + "x" * 200)) <= SLUG_MAX, True)
    check("branch prefix", branch_for("main.cpp").startswith("worker/"), True)
    check("worktree is a sibling of main", os.path.basename(worktree_for("Pl/pl_act", "/tmp/mhtri-dtk"))
          .startswith("mhtri-dtk.ws-"), True)
    check("outbox path is in MAIN",
          os.path.dirname(os.path.dirname(outbox_path("/tmp/mhtri-dtk", "Pl/pl_act"))),
          os.path.join("/tmp/mhtri-dtk", ".pi"))

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(main)
        check("registry starts empty", load_registry(main), {})
        save_registry(main, {"Pl/pl_act": {"branch": "worker/pl-act-1234", "claimed_at": "2026-01-01T00:00:00"}})
        check("registry round-trips", load_registry(main)["Pl/pl_act"]["branch"], "worker/pl-act-1234")

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    c = sub.add_parser("claim", help="reserve a unit (worktree + branch)")
    c.add_argument("unit")
    c.add_argument("--worker", default=None)
    c.add_argument("--dry-run", action="store_true")
    c.add_argument("--json", action="store_true")
    l = sub.add_parser("list", help="every claim git knows about")
    l.add_argument("--json", action="store_true")
    r = sub.add_parser("release", help="remove the worktree and the branch")
    r.add_argument("unit")
    r.add_argument("--force", action="store_true")
    r.add_argument("--dry-run", action="store_true")
    e = sub.add_parser("expire", help="claims older than N minutes with no outbox")
    e.add_argument("--minutes", type=int, default=120)
    e.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0

    main_wt = rc.main_root(rc.worktree_root())
    if args.cmd == "claim":
        out = claim(args.unit, main_wt, args.worker, args.dry_run)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        if out.get("dry_run"):
            print("would run: %s" % out["command"])
        else:
            print("claimed %s\n  branch   %s\n  worktree %s\n  base     %s"
                  % (out["unit"], out["branch"], out["worktree"], out["base"]))
            print("\nnext: python tools/units/brief.py %s          # write the brief\n"
                  "      read tools/units/briefs/%s.md in %s, do it, write your report"
                  % (out["unit"], slug(out["unit"]), out["worktree"]))
        return 0
    if args.cmd == "list":
        rows = claims_view(main_wt)
        if args.json:
            print(json.dumps(rows, indent=2))
            return 0
        if not rows:
            print("no active claims")
            return 0
        print("%-34s %-30s %-10s %-7s %-7s %s" % ("unit", "branch", "worker", "merged", "outbox", "claimed"))
        for row in rows:
            print("%-34s %-30s %-10s %-7s %-7s %s"
                  % (row["unit"][:34], (row["branch"] or "")[:30], (row.get("worker") or "")[:10],
                     row["merged"], row["outbox"], row.get("claimed_at") or ""))
        return 0
    if args.cmd == "release":
        out = release(args.unit, main_wt, args.force, args.dry_run)
        if out.get("dry_run"):
            for step in out["steps"]:
                print("would run: %s" % step)
        else:
            print("released %s (branch %s, merged=%s, outbox=%s)"
                  % (out["unit"], out["branch"], out["merged"], out["outbox"]))
        return 0
    if args.cmd == "expire":
        rows = expire(main_wt, args.minutes, args.apply)
        for row in rows:
            print("%-34s %-30s %.1f min  outbox=%s" % (row["unit"], row["branch"], row["age_minutes"], row["outbox"]))
        print("%d stale claim(s)%s" % (len(rows), " released" if args.apply else " (dry run: pass --apply)"))
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
