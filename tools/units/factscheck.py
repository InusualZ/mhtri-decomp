#!/usr/bin/env python3
"""The facts-preserved gate: every fact token a diff removes must survive somewhere. Spec: docs/tools/spec/factscheck.md.
CLI: factscheck.py [--base REF] [--head REF] [--allow-drop T --reason R] [--superseded-file F] [--explain] [PATH...]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import cli
from tools.lib import facts as _facts
from tools.lib import findings as _findings
from tools.lib import repo as _repo
from tools.lib.git import Git

TOOL = cli.Tool("factscheck", "docs/tools/spec/factscheck.md", tests="tools/tests/units/test_factscheck.py",
                description=(__doc__ or "").splitlines()[0], common=("json", "root"))


class AllowanceError(ValueError):
    """An allowance without its reason, or a superseded file line that is not `TOKEN[,TOKEN...]: reason`."""


def allowance_key(tok: str) -> tuple[str, object]:
    """How an allowance matches a removed token: by `lib.facts.classify` (a hex by value, `0x8009cd64` is
    `0x8009CD64`; a word or a number exactly)."""
    return _facts.classify(tok)


def parse_allowances(tokens=(), reason: str | None = None, superseded: str | None = None) -> dict:
    """`{key: (token as written, reason, origin)}` from `--allow-drop` (with its one `--reason`) and a superseded file
    (`TOKEN[,TOKEN...]: reason` per line; blank lines and `#` comments skipped). A missing reason refuses."""
    out: dict = {}

    def add(spelled: str, why: str, origin: str) -> None:
        for tok in (t.strip() for t in spelled.split(",")):
            if tok:
                out[allowance_key(tok)] = (tok, why, origin)

    if tokens:
        if not (reason or "").strip():
            raise AllowanceError("--allow-drop needs --reason: an allowance without a reason is not auditable")
        for spelled in tokens:
            add(spelled, reason.strip(), "--allow-drop")
    elif reason:
        raise AllowanceError("--reason without --allow-drop")
    if superseded:
        with open(superseded, encoding="utf-8") as fh:
            for n, line in enumerate(fh, 1):
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                toks, sep, why = line.partition(":")
                if not sep or not why.strip() or not toks.strip():
                    raise AllowanceError("%s:%d: expected `TOKEN[,TOKEN...]: reason`, got %r" % (superseded, n, line))
                add(toks, why.strip(), "%s:%d" % (superseded.replace("\\", "/"), n))
    return out


def run(root: str, base: str = "main", head: str | None = None, paths=(), allow: dict | None = None) -> dict:
    """Check the diff `base..head` (`head=None`: the working tree): `{spans, removed_spans, failures: [{file, line,
    tokens, text}], tokens_checked, dropped, stale_allowances}`. `allow` (`parse_allowances`) names tokens dropped on
    purpose: such a token that survives nowhere is listed in `dropped` with its reason instead of failing; an
    allowance that no otherwise-failing token used (it matches nothing, or the token survives anyway) is listed in
    `stale_allowances`. A token that survives is never an allowance's - it needs none."""
    allow = allow or {}
    used: set = set()
    dropped: list[dict] = []
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
            if where or tok in bad:
                continue
            key = allowance_key(tok)
            if key in allow:
                used.add(key)
                if not any(d["token"] == tok and d["file"] == s.file and d["line"] == s.line for d in dropped):
                    dropped.append({"file": s.file, "line": s.line, "token": tok, "reason": allow[key][1],
                                    "origin": allow[key][2]})
                continue
            bad.append(tok)
        explained.append({"file": s.file, "line": s.line, "tokens": verdicts})
        if bad:
            failures.append({"file": s.file, "line": s.line, "tokens": bad, "text": s.text})
    stale = [{"token": spelled, "reason": why, "origin": origin}
             for key, (spelled, why, origin) in allow.items() if key not in used]
    return {"base": base, "head": head or "(working tree)", "removed_spans": len(spans), "tokens_checked": checked,
            "failures": failures, "dropped": dropped, "stale_allowances": stale, "explained": explained}


def main(args) -> object:
    root = _repo.worktree_root(args.root)
    try:
        allow = parse_allowances(args.allow_drop, args.reason, args.superseded_file)
    except (AllowanceError, OSError) as exc:
        print("factscheck: %s" % exc, file=sys.stderr)
        return 2
    res = run(root, args.base, args.head, args.paths, allow)
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
    for d in res["dropped"]:
        print("%s:%d: %s dropped on purpose (%s) [%s]" % (d["file"], d["line"], d["token"], d["reason"], d["origin"]))
    for a in res["stale_allowances"]:
        print("stale allowance: %s (%s) [%s] matches no removed token that would fail"
              % (a["token"], a["reason"], a["origin"]))
    print("factscheck: %s..%s: %d removed span(s), %d fact token(s) checked, %d span(s) with unmatched tokens, "
          "%d token(s) dropped on purpose, %d stale allowance(s)"
          % (res["base"], res["head"], res["removed_spans"], res["tokens_checked"], len(res["failures"]),
             len(res["dropped"]), len(res["stale_allowances"])))
    return verdict


def build_parser():
    ap = TOOL.parser()
    ap.add_argument("--base", default="main", help="the old side of the diff (default: main)")
    ap.add_argument("--head", default=None, help="the new side (default: the working tree)")
    ap.add_argument("--explain", action="store_true", help="name where every removed fact token survives")
    ap.add_argument("--allow-drop", action="append", default=[], metavar="TOKEN[,TOKEN...]",
                    help="a fact token removed on purpose (an old name, a past measurement); needs --reason; repeatable")
    ap.add_argument("--reason", default=None, help="why the --allow-drop tokens are dropped (mandatory with it)")
    ap.add_argument("--superseded-file", default=None, metavar="FILE",
                    help="allowances, one `TOKEN[,TOKEN...]: reason` per line (# comments)")
    ap.add_argument("paths", nargs="*", help="limit the diff to these paths")
    return ap


if __name__ == "__main__":
    raise SystemExit(TOOL.run(main, parser=build_parser()))
