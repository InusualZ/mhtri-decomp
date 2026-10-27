---
id: 202
title: A copy member that returns *this keeps r3 and r4 live, so its loop takes fresh registers
status: works
problem: A small copy member matches except its trailing copy loop, where retail moves the pointers into r5/r6 and ours reuses r3/r4.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [171]
---

# 202. A copy member that returns *this keeps r3 and r4 live, so its loop takes fresh registers

**Problem.** A member that copies a record field by field ends in a word-copy loop (`lwz`/`lwzu`/`stw`/`stwu`
with `bdnz`); retail runs it on `addi r6,r3,..`/`addi r5,r4,..` while ours overwrites `r3`/`r4` with the loop
pointers. Every loop instruction reads as a register difference, and declaration orders change nothing because
there are no locals.

**Why it happens.** A `void` member lets `this` (r3) and the source (r4) die at the loop, so the allocator reuses
them. Returning `*this` (an assignment-operator shape) keeps `r3` live to the `blr`, and the allocator then picks
fresh volatile registers for both loop pointers, as retail did. The mangled name does not carry the return type,
so the map row is unchanged.

**How to work it.** Declare the member `T& assign(const T* src)` and end it with `return *this;`; callers that
ignore the result are unchanged. Re-measure the member and its callers.

**Result.** `Network/network_pat_control` `NetPeerRec::assign` 98.46 -> 100; its caller `copyPeerList` stays 100.

**Example.**

```
NetPeerRec& NetPeerRec::assign(const NetPeerRec* src)
{
    id_0x00 = src->id_0x00;
    name_0x0A = src->name_0x0A;
    blob_0x20 = src->blob_0x20;   /* the 0x100-byte word loop */
    return *this;
}
```
