---
id: 25
title: Use the shared memory dump as a name/signature/struct oracle
status: works
problem: Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has thousands of `fn_XXXX` names and no types at all.
tags: [symbols, process]
applies: []
demo:
---

# 25. Use the shared memory dump as a name/signature/struct oracle

**Problem.** Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has
thousands of `fn_XXXX` names and no types at all.

**Why try it.** A second Ghidra project on this machine holds a runtime memory dump of the game with real
SDK symbol names, function signatures, annotated struct layouts and the contents of data blobs this repo
does not own. It answers in one query what otherwise costs a disassembly read - and it answers questions
no static object can.

**Result.** For `RSO/runtime` it named eight of the unit's nine functions (`LocateObject`,
`RSOStaticLocateObject`, `RSOUnLocateObject`, `RSOLink`, `RSOUnLink`, `FindExportIndex`, `RSORelocate`,
`RSORelocateSmallDataSection` - the ninth, `fn_804DA7E4`, is a `zz_` placeholder there too), and it
separately named the four 4-byte `RSONotify*` thunks that sit just *before* the unit's range. It gave
their signatures (`RSOLink(RSOModule*, RSOModule*, ...)` - the second argument is the *exporting* module,
not a private "relocation table"), and its `RSOModule` layout confirmed every offset this project had
derived by hand, with real field names. It is **not** codegen evidence (idea 17), its annotations mix SDK
names with Ghidra placeholders, and a `splits.txt` range taken from it still has to be measured (idea 23).
Recipes and the full worked example: `docs/memory-dump.md`.

The same dump has a **companion symbol map** (`DumpSymbols.zip` -> `Dump_Loading85.raw.map`, 48 367 lines
of `name [args] address flags`), which needs no Ghidra session and answers two questions the project call
cannot answer in bulk: a real name (`fn_800406AC` -> `CntSdRsoTerminate`), and **whether an address is a
function at all**. The second one is worth a playbook row of its own because it is a *map* bug, not a
source bug: `src/auto/80040598_fn_80040598.cpp` carried five 4-byte `fn_80040794`-style symbols that were
not functions but the dead epilogue MWCC emits after a `mtctr`/`bctr` tail-call dispatcher. The dispatchers
compiled exactly 4 bytes longer than the map said (their bodies are byte-identical to *their own symbol
plus the artifact*), so objdiff could never pair them: growing the five owner sizes by 4 and deleting the
five artifacts closed four dispatchers outright (88.9 % -> 100 %) and removed 20 bytes of phantom code
from the map. The tell is in the dump: it carries `zz_<address>_` placeholders for genuinely unnamed
functions (including four inside that very range) and **no** line at those five addresses.
