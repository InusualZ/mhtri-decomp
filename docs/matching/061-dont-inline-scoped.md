---
id: 61
title: A kept `bl` inside one function: scope `#pragma dont_inline on` to it
status: works
problem: A `switch` case calls a small file-local helper that retail keeps as a `bl`, but our `-inline auto` folds the callee's body into the case: the kept call disappears, the case grows by the callee's size and every later register and offset shifts. It reads as a missing helper or a wrong case body, and no source shape *inside* the case recovers it - the callee is gone.
tags: [pragma, source-shape]
applies: []
demo: 061-dont-inline-scoped.cpp
reviewed: 2026-09-29
related: [16, 28, 32, 33]
---

# 61. A kept `bl` inside one function: scope `#pragma dont_inline on` to it

**Problem.** A `switch` case calls a small file-local helper that retail keeps as a `bl`, but our
`-inline auto` folds the callee's body into the case: the kept call disappears, the case grows by the
callee's size and every later register and offset shifts. It reads as a missing helper or a wrong case body,
and no source shape *inside* the case recovers it - the callee is gone.

**How it looks.** The first divergence is a run of stores (the folded body) where the target has one
`bl <helper>`; our function is longer than the target by the callee's size, and everything after shifts.

**Why it happens.** The inline decision of idea 28 is **unit-wide** (`-inline noauto` in the library's
`cflags_*`), which also de-inlines every call that unit wanted folded. `#pragma dont_inline on` ...
`#pragma dont_inline off` bracketing **one function** keeps that function's callees out of line and leaves
the rest of the TU alone - the per-function inverse of `-inline noauto`. It is defensible where idea 33's
warning against per-function flags does not bite, because the original build did not inline that call either
(the callee is called from another TU too, so it demonstrably existed as a symbol): the pragma restores the
original's *call*, not codegen the original never had.

**How to work it.** Bracket the one function (or the whole region that needs kept calls - both measure the
same codegen) with the pragma pair and re-measure the whole unit, not the function.

**When NOT to apply.** If several unrelated calls in the unit want to stay `bl`, that is the unit-wide setting
of idea 28, not this. If the callee has no other referrer in the target and no symbol of its own, retail may
simply have inlined it: check the target's symbol table first (a per-function pragma that "restores" a call the
original never made is idea 33's trap). Note the pragma also stops folding of *other* callees inside that
function; the demo shows the folding is back after `dont_inline off`.

**Result.** Measured at the time (dated 2026-09): `Network/network_state.cpp`'s `handleNetworkState1`
**80.01 -> 82.50 %** with the pair around that one function (retail keeps `bl resetNetworkState3` in case 255).

**Example.**

```c
#pragma dont_inline on          /* scoped to the one function that needs the kept `bl` */
s32 handleNetworkState1(NetworkInstance* self)
{
    switch (st->sessionState_6132) {
    case 255:
        resetNetworkState3(self);       /* retail keeps this `bl` */
        ...
    }
}
#pragma dont_inline off
```

**Demonstration.** `061-dont-inline-scoped.cpp` (`ideas.py demo-check 61`): the same `switch` body in three
functions - with `-inline auto` the static helper is folded (one `bl`), inside the pragma pair it is a `bl`
(two), and after `dont_inline off` it folds again.

**Evidence.** The same lever carried two `lobby` units: `fn_8020C588.cpp` measured `#pragma inline off`,
`#pragma inline_depth 0` and `#pragma dont_inline on` (all keep the `bl fn_80212060`, 220 B; with `-inline
auto` `fn_8021213C` came out 276 B against a 92 B target) and kept `dont_inline on`; `fn_801EC9F8.cpp` uses the
pair for two helpers (folded into `fn_801ED688`: 860 -> 1556 B, 7.00 %; 95.86 % with the pragma). The
lib-wide alternative of idea 28 also fixes `fn_8020C588` but moved four other `lobby` units (`fn_801E7530`
27.65 -> 31.76, `lb_npc` 13.37 -> 13.75, `fn_8021E1EC` 9.22 -> 9.66). Names and numbers as measured then.
