---
id: 155
title: A member cleanup record retail lacks: define the callee before its caller (MWCC's nothrow knowledge)
status: works
problem: Our constructor or destructor carries an extab cleanup record (a member's destructor, an array's destroy, a base destructor) that retail's lacks, with byte-identical code - the call it guards is one MWCC could not yet prove nothrow.
tags: [source-shape, sections]
applies: []
demo: 155-nothrow-callee-before-caller.cpp
---

# 155. A member cleanup record retail lacks: define the callee before its caller (MWCC's nothrow knowledge)

**Problem.** Our constructor or destructor carries an `extab` cleanup record (a member's destructor, an array's
destroy, a base destructor) that retail's does not, while the `.text` is byte-identical. `extab` grows by
0x14-0x1C bytes per record and the unit cannot flip; nothing in the function body is wrong.

**Why it happens.** MWCC records a live member (or base, or constructed array) around every call it cannot
prove nothrow. It knows a callee cannot throw only when the callee is **already compiled** when the caller is:
a function defined earlier in the TU, an `inline` function defined before the caller (its out-of-line copy is
still *emitted* after its first caller, so the `.text` order does not change), or a declaration carrying
`__attribute__((nothrow))` - MSL declares its C library that way (`_MSL_CANT_THROW`). A `throw()` exception
specification does not help, and neither does `extern "C"`. An inline member defined after its caller, or a
plain re-declaration of `memset` the TU sees first, makes the call may-throw again.

**How to work it.**

* Read the record's range entry: its pc is the return address of the call MWCC thinks may throw (a range at pc
  0x28 is the `bl` at 0x24).
* If the callee is this TU's inline helper (retail emits it right after its first caller), define it `inline`
  **before** the caller, and keep the calls out of line with `#pragma dont_inline on` over the callers (idea 61).
* If the callee is an MSL routine (`memset`, `memcpy`), make sure the only declaration the TU sees carries
  `__attribute__((nothrow))` (`Runtime.PPCEABI.H/memset.h`, `memcpy.h`).
* A missing record is the mirror image: a base or member destructor the project models as a plain virtual
  (`destroy(s16)`) cannot be a cleanup action; only a real destructor is (with `#pragma peephole off`, MWCC's
  deleting destructor keeps retail's `extsh`+`cmpwi` of the flag).

**Result.** `Network/NetworkCommunity` extab 244 -> 216 B (target 216), `NetworkLayer` 388 -> 360 (360),
`NetworkSessionManager` 1084 -> 1036 (1036), `NetworkCommunityPat` 1564 -> 1544 (1544), `NetworkLayerPat`'s two
request records fixed, `sNetworkLibraryWii` 312 -> 292 (292, the `memset` nothrow) and `NetworkConnection` /
`GameSpyInterfaceThread` 128 -> 148 / 328 -> 348 (the real `~NetworkPeerBase`); no `.text` byte moved, the DOL hash
held, and `NetworkConnection` and `sNetworkLibraryWii` trial-flipped green.

**Example.**

```cpp
#pragma dont_inline on
inline void NetworkLayerRequest::reset() { /* ... */ }   /* compiled first: known nothrow */
NetworkLayer::NetworkLayer()
{
    /* ... */
    for (i = 0; i < 2; i++) { req->reset(); req++; }   /* no pool cleanup record */
}
```
