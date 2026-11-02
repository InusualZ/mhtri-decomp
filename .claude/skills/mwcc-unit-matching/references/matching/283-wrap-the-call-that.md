---
id: 283
title: Wrap the call that initialises a saved local in a static helper so its return node is coloured first, leaving inlined copies alone
status: works
problem: Two callee-saved locals are swapped (r31/r30) in a function's own body, the other declaration order fixes it but swaps the same pair in every inlined copy of the function, and no block scope, `register`, dead initialiser, caller-side local or scoped pragma separates the two contexts.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
demo: 
related: [243, 244]
---

# 283. Wrap the call that initialises a saved local in a static helper so its return node is coloured first, leaving inlined copies alone

**Problem.** A function with two saved locals (`p_buf` from a pool call, `p` the address of a global) matches every
opcode but has r31/r30 swapped, while its two inlined copies (`-ipa file`) match. Declaring `p_buf` first fixes the
standalone body and costs both inlined copies, which want `p` first; block-local `p`, `register`, `= NULL`
initialisers, a caller-side local for the argument, reassigning the parameter and every scoped `#pragma`
(`opt_lifetimes`, `opt_common_subs`, `global_optimizer`, `scheduling`, `optimization_level`) follow the declaration
order in both contexts (72 trials on `BTE/btm_devctl`).

**Why it happens.** The colouring takes the nodes in descending virtual-register number (ideas 243/244). In the
inlined copy the parameter is a frontend temporary bound to the argument, `p = &global` is folded onto it, and the
coalesced node keeps the *lower* number of the pair - so `p`'s declaration rank decides both contexts alike. A
value returned by an inlined static helper is different: the local it initialises coalesces with the helper's
return temporary, which is numbered above the function's own locals, so that local is coloured first in the
standalone body - while in the inlined copies the same temporary is created inside the callee's block and keeps
its place below the folded parameter node.

**How to work it.** Move the initialising call into a one-line `static` helper the compiler inlines (`-ipa file`
or `-inline auto`), and assign the local from the helper. Keep the retail declaration order. Measure every caller
that inlines the function: the helper must leave them unchanged. Using the helper at the unit's other sites of the
same call is codegen-neutral, so one spelling can serve the whole file.

**Result.** `BTE/btm_devctl` (GC/3.0a5.2, `cflags_nw4r`): `BTM_SetLocalDeviceName` 98.91 -> 100 % with
`btm_reset_complete` and `btm_dev_timeout` (the inlined copies) unchanged at 100; unit 32/33 -> 33/33 rows,
99.9694 -> 100.0; trial flip links to the retail DOL (`bf4850739478caaedfe675949eb7c28595a7fde9`). The same helper
at the unit's five other `p_buf` sites changed no byte.

**Example.**

```
static BT_HDR* btm_get_cmd_buf(void) {
    return (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID);
}

u8 BTM_SetLocalDeviceName(char* p_name) {
    u8* p;              /* retail order kept: p above p_buf for the inlined copies */
    BT_HDR* p_buf;
    ...
    if ((p_buf = btm_get_cmd_buf()) != NULL) {   /* was (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID): r30/r31 swapped */
        p = (u8*)btm_cb.bd_name;
```
