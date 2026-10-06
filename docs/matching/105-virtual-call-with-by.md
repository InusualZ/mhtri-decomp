---
id: 105
title: A virtual call with by-value vector arguments and the parameter order behind the prologue's moves
status: works
problem: A slot call loads its table through the saved object (`lwz r11,0x1C(r29)`) where retail goes through r3, two argument copies swap stack slots, and the prologue moves the last integer argument one slot early
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [22, 63]
---

# 105. A virtual call with by-value vector arguments and the parameter order behind the prologue's moves

**Problem.** Every NW4R emitter-form row (`ef_cube`, `ef_cylinder`, `ef_disc`, `ef_line`, `ef_point`, `ef_sphere`,
`ef_torus`) showed the same three residuals once its body was right:
- the spawn's slot call loads the table through the saved object (`lwz r11,0x1C(r29)`) where retail goes through
  r3 (`lwz r12,0x1C(r3)`);
- the two copies of the position and velocity take each other's stack slots, and the `fmr f1,<momentum>` is a few
  slots late;
- the prologue moves the 8th integer argument (`mr r28,r10`) one slot before the float argument's `fmr`.

**Why it happens.** The table is a real vtable. The particle manager is a class whose first 0x1C bytes come from a
non-polymorphic base, so MWCC puts the vtable pointer after them, at +0x1C. The spawn is a virtual
`CreateParticle(u16 life, VEC3 pos, VEC3 vel, ...)` taking both vectors **by value**. A by-value aggregate argument
is copied into a compiler temporary as the call starts (velocity first, the lower slot). The scalar arguments are
then evaluated right to left (the momentum before the life). A genuine virtual call sets r3 and loads the table
through it.

The prologue order comes from the parameter list: `Emission(..., u16 life, f32 lifeRnd, const MTX34* space)` has
the float **before** the last integer. The registers are the same either way (r10 and f1), but MWCC saves the
arguments in declaration order.

**How to work it.**
1. Declare the manager as `struct Head { u8 pad[0x1C]; }; struct Pm : Head { virtual ... x3; virtual void
   CreateParticle(u16, VEC3, VEC3, ...); };`. Declare it only, so the unit emits no table (rule 10).
2. Call `pm->CreateParticle(calc_life(...), pos, vel, space, 1.0f + 0.01f * k * rand(), ...)` with the vectors
   by value and the momentum written in the argument list. Do not use pre-copied locals.
3. Put the float parameter before the last integer one. For a mangled row, the mangling changes (`...Uslf` ->
   `...Usfl`), so rename the map row (`symedit.py rename`) in the same change.
4. Declare the helpers with their owner's argument order and result type (`u16 calc_life(ctx, id, f32, em)`). The
   wrong order schedules the `fmr`/`mr` pair the other way, and a `u32` result adds `mr r0,r3`.

**Result.**

| Row | Before | After |
|---|---|---|
| `fn_800C9540` (torus) | 99.23 | 100 |
| `fn_800CCFB0` (line) | 98.03 | 100 |
| `fn_800CD584` (point) | 91.28 | 100 |
| `fn_800C9DD0` (cube) | 97.26 | 100 |
| `fn_800CB948` (cylinder) | 95.0 | 100 |
| `fn_800CBFB0` (cylinder) | 96.09 | 100 |
| `ef_sphere_spawn` | 98.63 | 99.72 |
| `fn_800CA200` (cube) | 99.63 | 99.77 |

Every row at 100 % is byte-identical, target against ours. Initialising the copies (`VEC3 a = b;` instead of
`a = b;`) is the partial step: it turns retail's `lwz`/`stw` copy back on, but the slots stay swapped.

**Example.**

```
struct PmHead { u8 pad_0x00[0x1C]; };
struct Pm : PmHead {
    virtual void slot_0x08(); virtual void slot_0x0C(); virtual void slot_0x10();
    virtual void CreateParticle(u16 life, VEC3 pos, VEC3 vel, s32 space, f32 momentum, u8* inherit, u32 ref,
                                u16 remain);
};
void Emission(s32 ctx, EfWork* em, Pm* pm, s32 n, u32 flags, EfParams* p, u16 life, f32 lifeRnd, s32 space) {
    ...
    pm->CreateParticle(calc_life(ctx, life, lifeRnd, em), pos, vel, space,
                       1.0f + 0.01f * (f32)em->scale_rate * ef_random_float(&em->progress), ...);
}
```
