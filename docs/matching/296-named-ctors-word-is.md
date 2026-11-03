---
id: 296
title: A named .ctors word is dead-stripped: emit the entry through a file-scope object's constructor
status: works
problem: the unit's object is byte-identical (every row 100 %, `.ctors` 4 B on both sides) and the flipped link still fails the DOL hash; `flipcheck` names the unit's own `.ctors` word as dead-stripped
tags: [sections, linker, source-shape]
applies: [Wii/1.0, Wii/1.3]
demo:
related: [46, 116, 193, 242]
---

# 296. A named .ctors word is dead-stripped: emit the entry through a file-scope object's constructor

**Problem.** A unit whose target has a static initialiser was written with a hand-made `.ctors` word -
`__declspec(section ".ctors") void* const unit_ctor = (void*)unit_static_init;` beside an ordinary
`extern "C" void unit_static_init(void)`. The object is byte-identical (the function at 100 %, `.ctors` 4 B on
both sides, `datagap` clean), but the flipped link moves the hash: mwld keeps the initialiser's `.text` (FORCEACTIVE)
and drops the word, so the linked `.ctors` table is one entry short and every later entry shifts. `flipcheck.py`
reports it as "the link dead-strips the named table entry(ies) ... the word has to be the compiler's anonymous entry".

**Why it happens.** Nothing references a named `.ctors` object, so the linker's dead-strip removes it like any other
unreferenced datum. MWCC's own entry for a TU with a static initialiser is an *anonymous* `.ctors` word pointing at
`__sinit_\<file>_cpp`, and mwld keeps those (and the runtime's `*_reference` names) by construction (idea 46 has the
class order). The `__sinit` is emitted after the last user function of the TU - exactly where retail's static
initialiser sits - and inlines the constructor of each file-scope object (a single `setVec3` becomes a tail call `b`).

**How to work it.** Delete the named word and the hand-written initialiser. Give the file-scope object's type a
constructor that does what the initialiser did, keep the object's definition (`YureWorld yure_world;` stays in
`.bss`), and rename the map row of the initialiser to the compiler's spelling with `symedit.py rename <old>
'__sinit_\<file>_cpp'` (the row pairs by name; the backslash is part of the name). Re-split, then `flipcheck` and a
trial flip. An out-of-line constructor after `__sinit` is idea 193/242; here the inlined form is retail's.

**Result.** `Pl/pl_yure`: `__sinit_\pl_yure_cpp` 100 % at the same offset, `.ctors` anonymous 4 B, `flipcheck`
READY (was "dead-strips the named table entry yure_ctor"), trial flip `main.dol: OK`.

**Example.**

```
struct YureWorld {
    /* +0x00 */ nw4r::math::VEC3 gravity_0x00;
    YureWorld() { setVec3(&gravity_0x00, 0.0f, -1.0f, 0.0f); }   /* becomes __sinit_\pl_yure_cpp: b setVec3 */
};
YureWorld yure_world;                                              /* MWCC emits the anonymous .ctors word */
```
