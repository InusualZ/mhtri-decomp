---
id: 1
title: Use a per-unit instrument, not the project-wide check
status: works
problem: The project-level pass/fail signal is useless while any object is `NonMatching`: `ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when the code is wrong. A unit can be at 1 % fuzzy with half its functions at 0 % and still look "complete".
tags: [measurement, tooling]
applies: []
demo:
reviewed: 2026-09-29
related: [2, 6, 15]
---

# 1. Use a per-unit instrument, not the project-wide check

**Problem.** The project-level pass/fail signal is useless while any object is `NonMatching`:
`ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when
the code is wrong. A unit can be at 1 % fuzzy with half its functions at 0 % and still look "complete".

**How it looks.** `ok` is red (or green off a stale `main.dol`) whatever you change, and the report shows
`complete_code_percent: 100.0` next to `fuzzy_match_percent: 1.77` for the same unit. Nothing moves when a flag
changes, so nothing can be improved.

**Why it happens.** `complete_code_percent` is a flag we set for `Object(Matching, ...)` units, not a
measurement (see idea 75), and `ok` compares the whole linked DOL, so one wrong unit hides which one.
A single-unit, instruction-level diff is the smallest signal that actually moves when a flag changes.

**How to work it.** Score the unit's own object pair (our `build/RMHE08/src/<Unit>.o` against the split target
`build/RMHE08/obj/<Unit>.o`) with objdiff: per-symbol `match_percent`, size, and the aligned instruction stream
attribute every byte of a mismatch. The pinned `objdiff-cli` only emits symbol and instruction data when a
symbol argument is given; without one you get a section-level diff and a silently empty symbol list.
`tools/objdiff/symdiff.py` wraps that, and `tools/objdiff/unitscore.py <unit>` lists every symbol of a unit from
one report read and refuses a stale report or object (idea 6).

**When NOT to apply.** Do not trust the per-unit percentage as the verdict either: it is positional (idea 2)
and version-dependent (idea 15). The linked DOL hash (`ninja build/RMHE08/ok`) is still the only end-to-end
proof once a unit is flipped to `Matching`.

**Result.** `symdiff.py` plus the first divergence (idea 2) is the instrument used for every later idea.

**Example**

```sh
python tools/objdiff/symdiff.py -u <unit> <symbol> 40      # runs objdiff itself, then prints
python tools/objdiff/symdiff.py -u <unit>                  # every symbol of the unit, worst first
python tools/objdiff/unitscore.py <unit>                   # same from report.json, with a freshness verdict
python tools/flags/frame.py -u <unit>                      # frame sizes only, much faster
```

**Evidence.** The 1.77 % / 100.0 pairing was observed on the Camellia unit early in the project (a 1.49 %
variant was seen later); numbers are as measured at the time.
