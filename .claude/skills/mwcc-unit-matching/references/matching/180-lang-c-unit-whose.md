---
id: 180
title: A -lang=c unit whose callees are C++ manglings compiles as C++ under #pragma cplusplus on
status: works
problem: A unit registered as a `.c` file (compiled `-lang=c`) calls C++ members and mangled free functions, so new bodies can only spell `foo__FPQ34nw4r4math4VEC3` identifiers and local externs, which the gate refuses as added rule 2/9 findings.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
---

# 180. A -lang=c unit whose callees are C++ manglings compiles as C++ under #pragma cplusplus on

**Problem.** A `.c` unit whose target is really C++ (extab/extabindex present, every callee a mangling such as
`setMatColor__6MHcharFUl12_GXChannelID8_GXColorb`) can only reach those callees by spelling the mangled names as C
identifiers in local `extern`s - each new one is an added rule 2 and rule 9 finding, so new bodies cannot land.

**Why it happens.** The registration fixes the file extension and `configure.py` passes `-lang=c`; MWCC still honours
`#pragma cplusplus on`, which switches the rest of the translation unit to C++ and defines `__cplusplus` from that
line on (checked: a function after the pragma mangles to `f__Fv`, an `#ifdef __cplusplus` block is taken).

**How to work it.** Put `#pragma cplusplus on` before the first `#include`, include the owners' C++ headers (`pl.h`
for `MHchar` members, `nw4r/math.h`, `fn_8004CAD8.h`, ...) and call members as members; wrap the unit's own
definitions in `extern "C" { ... }` so their map names stay plain. Fix the C-only spellings the switch exposes
(`void*` to `T*` conversions need a cast). Re-measure every existing row: none should move. A later registration move
to `.cpp` (`land.py --unit-rename`) makes the pragma unnecessary.

**Result.** `ef/fn_8010D1A8` (2026-10-26): the conversion moved no existing row (16 rows at 100 % stayed, two rose:
90.35 -> 94.71, 91.79 -> 92.56), dropped the file's lint findings 259 -> 7 once the bodies and names followed, and
let the eight unwritten rows (16 132 B) be written through owner headers (unit 19.95 -> 96.51 %).

**Example.**

```
#pragma cplusplus on
#include "pl.h"            /* MHchar::setMatColor */
extern "C" {
void light_tint(MHchar* m, _GXColor c) { m->setMatColor(9, GX_COLOR0A0, c, false); }
}
```
