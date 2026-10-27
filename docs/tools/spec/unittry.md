# `unittry` - Try source edits on a unit's real files; keep each only when the unit's score rises and no row drops

## Purpose

Applies one or more source edits to a unit's source (and its own headers), measures the unit after each candidate and
keeps the candidate only when the unit's matched bytes rise and no function row falls; anything else is undone byte for
byte. It is the keep-or-restore loop the lanes wrote by hand (`trial.py`, `tryv.py`, `perm_sheet.py`), with the counted
replace, the guarded site sweep and the statement permutation they asked for.

## Users

Unit lanes (`decompiler`, `fixer`) through `python .claude/skills/mwcc-unit-matching/scripts/mt.py tryvar`; the
`mwcc-unit-matching` skill.

Its sibling `tools/flags/tryvar.py` (`mt.py variants`/`permdecl`/`hill`, `spec/tryvar.md`) never writes the real
source: it compiles each variant as a probe beside it and ranks them. Use that one to rank alternatives of a `.cpp`;
use this one when the edit is cumulative (several sites, several trials in a row), spans a header, or must be measured
from the real files.

## CLI

```
python tools/units/unittry.py <unit> --spec FILE [--only NAME,...]           # trials from a block or JSON file
python tools/units/unittry.py <unit> --replace OLD NEW [--count N | --all] [--file P]
python tools/units/unittry.py <unit> --replace-files A B [--count N | --all] [--file P]   # OLD/NEW read from files
python tools/units/unittry.py <unit> --sites REGEX REPL [--literal] [--guard return-break] [--function FN] [--file P]
python tools/units/unittry.py <unit> --permute FN [--block-file F | --lines A-B] [--split-init] [--hill]
                             [--max-perms N] [--max-evals N] [--seed N] [--file P]
python .claude/skills/mwcc-unit-matching/scripts/mt.py tryvar <unit> ...    # the same
```

Run options: `--restore` (put every file back at the end: a measuring run whose table says what would be kept),
`--sdata2` (compare `.sdata2` with the target's bytes; refuse a candidate that breaks a target-identical one),
`--shared-header` (allow a header other units include - they are **not** re-measured), `--rename-map F` (score against
the target with pending map edits, `lib.pendingmap`), `--top N` (ranking rows), `--json`.

Exit codes: 0 the run finished (whether or not anything was kept), 1 the unit as it stands does not measure, 2 a usage
or spec error (`SystemExit` with the reason), 130 interrupted (the files are restored first).

`--json`: progress goes to stderr; stdout is `{unit, target, baseline: {fuzzy, matched, sdata2}, final: {...}, rows:
[{trial, name, verdict, why, delta, fuzzy, up, down, sdata2, seconds}], kept, written, interrupted}`.

## The spec file

**Block format** (any extension but `.json`): every body is verbatim source text - no escaping, no quoting, no heredoc;
a body loses its one trailing newline.

```
//@@ trial cast-the-index                  <- a named trial: its edits are one candidate
//@@ replace count=2                       <- `count=N`: OLD must occur exactly N times; `all`: every occurrence
    idx = (s16)row->index;
//@@ with
    idx = row->index;
//@@ replace file=src/menu/menu_result.h   <- an edit of the unit's own header in the same candidate
    s16 index;
//@@ with
    u16 index;
//@@ trial return-to-break
//@@ sites guard=return-break function=Lobby::step   <- one candidate per match, the improving ones kept
return;
//@@ with
break;
//@@ trial decls
//@@ permute function=Demo::step split-init max=120  <- the block, copied as it stands in the function
    int a = get();
    Demo::Kind* k = lookup(a);
    int b;
```

An edit block before any `//@@ trial` line is a trial of its own (`replace-1`, `sites-2`, ...). A `sites` or `permute`
block is alone in its trial. `sites` options: `guard=return-break`, `function=FN`, `literal` (pattern and replacement
are plain text), `file=P`; the pattern is a Python regex (`re.M`) on the body's first line, the replacement a `re`
template (`\1`). `permute` options: `function=FN` (required), `lines=A-B`, `split-init`, `hill`, `max=N`,
`max-evals=N`, `seed=N`, `file=P`; an empty body permutes the function's leading declarations.

**JSON** (`.json`): a list of trials, or `{"trials": [...]}`; each is `{"name", "replace": [{"old", "new", "count",
"file"}] }` (a `[old, new]` pair is accepted; `count` is a number or `"all"`), `{"name", "sites": {"regex", "repl",
"guard", "function", "literal", "file"}}` or `{"name", "permute": {"function", "block", "lines", "split_init", "hill",
"max", "max_evals", "seed", "file"}}`.

## Inputs and outputs

Reads the unit's source and its include closure (`lib.report.source_closure`), the unit's compile command and target.
Writes only the files the trials name (the source, its own headers) and `build/tmp/unittry/<unit>/` (`obj/` the
compiled object, `report/` the one-unit objdiff project and its `unitutil_report.json`, `target.pending.o` with
`--rename-map`). `build/RMHE08/src/<unit>.o` and `build/RMHE08/report.json` are neither read nor written.

## Invariants and rules

* **The verdict.** A candidate is kept when it compiled, no row's score fell (a row the candidate lost counts as a
  fall) and the unit's matched bytes (`sum(score x target size)`) rose by more than 1e-6; with `--sdata2`, a candidate
  that turns a target-identical `.sdata2` into a different one is refused too. Everything else is reverted. The
  comparison is against the accepted state the candidate was tried on, so trials accumulate in file order.
