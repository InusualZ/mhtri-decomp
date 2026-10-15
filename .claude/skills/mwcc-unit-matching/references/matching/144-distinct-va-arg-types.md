---
id: 144
title: Distinct pointee types per va_arg branch stop MWCC merging identical arms
status: works
problem: A formatter's if-chain of `va_arg` fetches is shorter in ours than in retail because several arms that look identical at the machine level are merged into one block.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [34]
---

# 144. Distinct pointee types per va_arg branch stop MWCC merging identical arms

**Problem.** In `printf`'s argument fetch (`size_t`, `ptrdiff_t`, `long`, `int`, `long double`, `double` ...) many
arms compile to the same instructions on a 32-bit target. Written with the same types, ours merges them into one
block and the function is shorter, with every later branch target shifted.

**Why it happens.** MWCC merges blocks whose IR is identical, and the IR carries the types: two
`va_arg(ap, u32)` arms are one block, `va_arg(ap, u32)`, `va_arg(ap, long)` and `va_arg(ap, int)` are three. The
original used each type the format named, so retail keeps one block per arm.

**How to work it.** Give each `va_arg` arm (and each store through the argument pointer) the C type its format
case names - `unsigned long`, `long`, `int`, `long double`, `f64` - even where they are the same size; do not
canonicalise them.

**Result.** `MSL_C/printf` (Wii/1.3): `__pformatter` 87.48 -> 95.4 from the distinct types (96.63 with the other
levers); the same in `MSL_C/wprintf` `__wpformatter` (87.25 -> 95.72 overall).

**Example.**

```c
if (format.argument_options == ARG_LONG) {
    long_num = *(long*)__va_arg(args, 1);
} else if (format.argument_options == ARG_SIZE_T) {
    long_num = *(u32*)__va_arg(args, 1);             /* not (long*) like its neighbours */
} else if (format.argument_options == ARG_PTRDIFF_T) {
    long_num = *(long*)__va_arg(args, 1);
} else {
    long_num = *(int*)__va_arg(args, 1);
}
```
