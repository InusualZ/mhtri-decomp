---
id: 98
title: A class's vtable pointer lands where its first virtual is declared
status: works
problem: After a hand-wired `void* vtable_00` becomes a real `virtual ~Class()`, every field access in the class moves four bytes and dozens of rows drop ten points with nothing but an offset changed.
tags: [source-shape, vtable]
applies: [Wii/1.3]
demo: 098-vptr-follows-first-virtual.cpp
reviewed: 2026-09-30
related: [52, 68, 76]
---

# 98. A class's vtable pointer lands where its first virtual is declared

**Problem.** Converting `GameSpyInterfaceThread` from a hand-written `void* vtable_00; ... void* destroy(s16)` to a
real `virtual ~GameSpyInterfaceThread()` (so MWCC emits the vtable and the constructor's store itself, rule 10) made
42 of 71 rows fall: `clearError` read `stb r0,0xC(r31)` where retail has `0x10`, every field sat four bytes low.

**Why it happens.** MWCC lays a class out in declaration order and inserts the vtable pointer when it meets the
**first virtual declaration**: fields declared *before* it keep their offsets and the pointer follows them (at the end
of the fields seen so far), fields after it start behind a pointer at +0. The annotated `/* +0x00 */ void* vtable_00`
the class used to carry is not reproduced by merely adding a virtual at the bottom.

**How to work it.** Declare the virtual (here the deleting destructor) **first** in the class body, before the first
field, and drop the hand-written pointer field; keep the offset annotations on the remaining fields. Check the
constructor against the target: the vtable store is now the compiler's, at the top of the body.

**Result.** `Network/fn_8041A87C` (2026-09-30): after moving `virtual ~GameSpyInterfaceThread()` to the top of the
class every row returned to its previous score, the unit emits `__vt__22GameSpyInterfaceThread` (12 B; retail's
table is 16 B with alignment padding) and `__dt__22GameSpyInterfaceThreadFv`, and `vtableaudit --diff` drops one
violation.

**Example.**

```
class Early { public: virtual ~Early(); int field; };   // field at +4
class Late  { public: int field; virtual ~Late(); };    // field at +0, pointer after it
```
