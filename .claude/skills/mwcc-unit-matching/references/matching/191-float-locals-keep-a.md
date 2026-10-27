---
id: 191
title: Float locals keep a quad's UV fadds that literal arguments fold away
status: works
problem: retail computes 0.0f + 1.0f at run time (fadds f1,f30,f28 from two hoisted registers); ours loads a folded 1.0f from the pool per vertex
tags: [source-shape]
applies: []
demo: 
---

# 191. Float locals keep a quad's UV fadds that literal arguments fold away

**Problem.** A textured-quad writer emits `fadds f1,f30,f28` / `fmr f2,f30` per vertex, with 0.0 and 1.0 loaded once
before the loop into saved FPRs; passing the literals straight into an `inline` quad helper gives `lfs` of a folded
constant per vertex instead (several hundred bytes of positional mismatch in a long draw routine).

**Why it happens.** Inline expansion substitutes literal arguments into the body, so the front end folds `0.0f + 1.0f`.
The optimizer does not propagate float constants through a local, so a local's sum stays an `fadds`.

**How to work it.** Assign the UV origin and span to `f32` locals in the loop body and pass the locals to the helper.
Literal arguments that retail never adds to (`0.25f`, `0.125f`) can stay literals: they are hoisted the same way.

**Result.** `fn_80047398` / `face_draw` (0x9F8): 91.93 % -> 97.88 % with the locals (and a `FaceWork* work` base),
99.45 % after the remaining shape fixes.

**Example.**

```
base = 0.0f;
span = 1.0f;
GXLoadTexObj(&work->texobj[work->face_type[i]], 0);
face_quad_draw(0, 0, 128, 128, 255, base, base, span, span);
```
