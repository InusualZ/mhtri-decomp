---
id: 246
title: lbz + extsh + sth with no clrlwi: a (u16) byte stored into an s16 field
status: works
problem: retail stores a u8 field into a halfword as lbz, extsh, sth and no cast of the byte reproduces the extsh
tags: [source-shape]
applies: [quest/quest_entry]
demo: 
---

# 246. lbz + extsh + sth with no clrlwi: a (u16) byte stored into an s16 field

**Problem.** Retail copies a byte into a 16-bit field as `lbz r0` / `extsh r0,r0` / `sth r0`. Into a `u16` field every
cast chain of the byte (`(s16)b`, `(s16)(u16)b`, an `s16` local) either drops the `extsh` or adds a `clrlwi r0,r0,16`
after it.

**Why it happens.** Under `-opt nopeephole` MWCC keeps the conversion it was asked for. Storing a `u16`-typed value
into an `s16` field is a narrowing conversion to `s16`, which it emits as `extsh`; a store into a `u16` field ends in
a `u16` conversion (`clrlwi ...,16`) instead, and a plain byte into an `s16` field is already in range (no `extsh`).

**How to work it.** Make the destination field `s16` and write the value as `(u16)byte`. If the field lives in a shared
header, prove every other includer's object byte-identical before and after (snapshot the `.o` files, rebuild, compare).

**Result.** quest/quest_entry: `Q_ItemPair.num` `u16` -> `s16` with `out->num = (u16)entry->value;` took
quest_lot_pick / _first / _last from 98.2-98.6 % to 100 % (and quest_result_deliver_items 98.36 -> 99.30); the five
other includers' objects stayed byte-identical.

**Example.**

```
struct P { u16 id; s16 num; };
void f(const E* e, P* o) { o->num = (u16)e->value; }   /* lbz, extsh, sth */
```
