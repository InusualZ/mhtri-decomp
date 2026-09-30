---
id: 97
title: A record passed by value to a virtual slot: give the pointer slot an inline by-value overload
status: works
problem: Retail builds a 12-byte error record twice (the record, then its argument copy with the constants re-materialised) and passes the copy's address through `lwz r12,0(r3)` / `lwz r12,0x288(r12)`; the pointer-typed virtual gives one record, and a hand-written `u32 info[6]` emulation scatters the frame.
tags: [source-shape, vtable]
applies: [Wii/1.3]
demo: 097-by-value-virtual-overload.cpp
reviewed: 2026-09-30
related: [52, 68, 76]
---

# 97. A record passed by value to a virtual slot: give the pointer slot an inline by-value overload

**Problem.** `NetworkInstance::postError(NetworkErrorInfo)` takes a 12-byte record *by value*. MWCC passes such a
struct by address of a caller-made copy, so every retail call site shows the record built in one stack object and
its constants stored again into a second, whose address is passed. A slot declared `virtual void post(Rec* p)` gives one
object; an earlier lane emulated the copy with a per-site `u32 info[6]` written half-first (74.25 -> 99.89) but the
frame layout still differed (`applyEvent` 94.96 with the five 12-byte frame objects landing as two 24-byte arrays).

**Why it happens.** A by-value parameter of a virtual member is lowered as copy-then-pass-address; declaring the
virtual with a pointer parameter removes the copy, and the class cannot carry both as virtuals (a second virtual of
the same name is a new slot).

**How to work it.** Keep the virtual exactly as the vtable has it (pointer parameter) and add an **inline
overload by value** that forwards: `inline void post(Posted info) { post(&info); }`. The caller's `post(error)` then
builds `error`, copies it into the overload's parameter temporary and calls the slot with that copy's address.
Other callers of the pointer form are untouched, so a class shared by several units does not change for them.
The record type lives beside the class (here `NetworkPostedError`, 0x0C).

**Result.** `Network/fn_8041A87C` (2026-09-30): the four `postError` sites lose their `u32 info[6]` emulation,
`gt2ConnectAttemptCallback`/`gt2ConnectedCallback` stay 100 %, `applyEvent` 94.96 -> 100 together with the
register fixes; `NetworkSessionManagerPat::move` (pointer caller) and `network_state` are unchanged.

**Example.**

```
class Dispatch { public: virtual void post(Posted* info); inline void post(Posted info) { post(&info); } };
Posted error; error.code = 0x80000000; error.param1 = 0; error.param2 = 0;
d->post(error);     // record + argument temporary, slot gets the temporary's address
```
