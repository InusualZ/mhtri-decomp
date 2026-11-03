---
id: 308
title: A temporary's cleanup record over an in-class inline member call: define the member out of class
status: works
problem: a function's `.text` is 100 % but its extab record is 0x18 B against retail's 8, with a cleanup range whose pc is the return address of an out-of-line call to an inline member (a `ut::Color` temporary converted through `operator u32` under `#pragma dont_inline on`); the record stays with `throw()`, `-inline off`, a placing stub ahead of the caller, and `#pragma exceptions off` drops the whole record
tags: [source-shape, sections]
applies: [Wii/1.3]
demo:
related: [155, 61]
---

# 308. A temporary's cleanup record over an in-class inline member call: define the member out of class

**Problem.** A function whose `.text` matches carries an extab cleanup range retail lacks: `204a0000 000001f0 00000010
00000000 82000008 <dtor>` (0x18 B) where retail has the bare `204a0000 00000000`. The range's pc is the return address of
the one call between the temporary's constructor and its destructor - an inline member of a header class
(`ut::Color::operator u32() const`), called out of line because the unit compiles under `#pragma dont_inline on`.
`flipcheck` refuses the unit on extab (+0x10) even though every row is 100 %.

**Why it happens.** MWCC guards a live temporary around every call it cannot prove nothrow (idea 155). An inline
member whose body is written **inside the class** never grants that knowledge, however early the class is seen and
however early its out-of-line copy is placed (a tail-call stub ahead of the caller places the copy first and the
record stays). An **out-of-class** `inline` definition does: the body is compiled at its definition, before the
caller, and the call is known nothrow. `throw()` does not help (155 says so too), `-inline off` in place of the
pragma changes nothing, and `#pragma exceptions off` around the caller removes the function's record entirely.

**How to work it.** Declare the member in the class and define it `inline Class::member() const { ... }` right after
the class body in the same header. Measure the other includers: the move is codegen-neutral for `.text` (hash every
built object before and after), and a unit that also converts a Color temporary loses the same cleanup range.

**Result.** `g3d/g3d_resanm`: fn_80089C6C's extab 0x18 -> 8 B (the target's), unit extab after the link 0x38 (claim
0x38), `.text` unchanged (10/10 rows at 100 %); 675 built objects re-hashed, only `g3d_anmscn.o` (NonMatching) moved -
`.text` identical, extab 0x2C8 -> 0x2B8 (one cleanup range fewer). flipcheck READY, trial flip `main.dol: OK`.

**Example.**

```cpp
struct Color : public _GXColor {
    /* ... */
    operator u32() const;           /* declared in class */
};

inline Color::operator u32() const { /* defined out of class: known nothrow at the call */
    return ToU32ref();
}
/* caller, under #pragma dont_inline on: return nw4r::ut::Color(r, g, b, a);  -> 8-byte extab record */
```
