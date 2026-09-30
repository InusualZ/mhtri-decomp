---
id: 5
title: Sweep one flag at a time, against the project's own command line
status: works
problem: Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`, `-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom.
tags: [flags, measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [3, 8, 9, 13, 14]
---

# 5. Sweep one flag at a time, against the project's own command line

**Problem.** Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`,
`-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom.

**How it looks.** Changing three flags together improves the score, and nobody can say which one did it or
whether one of them is harming another function.

**Why it happens.** A hand-written compiler command drifts from what ninja actually runs, so results stop being
reproducible. Overriding one flag in the *real* command line (the unit's `cflags`, from `configure.py`) is the
only change that means anything, and flags interact (a later `-O` replaces an earlier one).

**How to work it.** One flag per run, ordered by how cheap the symptom is to check (idea 3 tells you which
symptom points at which flag), and record the effect on sizes and per-function match. Cumulative overrides build
the final set; a flag that moves nothing is dropped immediately. `--flags-extra` *replaces* flags of the same
family, so `-O3` swaps out `-O4,p` instead of appending after it. Prove the final list separately (idea 14).

**When NOT to apply.** Stop when a variant fixes one symptom but reorders instructions the target does not
reorder (idea 13); and prefer a scoped pragma only if evidence rules out a unit-wide flag (idea 33).

**Result.** A minimal, evidenced flag set that goes into a per-library `cflags_*` override in `configure.py`.

**Example**

```sh
python tools/flags/mwcc_matrix.py -u <unit> --flags-extra "-O3 -inline noauto" 1.3
# then add one flag at a time: -opt nopeephole, -pool off, -use_lmw_stmw off, ...
python tools/flags/optsweep.py -u <unit>       # the whole -opt axis, one keyword at a time
```
