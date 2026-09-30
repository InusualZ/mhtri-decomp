---
id: 22
title: When retail's colouring is your exact mirror, stop
status: works
problem: A residual that is nothing but register numbers invites another hundred source variants, each costing a compile-and-diff cycle, none of which can be reasoned about.
tags: [allocator, process]
applies: [Wii/1.3, GC/3.0a3]
demo:
reviewed: 2026-09-29
related: [13, 18, 35, 63]
---

# 22. When retail's colouring is your exact mirror, stop

**Problem.** A residual that is nothing but register numbers invites another hundred source variants, each of
which costs a compile-and-diff cycle and none of which can be reasoned about.

**How it looks.** Same instructions, same sizes, same relocations, and every register differs in a consistent
pattern: retail's colour assignment is ours with two registers (or a whole register range) swapped.

**Why it happens.** MWCC's allocator assigns the *highest* free callee-saved register first (it minimises the
`_savegpr_*` range), so with N live webs (a *web* is one live range plus the copies that join it) the only free
variable is the *priority order* of the webs. If retail is the exact mirror of your build, the source spelling
that flips the order is not one a normal program would write.

**How to work it.** Budget the search: try the ideas that change web order once each (named temporaries and
operand order 18, declaration order 63, loop shape 19, a signed vs unsigned count 72), then record the residual on
the function (in the unit's header, per the matching policy) and move on. **Before stopping for good, try idea
35** - a chain of dead copies of the competing value flipped exactly this kind of mirror on `fn_8026F908`, after
~200 source shapes had failed. That is an *unnatural* source shape, so it belongs at the end of the list.

**Result.** `fn_804D9B4C`: ~300 source variants (element types, four positions for each induction variable,
`while`/`do..while`/pointer walks/array forms, ten declaration permutations), all 22 GC and all 9 Wii compilers,
and six `-opt` keyword permutations all produce the identical 21-row colour residual. `fn_804DAA24`'s
`R_PPC_REL24` case is the same story over 31 shapes. Both are documented as known residuals on the function.

**When NOT to apply.** The mirror must be *exact* (same instruction multiset, sizes and relocations). A residual
with any other difference (a missing instruction, a different size) is not a colouring problem - go back to idea
2 and read the first divergence.

**Evidence.** `RSO/runtime` `fn_804D9B4C` and `fn_804DAA24`, dated 2026-09-2x (the unit is `NonMatching`; the
residuals are in its header). Idea 35's counter-example is the reason "stop" is now "stop *after* the dead-copy
trick". Idea 13 is the general form ("stop when the diff is no longer flag-shaped").
