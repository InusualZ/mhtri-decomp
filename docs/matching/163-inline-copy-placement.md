---
id: 163
title: Out-of-line inline copies land after their first user in reverse first-use order, their strings in reverse definition order
status: works
problem: The unit's `.text` holds every function but in a permutation of retail's order (accessors and implicit copies placed elsewhere or inlined), or its weak `@STRING@` tail in `.data` is in another order.
tags: [source-shape, sections, data]
applies: [Wii/1.3]
demo: 
related: [80, 107, 116, 159, 174]
---

# 163. Out-of-line inline copies land after their first user in reverse first-use order, their strings in reverse definition order

**Problem.** Retail g3d and ef units call their inline accessors (`_ac.h` `IsValid`/`ref`/`ptr`, handle copies,
compiler-generated `operator=` and copy constructors) out of line, and each weak copy sits at a fixed place in
`.text`. Ours inlines them or emits them in another order, and the strings of those inline functions (the weak
`@STRING@` tail of `.data`, idea 80) come out permuted.

**Why it happens.** An `inline` function that is not inlined is emitted as a weak copy. Defined **after** its first
referencer, it is placed at its definition. Defined **before** it and kept out of line by `#pragma dont_inline on`,
it is placed right after the first function that references it; several copies come out in **reverse first-use
order**, a nested callee right after its caller. Template instances (the `std::sort` family, `std::max`) follow the
same rule. The weak strings of those inline functions are emitted in **reverse definition order**.

**How to work it.** Write natural nw4r C++ (the accessors and implicit copies as the library's inlines) under a
file-scope `#pragma dont_inline on`, define the functions in retail's `.text` order and let the copies fall into
place. Order the inline functions' definitions so their string tail reverses to retail's. A copy-construction of a
by-value argument stays inline (`lwz`/`stw`); a POD struct assignment becomes an out-of-line memberwise call (idea
162). A copy that retail places where a dead-stripped referencer sat needs that referencer back (idea 174).

**Result.** Wii/1.3, file-scope `dont_inline`: `g3d/g3d_draw` 38 -> 99.37, `g3d/g3d_draw1mat1shp` 53.13 -> 97.32,
`g3d/g3d_light` 23.27 -> 99.91 (the copies become calls, in retail's order). `g3d/g3d_calcmaterial` `.data` 0x158
byte-identical once its five asserting constructors were defined TexObj, TlutObj, TexSrt, TevColor, IndMtxAndScale.
`ef/ef_util` 99.96 -> 100 with BA78 defined before B840. Light and calcmaterial are now `Matching`.

**Example.**

```
#pragma dont_inline on
inline bool ResMat::IsValid() const { return mpData != NULL; }   /* defined first ...                */
void Draw(ResMat m) { if (m.IsValid()) ... }                     /* ... its copy lands after Draw */
```
