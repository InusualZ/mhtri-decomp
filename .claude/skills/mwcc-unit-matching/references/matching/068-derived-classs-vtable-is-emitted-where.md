---
id: 68
title: A derived class's VTABLE is emitted where its KEY FUNCTION is defined
status: works
problem: The unit reconstructs a derived class that must store the base's vtable pointer (`__vt__24NetworkSessionManagerPat`) without emitting a table of its own - the target's `.data` is the base table alone, 0x1C8 bytes. Declaring the class normally makes MWCC emit the derived vtable into our object and the unit's `.data` grows (rule 10's own "extra bytes" trap, from the other side).
tags: [vtable, data]
applies: []
demo:
---

# 68. A derived class's VTABLE is emitted where its KEY FUNCTION is defined

**Problem.** The unit reconstructs a derived class that must store the base's vtable pointer
(`__vt__24NetworkSessionManagerPat`) without emitting a table of its own - the target's `.data` is the base
table alone, 0x1C8 bytes. Declaring the class normally makes MWCC emit the derived vtable into our object
and the unit's `.data` grows (rule 10's own "extra bytes" trap, from the other side).

**Why try it.** MWCC follows the C++ key-function rule: the vtable is emitted in the translation unit that
**defines the class's first declared non-inline virtual member**. Declare that member - the key function -
**first**, and give it its body somewhere else (the next band), and MWCC emits the constructor's vtable
**store** but no table into this object. That is the way to reconstruct a class that stores a vtable without
owning it, and it is decidably different from the ordinary case: a class whose first declared virtual is the
destructor (as with the base here) *does* emit.

**Result.** `NetworkSessionManagerPat` declares `virtual void move();` FIRST - its body is in the next band
(0x803D70B8) - and only then the ctor/dtor/init/clear/release. MWCC emits the
`__vt__24NetworkSessionManagerPat` store into the constructor and **no** Pat vtable, which is exactly what
the target shows: `.data` is the base table alone at 0x1C8 bytes.

**Example.**

```cpp
class NetworkSessionManagerPat : public NetworkSessionManager {
public:
    virtual void move();            /* key function FIRST, body in the next band: store yes, table no */
    NetworkSessionManagerPat(...);
    virtual ~NetworkSessionManagerPat();
};
```
