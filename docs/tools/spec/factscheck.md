# `factscheck` - The facts-preserved gate: every fact token a diff removes must survive somewhere

## Purpose

Reads a diff and, for every run of removed lines, collects the fact tokens (addresses, names, sizes) and checks each
one survives: in the new text of the same file, in `configure.py`, `config/RMHE08/splits.txt`,
`config/RMHE08/symbols.txt` or anywhere under `docs/`. A token that survives nowhere is a fact the change deleted.

## Users

The comment sweep (`sweepcomments.py --history` asks it before removing an inherited header block; the sweep's gate
runs it on the whole branch diff). Anyone deleting prose who wants to know what would be lost.

## CLI

```
python tools/units/factscheck.py                         # merge-base(main, HEAD) .. the working tree
python tools/units/factscheck.py --base main --head HEAD # merge-base(main, HEAD) .. HEAD
python tools/units/factscheck.py --explain src/ef        # every token of every removed span, and where it survives
python tools/units/factscheck.py --json                  # lib.findings schema + removed_spans, tokens_checked, failures,
                                                         # dropped, stale_allowances
python tools/units/factscheck.py --allow-drop fn_old_name,0x8009CD64 --reason "superseded by the RANGE line"
python tools/units/factscheck.py --superseded-file build/tmp/net-superseded.txt   # TOKEN[,TOKEN...]: reason per line
python tools/units/factscheck.py --superseded-file a.txt --superseded-file b.txt  # repeatable: every file adds
python tools/units/factscheck.py --no-moved              # the old rule: a token moved to another changed file fails
python tools/units/factscheck.py --selftest
```
Flags: `--base REF` (default `main`; the diff starts at the merge base of REF and the head, as `stylelint --diff`
does), `--head REF` (default: the working tree, tracked and untracked-not-ignored files), `--explain`, `--json`,
`--root`, `PATH...` (limit the diff), `--allow-drop TOKEN[,TOKEN...]` (repeatable) with its mandatory `--reason TEXT`,
`--superseded-file FILE` (repeatable, every file adds; one `TOKEN[,TOKEN...]: reason` per line, blank lines and `#`
comments skipped), `--no-moved` (do not credit a token found in another changed file). Exit codes: 0
every removed fact survives or is dropped on purpose, 1 at least one does not, 2 could not run (an allowance without a
reason, a blank reason, a superseded line that is not `TOKEN: reason`, an unreadable file).

## Inputs and outputs

Reads `git diff --no-renames -U0`, the new side's files (the working tree or `git cat-file --batch` at the head), and
the corpora at the head. `symbols.txt` is read row by row through `lib.project.symbols` (names, addresses, sizes) and
never printed. Writes nothing.

## Invariants and rules

* **A fact token** is a hex value of four or more digits (`0x8009CD64`), a word that carries an underscore or camel
  humps and at least six characters (`known_symbol`, `drawTextRuns`, `802D0DCC_fn_802D0DCC`), or a decimal number of
  three or more digits. Short words, short hex and two-digit numbers are not facts.
