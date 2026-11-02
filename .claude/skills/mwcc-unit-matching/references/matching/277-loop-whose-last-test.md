---
id: 277
title: A loop whose last test leaves: if (c) break; else continue; keeps retail's bne exit; b head
status: works
problem: A for (;;) that ends with a conditional exit compiles to `beq head; b exit` where retail has `bne exit; b head` - one opcode and the back edge differ, and every row after shifts by nothing but still mismatches the branch.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [252, 274, 276]
---

# 277. A loop whose last test leaves: if (c) break; else continue; keeps retail's bne exit; b head

**Problem.** The last statement of a `for (;;)` body is a test that leaves the loop (`if (sub_error != 0) break;`),
and the loop's natural end is the back edge. Retail reads `cmpwi r0,0; bne exit; b head`; ours reads `cmpwi r0,0;
beq head; b exit`. `if (c) continue; break;` and `if (a && b) continue; break;` give the same inverted pair.

**Why it happens.** MWCC lowers `if (c) break;` (and `if (c) continue;`) to one direct conditional branch, so the
block order is `bne L_break; b head; L_break: b exit`. A later pass inverts a conditional branch over a single
unconditional jump into `beq head` and drops the `b` it jumped over, leaving `b exit` behind. An `if (c) S1 else
S2` is lowered as `b!c L_else; S1; b join; L_else: S2`, so with S1 = `break` and S2 = `continue` the order is `beq
L_else; b exit; L_else: b head`, and the same inversion now produces `bne exit; b head` - retail's pair. The
`#pragma peephole off` in force does not prevent the inversion, so it is not the peephole.

**How to work it.** Spell the loop's final exit test as `if (c) { break; } else { continue; }` (the `else` is the
whole point). Where retail's pair is `beq head; b exit`, the plain `if (c) break;` is the spelling.

**Result.** `Network/network_pat_control` `updateMessagePool` (Wii/1.3, -O4 by pragma, nopeephole): the step 6
re-entry's two rows `bne 0x2e6c; b 0x253c` match (88.09 -> 88.10 %, the rest of the function unchanged).

**Example.**

```
for (;;) {
    ...
    if (entry->state != 6) {
        break;
    }
    if (work->sub_error != 0) {      /* retail: cmpwi; bne exit; b head */
        break;
    } else {
        continue;
    }
}
```
