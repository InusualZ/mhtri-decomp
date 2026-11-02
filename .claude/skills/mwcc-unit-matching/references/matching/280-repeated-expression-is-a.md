---
id: 280
title: A repeated expression is a CSE temporary coloured ahead of the locals: name it as the last-declared local
status: works
problem: A same-size row whose only diff is one callee-saved register on a local (`li r26,0` / `clrlwi r26,r3,16` / `sth r26` where retail has r27), while a repeated expression such as `colon - str` holds that r26 and the local shares the register with it instead of with the earlier-dying local retail pairs it with
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [63, 165, 243, 244]
---

# 280. A repeated expression is a CSE temporary coloured ahead of the locals: name it as the last-declared local

**Problem.** `memcpy(buf, str, colon - str); buf[colon - str] = 0;` puts the length in r26 and then the later `port`
local in r26 too, where retail has `port` in r27 (sharing with the dead `colon` pointer) and only the length in r26.
Every opcode and every other register already matches; a declaration permute of the named locals never moves it, and
neither does `#pragma opt_lifetimes off`.

**Why it happens.** The repeated `colon - str` is merged by common-subexpression elimination into a compiler
temporary, and temporaries are numbered above every named local (idea 244), so the length is coloured *before*
`port`. It takes the next new callee-saved register, r26, and `port`, coloured after it and not interfering with it,
scans the handed-out registers from the lowest up (idea 243) and lands on r26 as well. Retail colours the length
last: `port` sees r27 free (the colon pointer is dead by then) and the length then takes a fresh r26.

**How to work it.** Name the repeated expression as a local **declared last** in the function (`u32 len;` after
every other declaration), assign it once (`len = colon - str;`) and use the local in both places. The lowest-numbered
local is coloured last, after the locals that must share registers with the earlier-dying values. Keep the store's
base as retail has it (`str = buf; str[len] = 0;` when retail writes `stbx r0,r28,r26` through the reassigned
pointer).

**Result.** `DWCi/gt2Utility` (Wii/1.3, `cflags_base` -O4,p): `DWCi_parseAddress` 99.80 -> 100, the unit 99.87 ->
100 (3/3 rows), flipcheck READY and the trial flip links the DOL byte-identical. Tried and no gain on the same
row: `#pragma opt_lifetimes off`, `port` declared first, `value` declared first; `int value` for the `atoi` result
cost 10.4 B.

**Example.**

```
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort)
{
    char buf[0x108];
    char* colon;
    char* p;
    u32 addr = 0;
    u16 port;
    u32 value;
    u32 len;                       /* declared last: coloured after port, takes the new r26 */
    ...
                len = colon - str;
                memcpy(buf, str, len);
                str = buf;
                str[len] = 0;      /* stbx r0,r28,r26 */
    ...
            port = (u16)value;     /* clrlwi r27,r3,16 - shares r27 with the dead colon pointer */
```
