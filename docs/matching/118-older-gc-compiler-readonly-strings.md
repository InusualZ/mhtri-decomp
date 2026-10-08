---
id: 118
title: Unpooled readonly strings (.sdata2 and 4-aligned .rodata) are an older GC compiler, not a flag
status: works
problem: the target keeps every string literal as its own object (small ones in .sdata2, the rest 4-aligned in .rodata) but Wii/1.3 always pools them into one @stringBase0, whatever -str says
tags: [flags, data]
applies: []
demo: 
---

# 118. Unpooled readonly strings (.sdata2 and 4-aligned .rodata) are an older GC compiler, not a flag

**Problem.** `MSL_C/locale` (data only) shows `.rodata` ours 1291 B against 1288 B with a single `@stringBase0`, and
`.sdata2` 0 B against 24 B. The target has one object per literal: the 1..6 byte strings (`.`, empty, `AM|PM`, `%T`) in
`.sdata2`, the longer ones in `.rodata`, each 4-aligned (the `gap_*` rows between them are the alignment fill).

**Why it happens.** In Wii/1.0..1.5 and GC/3.0a5 `-str readonly` forces `pool` (`-str reuse,nopool,readonly` and
`-str reuse,readonly` still produce one `@stringBase0`), while `noreadonly` puts the strings in `.sdata`/`.data`.
GC/1.3.2 .. GC/2.7 keep readonly literals unpooled: `.sdata2` up to the small-data threshold, `.rodata` above it.

**How to work it.** Probe with a scratch file holding the literals and the real command line, changing only the compiler
directory (`compilers/GC/2.7`): the symbol list tells which compiler lays the strings out like the target. Then register
the unit with `Object(..., mw_version="GC/2.7", extra_cflags=["-str reuse,nopool,readonly"])` (the library flags add
`pool` first, so the later `-str` must say `nopool`).

**Result.** `MSL_C/locale`: data 0 -> 3 sections matched, flipcheck READY, DOL hash unchanged after the flip.

**Example.**

```
const char* names[] = { ".", "", "AM|PM", "%a %b %e %T %Y" };   /* GC/2.7: @1..@3 in .sdata2, @4 in .rodata */
```
