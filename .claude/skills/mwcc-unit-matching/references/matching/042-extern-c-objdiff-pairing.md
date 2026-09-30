---
id: 42
title: A C++ free function needs `extern "C"` so objdiff can pair it by name
status: works
problem: A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs.
tags: [symbols, measurement]
applies: [Wii/1.3]
demo: 042-extern-c-objdiff-pairing.cpp
reviewed: 2026-09-29
related: [31, 48, 50]
---

# 42. A C++ free function needs `extern "C"` so objdiff can pair it by name

**Problem.** A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the
free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs and the
function contributes nothing.

**How it looks.** The report has both `fn_80073398` (target only) and `fn_80073398__FP9ResHandle` (ours only), each
unmatched; the same source compiled as a `.c` file pairs fine. This is the source half of idea 31.

**Why it happens.** A C++ front end encodes the parameter list into a free function's symbol (`__F` + argument
codes). The map names the address `fn_80073398`, so the names never agree. `extern "C"` gives the definition C
linkage: the emitted symbol is the plain identifier.

**How to work it.** Wrap the `fn_*` definitions in `extern "C"`:

```cpp
extern "C" void fn_80073398(ResHandle* self) { ... }
```

**When NOT to apply - and how this idea sits with idea 48.** Two ways to make the names agree exist, and they pull in
opposite directions. 42 changes the *source* (give the definition C linkage); 48 changes the *map* (rename the
`fn_XXXXXXXX` row to the mangled spelling the C++ definition emits, `tools/units/mangle.py` derives it). Choose by
what the original was: if the original function had C linkage (a C unit, or a hand-written `extern "C"`), the map
name is right and `extern "C"` is the correct source (this idea); if the original was ordinary C++ - retail names
are then mangled, and the map's unmangled `fn_` is only dtk's fallback for a name it could not demangle - the map
is under-specified and 48's rename is the honest fix, and it is the only fix for a member function, which cannot be
`extern "C"` at all. The demo pins the compiler half of both: the `extern "C"` definition emits the plain
identifier, the ordinary one a `__F...` mangling. And for a name that is already a true mangling in the map, never declare it as an
identifier (idea 50). If the unit is C++ only because of an `extab` (idea 49), check whether `extern "C"` is
consistent with the unit's language evidence.

**Result.** Measured at the time on `auto/80073398_fn_80073398` (since retired): `extern "C"` on the `fn_*`
definitions took every `fn_*` symbol from 0 to 100 (MWCC mangles a C++ free function as
`fn_80073398__FP9ResHandle`, which objdiff cannot pair).

**Demonstration.** `042-extern-c-objdiff-pairing.cpp` (`ideas.py demo-check 42`): the `extern "C"` definition is
the exact symbol `fn_plain_c` (no `fn_plain_c__Fi`), the plain one is `fn_mangled__Fi` (verified 2026-09-29).
