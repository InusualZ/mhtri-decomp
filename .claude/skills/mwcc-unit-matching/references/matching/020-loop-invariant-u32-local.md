---
id: 20
title: Force a loop-invariant address through a `u32` local
status: works
problem: Retail materialises a loop-invariant field address in a callee-saved register in the loop preheader (`addi r30,r26,84`); ours folds it into a load displacement (`lwz r4,0x54(r27)`), costing a register.
tags: [source-shape, allocator]
applies: [GC/3.0a3]
demo: 020-loop-invariant-u32-local.cpp
reviewed: 2026-09-29
related: [18, 19, 60]
---

# 20. Force a loop-invariant address through a `u32` local

**Problem.** Retail materialises a loop-invariant field address into a callee-saved register in the loop
preheader (`addi r30,r26,84` then `lwz r0,0(r30)`); our build folds it into a load displacement
(`lwz r4,0x54(r27)`). That costs one callee-saved register and changes the colouring of the whole function
(`_savegpr_24` where retail has `_savegpr_23`; `_savegpr_N` is the runtime helper that saves registers r`N`..r31).

**How it looks.** Retail has an `addi rN,rM,<disp>` in the loop preheader and `lwz rX,0(rN)` in the body; ours
has `lwz rX,<disp>(rM)` in the body and no preheader `addi`. One register fewer is live in ours, so every
later register shifts.

**Why it might work.** Taking the address (`&p->field`) does *not* prevent the fold; routing the address through
an integer type once did, on the unit below.

**How to work it.** Compute `(u32)pObject + disp`, park it in a `u32 buf[1]` local (or a scalar), and load
through it; declare the loop counter (`int i;`) before the count if the colouring is still off (idea 18).

```c
u32 buf[1];
buf[0] = (u32)pModule + 0x54;   /* retail: addi r30,r26,84 ; lwz r0,0(r30) - not lwz r0,0x54(r26) */
```

**Demonstration - the effect did NOT reproduce in isolation.** `020-loop-invariant-u32-local.cpp`
(`ideas.py demo-check 20`) compiles a call-in-a-loop over `m->table[0]` (+0x54) spelled four ways (direct,
`u32 buf[1]`, a scalar `u32`, an `int*`): all four fold the address into `lwz r3,84(r29)` and come out
byte-size identical (0x58; the `while (i-- > 0)` pair is 0x50 each). So on Wii/1.3 `-O4,p` a trivial loop is
not moved by this spelling. The original win was on a larger function whose register pressure differs, on
`GC/3.0a3`; treat the trick as a variant to *try* when retail shows the preheader `addi`, not as a rule. What
would need measuring: whether the difference needs a second use of the address, or higher pressure, or the
`GC/3.0a3` front end.

**When NOT to apply.** When retail's loop body reads the field by displacement (`lwz rX,0x54(rM)`) - forcing an
`addi` there would add an instruction.

**Evidence** (`RSO/runtime`, dated 2026-09-2x). `fn_804DA6C8` 94.77 -> **100 %** by computing
`(u32)pObject + 0x54`, parking it in a `u32 buf[1]` local and declaring `int i;` before `count`.
