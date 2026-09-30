---
id: 81
title: Compiler-version matrix
status: ruled-out
problem: A unit's residual invites the suspicion "the original used a different compiler release"; sweeping every installed version is flat for the unit tried, so the release is not the lever there.
tags: [flags, measurement]
applies: [Wii/1.3, Camellia]
demo:
reviewed: 2026-09-29
related: [4, 5, 17, 27]
---

# 81. Compiler-version matrix

**Problem.** "The original used a different compiler release" is the first suspicion when a unit stops short, and it
has to be closed once, per unit, before any time goes into flags or source shapes.

**How it looks.** A unit sits at 90-99 % with a residual that looks like scheduling or allocation, and the `.comment`
byte (idea 4) or a library's known toolchain makes a different release plausible.

**What was tried.** Compile the unit's own command line with every installed compiler and compare per function
(`python tools/flags/mwcc_matrix.py -u <unit> Wii/1.0 Wii/1.1 ... GC/3.0a3`; it overwrites the unit object, so
rebuild it afterwards). On `Camellia/camellia`, the unit this row was first run on, the result is flat across the
whole Wii family: re-run 2026-09-29, `camellia_setup256` is **99.81 %** with `Wii/1.0`, `1.1`, `1.3`, `1.5`, `1.6` and
`1.7` (same size 4860 B, same first difference at instruction 962), and clearly worse with the older GC families
(`GC/3.0a3` 92.10 %, `GC/2.7` 82.74 %, first difference at instruction 0). A flat Wii row and a worse GC row is the
signature of "the release is not the lever".

**Why it does not work (here).** The residual is a register-allocation/scheduling difference inside one family of
compilers whose optimizer did not change between the releases we have, so all of them emit the same code.

**What would need to change to be worth re-trying.** Only when the matrix is **not** flat: one family at 100 % on a
function while another is stuck (idea 17: `RSO/runtime` is `GC/3.0a3` while every Wii release emits one extra `lwz`), or a
size mismatch that hundreds of bytes wide no flag explains. Also worth repeating when a new compiler drop is added
under `build/compilers/`.

**Result.** Ruled out for `Camellia` and for the units swept the same way; idea 17 is the counter-example where the
version *is* the lever (a prebuilt SDK unit from another family). The matrix costs about 15 s per unit, so it is
cheap to close, but not to skip.

**Evidence.** Measured 2026-09-29 with `mwcc_matrix.py` on the current source (which carries the per-function
level-4 pragma of idea 16). Related: 4 (ignore `.comment` hints), 5 (one flag at a time), 27 (instruction order names
the `-O` level).
