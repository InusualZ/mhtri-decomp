---
id: 30
title: A C++ unit's exception settings live in its object, not in the source
status: works
problem: Every function of a unit matches, and the *unit* still measures short because its target object carries `extab`/`extabindex` while ours carries none (or different records); the search goes into the source and stays there.
tags: [flags, sections]
applies: [Wii/1.3]
demo: 030-exception-settings-in-object.cpp
reviewed: 2026-09-29
related: [46, 49, 59, 62, 83]
---

# 30. A C++ unit's exception settings live in its object, not in the source

**Problem.** Every function of a unit matches instruction for instruction, and the *unit* still measures short
because its target object carries `extab`/`extabindex` while ours carries none, or carries different records.
It reads as a codegen problem, so the search goes into the source and stays there.

**How it looks.** `python tools/elf/elfsect.py` on the two objects: the target has `extab` and `extabindex` (and
`.relaextabindex`), ours has neither. `.text` is byte-identical. (`extab` = the exception-unwind action tables;
`extabindex` = the per-function index into them.)

**Why it works.** `-Cpp_exceptions off` in `cflags_base` is a **build** setting, and not every retail object
was built with it. The object says which one it was: `extab`/`extabindex` present means exceptions were on, and
their *entries* say which constructs were used - a `throw()` specification emits one handler reference and an
empty action list, where a real `try`/`catch` emits far more. The setting is per unit, not per project.

**How to work it.** Make it part of a unit's measurement: compare the two objects' `extab`/`extabindex` **sizes and
bytes**, not only the score. If they are missing, enable exceptions - as a library flag when the whole library
carries them (`-Cpp_exceptions on`: `cflags_pl`, `cflags_network`, `cflags_main` in `configure.py`, each with its
evidence, flags-audit 2026-09-28) or as the pragma below for one unit. A unit whose target has *no* `extab` must
keep them off (`__init_cpp_exceptions.cpp` would *gain* extab its target does not have).

```c
#pragma exceptions on   /* the source spelling of the one flag, for a unit in a lib that has it off */
```

For a `$`-suffixed section (`.ctors$10`) the pragma **pair** is required: `#pragma section const_type ".ctors$10"`
to name it plus `__declspec(section ".ctors$10")` to place it - `__declspec` alone is rejected (error 33048), and
`code_type` would add a `.mwcats` section the target does not have.

**Demonstration.** `030-exception-settings-in-object.cpp` (`ideas.py demo-check 30`): a function with a local
object that has a destructor, compiled with the project's base flags (exceptions off) plus `#pragma exceptions on`,
gets `extab` 0x18 B and `extabindex` 0xC B; with `#pragma exceptions off` the object has neither. Both spellings
give the same 0x30 B of `.text`.

**When NOT to apply.** Exceptions enabled *with a `new` expression* also change `.text` (idea 62), so a missing
`extab` after a `new` is a source-shape problem, not just a flag. Idea 83 records the old "ruled-out" view of the flag
(it only adds the sections); read it with this one - its "does not change `.text`" holds only for code with no `new`
expression.

**Evidence** (batch 5, dated 2026-09-2x; the `Pl` and `main` libs now carry the flag, and the per-file pragma that
`sys_mem.cpp` and `fn_80040598.cpp` used to spell was removed as byte-identical once the lib flag existed).
`-Cpp_exceptions on` for the `Pl` lib left every `.text` byte unchanged and made all 12 extab entries we could
emit equal the target's; `#pragma exceptions on` in `sys_mem.cpp` turned its `operator new`/`operator delete`
`throw()` specs into the target's extab byte for byte; the same pragma on `Gecko_ExceptionPPC.cp` reproduced the
target's `extab 0x10` + `extabindex 0x18` exactly (it still carries it).
