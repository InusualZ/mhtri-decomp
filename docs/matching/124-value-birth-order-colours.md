---
id: 124
title: Move a value's birth to recolour it: name a product early, or spell a hoisted invariant in place
status: works
problem: Same instructions, two long-lived values swap registers (r8/r10, f3/f4) and no operand order or declaration order moves them.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [20, 22, 63]
---

# 124. Move a value's birth to recolour it: name a product early, or spell a hoisted invariant in place

**Problem.** The function is instruction-identical to retail but two values that are live at the same time take
each other's registers (`hy` r8 / `ly >> k` r10 where retail has r10 / r8; `hfsq + R` f4 / `ln2_hi` f3 where retail
has f3 / f4). Commuting the operands, reordering the declarations (idea 63) or adding plain temporaries changes
nothing, because none of them changes *when* the compiler creates the value.

**Why it happens.** The likely mechanism (not proven with the mwcc-debugger): the colouring order follows the order the optimizer creates the values' virtual registers, not
the source's declaration order: a value the loop-invariant pass hoists is created late (after the loop body has
been numbered), while a value assigned to a named local before another statement is created early. Moving a value's
creation relative to its rival flips which one is coloured first, and the first one takes the lower register.

**How to work it.** For the pair that swaps, find the value retail colours *later* and make it be born later, or
the one retail colours *earlier* and make it be born earlier:

* a loop-invariant subexpression held in a named local (`lyk = ly >> k;` before the loop) is born at that
  statement; spell it in place inside the loop (`(lx >> k) == (ly >> k)`) and the invariant pass hoists it with a
  late register - the inverse of idea 20;
* a product computed inline in a return expression is born where the expression is evaluated; hoist it into a
  named local *before* the statement it competes with (`khi = dk * ln2_hi; R = s * (hfsq + R); return khi - ...`).

Try both directions per pair with `tryvar.py --variants` (a text `//@@` file) and keep the one that matches; one
pair at a time.

**Result.** `MSL/e_fmod` `__ieee754_fmod` 99.73 -> 100.0 (in-place `ly >> k`); `MSL/e_log` `__ieee754_log`
99.74 -> 100.0 (`khi` named before `R = s * (hfsq + R)`, the in-return spelling and nine operand/temp variants
stayed at 99.74-99.88). Both units trial-flipped with `main.dol` sha1 `bf4850739478caaedfe675949eb7c28595a7fde9`.
It did not move the pairs in `MSL/e_pow`, `KBD/kbd` `KBDSetChannelLeds`, `IPC/ipcProfile` or `SC/SCProductInfo`.

**Example.**

```c
/* retail colours the final hy before ly >> k: let the invariant pass create it */
n = ix - iy;
k = n + 2;
while (n--) {
    hz = hx - hy;
    if (hz == 0 && (lx >> k) == (ly >> k)) { ... }   /* not: lyk = ly >> k; ... == lyk */
}

/* retail colours ln2_hi after hfsq + R: create the product first */
f64 khi = dk * 6.93147180369123816490e-01;
R = s * (hfsq + R);
return khi - ((hfsq - (R + dk * 1.90821492927058770002e-10)) - f);
```
