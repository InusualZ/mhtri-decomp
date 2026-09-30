---
id: 5
title: Sweep one flag at a time, against the project's own command line
status: works
problem: Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`, `-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom.
tags: [flags, measurement]
applies: []
demo:
---

# 5. Sweep one flag at a time, against the project's own command line

**Problem.** Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`,
`-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom.

**Why try it.** A hand-written compiler command drifts from what ninja actually runs, so results stop being
reproducible. Overriding one flag in the real command line is the only change that means anything.

**Result.** One flag per run, ordered by how cheap the symptom is to check, and record the effect on
sizes and per-function match. Cumulative overrides build the final set; a flag that moves nothing is
dropped immediately.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "-O3 -inline noauto" 1.3
# then add one flag at a time: -opt nopeephole, -pool off, -use_lmw_stmw off, ...
```
