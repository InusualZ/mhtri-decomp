---
id: 284
title: An early-return || chain whose last term branches around a b-exit block is the inverse guard of the rest of the function
status: works
problem: Every term of an early-return `||` chain branches straight to the epilogue except the last, which ours emits as `bne next; b exit` where retail has one `beq exit`, so the row is 4 bytes long and every later branch is shifted.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [255, 248, 282]
---

# 284. An early-return || chain whose last term branches around a b-exit block is the inverse guard of the rest of the function

**Problem.** `if (a == 0 || b == 1 || ... || f(x) == 12) return;` followed by the rest of the function: the first
terms branch to the epilogue directly in both objects, but the last term reads `bne next ; b exit ; next:` in ours
and `beq exit` in retail. Splitting the last term into its own `if` only moves the pair to the new last term.

**Why it happens.** The mwcc-debugger's first PCode shows the chain's `return;` body as its own block (`B34: b B102`
with one predecessor per term) laid out after the last term. The backend folds the earlier terms' `bt B34` into
`bt B102`, but the last term falls through into B34 and keeps `bf next` + the block. A lone `if (c) return;` is
emitted as a direct branch with no block (idea 280), and so is a chain whose terms target the function's own end
label: when the chain is the *inverse* condition guarding the rest of the body, every term branches to the closing
brace, which is the epilogue, and no `b exit` block exists.

**How to work it.** Rewrite `if (A || B || C) return; rest;` as `if (!A && !B && !C) { rest }` (De Morgan, each
term negated: `== 0` becomes `!= 0`, `<= 5` becomes `> 5`). The per-term branch polarity is unchanged, only the
last term's target moves. Other `return`s inside the rest keep their own blocks.

**Result.** `enemy/em_common` `em_act_pick`: 99.60 -> 100 % (1100 -> 1096 B), unit 465 -> 466 rows at 100 %.

**Example.**

```
if (em_busy_ck(self) != 0 && self->field_0x784 != 1 && em_status_ck(self, 0xFD) != 1 &&
    (u8)(self->action + 248) > 5 && em_sleep_ck(self, 0) != 1 && self->field_0x1F8 == 0 &&
    em_userdata_state_kind_get(self) != 12) {
    /* the rest of the body; was: if (... == 0 || ... == 1 || ... == 12) return; */
}
```
