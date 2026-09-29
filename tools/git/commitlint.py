#!/usr/bin/env python3
"""Lint commit messages against the convention AGENTS.md defines ("Commit messages follow one convention").

The convention is `<category>: <message>`, then an optional long description. AGENTS.md is the
specification; this tool checks only the part of it that is **mechanically** checkable:

| # | check | how |
| - | ----- | --- |
| 1 | subject shape | the first line is `<category>: <message>` - a category token, a colon, a space, then the message |
| 2 | category membership | the category names a place in the tree (`game/<module>`, `tools/<area>`, `agents/<profile|policy>`, `config/<what>`, `docs/<topic>`, `repo/<area>`) - see `derive_members` |
| 3 | unknown member / family | a member not in a known family is an **error** (a typo or an invented module is exactly what that catches); an unknown **family** is a **warning** (the convention calls the list open, so a new family is added to AGENTS.md deliberately) |
| 4 | message length | at most 120 characters, counted *after* `<category>: `, so the category and its separator are not charged against it |
| 5 | subject line, not a wall | the first line is not empty or all whitespace, and a body is separated from it by a blank line |

**What it deliberately does not check.** The long description's *content* is a review matter, not a
mechanical one. The convention bans *why* (reasoning, alternatives considered, an account of the work) from
the description, but "no why" cannot be decided by a regular expression, and a heuristic that guessed wrong
would flag messages a reviewer would accept - which is how a lint becomes hated and gets switched off. So
the description's content is left to review; only its *shape* is checked here (check 5).

**The membership sets are derived from the tree, never hard-coded** (`derive_members`), so the lint cannot
drift from the tree it describes:

* `game/<module>` - the directories under `src/`.  A member is matched **case-insensitively**: the
  convention's own examples lower-case the prose (`game/network`), while the tree directory is
  `src/Network`, and a lint that rejected the spec's example would be a false positive.
* `tools/<area>` - the directories under `tools/`, plus the top-level scripts as themselves.
* `agents/<name>` - the profiles in `.agents/agents/*.md`, plus `policy` for AGENTS.md.
* `config/{flags,symbols,splits}` - the three inputs the convention names (configure.py -> `flags`,
  symbols.txt -> `symbols`, splits.txt -> `splits`).
* `docs/<topic>` - the documents under `docs/`.
* `repo/<area>` - read from the files at the root: `readme`, `license`, `ci` (a `.github*` directory),
  `gitignore`.

    python tools/git/commitlint.py <message-file>         # the `commit-msg` hook shape git hands over
    python tools/git/commitlint.py --message "<subject>"  # a one-liner
    python tools/git/commitlint.py --last 20              # score the recent history
    python tools/git/commitlint.py --install-hook [--force]
    python tools/git/commitlint.py --selftest

Exit status follows the house convention (`--diff`'s 0/1/2): **0** clean (a warning alone does not fail -
an unknown family is legitimate), **1** one or more violations, **2** nothing was checked (no mode given,
`--last 0`, or a history with no commits), so the tool can be wired into a gate later.

**`--install-hook` is a convenience, not the enforcement path.** It writes an `sh`-compatible `commit-msg`
hook to `.git/hooks/commit-msg` (mode 0755) that calls this tool with `"$1"`. That hook is untracked and
therefore **per-clone**: it never travels with the repository, and it is not what stops a bad message - the
enforcement is a gate that runs this tool directly (the same `0/1/2` it already speaks). Note also that when
`core.hooksPath` is configured - `git config core.hooksPath tools/git/hooks` in this repo - git does **not**
consult `.git/hooks`, so the installer says so rather than pretend the hook is active.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import difflib

MAX_MESSAGE = 120                       # characters, counted after `<category>: `
SUBJECT_RE = re.compile(r"^([^:\s]+): (.+)$")
HISTORY_SEP = "\x1e"                    # git's record separator for `--last`
FIELD_SEP = "\x1f"                      # git's field separator for `--last`

# `config`'s members are the three inputs the convention names; the map from a file to its member is fixed
# by the convention's own text, not by a directory walk (there is no `config/flags/` directory to walk).
CONFIG_MEMBERS = {"flags", "symbols", "splits"}

HOOK_MARKER = "# commitlint (installed by tools/git/commitlint.py --install-hook)"
HOOK_BODY = """#!/bin/sh
%s
# Per-clone and untracked: installing this hook is a convenience, not the enforcement path - a gate that
# runs `python tools/git/commitlint.py` (exit 0/1/2) is. Git hands the message file over as "$1".
root=$(git rev-parse --show-toplevel)
exec python "$root/tools/git/commitlint.py" "$1"
""" % HOOK_MARKER


# --------------------------------------------------------------------------------------------------
# the categories, derived from the tree
# --------------------------------------------------------------------------------------------------
def _subdirs(path: str) -> set:
    """The non-private directory names directly under `path` (a missing path is an empty set)."""
    out = set()
    try:
        names = os.listdir(path)
    except OSError:
        return out
    for name in names:
        if name.startswith(".") or name.startswith("_"):
            continue
        if os.path.isdir(os.path.join(path, name)):
            out.add(name)
    return out


def derive_members(root: str) -> dict:
    """The valid members of every known family, read from the tree at `root`.

    Derived, never hard-coded, so the lint follows the tree: a module added under `src/` or a grouping added
    under `tools/` becomes a valid category the moment the directory exists.
    """
    root = os.path.abspath(root)
    members: dict = {}

    members["game"] = _subdirs(os.path.join(root, "src"))

    # tools/<area>: the directories under tools/ plus the top-level scripts as themselves (`tools/selftest`,
    # `tools/unitutil`, ...).  `__pycache__` and the `__init__` shim are not tools.
    tools: set = set()
    tdir = os.path.join(root, "tools")
    try:
        names = os.listdir(tdir)
    except OSError:
        names = []
    for name in names:
        if name.startswith(".") or name.startswith("_"):
            continue
        if os.path.isdir(os.path.join(tdir, name)):
            tools.add(name)
        elif name.endswith(".py"):
            tools.add(name[:-3])
    members["tools"] = tools

    agents = {"policy"}
    adir = os.path.join(root, ".agents", "agents")
    try:
        names = os.listdir(adir)
    except OSError:
        names = []
    for name in names:
        if name.endswith(".md"):
            agents.add(name[:-3])
    members["agents"] = agents

    members["config"] = set(CONFIG_MEMBERS)

    docs: set = set()
    ddir = os.path.join(root, "docs")
    try:
        names = os.listdir(ddir)
    except OSError:
        names = []
    for name in names:
        if name.endswith(".md"):
            docs.add(name[:-3])
    members["docs"] = docs

    repo: set = set()
    try:
        root_names = [n.lower() for n in os.listdir(root)]
    except OSError:
        root_names = []
    if any(n.startswith("readme") for n in root_names):
        repo.add("readme")
    if any(n.startswith("license") for n in root_names):
        repo.add("license")
    if any(n.startswith(".github") for n in root_names):
        repo.add("ci")
    if any(n == ".gitignore" or n.startswith(".gitignore.") for n in root_names):
        repo.add("gitignore")
    members["repo"] = repo

    return members


def _finding(level: str, message: str) -> dict:
    return {"level": level, "message": message}


def category_findings(category: str, members: dict) -> list:
    """Judge one category token against the derived member sets (errors and warnings, not exceptions)."""
    if "/" in category:
        family, member = category.split("/", 1)
    else:
        family, member = category, ""
    if family not in members:
        return [_finding("warning",
                         "unknown family `%s` - the list is open, but add it to AGENTS.md deliberately"
                         % family)]
    if not member:
        return [_finding("error",
                         "`%s` is a known family but names no member; use `%s/<...>`" % (family, family))]
    if member not in members[family]:
        # Case-insensitive, because the convention's own module examples are lower-cased prose
        # (`game/network`) while the tree directory is `src/Network`; a lint that rejects the spec's own
        # example is a false positive.  The tree's casing is used for the suggestion.
        lookup = {name.lower(): name for name in members[family]}
        if member.lower() not in lookup:
            close = difflib.get_close_matches(member.lower(), sorted(lookup), n=1, cutoff=0.6)
            hint = " (did you mean `%s/%s`?)" % (family, lookup[close[0]]) if close else ""
            return [_finding("error",
                             "`%s` is not a known `%s` member%s" % (category, family, hint))]
    return []


# --------------------------------------------------------------------------------------------------
# the message checks
# --------------------------------------------------------------------------------------------------
def _message_lines(text: str) -> list:
    """The message as lines, with git's `#` comment lines dropped and trailing blanks trimmed.

    `commit-msg` receives the file before git's cleanup runs, so an editor-driven commit carries the `#`
    instructions git itself ignores; dropping them keeps check 5 from reading a comment as the body.
    """
    raw = text.replace("\r\n", "\n").replace("\r", "\n")
    lines = [line for line in raw.split("\n") if not line.startswith("#")]
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def lint_message(text: str, members: dict) -> list:
    """Every mechanical finding for one commit message - a list of `{"level", "message"}` dicts."""
    lines = _message_lines(text)
    subject = lines[0] if lines else ""
    if not subject.strip():
        return [_finding("error", "the first line is empty - a subject is required")]

    match = SUBJECT_RE.match(subject)
    if not match:
        return [_finding("error",
                         "the subject is not `<category>: <message>`: %r" % subject)]

    category, message = match.group(1), match.group(2)
    findings = category_findings(category, members)

    if not message.strip():
        findings.append(_finding("error", "the message after `<%s>: ` is empty" % category))
    if len(message) > MAX_MESSAGE:
        findings.append(_finding("error",
                                 "the message is %d characters; the limit is %d (counted after "
                                 "`<category>: `)" % (len(message), MAX_MESSAGE)))
    if len(lines) > 1 and lines[1].strip():
        findings.append(_finding("error",
                                 "the body starts on line 2; separate the subject from the body with one "
                                 "blank line"))
    return findings


def render(source: str, findings: list) -> str:
    """One message's findings as the human-readable block the caller prints."""
    errors = [f for f in findings if f["level"] == "error"]
    status = "FAIL" if errors else ("warn" if findings else "ok")
    out = ["commitlint: %s: %s" % (source, status)]
    for f in findings:
        out.append("  %s: %s" % (f["level"], f["message"]))
    return "\n".join(out)


