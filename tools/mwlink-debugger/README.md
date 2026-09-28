# mwlink-debugger

Interrogate the Metrowerks linker (`mwldeppc.exe`) about a real link: which phase
is running and what it printed, how one **unit** of the project was placed and
relocated, the linker's own input-file records, where it enforces a section's
alignment, and - the production case - what it said when the link failed.

It is the linker-side sibling of [`tools/mwcc-debugger/`](../mwcc-debugger/README.md).
That tool answers "which optimizer pass did that?" for the *compiler* by driving
`mwcceppc.exe` under gdb; the compiler is tractable because every Wii
`mwcceppc.exe` ships a CodeView symbol blob naming its own functions.  **The
linker has no such blob.**  `info` proves it: all 31 `mwldeppc.exe` under
`build/compilers/{Wii,GC}/*` have an empty PE debug directory.  What it *does*
have, and what this tool uses instead, is its own diagnostics (`-v`, `-map`), a
message catalogue in its PE resources (RT_STRING, UTF-16 - that is why `strings`
finds no `Linking:` in the binary), and an import of `LoadStringA` that turns a
message id into the engine's text.

## Supported linker builds

**Wii/1.0 is what this repository links with**, and it is the row that is verified
against a real link.  `build.ninja`'s global `mw_version = Wii\1.0` is what the
`link` rule expands (`ninja -t commands build/RMHE08/main.elf`); the per-object
`mwcc` rules override it with **Wii/1.3** for the *compiler* only.  A lane that
assumes the linker is 1.3 has assumed the wrong binary: `default_linker()` reads
that variable, so every command here uses Wii/1.0 unless you pass another binary.

| linker | status |
|---|---|
| `Wii/1.0` | **verified here** - every derivation below, plus `trace`/`verify`/`records --prove`/`phases --prove` against this repository's own link (byte-identical to `build/RMHE08/main.elf`) |
| `Wii/1.0a`, `1.0RC1`, `1.1`, `1.3`, `1.5`, `1.6`, `1.7`, `0x4201_127` | the same derivations succeed (message catalogue 211/212 messages, `LoadStringA` loader found, record stride `0x2c`, the alignment site found) - **not link-verified here** |
| `GC/2.7` | derivations succeed but the **input-file record stride is `0x38`**, not `0x2c`: the record field table in `records` does not apply |
| `GC/3.0a3`, `3.0a3.2`, `3.0a3.3`, `3.0a3.4`, `3.0a3p1`, `3.0a5` | derivations succeed (`0x2c` record family) - not link-verified here |
| `GC/1.0` .. `GC/2.6` | **unsupported, and reported as such**: these linkers do not import `LoadStringA` and carry no RT_STRING catalogue, so `messages` is empty and `phases` has nothing to hang on. `phases` says so instead of printing a traceback |

## Requirements

* **capstone** (`pip install capstone`) for `anchors`, `phases`, `records` and
  `align` - they disassemble the linker.  `trace`, `verify`, `order`, `messages`
  and `info` do not need it.
* a **native Windows gdb** only for the `--prove` flags and `records --prove`:
  `python tools/mwcc-debugger/fetch_gdb.py` installs one, or pass `--gdb`.  The
  tool looks in `build/tools/gdb.exe`, `~/tools/mwcc-dbg/mingw64/bin/gdb.exe`
  and `PATH`, and prints that advice instead of a traceback when it finds none.
* nothing else: the PE/ELF/MAP readers are stdlib-only.

## Usage

```
python tools/mwlink_debugger.py <command> [options]
```

| command | what it answers |
|---|---|
| `trace <unit\|object>` | follow one **unit** through a real link: was it kept, where did each section land, how did its symbols resolve, which relocations were applied, where did its ctor/dtor fragment go |
| `diagnose` | run the build's own link with `-v`; print the phase stream and every diagnostic with its catalogue id and phase |
| `verify <map> <elf>` | health check: is this map the artifact `elf2dol` consumes (`--identity` byte-compares it) |
| `records` | the linker's internal input-file record: stride, array, fields - and `--prove` to read it out of a running link and cross-check it |
| `align [--unit U]` | where the linker aligns a fragment, what it compares, and which of a unit's claimed starts it cannot honour |
| `anchors` | `{code address: string}` sites derived from `.text` (`--prove` breaks on them in a real link) |
| `phases` | the message loader and the 1248 phase anchors (`--prove` observes the `(id, text)` stream) |
| `order` | the linker's fixed `.ctors`/`.dtors` priority list (Row 46) |
| `timeline` | run the linker's own `-v` diagnostics and name each line's catalogue id |
| `messages` | decode the RT_STRING message catalogue (`--grep`) |
| `info` | PE recon: sections, directories, the "no CodeView blob" check |

`trace` and `diagnose` never write `build/RMHE08/main.elf`: a link they start is
redirected into `--out` (default `build/scratch/mwlink-debug/`, with `-o` and
`-map` rewritten even when you pass your own `--args`).

