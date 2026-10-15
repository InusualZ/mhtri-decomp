---
id: 150
title: Under -RTTI off a secondary base's vtable part has a zero header too, and its overrides append to the primary part
status: todo
problem: A multiply-inherited class's vtable does not pair with ours - an expected `-offset` word in the secondary part is 0 in retail, and some slots sit after the secondary part instead of inside the primary one.
tags: [vtable]
applies: [GC/3.0a5.2]
demo: 
related: [52, 68, 76, 98]
---

# 150. Under -RTTI off a secondary base's vtable part has a zero header too, and its overrides append to the primary part

**Problem.** Modelling a class with two bases, the vtable that comes out has the expected primary part
(`0, 0, slots...`) but the secondary part's header and the position of the overriding slots do not match retail.

**Why it happens.** Read off retail (homebutton band, `-RTTI off`): both header words of the *secondary* part are
zero too (there is no `-offset` word), and a virtual of the secondary base that the derived class overrides is
appended to the primary part *after* the secondary part's slots (with the secondary part pointing at an adjustor
thunk, idea 149).

**How to work it.** Read the table words from the target before modelling the hierarchy (idea 76): a zero header
in the middle of a table is the start of a secondary part, and the slots after it that point at the derived
class's own methods are its overrides of the secondary base.

**Result.** Observed in `homebutton/gui`, `hbm_kb_list` and `hbm_kb_cursor` while reconstructing their classes;
no demo or before/after measurement yet.

**Example.**

```
__vt__Derived: 0, 0, Derived::a, Derived::b,          primary part
               0, 0, thunk(Derived::c),               secondary part (zero header)
               Derived::c                             override appended to the primary part
```
