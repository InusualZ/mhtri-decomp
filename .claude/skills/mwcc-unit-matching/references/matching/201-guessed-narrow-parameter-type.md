---
id: 201
title: A guessed narrow parameter type adds a callee-side extsh: read the retail caller for the conversion
status: works
problem: A member function's row has an `extsh`/`clrlwi` of an incoming parameter before a 32-bit compare that retail does not have, and the map's mangling (our own guess) says the parameter is `s16`/`u8`.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [57, 66, 185]
---

# 201. A guessed narrow parameter type adds a callee-side extsh: read the retail caller for the conversion

**Problem.** Ours sign-extends (or masks) an incoming parameter where retail uses the register as it arrives
(`extsh r0,r26; cmpw r3,r0` against `cmpw r0,r25`), every later use of the value takes a different register, and the
row sits a few points short. The mangled name in the map looks like evidence for the narrow type, but when the
project named the row itself it is only a guess.

**Why it happens.** Under `peephole off` MWCC re-narrows an `s16`/`u8` parameter before using it in 32-bit
arithmetic; with an `s32` parameter there is nothing to narrow. The caller side is decisive: a caller that passes an
`int` expression (`page * 8`) to an `s16` parameter must convert it (idea 57), so a retail caller with **no**
conversion before the `bl` proves the parameter is `s32`.

**How to work it.** Disassemble the retail caller (`objdump -dr build/RMHE08/obj/<caller>.o`) at the call; if the
argument reaches `r4..r10` unconverted, widen the parameter to `s32`, rename the map row to the new mangling
(`symedit.py rename --no-refs`; `s` -> `l`), re-split and re-measure the row and the caller's unit.

**Result.** `Network/network_pat_control` `copyCommunityViews` 95.82 -> 97.90 (`s16 first` -> `s32 first`, map row
`copyCommunityViews__9NetCtrlWkFP16NetCommunityViewllPs`), then 100 with the loop-increment and declaration order;
the caller `menu/menu_placeinfo` (`lb_community_select_step`) passes `page * 8` with no `extsh` and did not regress.

**Example.**

```
/* retail caller: lha r0,14(r30); slwi r4,r0,3; bl copyCommunityViews  - no extsh, so `first` is s32 */
s32 NetCtrlWk::copyCommunityViews(NetCommunityView* views, s32 first, s32 max, s16* total);
```
