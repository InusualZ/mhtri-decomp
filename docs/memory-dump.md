# The runtime memory dump (Ghidra) and what it is good for

This game is also being reverse-engineered in a separate Ghidra project on this machine. It contains a
**runtime memory dump of the game** - not the static DOL - and it is a useful oracle for this
decompilation **for names, signatures and data**, never for codegen.

## Where it is

* Ghidra project `MH3Shared` (`D:\WiiExperiment\Decompilation\MH3Shared`), program
  `/DolphinDump85.raw.keep` - a raw binary, language `PowerPC:BE:32:Gekko_Broadway`.
* Dumped from `D:/Monster Hunter Tri/MemoryDump/Dump_Loading85.raw`, i.e. a *loading-state* dump: it
  holds the DOL image and whatever the game had resident at that moment.
* 21 901 functions, 179 350 symbols, 814 data types, 14 memory blocks. The DOL's sections are
  configured as **overlay address spaces** (`.init`, `extab`, `extabindex`, `.text`, `.ctors`,
  `.dtors`, `.rodata`, `.data`, `.bss`, `.sdata`, `.sbss`, `.sdata2`, `.sbss2`), so an address can be
  qualified with the section it lives in.
* Reachable through the `ghidra` MCP server (instance `MH3Shared`).

## What it answers

| question | call | worked example (the `RSO/runtime` unit) |
| --- | --- | --- |
| what is this `fn_*` really called? | `search_functions`, `get_function_by_address` | `fn_804D9B44`/`fn_804D9B48` are `RSONotifyPreRSOLink`/`RSONotifyPostRSOLink`, `fn_804D9B4C` is `LocateObject`, `fn_804DA6C8` is `RSOUnLink`, `fn_804DA834` is `FindExportIndex`, `fn_804DAA24` is `RSORelocate`, `fn_804DABF0` is `RSORelocateSmallDataSection` |
| what are the parameter types? | `get_function_signature` | `RSOLink(RSOModule*, RSOModule*, undefined)`, `RSORelocate(RSORelocation*, int index, void* addr)`, `FindExportIndex(RSOModule*, char* symbol)` |
| what is the struct layout? | `get_struct_layout` | `RSOModule` (88 B) confirmed *every* offset this project had derived by hand and supplied the real field names (`export_symbol_table_offset` +0x40, `export_symbol_names_offset` +0x48, `import_symbol_table_size` +0x50, `import_symbol_names_offset` +0x54, `unresolved_function_offset` +0x2C, ...) - see `src/RSO/runtime.c` |
| what is inside a data blob the unit does not own? | `read_memory` | `.data:0x80629B90` is that unit's string pool: `"Warning! .ctors section[%d]! size=%x\n"`, its `.dtors` and `unknown section` siblings, followed by two function pointers |
| which section is an address in? | `list_segments`, the overlay spaces | `0x804DA598` is `.text`; overlays are addressed `<overlay>::<hex>` |

## What it must NOT be used for

* **Not codegen evidence.** Flags and compiler family come from diffing our object against the retail
  bytes (playbook 17). The dump's `Gekko_Broadway` language field is a CPU setting, not an answer to
  "which compiler built this".
* **Not authoritative by default.** Its symbols are a mix of real SDK names (`RSOLink`,
  `RSONotifyPreRSOLink`) and Ghidra placeholders (`zz_04da7e4_`, `SPEC1_MakeStatus`). Treat a name as
  evidence only when it reads like an SDK identifier *and* its signature agrees with the code.
* **A `splits.txt` claim derived from it can make things worse.** Claiming `.data:0x80629B90` for the
  RSO unit *lowered* `fn_804DABF0` from 99.36 % to 98.01 %: defining the symbol makes dtk drop the
  target's `R_PPC_NONE` pool relocations, so objdiff can no longer pair the pool-relative instructions.
  Claiming the compiler-generated jump table at `0x80629C08` was safe (+0.077 % on
  `RSOStaticLocateObject`). Always measure before and after.

## Recipe (per unit)

```sh
# MCP (ghidra server)
ghidra_list_instances                                   # -> MH3Shared
ghidra_connect_instance project=MH3Shared
ghidra_open_program     path=/DolphinDump85.raw.keep
ghidra_search_functions name_pattern="RSO"              # or get_function_by_address for a known addr
ghidra_get_struct_layout struct_name="RSOModule"        # offsets + real field names
ghidra_read_memory      address=0x80629b90 length=128   # data outside our split
```

To name a whole contiguous unit in one go, fan the address list out through `mcpScript` (one
`get_function_by_address` per address). That is how the name column of the table above was produced -
it takes one call and turns a region full of `fn_XXXX` into named SDK functions.
