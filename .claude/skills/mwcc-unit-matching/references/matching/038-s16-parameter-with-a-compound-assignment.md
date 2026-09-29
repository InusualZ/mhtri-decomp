---
id: 38
title: An `s16` parameter with a compound assignment is what makes a field store raw
status: works
problem: A store into a narrow field (`u8`/`u16`) comes out masked - a `clrlwi` before the `stb` - where retail stores the value as it stands. The function sits at 82-94 % with one extra instruction and every later register shifted, and no cast, temporary or operand order in the store itself moves it.
tags: [source-shape, allocator]
applies: []
demo:
---

# 38. An `s16` parameter with a compound assignment is what makes a field store raw

**Problem.** A store into a narrow field (`u8`/`u16`) comes out masked - a `clrlwi` before the `stb` - where retail
stores the value as it stands. The function sits at 82-94 % with one extra instruction and every later register
shifted, and no cast, temporary or operand order in the store itself moves it.

**Why try it.** That mask is MWCC narrowing the value to the field's width. Taking the value through a *signed*
16-bit parameter and writing the field with a **compound assignment** leaves the compiler holding a value that is
already the right width, so it stores it raw. It closed four functions outright in one round and improved four more.

**Result.** `Pl/pl_act` 97.86864 -> **98.43198 %**: matched bytes 10648 -> 12152, byte-exact functions 73 -> **80 of
115**, nineteen functions improved and **zero regressions**, `.text` still the target's exact 27436 B, no flag change.
A044 82.2 -> 100, 76CE8 92.7 -> 100, CA48 94.0 -> 100, C8B4/D5A4 -> 100, 76E08 -> 99.9, 78674 -> 99.0, B0BC
94.0 -> 95.7.

**Sibling shapes from the same round** (each worth trying before assuming a flag): the **field form**
(`self->equipC[0]`) for a load immediately followed by pointer formation (79414 -> 100, `Get_Shell_rate_adj` ->
99.9); `(u32)` on an `s32` helper to get `cmplwi`; `-0x1006` to get `li`; `(x * 7) << 1` for `* 14`. Two *real* layout
bugs fell out of the same sweep: one function stored into 0x449 where retail writes 0x44C, and `_MOVE_WORK.unk44F`
was declared one byte too long.

**Example.**

```c
void fn_8027A044(s16 amount) {   /* s16, not s32 and not u32 */
    self->field += amount;       /* compound assignment, not self->field = self->field + amount */
}
```
