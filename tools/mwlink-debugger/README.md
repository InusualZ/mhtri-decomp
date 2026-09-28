# mwlink-debugger: how the derived facts were established

`tools/mwlink_debugger.py` is the linker-side sibling of `tools/mwcc-debugger/`.
This note is its evidence trail: what the linker does and does not expose, how
each table the tool prints was *derived* rather than transcribed, which of its
dumps were checked against the artifact, and what the tool could not prove.
`tools/mwcc-debugger/locate/README.md` is the model.

Commands, in the order this note explains them:

```
python tools/mwlink_debugger.py info|messages|order|anchors|timeline|verify ...
python tools/mwlink_debugger.py phases [--prove]
python tools/mwlink_debugger.py trace <object> [--link]
```

## The lever the compiler had is absent here

Every Wii `mwcceppc.exe` ships a CodeView `NB11` symbol blob (Metrowerks appends
it after the last PE section and points the debug directory at it), which is why
compiler support is tractable: `locate/dissect.py syms` names ~5800 functions.

The linker does not.  All 31 `mwldeppc.exe` under `build/compilers/{Wii,GC}/*`
have an **empty** debug directory:

```
$ python tools/mwlink_debugger.py info build/compilers/Wii/1.0/mwldeppc.exe
debug directory: 0 entries  <- no CodeView blob; the compiler's symbol lever is absent
```

This was re-verified for every Wii build (`1.0 1.0a 1.0RC1 1.1 1.3 1.5 1.6 1.7
0x4201_127`) and for the whole GC row - `Pe.debug_entries()` returns `[]`
everywhere.  The compiler's `-sym on` / `debug_blob()` path simply does not exist
for the linker.

The **build actually links with Wii/1.0**, not Wii/1.3: `build.ninja`'s global
`mw_version = Wii\1.0` is what the `link` rule expands
(`ninja -t commands | grep mwld`), while the per-object `mwcc` rules override it
with Wii/1.3.  `default_linker()` reads that variable, so the tool defaults to
the binary the build depends on.

## What the linker has instead

1. **Its own diagnostics.**  `-v` / `-progress` print a phase timeline and
   `-map` writes the link map.  The tool's `timeline` and `verify` use those;
   `trace` reads the map.
2. **A message catalog in the PE resources.**  The engine's messages - every
   phase name included - are the RT_STRING (type 6) resource, stored as
   `uint16 length + length UTF-16LE WCHARs`, 16 slots per block.  That is why an
   ASCII `strings` pass over `mwldeppc.exe` finds no `Linking:`, no `Layout:`,
   no `Optimizing:`: they are UTF-16.  `messages` decodes the catalog.
3. **USER32's `LoadStringA`.**  The linker's own message loader calls it, and
   that import is the hook the `phases` command hangs the phase table on
   (below).

### The catalogue's id numbering - corrected against a real link

The id of a message is **the id `LoadStringA` is asked for**, and that is not the
index of the string in the table: the first RT_STRING block is named 1 and holds
ids 0..15, so `msgid = (block_name - 1) * 16 + slot`.

That formula is measured, not assumed.  Breaking at the linker's own message
loader on a real link (`phases --prove`) prints the `(id, string)` pairs the
linker really asks for, and they are exactly `27 = Linking: '%c'`,
`29 = Writing: '%c'`, `41 = Optimizing: '%c'`, `42 = Layout: '%c' (%c)`,
`36 = Writing: '%c' (%c)` - 50 of 50 observed messages agree with the catalogue's
`id -> text`.  An earlier revision of this tool numbered from `block_name * 16`
(16 too high: it printed `msgid=43 Linking: '%c'`), which is a *table index*,
not an id the linker ever uses.  `phases --prove` re-checks this every run and
stays loud when a message's id and text disagree.

## The derived tables

### The ctor/dtor order (Row 46)

`order` finds the ctor/dtor name pool (`.ctors`, `.ctors$00`, `.ctors$10`,
`.ctors$99`, `.dtors`, `.dtors$00`, `.dtors$10`, `.dtors$15`, `.dtors$99`) by
scanning `.rdata`/`.data` for null-terminated literals matching the name shape,
then finds every pointer into that pool and keeps the longest stride-consistent
run.  The record order in `.data` **is** the priority order:

