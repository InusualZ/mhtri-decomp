---
id: 214
title: A global whose address stays in a saved register all function long is read through a pointer local
status: works
problem: Retail forms a global's address once (`lis`/`addi` into a saved register) and every later field access goes through it, while ours re-forms `lis`/`addi` before each late access and saves one register fewer.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [136, 20]
---

# 214. A global whose address stays in a saved register all function long is read through a pointer local

**Problem.** Retail's prologue saves one more register than ours (`_savegpr_27` against four `stw`s), the global's
address is formed once at the top (`lis r3,g@ha; addi r31,r3,g@l`) and every later store (`stb r0,0x145(r31)`)
goes through r31. Ours keeps the address only while the first use needs it and re-forms `lis`/`addi` before each
later access, so the function is longer by two instructions per access and every saved register is renumbered.

**Why it happens.** A direct `g.field` is a fresh address expression at each use; MWCC CSEs it only inside a
region, so the copies after a call are rebuilt. A pointer local (`T* p = &g;`) is one variable that lives across
the whole body, so the allocator gives it a callee-saved register. This is the counterpart of idea 136, where
retail rebuilds the address and the pointer local is the wrong shape.

**How to work it.** Read the diff for an `addi rN,r3,g@l` into a saved register at the top and later `lis`/`addi`
pairs on our side only. Declare `T* p = &g;` as the first local and spell every access in the function `p->field`.
Check the function's own uses only: a sibling that rebuilds the address keeps the direct form (idea 136).

**Result.** `lobby/lb_npc` (Wii/1.3, -O4 by pragma): `lb_arm_wrestle_step` 93.16 -> 99.99 (1484 -> 1428 B) with
`LbLobbyWork* lw = &lobby_w;`; its neighbour `lb_player_control` keeps `lobby_w.` direct (retail rebuilds the
address there).

**Example.**

```
void lb_arm_wrestle_step(void)
{
    LbLobbyWork* lw = &lobby_w;              /* addi r31,r3,lobby_w@l once */
    LbArmWrestleWork* work = lw->arm_wrestle_0x0AC;
    ...
    lw->field_0x145 = 1;                     /* stb r0,0x145(r31) */
}
```
