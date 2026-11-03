---
id: 298
title: A deferred helper whose assert strings stay in the main pool is a static inline; an extern inline's literals go to the .data tail
status: works
problem: a group of helpers lands after their first caller in retail's `.text` (the deferred-inline order of idea 163), but the `.data` strings of some of them sit in the main first-use sequence while the header asserts' strings trail the vtable; making every helper `extern "C" inline never_inline` puts all their strings in the tail, each with its own copy of the `__FILE__` string
tags: [data, source-shape, sections]
applies: [Wii/1.3]
demo: 
related: [163, 193, 242]
---

# 298. A deferred helper whose assert strings stay in the main pool is a static inline; an extern inline's literals go to the .data tail

**Problem.** Retail's `.text` has seventeen helpers right after `Calc`, their first caller, in the reverse order of
their first use (idea 163's deferred-inline order), so they are `inline` functions, not definitions in source order.
But their strings disagree: the header asserts' strings (`"...add(=%p)..."`, `"particle.h"`, `"...num <
NumPtclTrack()"`, `"res_emitter.h"`) trail the class table at the end of `.data`, while `ef_pm_calc_emitter_pos`'s
three asserts sit in the main first-use sequence right after `Calc`'s jump table and share the one
`"ef_particlemanager.cpp"` copy. Spelling all seventeen `extern "C" inline __attribute__((never_inline))` puts every
string in the tail, with a fresh `__FILE__` copy per assert (`.data` 0x64A against 0x608).

**Why it happens.** MWCC keeps the literals of a weak (extern inline) function with the function, in a tail pool
emitted after the TU's own data and ordered in reverse definition order, so the linker can fold them with the
weak copy; they are not reused against the main pool. A `static inline` function is deferred the same way (its
code follows its first caller in the LIFO order) but has internal linkage, so its literals go to the ordinary pool
at the point it is compiled - right after the caller's own strings - and reuse the main `__FILE__` copy.

**How to work it.** Declare the header helpers `extern "C" inline __attribute__((never_inline))` ahead of the first
definition, in their headers' order (the strings' tail reads them reversed), and the file's own deferred helper
`static inline __attribute__((never_inline))` - inside an `extern "C" { }` block when the map row carries its C
name, so the local symbol keeps the unmangled spelling. Leave the definitions plain `extern "C"` (the lint reads an
inline definition's head as a declaration). Check with `objdump -t` that the helpers are `w` and the static `l`,
and compare `.data` byte for byte.

**Result.** `ef/ef_particlemanager`: `.data` 0x602 bytes identical to the target's (its 0x608 is end alignment),
`.text` in retail's order with the seventeen helpers at their retail offsets; flipcheck's `.data` and
`.text`-size lines cleared.

**Example.**

```
extern "C" inline __attribute__((never_inline)) u8* ef_emres_get_ptcl_track_at(void* self, u32 index);  /* res_emitter.h */
extern "C" inline __attribute__((never_inline)) nw4r::math::VEC3* fn_800AD0CC(EfPmParticle* self, nw4r::math::VEC3* add);  /* particle.h */
extern "C" {
static inline __attribute__((never_inline)) void ef_pm_calc_emitter_pos(nw4r::math::VEC3* result, BOOL emitterSpace, ...);
}
/* ... definitions as plain extern "C" / static, before Calc; .data tail = add, particle.h, num <, res_emitter.h */
```
