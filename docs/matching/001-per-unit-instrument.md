---
id: 1
title: Use a per-unit instrument, not the project-wide check
status: works
problem: The project-level pass/fail signal is useless while any object is `NonMatching`: `ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when the code is wrong. A unit can be at 1 % fuzzy with half its functions at 0 % and still look "complete".
tags: [measurement, tooling]
applies: []
demo:
---

# 1. Use a per-unit instrument, not the project-wide check

**Problem.** The project-level pass/fail signal is useless while any object is `NonMatching`:
`ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when
the code is wrong. A unit can be at 1 % fuzzy with half its functions at 0 % and still look "complete".

**Why try it.** Nothing can be measured, so nothing can be improved. A single-unit, instruction-level diff
is the smallest signal that actually moves when a flag changes.

**Result.** `objdiff-cli diff -p . -u <unit> <symbol>` plus a side-by-side printer becomes the instrument:
per-symbol `match_percent`, size, and the aligned instruction stream are enough to attribute every byte of
a mismatch. Note that the pinned objdiff-cli only emits symbol/instruction data when a symbol argument is
given; without it you get a section-level diff and a silently empty symbol list.

**Example**

```sh
python tools/objdiff/symdiff.py -u <unit> <symbol> 40      # runs objdiff itself, then prints
python tools/flags/frame.py -u <unit>                      # frames only, much faster
```
