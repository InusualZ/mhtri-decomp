---
id: 85
title: A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop)
status: ruled-out
problem: The target fills memory with paired-single instructions (`psq_st`/`psq_l`) where every compiler we have emits scalar `stfs`/`stfd`; it reads as a codegen lever and eats flag and shape sweeps.
tags: [source-shape, measurement]
applies: [Wii/1.3]
demo: 085-paired-single-op.cpp
reviewed: 2026-09-29
related: [3, 13, 21]
---

# 85. A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop)

**Problem.** A function's residual is a handful of neighbouring opcodes: the target fills a float array with
broadcast paired-single stores (`psq_st f0,off,0,qr0`; a paired-single is a Gekko instruction that moves two
32-bit floats at once through a graphics quantisation register) where our build emits `stfs`/`stfd`. It reads as
a codegen lever and eats flag and shape sweeps. The long form is `notes/paired-single.md`.

**How it looks.** The instruction stream is equal except the store (or load) mnemonics:

```
target:  psq_st f0,0(r3),0,qr0          ours:  stfs f1,0(r3)
                                               stfs f1,4(r3)
```

**Why it does not work.** Measured 2026-09-25 over the whole game: **176 of 19,916** functions contain a
paired-single instruction, and **0** of ~2,450 matched functions do - so the frontend cannot emit the body-store
form, and a function holding one was written by hand (the Dolphin SDK's vector library, `PSVEC*`). The demo shows
the two natural spellings of a fill (a loop, four straight stores) come out as scalar `stfs`, with no `psq_st`/`psq_l`.

**What would need to change to be worth re-trying.** A compiler that emits it: none of the 33 under `build/compilers`
does with `-fp spfp/dpfp/efpu`, `-vector on`, any `-O` level, `-func_align`, `-fp_contract off` or peephole on/off
(measured at the time; the demo checks only the default flags). Or evidence that the same function *matches* at
100 % somewhere else in the tree with a paired-single op (a count of matched functions containing `psq_*` above
zero) - re-run the census in `notes/paired-single.md` ("How to spot it early") before deciding.

**Result.** Ruled out as a lever: record the residual as this class ("paired-single, hand-written") in the unit's
header and move on; the unit stays `NonMatching` and its other functions still count. `fn_8007270C` in
`g3d_calcvtx.cpp` carries 28 psq ops, so part of its residual is unreachable rather than a source shape.

**When NOT to apply.** A `psq_*` in the target does not mean *give up on the whole unit*: only the functions that hold
one are unreachable. And an `stfd` vs two `stfs` difference (no `psq`) is an ordinary source-shape/flag question
(ideas 3, 13), not this idea.

**Evidence.** `g3d_cpu`'s `fn_8009A910`: all 90 instructions equal, only the 36 store mnemonics differ. Related: 3
(read the target's disassembly), 13 (stop when the residual is no longer flag-shaped), 21 (fingerprint by what the
compiler emits, not by disassembler names).
