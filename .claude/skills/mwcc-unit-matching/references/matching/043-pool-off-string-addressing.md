---
id: 43
title: Retail's per-string `lis`/`addi` addressing means `-str` had no `pool` - `-pool off` is not the lever
status: works
problem: Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base symbol retail never had.
tags: [flags, data]
applies: [Wii/1.3]
demo: 043-pool-off-string-addressing.cpp
reviewed: 2026-09-29
related: [23, 44, 45, 64]
---

# 43. Retail's per-string `lis`/`addi` addressing means `-str` had no `pool` - `-pool off` is not the lever

**Problem.** Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi`
displacements) where retail materialises each string with its own `lis`/`addi` (`lis` = load-immediate-shifted,
the `@ha` half of a 32-bit address; `addi` adds the `@l` half) - 0x20 bytes of `.text` short, and the
`.rela.text` records name a base symbol retail never had. (Title corrected 2026-09-29: it used to name `-pool off`
as the fix; that flag does not do this.)

**How it looks.**

```
ours (-str ...,pool):     lis r31,@stringBase0@ha ; addi r3,r31,@stringBase0@l ; ... ; addi r3,r3,12
retail:                   lis r3,str1@ha ; addi r3,r3,str1@l ; ... ; lis r3,str2@ha ; addi r3,r3,str2@l
```

Every string reference in retail carries its own `R_PPC_ADDR16_HA/LO` relocation pair against its own (anonymous
`@NN`) string symbol; ours share one `@stringBase0` symbol and one saved register.

**Why it happens.** `-str reuse,pool` (a `pool` sub-option of `-str`) makes the compiler put every string of the
unit in a single data object named `@stringBase0` and address them as base + displacement. Without `pool` each
literal is its own object. The separate `-pool[data] on|off` option ("pool like data objects", `mwcceppc -help`)
is not the string switch: the original probe that named it did not record its command line and cannot be re-run,
and the re-measurement below found **no** effect from it.

**How to work it.** Look at the unit's `-str` flags first: if retail addresses each string separately, remove
`pool` (the project's base has `-str reuse`, which is per-string; `cflags_runtime` adds `-str reuse,pool,readonly`,
which is pooled). It is a library/per-unit flag - there is no source pragma. Then check the string's *section*
(idea 44) and the `.data` layout.

**When NOT to apply.** Do not add `-pool off` hoping for this; measured 2026-09-29, `-pool off` (either order, with
or without `readonly`) leaves `@stringBase0` in place on **six** compilers (Wii 1.0, 1.1, 1.3, 1.5, 1.7 and GC
2.7), and it changed nothing for `static const` tables, non-const arrays, scalars or float constants either. The
project's `cflags_camellia` and `cflags_rso` carry `-pool off` from earlier evidence; it is harmless there, but
for those units the per-symbol `lis`/`addi` pairs the flag was credited with come from *tables* that are addressed
per symbol under every setting, so the credit was not demonstrated - a future pass could try dropping it from
those groups (measure; do not edit `configure.py` on this note alone, non-negotiable 3).

**Example.**

```
-str reuse            # per-string lis/addi (verified: no @stringBase0, 2 lis + 2 addi for two strings)
-str reuse,pool       # one @stringBase0, one lis
```

**Result.** Measured at the time on `auto/800CB948_fn_800CB948` (since retired): with the four `Panic` strings
written as literals, `-pool off` was reported to move `.rela.text` 0x2C4 -> 0x3B4 (retail's per-string pairs) and
`.text` 0xC44 -> 0xC64 with `.data` 0x00AD equal to retail. That result is the only evidence for `-pool off`
and is **not reproduced**: the same shape (two strings) under the unit's likely flags gives `@stringBase0` with
`-pool off` and per-string pairs only when `pool` is dropped. Treat the original attribution to `-pool off` as a
probable confound and re-measure with `-str reuse` first.

**Demonstration.** `043-pool-off-string-addressing.cpp` (`ideas.py demo-check 43`) compiles two strings with
`-str reuse,pool -pool off`: `@stringBase0` is still present, one `lis`, 0x3C bytes. The other half of the
statement (with `-str reuse` alone: two `lis`/`addi` pairs, 0x34 bytes, no `@stringBase0`) was verified by
compiling the same file with that flag (the demo checker allows one `FLAGS:` line per file).
