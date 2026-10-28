---
id: 270
title: Under global_optimizer off a block-scope local forces a frame pointer: declare every local at function scope
status: works
problem: A function scoped under `#pragma global_optimizer off` gains `mr r31,r1` after the register saves and a `mr r10,r31` / `lwz r10,0(r1)` epilogue, and every stack local is addressed through r31; retail has the plain `addi r1,r1,N` frame.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 270-under-global-optimizer-off.cpp
related: [120, 148]
---

# 270. Under global_optimizer off a block-scope local forces a frame pointer: declare every local at function scope

**Problem.** Ideas 120 and 148 scope `#pragma global_optimizer off` to one function to keep retail's compare-tree
switches and unspeculated `if`/`else` values, and report that it "costs a frame pointer": `mr r31,r1` after the
saves, stack locals through r31 (`stb r0,0x31(r31)`), and `mr r10,r31; ...; lwz r10,0(r1); lwz r0,4(r10); mr r1,r10`
at the end. That looks like a property of the pragma; it is a property of the source.

**Why it happens.** With the global optimizer off, a declaration inside a nested block (`case 5: { T* p; ... }`,
`if (k) { u32 level = ...; }`) makes the backend keep a frame pointer for the whole function. The same body with
the local declared at function scope has none (demo: `blockLocal` vs `functionLocal`). With the optimizer on, the
block scope makes no difference.

**How to work it.** Hoist every block-scope local of the function to its leading declarations (keep the name, assign
it where the block declared it), then re-measure; the frame pointer, its epilogue and the r31-relative addressing
go at once. Then the usual `global_optimizer off` shapes apply: a value retail joins (`li rX,A; b join; li rX,B;
join: stb`) is a named local assigned on both arms (a `?:` is still speculated, two stores stay two stores), and an
interrupt-guarded u8 read through an inline returns `u32` (a u8 return is zero-extended again at each compare
when the optimizer is off). Declaration order is still a colouring input (idea 242).

**Result.** `WUD/wud` (Wii/1.3, -O4,p): `WUDiSyncFiber` 93.44 -> 99.63 % (2788 B, size equal). The pragma alone
measured -75 B on the row; the both-arms locals and u32 inlines brought it to 98.15 %, hoisting the two block-scope
locals (`case 5`'s device pointer, `case 0x19`'s interrupt level) +50 B, and the declaration permute and the
per-case `state` local the rest (unit header SHAPES line).

**Example.**

```c
#pragma push
#pragma global_optimizer off
void fiber(void)
{
    u32 level;                  /* was `case 0x19: { u32 level = OSDisableInterrupts(); ... }` */
    ...
    case 0x19:
        level = OSDisableInterrupts();
        ...
}
#pragma pop
```
