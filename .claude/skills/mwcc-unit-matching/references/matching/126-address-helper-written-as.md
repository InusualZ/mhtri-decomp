---
id: 126
title: An address helper written as a macro instead of an inline function stops its argument copies recolouring the caller
status: works
problem: Same instructions and length, but two volatile registers that carry an offset and a derived pointer are swapped (r5/r6) through the whole function.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [63, 123]
---

# 126. An address helper written as a macro instead of an inline function stops its argument copies recolouring the caller

**Problem.** Every instruction matches except that two volatile registers are swapped from the first one on:
retail loads the list's offset into `r5` and keeps the element's link pointer in `r6`, ours the other way round
(`lhz r6,0xa(r3)` / `add r5,r4,r6`). Five shapes of the body itself (a local for `prev`, the decrement spelled
out, the offset in a local, `char*` arithmetic) leave it at the same score or worse.

**Why it happens.** Virtual registers are numbered in the order the IR creates them. A `static inline` helper
(`GetLink(list, object)`) copies its arguments into fresh virtual registers when it is expanded, so every call
site adds temporaries ahead of the caller's own values and the colouring order shifts. A macro expands to the
bare expression tree, with no argument copies.

**How to work it.** When the residual is a register permutation inside a function that calls a one-line address
helper several times, write the helper as a macro (NW4R spells these as macros), or expand the expression at
every site. Expanding only the first or only the later sites was worse (95.19) - all sites have to change together.
Re-measure the helper's other callers: here they stayed at 100 %.

**Result.** `nw4r/ut_list` `List_Remove` (0x6C): 97.22 -> 100 %; the unit 7/7 at 100 %, `flipcheck` READY.

**Example.**

```cpp
#define GetLink(list, object) reinterpret_cast<Link*>(reinterpret_cast<u32>(object) + (list)->offset)

void List_Remove(List* list, void* object) {
    Link* link = GetLink(list, object);
    if (link->prevObject == NULL) {
        list->headObject = link->nextObject;
    } else {
        GetLink(list, link->prevObject)->nextObject = link->nextObject;
    }
    /* ... */
}
```

**The reverse direction.** When retail *did* go through an inline wrapper, its argument copies are what
colours the caller: `MSL_C/printf` `double2hex` 99.92 -> 100 (Wii/1.3) with MSL's
`static inline f64 fabs(f64 x) { return __fabs(x); }` instead of calling `__fabs` directly (the f0/f1 pair
recoloured). Read retail's register order first, then decide macro or inline; an inline helper *invented* in place
of a `goto` recolours the same way and does not work (idea 145).
