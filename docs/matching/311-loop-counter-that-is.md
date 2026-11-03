---
id: 311
title: A loop counter that is an inline helper's local colours before its strength-reduced pointer
status: works
problem: a counted pointer-walk loop's counter and strength-reduced slot pointer swap two saved registers (retail r28 counter / r29 pointer, ours the reverse), nothing else differs
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [63, 165, 222, 282]
---

# 311. A loop counter that is an inline helper's local colours before its strength-reduced pointer

**Problem.** A `for (key = 0; key < 128; key++)` loop over `synth->slot[channel][key]` diffs only in two saved
registers: the counter and the strength-reduced slot pointer trade places. Declaration order and block-local
counters do not move it.

**Why it happens.** The mwcc-debugger's `regalloc-gpr-*-assigned.txt` shows two rules. (1) Colouring runs in
descending virtual-register order: named locals and parameters have the lowest numbers, an inlined helper's locals
are frontend `@` temporaries above them, and the frontend's induction pointers (`@` created later) sit between the
two. (2) Each node takes the **lowest already-used saved register it does not interfere with**, and only when none
is free a new one (counting down from r31). So a counter that is a plain local colours *after* the pointer and gets
the lower register only if r28 is not yet in use; a counter that is an inline helper's local colours *before* the
pointer, takes the lowest used free register, and pushes the pointer one up.

**How to work it.** Dump the function (`tools/mwcc-debugger/mwcc_debugger.py`), read the order in
`regalloc-gpr-pass-1-assigned.txt`, and work out which colouring order produces retail's registers under rule (2).
Then choose for each loop where its counter lives: a counter that must colour before its pointer belongs to an
inline helper that wraps the loop; one that must colour after it belongs to the caller (a function-level local the
loops share). Several loops in one function interact through rule (2), so decide them together.

**Result.** `SYN/syn` `SYNControlChange` 99.37 -> 100: the two all-notes-off loops and the sustain-pedal release
loop each became an inline helper with its own `s32 key` (the pedal loop's voice a block local inside it). Tried on
the way: a function-level counter shared by all loops 99.79 (release loops right, pedal loop voice/pointer swapped),
a per-iteration inline helper for the pedal body 99.85, the pedal loop as a helper while the release loops used the
caller's counter 99.66.

**Example.**

```
static inline void ReleaseHeld(Synth* synth, u8 channel)
{
    s32 key;                               /* the helper's local: an @ temporary, coloured before the walk */

    for (key = 0; key < 128; key++) {
        Voice* voice = synth->noteVoice[channel][key];
        if (voice != NULL && voice->releasePending != 0) {
            ...
        }
    }
}
```
