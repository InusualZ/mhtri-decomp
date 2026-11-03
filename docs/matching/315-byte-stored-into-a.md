---
id: 315
title: A byte stored into a window and compared afterwards is read back from the window
status: works
problem: "`lbzx` of the byte and the `clrlwi` of the window index hold each other's volatile register (r3/r4 swapped) around a `stbx` into a small window array"
tags: [source-shape, allocator]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
---

# 315. A byte stored into a window and compared afterwards is read back from the window

**Problem.** A loop copies a byte into a two-byte ring (`window[index & 1] = byte`) and then tests the byte; the
code is right but the loaded byte and the `index & 1` temporary sit in each other's volatile registers.

**Why it happens.** With a named `byte = buffer[index];` the load is the named local (lowest number), so the
`index & 1` backend temporary colours first and takes r3. Retail's byte took r3: the value it tests is a
temporary created *after* the index, which is what a read back from the window gives
(`window[index & 1] = buffer[index]; byte = window[index & 1];` - the read is forwarded from the store, so no
load is added, but the node is numbered after the index temporary).

**How to work it.** When a volatile pair around a store-then-test is swapped, store first and read the tested
value back from the stored location; the instruction count does not change.

**Result.** `NHTTP/NHTTP_thread` `NHTTPi_recvResponseBody` 99.99 -> 100 % (the last 5 instructions; the unit
reached 25/25 rows and flipcheck READY).

**Example.**

```
index = context->recvCount;
sizeWindow[index & 1] = buffer[index];
byte = sizeWindow[index & 1];
if ((s8)byte == ';' || ((s8)byte == '\n' && (s8)sizeWindow[(index - 1) & 1] == '\r')) {
```
