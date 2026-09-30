---
id: 41
title: `#pragma optimization_level 1` does not turn the peephole off
status: works
problem: A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it: it compiles, the level moves, and the narrowing/index folds are still there.
tags: [pragma, flags]
applies: [Wii/1.3]
demo: 041-optimization-level-1-peephole.cpp
reviewed: 2026-09-29
related: [8, 16, 33, 39]
---

# 41. `#pragma optimization_level 1` does not turn the peephole off

**Problem.** A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it:
`-O1` resolves to `-opt level=1`, and the level's switch set reads as if it includes the peephole. It compiles, the
level moves, and the narrowing still folds. (This is a *ruled-out* shortcut recorded as a working idea because the
finding - "the level is not a proxy" - is what to remember.)

**How it looks.** You wrote `#pragma optimization_level 1` around a function to get the target's unfused
`clrlwi` + `slwi`/`stb`, and the function still comes out as the folded `rlwinm`/raw `stb`; only unrelated code
(scheduling, register allocation) changed.

**Why it happens.** The peephole is a separate switch on the command line (`-opt peephole`/`nopeephole`) and the
pragma only sets the *level*, so the command line's `peephole on` survives. On the command line `-O1` produces the
same object as `-O3` + the pragma here, so the level is not a proxy for the pass either. `mwcceppc -help` lists
the two as independent options (idea 8 shows how to ask the compiler what a flag set resolves to).

**How to work it.** Use the pragma that actually names the pass: `#pragma peephole off` (idea 39), scoped or
whole-file, or `-opt nopeephole` in the library's flags.

```c
#pragma peephole off        /* not `#pragma optimization_level 1` */
```

**When NOT to apply.** Levels do change other things (`optimization_level 0`-`4` move scheduling and register
colouring, idea 16), so a level pragma can still be right for *another* reason; this idea only says it does not
switch the peephole. Whether a *lower* level such as `optimization_level 0` disables the peephole was not
measured (open: compile the demo under level 0).

**Result.** Measured independently by 3 workers, at the time:

* `auto/8009AA78_fn_8009AA78` - -O1 (= -opt level=1) on the command line - same object as -O3 + the pragma for every function in this unit (checked on a scratch helper and on the unit)
* `auto/800E46E8_fn_800E46E8` - not tried - the brief already recorded that it does not turn the peephole off
* `auto/800FCED4_fn_800FCED4` - fn_800FCED4 94.00, eft002_set 94.09, eft002_set_shell 93.17 - the level does not turn the peephole pass off

**Demonstration.** `041-optimization-level-1-peephole.cpp` (`ideas.py demo-check 41`, base flags): under
`#pragma optimization_level 1` the `u8` store is a raw `stb` and the index a single `rlwinm` (both folds intact),
while under `#pragma peephole off` the same code keeps `clrlwi` (+ `stb` / + `slwi`). Verified 2026-09-29 at both
`-O4,p` and `-O3 -inline noauto`.
