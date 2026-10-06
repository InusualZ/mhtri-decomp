---
id: 102
title: A signed byte's register pair names its type: in-place extsb is an s8 field, lbz r0 + extsb is a cast from u8
status: works
problem: A byte read that feeds a signed value differs only in registers - retail `lbz r4; extsb r4,r4` against ours `lbz r0; extsb r4,r0`, or the reverse - and no allocator lever moves it.
tags: [source-shape, allocator]
applies: [Network]
demo:
---

# 102. A signed byte's register pair names its type: in-place extsb is an s8 field, lbz r0 + extsb is a cast from u8

**Problem.** A byte field read as a signed value shows a one- or two-operand register difference: retail sign-extends
in place (`lbz r4,X(r3); extsb r4,r4`) while ours loads into `r0` first (`lbz r0; extsb r4,r0`), or the mirror image.
Declaration order, temporaries and pragmas do not move it.

**Why it happens.** Under MWCC, reading an `s8` lvalue is one value: the load and its extension share a register. A
`u8` lvalue converted to `s8` (a cast, an `s8` parameter, an `s8` local) is two values - the zero-extended byte and
its conversion - and the byte gets its own scratch register (`r0`).

**How to work it.** Read the pair: in-place `extsb` means the field is `s8` and the use has no cast; the `r0` pair
means the read converts from `u8`. Retype the field to the in-place shape and drop the `(s8)` casts at its uses; where
retail still shows the `r0` pair on an `s8` field, spell that read `(s8)(u8)field`.

**Result.** `Network/NetworkSessionStable`: `ownIndex_14826` from `u8` to `s8` with its `(s8)` casts removed takes
`leave` 98.89 -> 100, `put` 99.80 -> 100, `init` 96.61 -> 98.13 (plus `move`, `execControlOne`, `moveOutOfBand`
up); `getOwnIndex` stays 100 with `(s8)(u8)ownIndex_14826`, and `getUsableSlot` 99.77 -> 100 with
`(s8)(u8)relayIndex_10` on an `s8` field. `Network/NetworkConnectionStable::getIndex` 96.67 -> 100 with
`(s8)(u8)index_20A4`.

**Example.**

```c
s8 index;                     /* field */
f(index);                     /* lbz r4; extsb r4,r4 */
return (s8)(u8)index;         /* lbz r0; extsb r3,r0 */
```
