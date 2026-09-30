---
id: 68
title: A derived class's VTABLE is emitted where its KEY FUNCTION is defined
status: works
problem: A reconstructed derived class must store its base's vtable pointer without emitting a table of its own (the target's `.data` is the base table alone), but declaring the class normally makes MWCC emit the derived vtable and `.data` grows.
tags: [vtable]
applies: [Network]
demo: 068-vtable-key-function.cpp
reviewed: 2026-09-29
related: [52, 69, 76]
---

# 68. A derived class's VTABLE is emitted where its KEY FUNCTION is defined

**Problem.** The unit reconstructs a derived class that must store the base's vtable pointer
(`__vt__24NetworkSessionManagerPat`) without emitting a table of its own - the target's `.data` is the base
table alone, 0x1C8 bytes. Declaring the class normally makes MWCC emit the derived vtable into our object
and the unit's `.data` grows (rule 10's "extra bytes" trap, from the other side).

**How it looks.** Our `.data` is larger than the target's by one vtable (`datagap.py --unit` shows an
ours-extra `__vt__...`), while the constructor's `lis/addi` + `stw` of the table address is already right.

**Why it happens.** MWCC follows the C++ **key function** rule: the vtable is emitted in the translation
unit that **defines the class's first declared non-inline virtual member**. Declare that member - the key
function - **first**, and give it its body in another unit (the next band), and MWCC emits the
constructor's vtable *store* (a relocation against the table) but no table into this object. A class whose
first declared virtual is the destructor, with a body in this unit, *does* emit it.

**How to work it.** Order the class's virtual declarations so the first is the one whose body lives
elsewhere; keep everything else as is. Confirm with `datagap.py` / the object's symbol table that
`__vt__<Class>` is undefined here and referenced by the constructor.

**When NOT to apply.** If the target's `.data` *does* contain the derived table (the unit owns it), the key
function must be defined here - and rule 10 then says the table is compiler output, never hand-written. The
rule depends on the first declared virtual; an inline first virtual moves the key to the next non-inline one.

**Result.** Measured at the time (2026-09): `NetworkSessionManagerPat` declares `virtual void move();` FIRST
(its body is in the next band, 0x803D70B8) and then the ctor/dtor/init/clear/release: the constructor stores
`__vt__24NetworkSessionManagerPat` and no Pat vtable is emitted - `.data` is the base table alone.

**Example.**

```cpp
class NetworkSessionManagerPat : public NetworkSessionManager {
public:
    virtual void move();            /* key function FIRST, body in the next band: store yes, table no */
    NetworkSessionManagerPat(...);
    virtual ~NetworkSessionManagerPat();
};
```

**Demonstration.** `068-vtable-key-function.cpp` (`ideas.py demo-check 68`): `DtorFirst` (destructor first,
defined) gets a defined 0x14-byte `__vt__9DtorFirst`; `KeyFirst` (`kmove` first, no body) has only an
undefined `__vt__8KeyFirst` reference from its constructor.
