---
id: 87
title: `-sdata 0` for a game unit
status: ruled-out
problem: The target shows absolute `lis`/`addi` addressing for a global where our build uses small-data, which reads as a unit that was built with `-sdata 0`.
tags: [flags, data]
applies: []
demo:
---

# 87. `-sdata 0` for a game unit

**Problem.** The target shows absolute `lis`/`addi` addressing for a global where our build uses small-data, which reads as a unit that was built with `-sdata 0`.

**Why it does not work.** Measured on `Pl/pl_skill.cpp` (batch 6): it turns the target's absolute `lis`/`addi` to `lobby_w` into small-data accesses and breaks the two functions that *should* be small-data, so on the pre-fix source it was +0.085 on two functions and -0.11 on two others (net -0.03), and with the real fix landed it is purely harmful (-0.12 pt). The absolute access a target shows is a property of *that symbol's section*, not of the unit's addressing mode: declaring `lobby_w` as an unsized array (`lobby_w[0]`) gives the absolute form while `lbl_80792140/48` stay `@sda21`, which is what the target does.

**Result.** Ruled out (bullet of the old `ruled-out.md`); the working shape is playbook 64.
