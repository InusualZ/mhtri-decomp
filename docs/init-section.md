# The `.init` section: who emits each byte

`.init` is 9,928 bytes (0x26C8) at 0x80004000 and it is fully accounted for: four registered units, one
unowned range, and **two fragments the linker emits itself**. The table is the MW linker's own
`.init section layout` (`-map`, produced by `python tools/mwlink_debugger.py trace <unit>`; health-checked
against the ELF with `python tools/mwlink_debugger.py verify <map> <elf> --identity build/RMHE08/main.elf`,
`MATCH: 13 section(s)` + `identity: ... byte-identical`). Its last column is the authority for attribution:
an input object's name, or `Linker Generated Symbol File`, or `*fill*`.

| range | size | emitter | state |
| --- | --- | --- | --- |
| 0x80004000-0x80004350 | 848 | `src/Runtime.PPCEABI.H/memcpy.c` (`memcpy`, `__fill_mem`) | `Matching`, 100 % |
| 0x80004350-0x80004380 | 48 | `src/Runtime.PPCEABI.H/memset.c` | `Matching`, 100 % |
| 0x80004380-0x800062C0 | 8000 | **unowned** - the TRK interrupt-vector table, see below | `auto_00_80004380_init.o` |
| 0x800062C0-0x800065B8 | 760 | `src/Runtime.PPCEABI.H/__start.c` (6 statics) | `Matching`, 100 % |
| 0x800065B8-0x800065C0 | 8 | **`*fill*`** - the linker realigning `__ppc_eabi_init.o` (align 16) | not claimable |
| 0x800065C0-0x80006624 | 100 | `src/Runtime.PPCEABI.H/__ppc_eabi_init.cpp` | `Matching`, 100 % |
| 0x80006624-0x800066C8 | 164 | **`Linker Generated Symbol File`** - `_rom_copy_info` 0x84, `_bss_init_info` 0x20 | not claimable |

## The two fragments the linker emits

`_rom_copy_info` and `_bss_init_info` are the linker's own symbols: `mwldeppc.exe`'s string table lists both
next to `_eti_init_info`/`_ctors$99`/`_GLOBAL_OFFSET_TABLE_` and the messages `Linker Generated Symbol File`
and `%s found as linker generated symbol`, and the map's `.init` row names that file as the input. Their
**contents are link-time facts** - one `(addr, rom, size)` triple per data section at its finished address
(0x80004000/0x26C8, 0x800066E0/0x17D84, ... 0x80795AA0/0x7D28, then a 0 terminator) and the `.bss`/`.sbss`/
`.sbss2` runs to clear (0x80658500/0x138904, 0x80794760/0x1340, 0x8079D7E0/0x18). No translation unit can
emit them: the values change with every section that grows, so a definition in source would have to hard-code
the finished layout. They stay unclaimed, and `__start.c`'s `extern`s are the correct spelling.

## The TRK interrupt-vector table (0x80004380-0x800062B4 + 12 B pad)

It is the Metrowerks TRK exception-vector image: an array of stubs **indexed by the PowerPC vector offset**,
so that a raw copy to 0x80000000 puts each handler at its vector. Base 0x80004380 holds the banner string
`"Metrowerks Target Resident Kernel for PowerPC"` (45 B + 3 pad, 0x80004380-0x800043B0); the reset slot
0x100 holds one 4-byte word (0x48464BE0) that is not a decodable branch; every handler is a stub that saves
r2-r4 in SPRG1-3, plants `TRK_InterruptHandler` (0x8046C418 - the shared dump's name for it) in SRR0, loads
its vector ID in r3, and `rfi`s. 24 stubs, 1,508 bytes of content in the 8000-byte range:

```
vector  stub address  bytes   vector  stub address  bytes
0x000   0x80004380    48  (banner 45 + 3 pad)    0x1000  0x80005380    112
0x100   0x80004480     4      0x1100  0x80005480    112
0x200   0x80004580    76      0x1200  0x80005580    112
0x300   0x80004680    52      0x1300  0x80005680     52
0x400   0x80004780    52      0x1400  0x80005780     52
0x500   0x80004880    52      0x1500  -             zero
0x600   0x80004980    52      0x1600  0x80005980     52
0x700   0x80004A80    52      0x1700  0x80005A80     52
0x800   0x80004B80    52      0x1800-0x1B00  -     zero
0x900   0x80004C80    52      0x1C00  0x80005F80     52
0xA00, 0xB00  -  zero         0x1D00  0x80006080     52
0xC00   0x80004F80    52      0x1E00  0x80006180     52
0xD00   0x80005080    52      0x1F00  0x80006280     52
0xE00   0x80005180    52
0xF00   0x80005280   108  (`b` to 0x800052D4, then the 0xF20 and 0xF00 stubs)
```

The range ends mid-slot at 0x800062B4 (`gTRKInterruptVectorTableEnd`); the 12 bytes to 0x800062C0 are zero
and belong to the same auto unit. Nothing in the DOL references it - there is no `lis`/`addi` (or `ori`) pair
whose base is 0x8000* and whose displacement lands in the range, and no 4-byte literal 0x80004380 anywhere in
the DOL, so it is dead runtime-library data kept because its object was linked whole.

**It stays unowned** (`docs/plan.md` §10, decided queue 2: "zero relocations, not expressible in C without
hand-written assembly"). Confirmed, with two additions: the stubs' `rfi`/`mtsprg`/`mfsrr0` sequence has no C
construct, and the 0x100 stride is not reachable from a flag either - `-func_align` accepts only
`4, 8, 16, 32, 64, or 128` (measured: `mwcceppc.exe -func_align 256` -> `Unknown option '256'`), so the
position-coding has to be explicit layout in assembly text. Owning it means writing ~8,000 bytes of
hand-written assembly as `asm void` bodies (the precedent for that style is `src/main.cpp`'s GQR setter
`fn_8003F4D8`, and the SDK's own `targimpl.c` writes its TRK routines that way) - a premise change, so it is
the owner's call, not a lane's. The DOL is already byte-exact with the auto unit as the input.
