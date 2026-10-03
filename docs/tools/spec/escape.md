# `escape` - Write exact bytes / C-escaped text from one quoted argument without a heredoc; `--edit` is a count-asserted byte replace

<!-- generated from the module docstring of `tools/units/escape.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Write C string literals and source text from the command line **without a heredoc**.

## Users

profiles (`.claude/agents`) (12)

## CLI

```
python tools/units/escape.py --escape "raw text"          # -> the C string literal body
python tools/units/escape.py --bytes "line1\nline2"       # -> the exact bytes
python tools/units/escape.py --write OUT.txt "a\nb"       # write the exact bytes to a file
python tools/units/escape.py --write OUT.txt --append "a"  # append instead of replace
python tools/units/escape.py --edit FILE --old "a\nb" --new "a\tc" [--count N]
python tools/units/escape.py --selftest
```
Flags: `--append`, `--bytes`, `--count`, `--edit`, `--escape`, `--new`, `--old`, `--selftest`, `--write`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: arg -> file.

## Invariants and rules

* This tool replaces it: pass the text as **one quoted argument with C-style escapes** (`\n`, `\t`, `\x41`, ...), and it writes the bytes itself. `--escape` is the other direction - type raw text and get the literal body when you have to paste one. `--edit` is the safe rewrite: it operates on **bytes**, reports the match count, and refuses when the count is not the one you asserted (the "assert the match count" pattern) - so a replacement that matched zero or two places writes nothing instead of corrupting the file.
* The bytes are only ever Python `bytes`; nothing goes through a shell.

## Lib dependencies

text.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_escape.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

`--edit` folds into `edit.py replace`

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this exists (the paste/heredoc hazard).** Three separate incidents in one day: a shell heredoc silently ate a `\n` in a rewritten C string literal and left a broken comment line in a landed file; a lane's report had to be appended three times because bash truncated a long heredoc; and a working-tree edit replaced two real source lines with a stray `L`. A heredoc is a second parser between you and the bytes, and it is not the one that reads them.
