---
id: 76
title: A class hierarchy is traced from MANGLINGS, the CTOR's vtable store and SHARED SLOT OFFSETS - and a by-index accessor family is usually a container
status: works
problem: Source modelled four `Network` entities as four classes behind a family of `void*` accessors plus hand-modelled `*Vtable` structs, but nothing measured said the binary had four classes - what is the shared abstraction?
tags: [vtable, symbols]
applies: [Network]
demo: 076-class-hierarchy-tracing.cpp
reviewed: 2026-09-29
related: [52, 68, 69, 79]
---

# 76. A class hierarchy is traced from MANGLINGS, the CTOR's vtable store and SHARED SLOT OFFSETS - and a by-index accessor family is usually a container

**Problem.** Our source modelled four `Network` entities (`NetworkSessionManagerPat`, `NetworkPatSlot04`,
`NetworkCommunityPat`, `NetworkLayerPat`) as four classes behind a family of `void*` accessors
(`getXPat(self, index)`, `setXPat(self, void* value)`, `clearXPat`, `deleteXPat`), plus hand-modelled
`*Vtable` structs. Nothing measured said the binary had four classes, and rule 11 (no `void *` in a declaration)
bans the `void *` half outright - so the question "what is the shared abstraction?" has to be answered from the
binary before any of it is written.

**How it looks.** No single first divergence: the symptom is a *source* that is suspiciously uniform (four
look-alike types, accessors taking an index and a `void*`) while the map has almost no C++ mangling for them, and
the vtable rule (idea 52: a vtable we own must be compiler-emitted) cannot be applied because it is unclear which
types are classes at all.

**Why it happens.** The compiler leaves three different fingerprints, and hand-written source leaves none of
them. (`vptr` = the hidden pointer at `+0x00` of a polymorphic object; `vtable` = the table of function pointers it
points at; a *slot* is one entry, addressed by its byte offset in the table.)

**How to work it.** Three cheap, ordered steps, each of which can *end* the question:

1. **The map's manglings are the first filter.** `__ct__<len><Name>Fv`, `__dt__...` and `__vt__...` name a real
   class (`24` is the class-name length: `__vt__24NetworkSessionManagerPat`). A name with no mangled row, no ctor
   and no dtor is not a class, whoever wrote it. `grep -c "^__vt__" config/RMHE08/symbols.txt` is the whole test.
2. **A constructor that stores a table address declares the vtable and its owner.**
   `lis rX, vt@ha; addi rX, rX, vt@l; stw rX, 0(r3)` (or the address already in a register) marks `+0x00` as the
   vptr and names the table by its relocation. The demo shows the source-side shape: a class with virtual members
   gets a `__vt__` symbol and a vptr store; a class without any gets neither.
3. **The shared slot offsets at the indirect call sites are what actually tests "one interface".** An indirect
   call reads `lwz r12,0(obj); lwz r12,NN(r12); mtctr r12; bctrl` (a tail call ends in `bctr`). The first
   virtual method of a MWCC class sits at `NN = 0x08` (the first two words are a header, `0,0` with `-RTTI off`),
   and each further slot adds 4. Equal `NN` across *unrelated* objects means a **duck-typed protocol** - the same
   offsets used by objects with no common base - not a shared base class; that is exactly what a hand-modelled
   vtable struct erases.

**When NOT to apply.** A class whose constructor is inlined into its only caller has no `__ct__` row; the vptr
store then sits inside the caller (look for the table address there). And a missing mangled row proves nothing
for a file the map's analyser could not demangle: use step 2 on the stores, not step 1 alone.

**Result.** The trace cost no build and answered the question the other way round: **one non-polymorphic
container with four slots, not a hierarchy.** The offsets seen (`+0x08` deleting dtor, `+0x14` release, `+0x18` a
driver call) are a protocol shared by four unrelated root classes, two of which call no base constructor. Also
worth knowing: a vtable, its key function and its holder that sit in **unclaimed** ranges are invisible to
`vtableaudit.py` (it audits owned ranges only) - the audit passing is not evidence that a band is modelled.

**Evidence.** (Measured at the time, on the Network band.) The whole 65,700-line map carried exactly **one**
`__vt__` row (`__vt__24NetworkSessionManagerPat` 0x805FB0F0) and exactly one of the four names had any mangled row,
so three were slot labels, not classes; the holder is a 0x14-byte singleton at 0x806EFC44 whose ctor `memset`s
`+0x00..+0x0C` and writes `+0x10 = -1` (a 4-bit slot-enable mask); all four `deleteNetwork*` helpers read `+0x14`
then `+0x08` (flag 1) and the driver reads `+0x18`. Today the map has 8 `__vt__` rows (the peer/resolver/session
classes were reconstructed since). Full trace, dead ends and the seven source defects:
`.pi/notes/network-pat-abstraction.md` (local scratch, not tracked). Related: idea 68 (where a derived class's
vtable is emitted), 69 (array of polymorphic members), 79 (emit what the reconstruction supports).
