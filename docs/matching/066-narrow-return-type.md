---
id: 66
title: A narrow RETURN TYPE is visible at the caller
status: works
problem: A caller is one instruction off at the `bl`: retail uses the callee's result raw where ours masks it (`clrlwi`/`extsh`) - or the reverse - and every later instruction shifts. It reads as caller-side scheduling, where a mask cannot come from.
tags: [source-shape]
applies: []
demo: 066-narrow-return-type.cpp
reviewed: 2026-09-29
related: [57, 72]
---

# 66. A narrow RETURN TYPE is visible at the caller

**Problem.** The caller is one instruction off right after the `bl`: retail uses the result as it stands,
ours masks it first - a `clrlwi` (zero-extend a 16-bit value) or `extsh` (sign-extend) retail does not
have - or retail masks and ours does not, and every later instruction shifts with it. It reads as
caller-side scheduling, where a mask cannot come from.

**How it looks.** The first divergence is `clrlwi r3,r3,16` / `extsh` (or its absence) straight after the
`bl`, or on the first *use* of the result.

**Why it happens.** MWCC converts a value to the **declared** width of what it came from and of where it
goes, so a mask at the call site is evidence about the *callee's signature*, not about the caller's source.
This is idea 57 in the return position: there the callee's parameter was declared wider than the value; here
its return type is declared narrower (or wider) than the value the caller wants.

**Scope, as measured (2026-09-29, `-O4,p -inline auto`, Wii/1.3).** The mask appears **where the value is
used**, not at the store:

* returning a wide (`s32`) callee's result as `u16`: one `clrlwi`; returning a `u16` callee's result as `u16`
  is a bare tail call (4 bytes);
* **storing** the result into a `u16` field is a raw `sth` for both callee types - no mask at the store;
* a `u16` local fed by a wide callee (`u16 t = wide(); s->g = t + 1;`) is masked before the arithmetic; fed
  by a `u16` callee it is not; an `s32` local fed by a `u16` callee is masked again (return type and use
  disagree).

The original observation (the target's raw `sth` came back after `flag_48` was declared `u16`) is therefore
this idea seen through a use that also needed the value elsewhere - the store alone does not distinguish the
two declarations.

**How to work it.** When a `clrlwi`/`extsh` follows a `bl` (or is missing), change the **callee's declared
return type** to the width the target's uses imply, then re-measure every caller of that callee.

**When NOT to apply.** If both spellings store the result raw and nothing else reads it, this idea cannot
explain the diff. Also beware the callee is shared: fix it once and let the compiler list the callers.

**Result.** Measured at the time (2026-09): `NetworkLogger::flag_48` is `u16`, not `s32`: `receive`
**92.30 -> 94.99**, `send` +0.03. The other direction: `registerReceiver` is declared `s32` while its call
sites cast to `u8` - retail's early returns are `li r3, -1` (a wide -1, not `li r3, 0xff`) and the tail masks
the `u8` it returns.

**Example.**

```c
u16 t = flag_narrow();  s->f = t;  s->g = t + 1;    /* no mask: the u16 return is trusted   */
u16 t = flag_wide();    s->f = t;  s->g = t + 1;    /* clrlwi before the add                */
```

**Demonstration.** `066-narrow-return-type.cpp` (`ideas.py demo-check 66`) pins all of the above: the
`return` mask, raw `sth` for both stores, and the mask on the arithmetic use. (Stage-3's first demo only
showed the return position and reported the store form as not reproduced; the store is indeed raw either way,
and the idea's text now says so.)
