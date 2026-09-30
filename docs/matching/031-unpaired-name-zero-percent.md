---
id: 31
title: A function unpaired by name measures 0 %, not 60 %
status: works
problem: a unit's functions are written and their bytes are right, and the score stays near zero. The instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**.
tags: [measurement, symbols]
applies: []
demo:
reviewed: 2026-09-29
related: [42, 48, 50]
---

# 31. A function unpaired by name measures 0 %, not 60 %

**Problem.** A unit's functions are written and their bytes are right, and the score stays near zero. The
instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**.

**How it looks.** In the unit's objdiff view the same function appears twice, once only on the target side
(`fn_80457504`) and once only on ours (`ExPPC_FindExceptionFragment__FPcP12FragmentInfo`), each with no match.
An unpaired function contributes nothing to the unit percentage, so the unit reads far lower than its bytes.
The report row for a function with no `fuzzy_match_percent` key is 0 %, not "unknown".

**Why it happens.** Both sides are ELF objects and objdiff looks a function up by its symbol name. A map name that
does not match the name the compiled object emits - a `fn_XXXXXXXX` placeholder, a renamed function whose
definition was not renamed with it, a C++ function whose mangled name the map spells differently - leaves both
sides unpaired. The unit's own symbol table has the exact answer: the object says what it calls the function.

**How to work it.** Read the name the object emits and rename the map to it (a rename is two edits, the map row and
the source; here the source was already right, so only the map row moves):

```sh
build/binutils/powerpc-eabi-nm.exe --defined-only build/RMHE08/src/<Dir>/<unit>.o | grep <addr>
python tools/symbols/symedit.py rename fn_80457504 ExPPC_FindExceptionFragment__FPcP12FragmentInfo --dry-run
```

The same mechanism runs the other way: dtk names a *target* object's relocation targets from the map, so a target
object split before a rename keeps the old name and looks like a mismatch - a plain re-split (or
`ninja build/RMHE08/obj/...`) refreshes it. Check the target object's mtime against `symbols.txt`'s before
believing a reloc row, and never "fix" it by claiming the data range the reloc points at.

**When NOT to apply.** If the two names already agree and the function still scores low, pairing is not the
problem - read the first divergence (idea 2). For a C++ unit whose map names are unmangled see 42 and 48; for a
map name that is already a true C++ mangling see 50.

**Result.** Measured in batch 6: three of `Gecko_ExceptionPPC.cp`'s five functions measured 0 % while being
byte-perfect - the map called them `fn_80457504`/`fn_8045759C`/`fn_8045774C` and the object emitted
`ExPPC_FindExceptionFragment__FPcP12FragmentInfo`, `ExPPC_FindExceptionRecord__FPcP15MWExceptionInfo` and
`ExPPC_NextAction__FP14ActionIterator`. Three `symedit.py rename`s and **no source edit** took the unit from
9.82 % / 1 of 5 functions to ~99.5 % / 4 of 5 (the fifth is a preheader scheduling tie-break).

**Demo.** None: this is a measurement/bookkeeping idea about the map, not a codegen shape one object can show.

**Evidence.** Unit `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp`; the numbers are dated (batch 6) and the unit's
functions have since been named.
