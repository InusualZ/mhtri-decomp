---
id: 25
title: Use the shared memory dump as a name/signature/struct oracle
status: works
problem: Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has thousands of `fn_XXXX` names and no types at all.
tags: [symbols, process]
applies: []
demo:
reviewed: 2026-09-29
related: [17, 23, 67, 76]
---

# 25. Use the shared memory dump as a name/signature/struct oracle

**Problem.** Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has
thousands of `fn_XXXX` names and no types at all.

**How it looks.** A region is a hundred `fn_XXXXXXXX` with no strings or callers that name them, or a struct's
field offsets are known only from the disassembly, or a function's arguments are a guess.

**Why it works.** A second Ghidra project on this machine (`MH3Shared`, reachable through the `ghidra` MCP
server) holds a runtime memory dump of the game with real SDK symbol names, function signatures, annotated struct
layouts and the contents of data blobs this repo does not own. It answers in one query what otherwise costs a
disassembly read - and it answers questions no static object can. The recipes, and what it must not be used for,
are in `docs/memory-dump.md`; it is **read-only** to us (no renames, no types, no comments, no saves - our names go
into `symbols.txt` through `symedit.py`).

**How to work it.**

1. Query the dump by address for the name, signature and struct layout (`docs/memory-dump.md`, "Recipe").
2. For a bulk answer without a Ghidra session use the **companion symbol map** (`DumpSymbols.zip` ->
   `Dump_Loading85.raw.map`, 48 367 lines of `name [args] address flags`): a real name (`fn_800406AC` ->
   `CntSdRsoTerminate`), and **whether an address is a function at all**.
3. Confirm with the code: the dump is **not** codegen evidence (idea 17), its annotations mix SDK names with Ghidra
   placeholders (`zz_<address>_` marks a genuinely unnamed function), and a `splits.txt` range taken from it still
   has to be measured (idea 23).

**Result** (measured at the time). For `RSO/runtime` it named eight of the unit's nine functions
(`LocateObject`, `RSOStaticLocateObject`, `RSOUnLocateObject`, `RSOLink`, `RSOUnLink`, `FindExportIndex`,
`RSORelocate`, `RSORelocateSmallDataSection`; the ninth, `fn_804DA7E4`, is a `zz_` placeholder there too), and
separately named the four 4-byte `RSONotify*` thunks just *before* the unit's range. It gave their signatures
(`RSOLink(RSOModule*, RSOModule*, ...)` - the second argument is the *exporting* module, not a private "relocation
table"), and its `RSOModule` layout confirmed every offset this project had derived by hand, with real field names.

**The map answers "is this a function": a map bug, not a source bug.** The 48 367-line map carries `zz_<address>_`
placeholders for genuinely unnamed functions and **no** line at an address that is not a function. That exposed five
4-byte `fn_80040794`-style symbols in what is now `src/fn_80040598.cpp`: not functions, but the dead epilogue MWCC
emits after a `mtctr`/`bctr` tail-call dispatcher. The dispatchers compiled exactly 4 bytes longer than the map
said (their bodies were byte-identical to *their own symbol plus the artifact*), so objdiff could never pair them:
growing the five owner sizes by 4 and deleting the five artifacts closed four dispatchers outright (88.9 % -> 100 %)
and removed 20 bytes of phantom code from the map.

**When NOT to apply.** Never as codegen evidence, never to overrule the retail bytes, and never as an owner or
boundary signal on its own (idea 67 and idea 76 give the in-binary signals). The dump is a *loading-state* dump:
check `docs/memory-dump.md` before trusting a value that the game only fills in later.

**Evidence.** `RSO/runtime` and the `fn_80040598` dispatchers, dated 2026-09-2x. `docs/memory-dump.md` is the
maintained reference; the `src/auto/...` path this idea used to cite no longer exists (the unit is at its final home).
