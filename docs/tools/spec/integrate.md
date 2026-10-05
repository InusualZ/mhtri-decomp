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
    [--no-comments] [--no-mangle-check] [--retries N] [--no-commit] [--write-status] [--json]
python tools/units/land.py integrate ARGS...        # the same, forwarded unchanged
```

* default input: every `*-requests.json` in `MAIN/.pi/outbox`; `--lane` names slugs; `--names` supplies the decisions
  (`old new`, `old` a spelling or `0xADDR`) a GUESS or an unnamed target needs.
* `--dry-run` classifies, resolves and plans; writes nothing (no branch, no file, no sidecar).
* default: the green tree is committed as two commits - `config/symbols: name the N symbols ...` (the map and its
  sweep only, committed only when that stage builds on its own; otherwise it folds into the second) and
  `game/<module>: declare the integrated callees in their owners' headers` (everything else, written byte for byte from
  the tree that built: `commits_verified`); both subjects pass `commitlint.py`. `--no-commit` leaves the green tree in
  the working tree; `--dry-run` and `--no-build` never commit; `--commit` is accepted and ignored.
* `--no-build` copies the before report without a base build, so it checks it first (`lib.artifacts` `report`, policy
  warn: the operator asked for no build): a stale report is recorded as `before_report_stale` and logged with its reason
  and refresh command (2026-10-04). With a build, `ninja -k 0` is the freshness judge and nothing changes.
* `--write-status` records each request's verdict (`applied` / `judgement` / `deferred`) in its sidecar; a request the
  tree already satisfies is recorded `applied`.
* exit 0 when the batch went green (the verdicts are in the report); **1 on a refusal**: a dirty tree, a base that does
  not compile, a build that never went green (nothing is committed); 2 when it could not run.
* `--json`: `{summary: {requests, classes, already_applied, branch, base, base_build, rounds, build_seconds,
  drift {names_only, content}, report, objects, gate {stylelint, undefrefs, vtableaudit}, stopgap_mentions, units,
  config_patch, config_apply, commits, commits_verified, left_in_tree, final, land}, items: [...]}`; each round carries
  its `errors` (every error of every failed object) and `narrowed {excluded, leaf, forced, reverted}`.

## Inputs and outputs

