---
id: 86
title: Disassembler aliases as fingerprints
status: ruled-out
problem: `extrwi`, `clrlslwi` and friends look like a compiler fingerprint that can be counted in the target's disassembly.
tags: [measurement]
applies: []
demo:
---

# 86. Disassembler aliases as fingerprints

**Problem.** `extrwi`, `clrlslwi` and friends look like a compiler fingerprint that can be counted in the target's disassembly.

**Why it does not work.** `extrwi`, `clrlslwi` and friends are *aliases* GNU objdump never prints (it prints the underlying `rlwinm`), so counting them on either side proves nothing - see idea 21. Fingerprint what the compiler emits (record forms, save-helper calls, `lis` sharing), not what the disassembler happens to name.

**Result.** Ruled out (bullet of the old `ruled-out.md`).
