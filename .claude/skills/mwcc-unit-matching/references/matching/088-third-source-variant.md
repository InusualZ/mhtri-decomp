---
id: 88
title: Third historical source variant
status: ruled-out
problem: `camellia_setup256` matches NSS and NetBSD textually, but the retail file might be a third variant of the vendor source; diffing another copy's key-schedule region is the cheapest way to find the source shape the allocator liked.
tags: [source-shape, process]
applies: [Camellia]
demo:
reviewed: 2026-09-29
related: [18, 22, 89, 90]
---

# 88. Third historical source variant

**Problem.** Our `camellia_setup256` (the 256-bit key schedule of NTT's Camellia reference code) matches Mozilla NSS
and NetBSD textually, but the retail file may be a *third* variant of the vendor source (the original NTT 1.2.0 or a
copy the SDK vendor edited). When the last residual is a register/stack-slot difference no shape of *our* source
moves, a different starting text is the cheapest thing to try: it changes statement order and temporaries in ways
nobody would invent.

**How it looks.** Every function of a vendor unit is byte-identical except one whose difference is pure allocation
(here one 4-byte stack slot, all instructions equal except `r1` offsets); dozens of local rewrites (ideas 18, 89, 90)
change nothing.

**What was tried.** Compare the source of the function against every historical copy that could be fetched: NSS
3.19.1, 3.24 and 3.28.4, the older NSS fetch and NetBSD's NTT-derived copy. All are statement-identical to ours (only
`PRUint32` vs `u32`, the `SUBL` macro naming and whitespace differ); no third variant exists in that lineage.
Recorded before 2026-09-29 (the hand-kept table did not date the row); not re-run - it needs network access to the
upstream trees and the result is a property of those trees, not of our compiler.

**Why it does not work.** All reachable copies descend from one text, so they give the compiler the same input and the
same allocation. The residual is then a compiler-side tie-break, not a source difference between copies.

**What would need to change to be worth re-trying.** A copy from a *different* lineage: another vendor's embedding of
the NTT code (an SDK vendor's own copy, which nobody has fetched), or the original NTT
release archive if it differs in `camellia_setup256`; an independent reimplementation would not count, since the
retail bytes follow this code's structure. Also worth a look if the residual
shrinks to a single expression whose two spellings differ between copies.

**Result.** Ruled out within the NSS/NetBSD/NTT lineage. The productive levers turned out to be per-function pragmas
(idea 16) and the window analysis in ideas 90-92.

**Evidence.** Related: 18 (temporaries and operand order steer the allocator), 22 (when retail's colouring is your
mirror, stop), 89/90 (the perturbation and window searches on the same unit).
