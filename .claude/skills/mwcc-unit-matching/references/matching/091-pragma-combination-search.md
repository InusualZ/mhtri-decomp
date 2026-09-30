---
id: 91
title: Pragma combination search
status: ruled-out
problem: With the level-4 pragma the frame is correct and only a short window differs; a per-function pragma (or a combination) that suppresses the level-4 reassociation while keeping the frame would finish the job.
tags: [pragma, flags]
applies: [Camellia]
demo:
reviewed: 2026-09-29
related: [16, 41, 82, 90, 92]
---

# 91. Pragma combination search

**Problem.** With `#pragma optimization_level 4` on `camellia_setup256` the frame is correct and only a short window
of the absorb chain differs (idea 90). Level 4 switches several optimizer passes on at once, so a pragma that turns
off exactly the one that reassociates the XORs - or a combination of `opt_*` pragmas around level 4 - might keep the
frame and drop the reordering.

**How it looks.** The window is the same 99.81 % row on every variant, or the function gets worse (a frame or size
change) as soon as a pass is turned off.

**What was tried.** Before this pass the hand-kept table said "todo", but the tooling had already run part of it:
level 4 with `opt_common_subs off`, `opt_propagation off`, `opt_lifetimes off`, `opt_dead_code off`, `opt_dead_store
off`, `opt_strength_reduction off`, `opt_loop_invariants off`, `opt_cse off`, `opt_global off`, `opt_space on` and
`scheduling on/off` (`v27`..`v39`, `v45`, `v46` in `tools/flags/variants/Camellia.py`). **Reproduced and extended
2026-09-29** on the current source (baseline `camellia_setup256` 99.81 %, 4860 B):

| pragma set added before the function (on top of the level-4 pragma) | result |
| --- | --- |
| `opt_common_subs off` | 89.76 %, 4804 B |
| `opt_propagation off` | 96.62 % |
| `opt_lifetimes off` | 72.12 %, 4852 B |
| `opt_common_subs off` + `opt_propagation off` | 89.62 %, 4804 B |
| `opt_lifetimes off` + `opt_propagation off` | 71.81 % |
| `opt_common_subs off` + `opt_lifetimes off` | 70.52 %, 4812 B |
| `opt_common_subs`/`opt_propagation`/`opt_lifetimes` `on` (each, and all three) | 99.81 % (no-ops: already on at level 4) |
| `opt_loop_invariants`/`opt_strength_reduction`/`opt_dead_code`/`opt_dead_store on`; `opt_global on` + `opt_cse on`; `opt_space off` | 99.81 % (no-ops) |
| level 4 + `scheduling off` + `opt_common_subs on` | 99.81 % |

Every `off` re-run here is worse (four of them also shrink the function, a sign they remove real work), and every `on` is a
no-op. The other `off` variants of `v32`..`v36`, `v38` and the `scheduling` ones were not re-run; idea 16 records them as
no-ops. Only the whole
function is affected: `optimization_level`/`opt_*` pragmas are read once per function (idea 16), so a combination
cannot be scoped to the window.

**Why it does not work.** It reads as if the passes that create the reassociation (common subexpressions, propagation) are the
same ones that keep the retail frame; removing them costs more than the window is worth.

**What would need to change to be worth re-trying.** A pragma or option not in the explored set (a new compiler build
with extra `opt_*` keywords: enumerate with idea 9 first), or the IR view of `tools/mwcc-debugger` showing that the
reassociation is a different pass than the ones tried. Otherwise the window has to come from source shape (idea 90),
which has plateaued.

**Result.** Ruled out (status changed from `todo` on 2026-09-29 after the reproduction above; the sweep command is
`python tools/flags/tryvar.py -u Camellia/camellia --variants <file>` with `pragma_scope` lines as in the existing
`v27`..`v39`). The unit stays `NonMatching`.

**Evidence.** Numbers are as of the current `src/Camellia/camellia.c` (level-4 pragma before `camellia_setup256`,
defaults restored before `camellia_setup192`). Related: 16 (per-function pragmas), 41 (`optimization_level 1` does
not turn the peephole off - the same lesson: a level is a bundle), 82 (the `-opt` axis), 90 and 92.
