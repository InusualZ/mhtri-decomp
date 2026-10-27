---
id: 255
title: An early exit tested with bne over a b to the epilogue is the second operand of an || condition
status: works
problem: Retail has `cmpwi r3,0 ; bne next ; b epilogue` for an early return where ours has one `beq epilogue` (4 bytes short, every later branch shifted)
tags: [source-shape]
applies: [Wii/1.0, Wii/1.3]
demo: 
related: [131, 195]
---

# 255. An early exit tested with bne over a b to the epilogue is the second operand of an || condition

**Problem.** A guard that returns early shows in retail as a branch *over* an unconditional jump to the epilogue
(`bne next ; b epilogue`), and the guard just before it is an ordinary `beq epilogue`. Ours has two plain
`beq epilogue` tests and is one instruction short. The `!`, `(s32)` and `if (c) {} else return;` spellings all
compile to the single branch.

**Why it happens.** In `if (a == 0 || b == 0) return;` the first operand's true edge goes straight to the return
block, but the last operand is the condition of the `if` itself: MWCC branches on it into the fall-through
successor and reaches the return block through the `then` block's own `b`. With peephole off nothing folds the
pair. Two separate `if (...) return;` statements each branch straight to the epilogue.

**How to work it.** Where the `bne`/`b` pair follows another early exit, merge the two guards into one `||`
condition in retail's order (the `bne/b` test is the last operand). The same shape inside a function body (a
`n >= 3 || points == 0` test before a block) is the same fix.

**Result.** `Pl/pl_act` (Wii/1.0): `fn_80279C20` 98.74 -> 99.40 (`Pl_master_ck(self) == 0 ||
pl_item_page2_ck(self) == 0`), `talisman_roll` 97.31 -> 98.69 (two `n >= 3 || points == 0` exits), and
`fn_8027B0BC` +4.4 B (the `0x659`/`0x5C2` exits; with Wii/1.3 the row reads 100).

**Example.**

```c
if (Pl_master_ck(self) == 0 || pl_item_page2_ck(self) == 0) {
    return;
}
```
