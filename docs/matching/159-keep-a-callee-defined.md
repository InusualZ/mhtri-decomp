---
id: 159
title: Keep a callee defined ahead of its caller out of line: auto_inline off around the callee's definition
status: works
problem: Written in retail's function order, a large callee defined ahead of its caller is folded into it (the caller grows by the callee's size), and dont_inline also unfolds the callee's own helpers
tags: [pragma, source-shape]
applies: [DWCi]
demo: 159-keep-a-callee-defined.cpp
---

# 159. Keep a callee defined ahead of its caller out of line: auto_inline off around the callee's definition

**Problem.** A unit written in retail's function order has a callee defined ahead of its caller, and retail keeps
the call a `bl`. `-inline auto` folds the callee into the caller instead (here 0x334 and 0x308 B handlers made the
caller grow from 0x7BC to 0xD54 B), so the caller reads ~22 % although its own source is right. Defining the callee
after the caller avoids the fold but breaks the object's function order; `#pragma dont_inline on` around the
callee's definition keeps the call but also stops the callee's own static inlines from folding (the callee then
drops to ~30 %).

**Why it happens.** `-inline auto` lets MWCC inline any function whose body it has seen, whatever its size class
here; the decision for a callee is recorded with its definition. `#pragma auto_inline off` at definition time marks
the function as "not for automatic inlining" without touching the calls compiled inside it, so its explicitly
`inline` helpers still fold; `dont_inline` turns off inlining for the calls inside the scoped code as well (idea 61).

**How to work it.** Bracket the callee's *definition* - not the caller - with `#pragma push` / `#pragma auto_inline
off` / `#pragma pop`, keep retail's function order, and re-measure both functions.

**Result.** `DWCi/gt2Message`: with `gti2HandleServerChallenge` and `gti2HandleClientResponse` in retail order
ahead of `gti2HandleReliableMessage`, the caller read 22.12 %; `dont_inline` around the two definitions restored it
(96.82 %) but dropped them to 30.78 % / 33.26 %; `auto_inline off` kept all three at their best (96.82 / 100 /
99.75 %).

**Example.**

```
#pragma push
#pragma auto_inline off
s32 gti2HandleServerChallenge(DWCiReq* connection, u8* message, s32 len) { ... }
s32 gti2HandleClientResponse(DWCiReq* connection, u8* message, s32 len) { ... }
#pragma pop

s32 gti2HandleReliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len) { ... /* bl both */ }
```

**Demonstration.** `159-keep-a-callee-defined.cpp`: the plain callee is folded into its caller (three `bl other`),
the `auto_inline off` one stays a call while its static inline still folds (two compares), the `dont_inline` one
stays a call and calls its helper out of line (one compare, a `helper` relocation).
