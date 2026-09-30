---
id: 44
title: A string pool in `.data` means the build was not `-str readonly`
status: works
problem: The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not pair. It reads as a missing data range.
tags: [flags, data]
applies: []
demo: 044-str-readonly-pool-in-data.cpp
---

# 44. A string pool in `.data` means the build was not `-str readonly`

**Problem.** The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not pair. It reads as a missing data range.

**Why try it.** The section is chosen by the string flag: `-str reuse,pool` without `readonly` emits the pool into `.data`. It is a diagnostic first - the RSO unit's literal reconstruction scored lower than the `extern` form until the pool was written the way the flag needs - but it tells you which `-str` the original build used.

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `RSO/runtime` - -str reuse,pool - Retail's pool is in .data, so the original build was not readonly. Measured with the pool written as string literals: `-str reuse,pool,readonly` -> .text 0x12F8, .rodata 0xDA, .data 0x38; `-str reuse,pool` -> .text 0x12F8, .data 0x112 (retail's unit .data is 0x118). NO score gain today: the literal reconstruction scores RSOStaticLocateObject 98.2974 / RSORelocate 98.5217 / RSORelocateSmallDataSection 96.8649 vs 99.641030 / 99.478264 / 99.560814 for the extern form, because a...

**Example.**

```
-str reuse,pool
```

**Demonstration.** `044-str-readonly-pool-in-data.cpp` (`ideas.py demo-check 44`) reproduces it: `-str reuse,pool` puts
the strings in `.data` behind `@stringBase0`; adding `readonly` moves the same 0x18 B to `.rodata`.