* **Restored byte for byte.** The run snapshots each file's bytes and mtime on first touch. After a losing candidate,
  at the end, after an error and after Ctrl-C (`Runner.execute`'s `finally`), the accepted state is written back; a
  file whose accepted text is its original text gets its original bytes and mtime, so ninja sees nothing changed. A
  write keeps the file's CRLF/LF endings (`lib.text.atomic_write`). `--restore` puts the original back even over kept
  edits.
* **A fresh score, from the tool's own path.** The compile command is the scorer's own resolution
  (`lib.units.unit_tokens` + `reconcile_flags`, the one `measure.py` uses) with `-o` pointed at the scratch `obj/`;
  `lib.units.run_tokens` deletes the object first and the score refuses an object older than the compile. The score
  is `lib.report.score` (the official `report generate` metric `measure.py` and `unitscore --measure` read) on a
  one-unit project under the scratch `report/`, whose output is deleted before each run - never a `report.json` whose
  mtime has to be trusted.
* **Counts are asserted, never guessed.** A `replace` with the default count of one is refused when OLD occurs twice
  (the reason names the lines); `count=N` replaces exactly N occurrences, `all` every one. A refused trial is a `skip`
  row and nothing is compiled.
* **Sites one at a time.** A `sites` sweep compiles each match on its own, from the first, against the accepted
  state; a kept site stays and the sweep continues after its replacement. A pattern that matches the empty string is
  refused.
* **The `return-break` guard** skips (as `guarded`, uncompiled) every site it cannot prove equivalent: the site's
  innermost `switch`/loop must be a switch, nothing enclosing that switch may be a loop or another switch, the site may
  not be the body of a brace-less loop, and only closing braces or a `return;` may follow the switch to the end of the
  function. It is conservative: a guarded site may be equivalent, an unguarded one is.
* **Permutations.** The block is the `block` text (exact lines once in the function, else the same lines at any
  indent once), `lines=A-B`, or the leading run of up to six declarations. It is cut into statements (a statement runs
  to the line where the bracket depth is zero and the code ends in `;` or `}`; a blank or comment line goes with the
  next statement; a block holding `else` is refused). A statement stays after the earlier ones whose declared name it
  mentions (`lib.declperm.orders` with those dependencies). Every order is compiled against the accepted state and the
  best winner (most matched bytes) is kept; the run stops early when every row is at 100 %. More than `max` orders are
  a fixed-seed sample. `hill` climbs instead: one statement moved per step (`lib.declperm.neighbours`), the first
  strictly better order kept, until no neighbour wins or `max-evals` compiles.
* **`split-init`** turns each `T x = f();` of the block into `T x;` (in the permuted declaration part) and `x = f();`
  (after every declaration, in the original order with the block's other statements), and tries every order of the
  declarations, the identity included. `const`, reference, array, `static` and brace-initialised declarations are not
  split; they stay with the statements.
* **Functions** are found with `lib.cscan.function_declarations`, so a multi-line signature and a `::`-qualified name
  are read whole; `FN` may be the plain name, the qualified name or a mangled report name (`step__4DemoFii` -> `step`).
  A name two definitions share is refused with both qualified names.
* **Which files may be written.** The unit's source, and a header in its include closure that no other source file
  under `src/` reaches (an "own header"). A shared header is refused unless `--shared-header`, and then the other
  units are still not measured. Every `file=` is resolved before the first write.
* **Progress.** One line per candidate as it finishes (`[done/planned elapsed eta] verdict name delta up down`), then
  a ranking by matched-byte delta and a summary line; a guarded or skipped site prints its reason.

## Lib dependencies

`lib.units` (unit, compile command, `run_tokens`), `lib.report` (the score, the include closure), `lib.declperm`
(orders, neighbours, `declared`), `lib.cscan` (function bodies, comment masking), `lib.text` (atomic writes),
`lib.pendingmap`, `lib.repo`; `lib.text.marker_blocks` (the `//@@` block reader `tools/flags/tryvar.py` reads too).

## Test contract

Tier: fixture, `tools/tests/units/test_unittry.py`. Pins the block and JSON parsers (trial grouping, counts, options,
refusals); function lookup by plain, qualified and mangled name with a multi-line signature; the guard on the three
sites of the fixture (safe, loop inside the case, code after the switch) and a switch inside a loop; statement cutting
and the split initialiser; a losing replace, a failed compile and a net gain that drops one row reverted with the CRLF
bytes and the mtime unchanged;
a twice-occurring OLD refused by the default count and accepted with `count=2`; the guarded sweep compiling only the
safe site; the best split order kept; Ctrl-C mid-candidate leaving the kept edit and reverting the in-flight one;
`--restore`; an own header allowed, a shared one refused, one trial spanning source and header; the real `Measurer`
compiling a stub compiler into the scratch directory and never writing `build/RMHE08/src`. Each of these mutants fails
it: no mtime restore (1 check), the guard off (2), row drops ignored (4), a counted replace at the first occurrence
only (2), no restore on interrupt (2), the split identity order left out (3). The `mt.py tryvar` route is pinned in
`tools/tests/flags/test_mt_permdecl.py` (smoke).

## Known gaps

* Rows are paired by name: an edit that renames a function reads as a dropped row (and is refused).
* Each compile waits about a second (`lib.units.run_tokens` keeps object mtimes in distinct seconds); a 120-order
  permutation costs about four minutes on a 2-second unit.
* The guard knows `return;` -> `break;` only; other rewrites have no semantic check and rely on the score.
* Statement permutation checks declared-name dependencies only; reordering two plain statements is the caller's
  semantic call.
* Shared headers are not re-measured in their other units; the land gate's report is the check for those.
