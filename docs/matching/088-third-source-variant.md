---
id: 88
title: Third historical source variant
status: ruled-out
problem: Our `camellia_setup256` matches NSS and NetBSD textually, but the retail file may be a third variant (original NTT 1.2.0 / SDK copy). Diffing another copy's absorb region is the cheapest way to find the source shape the allocator liked.
tags: [source-shape, process]
applies: [Camellia]
demo:
---

# 88. Third historical source variant

**Problem.** Our `camellia_setup256` matches NSS and NetBSD textually, but the retail file may be a third variant (original NTT 1.2.0 / SDK copy). Diffing another copy's absorb region is the cheapest way to find the source shape the allocator liked.

**Result.** no - NSS 3.19.1 / 3.24 / 3.28.4, the older NSS fetch and NetBSD NTT-derived are all statement-identical to ours (only `PRUint32`/`SUBL` naming and whitespace); no third variant exists in that lineage.
