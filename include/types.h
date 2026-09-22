/*
 * The project's common scalar types: one definition, included by every unit that needs them.
 *
 * The compiler runs with `-nodefaults`, so nothing is defined for us; these are the Metrowerks / Dolphin SDK
 * spellings, and the units that used to carry their own copies of them (`main.cpp`, `__start.c`,
 * `RSO/runtime.c`, which also had `typedef int BOOL`) now include this file instead. The build tracks headers
 * (`-MMD`, `deps = gcc`), so an edit here rebuilds every unit that includes it.
 *
 * Rules that keep this file useful rather than a dumping ground:
 *   - **a declaration is added here the second time a unit needs it**, never the first: a type only one unit
 *     uses belongs to that unit (or beside it, like `src/Camellia/camellia.h`);
 *   - `src/Camellia/camellia.c` is the exception and keeps its own typedefs: it is a vendor file mirroring
 *     upstream, its `u32` is `unsigned int` rather than `unsigned long`, and its 100 % match rests on those
 *     spellings (AGENTS.md -> Conventions, "Vendor files keep vendor naming");
 *   - the SDK's own headers, when a unit needs more than the primitives (`GXRenderModeObj`, `Vec`, `Mtx`,
 *     `OSHeapHandle`, ...), get a `dolphin/` mirror under `include/` rather than being re-declared per unit -
 *     that is the same rule, one level up.
 */
#ifndef MHTRI_TYPES_H
#define MHTRI_TYPES_H

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32; /* PPC32: `long` is 32 bit, which is what the SDK assumes */
typedef signed long s32;
typedef unsigned long long u64;
typedef signed long long s64;

typedef float f32;
typedef double f64; /* the SDK's `f64`; note Metrowerks' `double` is 8 bytes on this target */

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

#endif /* MHTRI_TYPES_H */
