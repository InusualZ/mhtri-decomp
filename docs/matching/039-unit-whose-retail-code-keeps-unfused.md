---
id: 39
title: A unit whose retail code keeps unfused peephole folds needs the peephole pass off
status: works
problem: The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, a record-form `clrlwi.` the target does not have, or an `li r0,<slot>` + `psq_lx`/`psq_stx` epilogue. Our build, with the command line's peephole on, emits the fused form and lands one or two instructions short - a 4-25 % gap that reads as a source problem.
tags: [process]
applies: []
demo:
---

# 39. A unit whose retail code keeps unfused peephole folds needs the peephole pass off

**Problem.** The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, a record-form `clrlwi.` the target does not have, or an `li r0,<slot>` + `psq_lx`/`psq_stx` epilogue. Our build, with the command line's peephole on, emits the fused form and lands one or two instructions short - a 4-25 % gap that reads as a source problem.

**Why try it.** `#pragma peephole off` is the source spelling of `-opt nopeephole`; it turns the pass off for the file or a scoped region and restores the target's unfused form. Several units in the `auto` bucket were built with it off, so it is the first lever to try when the diff is a *fold*, not a shape. Note the level is not the lever: `#pragma optimization_level 1` leaves the command line's peephole on (see the trap below).

**Result.** 14 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - #pragma peephole off (scoped to fn_8007403C) - fn_8007403C: 28.0 -> 100.0
* `auto/800898B0_fn_800898B0` - #pragma peephole off (file scope, cflags_main has peephole on) - fn_80089F94 95.8 -> 100.0, fn_8008A220 95.8 -> 100.0 (record-form `clrlwi.` removed; retail object has zero record forms)
* `auto/8009AA78_fn_8009AA78` - #pragma peephole off (file-scoped) - all 8 symbols -> 100.0 (fn_8009AA78 98.90 -> 100, fn_8009AB28/38 71.25 -> 100, fn_8009AB8C 77.93 -> 100); .text 0x26C / extab 0x28 / extabindex 0x3C byte-identical
* `auto/800C9DD0_fn_800C9DD0` - #pragma peephole off  (== -opt nopeephole) - fn_800C9DD0: 94.24254 -> 99.94403 %; retail's `li r0,136; psq_lx f31,r1,r0,0,0` epilogue and the `clrlwi r4,r30,16` argument narrowing restored; the pragma's object is byte-identical in .text/extab/extabindex to the same source compiled with `-opt nopeephole`
* `auto/800CB948_fn_800CB948` - -opt nopeephole - fn_800CB948: paired-single epilogue restored to retail's `li r0,<slot>; psq_lx` form (0x5a4..0x660 byte-identical); 90.89 -> 97.29 together with -fp_contract off
* `auto/800CC5B0_fn_800CC5B0` - -opt nopeephole - fn_800CC5B0: 84.92 -> 88.64 %
* `auto/800CCCF8_fn_800CCCF8` - #pragma peephole off (per-unit, whole file - the source spelling of -opt nopeephole) - fn_800CCE38: 97.5 -> 100.0; fn_800CCDFC: 99.33 -> 100.0; all 10 symbols 100.0
* `auto/800CCFB0_fn_800CCFB0` - -opt nopeephole (via #pragma peephole off) - fn_800CCFB0: 94.63 -> 97.79, .text 0x5B8 -> 0x5D4 (target 0x5D4). Without it MWCC's peephole folds the epilogue's `li r0,off; psq_lx f,r1,r0` into `psq_l f,off(r1)`, 7 instructions short; the target keeps the indexed form. Same stand-in as the other auto unit (docs/plan.md 6.5)
* `auto/800D7F54_fn_800D7F54` - -opt nopeephole - sysSE_req/fn_800DBC84/fn_800DB4EC/fn_800DC53C 95.0/95.0/92.8/64.4 -> 100/100/100/100; no other function changed
* `auto/800DCFEC_fn_800DCFEC` - -O3 -opt nopeephole + `#pragma peephole off` - fn_800DCFEC 100.0 % (516 B / 516 B) - the committed state
* `auto/800E46E8_fn_800E46E8` - a per-region cflags group with `-opt nopeephole` (everything else as cflags_main), so the whole-file `#pragma peephole off` in this unit's source can go - Same source, real command line from build.ninja, measured with recompile.py: cflags_main as committed (peephole on) = fn_800E46E8 94.81 %, set_stream_main_vol_flag__FUcUc 71.25 %, fn_800E48E4 87.22 %, fn_800E4908 76.67 %; `#pragma peephole off` (the committed state) = 100 % on all four. The signature is the unfused `clrlwi r0,r3,24` (+ `cmp...
* `auto/800FCED4_fn_800FCED4` - #pragma peephole off, scoped to the five functions from fn_800FCED4 on (file scope NOT used) - fn_800FCED4 99.92 -> 100.0, eft002_set 97.73 -> 100.0, eft002_set_shell 99.62 -> 100.0; unit 99.59 -> 100.0; .text 0x64C and extab/extabindex byte-identical; the other five stay 100.0
* `auto/802B2978_fn_802B2978` - -opt nopeephole - fn_802B2978 99.932434 % (296 B, 73/74 rows)
* `auto/80324F7C_fn_80324F7C` - -O3 -opt nopeephole + `#pragma peephole off` - fn_80324F7C 100.0 % (308 B / 308 B) - the committed state

**Example.**

```c
#pragma peephole off   /* the whole unit, or a scoped pair around one function */
```

**Measure it, do not assume it.** On two units this lever is a *regression*: `auto/800FD718` goes 100.00 -> 98.72
(`.text` 0x14C -> 0x150) and `auto/80119C44` 99.61 -> 95.88, where it produces the target's `addi r0` but un-folds two
narrowing stores the peephole was correctly folding. Both targets keep *some* folds, so the pass is not a per-bucket
property - try it, then keep it only if the measurement agrees.
