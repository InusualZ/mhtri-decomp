---
id: 39
title: A unit whose retail code keeps unfused peephole folds needs the peephole pass off
status: works
problem: The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`slwi`/`cmpwi`, a record-form `clrlwi.` the target does not have, or an `li r0,<slot>` + `psq_lx` epilogue. A 4-25 % gap that reads as a source problem.
tags: [flags, pragma]
applies: [Wii/1.3]
demo: 039-peephole-off-unit.cpp
reviewed: 2026-09-29
related: [21, 32, 33, 38, 41]
---

# 39. A unit whose retail code keeps unfused peephole folds needs the peephole pass off

**Problem.** The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a
narrowing store, a separate `clrlwi`+`cmpwi` (or `clrlwi`+`slwi`), a record-form `clrlwi.` the target does not have,
or an `li r0,<slot>` + `psq_lx`/`psq_stx` epilogue. Our build, with the command line's peephole on, emits the fused
form and lands one or two instructions short - a 4-25 % gap that reads as a source problem.

**How it looks.** The target has two instructions where ours has one, and the pair is a *fusable* one:

```
target:  clrlwi r0,r3,16 ; slwi r3,r0,2            ours:  rlwinm r3,r3,2,14,29    (rlwinm = rotate-left-word-immediate-and-mask, the fused form)
target:  clrlwi r0,r4,24 ; stb  r0,12(r3)          ours:  stb r4,12(r3)           (store keeps only the low byte anyway)
```

The count of *record-form* instructions (`clrlwi.`, `srwi.` - the trailing dot sets the condition register) in the
target is the cheapest fingerprint: retail with zero of them in the whole unit points at peephole off (idea 21).

**Why it happens.** The peephole pass (a local rewrite of adjacent instructions) merges mask+shift pairs, drops masks
in front of narrowing stores and folds `li`+indexed paired-single loads. The unit's original build had it off
(`-opt nopeephole`), so the unfused forms survive in retail.

**How to work it.** `#pragma peephole off` is the source spelling of `-opt nopeephole`; it turns the pass off for a
file or a scoped region and restores the target's unfused form. It is the first lever to try when the diff is a
*fold*, not a shape. If several functions need it, put `-opt nopeephole` in the library's `cflags_*` in
`configure.py` (idea 33) rather than a pragma per function; a pragma region is not local (idea 32). Note the
*level* is not the lever: `#pragma optimization_level 1` leaves the peephole on (idea 41).

```c
#pragma peephole off   /* the whole unit, or a scoped pair around one function */
...
#pragma peephole reset
```

**When NOT to apply.** Measure it, do not assume it. On two units this lever is a *regression*: `auto/800FD718` goes
100.00 -> 98.72 (`.text` 0x14C -> 0x150) and `auto/80119C44` 99.61 -> 95.88, where it produces the target's `addi r0`
but un-folds two narrowing stores the peephole was correctly folding. Both targets keep *some* folds, so the pass
is not a per-bucket property - try it, then keep it only if the measurement agrees. The `auto/` unit names below are
the names at the time; the bucket has since been retired.

**Result.** Measured at the time, 14 units independently confirmed the same lever, so the evidence is grouped here
instead of one line per outbox:

* `auto/80073398_fn_80073398` - #pragma peephole off (scoped to fn_8007403C) - fn_8007403C: 28.0 -> 100.0
* `auto/800898B0_fn_800898B0` - file scope, cflags_main has peephole on - fn_80089F94 95.8 -> 100.0, fn_8008A220 95.8 -> 100.0 (record-form `clrlwi.` removed; retail object has zero record forms)
* `auto/8009AA78_fn_8009AA78` - file-scoped - all 8 symbols -> 100.0; .text 0x26C / extab 0x28 / extabindex 0x3C byte-identical
* `auto/800C9DD0_fn_800C9DD0` - 94.24254 -> 99.94403 %; retail's `li r0,136; psq_lx f31,r1,r0,0,0` epilogue and the `clrlwi r4,r30,16` argument narrowing restored; byte-identical to the same source compiled with `-opt nopeephole`
* `auto/800CB948_fn_800CB948` - -opt nopeephole - paired-single epilogue restored (0x5a4..0x660 byte-identical); 90.89 -> 97.29 together with -fp_contract off
* `auto/800CC5B0_fn_800CC5B0` - -opt nopeephole - 84.92 -> 88.64 %
* `auto/800CCCF8_fn_800CCCF8` - whole file - fn_800CCE38: 97.5 -> 100.0; fn_800CCDFC: 99.33 -> 100.0; all 10 symbols 100.0
* `auto/800CCFB0_fn_800CCFB0` - fn_800CCFB0: 94.63 -> 97.79, .text 0x5B8 -> 0x5D4 (target 0x5D4); with the peephole on MWCC folds the epilogue's `li r0,off; psq_lx f,r1,r0` into `psq_l f,off(r1)`, 7 instructions short
* `auto/800D7F54_fn_800D7F54` - -opt nopeephole - sysSE_req/fn_800DBC84/fn_800DB4EC/fn_800DC53C 95.0/95.0/92.8/64.4 -> 100 each; no other function changed
* `auto/800DCFEC_fn_800DCFEC` - -O3 -opt nopeephole + `#pragma peephole off` - 100.0 % (516 B / 516 B)
* `auto/800E46E8_fn_800E46E8` - a per-region cflags group with `-opt nopeephole`: cflags_main as committed 94.81 % / 71.25 % / 87.22 % / 76.67 %, `#pragma peephole off` 100 % on all four; the signature is the unfused `clrlwi r0,r3,24`
* `auto/800FCED4_fn_800FCED4` - scoped to the five functions from fn_800FCED4 on (file scope NOT used) - 99.92 -> 100.0, 97.73 -> 100.0, 99.62 -> 100.0; unit 99.59 -> 100.0
* `auto/802B2978_fn_802B2978` - -opt nopeephole - 99.932434 % (296 B, 73/74 rows)
* `auto/80324F7C_fn_80324F7C` - -O3 -opt nopeephole + `#pragma peephole off` - 100.0 % (308 B / 308 B)

**Demonstration.** `039-peephole-off-unit.cpp` (`ideas.py demo-check 39`), Wii/1.3 with the base flags: a
`u8` field store and a `(u32)u16 << 2` index. With the peephole on the store is a single `stb` and the index one
`rlwinm`; inside `#pragma peephole off ... reset` the same code keeps `clrlwi` + `stb` and `clrlwi` + `slwi`
(verified 2026-09-29). The paired-single epilogue and record-form cases were not demonstrated (a paired-single op
cannot be produced from source here, see `notes/paired-single.md`).
