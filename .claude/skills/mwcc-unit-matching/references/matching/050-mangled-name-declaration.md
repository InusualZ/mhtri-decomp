---
id: 50
title: An already-mangled map name must not be declared as a C++ identifier
status: works
problem: The map carries a *real* C++ mangling - `Panic__Q24nw4r2dbFPCciPCce`, not a `fn_XXXXXXXX` placeholder - and a C++ source declares it with that spelling as an identifier:
tags: [symbols]
applies: []
demo:
---

# 50. An already-mangled map name must not be declared as a C++ identifier

**Problem.** The map carries a *real* C++ mangling - `Panic__Q24nw4r2dbFPCciPCce`, not a `fn_XXXXXXXX`
placeholder - and a C++ source declares it with that spelling as an identifier:

```cpp
extern void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);
```

The front-end then mangles it **again**, appending its own argument list, and the object references
`Panic__Q24nw4r2dbFPCciPCce__FPCciPCce` - which nothing defines. The link fails with an undefined symbol
whose name looks like the right one with a suffix stuck on. It is **invisible while the unit is
`NonMatching`** - a `NonMatching` object is never linked, so the bug sits latent until the unit is flipped,
and then it is easy to mis-attribute (this is what hid the real `.ctors` cause of `800CCCF8`, see 46).

**Why try it.** The map's name *is* the correct mangling of the real declaration, so the fix is **not** a
rename and **not** `extern "C"`: write the real declaration and let the front-end reproduce the map's
spelling. `tools/units/mangle.py` (48) confirms it before you touch anything:

```sh
python tools/units/mangle.py 'namespace nw4r { namespace db { void Panic(const char*, int, const char*, ...) { } } }'
# Panic__Q24nw4r2dbFPCciPCce          <- byte-identical to symbols.txt; no rename needed
```

This is the complement of 48, and between them they settle the whole `extern "C"` question: **a
`fn_XXXXXXXX` stem is a placeholder, so write C++ and rename the map to the mangling (48); a map name that is
already mangled is the *real* name, so write the real declaration and it matches (50).** `extern "C"` is then
only for a symbol whose real name you genuinely cannot express.

**Result.** Found by flipping the parked `800CCCF8`. Five units carried it - `ef/ef_cube`, `ef/ef_cylinder`,
`ef/ef_emform`, `ef/ef_line`, `ef/ef_point` - and the reason they had it is instructive: the promotion pass
correctly flipped them from `.c` to `.cpp` (each names a `.cpp` `__FILE__`), and under **C** the declaration
`extern void Panic__Q24nw4r2dbFPCciPCce(...)` is verbatim, so it was right before the flip and wrong after.
Fixing the declaration made all five emit the map's exact name; the measurement is unchanged (the fix is
codegen-neutral), and it unblocked `ef/ef_emform`'s flip, whose link now succeeds.

**Example.** The check is a one-liner over our objects - a reference whose name ends in a *second* argument
list is this bug:

```sh
# read each object's symbol table and flag `X__...__...` where `X__...` is already a map name
```
