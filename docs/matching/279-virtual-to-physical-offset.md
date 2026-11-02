---
id: 279
title: A virtual-to-physical offset on a scaled index: subtract the constant outside the pointer sum so addis reuses the product's register
status: works
problem: Retail's `mullw r4,r4,r0; addis r4,r4,0x8000; add r4,r6,r4` comes out as `addis r0,r4,0x8000; add r4,r6,r0` - the addis result takes r0 where retail reuses the product's r4
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [227, 243]
---

# 279. A virtual-to-physical offset on a scaled index: subtract the constant outside the pointer sum so addis reuses the product's register

**Problem.** A buffer address converted for the DSP, `buffer + next * chunk_size - 0x80000000`, is three instructions
in both objects, but ours colours the `addis` result r0 (`mullw r4,r4,r0; addis r0,r4,0x8000; add r4,r6,r0`) and
retail keeps everything in r4 (`addis r4,r4,0x8000; add r4,r6,r4`). In-place forms (`next *= chunk; next -=
0x80000000;` as statements) get the registers right but hoist the `mullw` above the call's first-argument load; casts,
`+ 0x80000000`, a product-first sum and an index form all keep r0 or lose more.

**Why it happens.** The backend adds an interference with physical r0 to every node used as the `rA` of an `addi`,
`addis` or a load/store base (`rA = 0` would encode a literal zero), so the product - the `addis` source - can never
be r0 and takes r4; the `addis` result is a backend temporary with no such use, coloured first (highest number) with r0
free. Spelled as `(u32)(buffer + next * chunk_size) - 0x80000000` the frontend emits add-then-`addis`, and the
backend's add-propagation pass pushes the constant down onto the product (`addis` on the product, then the `add` with
the buffer): the `addis` now writes the node that feeds the final `add` into the argument register, and the product's
r4 is reused. The `mullw` stays inside the argument expression, after the first argument's load.

**How to work it.** When retail's `addis` of 0x8000 (the `- 0x80000000` virtual-to-physical conversion) is applied to a
scaled index before the base is added, and ours gets the registers wrong, move the subtraction outside the pointer
sum: `(u32)(p + i * n) - 0x80000000` instead of `(u32)(p + (i * n - 0x80000000))`. The emitted order is the same
(`mullw`, `addis`, `add`); only the node the `addis` writes changes.

**Result.** `sound/snd_stream_mgr`: `stream_voice_rewind` 99.84 -> 100 (its last two instructions); the unit's last
open row, 63/63 at 100 %.

**Example.**

```
addr = snd_format_bytes_to_addr(voice->format, (u32)(voice->buffer + next * voice->chunk_size) - 0x80000000);
/* lwz r3,0(r3); lwz r0,0x94(r31); mullw r4,r4,r0; addis r4,r4,0x8000; add r4,r6,r4; bl */
```
