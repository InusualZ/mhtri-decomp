---
id: 101
title: A materialised 0/1 that is tested again is an inlined helper's return
status: works
problem: retail sets `li r0,0` / `li r0,1` and then immediately re-tests r0 (`cmpwi r0,0; bne`) where ours branches straight to the return
tags: [source-shape]
applies: []
demo: 
---

# 101. A materialised 0/1 that is tested again is an inlined helper's return

**Problem.** Retail computes a boolean into a register (`li r0,0` on one path, `li r0,1` on the other, or
`neg/or/srwi` for an `x != 0`) and then tests that register again (`cmpwi r0,0; bne ...; li r3,0`). Our
`if (!f(...)) return 0;` chains branch straight to the return, so every such step is 3-5 instructions short.

**Why it happens.** MWCC inlines a `static inline` helper that returns 0/1 and keeps the helper's return value
as a value: the caller's `if (!helper(...)) return 0;` tests it again instead of threading the helper's exits
into the caller's. A helper ending in `return g(...) != 0;` gives the `neg/or/srwi` form.

**How to work it.** Find the steps in the diff that end in `li rX,0/1` + `cmpwi rX,0`, and move each step into
its own `static inline` helper returning 0/1 (early `return 1`s for the guard paths, `return call(...) != 0` where
retail shows `neg/or/srwi`). The caller keeps one `if (!helper(...)) return 0;` per step. If the caller itself is
a small wrapper, keep it from being inlined into its own caller with `#pragma auto_inline off` around it.

**Result.** `DWCi/dwc_nasfunc` (GameSpy GT2): `DWCi_requestIsTimedOut` 81.57 -> 100 (keep-alive / resend /
ack "think" helpers); `gti2HandleUnreliableMessage` 75.69 -> 100 (one helper per message type, if-chain
dispatch).

**Example.**

```c
static inline s32 keepAliveThink(Conn* c, u32 now) {
    if (now - c->lastSend > 30000) {
        if (!sendKeepAlive(c)) {
            return 0;
        }
    }
    return 1;
}

int think(Conn* c, u32 now) {
    if (!checkTimeout(c, now)) {
        return 0;
    }
    if (!keepAliveThink(c, now)) {   /* retail: li r0,0/1 then cmpwi r0,0 */
        return 0;
    }
    return ackThink(c, now) != 0;    /* retail: neg/or/srwi */
}
```
