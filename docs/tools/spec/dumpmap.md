# `dumpmap` - Resolve map names against the runtime dump's Dolphin symbol map: `lookup` one address, `join` for rename/confirm/conflict rows

<!-- generated from the module docstring of `tools/symbols/dumpmap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Resolve names in symbols.txt against the runtime dump's Dolphin symbol map.

## Users

configure.py / the build (9); profiles (`.claude/agents`) (2); skills (4); docs (19)

## CLI

```
dumpmap.py lookup 0x800406AC            # the dump's name/signature + what symbols.txt calls it
dumpmap.py lookup CntSdRsoTerminate     # by map name (or by dump name)
dumpmap.py join --limit 40              # rename candidates, confirmations, conflicts
dumpmap.py join --json --kind all
dumpmap.py --selftest
```
Flags: `--dump`, `--file`, `--json`, `--kind`, `--limit`, `--member`, `--section`, `--selftest`, `--stats-only`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: symbols.txt + DumpSymbols.zip -> rows.

## Invariants and rules

* docs/plan.md 7.7. `D:/WiiExperiment/DumpSymbols.zip` holds `Dump_Loading85.raw.map` - the loading-state dump's symbol table, 48 476 entries in Dolphin's map format (docs/memory-dump.md):
```
CntSdRsoTerminate 800406ac f     # name [demangled argument list] address flags; `f` = function
kbd_open(unsigned 80040798 f     # the demangled argument list is truncated at the first comma
zz_0040598_ 80040598 f           # `zz_<address>_` = the dumper had no name for it
```
* Two traps in the format: the file is missing the CRLF between 109 pairs of entries (the flag of one is glued to the name of the next, `... 800e8888 fgetInstance() 800e89d8 l`), and the same address can carry two names - the demangled one and the `__F`-mangled one (`all_reset(void)` and `all_reset__Fv`).
* It answers one question the Ghidra oracle answers only one function at a time: what does the dump call this address? Two subcommands:
* `join` walks symbols.txt and classifies every address the dump knows, in ascending order:
* `rename` - the dump has a real name and the map's name is generated (`fn_`/`lbl_`/`dtor_`/ `@etb_`/...) or differs from it. `confidence: high` when the dump's name can be spelled as a symbols.txt name (a plain identifier, or the `__F`-mangled form the map itself uses); `review` when the map's name is already a real name (it may be a deliberate rename) or the dump only has a demangled C++ name (`ns::Class::method`) that would have to be mangled by hand - `proposed_name` is then null and the row says why.
* `confirm` - the map's name is already right: the dump's real name normalises to it (so `kbd_init__FUc` vs `kbd_init(unsigned` confirms), or the dump only carries a `zz_`/address-embedded placeholder, which confirms the generated name stands.
* `conflict` - the dump's name is already used in the map by a different symbol, or the same dump name is proposed for two addresses. Never auto-rename one; the row names the holder.
* The dump is a name/signature oracle, never codegen evidence (docs/memory-dump.md), and its `zz_` means "unnamed", not "absent". `proposed_name` is a proposal, not a rename: the map is the ground truth and a rename is two edits (symbols.txt + the source) through `tools/symbols/symedit.py`.

## Lib dependencies

project.symbols, names.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/symbols/test_dumpmap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

needs the external `DumpSymbols.zip`
