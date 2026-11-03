---
id: 306
title: Backend temporaries are numbered in statement order: move the statement, and read a global into a local to put its load on the named tier
status: works
problem: A row matches every opcode but a block's volatile registers are permuted (a 64-bit difference in r12/r0 where retail has r8/r5, two pooled globals one register apart), the differing nodes are all backend temporaries, and no declaration order or pragma moves them
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [243, 244, 278]
---

# 306. Backend temporaries are numbered in statement order: move the statement, and read a global into a local to put its load on the named tier

**Problem.** `__AI_SRC_INIT` (AI_SDK/ai, -O4,p) differs in one block only by register numbers: ours computes
`end - start` into r12/r0 and loads the two timing windows into r8..r11, retail has the difference in r8/r5, the
first register clear's temporary in r0 and the windows in r9..r12 (`gAIWindow3000` first). `regsim verify` replays
our colouring exactly and `regsim want` names the nodes - and every one of them is a backend temporary (`r84/85`,
`r91/92`, `r93..r96`), so the declaration search (ideas 243/244) has nothing to permute: seven declaration, pragma
and expression spellings (`opt_lifetimes off`, `opt_common_subs off`, a zero initialiser, `register`, a cast, the
compare reversed, `diff` inlined) change nothing or lose bytes.

**Why it happens.** Backend temporaries are numbered in the order the frontend emits their statements, and the
select order is descending number (idea 243), so the colour a temporary gets is decided by *which statement comes
first*: the first-created temporary of a block is coloured last and takes whatever volatile register the later
ones left. Replaying custom orders on the dump's graph (`lib.regsim.simulate(graph, order)`) turns the retail
registers into one invariant - `windows < diff < first clear` - which no single statement order satisfies while
the two volatile clears keep their store order... unless the window loads leave the temporary tier altogether. A
global read into a plain local (`margin = gAIWindow3000;`) is emitted straight into the local's own vreg (no copy to
propagate), so the load sits on the **named** tier: numbered by reverse declaration order and coloured after every
temporary. The three shapes then compose: `diff = end - start` placed *before* the two clears numbers the
difference below the clears' temporaries (so the first clear is coloured first and takes r0, the difference's high
half r5 and its low half r8), and the two windows read into locals declared and assigned `margin` before `bound`
colour r9:r10 and r11:r12 and are scheduled in that order. Reading the windows *before* the clears instead loses
52 B: the scheduler will not move the non-volatile loads below the volatile stores, so they land above them.

**How to work it.** On a register-only residual whose differing nodes `regsim want` lists as backend temporaries:
1. dump the function, confirm `regsim verify` is EXACT, and replay candidate orders of the block's temporary
   groups with `lib.regsim.simulate(graph, order)` until the retail registers come out - that names the numbering
   invariant (here `windows < diff < first clear`);
2. turn it into statement order: move the computation whose temporary must be numbered lower *earlier* in the
   source (before the statements whose temporaries it must sit under), keeping volatile accesses in their order;
3. a value that must be coloured after every temporary gets a plain local assigned from the global (the load lands
   on the named tier); order those locals' declarations for their relative colour (first declared = coloured first)
   and their assignments for the schedule.
Measure each step with `mt.py tryvar` - the three steps here were +1.6 B, +1.2 B/+0.8 B and +1.2 B.

**Result.** `AI_SDK/ai` (Wii/1.3, -O4,p): `__AI_SRC_INIT` 98.96 -> 100 %, unit 11/12 -> 12/12 rows; flipcheck READY,
trial flip `main.dol: OK` (sha1 bf4850739478caaedfe675949eb7c28595a7fde9). Every compiler build in the tree
(Wii/1.0..1.7, GC/3.0a3..3.0a5.2) gives the same 98.96 % object from the old shape - it is a source shape, not a
version.

**Example.**

```
    s64 diff;
    s64 margin;                    /* declared before `bound`: coloured first (r9:r10, retail's gAIWindow3000) */
    s64 bound;
    ...
        end = OSGetTime();
        diff = end - start;        /* before the clears: its temporaries number below theirs, so the clear takes r0 */
        AI_REGS[0] = AI_REGS[0] & ~0x2;
        AI_REGS[0] = AI_REGS[0] & ~0x1;
        margin = gAIWindow3000;    /* a global read into a local: the load is on the named tier, coloured last */
        bound = gAIWindow31524;
        if (diff < bound - margin) {
```
