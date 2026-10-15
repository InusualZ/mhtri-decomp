---
id: 157
title: A constant tag block copied into a local is a brace-initialised local array emitted into the TU's own .sdata2 pool
status: works
problem: Retail copies a constant byte block into a stack local with both loads before the stores (`lwz r4,X@sda21 ; lwz r0,X+4@sda21 ; stw r4 ; stw r0`, or `lhz`+`lbz` for 3 bytes), the map names the words as separate `.sdata2` rows, and our copy from two declared externs interleaves load/store and leaves the unit's `.sdata2` short.
tags: [source-shape, data]
applies: [Wii/1.3]
demo: 
reviewed: 2026-10-15
related: [29, 146]
---

# 157. A constant tag block copied into a local is a brace-initialised local array emitted into the TU's own .sdata2 pool

**Problem.** A request builder fills a small local array with constants (`{1,2,3}`, `{1,2,3,5,4,6,7,8}`) and passes
it on. dtk splits the source pool object into 2+1 or 4+4 byte rows, which reads like separate scalar constants; a
source that declares them `extern` and assigns them one by one gets the stores interleaved with the loads, and the
TU's `.sdata2` misses those bytes, so every later pool literal sits at the wrong offset.

**Why it happens.** MWCC compiles `u8 tags[3] = {1, 2, 3};` (a local aggregate with a constant initialiser) into an
anonymous pool object in the TU's `.sdata2` plus a block copy (`lhz`/`lbz`, or two `lwz` before two `stw`). The
pool object is one symbol of the array's size; the map's split rows are the analyser's guess from the access widths.

**How to work it.** Write the local as a brace-initialised array and delete the `extern` constants; then compare the
unit's `.sdata2` bytes (`tools/units/datagap.py --unit`). The readers of the split rows reach 100 % once the map rows
are merged into one row of the array's size (a map edit: `symedit.py delete` the tail rows, `resize` the head row) -
file that as a request when the map is not yours.

**Result.** `Network/PatInterface` (2026-10-15): `sendReqOpcode1B` 94.74 -> 99.87 (100 with the merged rows),
`sendServerTimeout`'s `{1,2,3}` restored the 3-byte object at 0x8079C7D8, `.sdata2` 92.96 -> 98.67 (byte-identical
but the 4-byte tail padding), and 21 other builders' rows rose (the later pool literals moved to their retail
offsets); with the 16 tail rows merged in a local trial every one of those readers is 100.

**Example.**

```
s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b)
{
    u8 header[8] = {1, 2, 3, 5, 4, 6, 7, 8};   /* lwz r4,@N ; lwz r0,@N+4 ; stw r4,8(r1) ; stw r0,12(r1) */
    ...
    writeUInt8Array2(self, 8, header);
}
```
