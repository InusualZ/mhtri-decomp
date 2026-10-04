# `mwcc-debugger` - Windows-native port of cadmic/mwcc-debugger: run mwcceppc.exe under gdb and dump AST/PCode/regalloc per pass

<!-- generated from the module docstring of `tools/mwcc-debugger/mwcc_debugger.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

mwcc_debugger.py - dump MWCC compiler internals while it compiles a file.

## Users

no caller in the tracked tree

## CLI

Flags: `--args`, `--emulator`, `--exe`, `--gdb`, `--gdb-port`, `--timeout`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: source + flags -> build/mwcc-debug dumps.

## Invariants and rules

* Windows-native port of cadmic/mwcc-debugger (see PROVENANCE.md for the upstream commit and the list of changes). Upstream runs the Windows compiler under `retrowin32` - an emulator whose only purpose is to run a Windows x86 binary on a POSIX host - and attaches gdb to its gdb stub. On Windows the compiler already runs natively, so the emulator is the part that goes away: this port runs `gdb` directly on `mwcceppc.exe`. The emulator path is still available for POSIX hosts via `--emulator PATH`.
* Two roles, as upstream:
* started as `python mwcc_debugger.py ...` -> `start_gdb()`: work out which compiler build we were handed, sanity check the tool chain, then exec gdb with this same file as its command script.
* sourced by gdb -> `run_compiler()`: set the breakpoints for that build and dump compiler state to text files.
* Everything build-specific (addresses, table sizes, record layouts) lives in versions.py; this file is mechanism only.
* PORT: changes from upstream are marked with a `PORT:` comment.

## Lib dependencies

proc (the gdb session, `lib_proc.run`), binary (`pe`: `versions.detect`/`build`/`_verify_symbols`, `locate/dissect.py`,
`locate/pass_points.py`, `locate/extract_upstream_tables.py`). The launcher carries the repository prologue (emitted by
`make_port.py`), so `versions.py` imports `tools.lib` in the launcher and inside gdb's embedded Python.

## Test contract

Tier: fixture - `tools/tests/mwcc-debugger/test_mwcc_debugger.py`: `make_port.py upstream/mwcc_debugger.py` regenerates
the committed port byte for byte (so a hand edit of `mwcc_debugger.py` fails; edit `make_port.py`), and
`versions.detect` identifies a `PeBuilder` image carrying the Wii/1.3 probe, takes the row without a CodeView blob,
refuses (SystemExit, naming the symbol) a blob that puts a row symbol elsewhere, and answers None for an unknown build,
a non-i386 image and a missing file. `locate/verify_pcode_selftest.py` stays as it is. Measured in WP5 on the live
tree: `versions.detect` over the 30 `build/compilers/*/*/mwcceppc.exe`, `dissect.py syms|dis`, `pass_points.py` and
`extract_upstream_tables.py` print the same as before, and a real gdb session (Wii/1.3, `-O4,p`) writes 41 dump files
identical to the old port's.

## Known gaps

needs a native gdb (`fetch_gdb.py`); `mwcc_debugger.py` and `verify_pcode*.py` still carry their own `sys.path`
insert beside the prologue (the hyphenated directory is not importable as a package), so they stay on the prologue
pending list.

## Moved from the module docstring (WP6)

From `tools/mwcc-debugger/fetch_gdb.py`:

Why this exists: on Windows the mwcc-debugger port needs a *native* mingw-w64
gdb (it debugs the 32-bit mwcceppc.exe directly).  If you already have MSYS2,
`pacman -S mingw-w64-x86_64-gdb` is the short answer.  If you do not - or you
do not want to install one - this script does what pacman would do: it reads
the MSYS2 package database, walks the dependency graph and unpacks the packages
into a self-contained prefix.

    python fetch_gdb.py --dest ~/.local/tools/mwcc-dbg/prefix
    ~/.local/tools/mwcc-dbg/prefix/mingw64/bin/gdb.exe --version

Then point the debugger at it:

Requires `pip install zstandard` (used to unpack the .pkg.tar.zst files; a
system zstd is not assumed).

From `tools/mwcc-debugger/locate/dissect.py`:

Two jobs, both driven by the CodeView 'NB11' symbol blob that Metrowerks
embeds in mwcceppc.exe:

  syms  <exe> [regex]        list symbols (RVA, VA, name, section)
  dis   <exe> <start> <end>  disassemble an address range, annotating call
                             targets with their symbol names

Addresses on the command line may be hex RVAs (default) or VAs if prefixed
with 'va:'.  The image base is read from the PE header.

From `tools/mwcc-debugger/locate/extract_upstream_tables.py`:

Upstream stores absolute VAs assuming a 0x400000 image base; we store
image-relative RVAs, so this script subtracts the ImageBase it reads from the
PE header of the compiler it is given (default 0x400000 for every MWCC PE we
have).

From `tools/mwcc-debugger/locate/pass_points.py`:

Every backend pass is a named function (the CodeView symbol blob names them)
called from one of two drivers:

  * `_globallyoptimizepcode` (the -O2/-O3/-O4 peephole + propagation pipeline)
  * `_CodeGen_Generator`      (the shared passes: initial code, regalloc,
                               prologue/epilogue, scheduling, peephole, ...)

A breakpoint at the *return address* of a `call <pass>` is exactly upstream's
"just after that flag is checked" point: the pass has finished and the global
`pcbasicblocks` block list is current, so `print_pcode` reads the right state.

Usage: python pass_points.py <exe> [rva:rva ...]

From `tools/mwcc-debugger/make_port.py`:

This is the one-shot transformation used to produce tools/mwcc-debugger/
mwcc_debugger.py from the vendored upstream copy in tools/mwcc-debugger/
upstream/mwcc_debugger.py.  It is kept so the port's diff is reproducible and
reviewable: run it and diff the result against the committed file.

Every replacement below is anchored on text that must be present, so a change
in the upstream copy makes this script fail loudly instead of silently
producing a wrong port.

From `tools/mwcc-debugger/versions.py`:

Upstream mwcc-debugger kept this data *inline in executable code*: a chain of
`if <ten bytes at a magic address> == b"Metrowerks":` tests, each building an
MwccVersion with absolute virtual addresses.  Adding a compiler build meant
editing the middle of the driver.  Here a build is a row: the driver
(mwcc_debugger.py) only ever reads fields off MwccVersion.

Two address conventions
-----------------------
Every address stored here is IMAGE-RELATIVE (RVA) and gets `image_base` added
at use time.  Upstream stored absolute VAs that silently assumed the image
loads at 0x400000.  Every mwcceppc.exe in this repository does (their PE
DllCharacteristics is 0, i.e. no DYNAMIC_BASE), and mwcc_debugger.py checks
the MZ signature at image_base before trusting the table.

Adding a build
--------------
1. Derive the addresses.  locate/README.md records how the Wii/1.3 row was
   derived, and locate/dissect.py reads a Metrowerks compiler's own CodeView
   symbol blob (which several Wii builds ship) plus disassembles it, so the
   data-symbol half of a new row can be resolved by *name* instead of by hand.
2. Add a `detect` probe (an RVA plus the bytes that must be there) and a row.
3. Nothing else changes.

Field reference
---------------
addresses:
  codegen_start_addr / codegen_end_addr   entry-ish and exit point of the
      per-function driver (`CodeGen_Generator`); the start must be *after* the
      current-function global has been stored.
  gfunction_addr                          global holding the current object
      (None on builds where the object is only reachable from the stack).
  cmangler_getlinkname_addr               `CMangler_GetLinkName(void*)`.
  nodenames_addr / nodenames_size         AST node-type name table.
  ast_breakpoints                         {rva: pass name}; at each, [esp] is
      the statement-list pointer.
  opcodeinfo_addr / _size / _stride       PCode opcode table.
  pcbasicblocks_addr                      global head of the block list.
  pcode_breakpoints                       {rva: pass name}; at each, the
      block list is current.
  regalloc_breakpoint_addr                where a colouring pass has finished.
  interferencegraph_addr                  global interference-graph base.
  used_virtual_registers_gpr_addr / _fpr_addr   per-class virtual-register
      counters (read as s16).
  coloring_class_addr                     u8 global naming the class being
      coloured (None on builds that pass it on the stack).
  arguments_addr / locals_addr / temps_addr / frame_base_size_addr /
  frame_call_args_size_addr               variable-frame dump inputs
      (GC/1.1 only; see supports_variables).
layout:
  "gc11" | "gc26" | "wii13" - selects the record decoders in mwcc_debugger.py.
  The Wii/1.3 build is *not* the 2.6-generation layout: its object, PCode,
  block and interference-graph records are all different (bigger object,
  0x0E-byte PCode operands, 0x20-byte IG nodes in a flat array).
flags:
  linkname_needs_call   whether `CMangler_GetLinkName` may be *called* to force
      the mangled name into existence.  True for the GC builds; False for
      Wii/1.3, where calling it from the codegen_start breakpoint aborts the
      compiler with an internal error.
  supports_ast          AST dumps (frontend record layout derived).
  supports_variables    variables.txt.
  regalloc_assigned_stack  `[esp + n]` holding the assigned-node list at the
      regalloc breakpoint.
