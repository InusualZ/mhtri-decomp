---
id: 40
title: A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`
status: works
problem: Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default `-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later register shifts.
tags: [flags, pragma]
applies: [Wii/1.3]
demo: 040-fp-contract-off.cpp
reviewed: 2026-09-29
related: [33, 39]
---

# 40. A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`

**Problem.** Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default
`-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later
register shifts. It reads as a source-shape problem and sends you rewriting expressions that were already right.

**How it looks.** The target has `fmuls f1,f2,f1 ; fsubs f1,f1,f0` (multiply, then subtract) where ours has a
single `fmsubs f1,f2,f1,f0` ("fused multiply-subtract": one rounding, one instruction). Gekko's fused forms are
`fmadds`/`fmsubs`/`fnmsubs`/`fnmadds` (and the double-precision `fmadd`...). A whole-unit count of retail's
fused ops (zero, or far fewer than ours) is the fingerprint.

**Why it happens.** `-fp_contract on` lets the compiler contract `a*b + c` into a fused op. The original build had
it off (or the source used the pragma), so the two instructions stay.

**How to work it.** `#pragma fp_contract off` (or `-fp_contract off` for the unit, in `configure.py`'s per-library
flags, idea 33) turns the contraction off; the two instructions come back and the expression can stay natural.
The recurring shapes are `2.0f*x - 1.0f` and `1.0f + rate*t`. Restore it with `#pragma fp_contract on`.

```c
#pragma fp_contract off
float scale(float t) { return 2.0f * t - 1.0f; }    /* fmuls ; fsubs, not fmsubs */
#pragma fp_contract on
```

**When NOT to apply.** If the target *does* contain fused ops for some expressions and unfused for others, the whole
unit is not `off`: the difference is then the source's own evaluation order (a temporary that splits `a*b` from `+ c`
prevents contraction on its own, so `float m = a*b; return m + c;` may already give the pair). At `-O3` the
non-contracted version may also schedule the `lfs` constant loads differently (measured in the demo's
neighbourhood), so re-measure the whole function, not just the two instructions.

**Result.** Measured at the time, 7 units independently (names are the old `auto/` buckets, since retired):

* `auto/80073398_fn_80073398` - #pragma fp_contract off (file-scoped) - fn_80075940: 87.42 -> 93.47; unit 72.0 -> 73.14
* `auto/800898B0_fn_800898B0` - file scope - fn_80089B88 79.12 -> 100.0 (MWCC fused four fmuls+fadds pairs; retail has none)
* `auto/800C9DD0_fn_800C9DD0` - fn_800C9DD0: 99.94403 % (unchanged - temporaries reproduce the same unfused pairs); the natural expressions become usable
* `auto/800CB948_fn_800CB948` - 4 `fmadds`/`fmsubs` -> retail's separate `fmuls`+`fadds`/`fsubs`; no other function moves
* `auto/800CC5B0_fn_800CC5B0` - fn_800CC5B0: 84.92 -> 85.92 %
* `auto/800CCFB0_fn_800CCFB0` - with `on` MWCC contracts two a*b+c chains into fmadds; the target has only fmuls+fadds; `off` makes the whole 0x408-0x45C block instruction-identical
* `auto/800CD584_fn_800CD584` - fn_800CD584: 0 % (fused fmsubs/fmadds/fnmsubs) -> 93.69 %

**Demonstration.** `040-fp-contract-off.cpp` (`ideas.py demo-check 40`, base flags): `2.0f * t - 1.0f` compiles to one
`fmsubs` under the default and to `fmuls` + `fsubs` inside `#pragma fp_contract off ... on` (verified 2026-09-29).
