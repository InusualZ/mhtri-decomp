---
id: 243
title: MWCC's GPR colouring is a reproducible cascade: simulate it from the dump and renumber the locals
status: works
problem: A function matches every opcode but whole groups of callee-saved registers are permuted (parameters in r28-r31 where retail has them in r26-r29 under loop state, a counter and a pointer swapped), and declaration hill climbs never converge.
tags: [source-shape, allocator, tooling]
applies: [Wii/1.3]
demo: 
related: [18, 63, 165, 224]
---

# 243. MWCC's GPR colouring is a reproducible cascade: simulate it from the dump and renumber the locals

**Problem.** A long walker function (a loop over particles with three rotating ring indices, six parameters, a
dozen locals) is byte-equivalent except for register numbers, and the permutation is not a swap: retail puts the
two loop indices in r31/r30 above the parameters, ours puts the parameters on top. Hill climbs over the declaration
block (hundreds of orders) and `register`/inner-block tricks never reach it, because the answer needs a change
that is not a permutation of the existing declarations.

**Why it happens.** Wii/1.3's GPR allocator is plain Chaitin/Briggs and is fully reproduced by this model
(checked on 9 dumps from `tools/mwcc-debugger`, 0 mismatches):

* **Numbering.** Parameters take the first virtual registers (r32...), then the declared locals in *declaration
  order, first declared = highest number* (inner-block locals come after the outer ones, so lower), then the
  frontend's extra **webs** of a split variable (a variable whose live ranges are disjoint keeps its number for the
  range that occurs **first in the code** and gets a new, high number for every later range - numbered in the order
  of the variable's first occurrence in the function), then compiler temporaries.
* **Simplify** with K = 29 (r0, r3-r12, r14-r31): scan the nodes in **ascending** number, push every node whose
  current degree (neighbours not yet pushed, physical ones included) is < 29, repeat the scan until all are pushed.
  A node that stays above K through a scan is pushed in a later scan, so long-lived nodes scanned *early* (low
  number, all their neighbours still present) are pushed last.
* **Select** pops in reverse. A node that crosses no call takes the first free of r0, r3..r12. Otherwise it scans
  the callee-saved registers handed out so far **from the lowest upward** and takes the first one no coloured
  neighbour holds; if none, the next lower new register (r31 first).

So a node's register depends only on its number relative to the others and on its interference: to move a value
up, make it scanned later (declared earlier, or a later-created web), or make it stay above K longer (scanned
before the nodes that would release it).

**How to work it.**
1. Dump the function (`tools/mwcc-debugger/mwcc_debugger.py ... FUNC OUT`) and read
   `regalloc-gpr-pass-1-all.txt` (node, name, neighbours) and `backend-03-*.txt` (which vreg is which web).
2. Simulate the cascade on that graph and confirm it reproduces the dump's assignment.
3. Map retail's registers to your vregs (the aligned diff), then re-run the simulation with candidate scan orders
   (a permutation of the vreg numbers) until it produces retail's registers - that names the renumbering.
4. Turn the renumbering into source: move a declaration (earlier = higher), make an inner-block local a
   function-level one (or the reverse), give a value its own variable so the walk's range is the variable's
   *first* web (a `head = first(pm)` beside `p`), or add an initializer whose dead store puts a variable's first
   occurrence ahead of another's (`const f32* pair = trig;`).

**Result.** `ef/ef_drawsmoothstripestrategy`: the six ribbon/tube walkers (98.41-99.43 %), `ef_smooth_tube_curve`
(99.88 %) and `Draw` (99.54 %) all to 100 %; unit 42/50 -> 50/50 rows, report metric 99.651 -> 100.0. Each
walker took one rewrite once the simulation named the order (sample pointer, particle, `pm`, `ed`, functions, ...,
head, `old`, `prev`, `cur`).

**Example.**

```
void walk(Self* self, Ctx* ctx, s32 steps, u32 flags) {
    Ring* ring;          /* first declared: highest vreg, coloured early (retail r24) */
    Particle* p;         /* the walk's own variable: its loop range is its first web */
    Manager* pm;
    ...
    Particle* head;      /* the first particle, so `p` has no range before the loop */
    s32 old;
    s32 prev;            /* last declared: scanned first, stays above K, coloured before the parameters */
    s32 cur;
    head = first(pm);
    for (p = next(pm, head); p != NULL; p = next(pm, p)) { old = prev; prev = cur; ... }
}
```
