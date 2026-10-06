# The nw4r units

The NintendoWare for Revolution library units under `src/nw4r/` (`nw4r::math`, `nw4r::ut`, `nw4r::db`). The `g3d` and
`ef` parts of the library keep their notes in `docs/g3d.md` and their unit headers.

## Compiler and flags

The `nw4r` lib block in `configure.py` compiles `nw4r/db_assert.cpp`, `nw4r/math_arithmetic.cpp`,
`nw4r/math_triangular.cpp`, `nw4r/fn_805012C4.cpp` and `nw4r/fn_80502828.cpp` with **GC/3.0a5.2** and `cflags_nw4r` (`cflags_os` plus
`-fp_contract off`). The other `nw4r/` stubs stay in the `OS` block until their bodies are measured.

Evidence, from `mt.py matrix` on the written bodies:

| compiler | math_triangular | math_arithmetic | db_assert | fn_805012C4 (37 written rows) | fn_80502828 (Font/ResFontBase, 27 rows) |
|---|---|---|---|---|---|
| GC/3.0a3 .. GC/3.0a5.2, `-fp_contract off` | 5/5 at 100 % | 2/3 (FLog 97.6 %) | 5/6 (Warning 99.3 %) | 33/37 | 27/27 |
| Wii/1.0RC1 .. Wii/1.7, Wii/0x4201_127 | 1/5 | 1/3 | 5/6 | 25/37 | 20/27 |
| GC/2.6, GC/2.7 | below the GC/3.0a rows | | | | |

What separates them:

* **Fast casts.** The SDK's `OSf32tou16`-style inlines pass `&local` through a `register` pointer into `psq_st`/
  `psq_l`. GC/3.0a folds it into `psq_st fN,d(r1)` as retail does; every Wii compiler keeps an `addi rN,r1,d`
  (+4 B per cast: SinFIdx 104 vs 112 B, FExp 140 vs 156 B).
* **Fusion.** Retail keeps `fmuls`+`fadds` (SinFIdx, CosFIdx, VEC3TransformNormal, AABB::Set); `-fp_contract on`
  fuses them into `fmadds`.
* **Load merging.** CharStrmReader reloads `mCharStrm` for each access; the Wii compilers merge the loads.
* `-inline deferred` is ruled out: it scrambles the `.sdata2` pool order (math_triangular, math_arithmetic) and the
  `.data` string order (db_assert) away from retail's.

## `-ipa file` (measured, not in the flags)

The ut files inline functions defined further down and still emit in source order: CharWriter's constructor
inlines UpdateVertexColor (0x805045A0, defined below it), LinkListImpl's destructor inlines Clear, and
TagProcessorBase's members come out in vtable order (Process before CalcRect). Only `-ipa file` reproduces all three
on GC/3.0a3..3.0a5.2: plain `-inline auto` does not inline from below, and `-inline deferred` / `#pragma
defer_codegen on` inline but emit the functions in reverse. Under `-ipa file` TagProcessorBase::CalcRect goes to
100 %, the `.sdata2` pools keep their order, and FExp drops to 99.89 % (a stack slot) until its helpers are re-tuned.
The units stay on `-ipa`-free flags until that is ruled; the two `defer_codegen` scopes stand in for it.

## Source shapes that the codegen depends on

* Stack temporaries are laid out in inline-creation order, shallowest inline first, so the depth of the helper that
  owns a temporary decides its slot (FExp's FAddExpPart, AtanFIdx_ calling the OS casts directly).
* Paired-single bodies are C functions with one inline-asm block over `register` locals; MWCC schedules and
  colours the block with the surrounding code, so declaration order and variable reuse pick the registers.
* The ut files that inline a function defined further down (LinkListImpl's destructor, CharWriter's
  constructor) were built with deferred code generation; the math files were not (deferral reorders their
  `.sdata2` pools). `#pragma defer_codegen on` scopes it to the ut part of a unit.
* An unreferenced public function (`AtanFIdx`, `Assertion_ShowConsole`) is kept where retail's pool order or
  inlining shows it; the link drops it.
