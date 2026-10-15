---
id: 128
title: addi rX,rBase,0 with no relocation means separate file statics behind a section anchor, not one struct
status: works
problem: retail passes an offset-0 member as `addi r3,r31,0x0` (no relocation) and re-materialises `addi r3,r29,0x60` before each buffer read where ours emits `mr r3,r31` and folds the offset into the load
tags: [source-shape, data]
applies: [NWC24]
demo: 
---

# 128. addi rX,rBase,0 with no relocation means separate file statics behind a section anchor, not one struct

**Problem.** The diff shows `addi r3,r31,0x0` in retail and `mr r3,r31` in ours for the first member of a work
block. Nearby, retail recomputes `addi r3,r29,0x60` before every `lwz 4(r3)` where ours folds the offset into the
load (`lwz r0,0x64(r29)`), and an inline -1/0 helper stays a branch in retail but is if-converted in ours. It reads
as an optimiser or compiler-version problem. It is the data model.

**Why it happens.** MWCC addresses a TU's file statics in `.bss` from one section anchor (`...bss.0`) kept in a
register. Each static is `anchor + offset`, so the static at offset 0 still gets an `addi rX,anchor,0`, and the
address of a static array stays a value of its own, re-materialised for each use. A struct reached through a
`work = &block` pointer is folded by the front end: the first PCode dump (`backend-02-after-initpcode`) already
has `mr` and `lwz 0x64`, so no pass or pragma restores the retail shape. The anchor is the start of the object's
`.bss`, which also tells you where the object's `.bss` begins.

**How to work it.** Replace the struct with one `static` per member in the TU, at the same offsets (32-byte
buffers keep `__attribute__((aligned(32)))`; padding becomes the alignment). `.bss` is laid out in first-use order
(idea 113). If the anchor sits *below* the first referenced member, the leading bytes belong to a static that a
dead-stripped function uses first: write that function before the other users of `.bss`. Re-measure every row.
`.sbss` is laid out in reverse declaration order, and an unreferenced `.sbss` word is dead-stripped by mwld, so
an 8-byte slot in the map is one 8-byte object.

**Result.** `NWC24/nwc24_msg` (GC/3.0a5.2): 12/17 -> 17/17 rows at 100 % (`NWC24SuspendScheduler` 98.85,
`NWC24ResumeScheduler` 99.00, `NWC24iSetScriptMode` 94.47, `NWC24iRequestIoctl` 94.93,
`NWC24iRequestGenerateUserId` 92.13 -> 100); a trial flip keeps sha1 bf4850739478caaedfe675949eb7c28595a7fde9.
`NWC24/nwc24_io`: `NWC24iSetRtcCounter` 92.43 -> 100 once its inline thread check and init block were restored.

**Example.**

```c
/* not: struct { OSMutex mutex[2]; u8 pad[0x10]; u32 in[8]; u32 out[8]; } work; ... &work->mutex[0] */
static OSMutex sRequestMutex;                              /* addi r3,r31,0x0 */
static OSMutex sSchedulerMutex;                            /* addi r3,r31,0x18 */
static u32 sInBuffer[8] __attribute__((aligned(32)));      /* addi r3,r31,0x40 */
static u32 sOutBuffer[8] __attribute__((aligned(32)));     /* addi r3,r31,0x60; lwz r0,4(r3) */
```
