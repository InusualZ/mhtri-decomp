---
id: 50
title: An already-mangled map name must not be declared as a C++ identifier
status: works
problem: The map carries a real C++ mangling (`Panic__Q24nw4r2dbFPCciPCce`) and a C++ source declares that spelling as an identifier; the front-end mangles it AGAIN (`...__FPCciPCce`) and the link fails with an undefined name that looks right with a suffix.
tags: [symbols]
applies: []
demo: 050-mangled-name-declaration.cpp
reviewed: 2026-09-29
related: [42, 48, 51, 46]
---

# 50. An already-mangled map name must not be declared as a C++ identifier

**Problem.** The map carries a *real* C++ mangling - `Panic__Q24nw4r2dbFPCciPCce`, not a `fn_XXXXXXXX`
placeholder - and a C++ source declares it with that spelling as an identifier:

```cpp
extern void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);
```

The front-end then mangles it **again**, appending its own argument list, and the object references
`Panic__Q24nw4r2dbFPCciPCce__FPCciPCce` - which nothing defines. The link fails with an undefined symbol whose
name looks like the right one with a suffix stuck on. It is **invisible while the unit is `NonMatching`** - a
`NonMatching` object is never linked, so the bug sits latent until the unit is flipped, and then it is easy to
mis-attribute (this is what hid the real `.ctors` cause of `800CCCF8`, see 46).

**How it looks.** Every function of the unit matches, `flipcheck.py` says READY, and the flipped link stops with
`undefined: 'X__...__F...'` where `X__...` is already a name in `symbols.txt`. In our object's relocations
(`objdump -r`) the call target is the map name plus a second `__F<args>` suffix.

**Why it happens.** In C++ *every* function name is mangled from its declaration. A name that already looks
like a mangling is just a strange identifier to the front-end. (Under **C** the same declaration is verbatim,
which is why it is right before a `.c` -> `.cpp` promotion and wrong after it.)

**How to work it.** The map's name *is* the correct mangling of the real declaration, so the fix is **not** a
rename and **not** `extern "C"`: write the real declaration (here `namespace nw4r { namespace db { void
Panic(const char*, int, const char*, ...); } }`) and call it as `nw4r::db::Panic(...)`, and let the front-end
reproduce the map's spelling. `tools/units/mangle.py` (idea 48) confirms it before you touch anything:

```sh
python tools/units/mangle.py 'namespace nw4r { namespace db { void Panic(const char*, int, const char*, ...) { } } }'
# Panic__Q24nw4r2dbFPCciPCce          <- byte-identical to symbols.txt; no rename needed
```

This is the complement of 48, and between them they settle the `extern "C"` question: **a `fn_XXXXXXXX` stem is a
placeholder, so write C++ and rename the map to the mangling (48); a map name that is already mangled is the
*real* name, so write the real declaration and it matches (50).** `extern "C"` is then only for a symbol whose
real name you cannot express (idea 42). Style rule 9 (`docs/plan.md` 6.5, checked by `stylelint.py`) enforces the
call-through-the-owner form; idea 51 describes it.

**Result.** Found by flipping the parked `800CCCF8`. Five units carried it - `ef/ef_cube`, `ef/ef_cylinder`,
`ef/ef_emform`, `ef/ef_line`, `ef/ef_point` (as measured then) - and the reason they had it is instructive: the
promotion pass correctly flipped them from `.c` to `.cpp` (each names a `.cpp` `__FILE__`), and under C the
declaration is verbatim, so it was right before the flip and wrong after. Fixing the declaration made all five emit
the map's exact name; the measurement is unchanged (the fix is codegen-neutral), and it unblocked `ef/ef_emform`'s
flip, whose link now succeeds.

**When NOT to apply.** A `.c` unit needs no change - the C front-end does not mangle. A name with no `__F`/`__Q`
suffix (`fn_800CCCF8`, `Panic`) is not a mangling; use idea 48 for those.

**Example.** The check is a one-liner over our objects - a call/reloc target that ends in a *second* argument
list is this bug:

```sh
build/binutils/powerpc-eabi-objdump.exe -r build/RMHE08/src/ef/ef_emform.o | grep -E "R_PPC_REL24[[:space:]]+[A-Za-z0-9_]+__Q[A-Za-z0-9_]*__F[A-Za-z0-9_]+$"
```

(The pattern is for the qualified `__Q` shape of this idea's example; a doubled class-member or plain `__F` name
needs its own pattern. Only run against objects that compiled, and treat a hit as a candidate to compare with
`symbols.txt`.)

**Demonstration.** `050-mangled-name-declaration.cpp` (`ideas.py demo-check 50`) compiles both spellings: the
identifier form produces a relocation to `Panic__Q24nw4r2dbFPCciPCce__FPCciPCce`; the namespaced form
(`nw4r::db::Panic`) produces `Panic__Q24nw4r2dbFPCciPCce`, the map's own name. (Both compile - only the link would
fail, exactly as the idea says; the checker's reloc match accepts the map name as a prefix, so the doubled name is
the assertion that fails if the compiler stops re-mangling.)
