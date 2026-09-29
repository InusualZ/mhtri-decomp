---
id: 43
title: Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off`
status: works
problem: Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base symbol retail never had.
tags: [symbols, flags, allocator, data]
applies: []
demo:
---

# 43. Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off`

**Problem.** Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base symbol retail never had.

**Why try it.** `-pool off` stops the string pooling; the relocations become per-string and `.data` reproduces retail's pool. It has no source pragma, so it is a lib/per-unit flag request.

**Result.** 1 unit(s) measured the same lever independently, so the evidence is grouped here rather than written once per outbox:

* `auto/800CB948_fn_800CB948` - -pool off - string literals: .rela.text 0x2C4 -> 0x3B4 (retail's per-string lis/addi), .data 0x00AD == retail 0x80594D20; without it a @stringBase0 base register and .text 0xC44

**Example.**

```
-pool off
```