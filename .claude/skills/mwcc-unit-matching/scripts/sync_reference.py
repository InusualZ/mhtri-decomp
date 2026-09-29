#!/usr/bin/env python3
"""Regenerate this skill's references/matching/ from the repository's canonical playbook.

The repo is the source of truth: `docs/matching/` holds the playbook (README, generated index, toolbox, one
`NNN-slug.md` per idea, ruled-out, todo, examples/, notes/, and later the `NNN-slug.cpp` demonstrations). This
skill carries a byte-for-byte copy in `references/matching/` so it stays portable (a fresh session or a
subagent loads it without the repository docs), so nothing under `references/` may be hand-edited.

Usage:
    python scripts/sync_reference.py            # copy docs/matching/** into references/matching/
    python scripts/sync_reference.py --check    # exit 1 when the copy is stale (for CI or hooks)
"""
import argparse
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL = os.path.dirname(HERE)
DEST = os.path.join(SKILL, "references", "matching")
COPIED = (".md", ".cpp", ".h")


def repo_root(start):
    d = os.path.abspath(start)
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s" % start)
        d = parent


def tree(base):
    """{relative posix path: bytes} for every copied file under `base`."""
    out = {}
    for dirpath, _dirs, names in os.walk(base):
        for n in names:
            if n.endswith(COPIED):
                full = os.path.join(dirpath, n)
                rel = os.path.relpath(full, base).replace(os.sep, "/")
                with open(full, "rb") as f:
                    out[rel] = f.read()
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="do not write; fail when stale")
    ap.add_argument("--repo", help="repository root (default: search upwards)")
    args = ap.parse_args()

    root = repo_root(args.repo or SKILL)
    src = os.path.join(root, "docs", "matching")
    if not os.path.isdir(src):
        raise SystemExit("refusing: %s does not exist" % src)
    want = tree(src)
    have = tree(DEST) if os.path.isdir(DEST) else {}
    stale = sorted(k for k in want if have.get(k) != want[k])
    extra = sorted(k for k in have if k not in want)
    if args.check:
        if stale or extra:
            print("stale: references/matching/ differs from docs/matching/ (%d changed/missing, %d extra: %s)\n"
                  "run: python %s" % (len(stale), len(extra), ", ".join((stale + extra)[:6]),
                                       os.path.relpath(__file__, root)), file=sys.stderr)
            return 1
        print("references/matching/ is in sync with docs/matching/ (%d files)" % len(want))
        return 0
    for k in stale:
        path = os.path.join(DEST, *k.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(want[k])
    for k in extra:
        os.remove(os.path.join(DEST, *k.split("/")))
    for dirpath, dirs, files in os.walk(DEST, topdown=False):
        if not dirs and not files and dirpath != DEST:
            shutil.rmtree(dirpath)
    print("updated: %d written, %d removed  (source: docs/matching/)" % (len(stale), len(extra)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
