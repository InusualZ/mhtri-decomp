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

   That is catalogue **msgid 189** (the current catalogue numbering - an
   earlier revision of this note said 205, which is 16 too high: it was a table
   *index*, not an id.  `diagnose` now matches the linker's printed text against
   the catalogue and reports 189, and the id-to-text numbering itself is
   re-checked on every `phases --prove`).  It is the answer: the linker
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

## The linker's internal input-file record, derived and read back

The predecessor's residual was "the input-file list, the section table, placement
- no field documented".  The **input-file record** is now derived and verified;
the section table and placement are still reported as gaps.

**How it was found.**  The same lever `anchors` uses: the linker's `.comment`
parser is the function that compares the section's bytes against the literal
`CodeWarrior`, so the anchor that pushes that string *is* its address.  From that
one function:

* `imul esi, esi, 0x2c` (RVA 0x5395b) gives the record **stride 0x2c**;
* `add esi, dword ptr [0x533458]` (RVA 0x5396b) gives the record **array**: the
  base pointer lives at VA 0x533458 and the index is the position of the input
  in the link's own response file;
* `movzx eax, byte ptr [ebp + 0xb]` / `mov byte ptr [esi + 0x1f], al`
  (RVA 0x539ab / 0x539af) are the comment **version** load and store;
* `mov byte ptr [esi + 0x1e], 0/2/3` (0x5397b, 0x539c4, 0x53a60) are the comment
  **kind**;
* `and dl, 0xfb` / `or cl, 4` (0x53975, 0x539cf) and `and cl, 0xf7` / `or dl, 8`
  (0x53983, 0x539e2) are the **flag** clear/set pairs, gated on the memcmp at
  0x5399f and the `cmp byte ptr [ebp + 0xb], 0xb` at 0x539d5.

`records` prints that derivation; `records --prove` breaks at the linker's own
message loader, waits for `Optimizing:`, and dumps the array out of the running
linker.  Every field the table names is then cross-checked against the artifact:

```
$ python tools/mwlink_debugger.py records --prove --limit 2296
...
  ... 2296 record(s) dumped
# cross-check against the response file and the objects themselves: 2296/2296
# names, 2296/2296 comment versions, 2296/2296 section counts
```

That is three independent agreements - the record's name pointer against the
response file's entry at the same index, `record+0x1f` against the `.comment`
version byte read *from the object file*, and `record+0x16` against the same
object's `e_shnum`.  The fields that could **not** be derived are rows in the
table that say so (`+0x04` is a pointer to something unidentified; `+0x0c`
tracks the section count but its encoding is not established; `+0x1c` bit 7 is
*tested* at 0x5742c but its meaning is not derived).

## The alignment the linker enforces (the `objalign.py` question)

It is not a refusal.  mwld **silently aligns the fragment's address up** to the
*input section's own* `sh_addralign` and emits a `*fill*` row for the residue.
The check is findable from the map's own `*fill*` literal (anchor RVA 0x57492 in
the function at 0x57250):

```
0x057451  mov edi, dword ptr [edx + 0x20]   ; sh_addralign of the current input section
0x05745e  add eax, -1
0x057461  lea ecx, [edi - 1]
0x057464  not ecx                           ; ~(align - 1)
0x057466  and eax, ecx                      ; (addr + align - 1) & ~(align - 1)
...
0x057483  cmp byte ptr [esp + 0x80], 4       ; the row kind that gets a '*fill*'
0x05748d  cmp ebx, dword ptr [esp]          ; did the address move?
0x057492  push 0x4cca56                     ; '*fill*'
0x0574b8  call 0x4777a0                     ; print the row
```

The same function reads the input-file record's `+0x1c` bit 7 as a gate
(`mov cl, byte ptr [eax + edi + 0x1c]; shr cl, 7; test; je`, RVA 0x57428) - the
first cross-link between the record table above and the placement code.

**The differential.**  Take `build/RMHE08/src/Pl/fn_8023C2D0.o` (whose `.data`
claims 0x805C34D4, which is 4 mod 8), copy it under `build/scratch/`, set the
allocatable sections' `sh_addralign` to 8 in one copy, and relink both:

| copy | `.data` row | residue |
|---|---|---|
| align 4 (what `objalign.py` leaves) | off `+0x46cb4`, addr `0x805c34d4` | none |
| align 8 (what MWCC emits) | off `+0x46cb8`, addr `0x805c34d8` | a 4-byte `*fill*` at `0x805c34b8`... `0x805c34d8` |
| extab row | `0x80011cb4` -> `0x80011cb8` | moved 4 with it |

