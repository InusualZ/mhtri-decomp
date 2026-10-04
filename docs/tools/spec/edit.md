# `edit` - Line-ending-safe text edits: `replace` (CRLF/LF-agnostic, count-asserted), `normalise`, `check` (tree vs index endings)

<!-- generated from the module docstring of `tools/agents/edit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Line-ending-safe text edits: `replace`, `normalise`, `check`.

## Users

profiles (`.claude/agents`) (11); docs (3)

## CLI

```
python tools/agents/edit.py replace FILE --old-file A --new-file B [--count N]
python tools/agents/edit.py replace FILE --old "a\nb" --new "a\tc" [--count N]
python tools/agents/edit.py normalise FILE...
python tools/agents/edit.py check [--fix]
python tools/agents/edit.py check --blob [PATH...] [--rev HEAD] [--repo DIR]
```
Subcommands: `replace`, `normalise`, `check`.
Flags: `--blob`, `--count`, `--fix`, `--new`, `--new-file`, `--old`, `--old-file`, `--repo`, `--rev`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: files -> files, diff.

## Invariants and rules

* `replace` reads FILE as bytes, matches OLD across `\n` **or** `\r\n` (the texts come from files, so there is no shell escaping), converts NEW to the ending of the span it replaces (a mixed file keeps each region's own ending; a span with no line break takes the file's dominant ending), asserts the match count (default exactly 1; 0 or more than N is refused with the line numbers of the matches and nothing is written), writes back the same bytes everywhere else, and prints a unified diff.
* `normalise` rewrites the named text files to LF and reports which ones were CRLF/mixed.
* `check` lists tracked files in the working tree whose on-disk endings differ from the index (`git ls-files --eol`); `--fix` normalises them.
* `replace --old/--new` take the texts inline with C-style escapes (`lib.text.c_unescape`: `\n`, `\t`, `\xHH`, `\NNN`; an unknown escape stays literal) - `escape.py --edit`'s interface on `replace`'s rule. Each pair is exclusive with its `-file` form, and one of each is required. (Git Bash collapses a doubled backslash in an argument, so a literal backslash is safest in a file.)
* `check --blob` is the old `checklf.py`: per PATH (default: every path `git status` names) the working-tree bytes against the blob at `--rev` (`:` the index, or `HEAD`), by line-ending **style** - CRLF or a lone CR present on one side and not the other; a pure content change is not a finding, and a path git does not know is judged against the repository's LF convention. It exists because `git add` normalises a CRLF working tree to an LF blob, after which `git diff` and `git diff --cached` show nothing. Exit 1 on a finding. `--rev HEAD` was broken in checklf (it asked git for `HEADpath`); it reads `HEAD:path` now.

## Lib dependencies

text, git.

## Test contract

Tier: fixture (temp files).
`tools/tests/agents/test_edit.py` (the old `edit_selftest.py` plus `--old/--new` and `check --blob`, every case through the CLI); the per-path `check_path` cases (`test_check_path_cases`) came from the deleted `checklf` test.

## Known gaps

* `tools/units/checklf.py` (the shim) was deleted in WP6.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this exists: slot working copies mix CRLF and LF files while the index is LF (`.gitattributes` has `eol=lf`). A scripted exact-match edit (`str.replace` with `\n`) silently matches nothing on a CRLF file, and an edit that "works" can write LF lines into a CRLF file or the reverse. `escape.py --edit` normalises the *needle* to LF only. This helper never guesses:
