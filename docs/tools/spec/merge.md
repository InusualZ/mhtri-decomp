# `merge` - The merge family package `tools/units/merge/`: the one union rule, the landing union and its invariants, the apply guard, the lane merge

## Purpose

One package for every merge decision the campaign makes: `unionprose` (the one union rule - code hunks union, prose
hunks take the superset), `unionresolve` (the landing path's union of a registration conflict and the invariants
`check_union` asserts), `unionguard` (refuse a union of a conflicted apply unless every conflict is a disjoint
addition) and `mergebranch` (bring `main` into a held lane branch; its CLI and spec are `mergebranch.py` /
`mergebranch.md`). WP6 removed the three top-level shims (`tools/units/union{prose,resolve,guard}.py`); the modules
are imported by the landing gate (`tools/units/landing/branch.py`) and `mergebranch`, and have no CLI of their own.

## Users

`tools/units/landing/branch.py` (unionguard, unionresolve), `tools/units/merge/mergebranch.py`, `rescue.py`.

## Test contract

Tier: fixture - `tools/tests/units/merge/test_unionprose.py`, `test_unionresolve.py`, `test_unionguard.py` (each a
throwaway repository from `tools/tests/units/merge_fixtures.py`), and `tools/tests/units/test_mergebranch.py`.

## Module `unionprose`

The one union rule: code hunks union, prose hunks take the superset, mixed warns; shared by mergebranch and unionresolve

### Purpose

The one union rule: **code hunks union, prose hunks take the superset**, shared by every resolver.

### Users

imported by `mergebranch`, `unionresolve`

### CLI

None: a package module (its top-level shim was removed in WP6).

### Inputs and outputs

Inputs -> outputs: diff3 text -> text.

### Invariants and rules

