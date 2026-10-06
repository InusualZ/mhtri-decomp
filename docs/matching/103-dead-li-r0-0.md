---
id: 103
title: A dead li r0,0 + cmpwi r0,0 after a Panic is the assert written as an expression
status: works
problem: Retail has a branchless `li r0, 0x0` + `cmpwi r0, 0x0` right after an assert's `bl Panic`, ours goes straight on (2 target-only instructions, 8 bytes short).
tags: [source-shape]
applies: [ef]
demo: 
---

# 103. A dead li r0,0 + cmpwi r0,0 after a Panic is the assert written as an expression

**Problem.** The function is instruction-for-instruction equal except for two target-only instructions after an
assert's `bl Panic__Q24nw4r2dbFPCciPCce`: `li r0, 0x0` and `cmpwi r0, 0x0`, with no branch after them. They read
like dead code that no source can produce.

**Why it happens.** The NW4R assert macros are expressions, `(void)((exp) || (Panic(...), 0))`. After the
`Panic` call, the `, 0` arm materialises the result of the `||` (0) and tests it. Both arms then join, so the
branch is dropped, but with `#pragma peephole off` the load and the compare stay. An `if (!exp) Panic(...);`
statement has no value to test, so nothing is left after the call. A `do { ... } while (0)` wrapper is folded away
completely and does not reproduce it.

**How to work it.** Write that one assert as `(void)(IsValidPointer((u32)p) || (nw4r::db::Panic(file, line, msg, p),
0));`. Rewriting *every* assert of a unit into this form changed nothing in the rows without the residue: no row
moved in 9 ef units (ef_drawstripestrategy 36 asserts, ef_drawsmoothstripestrategy 38). So the form is
codegen-neutral except where its value survives, and it does not cure the separate "member load scheduled before
the assert's `li` flags" residual.

**Result.** `ef/ef_drawbillboardstrategy`: `ef_billboard_setup_gx` 98.40 -> 100 and `ef_directional_setup_gx`
98.29 -> 100. `ef/ef_drawfreestrategy`: the free strategy's GX setup (0x800BED2C) 98.29 -> 100.

**Example.**

```cpp
void ef_billboard_setup_gx(nw4r::ef::DrawStrategyImpl* self, const EfDrawInfo* em, EfDrawParticleManager* args) {
    (void)(IsValidPointer((u32)args) ||
           (nw4r::db::Panic(ef_billboard_file_str, 746, ef_billboard_pm_assert_str, args), 0));
    self->InitGraphics(args, *(EfEmitterDrawSetting*)ef_resource_draw_setting(args->resource), *em);
```
