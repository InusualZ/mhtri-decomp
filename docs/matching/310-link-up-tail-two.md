---
id: 310
title: A link-up tail two callers share goes through one static helper: the extra inline level renumbers a nested inline's loop values
status: works
problem: Two functions end with the same call sequence; an inlined lookup inside it has the right instructions but its loop counter, argument and entry pointer in a permuted set of callee-saved registers, and no declaration order in the callers or the callees moves them.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
related: [243, 283, 295]
demo: 
---

# 310. A link-up tail two callers share goes through one static helper: the extra inline level renumbers a nested inline's loop values

**Problem.** `btm_acl_created` and `btm_read_remote_features_complete` (BTE/btm_acl) both end with the same four
steps (packet types, link policy, `BTM_SetLinkSuperTout(p->remote_addr, btm_cb.btm_def_link_super_tout)`, the
change callback). The inlined `BTM_SetLinkSuperTout` -> `btm_bda_to_acl` loop matched every opcode but held the loop
counter, the timeout and the entry in r30/r29/r28 for retail's r29/r28/r30 (and r29/r30/r28 for r30/r28/r29 in the
second caller). Declaration orders in the callers and in `btm_bda_to_acl`, a split declaration of the entry, an
early return, `u8`/`int` counters and an index-form loop all measured the same or worse.

**Why it happens.** The node numbers of an inlined callee's parameters and locals depend on the inline nesting they
were expanded in (idea 243's colouring cascade reads those numbers). Written out in each caller, the nested inline
sits one level below the caller; moved into a shared `static` helper it sits two levels below, and its timeout
parameter colours lowest of the three (r28), which is retail's order in both callers at once. The node numbers themselves are not dumped: the unit is GC/3.0a5.2 and the Wii/1.3 stand-in compiles a different object.

**How to work it.** When two or more functions share a tail whose inlined calls mis-colour, factor the tail into a
`static` helper taking the shared object (later Broadcom stacks call this one `btm_establish_continue`) and call it
from each site. Measure every caller: the helper inlines into each, so a win must hold for all of them.

**Result.** BTE/btm_acl: `btm_acl_created` 99.57 -> 100, `btm_read_remote_features_complete` 99.39 -> 100, unit
99.91 -> 100 % (+6.8 matched bytes); flipcheck READY, trial flip `main.dol: OK`.

**Example.**

```c
static void btm_establish_continue(tACL_CONN* p) {
    btm_set_packet_types(p, btm_cb.btm_acl_pkt_types_supported);
    if (btm_cb.btm_def_link_policy) {
        BTM_SetLinkPolicy(p->remote_addr, &btm_cb.btm_def_link_policy);
    }
    BTM_SetLinkSuperTout(p->remote_addr, btm_cb.btm_def_link_super_tout);
    if (btm_cb.p_acl_changed_cb) {
        (*btm_cb.p_acl_changed_cb)(p->remote_addr, p->remote_dc, p->remote_name, p->features, TRUE);
    }
}
```