```
$ python tools/mwlink_debugger.py order build/compilers/Wii/1.0/mwldeppc.exe
stride:  0x2e  cells: 0xcae58, 0xcae86, 0xcaeb4, 0xcaee2, 0xcaf10, 0xcaf3e, 0xcaf6c, 0xcaf9a, 0xcafc8
the linker's fixed ctor/dtor order (record order in .data):
  0: .ctors$00   1: .ctors$10   2: .ctors      3: .ctors$99
  4: .dtors$00   5: .dtors$10   6: .dtors$15   7: .dtors     8: .dtors$99
```

Two details matter and neither was obvious:

* the stride is **0x2e**, not a multiple of 4, so every second record's pointer
  field is only 2-byte aligned.  A 4-byte-stepped scan finds 5 of the 9 entries
  and silently reports a wrong table (this is a real bug the tool had to be
  fixed for).
* the order is a **fixed list**, not a sort: `.ctors` (the plain name) sorts
  *between* `.ctors$10` and `.ctors$99`, which no lexicographic or `$NN`-numeric
  comparison produces.

The real link map uses exactly that list, in that order:

```
.ctors section layout
  00000000 000000 8056f2c0  1 .ctors$00  Linker Generated Symbol File
  00000000 000004 8056f2c0  1 .ctors$10  __init_cpp_exceptions.o
  00000004 000004 8056f2c4  1 .ctors     mh3_pad.o
  ...  74 more plain-`.ctors` fragments, in link order ...
  0000016c 000004 8056f42c  1 .ctors$99  Linker Generated Symbol File
```

