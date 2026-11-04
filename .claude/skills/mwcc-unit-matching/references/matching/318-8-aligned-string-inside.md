---
id: 318
title: An 8-aligned string inside a TU's inline tail: the tail's strings are globals defined after the last function
status: works
problem: Every `.text` row is at 100 and `.data` differs only by a 4-byte zero hole before one string in the middle of the `_ac.h` inline tail; a function-local `static ... aligned(8)` fixes the hole but moves the string ahead of every main-pool literal
tags: [data, sections, source-shape]
applies: [Wii/1.3]
demo: 
related: [80, 298, 304]
---

# 318. An 8-aligned string inside a TU's inline tail: the tail's strings are globals defined after the last function

**Problem.** `g3d/g3d_resanmscn`: retail's `.data` is `ResName records | getters' literals | the seven _ac.h
accessors' strings`, and the second-to-last string ("%s::%s: Object not valid.", 0x1A bytes) sits 8-aligned at
+0x290 after 4 zero bytes. The sibling fix (`g3d_resanmtexsrt`: `static char msg[] __attribute__((aligned(8)))`
inside the weak resolver) gives the hole, but here the static lands at +0xA0, before the getters' literals.

**Why it happens.** Measured on the unit's command line (Wii/1.3 `-O3 -inline noauto -str reuse`, scratch TUs):
`.data` is emitted in event order - a static object (file scope or function-local, `@LOCAL@<fn>@name`, weak or not)
when it is **parsed**, a normal function's literals when that function is generated (right after its definition),
and every inline function's literals (`@STRING@<fn>`) at the end of the TU in reverse creation order. So no static
can sit *after* an inline-tail literal, and a literal cannot be over-aligned (a weak literal is 8-aligned only when
its size is a multiple of 8, e.g. a 0x18-byte file name). texsrt worked only because its one inline was defined last
and had the only tail strings. A template's function-local static is created at its instantiation (the first use),
not at the end.

**How to work it.** Turn the whole tail into named globals: `extern char x[];` before the inline accessors, the
accessors pass the names, and the definitions (`char x[] = "...";`, the over-aligned one with
`__attribute__((aligned(8)))`) go after the last function, in retail's `.data` order. The code is unchanged
(`lis`/`addi` either way). Rename the map rows to the globals in the same change. Keep `.sdata` literals (here "ref")
as literals.

**Result.** `g3d/g3d_resanmscn`: `.data` 55 differing bytes -> byte-identical (0x2C2 of the 0x2C8 claim; the tail is
fill before the next unit's 8-aligned `.data`); trial `Object(Matching)` keeps `main.dol: OK`.

**Example.**

```
extern char tex_pat_fmt[];
__declspec(noinline) inline u32 ref(void *self) { if (!ok(self)) Panic(tex_pat_file, 92, tex_pat_fmt, name(), "ref"); ... }
/* ... every function ... */
char tex_pat_fmt[] __attribute__((aligned(8))) = "%s::%s: Object not valid.";
char tex_pat_file[] = "g3d_resanmtexpat_ac.h";
```
