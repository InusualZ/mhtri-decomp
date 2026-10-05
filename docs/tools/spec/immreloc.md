# `immreloc` - Find immediates dtk relocated against a code or unwind label and print the `block_relocations` entries

## Purpose

Lists every `lis`/`addi` (or lone `@l`/`@h`) half in a unit's split target object that dtk relocated against a code
label with an addend (`fn_8005FE68+0x1CC`) or an unwind-table label (`@eti_80030018+0xA`, `@etb_8000FFFC+0x5`), says
whether our compiled object builds the same value as a plain immediate, and prints the ready-to-paste
`block_relocations` entries that keep the value a constant. It applies nothing.

## Users

The orchestrator and fixer lanes facing a function whose only residual is a relocation on a constant (the network pilot
found 0x803D7564..756C and the extabindex ranges by hand); `integrate.py`'s `config` requests carry its entries.

## CLI

```
python tools/objdiff/immreloc.py                     # every target object of the tree
python tools/objdiff/immreloc.py -u Network/NetworkSocketWii [-u ...]
python tools/objdiff/immreloc.py --unblocked         # a scratch split without block_relocations (what the entries hide)
python tools/objdiff/immreloc.py --obj-dir DIR [--ours-dir DIR] [--all-code] [--json] [--root DIR]
```
Exit codes: 0 no proposed site outside config.yml's entries, 1 at least one (the findings), 2 no target objects.
`--json`: `{tool, obj_dir, objects, summary: {candidates, covered, new, new_proposed, new_review, entries: {entry: n}},
rows: [{unit, function, section, start, end, target, target_section, addend, value, halves, shape, ours, ours_immediate,
covered_by, score, proposed, entry}], unreadable, ok}`.

## Inputs and outputs

Reads `build/RMHE08/obj/**.o` (or `--obj-dir`), our `build/RMHE08/src/**.o`, `config/RMHE08/symbols.txt` (label and
function addresses), `config/RMHE08/config.yml` (`block_relocations`) and `build/RMHE08/report.json` (function scores by
address). `--unblocked` writes `build/tmp/immreloc/unblocked.yml` (config.yml without `block_relocations`, `write_asm`
off) and the split under `build/tmp/immreloc/unblocked/`; nothing else is written.

## Invariants and rules

* **What is a candidate**: an `R_PPC_ADDR16_HA`/`_HI`/`_LO` in `.text`/`.init` whose label is code with a non-zero addend
  (a code label with none is a function pointer, reported only with `--all-code`) or any `extab`/`extabindex` label (code
  never takes an unwind table's address). Halves of one function, label and addend more than 0x10 bytes apart are separate
  sites: an entry never spans the code between two of them.
* **Proposed vs review**: a candidate is proposed (gets a paste-ready entry, and makes the exit 1) when its value has the
  error-code shape - high half 0x8001..0x801F, low half under 0x100 - or our object builds the same value with
  `lis`+`addi|ori` and no relocation on it. Any other (a real address such as `OSDisableInterrupts+0xC`,
  `_eti_init_info`) is listed as `review` with no entry.
* **Covered**: a `source` entry covers a site inside its range; a `target` entry covers a value inside its range, in the
  label's section. The summary counts each config entry's rediscoveries, so a stale entry reads 0.
* **Entries** are config.yml's `- source: .text:A` / `end: .text:B` form (B = after the last instruction); for unwind
  labels it also prints one `target:` entry per run of values (16-byte lines, merged), the form the extabindex entries use.
  `lib.repo.config_change` admits both (`block_relocations` is a relocation-analysis key).

## Lib dependencies

`lib.binary.elf` (the objects), `lib.objcompare` (`be32`, `reloc_rows` for our object), `lib.ppc` (`materialisations`),
`lib.project.symbols` (the map), `lib.report` (`address_rows`), `lib.repo` (`config_blocks`, the tree), `lib.names`.

## Test contract

Tier: fixture. `tools/tests/objdiff/test_immreloc.py` on `ElfBuilder` objects: a code label with an addend is found and
proposed (our immediate, the score by address, the entry text), a function pointer is not found, a real mid-function
address is review only, two unwind sites 0x20 apart are two candidates covered by a `target` entry, our object relocating
the same site is not immediate evidence, both entry forms parse, `--json` and the exit codes, the `target:` runs. Each of
five mutations (keep function pointers, no run split, ignore our relocation, ignore the target form, a loose shape) fails
at least one check.

## Known gaps

* Replay (2026-10-05, `--unblocked`): 136 candidates in 415 objects; config.yml's three entries are rediscovered by 4
  (extabindex 0x80020000..10), 102 (extabindex 0x80030000..50) and 1 (`.text` 0x803D7564..756C) of them. On the current
  split it finds 29 new: 24 proposed (0x80060034 in `NetworkCommunityPat`/`NetworkLayerPat`, 0x800A0004 x5 in
  `NetworkFileFetcher`, 0x80010001..15 x16 in `NetworkSocketWii` against extab, 0x80050038 in `PatInterface`) and 5 to
  review (`__AXOutInitDSP` twice - its `lis` and `addi` 0x40 apart -, `bta_dm_send_hci_reset`, `OSLoadContext`,
  `__init_cpp_exceptions`). None
  is applied: each wants the before/after measurement a config change gets. Cross-check: main's `b774e3ace` then landed
  21 entries a lane wrote by hand for `NetworkFileFetcher` and `NetworkSocketWii`; they are exactly 21 of the 24 proposed
  here (same ranges, `0x803F8118..8150` cut in two the same way), and on the merged tree the replay rediscovers all 24
  config entries (128 of 136 candidates covered) and leaves 3 proposed: 0x803F17F8 (`NetworkCommunityPat`), 0x803E30F4
  (`NetworkLayerPat`), 0x803FD784 (`PatInterface`).
* The scan reads the objects dtk wrote; MAIN's `build/RMHE08/obj` holds 423 objects where a fresh split writes 415 (dtk
  never deletes a stale object either), so a stale object can add a row until the tree is re-split.
* An `@l` whose `lis` is shared with other sites (one `lis`, several `addi`) is reported as a lone half; its entry blocks
  that one instruction, which is what dtk needs, but our-object evidence is then `no matching immediate`.
