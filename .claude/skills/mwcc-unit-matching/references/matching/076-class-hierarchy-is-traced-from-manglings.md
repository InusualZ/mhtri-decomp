---
id: 76
title: A class hierarchy is traced from MANGLINGS, the CTOR's vtable store and SHARED SLOT OFFSETS - and a by-index accessor family is usually a container
status: works
problem: Our source modelled four `Network` entities (`NetworkSessionManagerPat`, `NetworkPatSlot04`, `NetworkCommunityPat`, `NetworkLayerPat`) as four classes behind a family of `void*` accessors (`getXPat(self, index)`, `setXPat(self, void* value)`, `clearXPat`, `deleteXPat`), plus hand-modelled `*Vtable` structs. Nothing measured said the binary had four classes, and rule 11 bans the `void *` half outright - so the question "what is the shared abstraction?" has to be answered from the binary before any of it is written.
tags: [vtable, symbols, measurement]
applies: []
demo:
---

# 76. A class hierarchy is traced from MANGLINGS, the CTOR's vtable store and SHARED SLOT OFFSETS - and a by-index accessor family is usually a container

**Problem.** Our source modelled four `Network` entities (`NetworkSessionManagerPat`, `NetworkPatSlot04`,
`NetworkCommunityPat`, `NetworkLayerPat`) as four classes behind a family of `void*` accessors
(`getXPat(self, index)`, `setXPat(self, void* value)`, `clearXPat`, `deleteXPat`), plus hand-modelled
`*Vtable` structs. Nothing measured said the binary had four classes, and rule 11 bans the `void *` half
outright - so the question "what is the shared abstraction?" has to be answered from the binary before any
of it is written.

**Why try it.** Three cheap, ordered steps, each of which can *end* the question:

1. **The map's manglings are the first filter.** `__ct__<len><Name>Fv`, `__dt__…` and `__vt__…` name a real
   class (`24` is the class-name length); a name with no mangled row, no ctor and no dtor is not a class,
   whoever wrote it. Here the whole 65,700-line map carries exactly **one** `__vt__` row
   (`__vt__24NetworkSessionManagerPat` 0x805FB0F0), and exactly one of the four names has any mangled row at
   all - so three of them are **slot labels**, not classes.
2. **A constructor that stores a table address declares the vtable and its owner.**
   `lis rX, vt@ha; addi rX, rX, vt@lo; stw rX, 0(r3)` (or the address in a register) marks `+0x00` as the vptr
   and names the table. `__ct__24NetworkSessionManagerPatFv` calls the base ctor 0x803D4904 and stores
   0x805FB0F0 at `+0x00`.
3. **The shared slot offsets at the indirect call sites are what actually tests "one interface".** An indirect
   call reads `lwz r12,0(obj); lwz r12,NN(r12); mtctr; bctrl`. Equal `NN` across *unrelated* objects means a
   **duck-typed protocol**, not a shared base class - and that is the distinction a hand-modelled vtable
   erases. All four `deleteNetwork*` helpers read `+0x14` (release) then `+0x08` (deleting dtor, flag 1), and
   the holder's driver reads `+0x18`: the protocol is `{+0x08, +0x14, +0x18}`, and the four elements sit in
   three unrelated root classes (two of which call no base ctor).

**Result.** The trace cost no build and answered the question the other way round: **one non-polymorphic
container with four slots, not a hierarchy.** The holder is a 0x14-byte singleton at 0x806EFC44 whose ctor
`fn_80419AD4` `memset`s `+0x00..+0x0C` and writes `+0x10 = -1`, a 4-bit slot-enable mask (so the missing
`+0x10` field in our struct is a defect, and slot `+0x04` has no setter anywhere in the DOL); the install site
`fn_804292F8` types each slot from what it stores; and our source's `NetworkPatEntry` base class is
fabricated. Also worth knowing: the Pat vtable 0x805FB0F0, its key function and the holder all sit in
**unclaimed** ranges, so `vtableaudit.py` is silent about them (it audits owned ranges only) - the audit
passing is not evidence that this band is modelled. Full trace, the dead ends, and the seven concrete source
defects: `.pi/notes/network-pat-abstraction.md`.