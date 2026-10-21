---
id: 174
title: A unit's claimed data is its own literals, type-name records, statics and inline tail: write them as source
status: works
problem: A unit whose code rows are all 100 % emits none of its claimed `.data`/`.sdata`/`.rodata`/`.bss`, so flipcheck refuses it - the source declares every string, type-name record and constant `extern`.
tags: [data, sections, source-shape]
applies: [Wii/1.3]
demo: 
related: [43, 80, 147, 163, 166, 169]
---

# 174. A unit's claimed data is its own literals, type-name records, statics and inline tail: write them as source

**Problem.** A g3d unit at 100 % code (g3d_anmvis: 8/8 rows, `.data` 0x120 + `.rodata` 0x10 + `.sdata` 0x8 claimed)
emits no data section, because its panic strings, type-name records and pool constants are `extern`s of map names.

**Why it happens.** The claimed bytes are what the TU's own source compiled to:
1. its string literals (`__FILE__`, the NW4R assert messages) in `.data`, first-use order, `-str reuse` merging
   repeats; a string of 8 bytes or less goes to `.sdata`;
2. type-name records `{u32 sizeof("Name"), "Name"}` (`NW4R_G3D_TYPE_OBJ_DEF`) in `.rodata`, from a `const` aggregate;
3. file-scope statics with constructors in `.bss`, with `__sinit_\<file>_cpp`, its `.ctors` word and its `.sdata2`
   constants;
4. the inline tail: each inline function's strings as weak `@STRING@<fn>` objects, never merged, after the vtables,
   in reverse creation order (idea 80), whether or not the function was inlined.

**How to work it.** Write literals with a file-scope `#pragma pool_data off` (each string its own `lis`/`addi`;
without it MWCC shares one base, idea 43), the type-name record as a defined `const` object, the statics as
definitions, and the `.text` definitions in retail's address order (the string and pool order follow it). Make the
`_ac.h` accessors `inline` and keep them out of line (idea 163). Where retail's referencer of an accessor was
dead-stripped, add a frameless one-call referencer (`return acc(p);`, a 4-byte `b`, no extab record) where it sat:
the linker strips it again and the weak copy lands in retail's place.

**Result.** Wii/1.3, trial-flipped together with the DOL hash unchanged: `g3d/g3d_anmvis` (all data byte-identical,
fn_8006EAC0 90.53 -> 100 with `pool_data off`), `g3d/g3d_resanmcamera` (93.70 -> 100), `g3d/fn_8005AA28` (`.bss`,
`.ctors`, `.sdata2` byte-identical). `g3d_basic`, `g3d_resnode`, `g3d_resanmfog`, `g3d_resanmamblight` and
`g3d_scnmdl` data byte-identical; the dead-referencer recipe also placed three `ef/ef_postfield` accessors. Ruled out:
lib-wide `-inline off` instead of `-inline noauto` (emits the copies, but ten units drop, e.g. anmchr 99.50 -> 98.45).

**Example.**

```
#pragma pool_data off
extern "C" const TypeNameData<sizeof("AnmObjVis")> anm_typename_AnmObjVis = {sizeof("AnmObjVis"), "AnmObjVis"};
u32 anm_obj_vis_res_is_valid(ResHandle* pRes) { return res_is_valid(pRes); }   /* a b; stripped by the link */
```
