---
id: 158
title: A zero initializer lays bss out in definition order instead of first-use order
status: works
problem: In a C unit the .bss/.sbss objects come out in first-use order, not in retail's order, and no declaration order moves them
tags: [data, source-shape, sections]
applies: [DWCi]
demo: 158-zero-initializer-lays-bss.cpp
---

# 158. A zero initializer lays bss out in definition order instead of first-use order

**Problem.** A C unit's `.bss` (or `.sbss`) objects come out in the order its code first references them, while
retail has them in another order: `datagap.py`/`flipcheck.py` report "symbol layout differs", the rows that address
the objects from one section base read a few points low, and permuting the definitions changes nothing.

**Why it happens.** In C an uninitialised file-scope object is a *tentative definition*; MWCC emits it into `.bss`
when the code first needs it (in the order the functions reference them), after every object that has an
initializer. An explicit zero initializer (`= {0}`, `= 0`, `= NULL`) makes it a real definition, which MWCC emits
in definition order - still into `.bss`/`.sbss`, the bytes are zero either way. C++ has no tentative definitions,
so a `.cpp` unit lays its objects out in definition order already (idea 113 is a C++ case this does not cover).

**How to work it.** Read the target's layout (`objdump -t` of `build/RMHE08/obj/<unit>.o`, or the flipcheck line),
define the objects in that order, and give each the zero initializer. Re-run `flipcheck.py <unit>`: the layout line
disappears, and the rows that address the objects from the section base usually move up.

**Result.** `DWCi/gsAvailable` (game name, host override, probe state): first-use order put the probe state between
the two names; with `= {0}` on the two names the `.bss` is byte-identical and `DWCi_natProbeStart` 90.40 -> 90.51.
`DWCi/gsPlatform`: the zero-initialised memory hooks come first in `.bss` and the tentative last-error word last in
`.sbss`, behind the function statics (resolver cache, local host record), byte-identical with retail.

**Example.**

```
char DWCi_availableGameName[0x40] = {0};   /* emitted here, in this order */
char DWCi_availableHostName[0x40] = {0};
DWCiAvailableCheck DWCi_availableCheck;    /* tentative: emitted when first used */
```

**Demonstration.** `158-zero-initializer-lays-bss.cpp` is compiled with `-lang c`: the uninitialised trio lands as
game name, probe state, host name (first use), the zero-initialised names land in definition order ahead of
everything else.
