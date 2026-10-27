---
id: 193
title: A constructor emitted after __sinit is an inline member with __declspec(noinline) defined after the object
status: works
problem: retail's `__sinit` calls a class constructor that sits right after it in `.text` (`bl __ct__...` then the ctor body); ours inlines the constructor into `__sinit`, or emits it before `__sinit` when it is an ordinary definition
tags: [source-shape, sections]
applies: [Wii/1.3]
related: [163]
demo: 
---

# 193. A constructor emitted after __sinit is an inline member with __declspec(noinline) defined after the object

**Problem.** Retail's static initialiser calls `__ct__<Class>Fv` out of line, and the constructor's body is the
function right after `__sinit` in the object. In ours either the constructor is inlined into `__sinit` (an in-class or
`inline` body - `-inline noauto` still inlines a declared-inline function, and `__sinit` is generated at the end of
the TU where every body is known), or it is a plain out-of-class definition and lands *before* `__sinit`, which MWCC
always emits after the last user function.

**Why it happens.** MWCC emits an inline function it did not inline as a weak copy after its first user (idea 163);
`__sinit` is that first user, so the copy follows it. `__declspec(noinline)` is refused on an in-class constructor
(`(10176) illegal type qualifier(s)`) but accepted on the out-of-class `inline` definition. The other levers do not
work: `#pragma dont_inline` around the definition is ignored (the decision is taken when `__sinit` is built), and
left on to the end of the file it also stops the inlining of every other constructor `__sinit` calls;
`#pragma always_inline` and `#pragma inline_max_size(N)` move nothing.

**How to work it.** Declare the constructor in the class, define the object, then after it:
`__declspec(noinline) inline Class::Class() { ... }`. Check with `objdump -t` that `__sinit_...` is followed by a weak
`__ct__...`.

**Result.** `lobby/lb_menu_pos_tbl`: `__sinit_\lb_menu_pos_tbl_cpp` 92.50 -> 100 and `__ct__11LbLobbyWorkFv`
0 -> 100, in retail order, with the `LbMenuPos` constructors (`setVec3`) still inlined.

**Example.**

```
struct Work { /* ... */ Work(); };
Work work;                                   /* __sinit: bl __ct__4WorkFv */
__declspec(noinline) inline Work::Work() { VEC3_ctor(&pos); }   /* emitted after __sinit */
```
