"""The subject row: the gate composes the landing's subject and `commitlint.py` checks it.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import sys

from tools.units.landing.common import Batch, command_detail, run


def land_subject(units):
    # The gate s subject follows CLAUDE.md s commit convention: a category that mirrors the tree, then an
    # imperative message of at most 120 characters. Derived from the batch, so the two cannot drift.
    entries = [u.strip() for u in (units or []) if u.strip()]
    if not entries:
        return "repo/batch: land a batch"
    first = entries[0].replace("\\", "/")
    for pre in ("src/", "include/"):
        if first.startswith(pre):
            first = first[len(pre):]
            break
    parts = first.split("/")
    root = parts[0]
    stem = root[:-4] if root.endswith(".cpp") else (root[:-2] if root.endswith(".c") else root)
    lo = stem.lower()
    if lo == "tools":
        cat = "tools/" + (parts[1] if len(parts) > 1 else "tools")
    elif lo == "docs":
        cat = "docs/" + (parts[1][:-3] if len(parts) > 1 and parts[1].endswith(".md") else (parts[1] if len(parts) > 1 else "docs"))
    elif lo == "configure.py":
        cat = "config/flags"
    elif lo == "config":
        base = parts[-1]
        cat = "config/" + {"symbols.txt": "symbols", "splits.txt": "splits", "configure.py": "flags"}.get(base, "config")
    elif lo == ".claude":
        cat = "agents/" + (parts[2][:-3] if len(parts) > 2 and parts[2].endswith(".md") else "policy")
    elif lo in (".github", ".git"):
        cat = "repo/ci"
    elif lo == "claude.md":
        cat = "agents/policy"
    elif lo.startswith("readme") or lo.startswith("license") or lo in (".gitignore", ".gitattributes", "gitignore", "gitattributes"):
        cat = "repo/" + ("readme" if lo.startswith("readme") else ("license" if lo.startswith("license") else lo))
    else:
        cat = "game/" + lo
    tail = "" if len(entries) == 1 else (" and %d more" % (len(entries) - 1))
    msg = "land " + first + tail
    if len(msg) > 120:
        msg = msg[:117] + "..."
    return cat + ": " + msg


def subject_lint(main: str, subject: str, runner=None, tool: str | None = None) -> tuple:
    """Lint the gate's own composed subject with `tools/git/commitlint.py` - the convention's own checker.

    Every landing is written under a subject `land_subject` composes, so a regression there (an invented
    category, a message past the limit) would land a message CLAUDE.md's convention forbids and nothing would
    notice. The row calls the tool rather than re-deriving the rules: CLAUDE.md is the convention,
    commitlint.py is its mechanical checker, and a second copy here would drift from both.

    The tool's exit codes are the contract (`--diff`'s 0/1/2): **0** clean, **1** a violation, **2** nothing
    was checked - and 2 is a *failure* here, because a lint that did not run must never read as approval.

    `runner` and `tool` are the seams the selftest uses (the real tool, pinned to a fixture `--root`); the
    gate passes neither.
    """
    tool = os.path.abspath(tool or os.path.join(main, "tools", "git", "commitlint.py"))
    if not os.path.exists(tool):
        return True, "commitlint.py not built yet - the row is skipped"
    runner = runner or (lambda args: run(args, main))
    p = runner([sys.executable, tool, "--message", subject, "--root", main])
    if p.returncode == 0:
        return True, ""
    detail = command_detail(p)
    if p.returncode == 2:
        return False, "commitlint checked nothing (exit 2), which is not a pass: %s" % detail
    return False, detail


def subject_row(b: Batch) -> None:
    """9. the gate's own composed subject follows the convention (`commitlint.py`, exit 2 a failure)."""
    b.subject = land_subject(b.units)
    ok_subject, subject_detail = subject_lint(b.main, b.subject)
    b.check("the gate's own subject follows the convention", ok_subject, subject_detail, info=b.subject,
            remedy="the composed subject `%s` is a commitlint violation, so the batch would land a message "
                   "CLAUDE.md's convention forbids; fix `land_subject` (or the batch's unit names) so the "
                   "category is a known member and the message is at most 120 characters, then re-run - "
                   "`python tools/git/commitlint.py --message \"<subject>\"` reproduces it" % b.subject)
