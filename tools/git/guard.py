#!/usr/bin/env python3
"""The pre-commit guards, as logic plus a CLI the hook calls.

`tools/git/hooks/pre-commit` used to be a shell script that could only *refuse* a bad commit.  This module
is the logic behind it now: it can say what a staged path needs, and its CLI does the fix.

    guard.py autocrlf    # warn (never refuse) when core.autocrlf=true, which silently defeats eol=lf
    guard.py eol         # EOL case: normalise a textish staged blob's CRs and re-stage it; refuse a binary
    guard.py config      # refuse a staged config.yml change outside the relocation-analysis keys

The decisions are plain functions - `eol_case`, `autocrlf_warning`, `config_change` - so a test can
call them directly (with the staged text or the staged paths handed in) without a shell and without first
building a git command; the CLI is the thin layer that reads the staged paths from git and performs the fix.

**The index blob, not the worktree.**  A commit carries the *index*, so the EOL case reads the staged blob
(`git cat-file -p :<path>`) rather than the file on disk: a CRLF blob in the index is what would land,
whatever the worktree looks like.

**Why this lives here and not in `prepcommit.py`.**  `prepcommit.py` is a staging/commit-message CLI: it
walks `git status`, classifies paths into stage/refuse and writes a message.  The guards are a different
concern and have to run for *any* commit, including a plain `git commit` that never calls prepcommit - so
they live in a small importable module with no side effects on import, which both the hook and
`guard_selftest.py` can call.

**`config.yml` is ground truth, with one door.**  Its keys name the DOL, its hash, the selfile and the map
paths (docs/plan.md 7.18), so a change to any of them refuses.  The analyzer's relocation hints
(`block_relocations`, `add_relocations`) are the exception the owner ruled on 2026-10-03: they change what dtk
*reads* as a relocation, never what the DOL is, and a verified change to them is committable.  The one
implementation is `tools.lib.repo.config_change`, beside the ground-truth reader: the hook calls
`guard.py config` (staged vs HEAD) and `prepcommit.py` calls the function (worktree vs HEAD), so the two can
never disagree.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os

from tools.lib.git import Git
from tools.lib.repo import CONFIG_PATH, config_change

def _git(root, *args, input=None):
    return Git(root).run_bytes(*args, input=input)


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


# --- the two decisions (pure enough to call without a hook) ------------------------------------

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


def _committed_text(root: str, spec: str) -> str | None:
    out = _git(root, "cat-file", "-p", spec)
    return out.stdout.decode("utf-8", "replace") if out.returncode == 0 else None


# --- what the CLI does with each decision ---------------------------------------------------------

def strip_cr(data: bytes) -> bytes:
    """CRLF and lone CR both become LF - the only EOL change this guard ever makes."""
    return data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


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


def cmd_config(args) -> int:
    """The staged config.yml (the index) against HEAD's; silent and 0 when the index carries no change."""
    root = args.root
    verdict = config_change(_committed_text(root, "HEAD:" + CONFIG_PATH),
                            _committed_text(root, ":" + CONFIG_PATH))
    if verdict["changed"] or not verdict["ok"]:
        print("guard: %s %s" % ("allowing" if verdict["ok"] else "refusing", verdict["reason"]))
    return 0 if verdict["ok"] else 1


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="the pre-commit guards behind tools/git/hooks/pre-commit")
    ap.add_argument("--root", default=None, help="repository root (default: `git rev-parse --show-toplevel`)")
    sub = ap.add_subparsers(dest="cmd")
    for name, fn, blurb in (
        ("autocrlf", cmd_autocrlf, "warn when core.autocrlf=true defeats eol=lf (never refuses)"),
        ("eol", cmd_eol, "normalise CRs in staged text blobs and re-stage; refuse staged binary blobs"),
        ("config", cmd_config, "refuse a staged config.yml change outside block_relocations/add_relocations"),
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
