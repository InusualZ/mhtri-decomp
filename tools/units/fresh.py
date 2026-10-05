#!/usr/bin/env python3
"""Every derived artifact of a tree (`lib.artifacts`): fresh or stale, why, its cost and its refresh command.
Spec: docs/tools/spec/fresh.md. CLI: fresh.py [status | refresh [NAME ...]] [--unit SRC] [--json] [--root TREE]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json
import os

from tools.lib import artifacts, cli, repo

TOOL = cli.Tool("fresh", "docs/tools/spec/fresh.md", description=(__doc__ or "").splitlines()[0],
                common=("json", "root"))


def unit_context(root: str, unit: str | None) -> artifacts.Context:
    """The context for `root`, scoped to one unit's source when `--unit` names one (`src/` optional)."""
    if not unit:
        return artifacts.Context(root)
    rel = unit.replace("\\", "/").strip("/")
    rel = rel[len("src/"):] if rel.startswith("src/") else rel
    src = os.path.join(root, "src", *rel.split("/"))
    if not os.path.isfile(src):
        raise SystemExit("no source %s in %s" % ("src/" + rel, root))
    stem = os.path.splitext(rel)[0]
    obj = os.path.join(root, artifacts.BUILD_REL, "src", *(stem + ".o").split("/"))
    return artifacts.Context(root, src=src, obj=obj)


def render(rows: list[artifacts.Status], root: str) -> None:
    print("artifacts of %s" % root.replace("\\", "/"))
    print("%-17s %-8s %-11s %-46s %s" % ("artifact", "state", "cost", "refresh", "why"))
    for s in rows:
        print("%-17s %-8s %-11s %-46s %s" % (s.name, s.state, s.cost, s.command or "-", s.reason))
        if s.note:
            print("%-17s %s" % ("", s.note))


def main(args) -> int:
    root = os.path.abspath(args.root if args.root != "." else (repo.cwd_tree() or "."))
    ctx = unit_context(root, args.unit)
    names = args.names or None
    if args.cmd == "refresh":
        rows = artifacts.ensure(names or list(artifacts.REGISTRY), ctx, "auto", out=sys.stderr)
        rows = [r for r in rows if not names or r.name in artifacts.order(names)]
    else:
        rows = artifacts.statuses(ctx, names)
    if args.json:
        print(json.dumps({"root": root, "artifacts": [r.as_dict() for r in rows]}, indent=1))
    else:
        render(rows, root)
    return 0 if all(r.fresh for r in rows) else 1


def build_parser():
    ap = TOOL.parser()
    ap.add_argument("cmd", nargs="?", choices=("status", "refresh"), default="status",
                    help="status (default): one row per artifact; refresh: rebuild the stale ones in dependency order")
    ap.add_argument("names", nargs="*", metavar="NAME",
                    help="artifacts to report or refresh (default: all): %s" % ", ".join(artifacts.REGISTRY))
    ap.add_argument("--unit", default=None,
                    help="scope `objects` and `report` to one unit's source (e.g. Camellia/camellia.c)")
    return ap


if __name__ == "__main__":
    sys.exit(TOOL.run(main, parser=build_parser()))
