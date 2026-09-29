---
id: 28
title: A kept `bl` to a tiny static names the unit's inlining setting
status: works
problem: the target calls a small file-local function the source could just as well inline (`bl fn_8003F554`), while our build inlines it away - the callee has no counterpart in our object and the caller comes out an instruction short, shifting every register after it. It reads as a missing helper, so the first instinct is to hunt for a source shape that suppresses the inline.
tags: [source-shape, allocator, symbols]
applies: []
demo:
---

# 28. A kept `bl` to a tiny static names the unit's inlining setting

Problem: the target calls a small file-local function the source could just as well inline
(`bl fn_8003F554`), while our build inlines it away - the callee has no counterpart in our object and the
caller comes out an instruction short, shifting every register after it. It reads as a missing helper, so the
first instinct is to hunt for a source shape that suppresses the inline.

Why try it: `-inline auto` in `cflags_base` inlines eagerly, and not every retail unit was built that way
(`-inline off`, or a threshold we cannot see). The inline setting is a per-library flag like the `-O` level,
and a `#pragma` can scope it to the one function that needs it.

Result: for the `main` lib (batch 2), `-O3` **plus `-inline noauto`** took `fn_8003F52C` and `fn_8003F564`
from 74 % to 100 % (retail keeps their `bl fn_8003F554`), `change_widemode_req_default__Fv` from 21.18 % to
100 %, and `main` itself from 71.12 % to 96.73 %. Pick the *middle* setting, not `-inline off`: `fn_8003F940`
is retail's inlined aggregate `GXRenderModeObj` copy, and `off` turns it into a call to the implicit
copy-assignment operator (99.02 %, and 4 bytes short) where `noauto` still inlines it (100 %). Two spelling
traps came out of the same batch: `#pragma peephole on/off` is honoured (it is what keeps retail's unfused
`srwi`+`clrlwi` in `change_widemode_req_default__Fv`), while `opt_peephole`/`peep` parse and do nothing, and
whole-unit `-opt nopeephole` cost `main` 2.2 points - scope the pragma to the function that needs it, and
never assume a pragma name works because it parses.

Example: the inline half belongs in the library's flags

```python
cflags_main = [
    *[f for f in cflags_lobby if f != "-inline auto"],
    "-inline noauto",
]
```

and the peephole half from the same batch is a pragma, scoped to the function that needs it because
`-opt nopeephole` for the whole unit cost `main` 2.2 points:

```c
#pragma peephole off
void change_widemode_req(unsigned char mode);
```
