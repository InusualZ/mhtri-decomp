---
id: 231
title: A 0xFFFF halfword store built as lis 1 + subi 1 means the field is u16, not s16
status: works
problem: retail stores 0xFFFF with `lis r3,0x1` + `subi r0,r3,0x1` + `sth`, ours with `li r0,-0x1` + `sth`
tags: [source-shape]
applies: []
demo: 
---

# 231. A 0xFFFF halfword store built as lis 1 + subi 1 means the field is u16, not s16

**Problem.** Retail materialises a halfword sentinel as the 32-bit value 65535 (`lis r3,0x1; subi r0,r3,0x1`) before
its `sth`, while ours folds the same store to `li r0,-0x1`: one instruction short and every later offset shifted.

**Why it happens.** MWCC converts the constant to the destination type before it picks the load. Stored into an `s16`
field, 0xFFFF becomes -1, which fits `li`'s signed 16-bit immediate; stored into a `u16` field it stays 65535, which
does not, so the compiler builds it with `lis`/`subi`.

**How to work it.** Make the field (or local) `u16`. When the field's address is handed to a callee taking an untyped
or differently typed pointer, nothing else changes; check every reader of the field for a changed sign extension.

**Result.** `menu/menu_item` `menu_work_update`: `MenuSlot::ticket` `s16` -> `u16`, +6.4 matched bytes (99.398 ->
99.732 % with the two exit fixes of the same pass), no other object changed (only this unit names the field).

**Example.**

```
struct Slot { u16 ticket; };          /* s16 gives li r0,-1 */
void f(struct Slot* s) { s->ticket = 0xFFFF; }   /* lis r3,1; subi r0,r3,1; sth r0,0(r3) */
```
