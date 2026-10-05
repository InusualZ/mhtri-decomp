#!/usr/bin/env python3
"""The facts-preserved gate: every fact token a diff removes must survive somewhere. Spec: docs/tools/spec/factscheck.md.
CLI: factscheck.py [--base REF] [--head REF] [--root TREE] [--explain] [--json] [PATH...] [--selftest]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import cli
from tools.lib import facts as _facts
from tools.lib import findings as _findings
from tools.lib import repo as _repo
from tools.lib.git import Git

TOOL = cli.Tool("factscheck", "docs/tools/spec/factscheck.md", tests="tools/tests/units/test_factscheck.py",
                description=(__doc__ or "").splitlines()[0], common=("json", "root"))


def run(root: str, base: str = "main", head: str | None = None, paths=()) -> dict:
    """Check the diff `base..head` (`head=None`: the working tree): `{spans, removed_spans, failures: [{file, line,
    tokens, text}], tokens_checked}`."""
    git = Git(root)
    base = git.merge_base(base, head or "HEAD") or base
    args = ["diff", "--no-renames", "--no-color", "--no-ext-diff", "-U0", base] + ([head] if head else [])
    if paths:
        args += ["--", *paths]
    p = git.run(*args)
    if p.returncode != 0:
        raise RuntimeError("git diff failed: %s" % (p.stderr or "").strip())
    spans = _facts.parse_diff(p.stdout or "")
    tree = _facts.Tree(root, head)
    corpora = _facts.Corpora(tree)
    texts = {f: tree.unit_text(f) for f in sorted({s.file for s in spans})}
    failures, checked, explained = [], 0, []
    for s in spans:
        verdicts = _facts.judge(s.text, texts.get(s.file) or "", corpora)
        checked += len(verdicts)
        bad = []
        for tok, where in verdicts:
            if not where and tok not in bad:
                bad.append(tok)
        explained.append({"file": s.file, "line": s.line, "tokens": verdicts})
        if bad:
            failures.append({"file": s.file, "line": s.line, "tokens": bad, "text": s.text})
    return {"base": base, "head": head or "(working tree)", "removed_spans": len(spans), "tokens_checked": checked,
            "failures": failures, "explained": explained}


def main(args) -> object:
    root = _repo.worktree_root(args.root)
    res = run(root, args.base, args.head, args.paths)
    rows = [_findings.Finding("facts", f["file"], f["line"], tok, "removed and found nowhere")
            for f in res["failures"] for tok in f["tokens"]]
    verdict = _findings.Verdict.of(rows)
    if args.json:
        payload = {k: v for k, v in res.items() if k != "explained" or args.explain}
        print(_findings.render_json(TOOL.name, verdict, **payload))
        return _findings.exit_code(verdict)
    if args.explain:
        for e in res["explained"]:
            print("%s:%d" % (e["file"], e["line"]))
            for tok, where in e["tokens"]:
                print("    %-40s %s" % (tok, where or "UNMATCHED"))
    for f in res["failures"]:
        print("%s:%d: %d unmatched: %s" % (f["file"], f["line"], len(f["tokens"]), " ".join(f["tokens"])))
    print("factscheck: %s..%s: %d removed span(s), %d fact token(s) checked, %d span(s) with unmatched tokens"
          % (res["base"], res["head"], res["removed_spans"], res["tokens_checked"], len(res["failures"])))
    return verdict


def build_parser():
    ap = TOOL.parser()
    ap.add_argument("--base", default="main", help="the old side of the diff (default: main)")
    ap.add_argument("--head", default=None, help="the new side (default: the working tree)")
    ap.add_argument("--explain", action="store_true", help="name where every removed fact token survives")
    ap.add_argument("paths", nargs="*", help="limit the diff to these paths")
    return ap


if __name__ == "__main__":
    raise SystemExit(TOOL.run(main, parser=build_parser()))
