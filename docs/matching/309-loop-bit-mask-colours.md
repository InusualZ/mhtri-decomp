---
id: 309
title: A loop bit mask colours before its strength-reduced walk: make the loop's mask the second web of a helper local
status: works
problem: A bit-walking loop over an indexed array (`for (bits = ...; bits; i++, bits >>= 1) if (bits & 1) use(a[i])`) matches every opcode but the mask and the strength-reduced walk pointer are swapped (ours mask r7 / walk r8, retail r8 / r7), or a pointer spelling gives `addi rP,rB,0xB8` + `lwz 0(rP)` where retail has `mr rP,rB` + `lwz 0xB8(rP)`
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [243, 244, 292]
---

# 309. A loop bit mask colours before its strength-reduced walk: make the loop's mask the second web of a helper local

**Problem.** A loop that walks a dirty-bit mask and an array in step - `for (bits = (dirty >> 16) & 0xFF; bits != 0;
i++, bits >>= 1) if (bits & 1) write(addr, gx->table[i])` - is byte-equivalent except that the mask and the walk are
swapped (ours `extrwi r7` / `mr r8,r31`, retail `extrwi r8` / `mr r7,r31`); the same loop written with an explicit
pointer (`p = gx->table; p++`) gets the registers but not the addressing (`addi r5,r31,0xB8` + `lwz 0(r5)` where retail
has `mr r5,r31` + `lwz 0xB8(r5)`, the strength-reduced form). Declaration orders, helper-vs-body spellings, `int`
counters and scoped optimisation pragmas leave the row where it is (~60 variants on `RVLGX/GXGeometry`).

**Why it happens.** The order the colouring runs in is fixed by how the frontend numbers its webs (read from the
`regalloc-gpr-pass-1-all.txt` of `tools/mwcc-debugger` across six dumps of one function; `regsim verify` reproduces
every one): (1) expression temporaries, then in descending `@` number (2) the strength-reduced walks the optimiser
creates (`mr r38,r37` / `addi r38,r38,4` in the first PCode; several loops are numbered in *reverse* textual order),
(3) the global-load webs, (4) the webs a *later def* of a variable starts (also reverse textual order), (5) the named
function locals in reverse declaration order. An inlined helper's parameter is the first `@` of its group and its
locals follow (reverse declaration order), all created before the walks. A variable's *first* web keeps the
variable's own number; every later def starts a new web in tier 4. So in a helper the mask local is tier 2-adjacent
and colours *before* the walk, taking r7, and the walk falls to r8; in the body the mask is a named local and colours
after the walk, but so do the address counter and the `addr + 0x10` local, so the walk takes r5 instead of r7. Retail's
registers say: address locals before the walk (helper locals), the mask after it (a tier-4 web).

**How to work it.** Keep the loop in a `static inline` helper so its address counters are helper locals, index the
array (`gx->table[i]`, never a pointer walk) so the walk is the optimiser's, and make the loop's mask the **second**
web of a helper local whose first web is a real stored value - here the count the same helper writes just before the
loop: `bits = gx->genMode.f.numTexGens; XF_WRITE(0x103F, bits); bits = (dirty >> 16) & 0xFF; for (...)`. A first
"web" that folds into a test (`bits = dirty & 0x02000000; if (bits)`) does not count: the compare absorbs it and the
loop def stays the local's first web. Shifting the parameter instead (`dirty = (dirty >> 16) & 0xFF`) makes the loop
web the parameter's first web, the earliest of the group (98.97 % vs 99.21 %). Check the result with `regsim want`.

**Result.** `RVLGX/GXGeometry` (Wii/1.3, -O4,p): `__GXSetDirtyState` (640 B) 99.21 -> 100 %, both loops; unit 8/9 ->
9/9 rows, flipcheck READY, trial flip `main.dol: OK`. Measured on the way (row score): shift the parameter 98.97;
mask as a split of a folded test 99.19; mask pre-shifted in the caller (hoisted above the count test) 97.87 / 97.84;
both loops in the body with reused `dirty`/`i`/`addr` 98.97; body chan loop with distinct `addr`/`lightMask`/
`lightBits` locals 99.59 (chan loop right, tex walk r5); `0x1040 + i` address inductions 92.10 (not strength-reduced).

**Example.**

```c
static inline void __GXSetTexGen(GXData* gx, u32 dirty) {
    u32 i;
    u32 bits;                 /* first web: the count written below; second web: the loop mask */
    u32 taddr = 0x1040;       /* helper locals number before the walk: r6 (and ptaddr r5) first */
    if (dirty & 0x02000000) { bits = gx->genMode.f.numTexGens; XF_WRITE(0x103F, bits); }
    bits = (dirty >> 16) & 0xFF;                      /* a later def: a new web, numbered after the walk */
    for (i = 0; bits != 0; taddr++, i++, bits >>= 1) {
        u32 ptaddr = taddr + 0x10;
        if (bits & 1) { XF_WRITE(taddr, gx->texGenCtrl[i].word); XF_WRITE(ptaddr, gx->postTexGen[i].word); }
    }                                                 /* mr r7,r31 (walk) ... extrwi r8,r8,8,8 (mask) */
}
```
