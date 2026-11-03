---
id: 305
title: A commutative sum of two products emits the simpler product first: name the complex operand to keep the pool order
status: works
problem: Two adjacent .sdata2 pool entries are swapped against the target and a few FPR temporaries around their loads shift by one register, while every opcode of the row already matches; swapping the two terms in the source changes nothing.
tags: [source-shape, data, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 211, 278]
---

# 305. A commutative sum of two products emits the simpler product first: name the complex operand to keep the pool order

**Problem.** `v = (p_l - (t - p_h)) * lg2 + t * lg2_l;` (fdlibm `__ieee754_pow`) compiles to the right opcodes, but the
unit's `.sdata2` has `lg2_l` pooled before `lg2` where the target pools `lg2` first (16 bytes differ at +0xC8), and the
FPR colouring around the two loads differs (`v`/`p_h` of the earlier log2 block end in f7/f4 instead of f4/f3, 27 bytes
of `.text`). Writing the sum as `t * lg2_l + (...) * lg2`, putting the constants first, or parenthesising the two
products gives the identical object - the textual order of the two terms is not what the compiler reads.

**Why it happens.** The first PCode dump (`tools/mwcc-debugger`, `backend-03`) shows the frontend emitting the
`t * lg2_l` product - its constant load, the `t` reload, the `fmul` - *before* the `(p_l - (t - p_h)) * lg2` one,
whichever side of the `+` it is written on: for a commutative operator the simpler operand (fewer nodes) is evaluated
first, and a product's constant is always placed first (`fmul f54, f226(lg2_h), f227(t)` for `t * lg2_h`). The pool is
written in emission order, so the constant of the simpler product is pooled first; and the constant-load temporaries are
numbered in emission order too, so the `lg2_l` load (coloured first, descending vreg) takes f3 and pushes the later
`p_h` web to f4 and `v` to f7. In the target the `lg2` product came first, i.e. retail's `+` had two operands of equal
shape.

**How to work it.** Hoist the complex operand into a named local (an existing one whose live range is free at that
point is enough: `w = p_l - (t - p_h); v = w * lg2 + t * lg2_l;`) so both products are `local * constant` and the sum
keeps its source order. A `v = A * lg2; v += t * lg2_l;` split pools correctly but assigns the first product to `v`
itself (retail keeps it a temporary) and loses bytes.

**Result.** `MSL/e_pow` (Wii/1.3, `-O4,p`, `fp_contract off`): `__ieee754_pow` 99.74855 -> 100 %, `.sdata2` 272 B
byte-identical, flipcheck READY, trial flip `main.dol: OK`. Measured and ruled out on the same statement: both term
orders, constant-first operands, both products parenthesised (all 99.74855 % with the pool still inverted), `-`
with the negated constant (99.88 %, inverted), the two-statement `+=` form (99.92 %, pool right).

**Example.**

```
u = t * 6.93147182464599609375e-01;
w = p_l - (t - p_h);                       /* was inside the sum: the frontend then emitted t*lg2_l first */
v = w * 6.93147180559945286227e-01 + t * -1.90465429995776804525e-09;
z = u + v;
w = v - (z - u);
```
