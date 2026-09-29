---
id: 10
title: Do not use frame size as a success signal
status: works
problem: When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant that produces the target's frame - and several do.
tags: [flags]
applies: []
demo:
---

# 10. Do not use frame size as a success signal

**Problem.** When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant
that produces the target's frame - and several do.

**Why try it.** A frame-size hit looks like a precise binary signal, which is exactly why it needs a second
check before it is believed.

**Result.** Frame size is an alignment artifact: locals are rounded up to the next 16 bytes, so a variant
can hit the target's frame while emitting completely wrong code. Always confirm with per-function match
percentages and function sizes; frame size alone is never the answer.

**Example**

```
-opt size   -> target frame -0x1d0 but 35% of the code matching   (not the answer)
baseline    -> frame -0x1e0, 82.72% (older metric) / 99.83% (v3.6.1)
```
