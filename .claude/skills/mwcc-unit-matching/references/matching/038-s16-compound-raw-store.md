---
id: 38
title: A compound assignment with an operand of the field's own type stores the field raw
status: works
problem: A store into a narrow field (`s16`/`u16`/`u8`) comes out with an extra `extsh`/`clrlwi` next to it where retail stores the value as it stands. The function sits at 82-94 % with one extra instruction and every later register shifted.
tags: [source-shape]
applies: [Wii/1.3]
demo: 038-s16-compound-raw-store.cpp
reviewed: 2026-09-29
related: [39, 57, 66, 72]
---

# 38. A compound assignment with an operand of the field's own type stores the field raw

**Problem.** A store into a narrow field (`s16`/`u16`/`u8`) comes out with an extra narrowing instruction
(`extsh` for `s16`, `clrlwi` for `u16`/`u8`) next to it where retail stores the value as it stands. The function
sits at 82-94 % with one extra instruction and every later register shifted, and no cast, temporary or operand
order in the store itself moves it. (Title sharpened 2026-09-29: the original said "`s16` parameter", but the
measurement shows it is the operand's *type matching the field*, and the flag matters - see below.)

**How it looks.** Ours has one extra instruction the target lacks, right before the store:

```
ours:    lhz r0,10(r3) ; add r0,r0,r4 ; clrlwi r0,r0,16 ; sth r0,10(r3)     (mask, then store)
retail:  lhz r0,10(r3) ; add r0,r0,r4 ;                    sth r0,10(r3)     (raw store)
```

**Why it happens.** C converts the result of `field = field + amount` to the field's width, and MWCC emits that
narrowing (`extsh`/`extsb`/`clrlwi`) before the store - **unless the peephole pass folds it into the store**
(with the peephole on, `sth`/`stb` store only the low bits, so the mask is redundant and is removed; idea 39).
So the mask survives exactly in units built with `-opt nopeephole` (like the `Pl` library's `cflags_pl`). A
compound assignment (`field += amount`) is evaluated differently: the *operand* is converted to the field's type
first (`amount` masked/extended once, on the way in) and the sum needs no narrowing afterwards. When the operand is
already the field's type (an `s16` parameter added to an `s16` field), no conversion happens anywhere and the store is
raw.

**How to work it.** In a unit that keeps the mask (peephole off), spell the update as a compound assignment and
give the parameter the **field's own type**:

```c
void fn_8027A044(s16 amount) {   /* the parameter has the field's type, s16 */
    self->angle += amount;       /* compound assignment, not self->angle = self->angle + amount */
}
```

Measured on the four spellings (Wii/1.3, `-O3 -inline noauto -opt nopeephole`, an `s16` field):

| spelling | instructions | note |
| --- | --- | --- |
| `f += a`, `a` is `s16` | `lha; add; sth` | raw store |
| `f = f + a`, `a` is `s16` | `lha; add; extsh; sth` | narrowed before the store |
| `f += a`, `a` is `s32` | `lha; extsh; add; sth` | operand narrowed on the way in |
| `u16` field, `f += a`, `a` is `s16` | `lhz; clrlwi; add; sth` | operand masked; a `u16` operand is raw |

**When NOT to apply.** With the peephole **on** (the base `-O4,p` command line) plain assignment is already raw and
`+=` with a mismatched operand type *adds* a mask on the operand (`h_cmp_s16` on a `u16` field: `clrlwi r0,r4,16`),
so this trick is the wrong way round there: the prose of the original idea described the opposite effect, and the
first demo (base flags) contradicted it. Decide the peephole state of the unit first (idea 39, `infer.py`) and
match the operand type to the field before trying anything else. The lever fixes a narrowing extra instruction; it
does not explain a wrong offset (see the layout bugs below).

**Result.** Measured at the time on `Pl/pl_act`: 97.86864 -> **98.43198 %**: matched bytes 10648 -> 12152,
byte-exact functions 73 -> **80 of 115**, nineteen functions improved and **zero regressions**, `.text` still the
target's exact 27436 B, no flag change. `fn_8027A044` 82.2 -> 100 (still in `src/Pl/pl_act.cpp`: `s16` field
`unk5E8`, `self->unk5E8 += arg1;`), 76CE8 92.7 -> 100, CA48 94.0 -> 100, C8B4/D5A4 -> 100, 76E08 -> 99.9, 78674 ->
99.0, B0BC 94.0 -> 95.7.

**Sibling shapes from the same round** (each worth trying before assuming a flag): the **field form**
(`self->equipC[0]`) for a load immediately followed by pointer formation (79414 -> 100, `Get_Shell_rate_adj` ->
99.9); `(u32)` on an `s32` helper to get `cmplwi` (idea 72); `-0x1006` to get `li`; `(x * 7) << 1` for `* 14`. Two
*real* layout bugs fell out of the same sweep: one function stored into 0x449 where retail writes 0x44C, and
`_MOVE_WORK.unk44F` was declared one byte too long.

**Demonstration.** `038-s16-compound-raw-store.cpp` (`ideas.py demo-check 38`), compiled with the `Pl` library's
flags. The first demo used the base flags (peephole on) and found no masked store in either spelling; re-measuring
under `-opt nopeephole` produced the table above, which is what the demo now asserts. Verified 2026-09-29 by
compiling the same functions with both peephole states.
