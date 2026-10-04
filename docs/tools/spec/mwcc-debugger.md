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
