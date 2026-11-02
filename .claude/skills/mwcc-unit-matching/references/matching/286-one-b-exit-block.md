---
id: 286
title: One b-exit block per return statement: an early if-return is a direct branch, a trailing return after the guarded break keeps its block
status: works
problem: Retail has one or two dead `b exit` after a switch case's `b join` that ours lacks (the row is 4 or 8 bytes short per case), while every conditional branch of the case already targets the epilogue directly in both.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [280, 255]
---

# 286. One b-exit block per return statement: an early if-return is a direct branch, a trailing return after the guarded break keeps its block

**Problem.** A switch case reads `bl copyVec3 ; b join ; b exit ; b exit` in retail and `bl copyVec3 ; b join ;
b exit` in ours; a case with a single early return reads `... ; b join ; b exit` in retail and `... ; b join` in
ours. The `b exit` blocks have no predecessor left in the final code, so no branch-shape change inside the
conditions reaches them.

**Why it happens.** The frontend emits a lone `if (c) return;` as a direct conditional branch to the epilogue with
no block, but every other `return;` statement gets its own `b exit` block, which the backend never deletes even
after the branch-to-branch folding has redirected its predecessors straight to the epilogue. Retail therefore
holds one block per `return;` *statement*: a case with two blocks had two return statements, the early one
spelled as a trailing return of a nested `if`.

**How to work it.** Count retail's `b exit` blocks after the case's `b join`, and spell that many `return;`
statements, each as the trailing return after a guarded `break`: `if (x == 0xFF) return; rec = ...; if (ok) {
copy; break; } return;` (one block) becomes `if (x != 0xFF) { rec = ...; if (ok) { copy; break; } return; }
return;` (two blocks); a case with one early return and no trailing one becomes `if (p != NULL) { use; break; }
return;`. `else { return; }`, `if/else` + `break` and extra unreachable returns are all folded to the same
object; only this nesting adds a block.

**Result.** `enemy/em_common` `em_target_pos_set`: 99.78 -> 100 % (1856 -> 1860 B, eight blocks over six cases).

**Example.**

```
case 8:
    em_special_part_apply(self, self->state_0x381);
    if (self->recs_0x9A4 != NULL) {          /* was: if (self->recs_0x9A4 == NULL) return; */
        em_data_point_get(self, ..., &self->vec_0x36C);
        break;
    }
    return;                                  /* retail's `b join ; b exit` */
```