`.ctors$00` and `.ctors$99` are the linker's own **sentinels** - it synthesizes
those two fragments (they carry `_ctors` and `_ctors$99`, which is how
`_rom_copy_info` and `__start` find the table's ends) and credits them to
"Linker Generated Symbol File".  The plain `.ctors` class is filled in **link
order**; the `$NN` classes are fixed slots.

### What actually decides a `.ctors$NN` slot (Row 46, re-derived)

The statement above ("the linker collects `$NN` fragments in a fixed name
order") is what the map *looks* like it says, and it is only half right.  The
slot is not chosen from the input section's name.  Three experiments on a real
link (all of them relinks into `build/scratch/`, never `build/RMHE08/main.elf`):

1. `mh3_pad.o`'s plain `.ctors` renamed to `.ctors$10` (a same-length rename in
   `.shstrtab`, with `sh_name` repointed) - **the word moves**: the map's `$10`
   slot gains `pad_r10.o` and the plain class starts after it.  So for a
   fragment with *no* named ctor entry, the section name does pick the class.
2. `__init_cpp_exceptions.o`'s `.ctors$10` renamed to `.ctors`, `.ctors$55`,
   `.ctors$01`, `.ctors$99` and even `.dtors$10` - **the output `.ctors` is
   byte-identical every time**, the word stays in the `$10` slot, and the map
   only re-credits that slot's fragment to "Linker Generated Symbol File".
   The fragment's column name in the map is then the *class*, not the section.
3. Renaming the *symbols* `__init_cpp_exceptions_reference`,
   `__fini_cpp_exceptions_reference` or `__destroy_global_chain_reference`
   (same length, nothing else touched) makes the link **fail** with the linker's
   own diagnostic:

   ```
   ### mwldeppc.exe Linker Error:
   #   runtime sources 'global_destructor_chain.c' and
   #   '__init_cpp_exceptions.cpp' both need to be updated to latest version.
   #   Please contact Freescale support.
   ```

   That is catalogue **msgid 205**, and it is the answer: the linker
   *validates those three symbol names*, because its C++ ctor/dtor support is
   keyed on them.  They are the names next to the class-name pool in the image
   (`__init_cpp_exceptions` 0x4cb416, `__init_cpp_exceptions_reference`
   0x4cb42e, `__init_cpp_exceptions.o` 0x4cb44e, `__fini_cpp_exceptions`
   0x4cb466, `__destroy_global_chain_reference` 0x4cb47e,
   `global_destructor_chain.o` 0x4cb4a2, `__fini_cpp_exceptions_reference`
   0x4cb4d6), and they are immediate operands of the section-name selector at
   RVA 0x42e15..0x42f80.

So: the slot for an MWCC-emitted ctor/dtor entry is decided by the **entry
symbol's name** (`..._reference`), not by the section's name; a tool that
compares only section names cannot see a difference here, and a tool that
renames sections has no effect on it.  `trace` prints both, per unit.

`order --map` cross-checks the derived order list against a real link map's
`.ctors`/`.dtors` layout and stays loud when the layout disagrees.

### The anchors

`anchors` scans `.text` for instructions whose immediate operand is the VA of a
known string (`{anchor RVA: string}`), the analogue of
`locate/pass_points.py` deriving return addresses of `call <pass>`.  The
ctor/dtor name compares are flagged `section-name`:

```
$ python tools/mwlink_debugger.py anchors
anchor     kind          compare  string
0x042e39   section-name  10 bytes .ctors$10
0x042fd9   section-name  7 bytes  .dtors
0x0430b0   section-name  10 bytes .dtors$10
0x043259   section-name  10 bytes .dtors$15
...
```

Each is proven by its shape (`mov edi, <va>; mov ecx, <len>; repe cmpsb`, so the
compare is exact) **and** by a gdb run: `anchors --prove` sets a breakpoint on
every derived anchor, runs a real link, and reports FIRED or UNPROVEN per
anchor.  A breakpoint that never fires is reported as unproven, never claimed.
12 of 27 fire on the project's own link, and the `section-name` ones fire with
the section name the linker is looking at (`0x42e39 ... matched '.ctors$10'`).

### The phases: the message machinery, derived

The open item this closes: the engine's phase messages are referenced by
resource *id*, so no immediate operand names them - but the loader that turns an
id into a string is findable, because it calls the imported `LoadStringA`:

* the IAT slot of `LoadStringA` comes from the import directory
  (RVA 0x11238c here); `call dword ptr [slot]` in `.text` has exactly two sites;
* **the message loader is the one whose `uID` argument is not a constant** - the
  other site pushes `0x65` and loads one fixed string of its own.  The loader is
  RVA 0x3d0b0 (12 callers), the one-off RVA 0x56f0-ish (16 callers, constant
  101).  That single rule is what keeps this derivation from picking the wrong
  function;
* the loader's **callers are the engine's message formatters**, one per message.
  Each pushes its id as *arg3* (the loader reads arg3 at `[esp+0x18]` after its
  own two pushes - arg1 is the buffer it keeps in EBX - and hands it to
  `LoadStringA` as `uID`).  12 of them, with ids 16, 17, 18, 21, 26 and some
  computed at run time;
* the function entries are found from the layout (`nop`/`int3` runs between
  functions) and the derivation **checks itself**: an entry nothing calls is
  reported as unproven;
* `phases` then lists every `return address of call <formatter>` as a **phase
  anchor** (1248 of them here).

`phases --prove` breaks on all of them *and* on the loader's observation anchor
(the instruction after the `LoadStringA` call - its stack layout was measured:
the uID is at `[esp+0x10]`, the buffer pointer is still in EBX), and attributes
each observed message to the most recent anchor.  On the project's own link:

```
# breakpoints: 1248 phase anchor(s), 1249 total
the link's phase stream, as the linker's own message loader printed it:
  (no anchor seen) id=27   Linking: '%c'  [catalogue agrees]
  anchor 537c9   id=41   Optimizing: '%c'  [catalogue agrees]
  anchor 4ece5   id=29   Writing: '%c'  [catalogue agrees]
  anchor 53847   id=42   Layout: '%c' (%c)  [catalogue agrees]
  anchor 59bcb   id=42   Layout: '%c' (%c)  [catalogue agrees]   (x13)
  ...
# catalogue cross-check: 50 of 50 observed messages match the catalogue's id -> text
phase anchors: 6/1248 fired in this link
```

Six anchors are *proven* by this link; the other 1242 are candidates that only
fire for diagnostics this link does not print, and they are reported as
unproven rather than claimed.  The phase order (Linking -> Optimizing ->
Writing -> Layout xN -> Writing) is the same timeline `-v` prints, but now with
the code address that printed each line.

### The health check

`verify <map> <elf>` ties the map to the ELF `elf2dol` consumes.  The invariant
that holds is **not** "sum the fragment sizes": a map lists both the input
fragments *and* the symbols inside them, so summing double-counts every section
(measured: 2.00x on `.init`, `.text`, `.bss`, `.data`).  The size is the largest
`offset + size`; the start is the first row's address, and every row must satisfy
`addr == start + offset` (`*fill*` rows excepted).  On a real link all 13 output
sections agree, and `--identity` byte-compares the ELF with `build/RMHE08/main.elf`.

## Tracing one input object: `trace`

`trace <object>` answers the question this lane exists for - **how does one
input object get linked with the rest of the units?** - and every claim it makes
is a comparison against the artifact:

* **did the link keep it, and where did each of its sections land** - the object
  is parsed as the linker parses it (ELF32 **big-endian** PowerPC, RELA
  relocations, `st_info`/`st_shndx`), and each allocatable section is matched
  with the map row that names it *as its source*.  The landing address is then
  read back: the object's own bytes must be in the output ELF at that address,
  **except** the 4-byte words a relocation lands on (the linker wrote those).
  A section the map lists per entry instead of per fragment (`extab`,
  `extabindex`, whose rows are named `@etb_<VA>`/`@eti_<VA>`) is landing-checked
  at its first row's address and says so.
* **how its symbols resolved** - definitions are checked against the map *and*
  against the output ELF's own `.symtab` (two independent witnesses); undefined
  symbols name the unit that defines them, which is the answer to "why was that
  other object kept at all".
* **which relocations were applied** - for every RELA in the object, the symbol
  is resolved, and the field the linker wrote is decoded out of the output ELF
  and compared with what the ABI says it must be: `ADDR32` (word), branch24
  (`(S+A-P)` in bits 2..25, LK/AA preserved), `ADDR16_HA/LO/HI` (which halfword
  is *reported*, not assumed), the small-data form (`type 109`:
  `(S - _SDA_BASE_) & 0xFFFF`, with `_SDA_BASE_` taken from the artifact), and
  a type whose semantics are not derived is reported `not checked` rather than
  guessed at.
* **where its ctor/dtor fragment went** - the class rank from the derived order
  list, the slot it occupies in the merged section, the whole neighbour list,
  and - since Row 46 above - whether the unit defines one of the three runtime
  entry symbols the linker keys the class on.

`trace --link` runs the build's **own** link first, into `build/scratch/`: the
argument list comes from `build.ninja`'s `ldflags` (including the per-build
`-lcf`), and the response file is derived from the `build ...: link ...`
statement's input list (ninja deletes the `.rsp` it writes).  Never
`build/RMHE08/main.elf`.

