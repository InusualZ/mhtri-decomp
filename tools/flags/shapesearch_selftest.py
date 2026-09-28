#!/usr/bin/env python3
"""Self-test for `shapesearch.py`'s candidate-expression mode (`--expr` / `--expr-file`).

    python tools/flags/shapesearch_selftest.py
    python tools/flags/shapesearch.py --selftest

Offline: no compiler, no target object, no repository state. It pins the pieces that turned "twelve
spellings hand-written into a scratch `.c`" into a mode:

* `expr_candidates` - the `--expr` order, the `---` file separator, the one-line-per-candidate file, the
  `#` comment skip, the `shapes.norm_code` dedupe, and a missing file refusing instead of scoring zero;
* `build_parser` actually wires `--expr`/`--expr-file` (the flag was the whole gap);
* `divergence_line` names the first differing row and its kind, or says MATCH - the "per candidate, its
  score and first divergence" half that the score alone does not answer.
"""
from __future__ import annotations

import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (HERE, os.path.dirname(HERE)):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import shapesearch as ss  # noqa: E402

CHECKS = 0
FAILURES: list[str] = []


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


def test_expr_candidates() -> None:
    check("a single --expr is one labelled candidate",
          ss.expr_candidates(["return 0;"], None), [("expr#1", "return 0;")])
    check("repeated --expr keeps its order",
          [lab for lab, _ in ss.expr_candidates(["a;", "b;", "c;"], None)],
          ["expr#1", "expr#2", "expr#3"])
    # dedupe is by normalised code, so a body written twice (whitespace/comments apart) compiles once
    check("a body repeated with only whitespace apart is one candidate",
          ss.expr_candidates(["a = 1;", "  a  =  1;  "], None), [("expr#1", "a = 1;")])
    check("distinct bodies stay distinct",
          [lab for lab, _ in ss.expr_candidates(["a = 1;", "a = 2;"], None)], ["expr#1", "expr#2"])

    with tempfile.TemporaryDirectory() as tmp:
        dashed = os.path.join(tmp, "dashed.txt")
        with open(dashed, "w", encoding="utf-8") as fh:
            fh.write("return a + b;\n---\nreturn a - b;\n---\nreturn a * b;\n")
        got = ss.expr_candidates([], dashed)
        check("a --- file yields one candidate per section", [lab for lab, _ in got],
              ["file#1", "file#2", "file#3"])
        check("... with the body text intact",
              [body.strip() for _lab, body in got],
              ["return a + b;", "return a - b;", "return a * b;"])

        lines = os.path.join(tmp, "lines.txt")
        with open(lines, "w", encoding="utf-8") as fh:
            fh.write("# a comment\nreturn a + b;\n\nreturn a - b;\n")
        got = ss.expr_candidates([], lines)
        check("a line file skips comments and blanks", [(lab, body) for lab, body in got],
              [("file#1", "return a + b;"), ("file#2", "return a - b;")])

        missing = os.path.join(tmp, "nope.txt")
        try:
            ss.expr_candidates([], missing)
            check("a missing --expr-file refuses", "no SystemExit", "SystemExit")
        except SystemExit as exc:
            check("a missing --expr-file refuses", "nope.txt" in str(exc), True)

        check("--expr and --expr-file combine, --expr first",
              [lab for lab, _ in ss.expr_candidates(["x;"], dashed)],
              ["expr#1", "file#1", "file#2", "file#3"])


def test_parser() -> None:
    ap = ss.build_parser()
    ns = ap.parse_args(["-u", "Pl/pl_act", "--expr", "return 0;", "--expr", "return 1;"])
    check("build_parser wires --expr", ns.expr, ["return 0;", "return 1;"])
    ns = ap.parse_args(["-u", "Pl/pl_act", "--expr-file", "c.txt"])
    check("build_parser wires --expr-file", ns.expr_file, "c.txt")
    ns = ap.parse_args(["-u", "Pl/pl_act", "--expr", "return 0;"])
    check("--expr defaults to empty", ap.parse_args(["-u", "Pl/pl_act"]).expr, [])


def test_divergence() -> None:
    check("no differing row is MATCH", ss.divergence_line([("", "a", "a"), ("", "b", "b")]),
          "MATCH (no differing row)")
    check("an empty probe is MATCH", ss.divergence_line([]), "MATCH (no differing row)")
    rows = [("", "cmpi r3, 0", "cmpi r3, 0"),
            ("DIFF_ARG_MISMATCH", "addi r3, r3, 0x4", "addi r3, r3, 0x8"),
            ("DIFF_REPLACE", "nop", "b .+8")]
    check("the first differing row is named with its kind and both sides",
          ss.divergence_line(rows),
          "first divergence at row 1 (DIFF_ARG_MISMATCH): target `addi r3, r3, 0x4` | "
          "ours `addi r3, r3, 0x8`")


def selftest() -> int:
    test_expr_candidates()
    test_parser()
    test_divergence()
    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECKS)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