Reads the request files, their sidecars, the lane outbox `<slug>.json` (its units are the removal scope), the tree's
map/splits/sources, `claims.py list --json` (live lanes: every unmerged row's `unit` and its `units` - a cluster claim's
or a spawned lane's unit set, `units_of_rows`). Writes only in the invocation tree: `config/RMHE08/symbols.txt`
(through `symedit.py rename-batch --rewrite --comments`), `src/`, `include/`, `config/RMHE08/config.yml` (relocation keys
only, during the build rounds - the change leaves as a patch file under `build/tmp/integrate/`, never in a unit
commit), the branch, and with `--write-status` the sidecars.

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
  source unless it is a `decl-move`. A rename's implicit declaration moves when the lane declared it locally, and is
  **written to the owner header when a caller would otherwise see none** (`needs_declaration`; the lane's spelling then
  comes from the STOPGAP block step 0 removed) - the L2 round-2 batch's 9 renames got none and the committed tree did not
  compile. When it cannot be written and no caller needs it, the rename still applies and the (renamed) local
  declaration stays; when a caller needs it, the rename is reverted (judgement).
* **A STOPGAP block's removal takes its markers and its declarations** (prototypes, `extern` variables); any other code
  in it - a typedef a call site casts through (L2's `CircleInfoSetSender`, `NetworkErrorInfoGetter`) - stays, unwrapped.
* **A prototype names the decided symbol:** the request's prototype is rewritten from the filed spelling, the lane's
  proposed name and the map's current name to the final one (L2: `isCircleListBusy` filed, `testAndSet611b` decided).
* **Leaf headers:** when a consumer cannot include the owner's full header (a redefinition error in it after this batch
  declared into or included it), the declaration moves to `include/<module>/<symbol>.h` (section 6.5 rule 2's leaf
  header, created from the template) in the next round; a rerun finds an existing leaf header and declares nothing in
  the full one.
* **Already applied:** a request the tree already satisfies - no rename left, no stale spelling in the code, no STOPGAP
  block of its id, every declaration in its owner or leaf header with no foreign declaration left in scope (or, for a
  rename, unneeded), a config whose items `config.yml` carries - is `done` and listed under `## already applied`, in the
  dry run too; the sidecar's `applied`/`rejected` does the same.
* **Cleanup:** an emptied `extern "C"` block goes, a comment opening with `STOPGAP` left without code goes, a comment that
  described a removed declaration goes, a blank run the edit made collapses; a unit header's prose is never rewritten
  (its STOPGAP mentions are listed as `stopgap_mentions`).
* **The base builds first:** before anything is applied, `ninja -k 0` on the base; a base that does not compile (a
  merge of main is the usual cause) is refused with every error of each failed object, nothing is applied or reset, and
  the integrate branch it cut is deleted.
* **Narrowing, by object:** a failed round's objects are each recompiled directly with `-maxerrors 0` (the project's
  `-maxerrors 1` stops MWCC at an object's first error) into a scratch directory, so one round sees every error of every
  object (`object_errors`); `narrow` then picks every culprit's remedy at once - a redefinition in an owner header ->
  leaf header; `undefined identifier` of a rename target whose declaration was skipped -> forced into the owner header
  (a second time: the rename is reverted); a declared symbol named in an error -> excluded everywhere; a rename named ->
  reverted; the rest by the line text, then the edits that touched the failed source. Each round re-applies from the
  base (`--retries`, default 12).
* **A tree that did not build is never committed.** When the build never goes green (or a failure is unattributed) the
  run exits 1, commits nothing and leaves the last attempt in the working tree for the operator (`left_in_tree`).
* **The config patch:** a relocation-analysis request (YAML, a range `block_relocations source .text:A..B`, instruction
  addresses `block_relocations: 0xA and 0xB`, or `add_relocations source A type R target S` - `lib.requests.config_items`)
  is applied for the build rounds and then written as `build/tmp/integrate/<branch>-config.patch`
  (`config_patch`/`config_apply`); `config.yml` is restored before the commits.
* **The gate's reading, before the gate:** split-target drift (`objcompare.fingerprint`; names-only vs content),
  compiled-object identity (`objsame.py` against a snapshot of the base objects), `report.json` moved rows,
  `stylelint --diff <base>`, `undefrefs --base <base> <units>`, `vtableaudit --diff <base>`. `--units` = units whose
  source changed (`git diff --name-only <base>` plus created files) plus every unit whose split target moved, names-only
  neighbours included - computed on the green tree, so a run that changed sources never prints an empty `--units`.
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
blame and exclusion; the 2026-10-04 gaps - a STOPGAP block keeps its typedef, a rename's declaration comes from its
removed STOPGAP block, a `--names` decision rewrites the prototype, already-applied detection (owner header, leaf
header, a STOPGAP block still present), `narrow` (forced, excluded, reverted, leaf), the diagnostic compile command,
the config edit, and `run()` on a fixture repository with the build faked: a never-green build exits 1 with nothing
committed and the attempt left in the tree, a broken base is refused before anything is applied, a green build is
committed with a non-empty land line. Every one of those has a mutation that fails it. Acceptance (measured, not in
the suite): the L3/L4 replays and the L2 round-2 replay in `docs/pipeline.md` 10.10.

## Known gaps

* An error that cascades from another (an `expression syntax error` after an undefined type) is attributed by its
  line text, so it can exclude a declaration that was fine (it comes back as judgement, with that message).
* A `config` request's address is applied as filed: L2's #42 named 0x803D7764 while the block the lane landed
  (measured) is 0x803D7564 - only the score check after `git apply` tells them apart.
* Type changes at call sites (`net_exp_heap` retyped, a cast, `&array` -> `array`) are never made: they come back as
  judgement with the compiler's message.
* The declaration comment is the address only; the evidence stays in the request.
