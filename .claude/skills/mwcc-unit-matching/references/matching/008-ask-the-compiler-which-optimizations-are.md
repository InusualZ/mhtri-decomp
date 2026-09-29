---
id: 8
title: Ask the compiler which optimizations are actually on
status: works
problem: After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and `-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once.
tags: [flags, source-shape]
applies: []
demo:
---

# 8. Ask the compiler which optimizations are actually on

**Problem.** After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and
`-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once.

**Why try it.** The compiler can print its own effective configuration, which turns a guess about macro
expansion into a fact and prevents "is scheduling on?" debates.

**Result.** `-opt display` prints the resolved option set, which is the check to run before declaring a
flag set "the" answer.

**Example**

```sh
mwcceppc.exe <candidate flags> -opt display -c src/<Unit>/<file>.c -o build/tmp/probe
# - global optimizer level 3
# - peephole optimizations off
# - no instruction scheduling
```
