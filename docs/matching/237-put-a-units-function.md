---
id: 237
title: Put a unit's function definitions in the target's .text order: extab and extabindex pair again
status: works
problem: Every row scores well, yet flipcheck reports extab/extabindex records pairing other functions and most .text bytes differing - the source defines the functions in another order than retail's .text.
tags: [sections, source-shape]
applies: [Wii/1.3]
demo: 
---

# 237. Put a unit's function definitions in the target's .text order: extab and extabindex pair again

**Problem.** objdiff pairs rows by name, so a unit whose functions are defined in another order than the
target's `.text` scores the same row by row, while `flipcheck.py` reports `extabindex` relocations that name other
functions (`+0x0 ours 0x802AE250, target 0x802AD9CC`), most `extab` bytes differing and the `.data` jump tables
and `.sdata2` pool in another order. The unit can never flip in that state.

**Why it happens.** MWCC emits functions, their unwind records and the jump tables and pooled constants they
create in definition order. A unit written family by family (helpers first, or a second half against another type
view) keeps every function correct and the object's layout wrong.

**How to work it.** List the target's order (`objdump -t build/RMHE08/obj/<unit>.o | grep " F .text" | sort`),
move every type, extern, data and `static inline` item of the source into one prelude in its original relative
order, then emit the functions in the target's order. Keep each function's own pragma state (wrap a function that
had `#pragma opt_common_subs off` in its own pair; a `namespace` view block is reopened around the functions
written inside it), and add one forward declaration for each function now called above its definition. A
throwaway script does this mechanically; check the order with the same `objdump` listing on our object.

**Result.** `stage/stg_w` (131 functions, Wii/1.3 `cflags_main`): the unit's report metric stayed 98.46285 %
(no row moved), `extab` 89 -> 3 differing bytes, `extabindex` 102 -> 12, `.text` bytes differing 26349 -> 21892;
20 forward declarations were needed.

**Example.**

```
/* prelude: types, externs, data, static inline helpers, then */
extern "C" void stage_light_reset();               /* called above its definition */

void* get_stg_w() { ... }                          /* retail's first function */
extern "C" void stage_work_init(void) { ... }      /* retail's second, and so on */
```
