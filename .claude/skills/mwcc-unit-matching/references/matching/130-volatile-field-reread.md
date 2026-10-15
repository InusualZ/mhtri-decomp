---
id: 130
title: A field retail re-reads after its own store or check is a volatile field
status: works
problem: Retail loads the same field again (`stw r0,0(r28)` then `lwz r0,0(r28)`, or a second `lwz` of a field ours keeps in a register) and the row is 4 bytes longer than ours.
tags: [source-shape]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [115, 120]
---

# 130. A field retail re-reads after its own store or check is a volatile field

**Problem.** Retail loads a field a second time where ours reuses the first load: a store immediately read back
(`stw r0,0x0(r28)` / `lwz r0,0x0(r28)`), a range check whose operand is loaded again for the clamp, or a pointer
field reloaded between a read and a store through it. The row is one instruction (4 bytes) longer than ours and
every later instruction is shifted.

**Why it happens.** MWCC never forwards or CSEs a `volatile` access: each source-level read is its own `lwz`.
SDK code shared between a worker thread (or an interrupt) and its callers - receive rings, request lists,
counters a thread fills, a DMA frame index - declares those fields `volatile`, so a single read in our source
turns into one load per use in theirs.

**How to work it.** Count the loads of the field in the retail function against the reads in the source. If
retail has exactly one load per source read, make the FIELD `volatile` in the struct (not a cast at the use
site), then spell the function so it reads the field as often as retail loads it (a separate local for a value
retail loads once). Rebuild every unit that includes the header and check no other row moved: a field read once
per function is unaffected. A reload with another cause (CSE across a pragma region, idea 120) does not want
`volatile`.

**Result.** `NHTTP/d_nhttp` and `NHTTP/NHTTP_os_RVL` (GC/3.0a5.2): `NHTTPSock.length` volatile took
`NHTTPi_SocRecvOffsetRange` 79.67 -> 100; `NHTTPRequestNode.request` took `NHTTPi_cancelRequestById` 97.76 -> 100
and `NHTTPi_cancelAllRequests` 97.38 -> 100; `NHTTPResponse.headerLength` took `NHTTPi_recvResponseHeaders`
97.56 -> 100; `NHTTPRecvBuf.capacity` took `NHTTPi_findHeaderField` 95.39 -> 96.82. No other row in `d_nhttp`,
`NHTTP_bgnend` or `NHTTP_os_RVL` moved. The same shape is in `AX/AXOut` (Wii/1.3): `__AXOutDmaFrame` is volatile
because the target re-reads it after every store (unit header SHAPES line).

**Example.**

```c
typedef struct NHTTPSock {
    u8 pad_0x000[0x1C];
    volatile u32 length;   /* the comm thread refills it */
    u8 pad_0x020[0x8];
    u8* base;
} NHTTPSock;

s32 recvRange(NHTTPSock* sock, s32 offset, s32 length) {
    s32 left;

    if (sock->length <= (u32)offset) {   /* lwz #1 */
        return -1003;
    }
    left = sock->length - offset;        /* lwz #2, as retail */
    if (length > left) {
        length = left;
    }
    return length;
}
```
