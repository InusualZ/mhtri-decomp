---
id: 11
title: Do not read a large size gap as "different source"
status: works
problem: The target object is often hundreds of bytes bigger than ours, and several functions sit at 0 %, which reads like the original source containing code ours does not have (different unrolling, extra rounds, extra statements).
tags: []
applies: []
demo:
---

# 11. Do not read a large size gap as "different source"

**Problem.** The target object is often hundreds of bytes bigger than ours, and several functions sit at
0 %, which reads like the original source containing code ours does not have (different unrolling, extra
rounds, extra statements).

**Why try it.** Size gaps are the most misleading signal in a decomp: aggressive optimizer flags *remove*
instructions, so "the target is bigger" can mean nothing more than "our flags are too aggressive".

**Result.** Check the flag hypothesis before rewriting code. In the worked example the entire 2820-byte gap
was flag-caused (peephole fusion, pooled table bases, `stmw`), and with the final flags every function size
matched exactly. A source rewrite at that point would have been wasted work on correct code.

**Example**

```
baseline  -O4,p -inline auto -use_lmw_stmw on    -> .text 21600 B, 6/10 functions at 0%
final     -O3 -inline noauto -opt nopeephole ... -> .text 24420 B, 9/10 byte-identical
```
