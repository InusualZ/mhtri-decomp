---
id: 113
title: bss symbol order follows first use in emitted code, so a unit can link to a different DOL while every row is 100 percent
status: works
problem: every row is 100 percent and flipcheck says READY, yet the flip moves the DOL hash because two .bss objects swapped places
tags: [data, sections, measurement]
applies: []
demo: 
---

# 113. bss symbol order follows first use in emitted code, so a unit can link to a different DOL while every row is 100 percent

**Problem.** `TRK/gdev_cc` has all 10 rows at 100 % and `flipcheck` reads READY, but flipping it fails the DOL hash:
`ninja diff` reports `Expected to find symbol @eti_8001E558`. Our object has the 0x20 B circle buffer at `.bss+0` and the
0x500 B receive buffer at `+0x20`; the target has them the other way round.

**Why it happens.** MWCC lays out zero-initialised globals in the order the code first references them, not in definition
order and not by name or size. `init(&circ, buf, n)` places `circ` first whatever order the two are defined in, and a function
that reads `buf` before `circ` places `buf` first. The section sizes and the code match, so no per-row tool sees it.

**How to work it.** After a flip candidate reads READY, compare the symbol order inside `.bss`/`.sbss` of
`build/RMHE08/src/<unit>.o` with the target object (`objdump -t`). To reorder, change which symbol the first emitted
reference touches (a read of the intended-first object ahead of the call); a bisect of the flips (one unit at a time through
`ninja build/RMHE08/ok`) finds the culprit when the order is not looked at.

**Result.** Measured on `TRK/gdev_cc`: reordering definitions, renaming the symbols and hoisting the buffer into a local
all left the order unchanged (circle buffer first), so the unit stays NonMatching with the residual recorded in its header.
The scratch test (`init(&circ, buf, 0x500)` with `buf` defined first) reproduces the circle-first layout.

**Qualification (re-measured on `TRK/gdev_cc` with the `.sbss` object fixed).** The first diagnosis blamed the `.bss` order alone, but
the same `ninja diff` line (`Expected to find symbol @eti_8001E558`) is printed for **any** layout defect, and the unit also had a
second one: its `.sbss` open-state object is 8 B in the target while the source declared only the 4 B word the code reads, so the
linker dead-stripped the unreferenced half and shifted the following addresses (`mwlink_debugger.py trace <unit>` shows the `.sbss`
size; modelling the object as a struct with an `unused_0x04` word fixes it, idea 116). With that fixed the flip **still** fails the
hash while `trace` shows `gdev_cc_circle_buffer .bss+0x0` and `gdev_cc_buffer .bss+0x20` (target: `+0x500` and `+0x0`), so the order
is a real, separate defect; but it is not "first use" alone: declaring the circle buffer as a plain `u8[0x20]` array, defining it
after the receive buffer, `static`, a local alias and an enclosing struct all leave the small object first (the layout looks
size-ascending for these two). Read `trace` for **every** section of a unit that reads READY before attributing the hash to one of
them, and fix the byte-less sections (`.sbss`/`.bss` object sizes) before spending time on their order.

**Example.**

```
unsigned char buf[0x500];
S circ;
void f(void) { init(&circ, buf, 0x500); }   /* .bss: circ at +0, buf at +0x20 */
```
