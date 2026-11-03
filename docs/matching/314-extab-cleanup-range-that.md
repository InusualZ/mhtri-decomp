---
id: 314
title: An extab cleanup range that ends before an MSL call: declare the callee __attribute__((nothrow))
status: works
problem: .text is 100 % but an extab range's u16 length is longer than retail's - retail's cleanup range for a live local object stops before a call to an MSL C function (memmove) and ours covers it.
tags: [source-shape, sections]
applies: [Wii/1.3]
demo: 
related: [155, 308]
---

# 314. An extab cleanup range that ends before an MSL call: declare the callee __attribute__((nothrow))

**Problem.** Every `.text` row matches, but `extab` differs in a range entry's 16-bit length word: retail
`00 00 00 44 00 11 00 10` against ours `00 00 00 44 00 1b 00 10`. The range is the cleanup range of a local
object with a destructor, and retail's ends just before a `bl memmove` that ours includes. When the call
is the only thing a range would hold, retail writes the range with length 0 and ours writes a real length.

**Why it happens.** MWCC leaves out of a cleanup range any call it knows cannot throw. For a function
defined earlier in the TU it works that out from the body (idea 155). For an external C function it knows
only what the declaration says. MSL's `string.h` declares `memmove` with `_MSL_CANT_THROW`, which is
`__attribute__((nothrow))`. Our plain prototype makes the call one that may throw. `throw()` on the
declaration **does not** have the same effect. Only the attribute does.

**How to work it.** Find the extab entry's function from `extabindex` (`{fn, size, etb}` records), take
the range's start and length (in words) from the bytes, and see which `bl` falls in our range but not in
retail's. If the callee is an MSL/SDK C function, add `__attribute__((nothrow))` to its prototype in the
owner's header. Then prove with `objsame.py --base-tree main` that no other object changes.

**Result.** `Network/NetworkUnitPacket` (Wii/1.3, `-O3`, `Cpp_exceptions on`): extab went from 2 of 608 bytes
differing to byte-identical. Two ranges were fixed: `networkStreamQueue_discard` (0x1B -> 0x11 words) and
`networkStreamQueue_removeMessage` (0x0A -> 0). flipcheck READY, trial flip `main.dol: OK`. `objsame
--base-tree main`: 675 of 676 objects identical (the one that differs is this unit's own source change).

**Example.**

```
void* memmove(void* dst, const void* src, u32 n) __attribute__((nothrow));   /* MSL _MSL_CANT_THROW */
```
