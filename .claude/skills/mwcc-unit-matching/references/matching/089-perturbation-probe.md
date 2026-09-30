---
id: 89
title: Perturbation probe
status: ruled-out
problem: The stack-slot outcome of `camellia_setup256` is sensitive to IR shape (level 4 flips the frame); deliberately perturbing one independent statement and watching whether `subL[29]` coalesces was meant to lead to a natural source form doing the same.
tags: [source-shape, allocator]
applies: [Camellia]
demo:
reviewed: 2026-09-29
related: [16, 18, 22, 35, 88, 90]
---

# 89. Perturbation probe

**Problem.** The slot outcome is sensitive to IR (intermediate representation) shape: at `-O3` the `subL[29]` value's
live range splits into two stack slots and pushes the frame from `-0x1d0` to `-0x1e0`, while `#pragma
optimization_level 4` keeps one slot and reproduces the retail frame. The idea: deliberately perturb one independent
statement, watch whether the value coalesces into one slot, then look for the *natural* source form that produces the
same perturbation.

**How it looks.** The whole unit is byte-identical except the frame size and the `r1` offsets of the locals (a stack
slot difference, found with `tools/objdiff/slotmap.py`), and no flag but the optimisation level moves it.

**What was tried.** Twenty-two statement/pragma perturbations (`tools/flags/variants/Camellia.py`, `v1`..`v24` for the
statement rewrites) at level 3: none flips the frame; dead-copy and duplicate-statement perturbations change the
slot *permutation* but never the frame, and neither do 14 operand/statement perturbations around the write and the
reads (recorded in the `src/Camellia/camellia.c` header). Re-run 2026-09-29 on the current source (which already carries
the level-4 pragma): `v1`, `v5`, `v15`, `v19`, `v22` leave `camellia_setup256` at the baseline 99.81 %; `v21_xor_dw`
lowers it to 96.76 %.

**Why it does not work.** Allocation here is a greedy slot-colouring tie-break driven by the whole function's live
ranges; local statement changes move *which* slot is which, not how many are needed, so the frame never changes.

**What would need to change to be worth re-trying.** A view of the allocator's decision instead of blind
perturbation: the compiler's own IR (`tools/mwcc-debugger`, the PCode after each pass and the register allocator's
choices) would show *which web* takes the second slot and what would have to change to merge it - idea 35 is the
worked case of a dead copy chain steering web priority. Without that, more perturbations are the same experiment.

**Result.** Ruled out at level 3. The productive version of this idea turned out to be per-function pragmas (idea 16),
which reach the retail frame directly.

**Evidence.** Related: 16 (per-function pragmas), 18 and 35 (declaration order and dead copies steering the allocator), 22
(stop when retail's colouring is your mirror), 88 and 90 (the neighbouring searches on the same unit).
