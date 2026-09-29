---
id: 49
title: `extab` in a no-exceptions lib is a cheap C++ language signal
status: works
problem: A unit's language has to be decided from evidence, not from convenience (`docs/plan.md`, "The language comes from the symbol"), because the extension decides the front-end (`-lang`) and the name objdiff pairs by. The two conclusive signals - a mangled definition, a `__FILE__` string that names a `.cpp` - may both be **absent** at the moment a unit is registered, so the `.c`/`.cpp` choice is a guess that a later pass has to undo (rows 42 and 48 are the two ends of getting it wrong).
tags: [symbols, allocator, sections, measurement]
applies: []
demo:
---

# 49. `extab` in a no-exceptions lib is a cheap C++ language signal

**Problem.** A unit's language has to be decided from evidence, not from convenience (`docs/plan.md`, "The
language comes from the symbol"), because the extension decides the front-end (`-lang`) and the name objdiff
pairs by. The two conclusive signals - a mangled definition, a `__FILE__` string that names a `.cpp` - may
both be **absent** at the moment a unit is registered, so the `.c`/`.cpp` choice is a guess that a later
pass has to undo (rows 42 and 48 are the two ends of getting it wrong).

**Why try it.** C has no exceptions. MWCC emits the `extab`/`extabindex` sections (the C++ exception and
unwind tables) only for a C++ translation unit, so **an object that carries those sections was compiled as
C++** - a cheap, mechanical signal read the moment the unit is split, before any source is written. Two
caveats make it honest. First, the **confound**: a lib whose `cflags` set `-Cpp_exceptions on`
(`cflags_pl`, `cflags_main`, `cflags_g3d`, `cflags_camellia`) makes a **C** unit emit `extab` too, so the
section is only evidence of C++ when the lib leaves the flag off (`tools/units/langcheck.py` resolves it
with `cflags_exceptions`). Second, it is **one-directional**: a C++ file with no `try`/`catch`/`throw`
emits no `extab`, so the section's *presence* is evidence and its *absence* is not - a unit without it must
not be inferred to be C.

**Result.** `langcheck` now reads the sections of the target object, threads the lib's exception setting in,
and reports an `extab` object in a no-exceptions lib as **suggested C++** (`medium`, `conclusive: False`)
beside the mangled-callee signal: it is listed with its reason, keeps the extension, and never renames on
its own; an exceptions-ON lib silences it. Measured on the registered tree (54 units; **43** objects carry
`extab`): **0** decisive candidates - every one of the 25 `.c` units with `extab` sits in `auto`/`main`/
`g3d`/`Camellia`, whose libs enable exceptions - 18 `.cpp` units consistent with it, and **2**
contradictions, `Runtime.PPCEABI.H/__ppc_eabi_init.cpp` and `__init_cpp_exceptions.cpp` (registered `.cpp`,
no `extab`, no mangled symbol: exactly the C++-without-exceptions the one-directional rule predicts, so not
evidence of C). The yield today is 0 units, and that is the honest number; the value is prospective - it is
decisive for the six no-exceptions libs (`Network`, `OS`, `RSO`, `Runtime.PPCEABI.H`,
`Runtime.PPCEABI.H/init`, `lobby`), where it catches a wrong extension the moment a new unit is registered.

**Example.**

```sh
python tools/units/langcheck.py --disagree       # the sweep, including the extab buckets
python tools/units/langcheck.py --unit RSO/runtime
```

The selftest pins the three properties that make the signal safe: an `extab` object in a no-exceptions lib
is C++/medium and **never conclusive**; a lib that enables `-Cpp_exceptions` silences it; and an object with
no `extab` is not evidence of C.
