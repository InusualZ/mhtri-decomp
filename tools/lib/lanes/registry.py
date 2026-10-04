"""The claim registry (`MAIN/.pi/claims.json`), the heartbeat files, the handoff paths, and the git branch that
is a claim's real lock. Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import json
import os
import time

from tools.lib import repo as librepo
from tools.lib.git import Git
from tools.lib.lanes import naming

STAMP = "%Y-%m-%dT%H:%M:%S"


def now() -> str:
    return time.strftime(STAMP)


def age_seconds(stamp: str | None) -> float | None:
    """Seconds since a `STAMP`-formatted local time, or None when absent/unparseable."""
    if not stamp:
        return None
    try:
        return max(0.0, time.time() - time.mktime(time.strptime(stamp, STAMP)))
    except ValueError:
        return None


# --- the registry ---------------------------------------------------------------------------------------------

def registry_path(main: str) -> str:
    return os.path.join(main, ".pi", "claims.json")


def load(main: str) -> dict:
    """The registry; `{}` when absent or unparseable (git, not this file, is the source of truth)."""
    path = registry_path(main)
    if not os.path.exists(path):
        return {}
    try:
        with open(path, encoding="utf-8") as fh:
            return json.loads(fh.read())
    except (OSError, json.JSONDecodeError):
        return {}


def save(main: str, data: dict) -> None:
    os.makedirs(os.path.dirname(registry_path(main)), exist_ok=True)
    with open(registry_path(main), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, sort_keys=True)


def read(main: str) -> tuple[dict, bool]:
    """`(registry, readable)`: an absent file is `({}, True)`; a file that exists but does not parse as a JSON
    object is `({}, False)` - a caller that hands out or reclaims a slot must refuse on that."""
    path = registry_path(main)
    if not os.path.exists(path):
        return {}, True
    try:
        with open(path, encoding="utf-8") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}, False
    return (data, True) if isinstance(data, dict) else ({}, False)


def key_of(registry: dict, unit: str) -> str | None:
    """The key `unit` is stored under: the extensionless spelling when present, else any equivalent key."""
    unit = naming.norm_unit(unit.strip("/"))
    if unit in registry:
        return unit
    for key in registry:
        if naming.norm_unit(key) == unit:
            return key
    return None


def record(main: str, unit: str) -> dict:
    """The claim record for `unit` under either spelling (raw, normalised, then any equivalent key)."""
    registry = load(main)
    unit = unit.strip("/")
    if unit in registry:
        return registry[unit]
    key = key_of(registry, unit)
    return registry[key] if key is not None else {}


def record_holding(main: str, unit: str) -> dict:
    """The claim that holds `unit`: its own record, else a cluster claim's record whose `units` lists it."""
    own = record(main, unit)
    if own:
        return own
    want = naming.norm_unit(unit.strip("/"))
    for rec in load(main).values():
        if isinstance(rec, dict) and want in {naming.norm_unit(u) for u in rec.get("units") or [] if isinstance(u, str)}:
            return rec
    return {}


def record_for_branch(registry: dict, branch: str) -> tuple[str | None, dict]:
    """`(key, record)` of the registry row holding `branch`; `(None, {})` when none does."""
    key = next((k for k, v in registry.items() if isinstance(v, dict) and v.get("branch") == branch), None)
    return key, (registry.get(key, {}) if key else {})


def claim_branch(main: str, unit: str) -> str:
    """The branch holding `unit`'s claim: the recorded branch, else `branch_for(unit)`."""
    return record(main, unit).get("branch") or naming.branch_for(naming.norm_unit(unit.strip("/")))


def claim_slug(main: str, unit: str) -> str | None:
    """The handoff slug of the unit's active claim (its branch minus `worker/`), None when unclaimed."""
    return naming.slug_of_branch(record(main, unit).get("branch"))


def handoff_slug(main: str, unit: str) -> str:
    """The slug the outbox and notes are named by: the claim's branch slug, else `slug(unit)`."""
    return claim_slug(main, unit) or naming.slug(unit)


def outbox_path(main: str, unit: str) -> str:
    return os.path.join(main, ".pi", "outbox", handoff_slug(main, unit) + ".json")


