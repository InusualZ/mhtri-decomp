---
id: 278
title: opt_lifetimes off keeps a reassigned local on one node: a cast sum reads the loaded operand first and colours beside it
status: works
problem: A local summed with a fresh load is `add r3,r3,r0` (variable first) where retail has `add r3,r0,r3`, and every spelling that keeps the source's operand order moves the sum into r0 instead
tags: [source-shape, allocator, pragma]
applies: [Wii/1.3]
demo: 
related: [18, 243, 244]
---

# 278. opt_lifetimes off keeps a reassigned local on one node: a cast sum reads the loaded operand first and colours beside it

**Problem.** `pan = cmd->pan - 64; pan = track->pan + pan;` compiles to `lbz r3; subi r3,r3,0x40; lbz r0; add r3,r3,r0`
while retail has `add r3,r0,r3`: the same registers, the loaded byte as the first operand. Every spelling of the sum
(`pan + track->pan`, a cast on the load, a named local for the load, a block-local, `- -pan`, `*&pan`, a reference) gives
the variable-first add, and the one-expression forms (`track->pan + (cmd->pan - 64)`) reassociate the `-64` to the end
(-9 B). A cast around the whole sum, `pan = (s32)(track->pan + pan)`, does keep the source's operand order - but the
sum then lands in r0 (`add r0,r0,r3`, the clamp and the `extsh` follow it), one register off on seven instructions.

**Why it happens.** Two frontend rules, visible in the first PCode dump (`tools/mwcc-debugger`): an assignment whose
right side is a commutative op with the destination as one operand is emitted destination-first (`add pan,pan,t`)
whatever the source order; and an assignment from any other expression computes into a temporary and renames the
variable's later uses to it - a lifetime split, the `@NNNN` web of idea 244. The split web has only the parameters
as neighbours, so it takes the first free register, r0. The first web of `pan` is live across the `lbz` of
`track->pan`, so it interferes with that r0 load and colours r3 - and that is the node retail's add writes: retail's
sum is on `pan`'s own node, with the loaded operand first. `#pragma opt_lifetimes off` is the frontend switch for
the split: with it off, the cast form's temporary is assigned back into `pan`'s single node, so the add reads
`(track->pan, pan)` and writes r3. The plain form stays destination-first under the pragma (the first rule is not a
lifetime).

**How to work it.** When the diff is a single commutative add with the loaded operand on the wrong side and the
result in the same register: wrap the function in `#pragma opt_lifetimes off` / `#pragma opt_lifetimes reset` and
write the sum with a cast around it, `pan = (s32)(track->pan + pan)`. Measure the whole function - the pragma keeps
every reassigned local of the function on one node, so a local reused for disjoint ranges can move; here none did.
Probe the shapes in a scratch TU with the unit's real flags before touching the unit (one compile a second).

**Result.** `sound/snd_stream_mgr` (Wii/1.3, -O3, peephole off): `stream_voice_setup` 99.97 -> 100 (the last
instruction of a 1456 B row), unit 61/63 -> 62/63 rows; no other row moved.

**Example.**

```
#pragma opt_lifetimes off
void stream_voice_setup(StreamVoice* voice, StreamCmd* cmd, StreamTrack* track, u32 id)
{
    s32 pan;
    ...
    pan = cmd->pan - 64;              /* lbz r3,0x8(r29); subi r3,r3,0x40 */
    pan = (s32)(track->pan + pan);    /* lbz r0,0x7(r30); add r3,r0,r3   (without the pragma: add r0,r0,r3) */
    if (pan > 127) { pan = 127; } else if (pan < 0) { pan = 0; }
    voice->pan = pan;                 /* extsh r0,r3; sth r0,0x5a(r28) */
}
#pragma opt_lifetimes reset
```
