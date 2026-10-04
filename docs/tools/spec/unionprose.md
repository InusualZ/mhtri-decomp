# `unionprose` - The one union rule: code hunks union, prose hunks take the superset, mixed warns; shared by mergebranch and unionresolve

<!-- generated from the module docstring of `tools/units/unionprose.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The one union rule: **code hunks union, prose hunks take the superset**, shared by every resolver.

## Users

imported by `mergebranch`, `unionresolve`

## CLI

```
python tools/units/unionprose.py --selftest
```
Flags: `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: diff3 text -> text.

## Invariants and rules

* The distinction is **per hunk, not per file**:
* an **additive declaration block** - one side's lines are additions to the other's - unions as before (`ours`, then `theirs`), which every registration merge depends on;
* a **comment paragraph both sides rewrote** takes the **superset** side - the copy that already carries every non-blank line the other side changed relative to the shared base - never a union;
* a **mixed** comment-and-code hunk prefers the superset when one is derivable and otherwise keeps the union and **warns**, naming the file; a prose hunk with no superset is **blocked**, never unioned.
* `prose_line` is deliberately line-local, not a C tokenizer: a line is prose when it is blank, starts with `//`, `/*` or `*`, or lies inside an open block comment (the paragraph's opening `/*` is frequently *before* the conflict hunk, so that state is carried in). That is exactly the shape that separates the two classes.

## Lib dependencies

none (stdlib). The implementation is `tools/units/merge/unionprose.py`, which also holds the one diff3 hunk reader (`segments`/`Hunk`, used by the union and by `unionguard.has_base_region`), `markers_in`, and `cover` - the superset computation `prose_superset` (one hunk) and `addadd_choice` (one add/add file) both decide on. `tools/units/unionprose.py` is a shim.

## Test contract

Tier: fixture (pure text).
`tools/tests/units/test_unionprose.py` (`--selftest` forwards); the real-conflict constants are in `tools/tests/units/merge_fixtures.py`.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `tools/units/mergebranch.py` (the lane's `main`-into-branch merge) and `tools/units/unionresolve.py` (the landing path's resolver, called by `land.py`) both union a `--diff3` conflict, and they carried the **same** defect independently: a comment paragraph both sides rewrote was unioned line-by-line, which duplicated the prose mid-sentence (`/* … /* …`) and reintroduced the older side's generated names. The merger lane hit it twice in one merge on 2026-09-29 (`include/unsplit/lobby.h`, `include/lobby/fn_801F3294.h`) and resolved by hand; `mergebranch` was fixed in `7099e70d9`, but `unionresolve.union_text` still had the old append-union. Fixing one and not the other is exactly how the two drift, so the classification and the superset rule live here once, and both import it.
