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
| 0x80004380-0x80004514 | 404 | `src/Runtime.PPCEABI.H/TRK_interrupt_vectors.c` | `Matching`, 100 % |
| 0x80004514-0x800062C0 | 7596 | `src/Runtime.PPCEABI.H/TRK_interrupt_vector_stubs.c` | `Matching`, 100 % |
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

## Update 2026-09-28: the TRK range is now owned, and the last 164 B are proven unclaimable

The two claims above were re-measured when the owner asked for the section to be completed. One of them was
wrong; the other is now confirmed from three directions.

**The TRK range IS referenced - the search above missed it because it is a relocation, not a literal.** No
`lis`/`addi` base lands in the range and no 4-byte literal 0x80004380 exists, but `.data+0x3ea8` of
`auto_07_8057C820_data.o` - the first word of the `.data` item at 0x805803EC, which `src/mh3_pad.cpp:143`
declares as a `.data` pool entry - carries an `R_PPC_ADDR32` to 0x80004514. A grep for literals cannot see a
relocation. Claiming the range without defining a symbol at that address fails the link with
`undefined: 'lbl_80004514'` (measured, first attempt).

**The range is now owned as two units, both `Object(Matching, ...)`, both 100.00000 %:**

| range | size | unit | evidence |
| --- | --- | --- | --- |
| 0x80004380-0x80004514 | 404 | `Runtime.PPCEABI.H/TRK_interrupt_vectors.c` | object `.init` == target `.init[0:0x194]`, byte for byte |
| 0x80004514-0x800062C0 | 7596 | `Runtime.PPCEABI.H/TRK_interrupt_vector_stubs.c` | object `.init` == target `.init[0x194:]`, byte for byte |

`ninja build/RMHE08/ok` green, DOL sha1 unchanged at `bf4850739478caaedfe675949eb7c28595a7fde9`, and the map's
`lbl_80004514` is renamed `gTRKSystemResetVectorSlot` (rule 7) - it is the start of the second unit, so the
reference is documented instead of anonymous. The name is marked a GUESS in both files: all that is known is
that the address lies inside the 0x100 (system reset) vector's 256-byte slot and its bytes are zero.

**Why two units and not one object: MWCC pads every object in `.init` to 8 bytes, so no interior label can sit
at a 4-mod-8 offset.** Measured with a probe in the same section - two 5-byte `u8` arrays land at +0x0 and
+0x8, and `__declspec(align(4))` is rejected with a usage warning - so a single 8,000 B array cannot also
define a boundary at 0x80004514 (0x194). Splitting the range at that address makes each unit one array with no
interior boundary; the second claim starts 4-mod-8, which is playbook row 55 - `tools/elf/objalign.py` lowers
the emitted alignment to `lowbit(claimed start)` (both objects report align 4, the target's own). The bytes are
generated by `tools/splits/gen_trk_vectors.py` from the split target object, and the source files are labelled
as data: the stubs carry their addresses as absolute immediates and the target object has no relocation
section at all, so any expression form would add a relocation the target does not have.

**The last 164 B is not claimable, and a claim on it is silently INERT (proven, not argued).** The section
above reasons from the linker's own symbol table; the experiment was then run:

* no split target object exists for 0x80006624-0x800066C8 at all (`ls build/RMHE08/obj/ | grep 80006624` ->
  nothing; a glob over `obj/*.o` finds no candidate), i.e. `dol split` attributes those bytes to no object;
* adding the claim plus a source that emits the exact 164 B **builds with 0 FAILED and links**, but
  `grep -c init_linker_info build/RMHE08/ldscript.lcf` is **0** - dtk never puts the range in the link, and the
  linker keeps generating the bytes itself;
* the proof: with one word deliberately corrupted to `0xDEADBEEF` the build still passes and the DOL is still
  byte-identical. Our bytes are not what the DOL contains.

So a claim there would be bookkeeping that lies - the ledger would count 164 B as claimed while the DOL's
bytes come from `mwldeppc`. They stay unowned, and `__start.c`'s `extern`s are the correct spelling.
