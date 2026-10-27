---
id: 240
title: A subfc/subfe compare feeding an addi is a ternary between consecutive constants
status: works
problem: Retail materialises a signed compare as `subfc ; srwi ; srwi ; subfe` and adds a constant, where ours (`K + (a >= b)`) uses `srawi ; srwi ; subfc ; adde` - and the two constants differ by one.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [119, 228]
---

# 240. A subfc/subfe compare feeding an addi is a ternary between consecutive constants

**Problem.** Retail computes a value from a signed compare of two `s16` fields as `subfc r0,b,a ; srwi rB,b,31 ;
srwi rA,a,31 ; subfe r3,rA,rB ; addi r,r3,K`, while ours, written `K + (a >= b)`, emits `srawi ; srwi ; subfc ; adde ;
addi`. It reads like an allocator detail, but the two forms compute different values.

**Why it happens.** `subfe` leaves `-(a < b)` (0 or -1), so retail's result is `K` or `K - 1`; MWCC lowers
`a < b ? K - 1 : K` to that sequence. `K + (a >= b)` is `K` or `K + 1` - an off-by-one in the source, not a codegen
difference.

**How to work it.** Decode the sequence by hand (`subfe rD,rA,rB` = `rB - rA - 1 + CA`, CA from the `subfc`), take the
two values it produces, and write the ternary with those constants in retail's compare direction (`a < b ? lo : hi`).

**Result.** `lobby/fn_8030121C` `lb_barter_list_draw` 95.29 -> 97.38 % (+19 B) with `type = page < pages_a ? 2 : 3`
in place of `3 + (page >= pages_a)`. That also fixed a real bug: the row drawer branches on `type == 2`.

**Example.**

```
type = 3 + (w->page >= w->pages_a);      /* srawi ; srwi ; subfc ; adde ; addi 3 - values 3/4 */
type = w->page < w->pages_a ? 2 : 3;     /* subfc ; srwi ; srwi ; subfe ; addi 3 - values 2/3 (retail) */
```
