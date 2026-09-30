---
id: 61
title: A kept `bl` inside one function: scope `#pragma dont_inline on` to it
status: works
problem: A `switch` case calls a small file-local helper that retail keeps as a `bl`, but our `-inline auto` folds the callee's body into the case: the kept call disappears, the case grows by the callee's size and every later register and offset shifts. It reads as a missing helper or a wrong case body, and no source shape *inside* the case recovers it - the callee is gone.
tags: [pragma, source-shape]
applies: []
demo:
---

# 61. A kept `bl` inside one function: scope `#pragma dont_inline on` to it

**Problem.** A `switch` case calls a small file-local helper that retail keeps as a `bl`, but our
`-inline auto` folds the callee's body into the case: the kept call disappears, the case grows by the
callee's size and every later register and offset shifts. It reads as a missing helper or a wrong case body,
and no source shape *inside* the case recovers it - the callee is gone.

**Why try it.** The inline decision row 28 names is **unit-wide** (`-inline noauto` in the library's
`cflags_*`), which also de-inlines every call that unit wanted folded. `#pragma dont_inline on` ...
`#pragma dont_inline off` bracketing **one function** keeps that function's callees out of line and leaves
the rest of the TU alone - the per-function inverse of `-inline noauto`, and the reason to prefer it when
only one call site needs the kept `bl`. It is defensible where row 33's per-function warning does not bite,
because the original build did not inline that call either (the callee is called from another TU too, so it
demonstrably existed as a symbol): the pragma restores the original's *call*, not codegen the original never
had. It also does not de-inline anything else, so the unit's other `bl`s keep the folding they matched with.

**Result.** `Network/network_state.cpp`'s `handleNetworkState1` measured **80.01 -> 82.50 %** with the pair
around that one function alone (retail keeps `bl resetNetworkState3` in case 255; `-inline auto` folds the
56-byte `resetNetworkState3`'s body into the case). The same lever carried two `lobby` units:
`fn_8020C588.cpp` measured all three spellings - `#pragma inline off`, `#pragma inline_depth 0` and
`#pragma dont_inline on` each keep the `bl fn_80212060` (220 B; with `-inline auto` `fn_8021213C` came out
276 B against a 92 B target) - and kept `dont_inline on`; `fn_801EC9F8.cpp` uses the same pair for
`fn_801ED3E8`/`fn_801ED464` (with `-inline auto` both are folded into `fn_801ED688`, 860 -> 1556 B, 7.00 %;
95.86 % with the pragma). The *lib-wide* alternative of row 28 also fixes `fn_8020C588`, but it moved four
other `lobby` units' numbers (`fn_801E7530` 27.65 -> 31.76, `lb_npc` 13.37 -> 13.75, `fn_8021E1EC`
9.22 -> 9.66) - the pragma keeps the deviation inside the one unit that needs it. The pair may bracket one
function or a whole file, whichever region needs the kept calls; both spellings measure the same codegen.

**Example.**

```c
#pragma dont_inline on          /* scoped to the one function that needs the kept `bl` */
s32 handleNetworkState1(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    switch (st->sessionState_6132) {
    case 255:
        resetNetworkState3(self);       /* retail keeps this `bl` */
        chooseServerAddress(st, 0, 0);
        updatePatInterface(st, 0, 0, 0);
        return 1;
    default:
        break;                          /* section 34's shared tail */
    }
    return 0;
}
#pragma dont_inline off
```
