#!/usr/bin/env python3
"""Regenerate this skill's references/ from the repository's canonical playbook.

The repo is the source of truth: `docs/matching.md` holds the playbook and `CLAUDE.md` holds the
index/todo table. This skill only carries the *method* plus generated copies, so nothing here should
ever be hand-edited.

Usage:
    python scripts/sync_reference.py            # write references/*.md from docs/matching.md
    python scripts/sync_reference.py --check    # exit 1 when the copies are stale (for CI or hooks)
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SKILL = os.path.dirname(HERE)
REFS = os.path.join(SKILL, "references")


def repo_root(start):
    d = os.path.abspath(start)
    while True:
        if os.path.exists(os.path.join(d, "configure.py")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise SystemExit("repo root (configure.py) not found above %s" % start)
        d = parent


HEADER = """<!-- GENERATED FILE - do not edit.
     Source: {source}
     Regenerate: python {script}
-->

"""


def split_sections(text):
    """[(heading, body)] for every `## ` section, plus the leading intro as ('', intro)."""
    parts = re.split(r"^## ", text, flags=re.M)
    out = [("", parts[0])]
    for p in parts[1:]:
        heading, _, body = p.partition("\n")
        out.append((heading.strip(), body))
    return out


def build(root):
    src = os.path.join(root, "docs", "matching.md")
    text = open(src, encoding="utf-8").read()
    rel = os.path.relpath(src, root).replace(os.sep, "/")
    script = ".claude/skills/mwcc-unit-matching/scripts/sync_reference.py"

    intro, numbered, ruled, example = [], [], [], []
    for heading, body in split_sections(text):
        if heading == "":
            intro.append(body)
        elif re.match(r"^\d+\.", heading):
            numbered.append("## %s\n%s" % (heading, body))
        elif heading.lower().startswith("ruled out"):
            ruled.append("## %s\n%s" % (heading, body))
        elif heading.lower().startswith("worked example"):
            example.append("## %s\n%s" % (heading, body))
        else:
            numbered.append("## %s\n%s" % (heading, body))   # unknown section: keep it in the playbook

    files = {
        "playbook.md": HEADER.format(source=rel, script=script)
                      + "".join(intro).strip() + "\n\n"
                      + "".join(numbered).strip() + "\n",
        "ruled-out.md": HEADER.format(source=rel, script=script)
                        + ("".join(ruled).strip() or "## Ruled out\n\n(none recorded)\n") + "\n",
        "worked-example.md": HEADER.format(source=rel, script=script)
                             + ("".join(example).strip() or "## Worked example\n\n(none recorded)\n") + "\n",
    }
    return files


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="do not write; fail when stale")
    ap.add_argument("--repo", help="repository root (default: search upwards)")
    args = ap.parse_args()

    root = repo_root(args.repo or SKILL)
    files = build(root)
    stale = []
    for name, content in files.items():
        path = os.path.join(REFS, name)
        old = open(path, encoding="utf-8").read() if os.path.exists(path) else None
        if old == content:
            continue
        stale.append(name)
        if not args.check:
            os.makedirs(REFS, exist_ok=True)
            open(path, "w", encoding="utf-8", newline="\n").write(content)
    if args.check and stale:
        print("stale: %s\nrun: python %s" % (", ".join(stale), os.path.relpath(__file__, root)), file=sys.stderr)
        return 1
    print(("would update: " if args.check else "updated: ") + (", ".join(stale) if stale else "nothing")
          + "  (source: docs/matching.md)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