No diagnostic, no error, exit 0 both times - which is exactly why the bug reads
as a link-order mystery.  `align <unit>` reports the condition per section from
the linker's side (`align --unit Pl/fn_8023C2D0` says every section is honoured
*after* `objalign.py` has run).

The same check over the whole link - the 2296 input objects, every allocatable
section whose name is unique in its object and whose start `splits.txt` claims -
compares **798 sections and finds 0 that cannot be honoured** (1 section name is
ambiguous: `build/RMHE08/obj/fn_80429B94.o` carries 7 sections called `.data`,
and `align` says so instead of comparing them all against one address).  That is
`tools/elf/objalign.py`'s regression proof seen from the linker's side.

## Row 36: the flag that decides the deadstrip, proved by four relinks

`docs/matching.md` row 36 says MWCC objects leave the `.comment` `active_flags`
byte clear, so the linker trims a trailing unreferenced function that the target
object (with `export_all: true`) keeps.  Which byte, and does the linker really
read it?  Four scratch relinks, each replacing `build/RMHE08/obj/main.o` in the
response file with a copy, answer both:

| copy of `main.o` | `.comment` magic | the 8 risk symbols' flag byte | its `.text` | risk symbols still in the map |
|---|---|---|---|---|
| `keep` (control) | valid | 0x08 on all 8 | `0x1278` | all 8 |
| `single` | valid | 0x08 except `fn_8003F200` = 0x00 | `0x126c` | 7 (`fn_8003F200`, 0xc bytes, gone) |
| `clear` | valid | 0x00 on all 8 | `0x1238` | 2 (`fn_8003FC64`, `fn_80040360`) |
| `badmagic` | broken | 0x00 on all 8 | `0x1278` | all 8 |

The `single` run drops **exactly one symbol**, and its `.text` shrinks by exactly
that symbol's size: the flag is per-symbol.  The `badmagic` run clears the same
bytes but breaks the `CodeWarrior` magic, and **nothing** is dropped - so the
decision really is driven by the parsed `.comment`, not by the ELF symbol table.
The flag is byte 5 of each 8-byte entry (`0x2c + 8*index + 5`), and the bit that
matters is **0x08** (force active / export, the same bit
`docs/comment_section.md` documents).

`fn_8003FC64` and `fn_80040360` survive even with the flag clear, so the
`flipcheck.py` census is a *superset*: 8 candidates, 6 actually trimmed.

**Which phase does it?**  Still not derived, and here is what was tried: the
`.comment` is `memcpy`'d into the linker's own buffer before the entries are
consulted - a read watchpoint on the flag byte inside the original buffer fires
on the `rep movsd` at RVA 0x3a58 (inside the `memcpy` at 0x3a30), and the copy
lands at a heap address that changes between runs, so a watch could not be
pre-armed on it.  There is also no message for the strip step, so the phase
anchors cannot name it.  What can be said: the `.comment` parses happen after
the link prints `Linking:` (observed with `MSG` breakpoints on the loader) and the
strip must precede the layout, so it is in the unannounced work between
`Linking`/`Optimizing` and `Writing`.  The record's `+0x1c` bit 7 gate at RVA
0x5742c is the next thread to pull.

## An erroring link is a first-class case

Most of a flip's failures are *link* failures, so the tool has to be useful when
`mwld` exits non-zero:

```
$ python tools/mwlink_debugger.py diagnose            # the build's own link, with -v
link: rc=0  (8898024 bytes)
phase stream:
  Linking      #   Linking: 'diagnose.elf'
  Optimizing   #   Optimizing: 'diagnose.elf'
  Writing      #   Writing: 'diagnose.elf'
  Layout       #   Layout: 'diagnose.elf' (.text)   (x13 more)
diagnostics:
  (none)
```

Against a link that fails, the diagnostic is matched against the catalogue and
attributed to the last phase printed before it:

```
$ python tools/mwlink_debugger.py diagnose --args '<link line>'
link: rc=1
diagnostics:
  [(no phase seen)] msgid=189: runtime sources 'global_destructor_chain.c' and
  '__init_cpp_exceptions.cpp' both need to be updated to latest version...
  [(no phase seen)] msgid=14: Link failed.
```

