---
id: 131
title: A dead branch after a failure return is an if/else whose else returns
status: works
problem: Retail branches on success (`ble <epilogue>` for the failure) and carries two back-to-back `b` instructions after the success block, where ours inverts the test and is 4 bytes shorter.
tags: [source-shape]
applies: [GC/3.0a5.2]
demo: 
related: [71]
---

# 131. A dead branch after a failure return is an if/else whose else returns

**Problem.** After a call, retail tests `ble <return>` (the failure goes straight to the epilogue with the call's
result still in r3), runs the success block, then has `b <next>` immediately followed by an unreachable
`b <return>`. Ours writes `if (r <= 0) return r;` and gets `bgt` over a `b <return>`: same work, one branch fewer,
every later instruction shifted.

**Why it happens.** MWCC lays an `if (ok) { ... } else { return r; }` out as the then-block, a jump over the else,
and the else's own jump to the return: the else's `b` survives even though the then-block ends with a branch
too. The early-return spelling has no else block, so that dead `b` never appears.

**How to work it.** Where the diff shows the condition sense flipped (`ble` vs `bgt`) next to an extra pair of
`b`s, rewrite the guard as `if (r > 0) { <success>; } else { return r; }`. A related shape: a failure that
retail returns directly without running the shared post-processing (`li r3,-1001; b <epilogue>`) is a
`return` inside the branch, not an assignment to the result variable. The opposite residual (ours has the extra
branch) is idea 71's exit shape.

**Result.** `NHTTP/d_nhttp` (GC/3.0a5.2): `NHTTPi_SocSend_sub` 96.96 -> 100, `NHTTPi_SocRecv_sub` 93.96 -> 99.55
(with its locals reordered); `NHTTPi_SocRecv` 98.18 -> 100 from the direct `return -1001`. The success-first
buffer test of `BTE/btm_inq` (`if ((p_buf = GKI_getpoolbuf(..)) != NULL) { .. } else return BTM_NO_RESOURCES;`,
GC/3.0a5.2) is the same layout in every scan-setting function (unit header SHAPES line).

**Example.**

```c
sent = SOSend(fd, aligned, head, flags);
if (sent > 0) {
    total = sent;
    if ((u32)sent < head) {
        return sent;
    }
    buf += sent;
    length -= sent;
} else {
    return sent;
}
```
