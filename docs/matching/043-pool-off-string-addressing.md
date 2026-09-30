---
id: 43
title: Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off`
status: works
problem: Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base symbol retail never had.
tags: [flags, data]
applies: []
demo: 043-pool-off-string-addressing.cpp
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

**Demonstration.** `043-pool-off-string-addressing.cpp` (`ideas.py demo-check 43`). In isolation `-pool off` was
**not** the lever for string literals: with `-str reuse,pool` the strings stay behind one `@stringBase0` whether
`-pool off` comes before or after it, with or without `readonly`, and `static const` arrays are addressed per symbol
(one `lis`/`addi` each) with or without the flag. The per-string `lis`/`addi` shape reproduces when `-str` carries no
`pool` (the base `-str reuse`), which is what the demo asserts; whether `-pool off` matters for a unit is decided by
that unit's `-str` flags, not by this section's claim alone.
