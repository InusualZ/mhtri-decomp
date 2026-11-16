---
id: 322
title: A nested u16 call argument is copied into r0 before its narrowing: hold it in a u32 local
status: works
problem: A u16-returning call passed as the last argument of another call reads `mr r0,r3 ... clrlwi r5,r0,16` in ours where retail narrows straight from the return register (`clrlwi r5,r3,16`, then the other arguments)
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [161, 261]
---

# 322. A nested u16 call argument is copied into r0 before its narrowing: hold it in a u32 local

**Problem.** `item_slots_room_get(id, &block->slots, userdata_box_capacity(block))` (a `u16` return passed to a `u16`
parameter, last argument evaluated first, idea 161) compiles to `bl capacity ; mr r0,r3 ; clrlwi r3,r31,16 ; lwz r4 ;
addi r4 ; clrlwi r5,r0,16 ; bl room` under `#pragma peephole off`; retail has `bl capacity ; clrlwi r5,r3,16 ;
clrlwi r3,r31,16 ; lwz r4 ; addi r4 ; bl room`. One extra `mr` and the narrowing scheduled after the other arguments
(4 B, 93.3 % on a 96 B row).

**Why it happens.** The return value of a `u16` call is a value already of the parameter's type, so the frontend keeps it
as a copy in a scratch temporary and re-narrows it at the call; an `int`-typed value is converted once, where it is
produced.

**How to work it.** Hold the nested call's result in a `u32` local and pass the local:
`u32 cap = userdata_box_capacity(block); return item_slots_room_get(id, &block->slots, cap);`. The conversion to the
`u16` parameter is then a plain narrowing of a wide value, emitted at its producer.

**Result.** `ef/eft052` (Wii/1.3, peephole off): `eft052_page_count_ck` 93.33 -> 100 %; `peephole on` around the
function measured 83.96.

**Example.**

```
u32 cap = userdata_box_capacity(lobby_world_block);
return item_slots_room_get(id, &lobby_world_block->field_0x0180, cap);
```
