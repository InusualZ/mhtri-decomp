---
id: 64
title: An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form
status: works
problem: A caller materialises a global's address with `lis r5, sym@ha` + `addi r5, r5, sym@l` where the target has a single `li r5, sym@sda21`; the extra instruction shifts every later register and offset.
tags: [data, source-shape]
applies: []
demo: 064-unknown-size-extern-sda.cpp
reviewed: 2026-09-29
related: [23, 29, 70]
---

# 64. An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form

**Problem.** A band declares `extern const char lbl_80793998[];` and the caller materialises the address with
`lis r5, sym@ha` + `addi r5, r5, sym@l`, where the target has a single `li r5, sym@sda21`. The extra
instruction shifts every later register and offset, so the function reads as a scheduling or source-order
residual.

**Terms.** SDA (small data area) is the 64 KB region addressed off a dedicated base register (`r13`/`r2`), so
a symbol in it takes one instruction (`sym@sda21`); outside it a symbol needs the absolute pair `@ha` (high 16
bits, adjusted) + `@l` (low 16 bits).

**How it looks.** Ours has `lis`+`addi` (relocations `R_PPC_ADDR16_HA`/`_LO`) where the target has one `li`
(`R_PPC_EMB_SDA21`).

**Why it happens.** MWCC cannot place an array of **unknown size** in the SDA - its extent is not known at
compile time - so it falls back to the absolute pair; a **scalar** `extern` is `sda21` either way, so the
unknown *size* is what forces the pair. The target object's relocations say which shape each of the band's
externs wants (`powerpc-eabi-readelf -r build/RMHE08/obj/<unit>.o`). One header legitimately holds both.

**How to work it.** Read the target's relocation for that symbol, and give the `extern` its size when it is
`SDA21`. The fix is per symbol, not per header.

**When NOT to apply.** Only when the target really has the SDA form: `lbl_80794380` is `ADDR16_HA`/`LO` in
the *same* object as the SDA pair, and a wrong size on it would move it into the SDA. A known size above the
small-data threshold is absolute too: measured with the base cflags, `[8]` is `li ...@sda21` and `[16]`,
`[256]`, `[9000]` are `lis`+`addi` (the threshold is a compiler setting, so a unit with `-sdata 0` /
`-sdata2 0` in its flags never gets the SDA form at all).

**Result.** Measured at the time (2026-09): `fn_8041B538` **98.61 -> 99.92**, `ConnectToAnybody`
**96.90 -> 100**. Probed with the unit's own command line over `[]`, `[4]`, `[3]`, `char` vs `const char` and
`u8`: only a known-size spelling gives the SDA form.

**Example.**

```c
extern const char lbl_80793998[];      /* lis + addi  - absolute */
extern const char lbl_80793998[4];     /* li sym@sda21 - retail   */
```

**Demonstration.** `064-unknown-size-extern-sda.cpp` (`ideas.py demo-check 64`) reproduces it: `extern const
char x[]` is `lis` + `addi` (absolute), `extern const char x[4]` is one `li` with an `SDA21` relocation.
