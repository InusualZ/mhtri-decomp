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

proc, binary (pe).

## Test contract

Tier: fixture: `verify_pcode` dumps + an assembled object.
No selftest today.
Target: `tools/tests/mwcc-debugger/test_mwcc-debugger.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

needs a native gdb (`fetch_gdb.py`)
