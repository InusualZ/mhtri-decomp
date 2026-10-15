---
id: 136
title: Read a global directly, not through a pointer local, when retail rebuilds its address
status: works
problem: Retail re-forms a global's address (`lis` scratch + `addi` into a saved register, or one per inlined copy) where ours keeps one pointer local live, so the counter and pointer registers or the code length differ.
tags: [source-shape, allocator]
applies: [Wii/1.3, GC/3.0a5.2]
demo: 
related: [20, 63, 126]
---

# 136. Read a global directly, not through a pointer local, when retail rebuilds its address

**Problem.** A function (or an inline helper expanded several times) works on one global block. Ours names it
once (`p_inq = &btm_cb.btm_inq_vars;`, `tables = viEncoderTables;`) and keeps that pointer
in a register; retail materialises the address with its own `lis` + `addi` at the uses (separately in every
inlined copy) and its counter/pointer registers come out in another order.

**Why it happens.** A pointer local is one long-lived value with its own web; a direct global reference is
re-formed from the symbol where it is used, so it adds no web of its own and the remaining values are coloured
in a different order. This is the opposite lever of idea 63's `messages = table` local - which one retail used is
read off the code: one saved register for the base means a local, a fresh `lis`/`addi` per use means the global.

**How to work it.** When retail re-forms a global's address where ours holds a pointer, drop the local and write
`btm_cb.btm_inq_vars.field` / `viEncoderTables[n]` at the uses; inside an inline helper do it in the helper.
Measure the size too: the direct form can be longer.

**Result.** `BTE/btm_inq` (GC/3.0a5.2): `btm_clr_inq_db` without its `p_inq` local takes `btm_inq_db_reset`
97.45 -> 100 and `BTM_ClearInqDb` 99.81 -> 100 (both inline it). `VI/vi3in1` (Wii/1.3): naming
`viEncoderTables` directly instead of through a `tables` local takes `__VISendEncoderRegs` +2.45 points, though
0x28 B longer (the rest of the residual is elsewhere). `AX/AXOut` writes the profile fields and `__AXDspTask` by
name for the same reason (the target re-forms the address).

**Example.**

```c
static void btm_clr_inq_db(u8* p_bda) {
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;    /* no p_inq = &btm_cb.btm_inq_vars local */
    u16 xx;

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        ...
                if (btm_cb.btm_inq_vars.p_inq_change_cb != NULL) {
                    (*btm_cb.btm_inq_vars.p_inq_change_cb)(&p_ent->inq_info, FALSE);
                }
    }
}
```
