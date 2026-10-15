---
id: 137
title: Copy a parameter into a local to hoist its address before a branch
status: works
problem: Retail moves a parameter into another register (`mr r7,r3`) and forms an address from it (`addi`) before a callback test, where ours forms the address after the branch and the function is far below 100 %.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
demo: 
related: [96, 124]
---

# 137. Copy a parameter into a local to hoist its address before a branch

**Problem.** A completion handler takes a pointer parameter, tests a stored callback and, if set, hands the
callback a record built from the parameter (a copy into `params.param_buf`). Retail copies the parameter (`mr r7,r3`) and forms
the address (`addi`) before the test; ours forms it inside the taken arm, so the block layout and every register
after the branch differ.

**Why it happens.** Named locals initialised at the top are born before the branch, so their computations stay
there; a sub-expression written in the call is born in the arm that uses it. The original assigned both to locals
first.

**How to work it.** Where retail computes a parameter-derived address before a test that ours only computes after
it, declare locals for the parameter and the address at the top and use them in the arm. Idea 96 is the same
lever for an address kept across a call.

**Result.** `BTE/btm_devctl` (GC/3.0a5.2): `btm_vsc_complete` 84.26 -> 100 with `p` and the `param_buf` address
copied into locals before the callback test.

**Example.**

```c
void btm_vsc_complete(u8* p, u16 opcode, u16 evt_len) {
    BtmCompleteCallback p_cb = btm_cb.p_vsc_cmpl_cb;
    tBTM_VSC_CMPL vcs_cplt_params;
    u8* p_src = p;                              /* mr r7,r3 */
    u8* p_dst = vcs_cplt_params.param_buf;      /* addi, before the test */

    btm_cb.vsc_busy = FALSE;
    btm_cb.p_vsc_cmpl_cb = NULL;
    if (p_cb) {
        vcs_cplt_params.opcode = opcode;
        vcs_cplt_params.param_len = evt_len;
        memcpy(p_dst, p_src, evt_len);
        ...
    }
}
```
