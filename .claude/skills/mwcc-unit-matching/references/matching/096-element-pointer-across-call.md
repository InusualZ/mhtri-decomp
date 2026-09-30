---
id: 96
title: An index used before and after a call: name the element pointer so the address survives it
status: works
problem: Retail computes `index * 4` (and `this + index * 4`) once and keeps both in callee-saved registers across a call, where ours rebuilds the scaled index after the call and keeps a different register set.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 096-element-pointer-across-call.cpp
reviewed: 2026-09-30
---

# 96. An index used before and after a call: name the element pointer so the address survives it

**Problem.** `unregisterReceiver` reads `receiverIds_34[index]` for a call, then loops, then writes
`receivers_14[index] = 0; receiverIds_34[index] = 0;`. Retail has `slwi r30,r4,2 ; add r31,r3,r30` once, at entry,
and uses r30/r31 after the loop; ours scales the index twice and keeps the loop bound (`count`) in r30 instead.
The row sat at 91.4 % through three rewrites (a cached `count`, reading `receiverCount_28` in the loop, assigning
it after the call - 90.9 / 85.9), each of them moving *every* callee-saved register.

**Why it happens.** The register allocator ranks live ranges across the call; `t->ids[index]` written twice as an
array subscript is two expressions to it (the second is rebuilt after the call), while one named pointer is one
long-lived value. The pointer, not the scaled index, is what retail's source held.

**How to work it.** When a register diff shows a value computed at entry and reused after a call that ours
recomputes, find the lvalue the source touches twice and bind it once: `u32* id = &t->ids[index]; ... *id = 0;`.
Keep the loop bound as the field read (`i < t->count`), not a local. Measure: the whole register set usually
snaps into place at once.

**Result.** `Network/fn_8041A87C` `unregisterReceiver` (2026-09-30): 91.375 -> 100.00 with the element pointer and
the loop reading the field directly. Every other spelling above scored 85.9-91.4.

**Example.**

```
u32* id = &receiverIds_34[index];      // one addi/add at entry, kept in r31
replyRequest((s32)*id);
for (i = 0; i < receiverCount_28; i++) { ... }
receivers_14[index] = 0;
*id = 0;
```
