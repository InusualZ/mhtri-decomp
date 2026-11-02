---
id: 276
title: Crossing backward branches: head the later target's loop, duplicate the earlier one once, and let a nested re-entry loop's exits fall to an inert test
status: works
problem: Retail has two backward branches into two different blocks of one case (steps 3/4 back into step 1's check, step 6 back into step 2's success block) whose loop intervals cross; the goto-free source repeats both blocks and the function is their size longer.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [213, 241, 252, 274, 275, 277]
---

# 276. Crossing backward branches: head the later target's loop, duplicate the earlier one once, and let a nested re-entry loop's exits fall to an inert test

**Problem.** A step machine's case holds two shared blocks that later steps branch back into: `HEAD` (step 1's
`checkLayerEntry` dispatch, re-entered by steps 3 and 4) and `SUCCESS` (step 2's success block, re-entered by
step 6). The loop `HEAD..step 4` and the loop `SUCCESS..step 6` cross (each contains the other's head but not its
back edge), so no nesting of `for (;;)` expresses both; the goto-free source keeps both as inline helpers, emitted
three times and twice, and the function is 0x3FC B long (`updateMessagePool`, 76.10 %).

**Why it happens.** Idea 275 nests the loops of two heads that lie in layout order inside one another; here they cross. Idea 252/274's loop needs every entry into the loop's middle to be a `case`/`default` label
of the *innermost* switch around the loop. The `HEAD` loop would need two mid-entries that test different values
(`cmd != 22` and `state != 1`), so no switch can host both; the `SUCCESS` loop needs one (`state != 2`) plus the
shared free that step 2's failure paths join. And a nested loop's exits cannot reach the enclosing switch's exit
through anything that follows the loop - but when what follows is a test that those paths provably fail, landing
on it is the same code path with one more compare that retail also executes.

**How to work it.**
1. Pick the loop whose mid-entries can all be labels (here `SUCCESS`: `switch ((u32)state) { case 2: ... for (;;) {
   SUCCESS; default: <step 3..6 chain> } }`); the other target stays a duplicated inline helper. Write the switch
   operand unsigned when retail compares with `cmplwi` (a `u8` operand gives `cmpwi`).
2. A path that joins a shared block *before* the loop head (step 2's failures -> the free after `SUCCESS`) does its
   own copy of that block and exits (+2 instructions each here, against a 169-instruction duplicate).
3. Inside that loop, the earlier target can still be shared by its own late re-entries: step 3's check heads a
   second `for (;;)` with step 4 as the `default:` of a one-case `switch ((u32)state)`; the inner loop's exits
   `break` to the step 5 test that follows, which is inert for them (`state` is 3 or 4 there and the record is then
   at step 2 or freed, so the step 6 test fails too), and step 4 falls to the back edge with its own `target`.
4. End the outer loop with idea 277's `if (c) break; else continue;` so the back edge stays an unconditional `b`.

**Result.** `Network/network_pat_control` (Wii/1.3, -O4 by pragma, nopeephole): `updateMessagePool` 76.10 ->
88.05 (step 1 and 2 above: 6528 -> 5876 B of 5504) -> 91.35 (step 3: 5716 B); with the chat scan as an inline
helper whose `return` is the branch past the send loop and the row copy counting with the pool's `i`, 94.33 %
(5688 B). What remains is one 42-instruction copy of the check, the two 2-instruction frees, and colouring.

**Example.**

```
switch ((u32)state) {
case 2:
    if (results[cmd] == 0) break;
    if (results[cmd] < 0) { ...; freePoolEntry(entry); break; }   /* retail: b to the shared free */
    for (;;) {                       /* SUCCESS heads the loop step 6 re-enters */
        success_block();
        freePoolEntry(entry);
default:
        state = entry->state;
        switch ((u32)state) {
        case 3:
            if (pending()) break;
            target = block; profile = prof;
            for (;;) {               /* step 3's check heads the loop step 4 re-enters */
                check(target, profile);
                break;               /* lands on the step 5 test: inert for step 3/4 */
        default:
                if (state != 4) break;
                if (pending4()) break;
                target = record; profile = NULL;
            }
            if (state == 5) { ... }
            if (entry->state != 6) break;
            if (sub_error != 0) break; else continue;
        }
        break;
    }
    break;
}
```
