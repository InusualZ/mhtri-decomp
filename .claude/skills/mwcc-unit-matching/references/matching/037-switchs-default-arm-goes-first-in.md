---
id: 37
title: A switch's `default` arm goes first in the source
status: works
problem: A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around 94 %, even though every case body is right. It reads as a missing case or a wrong table.
tags: [source-shape]
applies: []
demo:
---

# 37. A switch's `default` arm goes first in the source

**Problem.** A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the
target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around 94 %,
even though every case body is right. It reads as a missing case or a wrong table.

**Why try it.** MWCC emits the default body *after* the compare chain wherever it is written, so writing `default:`
**first** in the source lands it where retail has it. Written last, the compiler orders the chain the other way and
the function grows by a word.

**Result.** `auto/802D0DCC_fn_802D0DCC`'s only function went 0 -> **100.00 %** (360 B both sides, 90/90 rows, `.text`
byte-identical, all 42 `.rela.text` records matching) under the stock flags - no flag, no pragma. `default` written
last gave 364 B / 93.73 %. The sibling shape in the same function: the callee is called as `(self, entry)` so `r3`
stays `self` across the call.

**Example.**

```c
switch (entry->kind) {
default:                          /* first in the source; MWCC still emits it after the chain */
    fn_802D3398(self, entry);
    break;
case 1:
    ...
}
```
