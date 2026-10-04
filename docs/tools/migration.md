# Migration: the ordered work packages

The strategy is **extract-and-delegate, one concept at a time**: a lib module is written with its contract test, each old
implementation becomes a one-line delegate to it (its own selftest still green), then the delegates are deleted when nothing
imports them. Old entry points become shims first and are removed last (WP6). Every CLI the docs name keeps working at every
step, and the gate lands every package.

## Acceptance for every package (no exceptions)

1. `python tools/selftest.py` green (fixture tier and smoke tier), park list unchanged or shrunk; suite time recorded before/after.
2. `python tools/units/land.py verify --dry-run --no-outbox --units <the package's files>` on the package's own batch: every row PASS.
3. The landing gate lands a **no-op batch** (an empty-diff `verify` on `main`) after the package: proves the gate itself still runs.
4. A **real trivial landing** (one unit's header comment, or one doc line) through `land.py land` after the package lands.
5. `python tools/tests/smoke/test_cli_compat.py`: every invocation in the compatibility list below parses (`--help`) or dry-runs.
6. The package's "references to update" list (below) is applied in the same batch; `grep -rn` for each retired name under
   `.claude/`, `docs/`, `CLAUDE.md` returns only `docs/tools/retired.md`.
7. The in-code headers of every touched tool follow the template; `tests/lib/test_headers.py` passes.
8. Measured in the batch's commit body: lines removed, lines added, number of implementations collapsed (from `duplication.md`).

## The packages

### WP0 - the skeleton (M, blocking)

* `tools/lib/__init__.py`; the one-line prologue and its lint (`tests/lib/test_prologue.py`: every `tools/**/*.py` entry point has
  exactly the canonical line, no other `sys.path.insert`); `tests/lib/test_layering.py` (lib never imports tools; tool->tool only
  from an allow-list that shrinks per package).
* `lib/testing.py` (`Checker`, `FixtureTree`, `GitFixture`, tiers) with its own test.
* `tools/selftest.py`: discovery of `tools/tests/**/test_*.py` added **beside** the existing discovery (both run; an entry is
  deduped by key when a tool has both); `--tier`; one count regex for `Checker` output.
* `tests/lib/test_headers.py` (template check) - advisory until WP6, then refusing.
* No tool changes. Acceptance: items 1, 3, 5 (the compat test is created here from the list below).

### WP1 - the leaf lib (three lanes in parallel, each M)

* **1a** `lib/repo.py`, `lib/proc.py`, `lib/git.py`, `lib/text.py`, `lib/cache.py`, `lib/names.py`. Delegates: `unitutil.repo_root/
  main_tree/resolve_input/session_tmpdir`, `subproc.run`, `spawnretry`, the 16 `git()` wrappers, `edit.replace_bytes`,
  `sharedfiles.Transaction/line_ending`, `escape.atomic_write`, `callers.dump_signature`, `tudiscover.dump_stamp`, `undefrefs._sig`,
  `relocaudit/undefrefs.linkage_stem`, `callees/dumpmap.is_generated`.
* **1b** `lib/binary/`. Delegates: the 26 ELF readers, 10 DOL readers, `elfsect.sections`, `objalign/objextab.read_sections`
  (+ their writers), `dwarfmap`; the 13 + 7 fixture builders are replaced by `ElfBuilder`/`DolBuilder` in the tests as each tool's
  tests move.
* **1c** `lib/project/`. Delegates: `symedit.parse_line/entries/plan_rename/apply_rename/plan_merge`, the 24 splits parsers,
  the 14 configure parsers, `stylelint.Ownership/load_ownership(_at_ref)`, `symbolpreflight.load_*`, `ledger.Objects`,
  `sharedfiles.parse_ranges/find_overlap`.
* Acceptance: all eight items; the before/after count of parser implementations (24/18/14 -> 1/1/1 plus delegates).

### WP2 - the middle lib (three lanes in parallel)

* **2a** (L) `lib/ppc.py`, `lib/refs.py`. Delegates: `splitcheck.scan_refs/scan_calls/find_sda_bases/Ctx`, `phantom.index_refs`,
  `infer.Insn` + field helpers, `callers.build_index/build_elf_index/query/load_index`, `datagap.census`, `tudiscover.build_graph`.
  Measured acceptance: `callers.py --stats` and `datagap.py --census` report the same counts before and after on this tree;
  `splitcheck --baseline` the same PASS/FAIL/UNKNOWN totals.
* **2b** (M) `lib/report.py`, `lib/units.py`. Delegates: `unitutil.report_*`, `freshguard`, the 16 report readers, `reportdiff.*`,
  `land.report_snapshot/report_regressions`, the 30 unit-name functions, `recompile.unit_tokens/compile_unit/proposal_target`.
  Measured: `ledger.py --json` identical before/after; `measure.py <unit>` identical table; `metric_selftest` green.
* **2c** (M) `lib/findings.py`, `lib/cli.py`, `lib/outbox.py`, `lib/cscan.py`. Delegates: `land.check` row tuples (rendered through
  `Row`), `stylelint._finding/finding_identity/added_identities`, `handoff.CONFIG_REQUEST_SCHEMA/validate`, `stylelint.strip/match_brace/
  struct_defs/function_declarations`, `typeregistry.strip_comments/extract_decls`, `declclash.closure`, `shapes.strip_comments`.

### WP3 - the tool families (parallel lanes; each family is one batch)

* **3a** (L) object comparison: `lib/objcompare.py`; thin `datagap` + new `dataclosure.py`, `sectiongap`, `pairgap`, `relocdiff`,
  `flipcheck`, `undefrefs` (absorbing `relocaudit` as `--census`), `verifyunit`. Measured: `datagap --flip-blockers`, `flipcheck`
  (all units), `undefrefs --census`, `relocdiff --all --check` byte-identical output before/after on this tree.
* **3b** (M) scoring: `recompile`, `measure`, `unitscore`, `symdiff`, `slotmap`, `flags/*`; `freshguard.py` and `reportdiff.py`
  become shims. Measured: `measure.py Camellia/camellia` table identical; `unitscore` zero/one objdiff calls kept (its test).
* **3c** (L) seams and references: `callers`, `callees`, `accessextent`, `dossier` on `lib.refs`/`lib.ppc`; `tools/splits/seams/`
  (`tudiscover`, `dataorder`, `dataseams`, `poolseams` as entry points over one evidence module); `splitcheck` reduced to
  `--baseline` over `tools/splits/invariants/`; `attribute.py` retired (question 1) or thinned. Measured: `tudiscover at <addr>`
  on three addresses from `docs/splits/phase4/*.md` gives the same cuts; `dataorder scan --json` identical; `callers <addr>`
  identical for the two addresses the profiles quote.
* **3d** (L) symbols and source scanners: `symedit`, `dumpmap`, `phantom`, `mangle`, `methodize` on `lib.project`/`lib.names`;
  `stylelint` split into rules modules on `lib.cscan` + `Ownership`; `typeregistry`, `declclash`, `recordmerge`, `vtableaudit`'s
  source scan. Measured: `stylelint --budget --json` identical finding set; `stylelint --diff main` on the batch adds none;
  `vtableaudit --json` identical; `symedit check` identical.
* **3e** (L) lanes: `lib/lanes/` + thin `claims`, `slots`, `lane`, `rescue`, `wtsafe`, `queue`, `lanecmd`, `worktreehook`;
  `brief` split; herdr code deleted. Measured: `claims.py list --json`, `slots.py status --json`, `queue.py list --json` identical;
  a fixture claim/release round trip; `brief.py <unit> --stdout` byte-identical for three units.
* **3f** (M) merge and git: `tools/units/merge/` (`mergebranch`, `unionresolve`, `unionguard`, `unionprose` over one union rule),
  `commitlint`, `guard`, `prepcommit`; `edit.py` absorbs `checklf` and `escape --edit`. Measured: the merge fixtures of all three
  tools green on the one implementation; `commitlint --last 50` identical.
* **5** (M, parallel with WP3) debuggers: `tools/mwlink/` package with `mwlink_debugger.py` shim; `mwcc-debugger` on `lib.proc`
  and sharing `pe.py`. Measured: `mwlink_debugger.py verify` and `trace` on one unit identical.

### WP4 - the landing gate (L, after 3a, 3b, 3d, 3e, 3f)

* `tools/units/landing/` as in `design.md` section 5; `land.py` becomes the CLI; rows call the lib; `.pi/bin/*` scripts retired
  (question 2) with `CLAUDE.md` step 3 rewritten to `land.py land --branch`.
* Measured: on a recorded no-op batch and on a real unit batch, the row names and statuses are identical to the old gate's
  table (a golden file in `tests/units/landing/`); suite time; `land.py verify` wall time before/after.

### WP6 - the sweep (M, after WP4)

**Done 2026-10-04** (status and numbers: `retired.md`, "Folded into another tool"; the commits are the branch's
`tools/units: ...` series). Kept, with the reason in `retired.md`: `escape.py --edit` (the profiles name it), the
`mergebranch.py`/`land.py`/`stylelint.py`/`brief.py`/`mwlink_debugger.py` shims (the compatibility list).


* Delete the delegates and shims (`unitutil.py` -> re-export only what `mt.py` needs, then move `mt.py`'s forwarders to `lib.cli`;
  `--selftest` flags; `freshguard.py`, `reportdiff.py`, `subproc.py`, `spawnretry.py`, `sharedfiles.py`, `checklf.py`, `relocaudit.py`,
  `promote*.py`, `applysplits.py`, `dataattach.py`, `matchinggain.py`, `gen_trk_vectors.py`, `infer-run.md`, `relocaudit-findings.md`).
* Replace every module docstring with the header template; move the remaining prose into the specs (the specs already exist
  from this design, so this is a diff of what changed during migration).
* The reference sweep (list below) in the same batch; `docs/pipeline.md` section 9 replaced by a pointer to `docs/tools/README.md`.

## References to update (where the old names live)

From the path-qualified scan (`inventory.md`, "callers"); counts are lines.

| retired or renamed | `.claude/agents` | `.claude/skills` | `CLAUDE.md` | `docs/` | other tools |
| --- | ---: | ---: | ---: | ---: | ---: |
| `unitutil` (shim, then lib) | 0 | 4 (`mt.py`, `objdiff-verify`) | 1 | 2 | 47 |
| `--selftest` flags (73 tools) | 1 (`tudiscover --selftest`) | 1 | 0 | ~20 | runner only |
| `freshguard.py` | 0 | 1 | 0 | 1 | 11 |
| `reportdiff.py` | 0 | 0 | 0 | 0 | 0 |
| `relocaudit.py` -> `undefrefs --census` | 0 | 0 | 0 | 0 | 5 |
| `checklf.py` -> `edit.py check` | 0 | 0 | 0 | 0 | 3 |
| `escape.py --edit` -> `edit.py replace` | 12 (`decompiler.md` et al.) | 3 | 0 | 11 | 1 |
| `promote.py`, `promote_batch.py` | 0 | 1 | 0 | 2 | 13 |
| `applysplits.py` | 0 | 0 | 0 | 20 (`docs/splits/**`) | 0 |
| `dataattach.py` | 0 | 0 | 0 | 45 (`docs/splits-program.md`, `docs/splits/**`) | 2 |
| `matchinggain.py` | 0 | 0 | 0 | 6 | 2 |
| `splitcheck.py --proposal` | 0 | 0 | 0 | 34 | 6 |
| `attribute.py` (question 1) | 0 | 3 | 1 | 36 | 19 |
| `.pi/bin/applybranch.sh` (question 2) | 0 | 0 | 1 (step 3) | 2 | 5 |
| `subproc.py`, `spawnretry.py`, `sharedfiles.py` | 0 | 0 | 0 | 3 | 20 |
| `mwlink_debugger.py` (shim kept) | 5 | 12 | 1 | 23 | 19 |
| `docs/pipeline.md` section 9 roster | - | - | - | 1 section | - |

The `docs/splits/**` and `docs/splits-program.md` references describe the applied program; they are history and stay as they are,
with one line added at the top of `docs/splits-program.md` pointing at `docs/tools/retired.md`.

## CLI compatibility list (the invocations the profiles, skills and core docs spell)

Every line below must keep parsing with `--help` and keep its documented behaviour. Collected with
`grep -rhoE "python (tools|.claude/skills)/..." .claude/agents .claude/skills CLAUDE.md docs/pipeline.md docs/plan.md`
(counts in `inventory.md`'s method); `<X>` is any argument.

```
tools/symbols/symedit.py rename|rename-batch|merge-batch|find|show|at|range|refs|check
tools/units/vtableaudit.py --diff <ref> | --unit <X> | (no args)
tools/units/methodize.py <X> | --all | --batch | --exact
tools/agents/sync_playbook_index.py [--check]
tools/units/stylelint.py --diff <ref> | --ref <branch> | --budget
tools/splits/dump_asm.py [--check]
tools/flags/mwcc_matrix.py [-u <X>] [--list-versions]
tools/agents/sync_profiles.py [--check]
tools/agents/edit.py replace|normalise|check
tools/units/slots.py spawn --kind <X> | init | collect --path <X> --release
tools/units/mangle.py <X>
tools/units/escape.py --write|--escape|--edit
tools/units/claims.py ack|claim|release
tools/selftest.py [--changed [main]] [--json]
tools/objdiff/unitscore.py <X> [--report|--measure]
tools/flags/shapesearch.py -u <X>
tools/units/recompile.py <X> [--measure <sym>]
tools/units/ledger.py [--json] | unit <X>
tools/units/datagap.py --unit <X> | --census --unit <X> | --flip-blockers
tools/symbols/dumpmap.py lookup <X>
tools/splits/tudiscover.py at <X> | dataorder | stats | prune [--include-obj --apply] | cache --force | bench [--seeds|--compare] | --selftest
tools/objdiff/relocdiff.py <X> --by-owner
tools/mwlink_debugger.py trace|verify|order|align --unit <X>
tools/elf/elfsect.py <X>
.claude/skills/mwcc-unit-matching/scripts/sync_reference.py [--check]
.claude/skills/mwcc-unit-matching/scripts/mt.py diff -u <X> | variants --apply <X> | units | info | diff
tools/units/symbolpreflight.py <X>
tools/units/queue.py next [--profile <X>] [--count <N>]
tools/units/m2cinput.py build/RMHE08/obj/<X>.o -f <X>
tools/units/langcheck.py --disagree | --unit <X>
tools/units/land.py verify | land --branch <X>
tools/units/flipcheck.py <X>
tools/units/callers.py <X>
tools/units/attribute.py plan <X> <X>        (until question 1 is decided)
tools/objdiff/symdiff.py -u <X>
tools/objdiff/pairgap.py
tools/m2c/m2c.py -t ppc-mwcc-c ...
tools/flags/tryvar.py -u <X>
tools/flags/optsweep.py -u <X>
tools/agents/ideas.py where|find|check|new|demo-check [--all]
tools/units/worktreehook.py arm
tools/units/recordmerge.py --base <X> --other <X>
tools/units/poolseams.py [--unit <X>]
tools/units/mergebranch.py resolve
tools/units/measure.py <X> --against-main
tools/units/dataclaim.py --unit <X>
tools/units/backlog.py triage | --set-status
tools/splits/dataorder.py at <X>
tools/git/prepcommit.py [--split]
tools/git/commitlint.py --message <X>
tools/flags/frame.py -u <X>
```

Output shapes lanes parse (must not change): `flipcheck`'s `   - ` reason lines; `land.py verify`'s row table and its
`FAILED:`/`ok` summary; `claims.py release`'s step list and `complete`; `queue.py next`'s spawn block (cwd, name, task);
`symedit rename`'s one-line diff; `symedit refs`' `code`/`path`/`mention` classes; `stylelint --diff`'s `added`/`moved` lines;
`unitscore`/`symdiff` per-symbol tables; `measure.py`'s delta table.

## Parallelism summary

```
WP0 ----> {1a, 1b, 1c} ----> {2a, 2b, 2c} ----> {3a, 3b, 3c, 3d, 3e, 3f, 5} ----> 4 ----> 6
```

Six lanes at most (the slot budget), so WP3 runs as two waves: {3a, 3d, 3e} then {3b, 3c, 3f, 5}, each lane a `tooling` kind
(`slots.py spawn --kind tooling`), each package one landed batch, each landing preceded by a `codereviewer` pass on the diff.
