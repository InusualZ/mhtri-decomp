---
id: 313
title: A single-use load that must stay a named local: read it through a volatile lvalue so the frontend keeps it
status: works
problem: A read-modify-write of one field (`p->w = (p->w & 0x3F) | (v & 0xFC0)`) matches every opcode but the field address and the loaded word swap two volatile registers (`add r5,..; lwz r4,0x25c(r5)` where retail has `add r4,..; lwz r5,0x25c(r4)`), and no operand order, temporary, inline helper or declaration order moves them
tags: [source-shape, allocator]
applies: [Wii/1.3]
related: [243, 244, 260, 293]
demo: 
---

# 313. A single-use load that must stay a named local: read it through a volatile lvalue so the frontend keeps it

**Problem.** `KBD/kbd` `KBDSetChannelLeds` and the tail of `kbd_update_lock_state`: the address of the lock word
and the word loaded from it are in each other's registers. Retail colours the address first (it takes r4), ours
colours the load first.

**Why it happens.** The mwcc-debugger dump (`regalloc-gpr-pass-1-all.txt`, `backend-03`) shows the load as a
backend temporary and the address as a named (or inline `@`) local. Colouring runs from the highest virtual
register (idea 243), and every backend temporary is numbered above every local, so the load is always coloured
first. A local `u32 old = p->w;` does not help: the frontend forward-substitutes a single-use local into its use
(`old` has no node at all in the dump), and so does an inline helper's local or return value, a second assignment
to a reused local, or a separate declaration. Retail's order needs the load in a *named local numbered below the
address local* - a local that survives the frontend. `#pragma opt_propagation off` keeps it (the function then
matches), but in a function that reads a global twice it also merges loads retail keeps apart.

**How to work it.** Read the word through a volatile lvalue into the local: the frontend does not substitute a
volatile load, so the local keeps its node, numbered by declaration order below the address local declared before
it. The instructions are unchanged (one `lwz`). Check with `regsim verify` on a fresh dump that the local now has
a node and the colouring is retail's.

**Result.** `KBD/kbd` (Wii/1.3, `-O4,p`): `KBDSetChannelLeds` 99.19 -> 100, `kbd_update_lock_state` 99.87 -> 100
(after its `step` fix); unit 28/28, flipcheck READY, trial flip `main.dol: OK`. Measured and not enough: `old` as a
plain local, before or after the call, a reused function local, an inline getter, an inline merge helper, 15
operand-order/temporary spellings, a `volatile` field (breaks `KBDResetChannel`), and the inline helper with the
volatile read (its `@` local is still substituted).

**Example.**

```
BOOL enabled = OSDisableInterrupts();
KBDChannel* target = &kbd_state->channels[channel];     /* declared first: numbered above `old` */
u32 old = *(volatile u32*)&target->lockState;            /* a plain read is substituted away */
target->lockState = (old & 0x3F) | (value & 0xFC0);
OSRestoreInterrupts(enabled);
```
