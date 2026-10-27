---
id: 242
title: Member constructors kept out of line after __sinit with an in-class never_inline attribute
status: works
problem: retail's `__sinit` calls one class constructor, followed in `.text` by a chain of member-array constructors (`do { bl ctor; p += size; } while (p < end)`) that call out-of-line leaf constructors; ours inlines the whole tree into `__sinit`
tags: [source-shape, sections]
applies: [Wii/1.3]
related: [193, 163, 43]
demo: 
---

# 242. Member constructors kept out of line after __sinit with an in-class never_inline attribute

**Problem.** Retail's static initialiser makes one `bl` to the object's constructor, and the functions after
`__sinit` are that constructor and its member classes' constructors, each a `do { bl ...; p += sizeof; } while (p <
end)` loop per member array, ending in calls to C-named leaf functions (`VEC3_ctor`, `mhchar_construct`). Ours
inlines every constructor into `__sinit` (`-inline noauto` still inlines in-class bodies), and `#pragma dont_inline`
at the end of the file keeps them all out of line, the leaf wrappers too, so the leaf calls become `__ct__` calls.

**Why it happens.** MWCC decides inlining for `__sinit` and every compiler-generated or in-class constructor at the
end of the TU, under the pragma state there. A constructor it does not inline is emitted after its first user, and
the deferred list is processed last-in first-out: `__sinit`, the object's constructor, then its members'
constructors in reverse order of reference, a member's own members right after it.

**How to work it.** Give each class whose constructor retail keeps out of line an in-class constructor with the
attribute after the declarator, `X() __attribute__((never_inline)) {}` (before the declarator, or as
`__declspec(noinline)` in class, MWCC refuses it), and leave the leaf types as plain inline wrappers
(`StageVec3() { VEC3_ctor(this); }`) so their calls inline to the C names the map carries. Rename the map rows to
the compiler's manglings (`__ct__9StageWorkFv`). Add `#pragma pool_data off` when retail's `__sinit` addresses each
`.bss` object by its own symbol (idea 43).

**Result.** `stage/stg_w`: `__sinit_\stg_w_cpp` plus six constructors (`__ct__9StageWorkFv`, `__ct__12StageEftSeatFv`,
`__ct__9StagePartFv`, `__ct__9StageAreaFv`, `__ct__12StageAreaRowFv`, `__ct__9StageSlotFv`) emitted in retail's
order, all 100 %; `.ctors` and `.bss` 100 %.

**Example.**

```
struct StageVec3 : VEC3 { StageVec3() { VEC3_ctor(this); } };          /* inlined: bl VEC3_ctor */
struct StagePart { u8 state; u8 pad[3]; StageVec3 joint_base[3]; /* ... */
                   StagePart() __attribute__((never_inline)) {} };      /* out of line, after __sinit */
struct StageWork { StagePart parts[4]; StageWork() __attribute__((never_inline)) {} };
StageWork stage_w;                                                     /* __sinit: bl __ct__9StageWorkFv */
```
