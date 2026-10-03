# `unionresolve` - The landing path's append-union of splits/configure conflicts plus the four invariant assertions (`check_union`)

<!-- generated from the module docstring of `tools/units/unionresolve.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The union resolver and its invariant assertions, for `land.py`.

## Users

the landing gate (1); docs (1); imported by `land`, `rescue`

## CLI

```
python tools/units/unionresolve.py --selftest
```
Flags: `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: conflicted text -> text, verdict.

## Invariants and rules

* Eight of the thirteen live branches conflict with `main` on **one** class, and it is not a defect: two sibling bands register *adjacent address ranges*, so their `config/RMHE08/splits.txt` blocks and `configure.py` `Object(...)` lines append at the *same anchor*. That is an add/add conflict whose resolution is the pure **append-union** - ours block then theirs - which is also the address order both files require. There are no header conflicts in this class.
* It also carries the assertions a wrong union breaks *silently* - the reason the union is not trusted:
* **no duplicate unit key** in the merged `splits.txt`;
* **no duplicate `Object()` line** in the merged `configure.py`;
* **no overlapping `.text`/`extab`/`extabindex` range anywhere** in the merged `splits.txt`;
* **every unit `main` registered is still present** in both files (the union only adds; it must never drop a registration `main` already had, which is what an apply that took one side's whole file does).
* `land.resolve_conflicts` is the only caller: it unions the conflicted working-tree text in memory, runs `check_union` on the result *before* writing anything, and refuses on any violation - so a bad union never touches the tree.

## Lib dependencies

text, project, merge (unionprose).

## Test contract

Tier: fixture (pure text).
Today's selftest (`tools/units/unionresolve_selftest.py`): Pure text: no git, no repository state, no build. The cases pin the union (`ours` then `theirs`, the `--diff3` base section dropped, a clean file passed through) and each of the four invariant assertions `check_union` makes, because a wrong union breaks *silently* and the assertions are the only thing that sees it. The end-to-end git fixture, including the unsafe-union refusal, lives in `land.py --selftest`.
Target: `tools/tests/units/test_unionresolve.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The union is **per hunk, not per file** (`unionprose.union_markers`, the one rule shared with `mergebranch.py`): an additive declaration block still appends, but a **comment paragraph both sides rewrote** takes the **superset** side. This module's `union_text` used to append every hunk, which is the same prose-union defect `mergebranch` was fixed for (`7099e70d9`) - it duplicated the paragraph mid-sentence and reintroduced the older side's generated names (2026-09-29: `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h`). `land.py` refuses a header conflict before it reaches this union anyway, but `union_text`/`union_file` are public and a hand `union_file <header>` was a live path to the defect; now a prose hunk with no superset is `blocked` and `land._union_conflicts` refuses it rather than writing the placeholder.
* The union itself used to live in an **untracked** `.pi/bin/union.py`, driven by an untracked `.pi/bin/applybranch.sh` that ended in `git add -A` - "the land path depends on a script no reviewer or a fresh clone can see", and the `-A` sweep has bitten this campaign twice. This module is that resolver, brought into the repo and called by `land.py` directly, and it contains **no staging at all**: the caller stages the two scoped paths explicitly (`git add -- <paths>`), never the whole index.
