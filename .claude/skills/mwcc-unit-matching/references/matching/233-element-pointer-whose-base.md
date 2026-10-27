---
id: 233
title: An element pointer whose base must load first into its own register: assign the base, then index the pointer
status: works
problem: Retail loads an array's base pointer first, straight into the element pointer's register (`lwz r29,0x168(r30) ; mulli ; add r29,r29,r0`), where `p = &work->arr[i]` loads the index first or computes into a scratch register.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 233. An element pointer whose base must load first into its own register: assign the base, then index the pointer

**Problem.** `p = &work->profiles[work->index];` (the base a pointer field) compiles to `lwz r0,index ; mulli ; lwz
r3,base ; add r5,r3,r0` - the index first, or the base into a scratch register - where retail loads the base first and
adds into the pointer's own register (`lwz r3,base ; lwz r0,index ; mulli ; add r3,r3,r0`). The rows read as register
permutations around an otherwise identical body.

**Why it happens.** Two statements give the pointer variable a definition from the base load, so the add's
destination coalesces with it and the base load is scheduled first; the one-expression spelling makes the element
address a fresh temporary.

**How to work it.** Write the element pointer as two assignments to the same variable:
`p = work->profiles; p = &p[work->index];`. Try it per site (`mt.py tryvar --sites ... literal`): it is not a
whole-file rewrite.

**Result.** `Network/net_session_close` (`#pragma optimization_level 4`): `sessionReflectCallback` 97.97 -> 98.59 %
(three sites, +27 B), its entry `profile` +0.4 B, and `getProfileQuestRecord` 98.64 -> 100 %.

**Example.**

```
joined = work->profiles_0x168;
joined = &joined[work->profile_index_0x170];   /* lwz r3,0x168(r30) ; lwz r0,0x170(r30) ; mulli ; add r3,r3,r0 */
```
