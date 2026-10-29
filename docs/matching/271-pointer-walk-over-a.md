---
id: 271
title: A pointer walk over a local's array keeps its struct copy-outs as load/store pairs
status: works
problem: retail copies a local struct out as `lwz;lwz;stw;stw` pairs, ours hoists every load above the stores
tags: [source-shape, allocator]
applies: []
demo: 
---

# 271. A pointer walk over a local's array keeps its struct copy-outs as load/store pairs

**Problem.** A `*out = local;` struct copy reads, in retail, as two-word groups (`lwz r5,+0; lwz r0,+4; stw r0,+4;
stw r5,+0`, then the next pair), while ours loads all ten words into ten registers and then stores them. Only the
register numbers differ in the first pair, so the diff is easy to mistake for a colouring problem.

**Why it happens.** The front end emits the copy as pairs either way (dump `after-initpcode`). The pre-allocation
scheduler (`after-scheduling`) hoists the loads above the stores only when it can prove `*out` does not alias the
local. Once the local's address is taken by a pointer walk (`p = f.arr; *p++ = ...`), the local counts as possibly
aliased and the scheduler keeps every pair in order. An indexed loop (`f.arr[i] = ...`) does not mark it, even
though the unrolled code comes out the same.

**How to work it.** Look for an array member of the struct that the original source probably filled or walked
through a pointer (MSL writes `for (i = sizeof(x), p = x; i; --i) *p++ = v;`) and spell that loop with the pointer.
The loop's own code does not change after unrolling; the copy-outs do.

**Result.** `MSL_C/scanf` `parse_scan_format`: rewriting the `%s` set fill from `f.scan_set[i] = 0xFF` to
`*p++ = 0xFF` took the row from 90.61 % to 99.98 % (+157.7 matched bytes). An inline helper that receives
`f.scan_set` changed nothing, and neither did `__attribute__((aligned(8)))` (-339 B).

**Example.**

```
u8* p;
for (i = sizeof(f.scan_set), p = f.scan_set; i; --i) {
    *p++ = 0xFF;          /* marks f address-taken: later `*format = f` copies stay pairwise */
}
```
