---
id: 192
title: A retail frsp on an f32 value that was just stored raw is a (f32)(f64) round trip
status: works
problem: retail stores an f32 parameter or return value with plain `stfs f1` and then rounds the same register (`frsp f1,f1` / `frsp f0,f1`) for a compare or a second store; an `f64` parameter hoists the `frsp` above the stores instead
tags: [source-shape]
applies: [Wii/1.3]
related: [181]
demo: 
---

# 192. A retail frsp on an f32 value that was just stored raw is a (f32)(f64) round trip

**Problem.** Retail stores an `f32` value straight from `f1` (`stfs f1,0x30(r30); stfs f1,0x34(r30)`) and only then
rounds the same register (`frsp f1,f1` before an `fcmpo`, or `frsp f0,f1; stfs f0,...` for a second field). Ours has no
`frsp`; turning the parameter into `f64` produces one, but MWCC hoists it above the stores and every store then uses
the rounded register.

**Why it happens.** MWCC treats an `f32` parameter or return value as already rounded, so a plain `(f32)` cast is a
no-op. A cast through `f64` first makes the expression a double, and narrowing a double emits `frsp` at the point of
use - the stores of the plain value keep the raw register.

**How to work it.** Keep the value `f32`, and spell the one use retail rounds as `(f32)(f64)value`. Idea 181 is the
other source of the same instruction (a value read back through a field or temporary); try this one first when the
`frsp` sits after stores of the same register rather than before a negate.

**Result.** `lobby/lb_menu_pos_tbl`: `syncItemListClock` 97.87 -> 100 (`if ((f32)(f64)depth >= 0.5f)`), and
`lb_event_schedule_step` 94.49 -> 96.96 with `state->depth_0x34 = (f32)(f64)phase;` (`phase` the `f32` result of
`NetCtrlWk::getSchedulePhase`), 100 once its elapsed-frames local was an `f32`.

**Example.**

```
void sync(f32 depth) {
    state->scroll = depth;          /* stfs f1 */
    state->depth = depth;           /* stfs f1 */
    if ((f32)(f64)depth >= 0.5f) {  /* frsp f1,f1 ; fcmpo */
```
