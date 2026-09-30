---
id: 62
title: A `new` expression is not `operator new` plus a constructor call
status: works
problem: Retail's allocation is `bl __nw__FUl; mr r31,r3; cmplwi r3,0` where ours lacks the `mr`, so everything after shifts - or `.text` is perfect but the target's `extab` record is bigger than ours.
tags: [source-shape, sections]
applies: [Network]
demo: 062-new-expression-extab.cpp
reviewed: 2026-09-29
related: [30, 48, 49, 59]
---

# 62. A `new` expression is not `operator new` plus a constructor call

**Problem.** A function that allocates and constructs is one instruction short - retail has
`bl __nw__FUl; mr r31,r3; cmplwi r3,0x0` where ours has `bl __nw__FUl; cmplwi r3,0x0` - and everything after
it shifts (`self` moves to another register, the frame grows). Or the reverse: `.text` is 100 % and the
unit's `extab` is smaller than the target's.

**How it looks.** `NetworkWiiMediator`'s `initializeNetworkMediator` measured 1.89 % and `reflectInit` 88.34 %;
the first divergence was that one missing `mr r31,r3` (`self` then landed in r29, `value` in r30, the frame
grew 0x10 -> 0x20). `extab`/`extabindex` are the exception-unwind tables the compiler emits per function that
needs cleanup; `flipcheck.py` reports a size mismatch on them.

**Why it happens.** With C++ exceptions on (`#pragma exceptions on`, or `-Cpp_exceptions on`) a **`new`
expression whose constructor is called** (`p = new T(...)`) must keep the pointer alive *across* the
constructor call, because the unwind path frees it if the constructor throws - so MWCC copies it to a
callee-saved register (`mr r31,r3`) and attaches a cleanup record to `extab`. The hand-written
`p = (T*)operator new(n); if (p != NULL) ctor(p);` has no such region: `p` lives in r3 and dies at the call
when nothing uses it afterwards, and no cleanup is recorded.

**How to work it.** Write the allocation as a real `new T(...)` (declare a constructor; add a padding member
so `sizeof` equals the size passed to `operator new`), with `#pragma exceptions on` at file scope when the
library is built `-Cpp_exceptions off` (as `Network` is - without the pragma MWCC emits **no** `extab`
section at all). Check three things: `.text`, `extab` **and** `extabindex`, and the call site's *relocation*
(the real constructor, not a synthesised `__ct__...`) - a flip is bytes and relocs.

**When NOT to apply.** The `mr r31,r3` only differs when the pointer is **dead after the constructor**: if
the function goes on using `p` (stores it, returns it), the manual form also keeps it in a saved register
and the two spellings compile alike (the demo's `expr_dead`/`manual_dead` pair isolates the dead case;
measured, `p` used afterwards gave the same code both ways). With exceptions **off** neither spelling emits
the copy in the dead case. So a register-off residual in an allocating function is a candidate only when the
target has `extab` and the pointer is otherwise dead.

**Result.** Measured at the time (2026-09): both functions reached 100 % (212/212 B and 116/116 B); the unit's
`.data`, `extab` and `extabindex` matched the target afterwards; unit 95.09 -> 98.89 % in one commit.

**Complement (the opposite tell).** `Network/constructNetworkWiiMediator` spelled the allocation
`operator new(0x1408)` plus a null check: the **same 16 instructions**, so `.text` measured 100 % and every
objdiff row was green, but the target's `extab` record is 24 bytes and ours an 8-byte header. `flipcheck.py`
caught it in one command (*"splits.txt claims extab (0x18) but the object emits no such section"*); a blind flip
would have dropped bytes and shifted everything after. Restoring the `new` expression (ctor inlined, pragma on)
reproduced `.text`, `extab` and `extabindex`; three spellings had been byte-identical in those sections, and
the decider was the call site's relocation. So the manual form is either a register off in `.text` **or** a
missing unwind record with `.text` perfect; only the second survives a `.text`-only measurement.

**Example.**

```cpp
#pragma exceptions on
Med* p = (Med*)operator new(sizeof(Med)); if (p) Med_init(p);   /* no mr r31,r3, no cleanup record */
new Med();                                                       /* mr r31,r3 + an extab record */
```

**Demonstration.** `062-new-expression-extab.cpp` (`ideas.py demo-check 62`): `new Med();` gets `mr r31,r3`
around the constructor call, the manual form has no `mr` at all, and the file carries a 0x20-byte `extab`
although the flags say `-Cpp_exceptions off` (the pragma turns it on).

**Evidence.** This corrects the "ruled out" note on `-Cpp_exceptions`: exceptions on/off is not only an `extab`
question - with a new-expression in the body it changes `.text`. `initializeNetworkMediator` 1.887 -> 100.00,
`reflectInit` 88.345 -> 100.00 (2026-09-27); the flip of the complement case landed as `e2f4ab40c`.