* **A span** is one run of consecutive removed lines of one hunk; its line is the old file's.
* **Survives** when the token is in the new text of the unit (the file and its same-stem source/header, `x.cpp` +
  `x.h`), `configure.py`, `splits.txt`, `symbols.txt` or `docs/**` (hex compared by value; a generated stem's eight-digit address is indexed as a value too).
* **Moved** (`moved to <file>`): the token is in the new text of any other file the same diff changed - a declaration
  moved to its owner's header, a fact moved from a `configure.py` comment into the unit's header. The changed-file
  set is the whole diff's (`git diff --name-only`, plus untracked files for a working-tree head), never narrowed by
  the `PATH...` filter. `--no-moved` restores the stricter rule.
* **Derivable**: a generated stem (`fn_802D44F4`, `zz_0123abcd_`, `800CC5B0_fn_...`) whose address any corpus holds.
* **Accepted whole**: a path-qualified name that exists in the tree - as written, below `src/`, or through the retired
  `include/` root (movehdr's mapping).
* **Inert, listed**: a date (`2026-09-25`), a path glob (`include/Network/*.h`), a name glob (`fn_8004Bxxx`), the
  components of a dropped provenance path (`auto/<hex>_...`, `proposal/...`, `src/auto/...`, `docs/splits/phase4`),
  and the narrative words `PHASE`, `phase`, `proposal`, `phase4`.
* **Dropping history on purpose** is an allowance, never a parked copy: `--allow-drop`/`--superseded-file` name the
  tokens (an old name, a past measurement, a map line number) and the reason. An allowed token that survives nowhere
  is printed `<file>:<line>: <token> dropped on purpose (<reason>) [<origin>]` instead of failing; an allowance that no
  otherwise-failing token used - it matches nothing, or its token survives anyway - is printed `stale allowance: ...`
  (reported, not a failure). A token that survives needs no allowance, so none ever applies to it. Matching is
  `lib.facts.classify`'s: a hex by value (`0x8009cd64` is `0x8009CD64`), a word or a number exactly. This is the
  sanctioned alternative to moving history into `docs/` only so a token survives: the allowance and its reason are
  in the command line or the file the lane hands over, and the output carries both.
* The corpora are indexed once per run; a whole-tree diff of a thousand spans runs in about five seconds.

## Lib dependencies

`lib.cli`, `lib.facts` (the tokens, the corpora and the judgement), `lib.findings`, `lib.git` (`merge_base`), `lib.repo`.

## Test contract

Tier: fixture (`tools/tests/units/test_factscheck.py`: a temp git repo with a map, splits, configure.py and a doc; a
removed header paragraph pins which tokens survive (map, docs, the unit's own header, derivable stem, existing path)
and which three fail; the same removal passes once a doc carries the three - the mutation; the merge base excludes main's own
removals; the CLI's exits, JSON and `--explain`; the allowances: an allowed token is dropped with its reason and origin, a hex
matched by value, the stale ones named, the superseded file and its malformed line, a missing or blank reason).
Mutation checks: making every token survive fails 6 checks, dropping the inert classes fails 1, ignoring the sibling
header fails 4; for the allowances (28 checks) ignoring them fails 6, never reporting stale 2, an optional reason 2,
matching a hex by spelling 3, letting a surviving token use an allowance 1, accepting a line without `: reason` 1.
The moved credit and repeatable files (37 checks): indexing no changed file fails 5 (the moved declaration, the
configure.py fact moved into the unit header, the committed head, the PATH filter); keeping only the last
`--superseded-file` fails 1.

## Measured

* The Network sweep commit `6a1c9583a` against its parent: 140 removed spans, 3 290 tokens, 7 spans with 27
  unmatched tokens (exit 1); with a four-line superseded file naming those 27 plus one unused token: 0 failing spans,
  27 dropped on purpose, 1 stale allowance, exit 0; 3.8 s either way.
* The moved credit, each landing against its parent (unmatched tokens, `--no-moved` -> default): `9db8fbe62`
  280 -> 9, `65b341058` 132 -> 4, `7e9ddd8c3` 97 -> 1, `b59bb5041` 15 -> 7; the comment sweeps `faa04ea9c` 117 -> 44,
  `d29784bd7` 158 -> 99, `3f04cda47` 201 -> 147 (their remainder is history dropped on purpose). The landings'
  remainder is mostly the include guard of a deleted header. Run time unchanged (2.5-14 s).

## Known gaps

* A removed token that survives only in an unchanged file of `src/` counts as lost: `src/` is not a corpus, because a
  comment that repeats another unit's fact is not where that fact lives (a file the same diff changed is).
* The include guard of a deleted header (`MHTRI_UNSPLIT_IOS_H`) still needs an allowance.
* Judgement is token-level: a sentence whose words all survive elsewhere passes even when its claim does not.
