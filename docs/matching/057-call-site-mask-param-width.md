---
id: 57
title: A call-site mask comes from the callee's declared parameter type and the value's own type
status: works
problem: A caller sits at 50-90 % and the first divergence is one instruction at the `bl`: retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, `slwi`/`srawi`) and ours passes it through, or the reverse - nothing in the caller's source hints at it.
tags: [source-shape]
applies: []
demo: 057-call-site-mask-param-width.cpp
reviewed: 2026-09-29
related: [56, 66, 72, 38]
---

# 57. A call-site mask comes from the callee's declared parameter type and the value's own type

**Problem.** A wrapper (or any caller) sits at 50-90 % and the first divergence is one instruction at the `bl`:
retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, a `slwi`/`srawi` pair) and ours
passes it straight through (or the mirror: ours masks and retail does not). It reads as a scheduling or inlining
residual, nothing in the caller's own source hints at a mask, and the search goes to flags and to the caller's
statement order - where the mask cannot exist.

**How it looks.**

```
target:   clrlwi r4,r4,16        ; mask a 32-bit value down to 16 bits
          bl     take_narrow
ours:     bl     take_narrow      ; the mask is missing (or, in the mirror case, ours has one retail lacks)
```

(`clrlwi rA,rS,16` clears the high 16 bits - a `u16` conversion; `extsh` sign-extends a halfword - an `s16`.)

**Why it happens.** The mask is the *caller's* cost of a type conversion at the call, and MWCC decides it from two
declarations, neither of which is in the caller's body: the **callee's declared parameter type** (the prototype)
and the **value's own declared type**. Measured 2026-09-29 (`ideas.py demo-check 57`, `-O4,p -inline auto`):

| value passed | parameter | mask at the call |
| --- | --- | --- |
| `u32` | `u16` | `clrlwi r3,r3,16` |
| `u32` | `u32` | none |
| `int` sum | `s16` | `extsh` |
| `int` sum | `int` | none |
| `s16`-typed value | `int` | **`extsh` (re-extended anyway)** |
| `s16`-typed value | `s16` | `extsh` |

So a parameter declared **narrower** than the value adds a mask (widen the prototype to remove it), while a
**narrow-typed value** is re-extended whatever the parameter is (only the value's declared type can remove it).

**How to work it.** Find out which side owns the mask before editing.

* Retail masks and ours does not: the retail callee's parameter is *narrower* than the value you pass (or retail's
  value is narrow-typed): **narrow our prototype** (`u16`/`s16`/`u8`) or give the local/field the narrow type.
* Ours masks and retail does not: our prototype is narrower than retail's, or our local is narrower-typed than
  retail's: **widen the parameter** (`s32`/`u32`; narrow explicitly at the use inside the callee if its own code needs
  it) or widen the local's declared type and mask at the use.
* Check the callee's *own* row after touching its prototype: the type is shared by every caller (ideas 60/72: the
  declared shape is a codegen input), so measure the whole tree, not one wrapper.

**Result.** Measured across one `ai` band without any flag change: `fn_802D2ABC` 56.7 -> 90, `fn_802D287C`
85.9 -> 93.8, `fn_802D2264` 77.9 -> 81.7, and ten thin wrappers went to **100 %**. The mirror case is the same
lever: `fn_802D3984` (76.14 %) had our mask *too* wide, i.e. a parameter declared wider than retail's. The presence
*or absence* of the mask is a statement about declared types rather than about the caller's own statements.

**When NOT to apply.** A mask on a *return value* is idea 66 (the callee's return type). A store through a narrow
field is idea 38. Do not assume the direction: read which side has the `clrlwi`/`extsh`, and if it is an `s16`-typed
value, widening the callee's parameter will not remove it (last table row) - the value's type has to change.

**Demonstration.** `057-call-site-mask-param-width.cpp` (`ideas.py demo-check 57`, re-verified 2026-09-29):

```
pass_to_narrow(u32 v)   { take_narrow(v); }   // callee takes u16: clrlwi r3,r3,16 ; b take_narrow  (8 bytes)
pass_to_wide(u32 v)     { take_wide(v);   }   // callee takes u32: b take_wide                       (4 bytes)
s16_value_to_s32(s16 v) { take_s32(v);    }   // extsh r3,r3 although the parameter is int
```

**Finding (recorded by the 2026-09-29 review).** The idea's first wording said a callee parameter declared
`s32`/`u32` "forces a mask on a narrower value" and prescribed widening the parameter. The compiler shows the two
cases separately (table): widening the prototype removes a mask only when the value is at least as wide as the
parameter was declared; an `s16`-typed value keeps its `extsh` for an `int` parameter. The title and the direction
rules above were corrected accordingly.

**Evidence.** The `ai`-band numbers are dated evidence from the session that found the rule. The layout calculator
and the record-merge traps that came out of the same band (a `field_0x216` declared `s32` that was a `lbz`; a
header defining more than one struct) are idea 56's evidence: they concern merging shared work records, not
call-site masks.
