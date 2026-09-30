---
id: 92
title: Level-4 pragma + window source forms
status: superseded
problem: The window left by the level-4 pragma is a dozen rows of register choice plus where `CAMELLIA_RL1` is computed; a source form that makes level 4 emit the target order there would be a 100 % match - the same search idea 90 records.
tags: [pragma, allocator, source-shape]
applies: [Camellia]
demo:
superseded_by: 90
reviewed: 2026-09-29
related: [16, 35, 89, 90, 91]
---

# 92. Level-4 pragma + window source forms

**Problem.** With `#pragma optimization_level 4` on `camellia_setup256` the frame is right and the residual is a window
of register choices plus the placement of `CAMELLIA_RL1` (the vendor's rotate-left-by-one macro) in the absorb chain. A
source form that makes level 4 emit the target's order there would give 100 %. This entry asked for "more structural
rewrites of the whole kw4 chain" after five forms.

**How it looks.** A short window of differing rows in an otherwise identical function; every rewrite lands at 99.7-99.8 %.

**What was tried.** This is the same search as idea 90 ("level-4 pragma, source forms of the absorb chain") recorded a second
time from the hand-kept table, with "5 forms tried, more structural rewrites remain" as its status. The remainder was then
done: about 120 operand-order/statement/temp/anchor forms over the fifth group bottom out at 6 differing rows
(the `src/Camellia/camellia.c` header records it), and the five forms of the original entry reproduce as
no change on today's source (2026-09-29: 99.81 / 99.81 / 99.80 / 99.79 / 99.81 % against 99.81 %). Details, and why the
forms only move the mismatch between groups, are in idea 90.

**Why it does not work.** See idea 90: the reassociation is the level-4 pass's, so an anchor in one group leaves it active in
another.

**What would need to change to be worth re-trying.** A view of the compiler's IR (`tools/mwcc-debugger`) that names the
pass and its condition, or a structural rewrite of the whole function (not of the group) that changes the value
numbering - neither was tried; both belong to idea 90's follow-ups, not to a separate entry.

**Result.** Superseded by idea 90 (the same search, with the results). Status changed from `todo` on 2026-09-29 because
the entry duplicated 90 and its stated remainder has been explored; a genuinely new attempt should be recorded under
90 (or a new idea, if it is a different lever).

**Evidence.** Related: 16 (per-function pragmas), 35 (dead copy chain steers web priority), 89 (perturbation probe), 91
(pragma combinations, measured 2026-09-29: none helps).
