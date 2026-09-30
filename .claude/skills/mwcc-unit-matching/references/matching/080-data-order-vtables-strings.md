---
id: 80
title: A TU's `.data` is globals, strings, vtables in reverse, then inline-function strings - two vtable groups with strings between them are two TUs
status: works
problem: `Network/network_transport` converted its peer types to real classes so the compiler would emit their vtables, and every table came out at the right size - yet the unit's `.data` section scored 10.4 % of 4108 B. The retail run `0x805F94E0..0x805FA4EC` interleaves vtables with strings, and no source spelling of one TU produces that order. It reads as a data-claim or class-model problem and invites more class rewrites.
tags: [data, vtable]
applies: [Network, Wii/*, GC/3.0a3]
demo: 080-data-order-vtables-strings.cpp
---

# 80. A TU's `.data` is globals, strings, vtables in reverse, then inline-function strings - two vtable groups with strings between them are two TUs

**Problem.** `Network/network_transport` converted its peer types to real classes so the compiler would emit their
vtables, and every table came out at the right size - yet the unit's `.data` section scored 10.4 % of 4108 B. The
retail run `0x805F94E0..0x805FA4EC` interleaves vtables with strings, and no source spelling of one TU produces
that order. It reads as a data-claim or class-model problem and invites more class rewrites.

**Why it happens.** MWCC lays out one TU's `.data` as: initialised globals over 8 B in definition order; the strings
of out-of-line functions in first-use order (identical literals merge under `-str reuse`); **vtables in the reverse
of class order**; then the strings of **inline** functions - in-class bodies and free `inline` functions, one
unmerged copy per instance (the "inline tail"). Small (at most 8 B) objects go to `.sdata` and const tables to
`.sdata2`. Measured on all nine `Wii/*` compilers and `GC/3.0a3`..`3.0a5.2`; `GC/1.0`..`2.7` interleave each vtable
with inline strings. The linker concatenates TU fragments, so in retail `.data`: two vtable groups with strings
between them are two TUs (the boundary is in the gap, after any inline tail), and two adjacent vtables whose
owners' first code slots go *up* in address are two TUs (inside one TU they descend). **A vtable followed by strings
is not by itself a seam** - it may be the inline tail (a first version of this row said it was, and eight g3d seams
and one `ef` seam, all inline asserts naming `*_ac.h` or `particle.h`, proved it wrong).

**How to work it.** Classify the run's symbols (vtable: leading `0,0` header and code pointers; string:
printable NUL-terminated; jump tables are not vtables) and look for strings between two vtable groups and "up"
vtable pairs before claiming or reconstructing it (`python tools/splits/dataorder.py at <addr>`,
`docs/data-order-seams.md`). A bare header name (`x_ac.h`) or an unmerged repeated literal right after a vtable is
an inline tail. Cut the claim at the seams: one claim per TU fragment.

**Result.** The whole DOL has 231 vtables and 65 strings-between-vtable-groups seams: 4 inside registered units (all
in `network_transport`) and 61 in unclaimed `.data` (23 with a gap of at most 8 symbols), plus 58 "up" adjacent
pairs there - candidate seams for proposals no tool could cut (`python tools/splits/dataorder.py scan`).

**Example.** `network_transport`: gaps at 0x805F9570, 0x805F9610 and 0x805F9958 (a vtable follows each) split the
unit's `.data` into per-class TUs; 0x805F9A40 follows the last vtable and may be an inline tail.

**Demonstration.** `080-data-order-vtables-strings.cpp` (`ideas.py demo-check 80`) reproduces the order on Wii/1.3: the two
initialised globals, then the out-of-line strings, then `__vt__1B` before `__vt__1A` (reverse class order), then the
inline function's string; strings of 8 B or less go to `.sdata` and stay out of the run.
