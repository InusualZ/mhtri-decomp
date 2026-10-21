---
id: 107
title: Retail calls an implicit copy constructor out of line: dont_inline at end of TU
status: works
problem: retail calls a two-level out-of-line handle copy (`bl Derived copy` -> `bl ResCommon copy`) where our build copies the word inline
tags: [flags, source-shape]
applies: [Wii/1.3]
demo: 
related: [61, 159, 162, 163]
---

# 107. Retail calls an implicit copy constructor out of line: dont_inline at end of TU

**Problem.** The nw4r g3d units call a one-word handle copy out of line (`res_mat_copy_ctor` 0x800640EC ->
`res_mat_common_copy_ctor` 0x8006411C, `res_anm_chr_copy_ctor` -> `res_anm_chr_common_copy_ctor`, the ResMdl copy
0x80077E34 in the ScnMdlSimple constructor) where MWCC under the project's `-inline noauto` copies the word inline.
Every such site has so far been written as an explicit call to a C-linkage helper.

**Why it happens.** MWCC generates an implicit copy constructor out of line, as a weak function in the TU, whenever
inlining is suppressed. When `#pragma dont_inline on` is still in force at the end of the TU (where the implicit
functions are generated), it emits both levels, `__ct__1RFRC1R` calling `__ct__6RC<1D>FRC6RC<1D>`. A
`push`/`dont_inline on`/`pop` around one function emits only the derived level, with the base copy inlined into
it. `-inline off` on the command line emits both levels. This matches retail g3d, where every accessor
(`GetResMdl`, `ref`, `ptr`, `IsValid`) is also a call.

**How to work it.** Put `#pragma dont_inline on` at file scope and write the accessors and copies as the library's
inlines; where the copies land is idea 163. Measure the whole unit: every inline function and template defined in
a header then becomes an out-of-line weak copy, and each weak copy must be a row the target object actually has.
The map rows of the generated copies must also carry the compiler's manglings.

**Result.** The scratch compile (Wii/1.3, the g3d command line) shows the two-level weak pair only when
`dont_inline` is on at the end of the TU. On units, file-scope `dont_inline`: `g3d/g3d_draw` 38 -> 99.37,
`g3d/g3d_draw1mat1shp` 53.13 -> 97.32, `g3d/g3d_light` 23.27 -> 99.91 (ResMat/ResShp/MdlZ `operator=` and the
implicit copy constructors become calls in retail's order). Lib-wide `-inline off` is ruled out: ten g3d units drop
(idea 174). A POD record assignment also goes out of line under it, where retail copies inline (idea 162).

**Example.**

```
template <typename T> struct RC { T* p; explicit RC(void* d); RC() {} };
struct D { int x; };
struct R : RC<D> { explicit R(void* d); };
struct H { H(R r); R m; int k; };
#pragma dont_inline on
H::H(R r) : m(r) { k = 1; }      /* bl __ct__1RFRC1R; R's copy then bl __ct__6RC<1D>FRC6RC<1D> */
```
