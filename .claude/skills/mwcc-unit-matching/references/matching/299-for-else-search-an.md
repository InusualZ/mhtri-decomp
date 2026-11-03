---
id: 299
title: A for-else search: an inline helper returning the found pointer, the claim guarded and the pointer returned, puts the found path on the join
status: works
problem: retail's search loop jumps from its found path straight to the claim tail past the loop's `li r3,0; b end`; the `found` flag costs `li 0`/`li 1`/`cmpwi`/`bne` (+16 B) and an inline helper tested with `if (!p) return NULL` still +12 B
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [101, 145, 213, 217]
---

# 299. A for-else search: an inline helper returning the found pointer, the claim guarded and the pointer returned, puts the found path on the join

**Problem.** A pool allocator searches a table with a counted loop (`mtctr`/`bdnz`); retail's found path
does its bookkeeping (`used++`) and branches straight to the claim tail, while the loop's exhaustion falls
into `li r3,0; b end` placed before that tail (a for-else). A `found` flag reads `li r6,0` in the prologue,
`li r6,1` on the found path and `cmpwi r6,0; bne claim` after the loop (+16 B); an inline helper returning
the pointer, tested with `if (shell == NULL) return NULL;`, reads `li rX,0; cmpwi rX,0; bne claim; li r3,0;
b end` (+12 B).

**Why it happens.** The helper's `return shell` inside the loop and its `return NULL` after it both join at
the inlined call's result (idea 101), and the caller's test of that result is in `tools/mwcc-debugger`'s
first PCode dump (03, after-unreachable-block-deletion) in every goto-free spelling - no later pass threads
a branch whose condition is constant on an incoming edge (idea 217). What the spelling *can* choose is what
the join does: with `if (shell != NULL) { claim } return shell;` the found path arrives at the join through
its own `b` (the frontend's `b B9`, retail's `b claim` one instruction earlier), the test is one `cmpwi
r30,0; beq end`, and the exhaustion path's `li r30,0` falls into it and leaves through the function's
`mr r3,r30` - the explicit `return NULL` block disappears, so the function is +4 B, not +12/+16 B.

**How to work it.** Write the search as a `static inline` helper that returns the record on the found path
(after its bookkeeping) and `NULL` after the loop. In the caller: `shell = helper(pool); if (shell != NULL)
{ claim... } return shell;` - not `if (shell == NULL) return NULL;`. Record the residual: the join's
`li r30,0; cmpwi r30,0; beq end` against retail's `li r3,0; b end` is the frontend's flow graph and stays.

**Result.** `stage/shell` (Wii/1.3, `cflags_main`, nopeephole): `pull_shell_work` 93.59 (flag, 272 B) ->
97.34 % (260 B of 256). Measured lower on the same row: the helper with `if (!shell) return NULL` 95.16
(268 B), a pointer flag 93.28 (272 B), `i >= max` re-test 91.70 (276 B), outer `for (;;)` with the re-test
86.80, the claim inside the loop 85.63 (252 B), a one-case `switch` on the re-test 82.02 (296 B); the
scratch `goto` 100 %.

**Example.**

```
static inline _SHELL_W* shell_work_find_free(ShellPool* pool) {
    _SHELL_W* shell = pool->work_0x08;
    s32 i;
    for (i = 0; i < pool->work_max_0x04; i++, shell++) {
        if (shell->active_0x00 == 0) {
            pool->work_used_0x0C++;
            return shell;            /* retail: b claim */
        }
    }
    return NULL;                     /* retail: li r3,0; b end; ours joins and compares */
}

_SHELL_W* pull_shell_work(u32 work_size) {
    ...
    shell = shell_work_find_free(pool);
    if (shell != NULL) {
        claim...
    }
    return shell;
}
```
