# Build performance

What a `ninja` run costs, what it is spent on, and the knobs that change it. Measured on this machine
(Windows, 32 logical CPUs, Python 3.12) from `.ninja_log` and file mtimes; the numbers below are the
clean samples, so treat them as ~±50 % under load.

## Re-measured 2026-10-04 (415 split objects)

The tables below were measured at **13 584 objects**; the split now writes **415** (the units were registered at their
homes) and every cost fell with it. From MAIN's `.ninja_log` and a timed run in a worktree:

| edge | then | 2026-10-04 |
| --- | --- | --- |
| `config.json` (`dtk dol split`, `write_asm: false`) | ~18 s | **1.8-1.9 s** |
| the same split with the asm (`dump_asm.py`, 410 `.s`) | 200-400 s | **3.4-3.6 s wall** (dtk 3.0 s) |
| `main.elf` (the link) | ~70-88 s | **0.5-0.6 s** |
| `report.json` | ~1.5-3.5 s | **1.1-1.2 s** |
| a full slot seed (`build/RMHE08`, asm excluded) | - | **0.71 s** (892 files, 42 MB); an incremental `slots.py refresh` 0.16 s |

So `write_asm: true` would now add ~1.5 s to a split, not 200-400 s; `config.yml` keeps `write_asm: false` (the owner's
2026-10-04 decision) and the tools refresh the dump themselves instead (`lib.artifacts`, `python tools/units/fresh.py
status`). The derived caches on top of the dump cost more than the dump: `tudiscover`'s graph 17 s cold, `callers`' index
11.8 s cold (and dtk rewrites every `.s`, so each dump invalidates the latter).

## What dominates

| edge | measured | what it is |
| --- | --- | --- |
| `build/RMHE08/config.json` | **~18 s** as configured; **200–400 s** with `write_asm: true` | `dtk dol split` (rule `split`) — analyse the DOL, then write 13.5 k objects (skipped when unchanged) and, when enabled, 13.5 k `.s` files |
| `build/RMHE08/main.elf` | **~70–88 s** | `mwldeppc` over **13 584 objects**, fed through `@main.elf.rsp` |
| `report.json` / `baseline.json` | ~1.5–3.5 s each | `objdiff-cli report generate` over every unit (`ok` drags them in via `|| post-build`) |
| one source unit | 0.2–24 s | `mwcceppc`; the worst single unit so far is `Pl/pl_skill.o` |
| `main.dol`, `ok` | < 0.5 s | `dtk elf2dol`, then the sha1 check |

Measured end-to-end after the two knobs below (same tree, `write_asm: false`):

| edit | cycle |
| --- | --- |
| `symbols.txt`/`splits.txt`/`config.yml` touched, `obj/` unchanged (a rename) | **19 s** — split 18 s, manifest reload, no relink |
| the same, plus any object changed | **~86–105 s** — split 18 s, then the link (~70–88 s) runs *after* it |
| one `src/` file | compile + link, ~70–88 s (no split) |

`.ninja_log` is the source of truth: it records every edge that ran, start/end in ms, across all runs.
The split is also a **dirty-check input of itself**: `ninja -t deps build/RMHE08/config.json` lists
`orig/RMHE08/sys/main.dol`, `config/RMHE08/symbols.txt`, `config/RMHE08/splits.txt` and the sel file, so
**any** edit to symbols/splits re-runs the whole split (and `symbols.txt` is edited constantly).

## Knob 1 — the asm dump is on demand (`write_asm: false`)

`build/RMHE08/asm/` is ~92 MB / 13.5 k `.s` files. Nothing in the build reads it (`config.asm_dir` is
`None`, and no ninja edge names it — `grep -c 'RMHE08\asm' build.ninja` → 0). The one reader is
`tools/splits/tudiscover.py`, so `config.yml` sets `write_asm: false` and the dump is produced when a
session needs it:

```sh
python tools/splits/dump_asm.py              # one full split (~200-400 s), then stamp it
python tools/splits/dump_asm.py --check      # is the dump still current? (exit 1 if not)
python tools/splits/tudiscover.py stats      # prints the dump's state, warns on stderr when stale
```

`dump_asm.py` runs dtk against a temporary copy of `config.yml` (never the repo's file), with
`--no-update` so the hand-edited `symbols.txt`/`splits.txt` are left alone, and writes
`build/RMHE08/asm/.stamp.json` — the sha1 of symbols/splits/DOL plus the file count. The stamp exists
because stale asm is *silent*: `asm_files()`'s docstring records a stale copy printing
`bl fn_80456DD4` where the canonical copy prints `bl _savegpr_14`, which zeroes a codegen fingerprint.

A `write_asm: false` split **does not clean** the dump (three of them left all 13,790 `.s` files in
place), so the on-demand dump can live in the standard `build/RMHE08/asm/` and `tudiscover` needs no
override — but a dump does outlive the map it was made from, which is what the stamp catches.

## Knob 2 — one re-split per batch, renames included

The split's cost is per **run**, not per symbol, so the batch should be as large as is safe:
registrations already work that way ("one re-split and one ledger check per batch", `land.py`), and renames/phantom merges belong in the same batch. Collect them with
`tools/symbols/symedit.py rename-batch <file>` and re-split once; verify every renamed symbol after that
split, before the commit. The trade is explicit: per-symbol objdiff verification moves to the batch
boundary.

## The pitfall — the objects are undeclared outputs of the split

The split edge declares only `config.json`; the 13 584 objects the link consumes are not ninja outputs
(`ninja -t targets all` = 66 while `build/RMHE08/obj/` holds 13.8 k files). `tools/project.py` therefore
puts `config.json` in every link step's **`order_only`** inputs (`LinkStep.build_config_path`). Without it,
ninja plans the link in the same invocation as the split and **links while the split is still rewriting**,
so `ok` can go green from a stale link and the next run relinks.

`order_only` and not `implicit` on purpose: `config.json` is rewritten on every split, so an implicit
dependency relinks (~84 s) after a split that changed no object at all - and a rename/merge split changes
no object (measured: `.o` mtimes untouched, dtk skips identical writes). Ordering is what is needed, and it
holds at two levels: `build.ninja` itself depends on `config.json` (so a split always precedes the manifest
reload), and the link's order-only dep covers the case where the manifest was already fresh. The link's
*dirtiness* comes from the objects: a split that changed one relinks, one that did not, does not.

If you see a link that started at ~0 s in the same run as a split, that dependency is missing.

## Recipes

```sh
ninja -t query build/RMHE08/main.elf        # inputs (13 584), implicit deps (the split must be listed)
ninja -t deps  build/RMHE08/config.json     # what dirties the split
ninja -t targets all | wc -l                # 66 — the objects are not targets
ninja -n build/RMHE08/ok                    # what a run would do, without doing it
```

To read a past run out of `.ninja_log`:

```python
for l in open(".ninja_log"):        # start_ms, end_ms, mtime, output, hash
    p = l.rstrip().split("\t")
    if len(p) >= 4 and p[3].endswith("config.json"):
        print(int(p[1]) - int(p[0]), "ms", p[3])
```

## Upstream

`dtk` v1.8.3 (the pinned tag in `configure.py`) has no analysis cache and no "skip unchanged asm": every
split re-analyses the DOL from scratch and rewrites every `.s`. Worth asking upstream for (a) skipping
unchanged asm writes, (b) an analysis cache keyed on the DOL + `splits.txt`, and (c) declaring its object
outputs so build systems can order the edges.
