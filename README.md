# Monster Hunter Tri (RMHE08) — matching decompilation

A matching decompilation of *Monster Hunter Tri* for the Nintendo Wii (USA, disc ID `RMHE08`), built on
[decomp-toolkit](https://github.com/encounter/decomp-toolkit)'s project template.

The goal is not code that behaves the same — it is **C/C++ in `src/` that recompiles to a `main.dol`
byte-identical to the original**. Instructions, relocations and section layout must all match. A unit
that is merely equivalent stays `NonMatching`, and the original bytes stay in the binary.

| | |
| --- | --- |
| Original executable | `orig/RMHE08/sys/main.dol` |
| Its SHA-1 (verify this first) | `bf4850739478caaedfe675949eb7c28595a7fde9` |
| The test that matters | `ninja build/RMHE08/ok` (checks `config/RMHE08/build.sha1`) |

## Setup

Requires **Python 3.12+** and **`ninja`** on `PATH`. Everything else — binutils, the Metrowerks
compilers, `dtk`, `objdiff-cli`, `sjiswrap` — is downloaded into `build/` on the first build. On
non-Windows hosts the Metrowerks compilers need a Win32 wrapper (`wibo`); on Windows they run
natively.

**The original game files are not in this repository** (they are copyrighted; `orig/` is gitignored).
Supply them from your own disc:

```
orig/RMHE08/sys/main.dol      # the original executable
orig/RMHE08/files/mh3.sel     # the RSO module list
```

Then verify the executable before anything else, because every diff in this project is measured
against it:

```sh
sha1sum orig/RMHE08/sys/main.dol
# bf4850739478caaedfe675949eb7c28595a7fde9
```

Keep a copy outside the repository. `orig/` is untracked, so **no git command can restore it**: never
run a repository-wide clean, and if the file goes missing the build stops with
`orig/RMHE08/sys/main.dol not found` — restore it from your copy.

## Build

```sh
python configure.py            # generate build.ninja + objdiff.json (required after editing configure.py)
ninja                          # build and verify everything (default target: progress.json)
ninja build/RMHE08/ok          # build main.dol and check it against config/RMHE08/build.sha1  <-- the real test
ninja build/RMHE08/src/Camellia/camellia.o      # compile one translation unit
```

| target | effect |
| --- | --- |
| `ninja all_source` | compile every configured source file, without linking |
| `ninja build/RMHE08/report.json` | objdiff report — the per-unit and per-function scores |
| `ninja diff` | `dtk dol diff`: symbols in the linked ELF that do not match |
| `ninja apply` | `dtk dol apply`: write symbol/label info from the linked ELF back into `symbols.txt` |
| `ninja baseline`, `ninja changes` | save the current report, then compare against it (`regressions.md`) |
| `ninja tools` | (re-)download the pinned toolchain |

Two traps worth knowing on day one:

* **A stale build tree produces impossible results.** When a score looks wrong, rebuild:
  `rm -rf build/RMHE08 && python configure.py && ninja`.
* **`report.json` is an order-only target.** After editing a source, `ninja build/RMHE08/report.json`
  may answer "no work to do" and serve the *previous* build's numbers. Delete it, or force the compile.

Inside `build/RMHE08/`, `src/<Unit>.o` is **our** object (the candidate) and `obj/<Unit>.o` is the
**original** one (the target). Keeping those two straight is essential.

## Making a unit match

A unit is one translation unit: a range of the original binary that we re-implement as one source
file and measure against its split object. The loop is: find the unit → register it in
`configure.py` + `config/RMHE08/splits.txt` → write the bodies → measure per symbol → record the
residual in the unit's own file header.

```sh
python tools/objdiff/symdiff.py -u <Unit>          # every symbol in the unit, with its score
python tools/units/stylelint.py --diff main        # the campaign's style rules (add-only)
python tools/units/undefrefs.py --unit <Unit>      # references our object makes that nothing defines
```

The ideas that actually turn a non-matching function into a matching one are collected in
[docs/matching/](docs/matching/README.md) (the playbook) and in `.claude/skills/mwcc-unit-matching/`.
Work it as a list, one idea at a time, and record what failed too.

## Running the project as a pipeline

Contributing a unit needs none of this. *Running* the campaign does: work is done by short-lived
worker lanes, each owning one unit in its own git worktree, and an orchestrator that gates and lands
one unit at a time. One page of it:

1. **A claim takes a slot.** Six slots exist as siblings of the repository (`../mhtri-dtk.slot1..6`).
   A slot holds a **directory, never a branch** — every claim cuts a fresh branch off `main`'s
   current tip. A free slot *is* the concurrency cap; with all six taken, the queue refuses rather
   than starting a seventh.
2. **The brief is the contract.** `python tools/units/queue.py next` writes
   `tools/units/briefs/<slug>.md` and prints a paste-ready launch for the profile the job needs:
   `decompiler` for unit work, `fixer` for a refused gate or a measured regression, `merger` for a
   refused apply. The brief carries the tree, the range, the rules and the acceptance list.
3. **The lane writes source and commits on its branch.** It writes nothing outside its slot, builds
   with its own seeded tree, and reports data — per-symbol scores, residuals — not prose.
4. **The orchestrator lands one unit at a time, from `main`.**
   ```sh
   python tools/units/land.py land --branch worker/<slug> --units <unit>
   ```
   The gate is ~28 rows: full build, the batch's own objects compiled, ledger delta, style lint,
   rule-10 vtable ownership, undefined-reference check, tool selftests, and the DOL hash. A refusal is
   a finding — every row names the remedy, and a row that is only bookkeeping says so.
5. **Refill the slot.** Landed work first, then a new claim; a slot is never refilled while finished
   work sits unlanded.

```sh
python tools/units/slots.py status     # the pool, and why each slot is or is not usable
python tools/units/claims.py status    # acked / working / done / unacked / stalled claims
python tools/units/backlog.py          # the open-item register and the claim/credit balance
python tools/units/flipcheck.py        # which non-matching units are ready to flip
python tools/units/ledger.py           # the ledgers progress rests on
```

The authoritative documents are:

| document | owns |
| --- | --- |
| [docs/pipeline.md](docs/pipeline.md) | how a batch is run: the three-phase loop, the slot model, the gate, the merge procedure, the tool roster |
| [docs/plan.md](docs/plan.md) | the campaign plan: what "done" means, what a batch costs, roles, the coordinator protocol |
| [docs/matching/](docs/matching/README.md) | the playbook, one file per idea ([index](docs/matching/index.md)): every idea that has matched a function, and every idea ruled out |
| [CLAUDE.md](CLAUDE.md) | the non-negotiables and the conventions, for agents and humans alike |
| [docs/tooling-requests.md](docs/tooling-requests.md) | the tooling register: what is missing, and who asked for it |

## Rules that will bite you

* **Never modify `orig/RMHE08/**`.** It is the ground truth for every diff, and it is untracked.
* **Never commit build output or original files** — `build/`, `orig/`, `*.dol`, `*.o`, `*.map`.
* **Never change compiler flags, `mw_version` or tool versions to make something build.** They change
  codegen for every translation unit; a change needs instruction-level evidence and an explicit note.
* **Only claim `Object(Matching, ...)` when the unit actually matches.** A wrong flag breaks the final
  DOL hash for everyone.
* **A rename is two edits**: the map (`config/RMHE08/symbols.txt`) and the source that uses the name,
  in one change, through `python tools/symbols/symedit.py rename`. A unit's own symbols are named from
  context — no `fn_XXXXXXXX`/`lbl_XXXXXXXX` definitions are left behind.
* **`CLAUDE.md` holds no live working state**: session state goes in `.pi/state.md` (gitignored).

## Repository layout

```
configure.py              project configuration and build generator (flags, libs, tool versions)
config/RMHE08/            symbols.txt (the symbol map), splits.txt (which ranges belong to which TU),
                          build.sha1 (the pass/fail check), config.yml
src/<module>/<name>.ext   our C/C++ — a unit is registered once, at its final home
include/                  shared headers: types.h, one header per owning unit, unsplit/ for unclaimed bands
orig/RMHE08/              the original game files (untracked — see Setup)
build/                    everything generated (untracked): build.ninja, compilers, tools, RMHE08/
tools/                    our tooling by purpose: units/, objdiff/, flags/, symbols/, splits/, elf/,
                          agents/, plus the decomp-toolkit scripts at the top level
docs/                     all documentation — the table above is the entry point
.claude/agents/           the project's subagent profiles
.claude/skills/           loadable skills: matching, verification, symbol map, TU discovery
```

## Progress

Progress is measured, never estimated. `dtk`'s `complete_code_percent` is deliberately not quoted
anywhere in this repository — it reports 100 % for wrong code. For the current state, ask the tools:
`python tools/units/ledger.py`, `python tools/units/backlog.py`, and `build/RMHE08/report.json`.

## Credits

Built on [decomp-toolkit](https://github.com/encounter/decomp-toolkit) and its
[project template](https://github.com/encounter/dtk-template), with
[objdiff](https://github.com/encounter/objdiff), [decomp.me](https://decomp.me),
[wibo](https://github.com/decompals/wibo) and [sjiswrap](https://github.com/encounter/sjiswrap).
Nearly all active GC/Wii decompilation projects share this structure; the community is on the
[GC/Wii Decompilation Discord](https://discord.gg/hKx3FJJgrV) (`#dtk` for tooling questions).
