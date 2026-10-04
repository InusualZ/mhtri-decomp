# `integrate` - apply the lanes' integrator requests in one batch

## Purpose

Reads the lanes' `<slug>-requests.json`, classifies and resolves each request against the tree, applies the mechanical
ones (map renames with their source sweep, owner-header declarations, stale-spelling rewrites, STOPGAP removal,
relocation-analysis config keys) on an `integrate/<date>` branch, builds once, narrows what the build refuses, pre-runs
the gate's rows and prints the `land.py land --branch` line.

## Users

The orchestrator (the integrator role of the network pilot, `docs/pipeline.md` 10.10); `land.py integrate` forwards to
it; a lane under the trial rule runs `--dry-run` to prove names-only drift.

## CLI

```
python tools/units/integrate.py [--requests FILE..] [--lane SLUG..] [--outbox DIR] [--only ID..] [--names FILE]
    [--lane-units UNIT..] [--base BRANCH] [--branch NAME] [--dry-run] [--no-build] [--no-gate] [--no-claims]
    [--no-comments] [--no-mangle-check] [--retries N] [--commit] [--write-status] [--json]
python tools/units/land.py integrate ARGS...        # the same, forwarded unchanged
```

* default input: every `*-requests.json` in `MAIN/.pi/outbox`; `--lane` names slugs; `--names` supplies the decisions
  (`old new`, `old` a spelling or `0xADDR`) a GUESS or an unnamed target needs.
* `--dry-run` classifies, resolves and plans; writes nothing (no branch, no file, no sidecar).
* `--commit`: two commits - `config/symbols: name the N symbols ...` (the map and its sweep only) and
  `game/<module>: declare the integrated callees in their owners' headers` (everything else); both subjects pass
  `commitlint.py`.
* `--write-status` records each request's verdict (`applied` / `judgement` / `deferred`) in its sidecar.
* exit 0 when it ran (the verdicts are in the report), 1 on a refusal (a dirty tree), 2 when it could not run.
* `--json`: `{summary: {requests, classes, branch, base, rounds, build_seconds, drift {names_only, content}, report,
  objects, gate {stylelint, undefrefs, vtableaudit}, stopgap_mentions, units, commits, land}, items: [...]}`.

## Inputs and outputs

Reads the request files, their sidecars, the lane outbox `<slug>.json` (its units are the removal scope), the tree's
map/splits/sources, `claims.py list --json` (live lanes). Writes only in the invocation tree: `config/RMHE08/symbols.txt`
(through `symedit.py rename-batch --rewrite --comments`), `src/`, `include/`, `config/RMHE08/config.yml` (relocation keys
only), the branch, and with `--write-status` the sidecars.

## Invariants and rules

* **A request is applied only when it is mechanical, its owner unit has no live lane, and every name it needs is
  decided** (a GUESS needs a `--names` decision; a mangled decision is judgement - the class comes first).
* **The prototype's authority:** the owner's definition, else the request's `prototype`, else the lane's local
  declaration. A declaration whose types the owner header cannot see is refused (a C++-only header gets a forward
  declaration with the keyword the tree defines; a header read by C never does).
* **Linkage:** a mangled map name is declared outside the header's `extern "C"` region (and checked with `mangle.py`),
  an unmangled one inside it (one is opened when the header has none). A missing owner header is created from the
  template (guard `MHTRI_<PATH>_H`, `types.h`).
* **Scope of removal:** a foreign declaration is removed only in the lane's files (its units, files carrying a STOPGAP
  block), anywhere for a symbol a lane unit owns, in a band header only for a `decl-move`, and never in the owner's own
  source unless it is a `decl-move`. A rename's implicit declaration moves only when the lane declared it locally; when
  it cannot move, the rename still applies and the (renamed) local declaration stays.
* **Cleanup:** an emptied `extern "C"` block goes, a comment opening with `STOPGAP` left without code goes, a comment that
  described a removed declaration goes, a blank run the edit made collapses; a unit header's prose is never rewritten
  (its STOPGAP mentions are listed as `stopgap_mentions`).
* **One build:** `ninja -k 0`. A failed build is blamed on the declaration named in the compiler error (else a rename
  or rewrite named there, else the declarations that touched the failed source); those are dropped everywhere and the
  tree is re-applied from the base and rebuilt incrementally (`--retries`, default 4). A build that never goes green
  leaves the tree at the base.
* **The gate's reading, before the gate:** split-target drift (`objcompare.fingerprint`; names-only vs content),
  compiled-object identity (`objsame.py` against a snapshot of the base objects), `report.json` moved rows,
  `stylelint --diff <base>`, `undefrefs --base <base> <units>`, `vtableaudit --diff <base>`. `--units` = units whose
  source changed plus any unit whose split target's content moved.
* A crash mid-apply restores the tracked files and removes the files it created.

## Lib dependencies

`lib.requests`, `lib.cscan`, `lib.project`, `lib.objcompare`, `lib.report`, `lib.names`, `lib.text`, `lib.git`,
`lib.proc`, `lib.repo`, `lib.outbox`. Other tools by subprocess only: `symedit`, `claims`, `mangle`, `stylelint`,
`undefrefs`, `vtableaudit`, `objsame`, `commitlint`.

## Test contract

Tier fixture: `tools/tests/units/test_integrate.py` - planning on a fixture map (rename, GUESS deferred, member
judgement, stale rewrite, live owner, sidecar), the declaration edits on a fixture tree (legacy stopgap block and its
comment removed, a STOPGAP-BEGIN block removed, the include placed, out-of-scope declarations kept, idempotence, a new
header from the template, a C header refusing an unseen type), the build-log reader (an error block, never a warning),
blame and exclusion. Acceptance (measured, not in the suite): the L3/L4 replays in `docs/pipeline.md` 10.10.

## Known gaps

* MWCC reports one error per object per run, so a batch with k wrong declarations in one file takes k incremental rounds.
* Type changes at call sites (`net_exp_heap` retyped, a cast, `&array` -> `array`) are never made: they come back as
  judgement with the compiler's message.
* The declaration comment is the address only; the evidence stays in the request.