### The exact invocation that works in this repository

From the repository root, in git-bash:

```bash
python tools/mwlink_debugger.py trace Network/NetworkWiiMediator
```

That is the whole thing - a **unit name**, not a path.  The tool resolves it to
the object the build's *own* link statement consumes (from `build.ninja`), and
because the build writes no map of its own it links one into `build/scratch/`
first.  Real output:

```
# unit Network/NetworkWiiMediator: .../build/RMHE08/obj/Network/NetworkWiiMediator.o
#   resolved as: the object the build's link names (1 entry/entries)
# no link map at .../build/RMHE08/main.MAP (the build does not write one); linking into build/scratch/mwlink-debug/ instead
# .../build/compilers/Wii/1.0/mwldeppc.exe -fp hardware -nodefaults -lcf build\RMHE08\ldscript.lcf -o .../build/scratch/mwlink-debug/trace.elf -map .../build/scratch/mwlink-debug/trace.MAP @.../build/scratch/mwlink-debug/trace.rsp
# linked .../build/scratch/mwlink-debug/trace.elf (8898024 bytes)
object:   .../build/RMHE08/obj/Network/NetworkWiiMediator.o
KEPT:     YES - 179 map row(s) name this object
          in the link's input list: True

where each of its sections landed (map address, read back from the ELF):
  section          output          address     size  align  bytes
  extab            extab        0x8001ca4c    0x1a8      4  identical
  extabindex       extabindex   0x8003d2f0    0x234      4  identical
  .text            .text        0x80413c64   0x1970      4  identical
  .data            .data        0x806024b8    0x36c      8  identical
...
VERDICT: MATCH
```

Notes on that invocation:

* all four shapes work: `trace Network/NetworkWiiMediator`,
  `trace NetworkWiiMediator`, `trace build/RMHE08/src/Network/...o`, and
  `trace <path>` - the first two resolve through the link's own input list, the
  last two are taken as given.  `# resolved as: ...` always says which happened.
* a unit that links as a *flipped* object resolves to
  `build/RMHE08/src/<unit>.o`; one that links as the split target object resolves
  to `build/RMHE08/obj/<unit>.o`.  Picking by name alone would trace the wrong
  object, which is why the link's input list decides.
* `--link` forces a fresh link even if a map is already there; `--map/--elf` use
  someone else's artifacts; `--rsp` supplies the response file explicitly.

The health check, on the artifacts that invocation produced:

```bash
python tools/mwlink_debugger.py verify \
    build/scratch/mwlink-debug/trace.MAP build/scratch/mwlink-debug/trace.elf \
    --identity build/RMHE08/main.elf
# MATCH: 13 section(s) - the map is this ELF
# identity: build/RMHE08/main.elf byte-identical
```

And when a link fails, the same unit name plus `diagnose`:

```bash
python tools/mwlink_debugger.py diagnose
# link: rc=0  (8898024 bytes)
# phase stream:
#   Linking      #   Linking: 'diagnose.elf'
#   Optimizing   #   Optimizing: 'diagnose.elf'
#   Writing      #   Writing: 'diagnose.elf'
#   Layout       #   Layout: 'diagnose.elf' (.text)
#   ...
```

### Output

Every command prints a text report; `--json` is available where it makes sense
(`info`, `trace`, `phases`, `records`, `align`, `diagnose`).  Exit status is 0
for a report that found nothing wrong, 1 for a `FAIL`/failed link/`UNPROVEN`
run, and 2 for a usage or capability error (no capstone, no gdb, unknown unit) -
never a traceback.  `trace`'s `--json` object and the human report carry the same
claims.

## What works, and what does not

