---
id: 264
title: A function that opens with a b to its own next instruction is a tail call into an unsplit neighbour: split the map row
status: works
problem: a target row reads `addi r3,r3,8; b .+4; stw ...; blr` (or starts with `b .+4`), a branch with no relocation into its own body; folding the offset into the stores scores 66-80 % and no inline or pragma reproduces the branch
tags: [source-shape, symbols]
applies: [Wii/1.3]
related: [36]
demo: 
---

# 264. A function that opens with a b to its own next instruction is a tail call into an unsplit neighbour: split the map row

**Problem.** A short row ends its first block with an unconditional `b` to the very next instruction, and the code
after it uses `r3` as if it were an argument (`addi r3,r3,8; b +4; stw r4,0(r3); addi r0,r4,16; stw r0,4(r3); blr`).
No relocation sits on the `b`. Writing the body inline folds the offset into the stores; an `inline` helper is
inlined without the branch.

**Why it happens.** The map row covers two functions. The first is a tail call (`b callee`, peephole off keeps the
`b`), and the callee is the next function in `.text`, which the analyser never split because nothing else calls it -
so the `b` resolves to an address inside the row and dtk emits no relocation.

**How to work it.** Split the row at the `b`'s target (`symedit.py split <row> <offset> <name>`), write the callee
as its own non-inline function directly after the caller (a later definition is not inlined), and call it from the
caller. Two rows with identical callee bodies are two functions, each written once.

**Result.** `sound/snd_stream_reloc`: `fn_800E7EA4` 66.33 -> 100 and `fn_800E7EBC` 80.00 -> 100, plus the new rows
`snd_reloc_state1_set_fixed`/`snd_reloc_state0_set_fixed` (0x10 each) at 100.

**Example.**

```
void fn_800E7EA4(void* group, RelocTable* table)            /* addi r3,r3,8 ; b snd_reloc_state1_set_fixed */
{
    snd_reloc_state1_set_fixed(&((RelocGroup*)group)->state[1], table);
}
void snd_reloc_state1_set_fixed(RelocState* state, RelocTable* table)
{
    state->table = table;
    state->entries = (void**)&table->entries_16;
}
```
