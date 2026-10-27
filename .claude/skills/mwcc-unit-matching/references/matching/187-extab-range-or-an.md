---
id: 187
title: An extab range or an 8-aligned vtable that only a merged object reproduces names a false TU seam
status: works
problem: Every function of a unit is instruction-identical, yet one extab range starts a call too early and a vtable lands 4 bytes late in .data, so the unit can never flip.
tags: [sections, linker]
applies: [Wii/1.3]
demo: 
related: [80, 94]
---

# 187. An extab range or an 8-aligned vtable that only a merged object reproduces names a false TU seam

**Problem.** `Runtime.PPCEABI.H/ExceptionPPC` reached 9/9 functions at 100 %, but the trial flip moved the DOL:
`ExPPC_UnwindStack`'s `#pragma exception_terminate` range read `00000044 012B` against retail `00000054 0125`,
and `__vt__Q23std13bad_exception` linked at 0x8060E92C instead of 0x8060E928 (with the `what()` string 4 bytes
late after it).

**Why it happens.** Both are properties of the *whole* object. MWCC leaves a call out of a terminate range only when
it can see the callee cannot throw - i.e. the callee is defined earlier in the same TU (`ExPPC_FindExceptionRecord`,
in `Gecko_ExceptionPPC.cp`); a `throw()` on the declaration does not do it. And MWCC 8-aligns the vtable inside the
object's `.data`, which only lands on retail's address when the section starts where retail's object started
(0x8060E8A0, the other unit's jump table first), not at the registered seam 0x8060E8E4.

**How to work it.** Compile the two units' sources concatenated (a scratch file, the unit's real command line) and
compare the merged object with retail: here the extab entry and all `.data` offsets match exactly. That is seam
evidence - report a fold (`docs/pipeline.md` seam request), do not chase it in the source.

**Result.** Merged scratch object: extab `00000054 0125`, vtable at +0x88 (= 0x8060E928), base list +0x98, string
+0xA4 - all retail. Split object: two DOL diffs remain whatever the source says.

**Example.**

```
python .pi/tmp/<slug>/merge.py   # cat Gecko_ExceptionPPC.cp ExceptionPPC.cp > merged.cp, compile with mt.py info's line
objdump -s -j extab merged.o     # compare the entry with the retail bytes
```
