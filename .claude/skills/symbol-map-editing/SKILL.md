---
name: symbol-map-editing
description: Query and surgically edit a decomp-toolkit symbol map (config/RMHE08/symbols.txt and the per-module config/RMHE08/<module>/symbols.txt) without loading the file into context - name/address/range lookup, rename with reference checking, duplicate checks - through tools/symbols/symedit.py. Use when a unit needs a symbol renamed to its real name, when the symbols of an address or split range must be listed, or when a rename's other half (the references) must be found.
license: MIT
compatibility: decomp-toolkit project layout; map lines look like `name = section:0xADDR; // type:... size:...`
metadata:
  author: mhtri-dtk
  tool: tools/symbols/symedit.py
---

# Editing the symbol map without reading it

`config/RMHE08/symbols.txt` is ~65 700 lines / 4.5 MB, and the per-module RSO maps add 4 462 more lines.
Never paste it into a prompt, never read it whole, and never regenerate it to make a small change.
`tools/symbols/symedit.py` is the proxy: it prints only the lines you asked for, and it changes only the
name token you asked it to change.

## Commands

| goal | command |
| --- | --- |
| what is this symbol? | `python tools/symbols/symedit.py show <name> [<name> ...]` |
| what is called like X? | `python tools/symbols/symedit.py find "<regex>" [--section .text] [--type function] [--limit N]` |
| what is around this address? | `python tools/symbols/symedit.py at 0x804DA598 [--count N] [--section .text]` |
| what is inside this split range? | `python tools/symbols/symedit.py range 0x804D9B4C 0x804DAE40 [--section .text]` |
| where is this name referenced? | `python tools/symbols/symedit.py refs <name> [--roots src include docs] [--code-only]` |
| is the map sane? | `python tools/symbols/symedit.py check` |
| rename one symbol | `python tools/symbols/symedit.py rename <old> <new> [--dry-run] [--force] [--no-refs]` |
| rename many | `python tools/symbols/symedit.py rename-batch map.txt [--dry-run]` (lines of `old new`) |
| merge phantoms (7.9) | `python tools/symbols/symedit.py merge-batch batch.txt [--dry-run]` (lines of `merge <phantom> <previous> <size_hex>`) |
| a per-module RSO map | add `--file config/RMHE08/<module>/symbols.txt` |

`--json` gives machine-readable output, and every option works both before and after the subcommand.
All output is bounded by `--limit` (default 40) and says how many more there are, so a lookup can never
flood the context.

## Renaming: the rules that matter

1. **A rename is two edits in one change**: the map (it names the *target* object) and the source that
   defines or references the symbol. Rename only the map and objdiff stops matching the symbol by name and
   reports it as 0 %.
2. **Find the other half first**: `refs <name>` lists every in-repo mention, **classified** into `code` /
   `path` / `mention` - under **`src/` and `include/` by default**; `docs/` and anything under `tools/`
   only when named in `--roots`. So a rename does **not** sweep build-tool data:
   `tools/units/attribution-queue.json` (a regenerable cache keyed by `symbols_sha1`) keeps its own name
   strings, and the map plus `--roots` is the authority there, not the rows in it. `rename` runs the same
   scan and prints it after writing. **Use
   `--code-only` for any scripted rewrite.** The classification is load-bearing because a map name is also a
   *file* name whenever a unit is registered under a generated path, and the scan matches with `\b`, so `/`
   and `.` are word boundaries: `#include "DWCi/fn_805113B0.h"` is reported as a reference to `fn_805113B0`.
   Rewriting it corrupts the include - the file on disk keeps its name, and renaming a unit's *file* is a
   **registration move**, not a symbol rename - and only a build would notice. The degenerate case is real: a
   name can have **zero** code references and several path mentions, where a naive "rewrite every reference"
   changes nothing real and breaks three files.
3. **Dry-run, then apply.** The tool refuses when the old name is not defined exactly once or the new name
   is already taken (`--force` overrides the latter), writes atomically, preserves the file's line endings,
   and prints just the one line it changed - that output *is* the diff to quote in the report.
4. **Verify after**: `python .claude/skills/mwcc-unit-matching/scripts/mt.py diff -u <unit> <new-name>` must
   show the same match as before, and `ninja build/RMHE08/main.dol` must keep the same hash.
5. **Never regenerate the map for a rename.** It is hand-editable; a regeneration (`ninja apply`) brings the
   generated names back and silently loses every documented rename.
6. **Choosing the name is its own judgement call** - the real name when it is known, otherwise one **derived from
   context** that fits the surrounding naming scheme. A guess is licensed and marked in the unit header; a
   generated `fn_xxxxxxxx` left in `src/` is a defect. The rules are in `CLAUDE.md` -> Conventions ->
   "Commenting and naming", and `docs/memory-dump.md` is where real names come from.

## Merging phantom symbols (roadmap 7.9)

A **phantom** is an unnamed `fn_*` that is really the previous function's dead epilogue, so a merge grows
the previous symbol's `size:` and deletes the phantom's line. `tools/symbols/phantom.py` finds them;
`merge-batch` applies them. Per row it refuses - before any write - unless both symbols are defined
exactly once, in the same section, the previous ends exactly at the phantom's address, no other name sits
at that address, the stated size is exactly the two sizes added, the two scopes agree, and the phantom has
no in-repo reference. An already-merged row is a no-op. A refusal for an absent previous names the symbol
that actually ends at the phantom's address, so a **stale row after a rename pass** is obvious (the
previous symbol's real name changed) rather than silently skipped. A merge is a `splits.txt` dirty-check
input, so it rides the same re-split as the renames (docs/plan.md §4 item 4).

## Reading the map

* Prefer `find`/`at`/`range` over `grep`: they parse the format, so they can filter by section and type,
  and they print a fixed-width table instead of raw lines.
* `check` reports duplicate names and unparsed lines. Three duplicate names are **pre-existing** in this
  project's map (`@stringBase0` x3, `ShutdownFunctionInfo` x2, `BootInfo` x2 - dtk synthesizes the same name
  for different units' data). Do not "fix" them without knowing what references them.
* Address alias groups (two names on one address, e.g. `_dtors` and `__destroy_global_chain_reference`) are
  normal and are reported as information, not as a failure.
* The map is not the only name source: `docs/memory-dump.md` (the shared Ghidra runtime dump) has real SDK
  names and signatures for symbols this map still calls `fn_xxxxxxxx`.
