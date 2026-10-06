---
id: 104
title: A paired-single body is an asm function or an asm block with register locals
status: works
problem: A function full of `psq_l`/`ps_*` sits at 0-60 % as C, and when it is written as `asm`, the C code around the block (an assert) moves to other registers
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [85, 63]
---

# 104. A paired-single body is an asm function or an asm block with register locals

**Problem.** A row whose retail body is paired-single code (`psq_l`, `ps_madds0`, `ps_sum0`, `frsqrte`) stays at
0-60 % however its C is spelled (idea 85: no compiler emits these from C). Once it is written as `asm`, two
residuals remain: the extab/extabindex record is missing or extra, and the C code before the block (an NW4R pointer
assert) takes r7-r12/r28-r29 where retail uses r0 and r3-r10.

**Why it happens.** Retail wrote these in two ways, and the extab table tells them apart. An `asm` function with
`nofralloc` gets no frame and no extab record. A C function whose body is an `asm { }` block gets a frame when the
block uses a local, plus an extab record. In a C function, every physical register the block names (`r0`, `r3`,
`f0`, ...) is reserved for the whole function, so the allocator keeps the assert's temporaries off them. A GPR named
through a `register` local is allocated like any other variable, and the allocator gives it back the same number
retail has (r0 included).

**How to work it.**
1. Read `extabindex` for the function's address (12-byte records `{fn, size, extab}`). If there is no record, write
   an `asm` function with `nofralloc` and the trailing `blr`. If there is a record, write a C function holding an
   `asm { }` block. Replace a scratch slot `addi rX, r1, 8` with a local array and `la rX, work`.
2. Small data: write `lfs f8, sym(r0)` for an `@sda21` load. `sym(r13)` scores 100 in objdiff but encodes rA=13 where
   retail's object leaves rA=0 for the linker, so the raw bytes differ. Write `la r0, sym(r0)` for retail's
   `li r0, sym@sda21` (which feeds `psq_lx f0, r0, r0`). Declare the symbol with its size (`extern const f32 sym[2]`),
   otherwise the relocation comes out as `R_PPC_ADDR16`. Write `lis rX, sym@ha` / `addi rX, rX, sym@l` for a
   `lis`/`addi` pair.
3. A namespaced symbol (`nw4r::math::sSinCosTbl`) cannot be qualified inside `asm`. Add
   `using nw4r::math::sSinCosTbl;` and write the bare name.
4. In a C function with C code besides the block, name every GPR in the block through `register` locals (`tbl`,
   `idx`, `normalized`, ...) rather than `r3`/`r0`. FPRs can stay physical when the C part uses none.

**Result.** `ef/ef_util`: 73.15 -> 98.61 %. `ef_vec_sin_cos`, `ef_sin_cos`, `fn_8009CBA0`,
`ef_mtx34_scale_columns` and `ef_mtx34_column_length` reach 100 % as `nofralloc` asm functions.
`ef_mtx34_rotate_xyz` reaches 100 % as a C function with a block, matching extab 0x60 and extabindex 0x90.
`ef_vec3_normalize_to` goes 62 -> 100 % and `ef_vec3_from_rotation` 1 -> 100 %. Naming its GPRs through
`register` locals took `ef_vec3_from_rotation` from 95.82 to 100 %: physical names had reserved r0 and r3-r6
against the assert.

**Example.**

```
extern "C" VEC3* ef_vec3_from_rotation(register const EfRotation* rot, register VEC3* out) {
    f32 work[2];
    register const u8* tbl;
    register u32 idx;
    register f32* wk;

    NW4R_POINTER_ASSERT(out, 0x3A6, msg);
    asm {
        lis        tbl, sSinCosTbl@ha
        addi       tbl, tbl, sSinCosTbl@l
        la         idx, ef_util_f32_65536_pair(r0)
        psq_lx     f3, r0, idx, 0, 0
        la         wk, work
        ...
    }
    return out;
}
```