* The distinction is **per hunk, not per file**:
* an **additive declaration block** - one side's lines are additions to the other's - unions as before (`ours`, then `theirs`), which every registration merge depends on;
* a **comment paragraph both sides rewrote** takes the **superset** side - the copy that already carries every non-blank line the other side changed relative to the shared base - never a union;
* a **mixed** comment-and-code hunk prefers the superset when one is derivable and otherwise keeps the union and **warns**, naming the file; a prose hunk with no superset is **blocked**, never unioned.
* `prose_line` is deliberately line-local, not a C tokenizer: a line is prose when it is blank, starts with `//`, `/*` or `*`, or lies inside an open block comment (the paragraph's opening `/*` is frequently *before* the conflict hunk, so that state is carried in). That is exactly the shape that separates the two classes.

### Lib dependencies

none (stdlib). The implementation is `tools/units/merge/unionprose.py`, which also holds the one diff3 hunk reader (`segments`/`Hunk`, used by the union and by `unionguard.has_base_region`), `markers_in`, and `cover` - the superset computation `prose_superset` (one hunk) and `addadd_choice` (one add/add file) both decide on.

### Test contract

Tier: fixture (pure text).
`tools/tests/units/merge/test_unionprose.py`; the real-conflict constants are in `tools/tests/units/merge_fixtures.py`.

### Known gaps

None recorded.

### History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `tools/units/mergebranch.py` (the lane's `main`-into-branch merge) and `tools/units/unionresolve.py` (the landing path's resolver, called by `land.py`) both union a `--diff3` conflict, and they carried the **same** defect independently: a comment paragraph both sides rewrote was unioned line-by-line, which duplicated the prose mid-sentence (`/* … /* …`) and reintroduced the older side's generated names. The merger lane hit it twice in one merge on 2026-09-29 (`include/unsplit/lobby.h`, `include/lobby/fn_801F3294.h`) and resolved by hand; `mergebranch` was fixed in `7099e70d9`, but `unionresolve.union_text` still had the old append-union. Fixing one and not the other is exactly how the two drift, so the classification and the superset rule live here once, and both import it.

## Module `unionresolve`

The landing path's append-union of splits/configure conflicts plus the four invariant assertions (`check_union`)

### Purpose

The union resolver and its invariant assertions, for `land.py`.

### Users

the landing gate (1); docs (1); imported by `land`, `rescue`

### CLI

None: a package module (its top-level shim was removed in WP6).

### Inputs and outputs

Inputs -> outputs: conflicted text -> text, verdict.

### Invariants and rules

* Eight of the thirteen live branches conflict with `main` on **one** class, and it is not a defect: two sibling bands register *adjacent address ranges*, so their `config/RMHE08/splits.txt` blocks and `configure.py` `Object(...)` lines append at the *same anchor*. That is an add/add conflict whose resolution is the pure **append-union** - ours block then theirs - which is also the address order both files require. There are no header conflicts in this class.
* It also carries the assertions a wrong union breaks *silently* - the reason the union is not trusted:
* **no duplicate unit key** in the merged `splits.txt`;
* **no duplicate `Object()` line** in the merged `configure.py`;
* **no overlapping `.text`/`extab`/`extabindex` range anywhere** in the merged `splits.txt`;
* **every unit `main` registered is still present** in both files (the union only adds; it must never drop a registration `main` already had, which is what an apply that took one side's whole file does).
* `land.resolve_conflicts` is the only caller: it unions the conflicted working-tree text in memory, runs `check_union` on the result *before* writing anything, and refuses on any violation - so a bad union never touches the tree.

### Lib dependencies

project (`Splits`, `object_calls`) and the package module `tools/units/merge/unionprose.py` (`union_markers`). The implementation is `tools/units/merge/unionresolve.py` (`landing/branch.py` and `rescue.py` import it).

### Test contract

Tier: fixture (pure text).
Today's selftest (`tools/units/unionresolve_selftest.py`): Pure text: no git, no repository state, no build. The cases pin the union (`ours` then `theirs`, the `--diff3` base section dropped, a clean file passed through) and each of the four invariant assertions `check_union` makes, because a wrong union breaks *silently* and the assertions are the only thing that sees it. The end-to-end git fixture, including the unsafe-union refusal, lives in `tools/tests/units/test_land.py`.
Now: `tools/tests/units/merge/test_unionresolve.py`.

### Known gaps

None recorded.

### History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The union is **per hunk, not per file** (`unionprose.union_markers`, the one rule shared with `mergebranch.py`): an additive declaration block still appends, but a **comment paragraph both sides rewrote** takes the **superset** side. This module's `union_text` used to append every hunk, which is the same prose-union defect `mergebranch` was fixed for (`7099e70d9`) - it duplicated the paragraph mid-sentence and reintroduced the older side's generated names (2026-09-29: `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h`). `land.py` refuses a header conflict before it reaches this union anyway, but `union_text`/`union_file` are public and a hand `union_file <header>` was a live path to the defect; now a prose hunk with no superset is `blocked` and `land._union_conflicts` refuses it rather than writing the placeholder.
* This module is the land path's resolver, called by `land.py` directly, and it contains **no staging at all**: the caller stages the two scoped paths explicitly (`git add -- <paths>`), never the whole index.

## Module `unionguard`

Refuse a union of a conflicted apply unless every conflict is a disjoint addition (empty diff3 base, no delete/rename); undoes the apply on refusal

### Purpose

Refuse to union-resolve a conflicted branch-apply when the conflict is not a disjoint addition.

### Users

the landing gate (1); imported by `land`

### CLI

None: a package module (its top-level shim was removed in WP6).

### Inputs and outputs

Inputs -> outputs: index stages -> verdict, cleanup.

### Invariants and rules

* `land.py land --branch` lands a worker branch as `git apply -3 <merge-base diff>` and then resolves the conflicted registration paths with a plain **union** (`unionresolve`: ours block, then theirs block). That union is only correct when the two sides touched *disjoint* things - two units each appending a registration at the same anchor. When the two sides changed the *same* existing content it silently writes a tree that cannot build, and the damage is only found later by `ninja`:
* The signal that separates the two is the **base section** of each conflict hunk, which `git merge-file --diff3` makes explicit (this tool recomputes the three-way merge from the index's stage blobs, so it does not depend on the working tree's conflict-marker style):
* base section **empty** -> both sides *inserted* new lines at the same anchor. Disjoint addition. Union.
* base section **non-empty** -> one or both sides changed/removed existing lines. Overlap. Refuse.
* a stage 1 (base) entry with **no stage 2/3** -> that side *deleted* the path. Refuse.
* a rename in either side's diff that touches a conflicted path -> Refuse (the deleted half would be resurrected).
* For every conflicted path the tool prints what each side did (deleted / renamed / modified) and exits **non-zero** with the offending paths named, so the landing can stop *before* it stages a tree that cannot build. A tree whose conflicts are all disjoint additions still unions exactly as before.
* **A refusal also undoes the apply.** `land.py` runs this guard *after* `git apply -3`, which has already merged into the index: UU/AA entries for the conflicts and cleanly-applied hunks staged. Refusing and then only printing "resolve these by hand" left `main` mid-conflict - staged files, conflict stages, and a `ninja` that cannot regenerate `build.ninja` - until somebody cleaned it up by hand. So on refusal the guard restores every path the branch's diff touched (`git checkout HEAD -- <path>` when HEAD has it, a delete when it does not) and runs `git reset` to drop the index stages, printing exactly what it restored and anything still dirty. `--no-cleanup` keeps the old behaviour for a deliberate inspection.
* `land.py` passes the branch and the conflicted paths; with no paths it inspects `git ls-files -u`.
* **`git merge-file` exits with the conflict count** (truncated to 127; negative, i.e. 255, on error). The guard read any exit other than 0/1 as a failure, so a path with **two or more** overlapping hunks raised `RuntimeError` instead of being refused (WP3f, measured on a fixture: old raises, new classifies the overlap). `lib.git.merge_file_diff3` now raises only outside 0..127.

### Lib dependencies

git (`unmerged_stages`, `merge_bytes`, `renames`) and the package module `tools/units/merge/unionprose.py` (`has_base_region`, the one diff3 hunk reader). The implementation is `tools/units/merge/unionguard.py` (`landing/branch.py` imports it; `main(argv)` keeps the old flag set for a caller that wants it).

### Test contract

Tier: fixture (real git repos).
Today's selftest (`tools/units/unionguard_selftest.py`): The guard exists because a plain ours-then-theirs union is only safe for **disjoint additions**; on a delete, a rename, or both sides editing the same region it silently writes a tree that cannot build (measured 2026-09-25: `configure.py` registered both halves of a rename; ten shared g3d headers unioned into "illegal function overloading"). The cases below are the contract: every unsafe case must be refused, and the disjoint union must still pass through and union. A refusal must also undo the `git apply -3` that `land.py` has already run - clean tree, no UU/AA, nothing staged - while `--no-cleanup` keeps the conflicted index for inspection. Every case builds a real git repo and a real unmerged index - no repository state, no build, no mocks.
Now: `tools/tests/units/merge/test_unionguard.py` on `GitFixture`. The disjoint-union case unions with the real `merge.unionresolve.union_file`, not the local mirror the old selftest kept, and a two-hunk case pins the exit-count fix below.

### Known gaps

none (the old landing script is deleted, questions.md 2)

### History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Deletes and renames (measured 2026-09-25, commit `6fceb8f7`).** A `.c -> .cpp` rename made the branch's copy of `configure.py` still carry the old `g3d/g3d_resanmcamera.c` registration while main had already dropped it. The conflict's base side was non-empty; the union wrote ours-then-theirs and kept the stale `.c` line, so configure.py registered both `g3d_resanmcamera.cpp` **and** `g3d_resanmcamera.c`, one of which does not exist. (A rename arrives as delete+add, so the union resurrects the deleted half.)
* **Both sides editing one file (measured 2026-09-25, worker/80093990).** Ten shared `g3d` headers had moved on in main; the union kept both sides' declarations and 12 objects failed to compile ("illegal function overloading").

