# `edit` - Line-ending-safe text edits: `replace` (CRLF/LF-agnostic, count-asserted), `normalise`, `check` (tree vs index endings)

<!-- generated from the module docstring of `tools/agents/edit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Line-ending-safe text edits: `replace`, `normalise`, `check`.

## Users

profiles (`.claude/agents`) (11); docs (3)

## CLI

```
python tools/agents/edit.py replace FILE --old-file A --new-file B [--count N]
python tools/agents/edit.py normalise FILE...
python tools/agents/edit.py check [--fix]
```
Subcommands: `replace`, `normalise`, `check`.
Flags: `--count`, `--fix`, `--new-file`, `--old-file`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: files -> files, diff.

## Invariants and rules

* `replace` reads FILE as bytes, matches OLD across `\n` **or** `\r\n` (the texts come from files, so there is no shell escaping), converts NEW to the ending of the span it replaces (a mixed file keeps each region's own ending; a span with no line break takes the file's dominant ending), asserts the match count (default exactly 1; 0 or more than N is refused with the line numbers of the matches and nothing is written), writes back the same bytes everywhere else, and prints a unified diff.
* `normalise` rewrites the named text files to LF and reports which ones were CRLF/mixed.
* `check` lists tracked files in the working tree whose on-disk endings differ from the index (`git ls-files --eol`); `--fix` normalises them.

## Lib dependencies

text, git.

## Test contract

Tier: fixture (temp files).
Today's selftest (`tools/agents/edit_selftest.py`): python tools/agents/edit_selftest.py
Target: `tools/tests/agents/test_edit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

absorbs `checklf` and `escape --edit`

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this exists: slot working copies mix CRLF and LF files while the index is LF (`.gitattributes` has `eol=lf`). A scripted exact-match edit (`str.replace` with `\n`) silently matches nothing on a CRLF file, and an edit that "works" can write LF lines into a CRLF file or the reverse. `escape.py --edit` normalises the *needle* to LF only. This helper never guesses:
