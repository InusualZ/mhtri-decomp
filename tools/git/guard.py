#!/usr/bin/env python3
"""The two pre-commit guards, as logic plus a CLI the hook calls.

`tools/git/hooks/pre-commit` used to be a shell script that could only *refuse* a bad commit.  This module
is the logic behind it now: it can say what a staged path needs, and its CLI does the fix.

    guard.py autocrlf    # warn (never refuse) when core.autocrlf=true, which silently defeats eol=lf
    guard.py eol         # EOL case: normalise a textish staged blob's CRs and re-stage it; refuse a binary
    guard.py localonly   # pull the LOCAL-ONLY block out of a staged AGENTS.md and re-stage it

The three decisions are plain functions - `localonly_case`, `eol_case`, `autocrlf_warning` - so a test can
call them directly (with the staged text or the staged paths handed in) without a shell and without first
building a git command; the CLI is the thin layer that reads the staged paths from git and performs the fix.

**The index blob, not the worktree.**  A commit carries the *index*, so the EOL case reads the staged blob
(`git cat-file -p :<path>`) rather than the file on disk: a CRLF blob in the index is what would land,
whatever the worktree looks like.  The `localonly` case reads the staged AGENTS.md the same way, so the
auto-pull fires on exactly the commit that would otherwise carry the block (non-negotiable 8).

**Why this lives here and not in `prepcommit.py`.**  `prepcommit.py` is a staging/commit-message CLI: it
walks `git status`, classifies paths into stage/refuse and writes a message.  The two guards are a different
concern and have to run for *any* commit, including a plain `git commit` that never calls prepcommit - so
they live in a small importable module with no side effects on import, which both the hook and
`guard_selftest.py` can call.
"""
from __future__ import annotations

import argparse
import datetime
import os
import subprocess
import sys

AGENTS_MD = "AGENTS.md"
LOCALONLY_PREFIX = "<!-- LOCAL-ONLY"
PENDING_MARKER = "localonly-pending"
LOCALONLY_SCRIPT = os.path.join("tools", "agents", "localonly.py")
STATE_REL = os.path.join(".pi", "local-only.state.json")


def _git(root, *args, input=None):
    return subprocess.run(["git", "-C", root, *args], capture_output=True, input=input)


def _git_text(root, *args) -> str:
    out = _git(root, *args).stdout
    return out.decode("utf-8", "replace")


def repo_root(path: str = ".") -> str:
    """The working-tree root for `git -C path`, else `path` itself.  Hooks run at the root already."""
    out = _git_text(path, "rev-parse", "--show-toplevel").strip()
    return out or os.path.abspath(path)


def staged_paths(root: str = ".") -> list[str]:
    """Every path staged for this commit that exists in the index (added/copied/modified/renamed)."""
    return [line for line in _git_text(root, "diff", "--cached", "--name-only", "--diff-filter=ACMR")
            .splitlines() if line]


def index_blob(root: str, path: str) -> bytes | None:
    """The staged blob's bytes (`git cat-file -p :<path>`), or None when the path is not in the index.

    `:` names the index, deliberately: the guard must judge what would be committed, not what is on disk.
    """
    out = _git(root, "cat-file", "-p", ":" + path)
    return out.stdout if out.returncode == 0 else None


# --- the three decisions (pure enough to call without a hook) ------------------------------------

def localonly_case(staged_paths: list[str], staged_text: str | None = None,
                   root: str = ".") -> str:
    """"pull the block, re-stage AGENTS.md" when the *staged* AGENTS.md has a marker line, else "nothing".

    `staged_text` is the staged content when the caller already has it (a test); otherwise it is read from
    the index blob.  The marker is matched at the **start of a line**, not as a substring: rule 8's own
    prose quotes the marker, and a substring test matched that text as if it were the block and refused
    three legitimate landings of a refactored AGENTS.md on 2026-09-28.
    """
    if AGENTS_MD not in staged_paths:
        return "nothing"
    if staged_text is None:
        blob = index_blob(root, AGENTS_MD)
        if blob is None:
            return "nothing"
        staged_text = blob.decode("utf-8", "replace")
    for line in staged_text.splitlines():
        if line.startswith(LOCALONLY_PREFIX):
            return "pull the block, re-stage AGENTS.md"
    return "nothing"


def eol_case(root: str, staged_paths: list[str]) -> list[dict]:
    """One row per staged blob that contains a CR byte.

    textish (no NUL byte) -> "normalise the worktree file and re-stage it", with the CR count;
    binary                -> "refuse", naming the file.  A binary is never rewritten: a CR in a blob whose
    bytes are not Text is ambiguous, and guessing would corrupt it.
    """
    rows: list[dict] = []
    for path in staged_paths:
        blob = index_blob(root, path)
        if not blob or b"\r" not in blob:
            continue
        crs = blob.count(b"\r")
        if b"\x00" in blob:
            rows.append({
                "path": path, "kind": "binary", "action": "refuse", "crs": crs,
                "message": "refuse %s - its staged blob is binary and contains %d CR byte(s); normalise it "
                           "by hand or keep it out of this commit" % (path, crs),
            })
        else:
            rows.append({
                "path": path, "kind": "textish", "action": "normalise", "crs": crs,
                "message": "normalise the worktree file and re-stage it - %s has %d CR(s) in the staged "
                           "blob" % (path, crs),
            })
    return rows


def autocrlf_warning(root: str = ".") -> str | None:
    """A one-line warning while `core.autocrlf` is `true`, else None.  Warn, never refuse.

    On this host `autocrlf=true` was measured to defeat `eol=lf` on its own: the checkout came back CRLF
    (613 of 714 text files) while the blobs were LF.  It is a clone-local setting, so the guard only warns
    - refusing a legitimate commit over a config a reviewer cannot see would be worse.
    """
    out = _git(root, "config", "--get", "core.autocrlf")
    if out.returncode != 0:
        return None
    if out.stdout.decode("utf-8", "replace").strip().lower() == "true":
        return ("core.autocrlf=true silently defeats `eol=lf` (git rewrites the checkout to CRLF); it is a "
                "clone-local setting - fix it here with: git config core.autocrlf false")
    return None


# --- what the CLI does with each decision ---------------------------------------------------------

def strip_cr(data: bytes) -> bytes:
    """CRLF and lone CR both become LF - the only EOL change this guard ever makes."""
    return data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def pending_path(root: str) -> str | None:
    """The LOCAL-ONLY pending marker's path *in this worktree's git dir* (`--git-path` rewrites it)."""
    out = _git_text(root, "rev-parse", "--git-path", PENDING_MARKER).strip()
    if not out:
        return None
    return out if os.path.isabs(out) else os.path.join(root, out)


def _force_index_lf(root: str, path: str, data: bytes) -> bool:
    """Put exactly `data` into the index for `path`, filters bypassed.

    The normalisation has to be the *bytes we computed*: a repo whose `core.autocrlf` or attributes would
    re-add CRs on `git add` must not undo the fix, so this writes the blob and points the index entry at it.
    """
    sha = _git(root, "hash-object", "-w", "--no-filters", "--stdin", input=data).stdout.decode().strip()
    ls = _git_text(root, "ls-files", "-s", "--", path).strip()
    if not sha or not ls:
        return False
    mode = ls.split()[0]
    return _git(root, "update-index", "--cacheinfo", "%s,%s,%s" % (mode, sha, path)).returncode == 0


def normalise_and_restage(root: str, path: str) -> bool:
    """Make the index blob for `path` CR-free and re-stage it.  -> whether the index is now LF.

    When the worktree file is the staged blob with CRs (the usual case, and the one that bit this host), the
    worktree is rewritten to LF and `git add` re-stages it - "normalise the worktree file and re-stage it".
    When the worktree differs by more than EOL, only the *index entry* is rewritten, so the guard never
    sweeps an unstaged edit into the commit.
    """
    blob = index_blob(root, path)
    if blob is None or b"\r" not in blob:
        return True
    lf = strip_cr(blob)
    worktree = os.path.join(root, path)
    if os.path.isfile(worktree):
        with open(worktree, "rb") as fh:
            content = fh.read()
        if strip_cr(content) == lf:
            with open(worktree, "wb") as fh:
                fh.write(lf)
            _git(root, "add", "--", path)
        else:
            _force_index_lf(root, path, lf)
    else:
        _force_index_lf(root, path, lf)
    after = index_blob(root, path)
    return after is not None and b"\r" not in after


