#!/usr/bin/env python3
"""Check that every relative markdown link in the docs resolves, and flag root-absolute ones. Spec: docs/tools/spec/doclinks.md.
CLI: doclinks.py [--root DIR] [--json] [FILE...]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import sys
from urllib.parse import unquote

from tools.lib import repo as _repo

#: What is checked: every markdown file under these trees, plus these files (paths relative to the root).
TREES = ("docs", ".claude")
FILES = ("CLAUDE.md",)
#: Directories under those trees that are not this repository's docs: other checkouts (the agent worktrees).
EXCLUDED = (".claude/worktrees",)
#: An inline link or image `[text](target "title")`, and a reference definition `[label]: target`.
INLINE_RE = re.compile(r"!?\[(?:[^\[\]]|\[[^\]]*\])*\]\(\s*(<[^>]*>|[^)\s]+)(?:\s+(?:\"[^\"]*\"|'[^']*'))?\s*\)")
REFDEF_RE = re.compile(r"^[ \t]{0,3}\[[^\]]+\]:[ \t]*(<[^>]*>|\S+)", re.M)
#: A scheme (`https:`, `mailto:`, `file:`) or a protocol-relative `//host`: not a repository path.
EXTERNAL_RE = re.compile(r"^(?:[A-Za-z][A-Za-z0-9+.-]*:|//)")
FENCE_RE = re.compile(r"^\s{0,3}(```|~~~)")
CODE_SPAN_RE = re.compile(r"(`+)([^\n]+?)\1")


def markdown_files(root: str) -> list[str]:
    """Every checked file, relative to `root`, forward slashes, sorted."""
    out = [f for f in FILES if os.path.isfile(os.path.join(root, f))]
    for tree in TREES:
        base = os.path.join(root, tree)
        for dirpath, dirnames, filenames in os.walk(base):
            here = os.path.relpath(dirpath, root).replace("\\", "/")
            dirnames[:] = sorted(d for d in dirnames if d not in ("node_modules", "__pycache__")
                                 and "%s/%s" % (here, d) not in EXCLUDED)
            for fn in filenames:
                if fn.lower().endswith(".md"):
                    out.append(os.path.relpath(os.path.join(dirpath, fn), root).replace("\\", "/"))
    return sorted(set(out))


def prose(text: str) -> str:
    """The text with fenced blocks and code spans blanked (newlines kept), so a link-shaped string in code is no link."""
    lines = text.split("\n")
    fence = None
    for i, line in enumerate(lines):
        m = FENCE_RE.match(line)
        if fence is not None:
            if m and m.group(1) == fence:
                fence = None
            lines[i] = ""
        elif m:
            fence = m.group(1)
            lines[i] = ""
    body = "\n".join(lines)
    return CODE_SPAN_RE.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), body)


def links(text: str) -> list[tuple[int, str]]:
    """`(line, target)` for every inline link, image and reference definition outside code."""
    clean = prose(text)
    out = []
    for rx in (INLINE_RE, REFDEF_RE):
        for m in rx.finditer(clean):
            target = m.group(1)
            if target.startswith("<") and target.endswith(">"):
                target = target[1:-1]
            out.append((clean.count("\n", 0, m.start()) + 1, target))
    return sorted(out)


def check_link(root: str, rel: str, target: str) -> str | None:
    """None when the link is fine (or not a repository path), else why it is not."""
    if not target or target.startswith("#") or EXTERNAL_RE.match(target):
        return None
    path = unquote(target.split("#", 1)[0].split("?", 1)[0])
    if not path:
        return None
    if path.startswith("/"):
        return "root-absolute link (write it relative to %s)" % rel
    resolved = os.path.normpath(os.path.join(root, os.path.dirname(rel), path))
    if not os.path.exists(resolved):
        return "does not resolve (%s)" % os.path.relpath(resolved, root).replace("\\", "/")
    return None


def run(root: str, files: list[str] | None = None) -> dict:
    """`{files, links, findings: [{file, line, target, reason}]}` over the checked files (or `files`)."""
    files = files or markdown_files(root)
    findings, count = [], 0
    for rel in files:
        with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        for line, target in links(text):
            count += 1
            reason = check_link(root, rel, target)
            if reason:
                findings.append({"file": rel, "line": line, "target": target, "reason": reason})
    return {"files": len(files), "links": count, "findings": findings}


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("files", nargs="*", help="check only these markdown files (relative to --root)")
    ap.add_argument("--root", default=None, help="the tree to check (default: the invocation's)")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    root = os.path.abspath(args.root) if args.root else _repo.repo_root()
    res = run(root, [f.replace("\\", "/") for f in args.files] or None)
    if args.json:
        print(json.dumps(res, indent=2))
    else:
        for f in res["findings"]:
            print("%s:%d  %s  %s" % (f["file"], f["line"], f["target"], f["reason"]))
        print("doclinks: %d link(s) in %d file(s), %d broken" % (res["links"], res["files"], len(res["findings"])))
    return 1 if res["findings"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
