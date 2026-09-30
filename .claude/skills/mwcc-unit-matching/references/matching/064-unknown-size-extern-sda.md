---
id: 64
title: An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form
status: works
problem: A band declares `extern const char lbl_80793998[];` and the caller materialises the address with `lis r5, sym@ha` + `addi r5, r5, sym@l`, where the target has a single `li r5, sym@sda21`. The extra instruction shifts every later register and offset, so the function reads as a scheduling or source-order residual.
tags: [data, source-shape]
applies: []
demo:
---

# 64. An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form

**Problem.** A band declares `extern const char lbl_80793998[];` and the caller materialises the address with
`lis r5, sym@ha` + `addi r5, r5, sym@l`, where the target has a single `li r5, sym@sda21`. The extra
instruction shifts every later register and offset, so the function reads as a scheduling or source-order
residual.

**Why try it.** MWCC cannot place an array of **unknown size** in the small-data area - its extent is not
known at compile time - so it falls back to the absolute `ha`/`lo` pair; a **scalar** `extern` is `sda21`
either way, so the unknown *size* is what forces the pair. The target object's relocations say which shape
each of the band's externs wants: `R_PPC_EMB_SDA21` is the SDA form, `ADDR16_HA`/`ADDR16_LO` the absolute one
(`powerpc-eabi-readelf -r build/RMHE08/obj/<unit>.o`). One header legitimately holds both.

**Result.** `fn_8041B538` **98.61 -> 99.92**, `ConnectToAnybody` **96.90 -> 100**. Probed with the unit's own
command line over `[]`, `[4]`, `[3]`, `char` vs `const char` and `u8`: only a known-size spelling gives the
SDA form. `lbl_80794380` is `ADDR16_HA`/`LO` in the *same* object as the SDA pair, so the fix is per-symbol,
not per-header.

**Example.**

```c
extern const char lbl_80793998[];      /* lis + addi  - absolute */
extern const char lbl_80793998[4];     /* li sym@sda21 - retail   */
```
