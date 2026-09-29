# A paired-single instruction names a function that was not built from C

**Problem.** A function's residual is a handful of neighbouring opcodes: the target fills a float array with
broadcast paired-single stores (`psq_st f0,off,0,qr0`) where every compiler we have emits `stfd` or two
`stfs`. Nothing in the source moves it, and hours go into flag and shape sweeps that cannot pay.

**Why try it.** It looks like a codegen lever - `-fp spfp` exists, `-vector on` exists, and the rest of the
function matches instruction for instruction.

**Result: stop.** A paired-single op names a function that did not come from this C frontend. Measured
2026-09-25 over the whole game: `.text` holds **785** paired-single instructions (`psq_l` 759, `psq_st` 26) in
**176 of 19,916 functions (0.88 %)** - and **not one of them matches**, out of roughly 2,450 matched
functions. If our frontend could emit the form, some of the 176 would have matched by now. The population
names itself: the first candidates are `PSVECSubtract` and its neighbours, the Dolphin SDK's hand-written
vector library, and the rest of the list is the same kind of hand-optimised routine. It also re-reads a unit
already on the books: `g3d_calcvtx.cpp`'s `fn_8007270C` carries 28 psq ops, so part of its 66 % residual is
unreachable rather than a source shape.

**Example.** `g3d_cpu`'s `fn_8009A910`: all 90 instructions equal, only the 36 store mnemonics differ. All 33
compilers under `build/compilers` were scanned with both fill shapes and `-O1..-O4,p`, `-func_align 4/8`,
`-opt full/speed`, `-fp spfp/dpfp/efpu`, `-fp_contract off`, `-vector on` and peephole on/off - none emits
`psq_st` for a body store. Record the residual as this class and move on: the unit stays `NonMatching` and its
other functions still count.

**How to spot it early.** Scan the target for opcodes 56/61 (`psq_l`/`psq_st`) before spending a session:
per-function attribution only needs `main.elf`'s `.text` (it holds the original bytes for every `NonMatching`
region) plus the `.text` ranges in `symbols.txt`.
