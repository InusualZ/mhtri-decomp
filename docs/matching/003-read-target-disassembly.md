---
id: 3
title: Read the target's disassembly, not just the diff
status: works
problem: The diff shows *what* differs, not *what the original source looked like*. Several flags are only discoverable if you already know the target's code shape.
tags: [measurement, flags]
applies: [Wii/1.3]
demo: 003-read-target-disassembly.cpp
reviewed: 2026-09-29
related: [2, 5, 12, 21, 39]
---

# 3. Read the target's disassembly, not just the diff

**Problem.** The diff shows *what* differs, not *what the original source looked like*. Several flags are
only discoverable if you already know the target's code shape.

**How it looks.** The diff lists dozens of "different" rows and no single one says why; reading the target
straight through shows the pattern (every function saves registers the same way, every table gets its own
address pair).

**Why it happens.** Every split unit ships a full disassembly of the target object (`build/RMHE08/obj/...`, or
`objdump -dr` of it), the cheapest way to see prologues, tail calls, unrolling and table addressing. A prologue
is often a flag fingerprint: the compiler chooses `stmw`/`lmw` or the EABI save helpers by `-use_lmw_stmw`.

**How to work it.** Read the target's prologue and addressing before touching flags. Whole families can be read
off it: `stmw`/`lmw` versus `bl _savegpr_N` (the EABI save-register helper, a runtime function that spills
r*N*..r31), whether each table is materialised with its own `lis`+`addi` pair (`@ha`/`@l`, the high-adjusted and
low halves of an absolute address) or shares one base register, and whether index arithmetic is fused into
`rlwinm`/`extrwi` (idea 7, idea 21).

**When NOT to apply.** A prologue shape says which switch is set, not that the rest of the function will match;
and the same shape can come from a source difference (a local that forces a save). Confirm with a scratch
compile (idea 7) before writing the flag into `configure.py`.

**Result.** Flag hypotheses that come from the target's own instructions, not from a blind sweep.

**Example**

```
target: stwu r1,-0x140(r1); mflr r0; stw r0,0x144(r1); addi r11,r1,0x140; bl _savegpr_14
ours:   stwu r1,-0x140(r1); stmw r14,0xf8(r1)
```

**Demonstration.** `003-read-target-disassembly.cpp` compiles a function that keeps 18 values live across calls
with `-use_lmw_stmw off`: the object has `bl _savegpr_14` / `_restgpr_14` and no `stmw`. The same source with
`-use_lmw_stmw on` emits `stmw r14,24(r1)` / `lmw r14,24(r1)` (measured with the Wii/1.3 compiler when the demo
was written; the on case is not asserted because a demo carries one flag set). Whether the retail unit used the
helpers is therefore a flag question (`-use_lmw_stmw`), answerable from the target's first ten instructions.
