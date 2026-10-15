---
id: 142
title: Two stack homes of one pointer mean the source handed a copy to some callees
status: works
problem: Retail keeps two stack homes for a pointer parameter whose address is passed to out-of-line decoders (two `addi rX,r1,N` bases), where ours has one.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [123, 129]
---

# 142. Two stack homes of one pointer mean the source handed a copy to some callees

**Problem.** A report handler passes the *address* of its sample pointer to out-of-line decoders
(`decode(chan, &sample, ...)`), so the pointer lives on the stack. Retail has two stack homes (two different
`addi r4,r1,N` bases at the calls) where ours has one: some decoders get one address, the others and the
handler's own stores use the second.

**Why it happens.** The original copied the pointer (`WPADSample* decoded = sample;`) and handed `&decoded` to
some callees and `&sample` to others; each address-taken variable gets its own home.

**How to work it.** Read which callee receives which base in retail, add the copy, and route the same callees
through it; try both assignments (the copy for the handler's own uses and `&sample` for the decoders, and the
reverse).

**Result.** `WPAD/WPADHIDParser` (Wii/1.3): `wpadReportButtonsAccelDpd10Ext6` -> 100 (the copy for its own
stores, `&sample` for the Nunchuk and Classic decoders); `wpadReportButtonsAccelExt16` 81.6 -> 99.7.

**Example.**

```c
WPADSample* decoded = sample;                                      /* the second home */
...
wpadDecodeNunchukAt(chan, report + 6, &decoded);                   /* these get the copy */
wpadDecodeExtDualStick(chan, &decoded, cb->extensionSubType, report + 6, 0x10);
...
wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 3, 10);       /* this one the original */
```
