---
id: 28
title: A kept `bl` to a tiny static names the unit's inlining setting
status: works
problem: The target calls a small file-local function the source could just as well inline (`bl fn_8003F554`), while our build inlines it away, leaving the caller an instruction short and shifting every register after it.
tags: [flags, source-shape]
applies: [Wii/1.3]
demo: 028-kept-bl-inline-setting.cpp
reviewed: 2026-09-29
related: [5, 16, 33, 39, 61]
---

# 28. A kept `bl` to a tiny static names the unit's inlining setting

**Problem.** The target calls a small file-local function the source could just as well inline
(`bl fn_8003F554`), while our build inlines it away - the callee has no counterpart in our object and the caller
comes out an instruction short, shifting every register after it. It reads as a missing helper, so the first
instinct is to hunt for a source shape that suppresses the inline.

**How it looks.** The target has a `bl <small function>` (with its own row in the map, a few instructions long) and
ours has the callee's body pasted in the caller; the caller's remaining rows are all shifted by the difference.

**Why it works.** `-inline auto` in `cflags_base` inlines eagerly, and not every retail unit was built that way.
The inline setting is a per-library flag like the `-O` level (`-inline off`, `noauto`, or a threshold we cannot
see), and a `#pragma` can scope it to the one function that needs it (idea 61).

**How to work it.** Try `-inline noauto` first - it keeps the un-marked static helper as a call but still inlines
what the source marked `inline`. Pick the *middle* setting, not `-inline off`. Measure the whole unit under each
setting; then decide between a library flag (idea 33) and a scoped pragma (idea 61).

**Demonstration.** `028-kept-bl-inline-setting.cpp` (`ideas.py demo-check 28`), under `-inline noauto`: the
un-marked `static int tiny_helper(int)` is called twice with a `bl` (0x48 B for the caller; under `-inline auto`
the same source is 0x14 B, both calls folded), while a function marked `inline` is still inlined.

**Example.** The inline half belongs in the library's flags:

```python
cflags_main = [
    *[f for f in cflags_lobby if f != "-inline auto"],
    "-inline noauto",
]
```

and a peephole half found in the same batch is a pragma, scoped to the function that needs it because
`-opt nopeephole` for the whole unit cost `main` 2.2 points (idea 16):

```c
#pragma peephole off
void change_widemode_req(unsigned char mode);
```

**Two spelling traps.** `#pragma peephole on/off` is honoured (it keeps retail's unfused `srwi`+`clrlwi`);
`opt_peephole` and `peep` parse and do nothing. Never assume a pragma name works because it parses.

**When NOT to apply.** If retail *inlines* a helper that ours calls, the setting is the other way (`auto`, or a
missing `inline` in the source). A `bl` to something with a real body of its own (not a tiny static) is more often
a genuine function than an inlining setting.

**Evidence** (`main` lib, batch 2, dated 2026-09-2x). `-O3` **plus `-inline noauto`** took `fn_8003F52C` and
`fn_8003F564` from 74 % to 100 % (retail keeps their `bl fn_8003F554`), `change_widemode_req_default__Fv` from
21.18 % to 100 %, and `main` itself from 71.12 % to 96.73 %. `-inline off` is worse: `fn_8003F940` is retail's
inlined aggregate `GXRenderModeObj` copy, and `off` turns it into a call to the implicit copy-assignment operator
(99.02 %, 4 bytes short) where `noauto` still inlines it (100 %).
