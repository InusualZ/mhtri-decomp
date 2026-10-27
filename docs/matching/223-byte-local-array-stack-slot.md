---
id: 223
title: Address-taken one-byte locals in 4-byte stack slots are one-element arrays
status: works
problem: Several address-taken `s8` locals (passed as `const s8*`) sit at 0x8/0x9/0xA in ours and at 0x8/0xC/0x10 in retail, shifting every stack offset above them.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: []
---

# 223. Address-taken one-byte locals in 4-byte stack slots are one-element arrays

**Problem.** Inlined writers each pass `&term` (`s8 term`) to a call; ours packs the bytes at 0x8, 0x9, 0xA and
the next local at 0xC, retail gives each its own word (0x8, 0xC, 0x10) and the next local starts at 0x14.

**Why it happens.** MWCC packs scalar `char` locals at byte alignment but gives an array its own 4-byte aligned slot,
even an array of one.

**How to work it.** Declare the byte as `s8 term[1]`, write `term[0] = v` and pass `term`. The order of the slots
then follows the order the locals are first seen (inlined bodies in call order), so check that too.

**Result.** `Network/NetworkSessionStable` `move`: the three terminators and the following `recordLength` moved to
retail's 0x10/0xC/0x8/0x14 (part of the 97.45 -> 99.02 reshaping).

**Example.**

```
s8 term[1];
term[0] = -2;
sendStream(&stream, 1, 1, term, 0xFF);
```
