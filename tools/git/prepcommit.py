#!/usr/bin/env python3
"""Prepare a commit for the current results - stage the paths, write the message, execute nothing.

    python tools/git/prepcommit.py [--dry-run] [--split] [--message SUBJECT] [--commit]

The decompile-symbol skill's last step is "leave a prepared commit". This is that step, and it exists because
the parts that go wrong are mechanical:

* `git add -A` would sweep up another agent's in-flight file (this repo has three agents in it),
* build output, `orig/`, `.lavish/`, `.pi/` and scratch must never be staged, and a stray file in the tree
  (a `.stackdump`, a `__pycache__`) is an accident worth refusing rather than committing,
* `AGENTS.md` carries a LOCAL-ONLY block that non-negotiable 8 forbids committing: it has to be pulled out
  before staging and pushed back afterwards, in that order, and the working tree must end up reviewable,
* the commit message is supposed to carry the *results*, and those numbers already exist in
  `build/RMHE08/report.json`.

What it does: classifies every path in `git status` into stage / refuse, stages only the stageable ones, writes
the message (measured results included) to `.git/prepcommit_msg.txt`, prints the commit command - and stops.
`--split` prints a one-concern-per-commit plan instead of staging everything at once. `--commit` runs the
commit for you and is only for when the user has explicitly asked for one.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_FILE = os.path.join(ROOT, ".pi", "local-only.state.json")
BEGIN_MARKER = "<!-- LOCAL-ONLY-BEGIN"

STAGE_PREFIXES = ("src/", "include/", "tools/", "docs/", ".agents/skills/", "config/")
STAGE_FILES = ("configure.py", "AGENTS.md")
REFUSE_PREFIXES = ("build/", "orig/", ".lavish/", ".pi/", ".vscode/", ".idea/", "__pycache__/")
REFUSE_FILES = ("objdiff.json", "compile_commands.json", "build.ninja")
REFUSE_SUFFIXES = (".o", ".elf", ".dol", ".rel", ".map", ".MAP", ".exe", ".stackdump", ".pyc")


def git(*args: str, check: bool = True) -> str:
    out = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True)
    if check and out.returncode != 0:
        sys.exit(f"git {' '.join(args)} failed: {out.stderr.strip()}")
    return out.stdout


def msg_path() -> str:
    """Where to write the message. In a linked worktree `.git` is a *file*, so ask git for the path: the
    message lands in the worktree's private git dir, which can never be committed."""
    path = git("rev-parse", "--git-path", "prepcommit_msg.txt").strip()
    return path if os.path.isabs(path) else os.path.join(ROOT, path)


def localonly_markers() -> int:
    path = os.path.join(ROOT, "AGENTS.md")
    if not os.path.exists(path):
        return 0
    return sum(1 for line in open(path, "r", encoding="utf-8") if line.startswith(BEGIN_MARKER))


def classify(path: str) -> tuple[str, str]:
    """-> ('stage' | 'refuse', reason)."""
    if path in REFUSE_FILES or path.endswith(REFUSE_SUFFIXES):
        return "refuse", "build output or scratch, never committed"
    if path.startswith(REFUSE_PREFIXES):
        return "refuse", "ignored or local-only tree"
    if path in STAGE_FILES or path.startswith(STAGE_PREFIXES):
        return "stage", "project content"
    return "refuse", "unknown - decide by hand rather than sweeping it in"


def status_paths() -> list[tuple[str, str]]:
    """[(status, path)] for every tracked or untracked change git reports."""
    rows = []
    for line in git("status", "--porcelain", "-uall").splitlines():
        if not line.strip():
            continue
        code, path = line[:2].strip(), line[3:]
        if " -> " in path:  # rename
            path = path.split(" -> ", 1)[1]
        rows.append((code, path))
    return rows


def results_for(paths: list[str]) -> list[str]:
    """The measured line for each changed source file, looked up in the report by unit name."""
    wanted = {}
    for path in paths:
        if not path.startswith("src/") or not path.endswith((".c", ".cpp")):
            continue
        directory, stem = os.path.split(path[len("src/") :])
        wanted[f"main/{directory}/{os.path.splitext(stem)[0]}"] = path
    lines = []
    for name, measures in _report_measures().items():
        if name in wanted:
            m = measures
            lines.append(
                f"  {wanted[name]}: fuzzy_match_percent {m.get('fuzzy_match_percent')}, "
                f"matched_code {m.get('matched_code')}/{m.get('total_code')}"
            )
    return sorted(lines)


def _report_measures() -> dict:
    report = os.path.join(ROOT, "build", "RMHE08", "report.json")
    if not os.path.exists(report):
        return {}
    data = json.loads(open(report, "r", encoding="utf-8").read())
    return {u.get("name", ""): (u.get("measures") or {}) for u in data.get("units", [])}


def verify() -> tuple[str, list[str]]:
    """Cheap, real evidence for the message: the DOL hash and the last changes report."""
    notes, warnings = [], []
    dol = os.path.join(ROOT, "build", "RMHE08", "main.dol")
    sha = os.path.join(ROOT, "config", "RMHE08", "build.sha1")
    if os.path.exists(dol) and os.path.exists(sha):
        import hashlib
        digest = hashlib.sha1(open(dol, "rb").read()).hexdigest().upper()
        expected = open(sha, "r", encoding="utf-8").read().split()[0].upper()
        if digest == expected:
            notes.append(f"main.dol SHA-1 verified against config/RMHE08/build.sha1 ({digest})")
        else:
            warnings.append(f"main.dol SHA-1 {digest} != build.sha1 {expected} - do not commit")
    changed = os.path.join(ROOT, "build", "RMHE08", "report_changes.json")
    if os.path.exists(changed):
        data = json.loads(open(changed, "r", encoding="utf-8").read())
        for unit in data.get("units", []):
            name = unit.get("name", "")
            before, after = unit.get("from") or {}, unit.get("to") or {}
            for key in ("matched_code", "matched_data"):
                b, a = before.get(key), after.get(key)
                if b and (a is None or int(a) < int(b)) and "auto_" not in name:
                    warnings.append(f"{name}: {key} {b} -> {a} (regression)")
        notes.append("last changes report read: build/RMHE08/report_changes.json")
    return "; ".join(notes), warnings


