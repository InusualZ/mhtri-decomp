---
id: 235
title: Under peephole off, a redundant (s16) cast on a stored s16 parameter keeps retail's extsh
status: works
problem: Retail stores an s16 parameter through `extsh r0,rN ; sth r0,..` where ours stores the saved register directly (`sth rN,..`), one instruction short.
tags: [source-shape, pragma]
applies: [Wii/1.3]
demo: 
related: [95, 168]
---

# 235. Under peephole off, a redundant (s16) cast on a stored s16 parameter keeps retail's extsh

**Problem.** A setter copies its `s16` parameters into a record's `s16` fields. Retail extends one of them again
before the store (`extsh r0,r26 ; sth r0,0xa(r23)`); ours stores the parameter's saved register as it is, so the row
is one instruction short and every later instruction shifts.

**Why it happens.** The parameter is already sign-extended, so the conversion is a no-op the front end drops - unless
the source spells it. Under `#pragma peephole off` an explicit `(s16)` cast survives as its own `extsh`, exactly like
the narrowing conversions of idea 95 that the peephole would otherwise delete.

**How to work it.** In a `peephole off` unit, when retail has an `extsh` immediately before the `sth` of an `s16`
parameter or local and ours has none, write the store as `field = (s16)param;`. Re-measure the row; nothing else
moves.

**Result.** `lobby/fn_80212810` `lb_list_init` (2026-10-27): `list->str_base_0x0A = (s16)str_base;` 97.31 -> 100 %
(+4.2 matched bytes), the other eight stores of the setter unchanged.

**Example.**

```
void init(LbList* list, s16 count, s16 str_base) {
    list->count = count;              // sth r27,..   (no extsh)
    list->str_base = (s16)str_base;   // extsh r0,r26 ; sth r0,..
}
```
