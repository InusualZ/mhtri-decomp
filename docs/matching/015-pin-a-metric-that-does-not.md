---
id: 15
title: Pin a metric that does not drift
status: works
problem: Per-symbol fuzzy percentages change between tool versions: the same two objects can score 82.72 % with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is meaningless against the other.
tags: [measurement, symbols, tooling]
applies: []
demo:
---

# 15. Pin a metric that does not drift

**Problem.** Per-symbol fuzzy percentages change between tool versions: the same two objects can score
82.72 % with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is
meaningless against the other.

**Why try it.** Any progress metric that depends on the tool version will eventually "improve" or "regress"
without a single byte of code changing.

**Result.** Decide on version-independent facts: **`matched_functions X/Y`** from the project report, exact
function sizes, and "first divergence @N". Record the fuzzy percentage as context only, with the tool
version next to it.

**Example**

```sh
ninja build/RMHE08/report.json
# main/Camellia/camellia: matched_functions 9/10, matched_code 80.10%, fuzzy 99.9656%
```
