---
id: 210
title: Under peephole off, a 0/1 byte stored or passed with no clrlwi is a bool
status: works
problem: A peephole-off unit masks a flag with clrlwi r0,rX,24 before stb (or before passing it) where retail stores or passes the register raw, and no u8/u32/cast spelling removes it
tags: [source-shape, pragma]
applies: [Wii/1.3]
demo: 
related: [95, 38, 66, 201]
---

# 210. Under peephole off, a 0/1 byte stored or passed with no clrlwi is a bool

**Problem.** In a unit compiled `#pragma peephole off`, a flag byte comes out as `clrlwi r0,r3,24 ; stb r0,...`
where retail has `stb r3,...`: after a call (`flag[i] = isReady()`), after a comparison
(`host = role == 1` is `cntlzw ; srwi ; stb`), or for a byte parameter compared and stored
(`cmplw r28,r0 ; stbx r28,...`). The same mask shows up before a call that takes the byte
(`mr r5,r30` in retail, `clrlwi r5,r30,24` ours). `u8`/`u32` parameters, `u8` locals, casts and
`peephole on` around the function all leave a mask or move other instructions.

**Why it happens.** With the peephole off, MWCC keeps every narrowing conversion to `u8` as an
instruction - also the one on a `u8` return value or parameter, which it does not trust to be extended.
A `bool` is never narrowed: a `bool` return, comparison result, parameter or field is already 0/1, so
the store and the argument pass the register as it stands. The retail source used `bool` for these flags.

**How to work it.** When the masked value is a yes/no flag, type the whole chain as `bool`: the field,
the function that returns it (return types are not in the mangling), the local that holds it, and the
parameter that carries it (the parameter type *is* in the mangling: `Uc`/`Ul` -> `b`, a map rename per row,
`symedit.py rename-batch`). A chain stops at the first `u8` link, so follow it through every caller and
callee and check each owner's rows (`objsame --base-tree <base> --renames-from-git <base>` proves the
other objects unchanged but for the renames).

**Result.** `Network/NetworkLayerPat` (Wii/1.3, file-scope `peephole off`): `applyLoginRecord`
97.73 -> 100 (`bool hostMode_4C4`), `addFriendSlot` 98.55 -> 100 and `setFriendTransferModeById`
97.50 -> 100 (`bool` friend-flag arrays, `bool isVoiceReady`/`isAcceptedPeer`/`isFriend`, a `bool mode`
parameter), `setFriendTransferMode_E0` 99.09 -> 100 (`bool mode` through `setTransferSlotMode`,
`openTransferSlot`, `setFlag76` and the transfer slot's `mode` field), `move` 98.69 -> 98.98.
`Network/NetworkCommunityPat`'s callers lost their masks too (`handleUnblockPlayer` 99.75 -> 100).

**Example.**

```
u8   hostMode;   hostMode = role == 1;   // subi ; cntlzw ; srwi ; clrlwi ; stb
bool hostMode;   hostMode = role == 1;   // subi ; cntlzw ; srwi ; stb
```