def notes_path(main: str, unit: str) -> str:
    return os.path.join(main, ".pi", "notes", handoff_slug(main, unit) + ".md")


def ack_path(main: str, unit: str) -> str:
    """The heartbeat, keyed by the unit (a branch suffix must not move it); under `lib.repo.state("ack")`."""
    return os.path.join(str(librepo.state("ack", main)), naming.ack_name(unit))


def load_ack(main: str, unit: str) -> dict:
    path = ack_path(main, unit)
    if not os.path.exists(path):
        return {}
    try:
        with open(path, encoding="utf-8") as fh:
            return json.loads(fh.read())
    except (OSError, json.JSONDecodeError):
        return {}


def claimed_units(registry: dict) -> set[str]:
    """Every unit a registry row holds, normalised: its key, plus every unit a cluster claim lists in `units`."""
    out: set[str] = set()
    for key, rec in registry.items():
        out.add(naming.norm_unit(key))
        if isinstance(rec, dict):
            out.update(naming.norm_unit(u) for u in rec.get("units") or [] if isinstance(u, str))
    return out


# --- the git lock -----------------------------------------------------------------------------------------------

def branch_exists(main: str, branch: str) -> bool:
    return Git(main).run("show-ref", "--verify", "--quiet", "refs/heads/%s" % branch).returncode == 0


def ref_exists(main: str, ref: str) -> bool:
    return Git(main).run("show-ref", "--verify", "--quiet", ref).returncode == 0


def rescue_exists(main: str, unit: str) -> str | None:
    """The rescue ref holding `unit`'s work (`naming.rescue_ref`), or None."""
    ref = naming.rescue_ref(unit)
    return ref if ref_exists(main, ref) else None


def worker_branches(main: str) -> set[str]:
    """Every live `worker/` branch in one git call (none outside a repository)."""
    p = Git(main).run("for-each-ref", "--format=%(refname:short)", "refs/heads/" + naming.BRANCH_PREFIX)
    if p.returncode != 0:
        return set()
    return {line.strip() for line in p.stdout.splitlines() if line.strip()}


def lock_held(main: str, unit: str, branches: set[str] | None = None) -> bool:
    """Whether `unit`'s claim lock is taken: its branch exists or its throwaway worktree is still there."""
    unit = naming.norm_unit(unit.strip("/"))
    if branches is None:
        branches = worker_branches(main)
    return naming.branch_for(unit) in branches or os.path.exists(naming.worktree_for(unit, main))


def merged_into_main(main: str, branch: str) -> bool:
    """Every commit of `branch` is reachable from main - merged or cherry-picked (`git cherry`)."""
    p = Git(main).run("cherry", "main", branch)
    if p.returncode != 0:
        return False
    return not [line for line in p.stdout.splitlines() if line.startswith("+")]


def commits_ahead(main: str, branch: str, base: str = "main") -> int:
    """How many commits `branch` has that `base` lacks; 0 when either is missing."""
    if not branch or not branch_exists(main, branch):
        return 0
    p = Git(main).run("rev-list", "--count", "%s..%s" % (base, branch))
    out = p.stdout.strip()
    return int(out) if p.returncode == 0 and out.isdigit() else 0


def worktree_paths(main: str) -> list[str] | None:
    """Every path `git worktree list` knows, or None when git cannot be asked."""
    p = Git(main).run("worktree", "list", "--porcelain")
    if p.returncode != 0:
        return None
    return [line[len("worktree "):].strip() for line in p.stdout.splitlines() if line.startswith("worktree ")]


def toplevel_of(cwd: str | None = None) -> str:
    """The tree the caller stands in (`git rev-parse --show-toplevel` of `cwd`); SystemExit outside a repository."""
    here = cwd or os.getcwd()
    p = Git(here).run("rev-parse", "--show-toplevel")
    if p.returncode != 0:
        raise SystemExit("git rev-parse --show-toplevel failed in %s: %s" % (here, p.stderr.strip()))
    return p.stdout.strip()


def main_of(cwd: str | None = None) -> str:
    """MAIN for the tree the caller stands in: its toplevel's main checkout (`lib.repo.main_checkout`)."""
    return librepo.main_checkout(toplevel_of(cwd))


def same_path(a: str, b: str) -> bool:
    return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))
