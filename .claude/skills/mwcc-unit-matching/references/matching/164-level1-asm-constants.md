---
id: 164
title: A scoped optimization_level 1 keeps an inlined asm block's constants unfolded
status: works
problem: Retail inlines a setter whose body is an `asm` block and keeps its constant arguments' shift/or unfolded, while ours folds the constants into one immediate (or calls the setter out of line).
tags: [pragma]
applies: [Wii/1.3]
demo: 
related: [16, 41, 127]
---

# 164. A scoped optimization_level 1 keeps an inlined asm block's constants unfolded

**Problem.** A GQR setter (`OSSetGQR7(7, 8)`: an `inline` function whose body is an `asm` block over its register
parameters) is inlined in retail with the shift/or of its constant arguments still visible. Ours, at the unit's
`-O3`, folds them; written as an `asm` function, it is called rather than inlined.

**Why it happens.** At level 3 MWCC propagates the constant arguments into the inlined block and folds the
arithmetic around it; `#pragma optimizewithasm off` and `opt_propagation off` still fold. At level 1 the propagation
does not run.

**How to work it.** Bracket the callers with `#pragma optimization_level 1` ... `#pragma optimization_level reset`
(a codegen pragma lives in the TU, rule 14) and keep the setter a `static inline` function with an `asm` body.

**Result.** `g3d/g3d_init` (Wii/1.3): `g3d_set_gqr7_s16_8` and `g3d_set_gqr6_s16_5` 31 -> 87.6. The residual
(retail loads r3 before r4 and branches `b +4` into the body) is in the unit header; no shape tried reaches it.

**Example.**

```
#pragma optimization_level 1
void g3d_set_gqr7_s16_8(void) { OSSetGQR7(7, 8); }   /* inlined asm setter, constants unfolded */
#pragma optimization_level reset
```
