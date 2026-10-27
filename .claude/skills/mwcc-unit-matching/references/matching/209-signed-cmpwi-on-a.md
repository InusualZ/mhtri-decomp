---
id: 209
title: A signed cmpwi on a u8 or u16 value is a switch on the promoted operand
status: works
problem: Retail tests an unsigned byte or halfword (a u8 field, a u8 parameter, a call's u8 result) with a signed `cmpwi` where our `if (x == 2)` gives `cmplwi`, and the arms are often laid out in another order.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [109, 207]
---

# 209. A signed cmpwi on a u8 or u16 value is a switch on the promoted operand

**Problem.** Retail tests an unsigned byte or halfword with a signed compare (`lbz r0,0x170(r31)` /
`cmpwi r0,2` / `beq`) where our `if (x == 2)` on the same field gives `cmplwi`; the arms are often laid out in
another order too.

**Why it happens.** A `switch` compares its controlling expression after the integral promotions: a `u8`/`u16`
value promotes to `int`, so every case test is a signed `cmpwi`. An `if (x == 2)` keeps the unsigned type and
compiles to `cmplwi`. The switch also emits its case bodies in source order, with the default's body directly
after the compare chain.

**How to work it.** Rewrite the test as a `switch` on the value: `if (x != 2) A; else B;` becomes
`switch (x) { case 2: B; break; default: A; break; }` (the default body is emitted first, as retail's fall-through);
`if (x == 3) { ... }` becomes a one-case switch. A masked `u32` (`flag & 0xFF`) stays unsigned: write
`switch ((u8)flag)` (idea 207 is the other direction: a `(u32)` selector makes every test `cmplwi`). Then order the case bodies as retail lays them out, and re-measure: in a unit that has both
shapes, some tests really are `if`s.

**Result.** `ai/ai_npc` (Wii/1.3): `fn_802D1710` 93.00 -> 100 (with its stored-copy reads), `fn_802D1D90`
99.23 -> 100, `fn_802D348C` 97.84 -> 99.91, `fn_802D1AFC` 95.40 -> 96.98, `ai_npc_tuning_get` 98.38 -> 100,
`ai_npc_m2_step5_arm` 98.71 -> 99.19, `fn_802D1954` 93.54 -> 99.93 (with its other fixes). Case-body order alone
took `fn_802D0F34` 91.61 -> 100 and `fn_802CD588` 80.43 -> 100.

**Example.**

```
switch (self->variant) {    /* lbz; cmpwi r0,2; beq - an `if (self->variant != 2)` gives cmplwi */
case 2:
    fn_802D3474(self, 4, 0);
    break;
default:
    fn_802D3474(self, 0, 0);
    break;
}
```
