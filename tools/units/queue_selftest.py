#!/usr/bin/env python3
"""Self-test for tools/units/queue.py - the pool-to-worker handoff.

    python tools/units/queue_selftest.py

The checks run against a temp fixture with an injected claim function, so no git worktree, no `ninja` and
no repository state are touched. What they pin: the pool's state machine (`ready` / `claimed` / `written` /
`stale` / `unreadable`), the address order `next` picks, that `--dry-run` claims nothing, that the real flow
claims the unit *before* promoting the brief, that the promoted brief is a copy of the pooled one at the
claim's own slug, that a suffixed branch is re-rendered so its outbox path is the claim's, and that the
printed spawn carries the cwd, name and task the orchestrator pastes.

`queue.selftest()` holds the checks so `queue.py --selftest` and this entry point cannot drift.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools"))

from units import queue  # noqa: E402


def main() -> int:
    return queue.selftest()


if __name__ == "__main__":
    sys.exit(main())