# --------------------------------------------------------------------------------------------------
# where to read from
# --------------------------------------------------------------------------------------------------
def find_root(explicit: str | None) -> str:
    if explicit:
        return os.path.abspath(explicit)
    proc = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    if proc.returncode == 0 and proc.stdout.strip():
        return proc.stdout.strip()
    return os.getcwd()


def _git_rev_parse_dir(root: str, what: str) -> str:
    proc = subprocess.run(["git", "-C", root, "rev-parse", what], capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    value = proc.stdout.strip() if proc.returncode == 0 else ""
    if not value:
        return ""
    if not os.path.isabs(value):
        value = os.path.join(root, value)
    return value


def hook_dir(root: str) -> str:
    """The `.git` a hook belongs in - the *common* git dir, so a worktree's hook lands with its clone."""
    gitdir = _git_rev_parse_dir(root, "--git-common-dir") or _git_rev_parse_dir(root, "--git-dir")
    if not gitdir:
        gitdir = os.path.join(root, ".git")
    return os.path.join(gitdir, "hooks")


# --------------------------------------------------------------------------------------------------
# modes
# --------------------------------------------------------------------------------------------------
def check_text(source: str, text: str, members: dict, out=sys.stdout) -> int:
    findings = lint_message(text, members)
    print(render(source, findings), file=out)
    return 1 if any(f["level"] == "error" for f in findings) else 0


def check_file(path: str, members: dict, out=sys.stdout) -> int:
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError as exc:
        print("commitlint: cannot read %s: %s" % (path, exc), file=sys.stderr)
        return 2
    return check_text(path, text, members, out)


def check_last(root: str, count: int, members: dict, out=sys.stdout) -> int:
    if count <= 0:
        print("commitlint: --last %d checks nothing" % count, file=sys.stderr)
        return 2
    proc = subprocess.run(["git", "-C", root, "log", "-n", str(count),
                           "--format=%H" + FIELD_SEP + "%B" + HISTORY_SEP],
                          capture_output=True, text=True, encoding="utf-8", errors="replace")
    if proc.returncode != 0:
        print("commitlint: git log failed in %s: %s" % (root, (proc.stderr or "").strip()), file=sys.stderr)
        return 2

    records = []
    for chunk in proc.stdout.split(HISTORY_SEP):
        chunk = chunk.lstrip("\n")
        if not chunk.strip():
            continue
        sha, _, body = chunk.partition(FIELD_SEP)
        records.append((sha.strip(), body))

    if not records:
        print("commitlint: --last %d: no commits found - nothing checked" % count, file=sys.stderr)
        return 2

    clean = warn_only = violations = 0
    lines = []
    for sha, body in records:
        findings = lint_message(body, members)
        errors = [f for f in findings if f["level"] == "error"]
        subject = _message_lines(body)
        subject = subject[0] if subject else "(empty)"
        if errors:
            violations += 1
            verdict = "FAIL"
        elif findings:
            warn_only += 1
            verdict = "warn"
        else:
            clean += 1
            verdict = "ok"
        lines.append("  %s %s: %s" % (sha[:12], verdict, subject))
        for f in findings:
            lines.append("      %s: %s" % (f["level"], f["message"]))

    print("commitlint: --last %d: %d clean, %d with violations, %d with warnings only"
          % (count, clean, violations, warn_only), file=out)
    if violations:
        for line in lines:
            print(line, file=out)
    return 1 if violations else 0


def install_hook(root: str, force: bool, out=sys.stdout) -> int:
    path = os.path.join(hook_dir(root), "commit-msg")
    if os.path.exists(path):
        try:
            with open(path, encoding="utf-8", errors="replace") as handle:
                existing = handle.read()
        except OSError as exc:
            print("commitlint: cannot read the existing hook %s: %s" % (path, exc), file=sys.stderr)
            return 1
        if existing != HOOK_BODY and HOOK_MARKER not in existing and not force:
            print("commitlint: %s exists and is not this tool's; pass --force to replace it"
                  % path, file=sys.stderr)
            return 1
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(HOOK_BODY)
        os.chmod(path, 0o755)
    except OSError as exc:
        print("commitlint: cannot install %s: %s" % (path, exc), file=sys.stderr)
        return 1
    print("commitlint: installed %s" % path, file=out)

    hooks_path = subprocess.run(["git", "-C", root, "config", "--get", "core.hooksPath"],
                                capture_output=True, text=True, encoding="utf-8",
                                errors="replace").stdout.strip()
    if hooks_path:
        print("commitlint: core.hooksPath is `%s`, so git will not consult this hook - it is a "
              "per-clone convenience, not the enforcement path" % hooks_path, file=out)
    return 0


def main(argv: list | None = None) -> int:
    ap = argparse.ArgumentParser(description="Lint commit messages against AGENTS.md's convention.")
    ap.add_argument("message_file", nargs="?",
                    help="the message file a `commit-msg` hook receives")
    ap.add_argument("--message", metavar="MSG", help="lint MSG (a subject, optionally with a body)")
    ap.add_argument("--last", type=int, metavar="N", help="lint the last N commits")
    ap.add_argument("--install-hook", action="store_true",
                    help="write .git/hooks/commit-msg (untracked, per-clone - a convenience)")
    ap.add_argument("--force", action="store_true",
                    help="with --install-hook: replace an existing hook that is not this tool's")
    ap.add_argument("--root", metavar="DIR",
                    help="read the tree and history from DIR (default: this repository's toplevel)")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        import commitlint_selftest
        return commitlint_selftest.selftest()

    root = find_root(args.root)

    if args.install_hook:
        return install_hook(root, args.force)

    chosen = [args.message_file is not None, args.message is not None, args.last is not None]
    if sum(chosen) != 1:
        # ap.error exits 2 - "nothing was checked", the house meaning of 2
        ap.error("pass exactly one of: a message file, --message, --last N "
                 "(or --install-hook / --selftest)")

    members = derive_members(root)

    if args.message_file is not None:
        return check_file(args.message_file, members)
    if args.message is not None:
        return check_text("--message", args.message, members)
    return check_last(root, args.last, members)


if __name__ == "__main__":
    sys.exit(main())
