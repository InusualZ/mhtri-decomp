---
id: 238
title: A dead counter beside a bdnz loop is a u8 loop variable
status: works
problem: Retail keeps `li r4,0` before the loop and `addi r4,r4,1` beside the `bdnz` though nothing reads r4; ours with an `s32 i` drops the counter entirely.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 238. A dead counter beside a bdnz loop is a u8 loop variable

**Problem.** A fixed-count search loop (`for (i = 0; i < 6; i++, buf++)`) that MWCC turns into `mtctr`/`bdnz`: retail
still initialises and increments a register that no instruction reads (`li r4,0` ... `addi r4,r4,1`), and ours
emits neither, so every later instruction is shifted by one or two and the row reads ~92 %.

**Why it happens.** With an `int` counter the counted-loop pass replaces the induction variable by the CTR and the
dead-code pass deletes it. A `u8` counter carries an implicit narrowing on every increment, which the counted-loop
pass does not fold away, so the variable survives as a dead increment.

**How to work it.** Declare the loop variable `u8` (or another narrow type) when retail shows a counter that only
counts. Do not use it where the counter is also an array index: there the narrow type adds a `clrlwi` per use and
breaks the strength-reduced pointer (measured: `em_stat_level_pick` 91 -> 74 %).

**Result.** `enemy/enemy_control` (Wii/1.3, -O3): `em_res_buffer_find` 92.23 -> 100 % (three search loops),
`em_ride_hit_check`/`emc_event_act_end` first loops likewise (`s16 j` for a loop bounded by an `s16`).

**Example.**

```
EmResBuffer* buf = work->res_buf;
u8 i;                                  /* s32 i: the li/addi pair disappears */
for (i = 0; i < 6; i++, buf++) {
    if (buf->used == 0 && buf->size_class == cls) {
        return buf;
    }
}
```