def cmd_autocrlf(args) -> int:
    warning = autocrlf_warning(args.root)
    if warning:
        print("guard: " + warning)
    return 0


def cmd_eol(args) -> int:
    root = args.root
    refused = False
    for row in eol_case(root, staged_paths(root)):
        if row["action"] == "refuse":
            print("guard: " + row["message"])
            refused = True
            continue
        if normalise_and_restage(root, row["path"]):
            print("guard: normalised %d CR(s) in %s and re-staged it" % (row["crs"], row["path"]))
        else:
            print("guard: %s still has a CR in the index after re-staging - refusing" % row["path"])
            refused = True
    return 1 if refused else 0


def cmd_localonly(args) -> int:
    root = args.root
    decision = localonly_case(staged_paths(root), root=root)
    marker = pending_path(root)
    if decision == "nothing":
        if marker and os.path.exists(marker):
            # A previous commit was aborted after this hook pulled the block: the marker survived, so say
            # how it comes back.  On the next *successful* commit post-commit restores it; until then this
            # is the one command that does.
            print("guard: a LOCAL-ONLY pull is still pending (marker %s); restore it with: python %s push"
                  % (marker, LOCALONLY_SCRIPT))
        return 0
    script = os.path.join(root, LOCALONLY_SCRIPT)
    pull = subprocess.run([sys.executable, script, "pull"], cwd=root, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    if pull.returncode != 0:
        sys.stderr.write("pre-commit: refusing - the staged AGENTS.md carries the LOCAL-ONLY block and "
                         "pulling it failed:\n")
        sys.stderr.write((pull.stdout or "") + (pull.stderr or ""))
        sys.stderr.write("  restore the block with: python %s push\n" % LOCALONLY_SCRIPT)
        return 1
    add = _git(root, "add", "--", AGENTS_MD)
    if add.returncode != 0:
        sys.stderr.write("pre-commit: refusing - pulled the LOCAL-ONLY block but could not re-stage "
                         "AGENTS.md: %s\n" % add.stderr.decode("utf-8", "replace").strip())
        sys.stderr.write("  restore the block with: python %s push\n" % LOCALONLY_SCRIPT)
        return 1
    if marker:
        os.makedirs(os.path.dirname(marker) or root, exist_ok=True)
        with open(marker, "w", encoding="utf-8") as fh:
            fh.write("pulled %s; post-commit pushes the block back\n"
                     % datetime.datetime.now().isoformat(timespec="seconds"))
    print("guard: the staged AGENTS.md carried a LOCAL-ONLY marker line; pulled the block and re-staged "
          "AGENTS.md (stored in %s)" % STATE_REL)
    print("guard:   if the commit fails, restore it with: python %s push" % LOCALONLY_SCRIPT)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="the pre-commit guards behind tools/git/hooks/pre-commit")
    ap.add_argument("--root", default=None, help="repository root (default: `git rev-parse --show-toplevel`)")
    sub = ap.add_subparsers(dest="cmd")
    for name, fn, blurb in (
        ("autocrlf", cmd_autocrlf, "warn when core.autocrlf=true defeats eol=lf (never refuses)"),
        ("eol", cmd_eol, "normalise CRs in staged text blobs and re-stage; refuse staged binary blobs"),
        ("localonly", cmd_localonly, "pull the LOCAL-ONLY block out of a staged AGENTS.md and re-stage it"),
    ):
        sub.add_parser(name, help=blurb).set_defaults(func=fn)
    args = ap.parse_args(argv)
    args.root = args.root or repo_root()
    if not args.cmd:
        ap.print_help()
        return 0
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