**Works (each checked against this repository's own link):**

* `trace` - the unit resolution above, per-section landing verified by reading
  the object's bytes back out of the output ELF, symbol resolution against the
  map *and* the output ELF's symbol table, and every relocation's field decoded
  out of the artifact and compared with the ABI (`ADDR32`, `REL24`,
  `ADDR16_HA/LO`, SDA type 109 - relative to `_SDA_BASE_` for `.sdata`/`.sbss`
  and `_SDA2_BASE_` for `.sdata2`/`.sbss2`).  A type whose semantics are not
  derived is reported `not checked`, not guessed.  A sweep of 260 of the link's
  inputs (all `src/` objects, every 10th `obj/`) is 260/260 `MATCH` against the
  real map and the real ELF, including `main`.
* `verify` - 13/13 output sections of the real map agree with the real ELF
  (`MATCH`), and `--identity` byte-compares it with `build/RMHE08/main.elf`.
* `diagnose` - the phase stream and the diagnostics of a failing link, with
  catalogue ids (msgid 189/14 observed on real failures) and the phase that
  preceded them.
* `records --prove` - the linker's input-file record array read out of a running
  link and cross-checked: **2296/2296** names against the response file,
  **2296/2296** `.comment` versions and **2296/2296** `e_shnum` values against
  the objects themselves.
* `align` - the round-up site (RVA 0x57451 loads `sh_addralign`, 0x57466/0x5747d
  do `(addr + align - 1) & ~(align - 1)`, 0x57492 pushes the `*fill*` literal),
  plus a per-section "can this claimed start be honoured" report for a unit - on
  this tree, **798 sections compared and 0 unhonoured** (which is
  `tools/elf/objalign.py` doing its job), with the one ambiguous object
  (`fn_80429B94.o` has 7 `.data` sections) reported as ambiguous rather than
  compared against a single claimed address.
* `order`, `anchors`, `phases`, `messages`, `info`, `timeline`.

**Does not work yet - deliberately reported, not faked:**

| gap | what the tool says |
|---|---|
| the section table and symbol table records inside the linker | not documented at all; `records` prints only the input-file record it can derive |
| which **phase** dead-strips an unreferenced symbol (Row 36) | the *flag* is settled (byte 5 of the 8-byte `.comment` entry, bit 0x08 - four relinks), the phase is not: no message is printed for that step, and `records`/`phases` cannot name it |
| the exact instruction that reads the `.comment` active flag | not derived: the `.comment` is copied into a heap buffer first (a read watchpoint lands on the `rep movsd` at RVA 0x3a58), and that buffer moves between runs |
| relocation types other than 1/6/10/109 | `not checked (<name>)` in the trace, with the raw word printed |
| non-Wii linker builds | derivations only; not link-verified (see the matrix) |
| `GC/1.0`..`2.6` | `phases`: "no message loader - this build does not import LoadStringA" |
| `--prove` flags | need a gdb and a linker argument list (`--args`), i.e. they are slow and host-dependent by nature |

## If this tool misled you

**If this tool misled you, lacked a record you needed, or its invocation did not
work as written, add a bullet to `.pi/notes/mwlink-debugger-gaps.md` and say so
in your report's tooling section** - a trace or a `MATCH` that was not describing
the artifact you asked about is the most valuable report of all.  That note is
this tool's sibling of `.pi/notes/mwcc-debugger-gaps.md` and one of the sources
of `tools/units/tooling.py`'s ranked register, so one bullet per gap (phrased as
a capability, with what you tried and what it cost) is literally a filed request:
the row is promoted once two or more distinct filers ask for the same thing.

## How it works

`mwlink_debugger.py` has two roles in one file.  As a CLI it derives tables from
the linker binary and runs links; sourced by gdb (the `--prove` flags write a
gdb script) it sets breakpoints on the derived addresses and reports what fired.
A breakpoint that never fires is **unproven**, never an anchor.

Everything build-specific is *derived*, not transcribed:

* the phase anchors come from the one `call [LoadStringA]` site whose `uID`
  argument is not a constant - that is the message loader - and from its
  callers, one per message formatter;
* the input-file record's stride, array and flag setters come from the function
  that memcmps `.comment` against `CodeWarrior`;
* the alignment round-up comes from the function that prints the map's `*fill*`
  row;
* the `.ctors`/`.dtors` priority list comes from the pointer array in `.data`;
* the linker the build uses comes from `build.ninja`'s `mw_version`.

`locate/README.md` is the evidence trail: how each table was derived, what was
measured, and what could not be established.  It is the model of
`tools/mwcc-debugger/locate/README.md`.

## Verification

* `python tools/mwlink_debugger.py --selftest` - 76 fixture/pure checks, no gdb
  and no link is *run* (it is discovered by `python tools/selftest.py --changed`).
  The derivation checks read the real linker binary when capstone is present, and
  say so instead of failing when it is not.
* a sweep of 260 of the link's input objects (all 33 `src/` ones, every 10th
  `obj/` one) traces `MATCH` against the real map and the real ELF - 260/260 -
  which is the check that found the four `trace` bugs listed in
  `locate/README.md`;
* a scratch link from `build.ninja`'s own command line produced
  `sha256 5a64aec3af5ae73d5e33f0b289aaa6d04324eefa8665fb640b50c90d938be758`,
  byte-identical to the `build/RMHE08/main.elf` `ninja` built - and
  `verify --identity` re-checks that on every run;
* `records --prove` cross-checks 2296 records three ways (above);
* `phases --prove` cross-checks every observed message id against the catalogue
  (50/50 on a real link) and reports 6/1248 phase anchors fired - the other 1242
  are candidates for diagnostics this link does not print, and are reported
  unproven;
* the failure paths are exercised on real failures: a missing object
  (`msgid=? (not in the catalogue)`), a dangling symbol (`msgid=16 undefined:
  '%s'`), and a renamed runtime entry symbol (`msgid=189`), each with its phase
  attribution.

`build/RMHE08/main.elf` is never written by this tool: links are redirected into
`build/scratch/`, including explicit `--args` link lines.

## Layout

```
mwlink_debugger.py            the tool (stdlib-only except capstone + gdb)
locate/README.md              how every derived fact was established
../mwcc-debugger/             the compiler sibling (and the gdb installer)
```
