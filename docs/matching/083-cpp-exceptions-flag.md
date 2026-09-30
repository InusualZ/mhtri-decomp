---
id: 83
title: `-Cpp_exceptions`
status: superseded
problem: The presence of `extab`/`extabindex` in the target suggests exceptions were on; the flag adds those sections - but the old claim that it never changes `.text` is wrong once a `new` expression is in the body.
tags: [flags, sections, source-shape]
applies: [Wii/1.3]
demo: 083-cpp-exceptions-flag.cpp
superseded_by: 62
reviewed: 2026-09-29
related: [30, 49, 62]
---

# 83. `-Cpp_exceptions`

**Problem.** `extab`/`extabindex` (the unwind-table sections that C++ exception handling adds) present in a target
object suggests the unit was built with exceptions on, and a "ruled out" note recorded that turning the flag on
"only adds those sections and does not change `.text`". That is true for a function without an allocation and
false for one with a `new` expression - so the flag was treated as a pure section-size lever when it is also a
codegen lever.

**How it looks.** The unit's sections are wrong (`extab`/`extabindex` missing or the wrong size) *or* an allocating
function is a register off: the target has `bl __nw__FUl; cmpwi r3,0; mr r31,r3` where ours has no `mr`
(idea 62's first divergence).

**Why it happens.** With exceptions on, a `new T` whose constructor is called must keep the new pointer alive
across the constructor so the unwind path can free it; MWCC spills it to a callee-saved register (`mr r31,r3`) and
emits an `extab` record for the cleanup region. With exceptions off nothing is emitted and the value dies in r3.

**How to work it.** Read the target object's sections (`python tools/elf/elfsect.py <obj>`): `extab`/`extabindex`
present means the unit needs `-Cpp_exceptions on` (the project sets it per library, e.g. `cflags_network`, never in
`cflags_base`, which says `off`; a file-scoped `#pragma exceptions on` is the alternative and is *required* in a lib
built `off`). Then follow idea 62 for the source shape (`new` expression, not `operator new` plus a ctor call) and 30
for the section-completeness side.

**Demonstration.** `083-cpp-exceptions-flag.cpp` (`ideas.py demo-check 83`) compiles the same two functions under
`-Cpp_exceptions on`: `extab` is 0x20 bytes, `make_new_expr` (`new T;`) keeps the pointer with `mr r31,r3` and
`make_manual` (`operator new` + null check + a plain ctor call) has no `mr`. With the flag `off` (checked by hand
2026-09-29, same source) there is no `extab`/`extabindex` section at all, no `mr`, and `.text` is 0x68 bytes against
0x78 for `on`: the flag moved `.text`, not just the section list.

**Result.** Superseded by idea 62 (which recorded the correction) and refined by 30 and 49: the flag is not a
ruled-out axis - it is the unit's exception setting, read from the target's `extab` presence.

**Evidence.** The original entry read "It only adds `extab`/`extabindex` sections (needed at the end for a full match)
and does not change `.text` here" and was kept ruled-out until `Network/NetworkWiiMediator` showed otherwise (62).
`configure.py` now carries `-Cpp_exceptions on` for the Network lib and other libs whose targets carry `extab`,
each with its own evidence comment (flags audit 2026-09-28).
