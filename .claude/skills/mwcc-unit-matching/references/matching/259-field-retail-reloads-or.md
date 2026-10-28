---
id: 259
title: A field retail reloads (or an address it recomputes per iteration) is read inside an inline helper: the inline is a CSE boundary
status: works
problem: retail reloads a field after a test (or recomputes `addi rX,self,OFF` per iteration) where ours reuses the register, or a loop's counter and induction pointer swap saved registers
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
demo: 
---

# 259. A field retail reloads (or an address it recomputes per iteration) is read inside an inline helper: the inline is a CSE boundary

**Problem.** Retail loads the same field twice (once for a compare into `r0`, again into an argument register) or
recomputes a member address in every loop pass, while ours keeps the first value or hoists the address into a saved
register; a neighbouring symptom is a loop whose counter and induction pointer take each other's saved registers.

**Why it happens.** MWCC (GC/3.0a5.2, -O4,s, -inline auto off) does not common an expression across an inlined
call's boundary: the helper's body is its own expression tree, so the caller's later read is a fresh load, and a loop
moved into a helper has its counter and pointer numbered in the helper's scope.

**How to work it.** Move exactly the reading expression (the test, the append, the loop) into a `static inline`
helper that takes the object (or the embedded member's address) as its parameter; leave the other reads in place.
Measure with `mt.py tryvar` - it is a per-site lever, and the same move elsewhere can measure worse.

**Result.** homebutton/hbm_kb_widget: hbmKbWidgetScrollPrevPage 93.82 -> 100 % (`first > 0` through an inline
predicate), hbmKbWidgetClearCandidateTexts 93.14 -> 100 % (the 20-pass loop through an inline over the candidate
list), hbmKbWidgetCreateParts 95.36 -> 96.33 % (List_Append through an inline: the list address is recomputed per
key and the freed saved register takes retail's constant 1). An inline visibility test in
hbmKbWidgetUpdateScrollButtons measured +0.

**Example.**

```
static inline bool hbmKbCandHasPrev(const HbmKbCandidateList* list) { return list->first > 0; }

void scrollPrevPage(HbmKbWidget* self) {
    if (hbmKbCandHasPrev(&self->candidates)) {          /* lwz r0 + cmpwi */
        hbmKbCandScrollTo(&self->candidates, self->candidates.first);   /* lwz r4 reloaded */
    }
}
```
