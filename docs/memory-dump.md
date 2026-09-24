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

**READ-ONLY: `MH3Shared` is shared with another effort, so this repo only ever reads it.**

* Never modify it: no renames, no type/struct creation, no comments, no imports, no re-analysis, no saves.
* Our names live in `config/RMHE08/symbols.txt`; change one with
  `python tools/symbols/symedit.py rename <old> <new>` - in this repo, never in Ghidra.
* Why it matters: a change there is invisible to this repo (no diff, no review) and can silently break the
  other effort working in the same project.

## The symbol map of the same dump (`DumpSymbols.zip`)

A second, cheaper oracle sits next to the dump: `D:/WiiExperiment/DumpSymbols.zip` holds
`Dump_Loading85.raw.h` and `Dump_Loading85.raw.map` - **48 367 symbol lines** for the same
loading-state dump, in Dolphin's map format:

```
CntSdRsoTerminate 800406ac f          # name [demangled argument list] address flags; `f` = function
kbd_open(unsigned 80040798 f
zz_0040598_ 80040598 f                # `zz_<address>_` means the dumper had no name for it
```

It is a plain text file, so it needs no Ghidra session and no MCP call - which makes it the first thing to
consult when a region is full of `fn_XXXXXXXX`. Addresses are the DOL's own (checked against the map:
`kbd_move` 0x80040770, `set_kbd_param` 0x800407C4, `ck_sub_ovl_idx` 0x800408A0 all agree), and it settles
questions the Ghidra project answers only one function at a time:

* **a real name**, e.g. `fn_800406AC` -> `CntSdRsoTerminate` (batch 6; the rename went map + source as usual);
* **whether an address is a function at all** - a `fn_*` in our map with no line in this one, where the
dump does carry `zz_` placeholders for genuinely unnamed functions, is usually not a function. Five
4-byte `fn_80040794`-style symbols in `src/auto/80040598_fn_80040598.cpp` turned out to be the dead
epilogue MWCC emits after a `mtctr`/`bctr` tail-call dispatcher: merging each into the dispatcher before it
(map sizes +4, symbols deleted) closed four of them outright;
* **a signature**, since the argument list is demangled (`kbd_init(unsigned`, `set_kbd_param(char`).

Read it with `python -c "import zipfile;z=zipfile.ZipFile('D:/WiiExperiment/DumpSymbols.zip');
print(z.read('Dump_Loading85.raw.map').decode('latin-1'))"` - or grep the `.map` member without extracting
it. It is **not** codegen evidence (same rule as the dump itself), and a name it does not have is not
proof of anything: `zz_` means "unnamed", not "absent".

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
