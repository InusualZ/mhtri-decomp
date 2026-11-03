---
id: 294
title: A halved count folded forward takes the pipe base's register: cast the halving into the initialiser and or the word in one nested expression
status: works
problem: Every instruction of a register-write function is retail's but two volatile registers are swapped (retail `lis r6` for the pipe base and `r7` for a halved count, ours r7/r6) and the last `or` writes the word's web where retail writes a fresh register.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 294-halved-count-folded-forward.cpp
---

# 294. A halved count folded forward takes the pipe base's register: cast the halving into the initialiser and or the word in one nested expression

**Problem.** A function computes two counts as `33 - __cntlzw(field)`, halves them, and ors them with a third into a
word it writes to a hardware pipe (`GXWGFifo`). Every instruction is retail's, in retail's order, but retail colours
the pipe base `lis` r6 and the halved count chain r7 where ours gives them r7 and r6, and retail's last `or` writes
r4 (the register of the `nNrm << 2` term) where ours writes the word's own register.

**Why it happens.** Two numbering effects, visible in the mwcc-debugger's first PCode dump and replayed exactly by
`regsim`. (1) A separate `nCols /= 2;` statement is folded forward by the frontend to the count's first use, so the
`srwi` temp is numbered at the word's expression - after the `lis` of the pipe write that precedes it in the source.
The colouring runs from the highest number down, so the `lis` takes the lower free register (r6) only when it is
numbered below the shift. (2) `reg = a | b; reg |= c;` writes the `reg` web twice; the web is live across the
`nNrm << 2` definition, so it cannot take that term's register, and the second `or` reads `rD = rA`. Retail's
second `or` is a fresh temp (`or r4,r5,r4`, rD = rB), which only a single expression produces - but a flat
three-term chain `(a | b) | c`, and a single-use `reg` folded into the store, are reassociated by the frontend
with the `nTex` term last (`or r5,r4,r8` then `| nTex << 4`).

**How to work it.** Put the halving inside the initialiser with a `u32` cast ahead of it - `nCols = (u32)(33 -
__cntlzw(x)) / 2;` - so the `srwi` is the web's own definition and nothing is folded forward (without the cast the
`int` operand makes it a signed halving: `srawi` plus the sign fix, which the as-is `u32` local avoided). Then build
the word as one nested expression that the frontend keeps as written: `reg = (nTex << 4) | (nCols | (nNrm << 2));`.
Keep the pipe's command byte write between the counts and the word.

**Result.** `RVLGX/GXAttr` `__GXSetVCD` (176 B, the inlined `__GXXfVtxSpecs`): 98.30 -> 100 %; the unit 13/13 rows,
flipcheck READY, trial flip `main.dol: OK`. Measured on the way (scores of the row): the operand order alone
`(nTex << 4) | nCols` 92.39, halvings as `(33 - ..) / 2` without the cast 73.30 (`srawi`), the final or in the store
expression or a second local 34/44 instructions (reassociated), a flat three-term chain 32/44.

**Example.**

```c
nCols = 33 - __cntlzw((d->lo >> 13) & 0xF);  nCols /= 2;          /* folded to the word: srwi numbered after lis */
reg = nCols | (nTex << 4);  reg |= nNrm << 2;                        /* web written twice: or r5,r5,r4 */

nCols = (u32)(33 - __cntlzw((d->lo >> 13) & 0xF)) / 2;              /* srwi is the web's definition */
reg = (nTex << 4) | (nCols | (nNrm << 2));                           /* or r5,r5,r7; or r4,r5,r4 */
```
