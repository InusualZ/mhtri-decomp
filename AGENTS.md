# AGENTS.md

Guidance for AI agents (and humans) working in this repository.

## What this repository is

A **matching decompilation of Monster Hunter Tri** (Nintendo Wii, USA, disc ID **`RMHE08`**), built on the
[decomp-toolkit](https://github.com/encounter/decomp-toolkit) project template ([`encounter/dtk-template`](https://github.com/encounter/dtk-template)).

"Success" here means one specific thing: **the C/C++ source in `src/` recompiles to code that links into a
`main.dol` byte-identical to the original game**. It is not enough for code to compile, look correct, or
produce the same output — it must match the original instructions, relocations and section layout.

The original binary is split into relocatable objects by decomp-toolkit (no hand-written assembly, no game
assets in the repo), and the final `main.dol` is verified against `config/RMHE08/build.sha1`.

* Original binary: `orig/RMHE08/sys/main.dol` — SHA-1 `BF4850739478CAAEDFE675949EB7C28595A7FDE9`
* Current state: `src/Camellia/camellia.c` is the only matching (linked) unit, plus two non-matching
  `Runtime.PPCEABI.H` stubs. Everything else is still unsplit.

## Non-negotiables

1. **Never modify `orig/RMHE08/**`.** It is the original game data and the ground truth for every diff.
   Read-only, always.
2. **Never commit build output or original files.** `build/`, `orig/RMHE08/**` (except `.gitkeep`),
   `*.dol`, `*.rel`, `*.elf`, `*.o`, `*.map`, `objdiff.json` and `compile_commands.json` are gitignored —
   keep it that way.
3. **Do not edit `configure.py` compiler flags, `mw_version` values or tool version tags to make something
   build.** Those settings change codegen for every translation unit. Changing them is only acceptable with
   concrete evidence (an instruction/size diff that points at the flag), and must be called out explicitly.
   See "Gotchas" for the incident that motivates this rule.
4. **Only mark an object `Object(Matching, ...)` when it actually matches.** Otherwise use `NonMatching`.
   A wrong `Matching` flag breaks the final DOL hash for everyone.
5. **Don't rename or delete symbols that already exist in `config/RMHE08/symbols.txt`** unless you have
   verified nothing else depends on them. Symbol names are referenced by `splits.txt`, the linker script
   and the analysis output.
6. **Never commit or push without explicit approval.** Do not run `git commit`, `git push`, or anything
   that rewrites history (`rebase`, `commit --amend`, `reset --hard`, force-push, deleting/moving tags)
   unless the user's instruction for the current task explicitly says to do it. Finishing the work is not
   approval to commit it — leave the changes in the working tree and report exactly what you changed and
   what you would commit. `origin` is the *upstream template* (`encounter/dtk-template`), not a fork of
   this project, so work stays on the local `main` branch and is never pushed there.
7. **Never paste `config/RMHE08/symbols.txt` into a prompt/tool output.** It is ~65,700 lines / 4.5 MB.
   Grep it, slice it, or use `dtk`/objdiff; do not print it.

## Repository layout

```
configure.py              Project config + build generator (compiler flags, libs, tool versions)
config/RMHE08/config.yml  Analyzer/build settings, DOL path + hash, selfile (RSO list)
config/RMHE08/symbols.txt Symbol map: name = section:address; // type/size/scope  (generated, hand-editable)
config/RMHE08/splits.txt  Which address ranges belong to which translation unit / section
config/RMHE08/build.sha1  SHA-1 of each built artifact — the pass/fail check for the whole project
src/                      Our C/C++ source (currently Camellia/)
include/                  Our headers (does not exist yet; `cflags` already get `-i include`)
orig/RMHE08/              Original game files (read-only, gitignored). main.dol, files/mh3.sel, ...
build/                    Everything generated: build.ninja, compilers/, tools/, RMHE08/ (gitignored)
tools/                    Shared dtk-template scripts (project.py, download_tool.py, ...)
docs/                     Where all documentation lives — ours and dtk-template's. Anything worth
                          writing down goes here. Keep docs short and to the point, not dense.
```

Inside `build/RMHE08/`:

* `src/<Unit>.o` — **our** compiled object (the candidate; ninja target `build/RMHE08/src/...`)
* `obj/<Unit>.o` — the **original** object split out of the DOL (the target)
* `main.elf`, `main.dol`, `main.MAP`, `ldscript.lcf`, `report.json`, `progress.json`, `ok`

objdiff compares the `src/` candidate against the `obj/` target. Keeping those two straight is essential.

## Build & verify

Windows-friendly (this is the supported setup here): Python 3.12 + `ninja` on `PATH`. The toolchain
(binutils, Metrowerks compilers, dtk, objdiff-cli, sjiswrap) is downloaded automatically into `build/`.

```sh
python configure.py            # regenerate build.ninja + objdiff.json (needed after editing configure.py)
ninja                          # default target: build/RMHE08/progress.json (builds + verifies)
ninja build/RMHE08/ok          # build main.dol and check it against config/RMHE08/build.sha1  <-- the real test
ninja build/RMHE08/src/Camellia/camellia.o   # compile a single translation unit
```

Other useful targets:

| Target | What it does |
| --- | --- |
| `ninja all_source` | Compile all configured source files (no link) |
| `ninja build/RMHE08/report.json` | objdiff report for all units (progress + per-function diffs) |
| `ninja diff` | `dtk dol diff` — report symbols in the linked ELF that don't match |
| `ninja apply` | `dtk dol apply` — apply symbol/label info from the linked ELF back into `symbols.txt` |
| `ninja baseline` | Save the current report as a baseline for regression checks |
| `ninja changes` / `ninja changes_all` | Compare against the baseline (markdown: `regressions.md`) |
| `ninja tools` | (Re-)download the pinned toolchain |

Notes:

* `python configure.py --warn all|error` adds compiler warnings; the committed default has no `-W` flag.
  `--non-matching` builds "equivalent" code for extra units without linking them.
* On non-Windows hosts a `--wrapper` (wibo/wine) is required to run the Metrowerks compilers; on Windows
  they run natively.
* `mwcc_sjis` (sjiswrap) wraps the compiler, so keep source files UTF-8 with no BOM.
* If results look impossible, the build tree is probably stale: `rm -rf build/RMHE08` then
  `python configure.py && ninja`.

## The core loop: adding / matching a translation unit

1. **Find the unit.** Locate the function in `config/RMHE08/symbols.txt` (`grep`), get its address and size,
   and find the surrounding section ranges in `config/RMHE08/splits.txt`.
2. **Register it** in `config.libs` in `configure.py`: pick the right `mw_version` (this is a Wii title:
   `Wii/1.0` for REL-type code, `Wii/1.3` for runtime-style code — **not** the GC compilers) and an
   appropriate `cflags` group (`cflags_runtime` for runtime units, `cflags_base` otherwise).
   Start with `Object(NonMatching, "Dir/file.c")`.
3. **Create `src/Dir/file.c`** (and headers under `include/` if needed).
4. **Add the splits** to `config/RMHE08/splits.txt`: one line per section with exact `start:`/`end:`
   addresses, including the small `.ctors`/`.dtors`/`.sdata` fragments that runtime units own (Wii linkers
   use `.ctors$10`, `.dtors$10`, `.dtors$15` — see `docs/getting_started.md`, "GC 2.7+ and Wii linkers").
5. **Compile and diff:**
   `python configure.py && ninja build/RMHE08/src/Dir/file.o`, then produce/refresh the report and inspect
   the unit's per-function diff (objdiff GUI reads the generated `objdiff.json`).
6. **Flip to `Object(Matching, ...)`** only once the unit matches (bytes/instructions + relocations).
7. **Prove it end-to-end:** `ninja build/RMHE08/ok` must finish green, i.e. `main.dol` matches
   `config/RMHE08/build.sha1`.

Keep changes small and verified. A micro-optimization for a function that was already matching is a
regression if the hash goes red.

## Gotchas learned in this repo

* **Compiler-flag drift is silent and fatal.** An earlier revision of `cflags_base` had `-O4,p`,
  `-inline auto`, `-Cpp_exceptions off` and `-RTTI off` removed (and `-use_lmw_stmw on` commented out).
  Nothing failed to compile; the resulting `main.dol` was just wrong. `Camellia` compiled 0x3244 bytes too
  large, and per-function size deltas summed to **exactly the total DOL growth (+12,868 bytes)** — that
  sum-vs-total check is a great way to diagnose "everything is slightly bigger" symptoms.
* **Don't trust a single objdiff number.** `complete_code_percent: 100.0` has been observed alongside
  `fuzzy_match_percent: 1.77` for the same unit. Cross-check `fuzzy_match_percent`, the per-function diff,
  and ultimately `build.sha1` / `ninja build/RMHE08/ok`.
* **`mw_comment_version: 14`** in `config.yml` must match the `.comment` section of the original objects.
  A mismatch makes the analyzer mis-identify the toolchain.
* **`quick_analysis: false`** is required while function boundaries are still being discovered; setting it
  to `true` skips boundary analysis and is only valid after analysis is complete and `symbols.txt` /
  `splits.txt` are generated.
* Sizes/addresses in `symbols.txt` and `splits.txt` are absolute addresses from the **unlinked** original
  DOL; they are not offsets, and section order matters (`.init`, `extab`, `extabindex`, `.text`, ...).
* Stale `build/` output causes false conclusions (an old object from different flags can look "matching").
  Prefer a clean rebuild of the specific unit, and `rm -rf build/RMHE08` when in doubt.
* Local agent scratch directories (`.pi/`, `.lavish/`, `.agents/`, `openspec/`) are gitignored; keep them
  that way and never add their contents to commits.

## Conventions

* **Commit messages** (only once a commit has been approved — see Non-negotiables rule 6): short imperative
  subject, area-prefixed, e.g. `Camellia: match Camellia_Ekeygen`, `RMHE08: refresh symbols.txt`,
  `configure.py: add REL flags`. Describe *why* when fixing a mismatch.
* **Keep generated/large churn separate.** A `symbols.txt` regeneration or an analyzer settings change gets
  its own commit; never mix it with source changes or unrelated formatting.
* **Naming:** use the real name when it's known from the original binary/symbol map; leave dtk's generated
  `FUN_xxxxxxxx` names in place until they're understood. Vendor files keep vendor naming
  (e.g. `Camellia/` uses `CAMELLIA_*` constants and its original MPL-1.1 header — keep those intact).
* **Style:** match the file you're editing (vendor sources mirror upstream formatting; new project code
  follows the surrounding 4-space-indent C style). Files are UTF-8, LF endings (`.gitattributes`
  enforces the checkout).
* **Documentation:** `docs/` is the home for all documentation — put new knowledge there instead of
  leaving it in chat, commit messages or code comments. Write straight to the point: setup steps, recipes
  and findings as short bullets, not dense prose or oversized files. Split into one file per topic rather
  than growing a single wall of text. dtk-template docs already in `docs/` stay authoritative for template
  behaviour; add project-specific notes alongside them instead of rewriting them.

## Before claiming success

* [ ] `ninja build/RMHE08/ok` passes (for anything affecting the linked DOL), or the change is explicitly
      described as unverified.
* [ ] For a single unit: the object compiled **and** its objdiff diff shows the claimed match level.
* [ ] `git status --short` shows only intended files (no `build/`, no `orig/`, no scratch dirs).
* [ ] `symbols.txt` / `splits.txt` edits are byte-clean for the lines you didn't mean to touch
      (`git diff --stat` sanity check — these files are huge).
* [ ] No new compiler flags / tool version changes smuggled in.
* [ ] Nothing was committed or pushed unless the user asked for it (see Non-negotiables rule 6); staged vs.
      unstaged state reported clearly.
