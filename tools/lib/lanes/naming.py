"""Unit, slug, branch, worktree and rescue-ref names - one spelling rule for every lane tool.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import hashlib
import os
import re

SLUG_MAX = 40
BRANCH_PREFIX = "worker/"
#: The branch prefixes a lane may use (`lane.py teardown` refuses anything else). `main` is never a lane.
LANE_PREFIXES = ("experiment/", "worker/", "wip/", "lane/")
RESCUE_PREFIX = "refs/rescue/"
_SOURCE_EXT = (".cpp", ".cp", ".c")


def norm_unit(unit: str) -> str:
    """The key a claim is held under: the unit path without its source extension (`auto/X.c` -> `auto/X`)."""
    for ext in _SOURCE_EXT:
        if unit.endswith(ext):
            return unit[: -len(ext)]
    return unit


def slug(unit: str) -> str:
    """A stable, filesystem- and git-safe name for a unit: its basename, cleaned, plus 4 hex of the path's sha1.

    The unit is normalised first, so `X` and `X.cpp` share one slug (and therefore one branch, the lock)."""
    unit = norm_unit(unit)
    stem = os.path.splitext(os.path.basename(unit.strip("/")))[0]
    cleaned = re.sub(r"[^A-Za-z0-9]+", "-", stem).strip("-").lower()
    digest = hashlib.sha1(unit.strip("/").encode("utf-8")).hexdigest()[:4]
    room = SLUG_MAX - len(digest) - 1
    return "%s-%s" % (cleaned[:room].strip("-"), digest)


def branch_for(unit: str) -> str:
    """The claim branch `claims.py claim` cuts for `unit`: `worker/<slug>`."""
    return BRANCH_PREFIX + slug(unit)


def worktree_for(unit: str, main: str) -> str:
    """The throwaway (no-slot) worktree path: the sibling `<repo>.ws-<slug>` of MAIN."""
    main = os.path.abspath(main)
    return os.path.join(os.path.dirname(main), "%s.ws-%s" % (os.path.basename(main), slug(unit)))


def slug_of_branch(branch: str | None) -> str | None:
    """The handoff slug a branch names: `worker/<slug>` minus the prefix; `None` for an empty branch."""
    branch = branch or ""
    if branch.startswith(BRANCH_PREFIX):
        return branch[len(BRANCH_PREFIX):]
    return branch or None


def lane_slug(branch: str) -> str:
    """A lane branch's rescue name: everything after its first `/` (`experiment/wP-promote` -> `wP-promote`)."""
    return re.sub(r"^[^/]+/", "", branch)


def is_lane_branch(branch: str) -> bool:
    return any(branch.startswith(p) for p in LANE_PREFIXES)


def rescue_ref(unit: str) -> str:
    """Where a release parks a claim's unmerged commits: `refs/rescue/<slug(unit)>`."""
    return RESCUE_PREFIX + slug(norm_unit(unit.strip("/")))


def rescue_ref_for_branch(branch: str) -> str:
    """Where a lane teardown (no claim) parks a branch's tip: `refs/rescue/<lane_slug(branch)>`."""
    return RESCUE_PREFIX + lane_slug(branch)


def ack_name(unit: str) -> str:
    """The heartbeat file name, keyed by the unit (never by the claim's branch): `<slug(unit)>.json`."""
    return slug(unit) + ".json"
