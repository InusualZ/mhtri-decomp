---
id: 31
title: A function unpaired by name measures 0 %, not 60 %
status: works
problem: a unit's functions are written and their bytes are right, and the score stays near zero. The instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**.
tags: [measurement, symbols]
applies: []
demo:
---

# 31. A function unpaired by name measures 0 %, not 60 %

Problem: a unit's functions are written and their bytes are right, and the score stays near zero. The
instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**.

Why try it: a map name that does not match the name the object emits (a `fn_XXXXXXXX` placeholder, a renamed
function whose definition was not renamed with it, a C++ function whose mangled name the map spells
differently) leaves both sides unpaired, and an unpaired function contributes nothing at all. The unit's own
symbol table has the exact answer: the object says what it calls the function.

Result (batch 6): three of `Gecko_ExceptionPPC.cp`'s five functions measured 0 % while being byte-perfect -
the map called them `fn_80457504`/`fn_8045759C`/`fn_8045774C` and the object emitted
`ExPPC_FindExceptionFragment__FPcP12FragmentInfo`, `ExPPC_FindExceptionRecord__FPcP15MWExceptionInfo` and
`ExPPC_NextAction__FP14ActionIterator`. Three `symedit.py rename`s and **no source edit** took the unit from
9.82 % / 1 of 5 functions to ~99.5 % / 4 of 5 (the fifth is a preheader scheduling tie-break).

Example: read the name the object emits, then rename the map to it:

```sh
build/binutils/powerpc-eabi-nm.exe --defined-only build/RMHE08/src/main/<Dir>/<unit>.o | grep <addr>
python tools/symbols/symedit.py rename fn_80457504 ExPPC_FindExceptionFragment__FPcP12FragmentInfo --dry-run
```

The same mechanism runs the other way: dtk names a *target* object's relocation targets from the map, so a
target object split before a rename keeps the old name and looks like a mismatch - a plain re-split (or
`ninja build/RMHE08/obj/...`) refreshes it. Check the object's mtime against `symbols.txt`'s before believing
a reloc row, and never "fix" it by claiming the data range it points at.
