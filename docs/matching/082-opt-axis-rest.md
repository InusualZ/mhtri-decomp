---
id: 82
title: The rest of the `-opt` axis
status: ruled-out
problem: An unknown `-opt` sub-option (a pass keyword such as `cse`, `loop`, `space`) might be what controls instruction fusion or stack allocation; sweeping them all shows most are no-ops for the unit.
tags: [flags, measurement]
applies: [Wii/1.3]
demo:
reviewed: 2026-09-29
related: [8, 9, 10, 16, 84]
---

# 82. The rest of the `-opt` axis

**Problem.** An unknown sub-option of `-opt` might be what controls fusion or stack allocation, and there are a few
dozen keywords (`level=N`, `peephole`, `schedule`, `cse`, `commonsubs`, `deadcode`, `deadstore`, `lifetimes`,
`loop`, `loopinvariants`, `propagation`, `strength`, `space`/`speed`, `all`/`full`, `on`/`off`, each with a `no` form).

**How it looks.** A residual that resembles a missed optimisation (a value kept in a register, a hoisted expression,
a different frame size) and no flag in use that obviously explains it.

**What was tried.** Sweep the compiler's own keyword list once (idea 9 shows how to enumerate it): `python
tools/flags/optsweep.py -u <unit>` compiles the unit once per keyword and prints per-function frames next to the
target's. Re-run 2026-09-29 on `Camellia/camellia`: of 36 keyword rows, 28 leave every frame identical to the target's, and the
8 that change something are the coarse switches (`level=0..2`, `nolifetimes`, `off`, `on`, `all`, `full`), which move
frames to values the target does not have. (That run carries the per-function level-4 pragma of idea 16 in the
source, so the `setup256` frame reads correct in every row; the informative rows are the other functions.)

**Why it does not work.** Beyond the one or two keywords that matter for a unit (`peephole` and the `level`, ideas 27,
39, 41), the rest are no-ops or make the code worse; the frame-size traps that make a wrong flag look right are in
idea 10.

**What would need to change to be worth re-trying.** A frame or size that *no* keyword reproduces is not a flag
problem, so this axis does not need another pass; retry only for a new compiler family whose keyword list differs
(re-run idea 9's enumeration first), or when a unit's first divergence names a pass (then look at that pass's
keyword specifically, and per function via idea 16).

**Result.** Ruled out as a sweep; the two levers that survive are `peephole` (39, 41) and the optimisation level
(27; see also 84 for the level, schedule and fp switches). Note that idea 16 shows a `#pragma` can scope `optimization_level`/`opt_*` to a whole function,
so a keyword that is wrong project-wide can still be right for one function.

**Evidence.** Frames are a weak signal (idea 10): a keyword that reproduces the frame is not thereby correct. Related:
8 (ask the compiler which optimizations are on), 9 (enumerate the option space), 84 (the level / schedule / fp switches).
