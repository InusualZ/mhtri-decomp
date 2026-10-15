---
id: 138
title: Stores into a record follow the source's statement order: write the fill in the SDK's order
status: works
problem: A descriptor or request initialisation stores the right values to the right offsets but in another order, so a long run of `stw`/`sth`/`stb` reads as a scheduling residual.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [18, 47]
---

# 138. Stores into a record follow the source's statement order: write the fill in the SDK's order

**Problem.** A function that fills a record (a DSP task descriptor, a USB control request) has every store of
retail, but in another order; the first divergence is a store, and the rows after it are permuted.

**Why it happens.** MWCC keeps independent stores in the source's statement order (the scheduler rarely
reorders two stores to the same base). Retail's order is the SDK author's: fields first and flags last for a
DSP task, request type then the byte-reversed halfwords for a USB setup packet - not the struct's field order.

**How to work it.** Read the store offsets off retail's code and write the assignments in that order. For more
than three or four stores whose order is unclear, enumerate permutations with a scored search (idea 47);
24-120 orders compile in seconds.

**Result.** Wii/1.3: `AX/AXOut` `__AXOutInitDSP` 79.41 -> 87.76 (+8.35) with the descriptor fields first and the
ready flag, priority and done flag after them; `USB/usb` `__CtrlMsgInt` 97.74 -> 98.34 with the request type,
the three `sthbrx` halfwords, the request byte and the stage byte (all 120 orders measured; `sthbrx` stays three
slots early, the residual). Ruled out on `TRK/msghndlr` `TRKDoDisconnect`: all 24 reply field orders, no gain -
its residual is not store order.

**Example.**

```c
__AXDspTask.iramMmemAddr = __AXDspImage;        /* the descriptor fields first */
__AXDspTask.iramLength = __AXDspImageSize;
...
__AXDspTask.requestCallback = __AXOutRequestCallback;
__AXOutDspReady = 0;                            /* then the ready flag, priority and done flag */
__AXDspTask.priority = 0;
__AXOutTaskDone = 0;
```
