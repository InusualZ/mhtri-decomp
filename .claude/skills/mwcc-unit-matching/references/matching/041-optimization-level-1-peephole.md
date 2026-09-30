---
id: 41
title: `#pragma optimization_level 1` does not turn the peephole off
status: works
problem: A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it: `-O1` resolves to `-opt level=1`, and the level's switch set reads as if it includes the peephole. It compiles, the level moves, and the narrowing still folds.
tags: [pragma, flags]
applies: []
demo:
---

# 41. `#pragma optimization_level 1` does not turn the peephole off

**Problem.** A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it: `-O1` resolves to `-opt level=1`, and the level's switch set reads as if it includes the peephole. It compiles, the level moves, and the narrowing still folds.

**Why try it.** The peephole is a separate switch on the command line and the pragma only sets the level, so the command line's `peephole on` survives. Several workers measured the same non-result independently. On the command line `-O1` produces the same object as `-O3` + the pragma here, so the level is not a proxy for the pass either.

**Result.** Measured independently by 3 worker(s):

* `auto/8009AA78_fn_8009AA78` - -O1 (= -opt level=1) on the command line - same object as -O3 + the pragma for every function in this unit (checked on a scratch helper and on the unit)
* `auto/800E46E8_fn_800E46E8` - `#pragma optimization_level 1` (probe) - not tried - the brief records that it does not turn the peephole off
* `auto/800FCED4_fn_800FCED4` - #pragma optimization_level 1 - fn_800FCED4 94.00, eft002_set 94.09, eft002_set_shell 93.17 - the level does not turn the peephole pass off

**Example.**

```c
#pragma peephole off        /* not `#pragma optimization_level 1` */
```