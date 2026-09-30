---
id: 15
title: Pin a metric that does not drift
status: works
problem: Per-symbol fuzzy percentages change between tool versions: the same two objects can score 82.72 % with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is meaningless against the other.
tags: [measurement, tooling]
applies: []
demo:
reviewed: 2026-09-29
related: [1, 2, 10, 75]
---

# 15. Pin a metric that does not drift

**Problem.** Per-symbol fuzzy percentages change between tool versions: the same two objects can score 82.72 %
with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is meaningless against
the other.

**How it looks.** A function "improves" or "regresses" after a tool update with no byte of code changed, or two
lanes quote different percentages for the same unit.

**Why it happens.** The fuzzy score is an implementation detail of objdiff's matcher, revised between releases;
any progress metric that depends on the tool version will eventually move by itself.

**How to work it.** Decide on version-independent facts: **`matched_functions X/Y`** from the project report, exact
function sizes, and "first divergence @N" (idea 2). Record the fuzzy percentage as context only, with the tool
version next to it. The pinned version lives in `configure.py` (`objdiff-cli` 3.6.1 when this was reviewed, the
"v3.6.1" in idea 10's example).

**When NOT to apply.** `matched_functions` and `complete_code_percent` have their own trap: for an
`Object(Matching, ...)` unit the report pins completion at 100 whatever the bytes are (idea 75), so the count
is only evidence for `NonMatching` units; the byte comparison (`verifyunit`) is the backstop.

**Result.** A metric that means the same thing next month.

**Example**

```sh
ninja build/RMHE08/report.json
# main/Camellia/camellia: matched_functions 9/10, matched_code 80.10%, fuzzy 99.9656%
```

**Evidence.** The 82.72 % / 99.83 % pair is the Camellia unit's `camellia_setup256` under two objdiff builds, and
the report line above was measured then; today's number for that symbol is 99.80576 % (`symdiff.py -u
Camellia/camellia`, 2026-09-29), a further small drift with the pinned tool.