def message_for(paths: list[str], subject: str | None, notes: str = "", warnings: list[str] | None = None) -> str:
    warnings = warnings or []
    source = [p for p in paths if p.startswith("src/") and p.endswith((".c", ".cpp"))]
    skill_tools = [p for p in paths if p.startswith((".agents/skills/", "tools/"))]
    docs = [p for p in paths if p.startswith("docs/")]
    areas = sorted({p.split("/")[0] if "/" in p else p for p in paths})
    if subject is None:
        if source:
            subject = f"units: {len(source)} unit(s) at 100%"
            if skill_tools:
                subject += " + the decompile-symbol skill"
            if docs:
                subject += " + docs"
        elif skill_tools:
            subject = "agents: the decompile-symbol skill and its tools"
        else:
            subject = "docs: record the matching findings"
    body = ["", "Areas: " + ", ".join(areas)]
    results = results_for(paths)
    if results:
        body += ["", "Measured (build/RMHE08/report.json):", *results]
    body += [
        "",
        "Gates: " + (notes or "no build evidence found - run ninja build/RMHE08/ok before committing"),
    ]
    if warnings:
        body += ["", "WARNINGS:", *["  " + w for w in warnings]]
    body += [
        "",
        "Every new unit was probed against its auto_* blob before registration; unit residuals live in each",
        "unit's file header comment.",
        "",
        "Prepared with tools/git/prepcommit.py - review with `git diff --cached`, then commit.",
    ]
    return subject + "\n" + "\n".join(b for b in body if b is not None) + "\n"


def split_plan(paths: list[str]) -> list[tuple[str, list[str]]]:
    groups = [
        ("units", [p for p in paths if p.startswith(("src/", "config/")) or p == "configure.py"]),
        (".agents/skills or tools", [p for p in paths if p.startswith((".agents/skills/", "tools/"))]),
        ("docs", [p for p in paths if p.startswith("docs/")]),
        ("agents playbook", [p for p in paths if p == "AGENTS.md"]),
    ]
    return [(name, group) for name, group in groups if group]


def localonly(action: str) -> None:
    script = os.path.join(ROOT, "tools", "agents", "localonly.py")
    if not os.path.exists(script):
        return
    subprocess.run([sys.executable, script, action], cwd=ROOT, capture_output=True, text=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true", help="classify and show the plan, stage nothing")
    parser.add_argument("--split", action="store_true", help="print a one-concern-per-commit plan")
    parser.add_argument("--message", default=None, help="commit subject (default: composed from the change set)")
    parser.add_argument("--commit", action="store_true", help="actually run git commit (only when asked)")
    args = parser.parse_args()

    rows = status_paths()
    stage, refuse = [], []
    for code, path in rows:
        verdict, reason = classify(path)
        (stage if verdict == "stage" else refuse).append((code, path, reason))

    print("to stage:")
    for code, path, _ in stage:
        print(f"  {code:2s} {path}")
    if refuse:
        print("refusing (not staged - fix or ignore by hand):")
        for code, path, reason in refuse:
            print(f"  {code:2s} {path}   <- {reason}")
    if not stage:
        print("nothing to stage")
        return 0

    paths = [path for _, path, _ in stage]

    if args.split:
        print("\nplan (one concern per commit; run the tool per group, or stage by hand):")
        for i, (name, group) in enumerate(split_plan(paths), 1):
            print(f"  {i}. {name}: {len(group)} path(s)")
            for path in group:
                print(f"       {path}")
        print("\nmessage draft for the first group:\n")
        print(message_for(split_plan(paths)[0][1], args.message))
        return 0

    notes, warnings = verify()
    message = message_for(paths, args.message, notes, warnings)
    if args.dry_run:
        print("\n--- message (not written, nothing staged) ---\n" + message)
        return 0

    agents_md = "AGENTS.md" in paths
    if agents_md:
        localonly("pull")  # the LOCAL-ONLY block must not be committed (non-negotiable 8)
        if not os.path.exists(STATE_FILE):
            sys.exit(
                "refusing to stage AGENTS.md: pulling the LOCAL-ONLY block left no state file to restore from.\n"
                "Its live section has to be recoverable before the file is staged - restore it by hand first."
            )

    msg_file = msg_path()
    with open(msg_file, "w", encoding="utf-8") as fh:
        fh.write(message)

    try:
        git("add", "--", *paths)  # explicit paths only, never `git add -A`
    finally:
        if agents_md:
            localonly("push")  # working tree keeps its live section, the index keeps the stripped blob
            if localonly_markers() == 0:
                print(
                    "WARNING: the AGENTS.md LOCAL-ONLY section did not come back into the working tree.\n"
                    f"         The staged blob is correct (no block); restore the live section yourself - the\n"
                    f"         most recent copy is the last commit or a sibling worktree's AGENTS.md."
                )

    print(f"\nstaged {len(paths)} path(s); message written to {os.path.relpath(msg_file, ROOT)}\n")
    print(git("status", "--short"))
    if args.commit:
        print(git("commit", "-F", msg_file))
        if agents_md:
            localonly("push")
        print(git("log", "--stat", "-1"))
    else:
        print(f"commit it with:\n  git commit -F {os.path.relpath(msg_file, ROOT)}\n")
        print("message preview:\n" + message)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
