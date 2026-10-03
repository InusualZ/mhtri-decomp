# `vtslot` - Reverse of `vtableaudit --at`: every DOL data word equal to an address, its owning range, the code-pointer run and RTTI base around it

<!-- generated from the module docstring of `tools/units/vtslot.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

The reverse of `vtableaudit --at`: given a function address, find the `.data` table that holds it.

## Users

no caller in the tracked tree

## CLI

```
python tools/units/vtslot.py 0x803CDFC0                  # -> socket table 0x805F95E0, slot 0x24
python tools/units/vtslot.py 0x803CD854 0x803CF7A8       # a slot and a plain function (no hits)
python tools/units/vtslot.py --json 0x803CD854
python tools/units/vtslot.py 0x803CD854 0x803CDFC0 0x803CF7A8
python tools/units/vtslot.py --scan 0x805F9510 0x805FA908 # bounded band, report every pointer to it
python tools/units/vtslot.py --selftest
```
Flags: `--json`, `--main`, `--scan`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: DOL, splits, map -> hits.

## Invariants and rules

* `vtableaudit --at <table>` goes table -> slots. Lanes kept re-deriving the other direction by hand - the `network_transport` lane named 15 of 19 rows in ten minutes by packing a function's address big-endian and scanning the DOL's data sections for it, because the hit *is* the table slot and therefore names the class and the vtable offset. Its evidence is `MAIN/.pi/notes/net-transport-eeda.md`:
```
the .data run 0x805F94E0..0x805FA908 is TWO vtables with one log string between them, not five
interleaved tables: 0x805F9510 = NetworkPeerBuffer, 0x805F95E0 = NetworkPeerSocket.
```
* This is that hand recipe as a tool, for one or more addresses. For each address it reports **every 4-byte location in the DOL's loaded data sections whose value equals the address** - a vtable slot, a jump-table entry, an extabindex entry and a plain pointer in a struct all surface the same way - each labelled by the registered range that contains it (which unit, which section) or as an unregistered band. When the location sits inside a run of consecutive code pointers, it prints the table's base, the slot index and the byte offset, and the neighbouring slots' targets with their owning units and map symbols - the adjacency that is what makes a class name guessable.
* The table -> slots half is **not** re-implemented here: the run's slots are enumerated with `vtableaudit.vtable_slots` (the same function `--at` uses), and the owner of each target comes from `vtableaudit.owner_at`. The unit spelling goes through `claims.norm_unit`, the one definition every tool shares. The only new reader is the 4-byte big-endian word scan itself.
* What it does, in the order the report reads it:
* **every word hit.** A 4-byte, 4-aligned word in a DOL *data* section whose value equals the address. `.text` is not scanned: an address appearing there is an instruction immediate (the `lis`/`addi` pair's split halves never form the full aligned word), not a pointer.
* **where the hit is.** The registered split range that covers it, with the unit normalised through `claims.norm_unit`, or `unregistered` when no range owns the band (the state the transport unit's `.data` is still in - it is a config *request*, not a claim).
* **the run it sits in.** A maximal run of consecutive words that are code pointers (addresses inside the DOL's `.text`). One word is an ordinary pointer, not a table; `vtableaudit.MIN_RUN_WORDS` (2) is the same threshold `--at` uses. A run is bounded by the DOL data section, by the registered range when the hit is in one, and by a hard word cap; a run clipped by the cap says so (`truncated`) instead of guessing.
* **the table base.** A Metrowerks/Itanium vtable is two RTTI words (offset-to-top and typeinfo) followed by the function pointers; when those two words immediately precede the run and are zero, the base is reported *including* them, which is the base the transport lane labelled its slots from (`networkPeer_init` is slot 0x24 of the socket table at 0x805F95E0, `networkPeer_setInfo` slot 0x0C). The map symbol at the base (`lbl_805F95E0`) is reported when `symbols.txt` names it.
* Read-only by construction: no `ninja`, no compile, no link, no write anywhere. The DOL header, sections and maps are read through `vtableaudit`'s readers (`dol_segments`/`dol_read`/`dol_text_ranges`) and `vtableaudit.load_tree`, never a second parser.
* `--scan` flips the question to "what points INTO this band": every 4-aligned code pointer whose target lands in `[start, end)` is reported with the same per-hit labelling, which names a whole table's callers at once. `--main` points at a tree holding `orig/RMHE08/sys/main.dol` and `config/RMHE08/` (default: this file's tree).

## Lib dependencies

binary.dol, project, lanes.naming.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/vtslot_selftest.py`): The two acceptance cases are pinned here as fixtures, because they are the two ways this tool can lie: * **a vtable slot is found** - the word equals the function's address, it sits in a run of code pointers, the two RTTI words before the run give the table base the transport lane numbered its slots from, and the neighbouring slots resolve to their owning unit and map symbol; * **a plain function reports no hits** - not a false positive from an unaligned overlap, not a `.text` instruction immediate, not a data pointer mislabelled as a code-pointer run. The fixtures are a hand-built DOL blob (one `.text` section, one `.data` section) and a hand-written `splits.txt`/`symbols.txt`; no real `src/`, `build/`, `config/` or DOL is read or written. The end-to-end CLI check runs the tool against a temp tree and confirms every fixture file is byte-identical afterwards.
Target: `tools/tests/units/test_vtslot.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
