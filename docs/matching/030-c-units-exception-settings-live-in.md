---
id: 30
title: A C++ unit's exception settings live in its object, not in the source
status: works
problem: every function of a unit matches instruction for instruction, and the *unit* still measures short because its target object carries `extab`/`extabindex` while ours carries none, or carries different records. It reads as a codegen problem, so the search goes into the source and stays there.
tags: [sections, measurement, process]
applies: []
demo:
---

# 30. A C++ unit's exception settings live in its object, not in the source

Problem: every function of a unit matches instruction for instruction, and the *unit* still measures short
because its target object carries `extab`/`extabindex` while ours carries none, or carries different records.
It reads as a codegen problem, so the search goes into the source and stays there.

Why try it: `-Cpp_exceptions off` in `cflags_base` is a **build** setting, and not every retail object was built
with it. The object says which one it was: `extab`/`extabindex` present means exceptions were on, and their
*entries* say which constructs were used - a `throw()` specification emits one handler reference and an empty
action list, where a real `try`/`catch` emits far more. The setting is per unit, not per project: in this repo
the `Pl` lib and `Gecko_ExceptionPPC.cp` need it on, while `__init_cpp_exceptions.cpp` would *gain* extab its
target does not have.

Result (batch 5): `-Cpp_exceptions on` for the `Pl` lib left every `.text` byte unchanged and made all 12 extab
entries we could emit equal the target's; `#pragma exceptions on` in `sys_mem.cpp` (a unit whose lib keeps the
flag off) turned its `operator new`/`operator delete` `throw()` specs into the target's extab byte for byte; the
same pragma on `Gecko_ExceptionPPC.cp` reproduced the target's `extab 0x10` + `extabindex 0x18` exactly. So make
it part of a unit's measurement: compare the two objects' `extab`/`extabindex` **sizes and bytes**, not only the
score.

Example:

```c
#pragma exceptions on   /* the source spelling of the one flag, for a unit in a lib that has it off */
```

and for a `$`-suffixed section the pragma **pair** is required: `#pragma section const_type ".ctors$10"` to name
it plus `__declspec(section ".ctors$10")` to place it - `__declspec` alone is rejected (error 33048), and
`code_type` would add a `.mwcats` section the target does not have.
