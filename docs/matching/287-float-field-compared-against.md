---
id: 287
title: A float field compared against a constant colours the constant second: read the field through a local assigned inside the condition
status: works
problem: In `if (0.0f != em->field)` retail loads the constant into f0 and the field into f1 (`fcmpu cr0, f0, f1`), ours the other way round; operand order, a `zero` local and inverted tests do not move it.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [63, 165, 243, 280]
---

# 287. A float field compared against a constant colours the constant second: read the field through a local assigned inside the condition

**Problem.** A float-field-against-constant test (`if (0.0f != em->dir_weight)`) is byte-equivalent except for
the two scratch registers: retail has `lfs f0, zero; lfs f1, 0x68(r28); fcmpu cr0, f0, f1` and every later use of
the constant (the stores of a zeroing `else`) in f0, ours has the constant in f1 and the field in f0. Swapping the
operands, testing `if (w)`, a `zero` local, a reversed or chained zeroing and an inverted test all keep ours.
Only some of the tests in one function show it; the others already match with the field read directly.

**Why it happens.** Both loads are backend temporaries, numbered in creation order: the constant's temporary is
created first (it is the left operand), so it has the lower number, is pushed first in simplify and popped last
in select - the field's temporary takes f0 and the constant f1 (idea 243's model, FPR class; the dump's
`regalloc-fpr-pass-1-all.txt` shows `f37 -> f1` for the constant and `f38 -> f0` for the field). For retail's
colouring the field must be numbered *below* the constant, which is the position of a **named local**: locals
are numbered under every backend temporary, so a field read through a local is popped after the constant and
the constant takes f0. A plain `w = em->field; if (0.0f != w)` gets the registers right but emits the field load
*before* the constant load (the assignment is its own statement); writing the assignment inside the condition,
`if (0.0f != (w = em->field))`, evaluates the constant first and still stores into the local.

**How to work it.** Declare one `f32` local per test that needs the retail colouring, and read the field through
it inside the condition. Leave the tests that already match reading the field directly: the local only moves the
field below the constant, so a test retail compiled with the constant second must keep the direct read. Check the
dump's FPR assignment (`tools/mwcc-debugger`, `regalloc-fpr-pass-1-all.txt`) when the pattern is unclear.

**Result.** `ef/ef_emitterform`, `CalcVelocity` (1532 B): 99.856 % -> 100 % (the plain-assignment form measured
99.373 %, registers right and loads swapped); the unit flipped, `main.dol: OK`.

**Example.**

```
    f32 dir_weight;
    f32 rot_weight;
    ...
    if (0.0f != (dir_weight = em->dir_weight)) {   /* lfs f0,zero; lfs f1,0x68(r28); fcmpu cr0,f0,f1 */
        vec3_scale_by(out, rot);
    } else {
        out->x = 0.0f;                              /* stfs f0,0(r27) - the constant's temporary, f0 */
        ...
    }
    if (0.0f != (rot_weight = em->rot_weight)) { ... }
    if (0.0f != em->spread_scale) { ... }           /* retail: lfs f1,zero; lfs f0,0x70(r28) - direct read */
```
