---
id: 3
title: Read the target's disassembly, not just the diff
status: works
problem: The diff shows *what* differs, not *what the original source looked like*. Several flags are only discoverable if you already know the target's code shape.
tags: [flags]
applies: []
demo:
---

# 3. Read the target's disassembly, not just the diff

**Problem.** The diff shows *what* differs, not *what the original source looked like*. Several flags are
only discoverable if you already know the target's code shape.

**Why try it.** Every split unit ships a full disassembly of the target, which is the cheapest way to see
prologues, tail calls, unrolling and table addressing - and a prologue is often a flag fingerprint.

**Result.** Whole flag families can be read off the target's prologue and addressing: whether it uses
`stmw`/`lmw` or the EABI save helpers, whether it materialises each table with its own `lis`+`addi` pair or
shares one base, and whether index arithmetic is fused.

**Example**

```
target: stwu r1,-0x140(r1); mflr r0; stw r0,0x144(r1); addi r11,r1,0x140; bl _savegpr_14
ours:   stwu r1,-0x140(r1); stmw r14,0xf8(r1)
```
