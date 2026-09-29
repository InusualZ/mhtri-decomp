---
id: 63
title: A local's DECLARATION ORDER colours registers - locals are coloured before parameters
status: works
problem: `NHTTP/NHTTP_bgnend`'s `NHTTPi_Startup` measured **0 %** with an instruction stream that was otherwise byte-equivalent: the message-group address was materialised late (`lis r31`) where retail materialises it first (`lis r30`, with the parameter getting `r31`), and that single swap shifts every use after it. The residual reads as allocator luck, and no shape *inside* a statement moves it.
tags: [allocator, source-shape, measurement]
applies: []
demo:
---

# 63. A local's DECLARATION ORDER colours registers - locals are coloured before parameters

**Problem.** `NHTTP/NHTTP_bgnend`'s `NHTTPi_Startup` measured **0 %** with an instruction stream that was
otherwise byte-equivalent: the message-group address was materialised late (`lis r31`) where retail
materialises it first (`lis r30`, with the parameter getting `r31`), and that single swap shifts every use
after it. The residual reads as allocator luck, and no shape *inside* a statement moves it.

**Why try it.** MWCC's allocator colours the **local** webs before the **parameter** webs, so the order the
locals are declared in is the order their webs enter the colouring - a local that must live in `r31` has to
be written **before** its siblings. A parameter can never be moved to the front (proved: copying the
parameter to a local first did not do it), because its web is coloured after every local's however it is
written. This is row 18 with a mechanism: there "declaration order" meant the operands of one expression;
here it means the order of the declarations above the body.

**Result.** Two independent filers, which is the register's own promotion rule:

* `NHTTPi_Startup` **0 -> 57.16 -> 93.85 %** with `const char* messages = NHTTPi_startupMessages;` declared
  **first** (unit 75.19 -> 89.36); `NWC24SuspendScheduler` / `NWC24ResumeScheduler` to **98.85 / 95.67 %**
  with `NWC24RequestWork* work = &sNwc24Work;` first;
* `Network/fn_8041A87C`'s `ConnectToAnybody` **99.40 -> 100** on `s32 phase; s32 state;` (retail `r31`/`r30`)
  after two other shapes measured byte-identical.

**Example.** The whole change is where the first line sits:

```c
s32 NHTTPi_Startup(u32 group)                 /* retail: `lis r30` for the table, parameter in r31 */
{
    const char* messages = NHTTPi_startupMessages;   /* FIRST - its web is coloured before the parameter's */
    u32 i;
    ...
}
```
