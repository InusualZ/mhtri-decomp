---
id: 37
title: A switch's `default` arm goes first in the source
status: works
problem: A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around 94 %, even though every case body is right. It reads as a missing case or a wrong table.
tags: [source-shape]
applies: [Wii/1.3]
demo: 037-switch-default-first.cpp
reviewed: 2026-09-29
related: [34, 53, 71]
---

# 37. A switch's `default` arm goes first in the source

**Problem.** A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the
target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around
94 %, even though every case body is right. It reads as a missing case or a wrong table.

**How it looks.** Ours is one word (4 bytes) larger than the target's. The compare chain (`cmpwi`/`beq` per case)
ends in an unconditional `b default` in ours, while the target's last test falls straight into a body; the
default's helper call sits in the middle of the arms instead of where retail has it.

**Why it happens.** (Read off the disassembly, not off the compiler's IR.) The case bodies are laid out in an order derived from the *source* order of the arms, and the
last compare of the chain falls through into whichever body is emitted right after it. With `default:` written
last the chain needs an extra `b <default>` at its end; written **first** the layout puts the default body where
retail has it and the extra branch disappears.

**How to work it.** Move `default:` to the top of the `switch` (before the first `case`). Nothing else changes -
control flow is identical - so verify the *size* first (4 bytes smaller), then the diff.

```c
switch (entry->kind) {
default:                          /* first in the source; MWCC still emits it after the chain */
    fn_802D3398(self, entry);
    break;
case 1:
    ...
}
```

**When NOT to apply.** When the target's switch uses a jump table (a dense `switch` compiles to an indirect
`bctr` through a `.data` table, idea 53) the layout question is different, and when the default arm returns a
constant that also ends the function idea 34's tail-merging rules decide the shape, not this one. It costs
nothing to try, but it only helps when the target size is *smaller* by one branch.

**Result.** `auto/802D0DCC_fn_802D0DCC`'s only function went 0 -> **100.00 %** (360 B both sides, 90/90 rows,
`.text` byte-identical, all 42 `.rela.text` records matching) under the stock flags - no flag, no pragma (measured
at the time; the unit has since been renamed). `default` written last gave 364 B / 93.73 %. The sibling shape in
the same function: the callee is called as `(self, entry)` so `r3` stays `self` across the call.

**Demonstration.** `037-switch-default-first.cpp` (`ideas.py demo-check 37`) reproduces the effect: the same
five-arm `switch` is 0x48 B with `default:` first and 0x4C B with it last (4 bytes, as in the 360/364 above). In the
`default_last` object the chain ends `cmpwi r3,4 ; beq ; b <default>`; in `default_first` it ends `cmpwi r3,4 ;
bne <default>` with the last case's body falling straight through.
