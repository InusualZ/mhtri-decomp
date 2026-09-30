---
id: 86
title: Disassembler aliases as fingerprints
status: ruled-out
problem: `extrwi`, `clrlslwi` and friends look like a compiler fingerprint that can be counted in the target's disassembly, but GNU objdump prints the underlying `rlwinm` instead, so the count is always zero.
tags: [measurement]
applies: [Wii/1.3]
demo: 086-disassembler-alias-fingerprints.cpp
reviewed: 2026-09-29
related: [21, 39, 77]
---

# 86. Disassembler aliases as fingerprints

**Problem.** `extrwi` (extract and right-justify), `clrlslwi` (clear left, shift left) and their kin look like a
compiler fingerprint that can be counted in the target's disassembly - "the DOL contains no `extrwi`, so this
unit's compiler never emits it".

**How it looks.** A census script or `grep` over an `objdump -d` listing reports zero for an instruction name that the
compiler certainly emits, and a flag decision (peephole on/off, an older compiler) is built on the zero.

**Why it does not work.** `extrwi`, `clrlslwi` and friends are *assembler aliases* of `rlwinm rA,rS,SH,MB,ME`; GNU
objdump decides per encoding which name to print, and for these it prints the underlying `rlwinm`, so counting the
alias proves nothing on either side (a raw-word scan finds thousands of encodings that are `extrwi`-shaped). The demo
compiles `(x >> 8) & 0xFF` (which a reader calls `extrwi r3,r3,8,16`) and `(x & 0xFFFF) << 2` (`clrlslwi
r3,r3,16,2`): objdump prints `rlwinm` for both. **Some aliases are printed** - `clrlwi`, `srwi`, `slwi`, `clrrwi`
appear by name (the demo's `x & 0xFF` is `clrlwi r3,r3,24`) - so which aliases can be counted is a property of the
disassembler version, not of the compiler; do not assume either way, test one known encoding first.

**How to work it.** Fingerprint what the *compiler* emits, decoded from the raw word or from stable mnemonics:
record forms (`add.`, `clrlwi.`), save-helper calls (`_savegpr_`), `lis` sharing, the mask operands of `rlwinm` itself
(idea 77 reads `MB`/`ME`). Idea 21 is the worked version: it counts record forms (`srwi.`, `add.`, `extsb.`, `clrrwi.`) that objdump does print, per unit.

**When NOT to apply.** The aliases are fine for *reading* a listing (`srwi` is clearer than `rlwinm ...,31`); the
mistake is counting them across two toolchains or two disassembler versions.

**Result.** Ruled out as a fingerprint. No status change was needed; the demo (2026-09-29) is a regression test that
the printed names are what this entry says on the pinned binutils.

**Evidence.** Idea 21 records the original incident: "0 `extrwi` in 1,357,339 instructions" said nothing at all, while a
raw-word scan found 5,386 words of that form. Related: 21, 39 (peephole folds), 77 (reading `rlwinm` operands).
