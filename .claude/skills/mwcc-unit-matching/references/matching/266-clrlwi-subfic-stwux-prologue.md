---
id: 266
title: A clrlwi/subfic/stwux prologue means a 32-aligned local; a scalar kept in memory lives in a struct
status: works
problem: Retail opens with `clrlwi r11,r1,27; subfic r11,r11,-N; mr r12,r1; stwux r1,r1,r11` and keeps one scalar in a stack slot (`stw`/`lwz` around every join) where ours has a static `stwu` frame and the scalar in a register, so every stack offset and the save range differ.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [10, 170, 229]
---

# 266. A clrlwi/subfic/stwux prologue means a 32-aligned local; a scalar kept in memory lives in a struct

**Problem.** The function's first four instructions are a dynamic stack realignment (`clrlwi r11,r1,27`,
`subfic r11,r11,-0x840`, `mr r12,r1`, `stwux r1,r1,r11`, then `stw r0,4(r12)` and `mr r11,r12` before the
`_savegpr_` call), and a byte count is stored to and reloaded from one stack slot at every join. Ours has a plain
`stwu r1,-0x8F0(r1)`, so the whole function reads ~75 % with every `r1` offset and the frame size wrong.

**Why it happens.** MWCC realigns the frame at run time when any local asks for more than 8-byte alignment: the
`clrlwi 27` is a 32-byte alignment. Locals are then laid out in declaration order from `0x20`, each at its own
alignment, so the gaps in retail's offsets show which locals carry the attribute (a DVD file record at `0x20`, a
spawn set rounded up to `0x240`). A scalar that MWCC never promotes to a register is a struct member: retail's
`size` sits at `+0x128` right before the path buffer at `+0x12C`, one record.

**How to work it.**
1. Read retail's `r1` offsets for each local; one that starts on a 32-byte boundary after a gap gets
   `__attribute__((aligned(32)))`. The first such local turns the prologue into the `stwux` form.
2. Size the buffers from the gaps (the path was `char[0x100]`, not `[0x200]`), and order the declarations by offset.
3. A scalar retail keeps in memory next to a buffer becomes a field of a small record with that buffer.

**Result.** `ef/fn_800FD864_fx` `fn_800FE978` (3784 B): 75.74 -> 86.99 % from the two `aligned(32)` locals and the
buffer size alone, 97.58 % once the byte count moved into `EftFileLoad { u32 size; char name[0x100]; }` (frame,
offsets and size identical), 99.78 % after the register work (idea 243).

**Example.**

```
struct EftFileLoad { u32 size; char name[0x100]; };
void f(void) {
    nw4r::math::MTX34 mtx;
    EftFileStat info __attribute__((aligned(32)));   /* +0x20: the clrlwi/subfic/stwux prologue */
    EftFileHeader header;
    EftFileLoad file;                                /* file.size stays in its stack slot */
    EftSpawnSet set __attribute__((aligned(32)));    /* rounded up to +0x240 */
}
```
