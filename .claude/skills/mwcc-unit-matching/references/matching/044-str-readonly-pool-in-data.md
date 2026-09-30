---
id: 44
title: A string pool in `.data` means the build was not `-str readonly`
status: works
problem: The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not pair. It reads as a missing data range.
tags: [flags, data]
applies: [Wii/1.3]
demo: 044-str-readonly-pool-in-data.cpp
reviewed: 2026-09-29
related: [23, 43, 45]
---

# 44. A string pool in `.data` means the build was not `-str readonly`

**Problem.** The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our
strings land in `.rodata` and the section does not pair. It reads as a missing data range (idea 23).

**How it looks.** The target object has the string bytes in `.data` (or `.rodata`) and ours has them in the other
section: one section is "extra" and the other "missing" in the section list, with the same total size (`.rodata`
0x38 vs `.data` 0x20 in the demo's two states).

**Why it happens.** The string flag chooses the section: `-str reuse,pool` emits the pool into `.data`, and adding
`readonly` (`-str reuse,pool,readonly`, also spelled `-readonlystrings`/`-rostr`) moves the same bytes to `.rodata`.
`cflags_runtime` (the runtime library group) carries `readonly`; `cflags_base` does not (`-str reuse`, per-string).

**How to work it.** Read the section of the target's string bytes and choose the `-str` sub-options to reproduce it
(`pool` decides one shared `@stringBase0` object versus per-string objects, idea 43; `readonly` decides `.data`
versus `.rodata`). It is a diagnostic first - it tells you which `-str` the original build used - and a per-library
flag second (non-negotiable 3: evidence in `configure.py`).

**When NOT to apply.** If the string bytes are in `.data` only because a *different* claim owns them (a `.data`
range another unit claims, ideas 23/70), no flag will move them. Do not let a section match tempt a literal
reconstruction that scores lower than the `extern` form: the RSO unit's literal reconstruction scored below the
`extern` one until the pool was written the way the flag needs.

**Example.**

```
-str reuse,pool            # strings in .data behind @stringBase0
-str reuse,pool,readonly   # the same bytes in .rodata
```

**Result.** Measured at the time on `RSO/runtime` with the pool written as string literals: `-str
reuse,pool,readonly` -> .text 0x12F8, .rodata 0xDA, .data 0x38; `-str reuse,pool` -> .text 0x12F8, .data 0x112 (retail's
unit .data is 0x118). No score gain then: the literal reconstruction scored RSOStaticLocateObject 98.2974 /
RSORelocate 98.5217 / RSORelocateSmallDataSection 96.8649 against 99.641030 / 99.478264 / 99.560814 for the
`extern` form, because the emitted data/relocations differed from retail's. `cflags_rso` still inherits `readonly`
from `cflags_runtime` (in `configure.py`), so the retail-section finding has not been applied there.

**Demonstration.** `044-str-readonly-pool-in-data.cpp` (`ideas.py demo-check 44`) reproduces it: `-str reuse,pool`
puts the strings in `.data` behind `@stringBase0`; adding `readonly` moves the same 0x18 B to `.rodata`
(re-measured 2026-09-29, also with `-pool off` present: the section does not change).
