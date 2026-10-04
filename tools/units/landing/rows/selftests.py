"""The suite row: every tool's selftest passes except the parked list (`tools/selftest.py --json`).
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import json
import os
import subprocess
import sys

from tools.units.landing.common import Batch, command_detail, run


def selftest_detail(p: subprocess.CompletedProcess) -> str:
    """A one-line reason from `tools/selftest.py --json`, so the gate NAMES the failing tool.

    `command_detail` reports the *head* of a command's output, which for the selftest table is its header and
    first data row - the failures are printed below the table. Parse the runner's own JSON instead and name
    them, so the gate's refusal says which tool's selftest failed rather than showing a table header.
    """
    if p.returncode == 0:
        return ""
    try:
        data = json.loads(p.stdout or "")
    except ValueError:
        return command_detail(p)
    parts = []
    bad = [f["name"] for f in (data.get("failures") or [])]
    if bad:
        more = " (+%d more)" % (len(bad) - 6) if len(bad) > 6 else ""
        parts.append("failed: " + ", ".join(bad[:6]) + more)
    stale = data.get("stale_parks") or []
    if stale:
        parts.append("stale park: " + ", ".join(stale[:3]))
    if data.get("tree_clean") is False:
        parts.append("a selftest changed the tree: " + ", ".join((data.get("tree_offenders") or [])[:3]))
    return "exit %d: %s" % (p.returncode, "; ".join(parts) or command_detail(p))


def selftest_tails(p: subprocess.CompletedProcess) -> str:
    """What the selftest row prints beside its one-line refusal: every failing tool's last output lines and every
    flake (a selftest that failed, then passed alone) - `tools/selftest.py --json`'s `failures[].tail` and `flaky`.

    The one-line detail only NAMES the tool (`failed: tools/units/claims`); the lines that say why are here, so a
    refusal never needs the whole suite re-run by hand to learn what failed. "" when the output is not the JSON.
    """
    try:
        data = json.loads(p.stdout or "")
    except ValueError:
        return ""
    out = []
    for f in data.get("failures") or []:
        out.append("selftest FAILED: %s (%s, exit %s) - last lines:" % (f.get("name"), f.get("status"),
                                                                       f.get("returncode")))
        out.extend("    " + ln for ln in (f.get("tail") or f.get("head") or "").splitlines())
    for f in data.get("flaky") or []:
        out.append("WARNING selftest FLAKY: %s %s, then passed on the isolated re-run (logged in "
                   ".pi/selftest-flakes.jsonl) - first failure's last lines:" % (f.get("name"),
                                                                              f.get("first_status")))
        out.extend("    " + ln for ln in (f.get("first_failure") or "").splitlines()[-12:])
    return "\n".join(out)


SUITE_ROW = "all tool selftests pass (except the parked list)"


def selftests_row(b: Batch) -> None:
    """6. every tool's selftest passes except the parked list (`tools/selftest.py --json`); `--no-selftests` skips."""
    selftests = os.path.join(b.main, "tools", "selftest.py")
    if b.no_selftests:
        b.check(SUITE_ROW, True, info="--no-selftests (fast path) - the suite did not run")
    elif os.path.exists(selftests):
        p = run([sys.executable, selftests, "--json"], b.main)
        tails = selftest_tails(p)
        if tails:
            print(tails, file=sys.stderr)
        b.check(SUITE_ROW, p.returncode == 0, selftest_detail(p),
                remedy="fix the named tool's selftest, or park a *pre-existing* failure in "
                       "tools/selftests-known-failures.json with a reason and a date (explicit and greppable, "
                       "never a silent skip); `python tools/selftest.py --changed` is a lane's fast loop")
    else:
        b.check(SUITE_ROW, True, info="tools/selftest.py not built yet")
