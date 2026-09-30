---
id: 87
title: `-sdata 0` for a game unit
status: ruled-out
problem: The target addresses one global with absolute `lis`/`addi` where our build uses a small-data `@sda21` access, which reads as a unit built with `-sdata 0`; as a flag it fixes those rows and breaks the ones that should stay small-data.
tags: [flags, data]
applies: [Wii/1.3, Pl]
demo: 087-sdata-0-game-unit.cpp
reviewed: 2026-09-29
related: [23, 29, 64]
---

# 87. `-sdata 0` for a game unit

**Problem.** The target shows absolute `lis`/`addi` addressing for a global where our build uses small-data, which
reads as a unit that was built with `-sdata 0`. (Small data: the compiler keeps globals up to `-sdata N` bytes in a
64 KB area addressed off `r13`/`r2`, so an access is one instruction, `li rX,sym@sda21` or `lwz rX,sym@sda21(0)`;
absolute addressing is `lis rX,sym@ha` + `addi`/`lwz ...,sym@l`, two instructions.)

**How it looks.** The first divergence is an extra `lis` on the target side for one symbol, while other globals in the
same function are `@sda21` on both sides:

```
target:  lis r3,lobby_w@ha ; addi r3,r3,lobby_w@l     ours:  li r3,lobby_w@sda21
```

**Why it does not work.** Measured on `Pl/pl_skill.cpp` (batch 6): `-sdata 0` gives `lobby_w` the target's absolute
access, but also makes the byte tables absolute, which breaks the two functions that *should* be small-data (the
`lbl_80792140/48` byte tables *are* small data in the target): on the pre-fix source it was +0.085 on two functions and
-0.11 on two others (net -0.03), and with the real fix landed it is purely harmful (-0.12 pt). The demo
(`-sdata 0`) shows the flag's real effect: *every* global in the unit becomes `lis` + `lwz`, none is `@sda21`. The absolute
access a target shows is a property of *that symbol's section*, not of the unit's addressing mode: declaring `lobby_w`
as an unsized array gives the absolute form for that symbol only while `lbl_80792140/48` stay `@sda21`, which is
what the target does (idea 64, which has its own demo of the two spellings).

**How to work it.** When exactly one symbol is absolute in a unit that is otherwise small-data, give that symbol's
`extern` an **unknown size** (`extern u8 lobby_w[];`, idea 64) - the compiler cannot prove it fits the small-data area.
When *every* global in the unit is absolute in the target, `-sdata 0` (with `-sdata2 0`) is the right flag: the
project's `cflags_rel` carries it for the REL-type units, with its evidence.

**When NOT to apply.** Do not read this entry as "`-sdata 0` never applies": it is ruled out for the *game* unit it was
tried on (`Pl/pl_skill`), and legitimately in use for REL-type code. The deciding test is per symbol: are *all* the
unit's globals absolute in the target, or one?

**Result.** Ruled out for `Pl/pl_skill`; the working shape is idea 64, and it is still in the source: `src/Pl/pl_skill.cpp`
declares `extern u8 lobby_w[];`.

**Evidence.** (Measured at the time on `Pl/pl_skill.cpp`; the unit header records the numbers.) Related: 23 (data ranges
and what objdiff can fix), 29 (claimed literal pools), 64 (unknown-size extern).
