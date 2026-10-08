---
id: 119
title: Select the conditional store value with a ternary, accumulate into a parameter, order non-leading locals
status: works
problem: a function is instruction-equal but the target stores a conditionally chosen word once and keeps a parameter register as the scratch, where ours stores twice or spends a callee-saved register
tags: [source-shape, allocator]
applies: [TRK]
demo: 
---

# 119. Select the conditional store value with a ternary, accumulate into a parameter, order non-leading locals

**Problem.** Three `TRK/targimpl` rows read 65.7, 83.9 and 89.1 with equal instruction counts: only register numbers and
one store differ.

**Why it happens.** The original source folded values the way MWCC colours them: (1) the store of a value that depends on
a flag is a single `x = flag ? a : b` (the target has one `stw` after a `beq` over one `oris`), where
`x = a; if (flag) x = b;` stores twice; (2) a parameter that is dead after its last read is reused as the scratch
(`length += address; last = length - 1;` puts the sum in the `length` register, a separate `end` local does not); (3) a
value read before every call (`word = *ConvertAddress(n); append(msg, word);`) is loaded into the return register
first, where a nested call argument reloads `r3` after `mr r3,msg`.

**How to work it.** Read the target's store/branch shape first. Then: ternary select; fold a dead parameter instead of
declaring `end`; split a nested `*f()` argument into a named temporary. `permdecl` permutes only the *leading* run of
declarations: for a swap of two callee-saved registers also try the order of the declarations after the first
initialised one (`u32* reg; s32 index; s32 err;` fixed `TRKTargetAddStopInfo`).

**Result.** `TRKTargetAccessPairedSingle` 65.7 -> 100, `TRKTargetAddStopInfo` 83.9 -> 100 (temporaries 99.2, then
declaration order), `TRKTargetCheckMemoryRange` 89.1 -> 100.

**Example.**

```
instructions[0] = is_read != 0 ? ((reg << 21) | 0xF0030000) : ((reg << 21) | 0xE0030000);
```
