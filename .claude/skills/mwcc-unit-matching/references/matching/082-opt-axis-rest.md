---
id: 82
title: The rest of the `-opt` axis
status: ruled-out
problem: An unknown sub-option might be what controls fusion or stack allocation.
tags: [flags]
applies: []
demo:
---

# 82. The rest of the `-opt` axis

**Problem.** An unknown sub-option might be what controls fusion or stack allocation.

**Why it does not work.** Sweep the compiler's own keyword list once. Beyond the one or two keywords that matter, the rest are no-ops or make the code worse; the frame-size traps are in trick 10.

**Result.** Old table status: no. (Bullet text above: "The whole `-opt` axis.")
