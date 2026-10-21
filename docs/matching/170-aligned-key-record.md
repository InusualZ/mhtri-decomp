---
id: 170
title: A 32-aligned key record keeps the by-value copy's address in a saved register
status: works
problem: A dictionary lookup that takes a `ResName` key by value rebuilds the copy's address at each use, where retail keeps it in a saved register (`addi r31,r1,0x10` right after the copy).
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [72]
---

# 170. A 32-aligned key record keeps the by-value copy's address in a saved register

**Problem.** g3d's name lookups copy a `ResName` key record onto the stack and pass its address on. Retail forms the
address once, in a callee-saved register, right after the copy; ours re-forms it at each use, and the rows sit at
87-90 %.

**Why it happens.** The key records are 32-byte aligned in the original source. The alignment of the record's
definition (or of the `extern` the unit sees) is a codegen input: with it, MWCC keeps the copy's address in a saved
register.

**How to work it.** Define the record (or declare its `extern`) with `__attribute__((aligned(32)))` and re-measure
the lookups.

**Result.** Wii/1.3: `g3d/g3d_resanmscn` lookups 87.38 -> 97.15; `g3d/g3d_resanmtexsrt` GetNumXxx 89.26 -> 100 from
the attribute alone.

**Example.**

```
static ResNameData32 res_name_lights __attribute__((aligned(32))) = {12, "Lights(NW4R)"};   /* addi r31,r1,0x10 after the copy */
```
