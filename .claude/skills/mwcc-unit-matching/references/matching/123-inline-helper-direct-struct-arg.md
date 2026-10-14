---
id: 123
title: A fill loop and by-value struct temps written in place colour differently from an inline helper fed the call result
status: works
problem: Same instructions, but a loop's pointer/counter/value registers are permuted among r4-r7 and two by-value struct copies sit in swapped stack slots.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [63, 119]
---

# 123. A fill loop and by-value struct temps written in place colour differently from an inline helper fed the call result

**Problem.** Same instructions, but a simple fill loop's pointer, row counter and two colour bytes take other
volatile registers (ours `r7`/`r6`/`r4`/`r5`, target `r4`/`r5`/`r6`/`r7`), and the argument copies of two
by-value `GXColor` conversions sit in swapped stack slots (`0x14` vs `0x10`). Declaration order does not move
any of it (twelve permutations measured identical).

**Why it happens.** Virtual registers are numbered in the order the IR creates them. An inline function's
parameters and locals are created when the call is expanded, so a loop written inside an inline helper gets its
registers in the helper's order, not the caller's. Likewise a named `GXColor` local that receives an inline
conversion's result adds a copy whose stack slot is allocated before the argument copy; passing the call result
straight to the consumer drops it.

**How to work it.** Move the loop into a `static inline` helper that takes the buffer and the value
(`FillFrame(u8* fb, GXColor color)`), and pass inline conversion results directly as arguments
(`FillFrame(fb, RGBToYCbCr(param->bg))`, `DrawFatalText(..., RGBToYCbCr(param->fg), ...)`) instead of through named
locals. Measure the helper and the direct argument one at a time.

**Result.** `OS/OSFatal` `FatalScreenFiber` (0x5E8): 98.595 (in place) -> 99.963 (helper, named local) -> 99.984
(helper, direct argument) -> 100 % (the second colour also passed directly).

**Example.**

```c
static inline void FillFrame(u8* fb, GXColor color) {
    s32 y;
    s32 x;

    for (y = 0; y < 480; y++) {
        for (x = 0; x < 320; x++) {
            fb[0] = color.r;
            fb[1] = color.g;
            fb[2] = color.r;
            fb[3] = color.b;
            fb += 4;
        }
    }
}
...
    FillFrame(frameBuffer, RGBToYCbCr(param->bg));
```
