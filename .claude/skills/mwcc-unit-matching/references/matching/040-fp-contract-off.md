---
id: 40
title: A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`
status: works
problem: Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default `-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later register shifts. It reads as a source-shape problem and sends you rewriting expressions that were already right.
tags: [flags]
applies: []
demo:
---

# 40. A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`

**Problem.** Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default `-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later register shifts. It reads as a source-shape problem and sends you rewriting expressions that were already right.

**Why try it.** `#pragma fp_contract off` (or `-fp_contract off`) turns the contraction off, the two instructions come back, and the expression can stay natural. The recurring shapes are `2.0f*x - 1.0f` and `1.0f + rate*t`.

**Result.** 7 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/80073398_fn_80073398` - #pragma fp_contract off (file-scoped) - fn_80075940: 87.42 -> 93.47; unit 72.0 -> 73.14
* `auto/800898B0_fn_800898B0` - #pragma fp_contract off (file scope) - fn_80089B88 79.12 -> 100.0 (MWCC fused four fmuls+fadds pairs; retail has none)
* `auto/800C9DD0_fn_800C9DD0` - #pragma fp_contract off - fn_800C9DD0: 99.94403 % (unchanged - temporaries reproduce the same unfused pairs); the natural expressions become usable, and auto/800CB948 measured 4 fused ops removed by the same flag
* `auto/800CB948_fn_800CB948` - -fp_contract off - fn_800CB948: 4 `fmadds`/`fmsubs` -> retail's separate `fmuls`+`fadds`/`fsubs`; no other function moves
* `auto/800CC5B0_fn_800CC5B0` - -fp_contract off - fn_800CC5B0: 84.92 -> 85.92 %
* `auto/800CCFB0_fn_800CCFB0` - -fp_contract on (auto lib default) vs #pragma fp_contract off - fn_800CCFB0: with `on` MWCC contracts two a*b+c chains into fmadds; the target has only fmuls+fadds. `off` restores the target's FP exactly (the whole 0x408-0x45C block becomes instruction-identical)
* `auto/800CD584_fn_800CD584` - #pragma fp_contract off (scoped to this unit's source) - fn_800CD584: 0 % (fused fmsubs/fmadds/fnmsubs) -> 93.69 % (fmuls/fadds/fsubs as in the target)

**Example.**

```c
#pragma fp_contract off
```