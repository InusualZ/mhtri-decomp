---
id: 307
title: A fully unrolled clear loop's zero merges with its 0-valued counter: start the counter at 1
status: works
problem: A constant-count `itd[i] = 0` loop beside a run of field copies has every opcode of retail, but the zero lands in a volatile register retail does not use (r4) and the copy temporaries take r0 where retail has r3 - a colouring-only residual of one block
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [184, 243, 306]
---

# 307. A fully unrolled clear loop's zero merges with its 0-valued counter: start the counter at 1

**Problem.** `for (i = 0; i < 16; i++) itd[i] = 0;` after seven `dst->x = pb->x` halfword copies compiles to retail's
instruction stream exactly - the `li` of the zero hoisted between the first copy's load and store, sixteen `stw` after the
pointer load - but the zero sits in r4 and the seven copy temporaries in r0, where retail has the zero in r0 and the copies
in r3. `regsim verify` is EXACT and `regsim want` names the nodes: the named local `i` (ours r4, retail r0) and seven
backend temporaries (ours r0, retail r3). No declaration order moves it: `i` is a named local, numbered below every
temporary, so it is always coloured after the copies.

**Why it happens.** The frontend unrolls the loop by 8 (`i += 8`, compare 0x10, a `li rN,0` per store), `loop-transforms`
unrolls it fully, and the dead counter is then constant 0. The `global-optimization` pass (dump 25 of `tools/mwcc-debugger`)
sees two nodes holding the same constant - the zero temporary `r78` and `i` (`r33`) - and rewrites the sixteen stores to read
`r33`: the zero now lives on the **named** node, coloured after the high-numbered copy temporaries, which take r0 first.
Retail's zero is on a temporary numbered above the copies (coloured first, r0; the copies, which interfere with the hoisted
`li`, then take r3). The frontend's and the backend's numbering (ideas 243/306) are not the lever: the merge happens after
them, on the value.

**How to work it.** Keep the counter away from the value 0 so the pass has nothing to merge: a one-based index
(`for (i = 1; i <= 16; i++) itd[i - 1] = 0;`) keeps the ascending store order and the zero on its own temporary. A countdown
(`for (i = 16; i > 0; i--) itd[i - 1] = 0;`) fixes the registers too but reverses the stores (+5.56 of +6.2 B); an inline
`while (count--) *p++ = 0;` helper loses 36 B (its locals become named nodes and the pointer walk changes the stream). Read
dump 25 before and after: the stores must read a temporary, not the counter.

**Result.** `AX/AXVPB` (Wii/1.3, -O4,p): `__AXServiceVPB` 99.54 -> 100 (1360 B), unit 16/17 -> 17/17 rows, 100.0000 %;
rawsame clean; trial flip `main.dol: OK` (sha1 bf4850739478caaedfe675949eb7c28595a7fde9).

**Example.**

```
        itd = vpb->itdBufferData;
        for (i = 1; i <= 16; i++) {     /* zero-based: global optimization folds the zero onto `i`'s node (r4) */
            itd[i - 1] = 0;
        }
```