### The trace that was produced

`build/RMHE08/src/Runtime.PPCEABI.H/__init_cpp_exceptions.o` (a flipped unit's
object - the one that owns the `.ctors$10` word), traced through a link this
tool ran itself (`trace --link`, whose output ELF is byte-identical to the one
`ninja` built):

```
object:   build/RMHE08/src/Runtime.PPCEABI.H/__init_cpp_exceptions.o
KEPT:     YES - 11 map row(s) name this object
          in the link's input list: True

where each of its sections landed (map address, read back from the ELF):
  section          output          address     size  align  bytes
  .text            .text        0x80457420     0x70      4  identical
  .sdata           .sdata       0x80793cc8      0x4      8  identical
  .ctors$10        .ctors       0x8056f2c0      0x4      4  identical
  .dtors$10        .dtors       0x8056f440      0x4      4  identical
  .dtors$15        .dtors       0x8056f444      0x4      4  identical

how its symbols resolved:
  D fragmentID                       .sdata+0x0 = 0x80793cc8  [__init_cpp_exceptions.o]  ELF 0x80793cc8
  U _eti_init_info                   -> Linker Generated Symbol File at 0x8003f1c8
  U __register_fragment              -> Gecko_ExceptionPPC.o at 0x80457490
  D __init_cpp_exceptions            .text+0x0 = 0x80457420  [__init_cpp_exceptions.o]  ELF 0x80457420
  U __unregister_fragment            -> Gecko_ExceptionPPC.o at 0x804574dc
  D __fini_cpp_exceptions            .text+0x3c = 0x8045745c  [__init_cpp_exceptions.o]  ELF 0x8045745c
  D __init_cpp_exceptions_reference  .ctors$10+0x0 = 0x8056f2c0  [__init_cpp_exceptions.o]  ELF 0x8056f2c0
  D __destroy_global_chain_reference .dtors$10+0x0 = 0x8056f440  [__init_cpp_exceptions.o]  ELF 0x8056f440
  U __destroy_global_chain           -> global_destructor_chain.o at 0x804566bc
  D __fini_cpp_exceptions_reference  .dtors$15+0x0 = 0x8056f444  [__init_cpp_exceptions.o]  ELF 0x8056f444

relocations applied to it (value decoded out of the output ELF):
  .text+0x00c type 109     'fragmentID' S=0x80793cc8 A=0x0 -> 0xaea8 (sda disp at +2)  APPLIED
  .text+0x01a ADDR16_HA    '_eti_init_info' S=0x8003f1c8 A=0x0 -> 0x8004 (high half at +0)  APPLIED
  .text+0x022 ADDR16_LO    '_eti_init_info' S=0x8003f1c8 A=0x0 -> 0xf1c8 (low half at +2)  APPLIED
  .text+0x024 REL24        '__register_fragment' S=0x80457490 A=0x0 -> 0x4c (branch24)  APPLIED
  .text+0x028 type 109     'fragmentID' S=0x80793cc8 A=0x0 -> 0xaea8 (sda disp at +2)  APPLIED
  .text+0x048 type 109     'fragmentID' S=0x80793cc8 A=0x0 -> 0xaea8 (sda disp at +2)  APPLIED
  .text+0x054 REL24        '__unregister_fragment' S=0x804574dc A=0x0 -> 0x68 (branch24)  APPLIED
  .text+0x05c type 109     'fragmentID' S=0x80793cc8 A=0x0 -> 0xaea8 (sda disp at +2)  APPLIED
  .ctors$10+0x000 ADDR32   '__init_cpp_exceptions' S=0x80457420 A=0x0 -> 0x80457420 (word)  APPLIED
  .dtors$10+0x000 ADDR32   '__destroy_global_chain' S=0x804566bc A=0x0 -> 0x804566bc (word)  APPLIED
  .dtors$15+0x000 ADDR32   '__fini_cpp_exceptions' S=0x8045745c A=0x0 -> 0x8045745c (word)  APPLIED

its ctor/dtor fragment (the Row 46 question):
  .ctors$10    size 0x4  fixed-order rank 1  -> .ctors slot 1
      row 0x8056f2c0 size 0x4 credited to __init_cpp_exceptions.o
      -> [0] 0x8056f2c0 .ctors$00    Linker Generated Symbol File
      -> [1] 0x8056f2c0 .ctors$10    __init_cpp_exceptions.o
         [2] 0x8056f2c4 .ctors       mh3_pad.o
         ...
      runtime symbol '__init_cpp_exceptions_reference' -> .ctors$10 (the linker keys this class on the symbol name)
```

