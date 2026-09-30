---
id: 90
title: Absorb `dw` / `CAMELLIA_RL1` IR shape
status: ruled-out
problem: The level-4 near-miss changes exactly this chain, and it is the only region whose IR shape demonstrably moves the frame. Rewrites here (temps, ordering, expression form) are the highest-probability remaining lever.
tags: [source-shape, allocator]
applies: [Camellia]
demo:
---

# 90. Absorb `dw` / `CAMELLIA_RL1` IR shape

**Problem.** The level-4 near-miss changes exactly this chain, and it is the only region whose IR shape demonstrably moves the frame. Rewrites here (temps, ordering, expression form) are the highest-probability remaining lever.

**Result.** no - five source forms tried *with* the level-4 pragma (split comma, RL1 temp, operand swap, `tl` rewrite, `tl` temp): the 12-row window is unchanged, so it is level-4 optimizer behaviour, not source shape.
