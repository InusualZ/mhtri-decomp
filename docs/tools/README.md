# Tools: the map

`tools/` is one package of shared concepts (`tools/lib/`) and the thin tools on top of it. This page is the index: which
concept lives in which lib module, which tools stand on it, where each tool's spec is, how to add a tool, and the header every
tool file carries. The design is `design.md`; the analysis that produced it is `inventory.md`, `duplication.md`, `prose-audit.md`;
the plan to get there is `migration.md`; what is dropped is `retired.md`; what the owner still has to decide is `questions.md`.

## Concept -> lib module -> tools

| concept | lib module | spec | tools standing on it |
| --- | --- | --- | --- |
| trees, paths, scratch, state, ground truth | `lib/repo.py` | `spec/lib-repo.md` | every tool |
| subprocess with the codec rule | `lib/proc.py` | `spec/lib-proc.md` | every tool that shells out |
| git calls | `lib/git.py` | `spec/lib-git.md` | land, claims, slots, lane, rescue, mergebranch, stylelint, guard, prepcommit, commitlint, edit |
| bytes, line endings, atomic writes, anchors | `lib/text.py` | `spec/lib-text.md` | edit, escape, symedit, dataqueue, backlog, sync_profiles, mergebranch |
| stamped caches | `lib/cache.py` | `spec/lib-cache.md` | callers, tudiscover, undefrefs, verifyunit |
| generated names and manglings | `lib/names.py` | `spec/lib-names.md` | callees, dumpmap, undefrefs, langcheck, mangle, methodize, stylelint, typeregistry |
| `symbols.txt`, `splits.txt`, `configure.py`, ownership | `lib/project/` | `spec/lib-project.md` | symedit, stylelint, symbolpreflight, ledger, datagap, dataclaim, dataqueue, flipcheck, vtableaudit, unwindcut, land, brief, queue, ... |
| ELF, DOL, objdump text, DWARF, fixture builders | `lib/binary/` | `spec/lib-binary.md` | elfsect, objalign, objextab, dwarfmap, dossier, linkorder, langcheck, flipcheck, verifyunit, datagap, sectiongap, pairgap, relocdiff, undefrefs, vtableaudit, vtslot, unwindcut, m2cinput, callees, infer, mwlink |
| instruction decode, reference scan | `lib/ppc.py` | `spec/lib-ppc.md` | splitcheck, phantom, infer, dossier, accessextent, callers, vtableaudit |
| who references what (index, census) | `lib/refs.py` | `spec/lib-refs.md` | callers, callees, accessextent, datagap, dataclaim, poolseams, tudiscover, splitcheck |
| scores, the metric, freshness, regression | `lib/report.py` | `spec/lib-report.md` | ledger, verifyunit, unitscore, symdiff, measure, recompile, pairgap, datagap, brief, land, reportdiff, flags/* |
| the unit and its compile command | `lib/units.py` | `spec/lib-units.md` | recompile, measure, unitscore, symdiff, flipcheck, datagap, verifyunit, brief, flags/* |
| target vs ours comparisons | `lib/objcompare.py` | `spec/lib-objcompare.md` | datagap, dataclosure, sectiongap, pairgap, relocdiff, flipcheck, undefrefs (+ relocaudit), verifyunit |
| C/C++ text scanning | `lib/cscan.py` | `spec/lib-cscan.md` | stylelint, typeregistry, declclash, recordmerge, methodize, vtableaudit, shapes |
| findings, rows, verdicts, add-only diff | `lib/findings.py` | `spec/lib-findings.md` | land, stylelint, vtableaudit, undefrefs, datagap, splitcheck, flipcheck, verifyunit, dataclaim, symbolpreflight, handoff |
| the CLI entry point | `lib/cli.py` | `spec/lib-cli.md` | every tool |
| the test harness | `lib/testing.py` | `spec/lib-testing.md` | every test under `tools/tests/` |
| the outbox and notes | `lib/outbox.py` | `spec/lib-outbox.md` | handoff, backlog, tooling, playbook, brief, land, slots |
| lanes: naming, registry, rescue, teardown, launch | `lib/lanes/` | `spec/lib-lanes.md` | claims, slots, lane, rescue, wtsafe, queue, lanecmd, worktreehook, brief |

## Spec index (the kept and new tools)

Core gate: `land`, `verifyunit`, `stylelint`, `vtableaudit`, `undefrefs`, `flipcheck`, `datagap` (+ `dataclosure`), `langcheck`,
`ledger`, `recompile`, `handoff`, `playbook`, `commitlint`, `guard`, `prepcommit`, `selftest`, `objalign`, `objextab`,
`ideas`, `sync_playbook_index`, `sync_profiles`.

Core evidence: `callers`, `callees`, `accessextent`, `dossier`, `symedit`, `dumpmap`, `phantom`, `mangle`, `methodize`,
`symbolpreflight`, `tudiscover`, `dataorder`, `dataseams`, `poolseams`, `splitcheck`, `dump_asm`, `dataclaim`, `dataqueue`,
`sectiongap`, `pairgap`, `relocdiff`, `unitscore`, `symdiff`, `measure`, `unwindcut`, `vtslot`, `linkorder`, `m2cinput`,
`typeregistry`, `declclash`, `elfsect`, `dwarfmap`, `reportdiff`.

Agent plumbing: `claims`, `slots`, `lane`, `rescue`, `wtsafe`, `queue`, `lanecmd`, `worktreehook`, `brief`, `backlog`,
`tooling`, `mergebranch` (+ `unionresolve`, `unionguard`, `unionprose`), `recordmerge`, `edit`, `escape`, `profileprobe`, `install`.

Matching experiments: `infer`, `frame`, `optsweep`, `mwcc_matrix`, `tryvar`, `shapesearch` (+ `shapes`), `slotmap`,
`mwcc-debugger`, `mwlink`.

Dormant: `rso`. Template (dtk-template, no spec of ours): `project`, `ninja_syntax`, `download_tool`, `transform_dep`,
`decompctx`, `changes_fmt`. Retired: `retired.md`.

Each spec is `docs/tools/spec/<name>.md` with the fixed sections: Purpose, Users, CLI (flags, exit codes, JSON), Inputs and
outputs, Invariants and rules, Lib dependencies, Test contract, Known gaps.

## How to add a tool

1. Write the spec first (`spec/_template.md`), including the test contract; name the lib modules it stands on. If a concept it
   needs is not in `lib/`, add it to the lib (with its contract test) rather than to the tool.
2. Create `tools/<area>/<name>.py` with the header below, the prologue line, a `lib.cli.Tool`, and functions that return
   `Finding`/`Row` values (never print inside the logic).
3. Create `tools/tests/<area>/test_<name>.py` with `TIER = "fixture"` on `lib.testing`; add a smoke test only for a check that
   genuinely needs the live tree, and make it skip (not fail) when its input is absent and never pin a count.
4. Add the tool to the spec index above and, if a profile or skill should call it, to that profile's prose.
5. `python tools/selftest.py --changed` green; `python tools/units/stylelint.py --diff main` is not for tools (it lints `src/`).

## The in-code header template

The only module-level prose a tool carries:

```python
"""<one-line purpose>. Spec: docs/tools/spec/<name>.md. CLI: <shape>."""
```

Example:

```python
"""Can our object fill every section the unit claims? Spec: docs/tools/spec/flipcheck.md. CLI: flipcheck.py [unit..]."""
```

A rule, an incident, a design note or a usage block does not go here; the spec holds them, and `tests/lib/test_headers.py`
refuses a module docstring longer than three lines or without the spec path.
