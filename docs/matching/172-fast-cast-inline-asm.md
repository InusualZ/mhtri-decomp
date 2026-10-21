---
id: 172
title: Quantised fast-cast loads and stores are inline asm psq_l/psq_st with a GQR index
status: works
problem: Retail converts a quantised `s16`/`u8` with one `psq_l fD,0(rA),1,N` (or stores with `psq_st ...,1,N`) where ours, written as a C cast, emits `lha`/`lbz` plus the int-to-float magic sequence.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [85, 104, 164, 173]
---

# 172. Quantised fast-cast loads and stores are inline asm psq_l/psq_st with a GQR index

**Problem.** Animation key readers load a quantised value with a single paired-single quantised load through a GQR
(set up by `OSInitFastCast` and the g3d setters, idea 164). A C cast compiles to an integer load and the
`xoris`/`lfd`/`fsubs` conversion, so the row is far off.

**Why it happens.** MWCC does not emit `psq_l`/`psq_st` from C (idea 85); retail used the SDK's fast-cast inlines,
which are `asm` blocks.

**How to work it.** Write the conversion as a small function with an `asm` block over `register` locals
(`psq_l result, 0(p), 1, N`, `w = 1` for one value, `N` the GQR index), plus the integer load or store where retail
has one. A 0x14 map row that is `b` + `psq_l` + `blr` is two functions: ask for the split before matching it.

**Result.** Wii/1.3: `g3d/g3d_resanm` fn_80089F78 (`psq_st` through GQR5, then `lha`) and the `g3d/g3d_resanmchr`
quantised readers and writers (fn_8008AFE4, fn_8008B78C, fn_8008BFBC, res_anm_chr_load_qr7, fn_8008B704, fn_8008BEF0,
through GQR2/3/6/7) reproduce retail's casts exactly; the lane that wrote them took `g3d/g3d_resanm` 95.71 -> 97.43 and
`g3d/g3d_resanmchr` 85.35 -> 86.20 (with other levers; the per-row figures were not recorded).

**Example.**

```
f32 fn_8008AFE4(register const f32* p) {
    register f32 result;
    asm { psq_l result, 0(p), 1, 3 }
    return result;
}
```
