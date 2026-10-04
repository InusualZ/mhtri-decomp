# `mergebranch` - Bring `main` into a held branch and resolve conflicts by class (map rows, splits/configure union, header superset, add/add by rename base), prove, commit

<!-- generated from the module docstring of `tools/units/mergebranch.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Bring `main` into a held lane branch, resolve the conflicts **by class**, prove the result, commit it.

## Users

profiles (`.claude/agents`) (2); CLAUDE.md (1); docs (8)

## CLI

```
python tools/units/mergebranch.py resolve [--branch B] [--dry-run] [--json]
python tools/units/mergebranch.py status
python tools/units/mergebranch.py selftest
```
Subcommands: `resolve`, `status`.
Flags: `--branch`, `--dry-run`, `--json`, `--root`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: git merge state -> resolved commit.

## Invariants and rules

* Run it **inside the lane's own worktree**: it merges `main` into the current branch and commits the resolution there, so the lane keeps working and the orchestrator still lands one branch with `land.py`.
* **Resolution is per class, because each class has exactly one correct rule:**
| conflicted path | rule |
|---|---|
| `config/**/symbols.txt` | main's file, then the branch's own rename pairs re-applied as **exact row replacements** - a symbol map is address-ordered, so a textual union reorders it and renames nothing |
| `src/**` | whichever side already carries the other side's work (the branch's edit there is usually a rename sweep, and main's file may already hold it). If neither does, a **comment-only** delta (identical code after `stylelint.strip`) keeps main's block and records the branch's dropped paragraph; otherwise **refuse** and name what is missing |
| an **add/add** path (no blob in the merge base) | each side diffed against the base of the path *it was renamed from*; with no such base, the **superset** of the two copies - and the side taken is always named, never guessed |
| an unsplit band header (`include/unsplit/*`) | a **union by hunk class** (below), then the **rule-2 address sweep**: a declaration whose address is inside a registered `.text` range belongs to that unit's header, and where it should move is reported |
| anything else | a **union by hunk class**: an additive declaration block unions as before; a comment paragraph both sides rewrote takes the **superset** side (named in the output, with the evidence); a mixed comment-and-code hunk prefers the superset and otherwise keeps the union and warns; a prose hunk with no superset is refused |

## Lib dependencies

git (`Git.merge_bytes`, `renames`, `show`), text (`line_ending`, `atomic_write`), cscan (`strip`), project, repo (`state`), and the package module `tools/units/merge/unionprose.py` (the union rule, `addadd_choice`). The implementation is `tools/units/merge/mergebranch.py`; `tools/units/mergebranch.py` is the entry point and forwards every old name to it.

## Test contract

Tier: fixture (GitFixture).
`tools/tests/units/test_mergebranch.py` (120 checks, the old in-file selftest re-homed; `--selftest` forwards to it). The two real prose conflicts and the pre-fix union live once in `tools/tests/units/merge_fixtures.py`.

## Known gaps

* `newline_of` is `lib.text.line_ending` over the whole blob (it read the first 64 KiB): a file whose first CRLF lies past 64 KiB now restores as CRLF (WP3f; 0 such files in the tree when measured).
* The pre-flight still imports `land.py` (lazily, never fatal) for `rule7_defer_growth`, `band_ownership_warnings` and `units_from_branch`: those move with the gate's rows in WP4.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this exists.** A proposal branch is cut from main and works for hours; by the time it is finished main has moved, so `land.py` refuses ("it will not union a header") and the lane has to merge by hand. Measured 2026-09-27: three lanes, ~1.5 h of hand work, and **content lost twice** - not from a hard conflict but from the two failure modes this tool is built against:
* **`git apply --3way` is the wrong tool.** It refuses with "does not match index" as soon as the working tree differs from the index, and it resolves a *conflict-marked* merge in whatever way it fancies. The correct primitive is `git merge-file -p --diff3 ours base theirs`, which is a real three-way merge and names the three sides explicitly.
* **Never infer "already resolved" from the absence of markers.** After a crash that leaves a file checked out from main, a marker-based guard skips it - and `src/…`, driven that way, silently dropped 157 header lines and a whole unit registration. This tool records the conflicted-path list **before** touching anything and requires every path on it to be *verifiably* represented, by content.
* **A merge in progress is not a dirty tree.** The merge itself stages every auto-merged file and leaves the conflicted paths unmerged, so a `status --porcelain` guard run *before* the merge is detected refuses the re-run this tool's own failure message tells the operator to make (2026-09-29: a resolution that crashed on an add/add path could not be resumed at all). The merge is checked **first**, and a resume is allowed exactly when this tool's state file describes it and nothing *outside* the merge has been touched.
* **An add/add path has no three-way base, and that is a class, not an accident.** A path added on both sides (a re-home on each side, or two files landing on one name) has no blob in the merge base, so `git merge-file` cannot run on it at all. Each side is diffed against the base of the path it was **renamed from** - for a re-home that pre-rename path is the true three-way base - and the side that is the **superset** wins; with no base to derive, the two copies are compared whole. Neither being the superset is a refusal, and whichever side was taken is named in the output and in the commit.
* **The union is per hunk, not per file.** A plain union stays right for an additive declaration block - one side's lines are additions to the other's - and that behaviour is pinned by the selftest. It is wrong for a **comment paragraph both sides rewrote**: `union_markers` appended one side's lines to the other's, which duplicated the prose mid-sentence (`/* … /* …`) and reintroduced the older side's generated names (a merger lane hit this twice in one merge on 2026-09-29 and overrode the tool by hand). A hunk whose lines are all comment prose is therefore resolved to the **superset** side - the copy that already carries every non-blank line the other side changed relative to the shared base, the same rule `addadd_choice` uses - and the side taken, and why, is reported so the operator audits the choice. A hunk that mixes comment and code lines prefers the superset when one is derivable and otherwise warns (naming the file) rather than unioning silently; a prose hunk with no superset is refused (`BLOCKED`), never unioned.
* Every resolution is then checked before the commit: no conflict markers, each conflicted path's branch side present (the **code** for a comment-only resolution), every `fn_XXXXXXXX`/`lbl_XXXXXXXX` the branch sources still *call* still in the map (a name that is part of a unit file's **path**, or one the map cannot resolve at its address, is not a symbol reference - both false positives cost a hand merge on 2026-09-28), and - when the tooling is importable - `land.py`'s own pre-flight rows (rule 7 growth, band ownership, registration) plus the affected units' compile, which is the only check that sees a `NonMatching` unit's object.