Two things are deliberate there.  The id comes from the catalogue (189), and the
phase is `(no phase seen)` because that failure is emitted *before* the linker
prints its first phase line - a proven negative, not a placeholder.  And a
diagnostic the catalogue does not hold is reported without an id rather than
being given one: a link pointed at a missing object prints
`### mwldeppc.exe Usage Error: / #   Specified file '...' not found`, which is
**not** in the RT_STRING table, so the tool says
`msgid=? (not in the catalogue)`.  `trace --link` on a failing link runs the same
attribution and then still traces the object against whatever map exists.

## Four real bugs the trace had, found by tracing `main`

`trace Network/NetworkWiiMediator` was `MATCH` long before this iteration, which
hid four defects that only `trace main` (and a sweep over the link's inputs)
exposes.  All four are fixed and each is pinned by a selftest fixture.

1. **The map's fifth column is an alignment, not a type.**  `trace` read it as a
   row kind and required `4` for a symbol, `1` for a fragment.  Every symbol in
   this link prints 4/8/16 - except a **1-byte label**, which prints `1` and was
   therefore classified as a *fragment*, so `lbl_807947A5` and friends "resolved
   nowhere".  Measured distribution in the real map: fragments print 1 (4318
   rows) and symbols print 4 (66590), 1 (3450), 8 (513) or 16 (8); the `*fill*`
   rows print the alignment they filled to (one prints 16).  The row kind now
   comes from the *name* (`is_section_row`).
2. **Type-109 SDA displacements are relative to the right base register.**  The
   tool always used `_SDA_BASE_` (r13), but a symbol in `.sdata2`/`.sbss2` is
   relative to `_SDA2_BASE_` (r2).  `main.o`'s `.sdata2` labels made the field
   read `0x8020` against an expected `0xCCA0`; with the right base
   `0x80795AC0 - 0x8079DAA0 = 0x8020` is exactly what the output ELF holds.
3. **The map's `Linker generated symbols:` listing was not read.**  Rows there
   have no offset/size columns, so `parse_map` skips them - and `main.o`
   relocates `_f_data` and `_f_bss`, which appear only there.  `parse_map_symbols`
   now resolves them from the artifact's own statement of their addresses.  The
   same listing's `_savegpr_27 (entry of __save_gpr)` rows also have an **empty**
   alignment column, which the row regex did not accept at all: the 48
   `(entry of ...)` rows of this link were being parsed with an empty name, so a
   reference to the symbol they define "resolved nowhere".
4. **Section tables were keyed by name, not by index.**  A split target object
   can hold several sections with one name - `build/RMHE08/obj/fn_80429B94.o`
   has **seven** `.data` sections - and a name-keyed `landing`/`out_of`/
   `reloc_words` answers all of them with the last address.  They are now keyed
   by the object's section *index* (which the RELA header gives exactly), and the
   k-th section of a name is paired with the k-th map row of that name.

After the four: `trace main --map <the real map> --elf <the real ELF>` is
`VERDICT: MATCH`, `trace Network/NetworkWiiMediator` stays `MATCH`, and a sweep
of the link's **260** input objects (all 33 `src/` objects plus every 10th `obj/`
one) is **260/260 `MATCH`** - checked against the real map and the real ELF.

## Not attempted / still open

* The **section table** and the **symbol table** records (and where the map's
  placement decision is stored).  The *input-file* record is done (above); the
  other two are not, so their fields are not documented here - better an honest
  gap than an invented offset.  `records` prints only the table it can derive,
  and it marks the fields it could not name.
* **Which phase drops a dead-stripped symbol** (Row 36).  The *flag* is settled
  (byte 5 of the 8-byte `.comment` entry, bit 0x08, proved by four relinks) and
  the strip is bounded to the work between the `Linking`/`Optimizing` and
  `Writing` messages - but no phase name is attached to it, because the linker
  prints no message for that step.  The next thread is the record's `+0x1c`
  bit-7 gate at RVA 0x5742c.
* The **alignment** question is settled (there is no refusal; the round-up and
  the `*fill*` row are at RVA 0x57451/0x57492), but the *exact instruction that
  reads the `.comment` active-flags byte* is not: the `.comment` is copied into
  a heap buffer first (the read watchpoint lands on the `rep movsd` at RVA
  0x3a58) and that buffer's address changes per run.
* Non-Wii linker builds are only *derivation*-checked, not link-checked: see
  the support matrix in the README.
