---
id: 300
title: A loop with three exits: break on two of them and re-test the loaded byte after the loop, which CSE keeps, to move the tail after the loops
status: works
problem: retail's inner loop leaves three ways (restart the outer loop, a tail placed after both loops, a shared fail block) and the goto-free source puts the tail inline in the loop, so the `beq tail` forward branch, the tail's position and everything after it differ
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [145, 213, 277, 299]
---

# 300. A loop with three exits: break on two of them and re-test the loaded byte after the loop, which CSE keeps, to move the tail after the loops

**Problem.** A run-finding allocator: an outer `for (;;)` holds a skip loop and a run loop; the run loop
reads a map byte and leaves three ways - the byte is set (restart the outer loop), the run is long enough
(the fill, which retail lays out *after* both loops behind a forward `beq`), or the index hits the bound
(a `return NULL` block retail shares with the skip loop's bound exit). A C loop has one `break` target, so
the fill written inside the `if (run == count)` sits inline in the loop body and the forward `beq` becomes
a `bne` around it.

**Why it happens.** `break` on both the set-byte and the found exits lands after the inner loop; the
restart then needs a test there, and the obvious re-test `if (*map == 0) break;` looks like a second
load. It is not: the byte was loaded at the top of the iteration and nothing stored in between, so
common-subexpression elimination keeps the register and the re-test is one `cmpwi r0,0; bne outer_head`
(the restart branch retail has inside the loop, now after it); the fill follows the outer loop as retail
places it, and its own `beq` is the loop's `break`.

**How to work it.** In the inner loop write `if (*map != 0) break;` and `if (run == count) break;`, keep
the bound exit as `return NULL`, and after the inner loop `if (*map == 0) break;` so the outer loop's
exit is the tail. Check the first `tools/mwcc-debugger` dump (03) for the re-test's `lbz`: if the load is
repeated there, a store in the loop body defeats the CSE and the spelling costs a load. Idea 277's `else
continue` does nothing here (the then-arm is a return, not a single jump). The shared fail block stays a
residual: two `return NULL` statements are two `li r3,0; blr` blocks in every goto-free spelling (an
inline search helper with two `return -1` merges them through its return join but then re-tests the
result, idea 101), so the bound branch inverts to `blt head` over the inline block.

**Result.** `stage/shell` (Wii/1.3, `cflags_main`, nopeephole): `shell_block_alloc` 84.68 (312 B, fill
inline) -> 87.86 % (320 B of 308). Measured lower on the same row: the single-loop spelling 74.87 (296 B),
an inline search helper returning the first index 74.53 (336 B), an inline fill helper 83.96, the skip
loop's bound exit as `break` with an `i >= max` re-test 82.14, both run-loop bound exits as `break` with
the re-test 84.55; the scratch `goto fail`/`goto fill` 100 %.

**Example.**

```
for (;;) {
    run = 0;
    while (*map != 0) { map++; i++; if (i >= max) return NULL; }
    first_p = map; first = i;
    for (;;) {
        run++;
        if (*map != 0) break;        /* retail: bne outer_head */
        if (run == count) break;     /* retail: beq fill */
        map++; i++;
        if (i >= max) return NULL;   /* retail: bge fail (shared) */
    }
    if (*map == 0) break;            /* the CSE'd byte: cmpwi r0,0; bne outer_head */
}
fill...
```