A second unit, the plain `.ctors` case (the unit Row 46's audit was about):

```
object:   build/RMHE08/obj/ef/ef_emform.o
KEPT:     YES - 31 map row(s) name this object
  extab            extab        0x8000a584     0x48      4  identical
  extabindex       extabindex   0x80023bec     0x6c      4  identical
  .text            .text        0x800cccf8    0x2b8      4  identical
  .ctors           .ctors       0x8056f2e8      0x4      4  identical
its ctor/dtor fragment:
  .ctors       size 0x4  fixed-order rank 2  -> .ctors slot 11
      ... -> [11] 0x8056f2e8 .ctors       ef_emform.o
```

## Verification performed

* `python tools/mwlink_debugger.py --selftest` - 35 fixture checks, no gdb, no
  compiler, no linker (`python tools/selftest.py --changed`).
* A real link driven by the derived anchors produced
  `sha256 19d942277ffbb288ab3754df469a8d8e43e984d4b93a9a98d941dc1a1005dbfc`,
  byte-identical to the `build/RMHE08/main.elf` `ninja` built; `trace --link`
  reproduces that hash from `build.ninja`'s own command line.
* `verify` against a *partial* link (`-r`, six objects) reports `FAIL` with the
  first divergence - the loudness contract, on a real artifact.
* `phases --prove` and `anchors --prove` are gdb runs on real links; every
  UNPROVEN line is a live negative, not a formality.

## Not attempted

* The linker's internal **records** (input-file list, section table, symbol
  table, placement).  The map is the artifact that *reports* them and `trace`
  reads the map, so no field of those structures is documented here - better an
  honest gap than an invented offset.
* Which phase drops a dead-stripped object (Row 36), and the alignment refusal
  `tools/elf/objalign.py` works around: `trace` can show that a unit is missing
  or misaligned in the artifact, but not yet the branch that decides it.  The
  phase anchors are the way in: break on the ones that fire and read the flags
  of the unit being considered.
