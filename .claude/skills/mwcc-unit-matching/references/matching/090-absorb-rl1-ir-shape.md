---
id: 90
title: Absorb `dw` / `CAMELLIA_RL1` IR shape
status: ruled-out
problem: With the level-4 pragma the frame is right and only a window in the key-schedule absorb chain differs (register choice plus where `CAMELLIA_RL1` is computed); source rewrites of that chain were the highest-probability lever - they narrow the window but do not close it.
tags: [source-shape, allocator]
applies: [Camellia]
demo:
reviewed: 2026-09-29
related: [16, 18, 22, 35, 89, 91, 92]
---

# 90. Absorb `dw` / `CAMELLIA_RL1` IR shape

**Problem.** The level-4 near-miss (`#pragma optimization_level 4` on `camellia_setup256`, idea 16) changes exactly the
absorb chain: it reassociates a `tl`/`tr` group so it emits `(subr(22) ^ subr(26)) ^ RL1(dw)` where retail keeps
`tr = subr(26) ^ RL1(dw)` in a register (`CAMELLIA_RL1` is the vendor's rotate-left-by-one macro, `dw` a temporary).
It is the only region whose IR (intermediate representation) shape demonstrably moves the result, so rewrites here
(temporaries, ordering, expression form) looked like the highest-probability remaining lever.

**How it looks.** With the frame already correct, the diff is one short window (originally 14 rows, later fewer) in
the middle of a 1215-instruction function: a register choice and the position of one `rlwinm`/`xor`.

**What was tried.** Five source forms *with* the level-4 pragma (split comma, RL1 temp, operand swap, `tl` rewrite, `tl`
temp; `v40`..`v44` in `tools/flags/variants/Camellia.py`): the window is unchanged, so the difference reads as level-4
optimizer behaviour, not source shape. Re-run 2026-09-29 on the current source: the five forms score 99.81 / 99.81 /
99.80 / 99.79 / 99.81 % against a 99.81 % baseline - unchanged or slightly worse. **But those five forms rewrote the
*first* group; the reorder sits in the *fifth*.** Aimed there, rewrites did help (idea 16, "re-locate the window
first"): reusing the rotated value's own variable (`dw = CAMELLIA_RL1(dw);`) took the window from 14 rows to 9, and a
dead duplicate of the group's `tl`/`dw` pair (steering the allocator's web order, idea 35) to 6; about 120
operand-order/statement/temp/anchor forms then bottom out at 6 rows (the `src/Camellia/camellia.c` header records this).
The spelling that gives retail's association (`w0 = dw ^ subr(22); SubkeyR = subr(26) ^ w0`) makes level 4 reassociate the
*fourth* group the same way, for a net 15 rows.

**Why it does not work (to close it).** The reassociation is done by the level-4 pass, so any anchor that suppresses it in one
group leaves it active in another; rewrites move the mismatch between groups without removing it.

**What would need to change to be worth re-trying.** The compiler's own IR: `tools/mwcc-debugger` (PCode after each pass)
would show which pass reassociates and under what condition it declines, which is a much smaller search than
rewriting source blind; or a pragma that switches only that reassociation off (idea 91 tried the combinations and found
none). Do not repeat operand-order rewrites of groups 1-5.

**Result.** Ruled out as a way to *close* the window; it narrowed it (14 -> 6 rows, `camellia_setup256` 99.81 %). The unit stays
`NonMatching`. Note the contradiction with the original text of this entry ("window unchanged"): true for the five forms
listed, not for source rewrites in general.

**Evidence.** Related: 16 (per-function pragmas, where the productive rewrites are recorded), 18, 22 (stop when
retail's colouring is your mirror), 35 (dead copy chain steers web priority), 89 (the perturbation probe), 91 and 92
(the pragma-combination and window-form follow-ups).
