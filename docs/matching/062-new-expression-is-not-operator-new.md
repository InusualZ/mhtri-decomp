---
id: 62
title: A `new` expression is not `operator new` plus a constructor call
status: works
problem: `NetworkWiiMediator`'s `initializeNetworkMediator` measured 1.89 % and `reflectInit` 88.34 %, and the first divergence was one instruction from the top: retail has `bl __nw__FUl; mr r31,r3; cmplwi r3,0x0` where ours had `bl __nw__FUl; cmplwi r3,0x0`. Everything after it shifted with it - `self` moved to r29, `value` to r30, the frame grew 0x10 -> 0x20 - so a function missing one instruction read as a function that was wrong.
tags: [measurement, source-shape]
applies: []
demo:
---

# 62. A `new` expression is not `operator new` plus a constructor call

**Problem.** `NetworkWiiMediator`'s `initializeNetworkMediator` measured 1.89 % and `reflectInit` 88.34 %, and the
first divergence was one instruction from the top: retail has `bl __nw__FUl; mr r31,r3; cmplwi r3,0x0` where ours
had `bl __nw__FUl; cmplwi r3,0x0`. Everything after it shifted with it - `self` moved to r29, `value` to r30, the
frame grew 0x10 -> 0x20 - so a function missing one instruction read as a function that was wrong.

**Why try it.** The unit compiles `#pragma exceptions on`. A manual
`T* p = (T*)operator new(n); if (p != NULL) ctor(p);` is coalesced: `p` lives in r3 and dies at the constructor.
A **`new` expression whose constructor is called** (`p = new T(...)`) must keep the value alive *across* the
constructor call, because the unwind path needs it, so MWCC spills it to a callee-saved register - that is the
`mr r31,r3`. Five spellings of the manual form were measured; none of them emits it.

**Result.** Both functions reached 100 % (212/212 B and 116/116 B), and the classes then needed a declared
constructor plus a padding member so that `sizeof` is the size passed to `operator new` (0xD640 / 0x816C / 0x44A0).
The unit's `.data`, `extab` and `extabindex` all matched the target afterwards.

**Floor.** This corrects the "ruled out" note on `-Cpp_exceptions`: exceptions on/off is not only an `extab`
question. With a new-expression in the body it changes **`.text`** - this instruction, and the register colouring
that follows from it - so a unit whose target has `extab` and whose allocating functions are a register off is a
candidate even where the sections already match.

**Example.** `Network/NetworkWiiMediator.cpp`: `initializeNetworkMediator` 1.887 -> 100.00, `reflectInit` 88.345 ->
100.00, unit 95.09 -> 98.89 % in one commit.


**Complement (2026-09-27): the same mistake, with the opposite tell.** `Network/constructNetworkWiiMediator`
was the mirror image of this row and it hid better. Its landed body spelled the allocation as
`operator new(0x1408)` plus a null check - and *that* spelling lowers to the **same 16 instructions**, so
`.text` measured **100.00 %** and every objdiff row was green. What differed was the object, not the code:
the target's `extab` record is 24 bytes and ours was an 8-byte header, because the cleanup MWCC attaches to
the ctor-call region of a real `new` expression has nothing to attach to in the manual form. `flipcheck.py`
caught it in one command (*"splits.txt claims extab (0x18) but the object emits no such section"*), and a
blind flip would have dropped 36 bytes and shifted everything after it. So the row cuts both ways: the
manual form is either a register off in `.text` (above) or a missing unwind record with `.text` perfect -
and only the second case survives a `.text`-only measurement. Two other facts from that measurement: the
lane compiled **three** spellings that were byte-identical in `.text`, `extab` *and* `extabindex`, so the
deciding evidence was the call site's **relocation** (the real callee, not a synthesized `__ct__…`); and
the `Network` library sets `-Cpp_exceptions off`, so a file-scoped `#pragma exceptions on` is *required* -
without it MWCC emits no `extab` section at all. The flip landed (`e2f4ab40c`), the 33rd.