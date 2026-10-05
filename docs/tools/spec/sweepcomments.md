# `sweepcomments` - Comment-only rewriter for the comment sweep: stale paths, narrative history, range fixes, `#if 0`

## Purpose

Rewrites the comment text of `src/**/*.{c,cp,cpp,h,hpp,inc}` (never code, string literals or `#include` lines),
`configure.py`'s `#` comments and, for paths, the hand-written docs, one operation per run so each lands as its own
commit. It also counts what is left: the stale-path markers rule 15 refuses and the narrative markers rule 15 counts as advisory.

## Users

The comment sweep's mechanical stage (stage 1); `--list-stale` counts what rule 15 refuses (the same `lib.comments` data); `--markers`
sizes stage 3's judgement batches.

## CLI

```
python tools/units/sweepcomments.py --paths   [--apply] [--json] [FILE...]  # include/, docs/matching.md N, provenance
python tools/units/sweepcomments.py --history [--apply] [--json] [FILE...]  # narrative boilerplate, inherited headers
python tools/units/sweepcomments.py --fixes   [--apply]                     # header ranges that contradict splits.txt
python tools/units/sweepcomments.py --if0     [--apply] [FILE...]           # `#if 0` blocks
python tools/units/sweepcomments.py --list-stale [--scope src|all] [--top N] [--json]
python tools/units/sweepcomments.py --markers    [--scope src|all] [--top N] [--json]
python tools/units/sweepcomments.py --selftest
```
Without `--apply` a pass reports what it would change and writes nothing. Exit codes: 0 done (or nothing to do), 1 a
`--fixes` entry was refused, 2 could not run or no operation named. `--json`: `{op, applied, files_changed,
lines_removed, lines_rewritten_or_removed, rules, files, refused_protected, dangling_include, unmapped_provenance,
inherited_kept, inherited_removed, refusals}`; the census: `{scope, totals, files}`.

## Inputs and outputs

Reads `git ls-files`, the selected files, `config/RMHE08/splits.txt` and, for `--history`, the factscheck corpora.
Writes only the selected files, byte-exactly (`lib.text.atomic_write`, line endings kept).

## Invariants and rules

* **Comments only.** C comments come from `lib.cscan.spans`; `configure.py`'s from `tokenize`. A comment is modelled
  as lines (head, content, tail); a pass edits the joined content, so a clause may span lines. A line an edit empties
  is dropped, with the blank line that leaves doubled; the comment's opening and closing lines are never dropped.
* **`--paths`**: `include/X` -> `X` (C, `configure.py`) or `src/X` (docs) when the tree has it (`lib.repo.moved_header`:
  movehdr's mapping and exception table); a dangling or glob path is left and listed. `docs/matching.md [row|section] N` -> `playbook N`.
  The `docs/splits/phase4` / `window x` atoms leave their parenthetical. `from <auto|proposal path>` is dropped, a
  sentence left as `Registered.` goes, `Promoted from auto/... (docs/plan.md: ...)` goes; any other `auto/<hex>_...` /
  `proposal/<hex>...` name becomes the unit whose `.text` **starts** at that address, and is left and listed when the
  address sits inside a unit (a fold or a re-cut: which unit it meant is a judgement).
* **`--history`** (C only): a parenthetical loses its narrative atoms (`phase 4`, `round N`, `pilot ...`, `<x> lane`,
  `wave N`, a date, the phase-4 pointers) and goes when none is left; the fixed boilerplate sentences go (`Phase 4: recut
  registered unit; ...`, `PHASE 4 (...).  Recut of X: its functions ...`, a bare `PHASE 4.` label, `They are the next
  pass's work.`); `phase 4 unit, ` -> `unit, `. A dropped atom is a deletion of exactly its characters and separator, so
  the atoms that stay keep their line breaks. A `header inherited from` banner and the block it introduces go together
  only when the block is at most `INHERITED_MAX_LINES` (8) lines and `lib.facts` finds every fact token of the pair
  elsewhere; otherwise both stay and are listed (token survival is not insight survival: a longer block is evidence
  prose for stage 3 to merge). A sentence whose neighbours lean on it (`Merged <date>: a second lane ...` followed by
  `Declarations it needed ...`) is not a template: anything else narrative is stage 3's (`--markers` counts it).
