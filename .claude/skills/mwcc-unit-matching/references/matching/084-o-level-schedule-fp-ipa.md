---
id: 84
title: `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off`
status: ruled-out
problem: Another optimizer level or codegen switch (`-O2`, `-O4`, `-schedule off`, `-fp_contract off`, `-ipa off`) might be the retail setting for a whole unit; each is worth exactly one sweep and is unmistakable when wrong.
tags: [flags, measurement]
applies: [Wii/1.3]
demo:
reviewed: 2026-09-29
related: [5, 8, 27, 39, 40, 82]
---

# 84. `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off`

**Problem.** After the unit's obvious flags are set, another optimizer level or codegen switch might be the retail
setting: `-O2`/`-O3`/`-O4`/`-O4,p` (`,p` adds the Gekko scheduler and 16-byte function alignment), `-schedule
off`, `-fp_contract off` (turn off fused multiply-add contraction) and `-ipa off` (interprocedural analysis).

**How it looks.** If the level is wrong the symptom is unmistakable: sizes off by hundreds of bytes and fused or
hoisted code in every function, not in one. A residual confined to one or two functions is *not* this idea.

**What was tried.** One sweep each, per unit, with the unit's real command line (`python tools/flags/mwcc_matrix.py
-u <unit> --flags-extra "..."`, `tools/flags/optsweep.py`), comparing sizes and first divergences. The hand-kept
table this entry came from recorded the outcome ("no") but neither dated the rows nor named the units, so the
claim below is the recorded one; the cheap re-test is exactly that sweep on a unit that shows the symptom.

**Why it does not work (as a blanket).** If the level is wrong the symptom is unmistakable, and for the units swept
none showed it; a switch that helps one unit is a per-unit finding, not a project setting - which is why the
survivors each earned their own idea instead of a place in `cflags_base`: `-O3` vs `-O4,p` is idea 27's
order-of-instructions tell, `-fp_contract off` became idea 40 (units whose retail code keeps unfused
multiply-adds), and the peephole switch is 39/41.

**What would need to change to be worth re-trying.** A unit whose sweep was never run with its *real* flags (idea 5)
or a new library whose objects come from a different build; and whenever the first divergence names one of these
behaviours (a fused `fmadds` where the target has `fmuls`+`fadds`, a `bl` kept where ours is inlined). In those
cases do it per unit, scoped by `configure.py`'s per-library `cflags_*` (never `cflags_base`), and scope it further
with a pragma when only one function needs it (idea 16).

**Result.** Ruled out as a project-wide guess; `-fp_contract off` and the optimisation level do matter per unit
(40, 27). Cross-check with 82 (the rest of the `-opt` keywords).

**Evidence.** Frame sizes alone are not a success signal (idea 10). The units whose evidence is grouped under idea
40 measured the fp lever independently (e.g. `fn_800898B0` 79.12 -> 100.0).
