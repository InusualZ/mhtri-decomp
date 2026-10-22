---
id: 175
title: A real constructor call with no unwind record is placement new over operator new
status: works
problem: The allocating function's `.text` is 100 % once the class is real, but its `extab` record grows by 0x10 with a `__dl__FPv` relocation the target does not have.
tags: [source-shape, sections, vtable]
applies: [enemy]
demo: 
related: [62, 68]
---

# 175. A real constructor call with no unwind record is placement new over operator new

**Problem.** A hand-written `p = (T*)operator new(n); if (p) T_ctor(p);` is turned into a real class so MWCC
emits the class table (rule 10). Written as `p = new T;` the `.text` stays 100 %, but the caller's `extab`
record grows from 8 to 0x18 bytes and `.relaextab` gains a `__dl__FPv` entry: flipcheck reports `extab` 0x10
over the claim. The target has the constructor call, the null test and the saved pointer, and no unwind
record.

**Why it happens.** With `-Cpp_exceptions on` a new-expression whose constructor may throw gets a landing pad
that frees the block through the matching deallocation function (idea 62). MWCC adds it for every global
`new T` - a visible inline constructor, a class `operator new`, or `throw()` on the constructor (that one adds
a spec record to the constructor instead) all keep it. A *placement* new-expression looks for a placement
`operator delete(void*, void*)`; with none declared there is no landing pad, and MWCC still null-tests the
placement result before the constructor call.

**How to work it.** Allocate with `operator new(sizeof(T))` into the pointer, then construct with
`new (p) T;` through the scalar placement `operator new(unsigned long, void*)` of `MSL/new.h` (inline,
returns its block; no placement delete declared). Do not wrap it in `if (p)`: the placement result is
already tested, a second test adds a `beq`. Do not assign the placement result either: retail keeps the
`operator new` result (`mr r4,r31`), the new-expression's value is the constructor's `this` (`mr r4,r3`).

**Result.** `enemy/em009_act`: `em009_init` 100 % with `extab` 0x188 = the claim (0x198 with `new T`, 99.44 %
with `p = new (operator new(n)) T`); `.data` gains the class table `__vt__13Em009UserData` byte-identical, and
the unit's data match went 1108 -> 6012 B. The same spelling holds the allocation rows of em003, em008,
em015, em016, em018, em019, em020, em022 and em025 at their previous scores, with each unit's table emitted.

**Example.**

```cpp
#include "MSL/new.h"
helper = (Em009UserData*)operator new(sizeof(Em009UserData));
new (helper) Em009UserData;           /* null test + ctor call, no landing pad */
em_res_user_data_set(self, helper);   /* the operator new result, as retail passes it */
```
