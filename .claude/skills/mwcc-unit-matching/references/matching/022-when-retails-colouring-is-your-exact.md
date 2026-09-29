---
id: 22
title: When retail's colouring is your exact mirror, stop
status: works
problem: A residual that is nothing but register numbers invites another hundred source variants, each of which costs a compile-and-diff cycle and none of which can be reasoned about.
tags: [allocator]
applies: []
demo:
---

# 22. When retail's colouring is your exact mirror, stop

**Problem.** A residual that is nothing but register numbers invites another hundred source variants, each
of which costs a compile-and-diff cycle and none of which can be reasoned about.

**Why try it.** MWCC's allocator assigns the *highest* free callee-saved register first (it minimises the
`_savegpr_*` range), so with N live webs the only free variable is the *priority order* of the webs. If
retail is the exact mirror of your build - same instructions, same sizes, mirrored colours - the source is
not the lever, and the honest move is to record the residual on the function and spend the time elsewhere.

**Result.** `fn_804D9B4C`: ~300 source variants (element types, four positions for each induction
variable, `while`/`do..while`/pointer walks/array forms, ten declaration permutations), all 22 GC and all
9 Wii compilers, and six `-opt` keyword permutations all produce the identical 21-row colour residual.
`fn_804DAA24`'s `R_PPC_REL24` case is the same story over 31 shapes. Both are documented as known
residuals on the function (see idea 13).
