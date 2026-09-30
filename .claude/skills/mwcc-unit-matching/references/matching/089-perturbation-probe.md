---
id: 89
title: Perturbation probe
status: ruled-out
problem: The slot outcome is sensitive to IR shape (level 4 flips the frame). Deliberately perturb one independent statement, watch whether `subL[29]` coalesces, then look for the natural source form that produces the same perturbation.
tags: [source-shape, allocator]
applies: [Camellia]
demo:
---

# 89. Perturbation probe

**Problem.** The slot outcome is sensitive to IR shape (level 4 flips the frame). Deliberately perturb one independent statement, watch whether `subL[29]` coalesces, then look for the natural source form that produces the same perturbation.

**Result.** no - 22 statement/pragma perturbations tried; none flips the frame at level 3. The productive version of this idea turned out to be per-function pragmas (row 16).
