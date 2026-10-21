---
id: 143
title: An SDK library function matches its vendor's own source text, slips included
status: works
problem: A library function (MSL C, the RVL SDK, Broadcom BTE) sits at 89-99 % with a correct-looking reconstruction, and generic shape levers move it little.
tags: [source-shape]
applies: [Wii/1.3, GC/3.0a5.2]
demo: 
related: [101, 145]
---

# 143. An SDK library function matches its vendor's own source text, slips included

**Problem.** A clean, idiomatic reconstruction of a vendor library function is close but not exact, and the usual
levers (temporaries, declaration order, operand order) each move a register or two without closing the row.

**Why it happens.** The retail object was compiled from the vendor's source, and MWCC's output follows that text:
its helper structure (inline helpers with early returns), its expression forms (one conditional expression where
a reconstruction writes two statements), its loop variables and even its copy-paste slips. Spelling the vendor's
form reproduces every allocator decision at once.

**How to work it.** Identify the vendor and the era (MSL_C.PPCEABI of the mid-2000s for `MSL_C`, the RVL SDK for
`MEM`/`WPAD`/`USB`, Broadcom's BTE for `BTE`) and write the function the way that code base writes it: its helper
functions (`__rounddec`, `__must_round`, `RemoveRegion_`, `bta_dm_get_remname`), its expressions and its local
names. Keep its slips (a redundant test, a `snum ? snum - 0x3FF : 0`) - they are codegen.

**Result.** Wii/1.3: `MSL_C/printf` `__FileWrite` 95.0 -> 100 (`return (fwrite(..) == n) ? f : 0`), `double2hex`
89.48 -> 100 (`snum ? snum - 0x3FF : 0`, the exponent from the two top bytes), `long2str`/`longlong2str` -> 100
(digits added in place, the MSL loop variable); `MSL_C/ansi_fp` `__num2dec` 92.05 -> 98.67 through MSL's inline
`__rounddec`/`__must_round`/`__dorounddecup` (100 with idea 124's birth order); `MEM/mem_expheap` follows the RVL
SDK helper structure (`RegionBody_`, `RemoveRegion_`, `InsertRegionAfter_`) to 100. GC/3.0a5.2: `BTE/bta_dm_act`
`bta_dm_sdp_result` 99.81 -> 100 with Broadcom's static `bta_dm_get_remname` inlined at the three discovery sites.

**Example.**

```c
static WriteArg* __FileWrite(WriteArg* file, const char* buffer, u32 count)
{
    return (__fwrite(buffer, 1, count, (FILE*)file) == count) ? file : NULL;
}
```

**MSL's own gotos (rule 8 exemption for `src/MSL_C/` only).** Retail MSL_C was compiled from source with `goto`s,
and only those shapes close the rows: `__str2dec`, `__timesdec`, `__minus_dec` (ansi_fp 99.31 -> 100),
`round_decimal`'s `goto return_zero`, `float2str`'s `goto e_format`/`f_format`, and `__pformatter`'s
`if ((buff_ptr = long2str(...)) == NULL) goto conversion_error;` inside each `long`/`long long` branch, not after them
(98.01 -> 99.47 before the colouring search, then 100 with idea 63's order search); `printf` 98.62 -> 100 and
`wprintf` 98.16 -> 100, all three trial-flipped with the DOL hash unchanged. The owner's ruling (`docs/pipeline.md`
11, 2026-10-15) exempts a `goto` in a file under `src/MSL_C/` and nowhere else.
