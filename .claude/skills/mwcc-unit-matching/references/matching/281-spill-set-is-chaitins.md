---
id: 281
title: The spill set is Chaitin's cost/degree pick: make retail's spilled webs cheap and the kept ones dear
status: works
problem: Every opcode matches but ours spills a different value than retail (a loop bound in a stack slot where retail keeps it in r14 and spills two short-lived counts instead), so the row carries a size gap and a register cascade.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [243, 244]
---

# 281. The spill set is Chaitin's cost/degree pick: make retail's spilled webs cheap and the kept ones dear

**Problem.** A long function with more callee-saved candidates than registers: ours spills `rows` (five `stw` at
its defs, two `lwz` in the loop, +16 B) while retail keeps `rows` in r14 and spills three other values (two running
counts and one `state`) to 0x68/0x6C/0x70. `mt.py regsim` reproduces the colouring but not the spill choice, and
declaration hill climbs never reach it because the answer is not a permutation.

**Why it happens.** The `regalloc-gpr-pass-N-assigned.txt` dump of `tools/mwcc-debugger` shows the rule. Every node
carries `cost` = sum over its references of the block weight (1 outside a loop, 8 inside a depth-1 loop), a def
counting once and a use twice: `rows` = 5 defs x 1 + 2 loop uses x 16 = 37. When simplification stalls (no node
below K = 29), MWCC pushes the remaining node with the **lowest `cost / current degree`** (`adjusted cost` in the
dump, the degree being the neighbours still in the graph), and keeps stalling in ascending adjusted cost until the
rest simplify; at select time a stalled node that finds no free colour is the one spilled (`-> none`, `fSpilled`).
Spill slots are then handed out in ascending vreg order. So which value is spilled is decided by the *costs and
degrees* the source gives each web, and by the web's vreg number for the slot: a frontend-renamed web (`@NNNN`, the
second disjoint range of a variable) sits below the hoisted loop invariants, a backend temp (a value that reached the
variable through `mr` from a call result or a masked load) sits above them in code order.

**How to work it.**
1. Dump the function and read the `-> none` rows of pass 1 with their `cost`/`adjusted cost`, then list retail's
   spills from the diff (`stw rX, slot` right after a def, `lwz` before the use) and the registers retail hands out.
2. Make retail's spilled values *cheaper* than the one we spill: a value retail stores as a word is `s32` (a `u8`
   local spills as `stb`), a sum kept across a second call is its own variable (`first = count; ... total = first +
   count` gives a def + one use web of cost 24, below `rows`'s 37/36), and a value tested once is not masked
   (`s32 total` drops the `clrlwi` before `cmpwi`, one use fewer).
3. Make the slot order right by numbering: a `& 0xFF` on a `u8` load turns the variable into a backend temp
   (numbered after the hoisted `@` temps), a `?:` becomes a parse-time temp (numbered above every `@` temp and
   coloured right after the hoisted constants), a pointer formed as `&tbl[0][row]` is a renaming web where retail's
   CSE of `tbl[row]`/`tbl[row + 8]` is a backend temp.
4. Re-dump and confirm the `-> none` set and the slot order before touching declarations.

**Result.** `menu/menu_infomation` `ammo_grid_draw` (Wii/1.3, -O3, peephole off): 95.58 % with 3188 vs 3172 B ->
100 % in seven measured steps (`s32 total` 95.84, `first` local 97.53, `s32 first` 97.84, `row` as `?:` 98.43,
`strs` local 99.19, `skilled` before `i` + `rows` as `?:` 99.82, no `ammo` pointer 99.99, `& 0xFF` 100).

**Example.**

```c
s32 total;
s32 first_count;                       /* retail's 0x68/0x6C spills: a word, one def and one use */
row = (mode & 4) ? rows + i : i;       /* a parse-time temp: coloured r25 right after grid8 */
first_count = count & 0xFF;            /* a backend temp: its spill slot comes after the hoisted mode & 4 */
total = first_count + count;           /* add r20, r0, r3; cmpwi r20, 0 with no clrlwi */
```