* **`--fixes`**: a hand-curated table (`RANGE_FIXES`) of header ranges that contradict `splits.txt`, each re-checked
  against `splits.txt` before it writes (refused when it no longer agrees); counts and sizes are the map's. Five
  entries at 8cd5ee491: three stale ranges (`ef/ef_util.cpp`, `ef/eft053.cpp`, `quest/arenatask.cpp`) and two headers
  that state a sub-range without saying so (`Network/NetworkSessionManagerPat.cpp`'s key function,
  `Network/network_pat_control.cpp`'s pat-control band). Already applied is a no-op.
* **`--if0`**: deletes `#if 0 ... #endif` (nested conditionals followed) with the full-line block comment directly
  above it that describes it, and the blank line the deletion doubles; a block with an `#else`/`#elif` arm is left.
* **Never removed** (an edit that would is refused and listed): `size: 0x`, `/* +0xNN */`, `untyped:`, `free:`,
  `STOPGAP`, `GUESS`. Never touched: `src/Camellia/` (MPL-1.1), generated markdown (`docs/matching/index.md`, the skill's
  `references/`, the profiles' section 6.5 block), and the record docs in `MD_RECORDS` (lane reports, test records,
  audits, the move's spec), whose paths describe the tree as it was.
* **The `__LINE__` lock**: in a file that expands `__LINE__` (directly, or through a macro whose definition does), the
  lines from the governing `#line` directive (or the file's start) through each use are locked; a comment touching a
  locked line keeps its line count (an emptied line is blanked, a joined line padded). objsame is still the arbiter.
* **The census** (`--list-stale`) counts, in comment text, `include/`, `src/auto`, `auto/<hex>_`, `proposal/`, `.pi/`,
  `docs/splits/phase4` (not the live `homebutton-carried-notes.md`) and the hand-curated retired tool names
  (`attribute.py`, `applysplits`, `dataattach`, `matchinggain`, `promote.py`, `promote_batch`, `herdr`, `applybranch`,
  `union.py`, `mergelane`); a path that exists and the files `docs/splits-program.md`, `docs/tools/retired.md`,
  `docs/splits/phase4/homebutton-carried-notes.md` are whitelisted. The markers, the whitelist and the stale judgement
  are `lib.comments`' (`STALE_MARKERS`, `HISTORY_MARKERS`, `stale_hits`), the data section 6.5 rule 15 refuses from:
  one copy, re-exported here under the old names.
* Idempotent: a second run of any pass changes nothing.

## Lib dependencies

`lib.cli`, `lib.comments` (the census vocabulary and the stale judgement), `lib.cscan`, `lib.facts` (the
inherited-header decision), `lib.git`, `lib.project.splits`, `lib.repo`
(`include_spelling`, `moved_header`), `lib.text`.

## Test contract

Tier: fixture (`tools/tests/units/test_sweepcomments.py`: a temp git repo; every path rule, the dry run writing nothing,
code/strings/`#include` untouched, CRLF kept, Camellia and record docs excluded, `configure.py` and markdown spellings,
idempotence; the history rules, the inherited-header pair kept for a unique fact and removed otherwise, the lock keeping
a line count; the model reproducing every comment shape; the protected refusal; `#if 0`; a fix refused against splits;
the census). Mutation checks: no lock fails 3 checks, no protected guard 1, no facts gate 1, every include resolving 1,
`unit_at` ignoring the start address 1.

## Known gaps

* Sentences are matched by fixed templates; a reworded variant of the boilerplate is left for stage 3.
* A comment sharing a line with code is edited in place but never dropped.
