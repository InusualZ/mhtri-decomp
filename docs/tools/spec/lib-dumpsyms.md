# `lib.dumpsyms` - The runtime dump's Dolphin symbol map: parse it and index it by address

## Purpose

Reads the `.map` member of the shared runtime dump's `DumpSymbols.zip` (`docs/memory-dump.md`) into entries and an
address index, and tells a real name from a placeholder (`zz_`, `FUN_`, a Ghidra auto-name). One reader for every tool
that compares a name with the dump's.

## Users

`tools/symbols/dumpmap.py` (its command line), `tools/units/lanecheck.py` (the GUESS check, by address).

## CLI

None (library).

## Inputs and outputs

`load_dump(path=DEFAULT_DUMP, member=None)` reads the zip (`$MHTRI_DUMP_SYMBOLS` or `D:/WiiExperiment/DumpSymbols.zip`)
and returns `(index, member)`; it raises SystemExit naming the path or the member when either is missing. Writes nothing.

## Invariants and rules

* `parse_map_text(text) -> [entry]`: `{name, address, flag, lineno, mangled, placeholder, clean, signature}`; a line may
  carry two entries (the dumper lost a newline), `ENTRY_RE` takes both.
* `index_by_address(entries) -> {address: [entry]}`: the dump is joined by ADDRESS, never by name (a class-qualified
  name in the dump and its mangled map spelling never compare equal as text).
* `base_name(name)`: the name without the demangled argument list and the `__F` suffix.
* `is_placeholder(name, address)`: `zz_`/`FUN_`/a Ghidra `<kind>_<addr>` prefix, or an address tail that is the entry's
  own address.

## Lib dependencies

None (standard library only).

## Test contract

Tier: fixture, through `tools/symbols/dumpmap.py --selftest` (66 checks: the parser, the placeholder rule, the join and
the lookup on an in-memory zip) and `tools/tests/units/test_lanecheck.py` (the GUESS check with an injected index).

## Known gaps

* The default path is one machine's; a tree without the dump answers the GUESS check "not checked".
