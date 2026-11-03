---
id: 292
title: Walk the tables through named pointers declared after the index when retail colours the index before the walks
status: works
problem: A counted loop over two tables matches every opcode but the index and the two strength-reduced table pointers are rotated (ours r9/r7/r8, retail r7/r8/r9), and no declaration order, index type or pragma moves them
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [110, 111, 243, 244]
---

# 292. Walk the tables through named pointers declared after the index when retail colours the index before the walks

**Problem.** A loop `for (i = 0; i < N; i++) if (A[i] == x && B[i] == y) { A[i] = p; B[i] = q; return; }` is
byte-equivalent except that the index and the two table walks are rotated: ours `li r9,0` / `lwz r0,0(r7)` /
`lwz r0,0(r8)`, retail `li r7,0` / `lwz r0,0(r8)` / `lwz r0,0(r9)`. Declaration order, `u32`/`s32` index, `while`,
nested `if`s, `break`, a dead `i = 0` initialiser, counter locals and scoped `optimization_level`/`opt_common_subs`
pragmas all leave the row where it is.

**Why it happens.** In the indexed form the two walks are induction pointers the frontend optimiser creates (`@147`,
`@149` in the `regalloc-gpr-pass-1-all.txt` of `tools/mwcc-debugger`), and the frontend's `@` temporaries are numbered
*above* every named local (and, among themselves, in reverse creation order: `@151` r35, `@149` r36, `@147` r37, idea
244). Select colours from the highest number down, so the two walks take the first free registers (r7/r8, after the
exit path's base temporaries hold r5/r6) and the index, the lowest node, is coloured last and takes r9. Retail's
`r7/r8/r9` says its index was coloured *before* the walks: the walks are nodes numbered below the index, i.e. named
locals declared after it (named locals number in reverse declaration order). The walked form also fixes the loop
tail: the `for` step is emitted in the source's order, and retail bumps the pointers before the index (the mirror of
idea 110).

**How to work it.** When the index colours *after* the walks in ours and *before* them in retail (read the rotation
from the diff, or `regsim want`), declare the index first and one pointer local per table after it, initialise the
pointers before the loop and step them in the `for` step in retail's tail order; keep the exit-path stores indexed
(`A[i] = p`) when retail recomputes the bases there (`lis`/`addi` + `stwx`). If the walks must colour *before* the
index, keep the indexed form (idea 111).

**Result.** `IPC/ipcProfile` (Wii/1.3, -O4,p): `IPCiProfQueueReq` 97.58 -> 99.03 with the named walks (registers
now retail's, the loop tail `addi i; addi p; addi q` still ours), -> 100 with the step order `reqp++, fdp++, i++`;
unit 3/4 -> 4/4 rows and flipped (`main.dol: OK`).

**Example.**

```
s32 i;
IPCRequest** reqp;   /* declared after i: numbered below it, coloured after it */
s32* fdp;
reqp = IpcReqPtrArray;
fdp = IpcFdArray;
for (i = 0; i < IPC_MAX_REQS; reqp++, fdp++, i++) {   /* retail's tail: pointers, then the index */
    if (*reqp == NULL && *fdp == -1) {
        IpcReqPtrArray[i] = req;   /* exit path stays indexed: lis/addi + stwx */
        IpcFdArray[i] = fd;
        return;
    }
}
```
