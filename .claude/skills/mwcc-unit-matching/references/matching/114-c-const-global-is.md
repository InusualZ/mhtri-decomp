---
id: 114
title: A C const global is folded into a pool literal: declare it extern before the bodies and define it after them
status: works
problem: A unit whose asm and C bodies load the same named `.sdata2` constants emits extra anonymous pool entries (`@NNN`) beside the named ones, so .sdata2 grows
tags: [data, sections, source-shape]
applies: [Wii/1.3]
demo: 
---

# 114. A C const global is folded into a pool literal: declare it extern before the bodies and define it after them

**Problem.** An SDK unit (`MTX/mtx.c`) mixes `nofralloc` asm functions that load constants by name (`lfs f0, sMtxOne(r0)`)
with C functions that need the same values. Defining `const f32 sMtxOne = 1.0f;` at the top of the file makes the C bodies
fold the value into the compiler's own pool: the object gains six anonymous `@NNN` entries (`.sdata2` 0x34 bytes
against the target's 0x20) while the named constants stay.

**Why it happens.** In a C translation unit MWCC treats a `const` global with a visible initialiser as a compile-time
constant, so a read of it becomes a pooled literal load instead of a load of the symbol.

**How to work it.** Declare the constants `extern const f32 sName;` before the bodies (the asm and the C code both load
them) and put the definitions after the last function. The C bodies then load the named symbol (`lfs fN, sName@sda21`),
no `@NNN` entry appears, and the `.sdata2` bytes and symbol order are the definitions' order. A zero-filled `const f32 x[2]`
goes to `.sbss2` instead; an 8-byte pool object whose second word is padding stays a 4-byte object (the link pads it).

**Result.** `MTX/mtx`: `.sdata2` 0x34 -> 0x1C bytes (the target's 0x20 minus 4 bytes of alignment padding), 17 of 19 rows at
100 %; `MTX/vec`, `MTX/mtx44`, `MTX/quat` the same.

**Example.**

```
extern const f32 sMtxOne;                 /* before the bodies */
void C_MTXLightOrtho(...) { m[2][3] = sMtxOne; ... }
const f32 sMtxOne = 1.0f;                 /* after the last body */
```
