# `guard` - Pre-commit guard logic: warn on `core.autocrlf`, normalise a CRLF text blob in the index, refuse a binary with CR

<!-- generated from the module docstring of `tools/git/guard.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The pre-commit guards, as logic plus a CLI the hook calls.

## Users

no caller in the tracked tree

## CLI

```
guard.py autocrlf    # warn (never refuse) when core.autocrlf=true, which silently defeats eol=lf
guard.py eol         # EOL case: normalise a textish staged blob's CRs and re-stage it; refuse a binary
guard.py config      # refuse a staged config.yml change outside block_relocations / add_relocations
```
Flags: `--root`.
`config` exits 0 when the index's `config/RMHE08/config.yml` equals HEAD's, differs only in comment/blank lines, or
differs only in the relocation-analysis keys (it prints `guard: allowing ...` naming them); 1 otherwise, with
`guard: refusing ...` naming the frozen keys.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: index (and HEAD, for `config`) -> index.

## Invariants and rules

* **The hook's path refusals.** `orig/**`, `build/**` and `config/RMHE08/build.sha1` are refused on any commit -
  added, modified, deleted or renamed (`--no-renames --diff-filter=ACMD`). Until 2026-10-03 the regex anchored every
  alternative with `$`, so only the literal paths `orig/` and `build/` matched and a forced-added `orig/**` or
  `build/**` file (or a deletion) went through; the selftest pins all four.
* **`config.yml` has one door (owner ruling 2026-10-03).** Every top-level key is the DOL's ground truth (object, hash,
  selfile, map paths, `mw_comment_version`, ...) except dtk's relocation hints `block_relocations` and
  `add_relocations`, which change what the analyzer reads and never what the DOL is. `config` compares the staged copy
  with HEAD's key by key; a change confined to those keys and to full-line comments/blank lines passes, anything else
  - a frozen key changed, added, removed or renamed, a hint riding a frozen change, the file created or deleted, a line
  the minimal reader cannot place, a repeated key - refuses. A trailing comment on a value line counts as content.
* **One implementation.** The decision is `tools.lib.repo.config_change` (beside `ground_truth`), stdlib only; the hook
  reaches it through `guard.py config`, `prepcommit.classify` calls it directly (the layering rule forbids a
  tool->tool import). When `guard.py` is missing the hook refuses a staged `config.yml` rather than failing open.

* The two decisions are plain functions - `eol_case`, `autocrlf_warning` - so a test can call them directly (with the staged text or the staged paths handed in) without a shell and without first building a git command; the CLI is the thin layer that reads the staged paths from git and performs the fix.
* **The index blob, not the worktree.** A commit carries the *index*, so the EOL case reads the staged blob (`git cat-file -p :<path>`) rather than the file on disk: a CRLF blob in the index is what would land, whatever the worktree looks like.
* **Why this lives here and not in `prepcommit.py`.** `prepcommit.py` is a staging/commit-message CLI: it walks `git status`, classifies paths into stage/refuse and writes a message. The guards are a different concern and have to run for *any* commit, including a plain `git commit` that never calls prepcommit - so they live in a small importable module with no side effects on import, which both the hook and `guard_selftest.py` can call.

## Lib dependencies

git, text, repo (`config_change`, `CONFIG_PATH`).

## Test contract

Tier: fixture (temp repo).
Today's selftest (`tools/git/guard_selftest.py`): This is the one gate whose failure mode is silent and total - a rewritten `build.sha1` would make every later `ninja build/RMHE08/ok` meaningless - so it is tested rather than trusted.
It pins `config_change` on 8 allowed and 11 refused pairs (each also through `prepcommit.classify`), and 14 commit
attempts through the real hook in a throwaway repo (allowed: a `block_relocations` block, a comment edit, an ordinary
file; refused: `object`, `selfile`, a deleted key, a renamed key, a mixed hint + frozen change, `config.yml` deleted,
`build.sha1` touched or deleted, an allowed hint beside `build.sha1`, a file under `orig/`, a file under `build/`).
Against the previous hook 6 of those fail (both allowed cases, both deletions, `orig/**`, `build/**`).
Target: `tools/tests/git/test_guard.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* `guard_selftest.py` is still the legacy shape (it reads the live hook and the ground truth): splitting it into a fixture module and a smoke module is open. `index_blob`/`repo_root`/`staged_paths` are `lib.git` calls (`cat_index`, `toplevel`, `out`) since WP3f.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* `tools/git/hooks/pre-commit` used to be a shell script that could only *refuse* a bad commit. This module is the logic behind it now: it can say what a staged path needs, and its CLI does the fix.
