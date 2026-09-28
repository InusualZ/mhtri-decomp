# mwlink-debugger: how the derived facts were established

`tools/mwlink_debugger.py` is the linker-side sibling of `tools/mwcc-debugger/`.
This note is its evidence trail: what the linker does and does not expose, how
each table the tool prints was *derived* rather than transcribed, and what the
tool could not prove.  `tools/mwcc-debugger/locate/README.md` is the model.

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
for the linker, and `locate/dissect.py syms` fails on it for the same reason.

The **build actually links with Wii/1.0**, not Wii/1.3: `build.ninja`'s global
`mw_version = Wii\1.0` is what the `link` rule expands
(`ninja -t commands | grep mwld`), while the per-object `mwcc` rules override it
with Wii/1.3.  `default_linker()` reads that variable, so the tool defaults to
the binary the build depends on.

## What the linker has instead

1. **Its own diagnostics.**  `-v` / `-progress` print a phase timeline and
   `-map` writes the link map.  The tool's `timeline` and `verify` use those.
2. **A message catalog in the PE resources.**  The engine's messages - every
   phase name included - are the RT_STRING (type 6) resource, stored as
   `uint16 length + length UTF-16LE WCHARs`, 16 slots per block.  That is why an
   ASCII `strings` pass over `mwldeppc.exe` finds no `Linking:`, no `Layout:`,
   no `Optimizing:`: they are UTF-16.  `messages` decodes the catalog:

   ```
   $ python tools/mwlink_debugger.py messages --grep 'Linking|Layout|Optimizing|Writing:'
   msgid=43   Linking: '%c'
   msgid=44   Copying: '%c'
   msgid=45   Writing: '%c'
   msgid=52   Writing: '%c' (%c)
   msgid=57   Optimizing: '%c'
   msgid=58   Layout: '%c' (%c)
   ```

   (211 messages in Wii/1.0, 212 in Wii/1.3.  The engine prints strings with
   `%c`, not `%s`, because its printer walks the string itself.)

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
  comparison produces.  That is precisely Row 46 - a fragment named `.ctors$10`
  occupies a different fixed slot from one named `.ctors`, so two byte-identical
  objects can still swap words in the final `.ctors`.

The same list is derived from Wii/1.3 (cells `0xcbed8..0xcc048`) and Wii/1.7
(`0xcbee8..0xcc058`); only the addresses move.  `order --map` cross-checks the
list against a real link map's `.ctors`/`.dtors` layout and stays loud when the
layout disagrees.

### The anchors

`anchors` scans `.text` for instructions whose immediate operand is the VA of a
known string (`{anchor RVA: string}`), which is the analogue of
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

### The health check

`verify <map> <elf>` ties the map to the ELF `elf2dol` consumes.  The invariant
that holds is **not** "sum the fragment sizes": a map lists both the input
fragments *and* the symbols inside them, so summing double-counts every section
(measured: 2.00x on `.init`, `.text`, `.bss`, `.data`).  The size is the largest
`offset + size`; the start is the first row's address, and every row must satisfy
`addr == start + offset` (`*fill*` rows excepted - their printed address is the
next fragment's).  On a real link all 13 output sections agree.

## Verification performed

* `python tools/mwlink_debugger.py --selftest` - fixtures only, no gdb, no
  compiler, no linker.
* A real link driven by the derived anchors produced
  `sha256 19d942277ffbb288ab3754df469a8d8e43e984d4b93a9a98d941dc1a1005dbfc`,
  byte-identical to the `build/RMHE08/main.elf` `ninja` built.
* `verify` against a *partial* link (`-r`, six objects) reports `FAIL` with the
  first divergence - the loudness contract, on a real artifact.

## Not attempted

* A *static* map from linker phase to code for the engine phases
  (`Linking:`/`Optimizing:`/`Layout:`), because those messages are referenced by
  resource **id**, not by string address, so no immediate operand names them.
  `timeline` gets the phases from the diagnostic stream instead and names each
  one from the catalogue; breakpointing the message printer would need its
  address, which is not derived here.
* Decorating the map's dead-strip decision (Row 36) with the flag it reads; the
  `-r1`/`-strip_partial` path is the entry point but was not traced.
