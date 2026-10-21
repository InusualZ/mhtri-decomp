---
id: 173
title: An inline asm block inside a C loop is unrolled with its displacements rebased
status: works
problem: Retail has a long straight run of paired-single stores or loads (`psq_st f0,0..248(r30)`, a 4x `psq_l`/`ps_madd` key loop) that no C loop produces and an asm function reproduces only by writing every copy out.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [19, 85, 104, 172]
---

# 173. An inline asm block inside a C loop is unrolled with its displacements rebased

**Problem.** A fill or blend loop in retail is an unrolled run of paired-single instructions with rising
displacements and a counter loop around it. C cannot emit the instructions (idea 85); an `asm` function has to spell
every copy and still misses the loop control.

**Why it happens.** MWCC treats an `asm { }` block inside a C loop body like ordinary code: the loop unroller copies
it and rebases each copy's displacements by the stride. Register variables named in the block keep their
compiler-chosen registers, and the function's FPR saves become `stfd` + `psq_st`.

**How to work it.** Write the loop in C, with one iteration's paired-single work as an `asm` block
over `register` locals, and let the unroll factor come out of the compiler. Ruled out for the fill: `f64` stores
(`stfd`), `__vec2x32float__` (`ps_merge00` + `psq_stx`) and `__psq_st0` (undefined).

**Result.** Wii/1.3: `g3d/g3d_cpu` ZeroMemory32ByteBlocks 76 -> 100 (a 4-store block, unrolled 8x; the unit is
`Matching`); `g3d/g3d_calcvtx` fn_8007270C 68.8 -> 93.0 (the key loop unrolled 4x as retail); `g3d/g3d_calcworld`
`g3d_calc_skinning` 0 -> 99.89 (register-variable accumulators f31..f26).

**Example.**

```
register f32 value = 0.0f;
register u8* dst = (u8*)pDst;
size /= 32;
while (size != 0) {                     /* unrolled 8x: psq_st f0,0..248(r30) */
    asm {
        psq_st value, 0(dst), 0, 0
        psq_st value, 8(dst), 0, 0
        psq_st value, 16(dst), 0, 0
        psq_st value, 24(dst), 0, 0
    }
    dst += 32;
    size--;
}
```
