# Retired tools

Each entry: the tool, the evidence it is dead or superseded, the replacement, and the one paragraph of history worth keeping.
Code stays in git history (the retiring commit is named in the migration batch); nothing here is kept in the tree after WP6.

## The splits-program proposal pipeline

**DONE 2026-10-03** (owner: "Retire now"), removed by the commits whose subjects are:
`tools/splits: retire applysplits, dataattach, matchinggain and the splitcheck proposal half` (the three tools, the
proposal half of `splitcheck`, its spec rewritten), `tools/attribute: retire the tiler, its queue file and promote,
promote_batch` (`attribute.py`, `attribution-queue.json`, `promote.py`, `promote_batch.py` and their selftests),
`docs/splits-program: rewrite as the program record, delete the proposals and window manifests` (the record is
`docs/splits-program.md`; `docs/splits/phase4/homebutton-carried-notes.md` stays because 15 `src/`/`include/` headers cite it).
The entries below are the evidence and history that were kept; `tools/splits/gen_trk_vectors.py` is still to retire.

* **`tools/splits/applysplits.py`** (1 899 lines, last 2026-10-01). Phase 4 of the program: plan/apply/manifest/verify per window.
  Evidence: the six windows are applied (`docs/splits/phase4/land-*.txt`, `manifest-*.md`); referenced only by `docs/splits/**` (20
  lines) and nothing in `tools/`, `.claude/` or `CLAUDE.md`. Replacement: none needed; a future re-cut uses `unwindcut.py` and
  `symedit`, and `splitcheck --baseline` audits the result. History: the component model (identical / changed units linked by name
  or overlap, assigned to the window of the lowest `.text` start), the `names.json` placeholder map and the `verify` checklist
  (plan equality, registration on three axes, no function twice, no demoted unit still `Matching`, no regression) are the
  acceptance a landed window had to meet; `verifyunit.registration_problems` and `lib.report.regression` keep the two reusable
  halves.
* **`tools/splits/dataattach.py`** (2 377 lines). The phase 2 data-attachment engine: reader evidence from the retail text, a
  link-order dynamic program with decided/ambiguous verdicts, interleave zones, pool first-use order, settling against the
  invariants, holdout precision. Evidence: its output (`docs/splits/proposals/phase2-reconcile.json`) is applied; referenced by
  `docs/splits-program.md` (45 lines) and by `applysplits`/`matchinggain` only. Replacement: the evidence kinds it proved live on
  in `lib.refs` (readers), `lib.ppc.scan_refs` (its scanner, through `splitcheck.Ctx`) and `dataclaim.py --unit` (one run's owner).
  History worth keeping: *link order is a monotone assignment* (units link in text order in every section, so data ownership
  along an unowned run is non-decreasing), and *a strong read the optimum violates is a contradiction* that names a multi-TU unit
  or a foreign read - the two rules a future data attribution must honour.
* **`tools/splits/matchinggain.py`** (139 lines). The `Matching` units whose data the candidate changes. Evidence: a one-use phase 4
  list; its result is in the manifests. Replacement: `datagap.py --unit U` reports the same gap for one unit today.
* **`tools/splits/splitcheck.py` - the proposal half** (`load_proposal` ... `render`, `lint_proposal_full`, `--proposal`,
  `--emit-splits`, `--readers`, `--data-by-reader`, ~900 lines). Evidence: proposal files are history (`docs/splits/proposals/`).
  The `--baseline` audit (11 invariants) is kept and becomes `Row`s (`design.md` section 5). History: the proposal format
  (`docs/splits-program.md`) and the grades strong/medium/guess stay documented there.
* **`tools/units/attribute.py`** (1 361 lines) + `attribution-queue.json` + `attribute_selftest.py` - retired (question 1 ruled).
  Evidence: the program put every TU edge into `splits.txt` as a split-proven unit; `queue.py next` already prefers the pool of
  registered body-less units; the tiler's own docstring (lines 1963-1975) describes the queue as a snapshot that goes stale on every
  landing and once destroyed itself. Replacement: `splits.txt` is the queue; `tudiscover at` answers the one-address question.
  History: `segments` (too small joins a neighbour, too large is split and flagged), the coverage guard on `queue` (a rewrite may
  lose no proposal the file already had) and the `--max-total-bytes` cap are the rules a future tiler would need.

## Superseded by the register-once rule

* **`tools/units/promote.py`** (1 223) and **`tools/units/promote_batch.py`** (514) with their selftests (880) - retired with the tiler. Evidence: the
  `src/auto/` scaffolding bucket is retired (`docs/plan.md` section 12; `CLAUDE.md`, layout); nothing imports `promote` but
  `promote_batch`; no profile, skill or `CLAUDE.md` line names either. Replacement: a unit is registered once at its final home;
  a rename is `symedit rename` + `git mv` + the registration edit through `lib.project` (the three edits `promote` did are each a lib
  call now). History: the six-edit list (map, source, `git mv`, `splits.txt` key, `configure.py` object line with the lib's flags
  preserved, the pooled brief) and the byte-identity check across a move (`compare_objects`: a rename changes no instruction;
  a language change is expected to move bytes) are the invariants any future move tool must keep - `lib.objcompare.fingerprint`
  is the check.
* **`tools/splits/gen_trk_vectors.py`** (119). One-shot generator of the two TRK interrupt-vector units; they were claimed and matched
  on 2026-09-28 (`preflight_selftest.py` docstring). Replacement: none. History: the reason there are two units (a label in the middle
  of an array; MWCC pads `.init` objects to 8 bytes) is in the unit's own header comment.

## Folded into another tool

* **`tools/units/relocaudit.py`** (396) -> `undefrefs.py --census --linkage` (WP3a: the sweep, its label and its JSON moved into undefrefs, the rule into `lib.objcompare.linkage_audit`, the selftest to `tools/tests/units/test_undefrefs_linkage.py`; `relocaudit.py` forwards until WP6). Evidence: `undefrefs.unresolved_names` + `linkage_stem` is the
  same comparison per unit, and `undefrefs` is what the gate runs; `relocaudit` has no caller. History: the exclusion list (locals,
  `@NNN`/`@etb_*` labels, `STT_FILE`, `.comment`) is the spec of `lib.objcompare.undefined`.
* **`tools/units/relocaudit-findings.md`**, **`tools/flags/infer-run.md`**: dated outputs of one run; deleted (the commands that
  regenerate them are in the specs).
* **`tools/units/checklf.py`** (153) -> `agents/edit.py check --blob` (a shim since WP3f, `spec/checklf.md`). Evidence: same concern as `edit.py check` (working tree vs
  index endings); no caller. History: the invisibility it documents - after `git add`, `git diff`/`status`/`diff --cached` are all
  empty while the file on disk is CRLF - is the rule in `lib.text`'s spec.
* **`tools/units/escape.py --edit`** -> `agents/edit.py replace`; `--escape`/`--bytes`/`--write` stay (the profiles use them).
* **`tools/objdiff/freshguard.py`** -> `lib.report.Freshness`; **`tools/units/reportdiff.py`** -> `lib.report.regression` (+ a kept
  `reportdiff` CLI); **`tools/units/subproc.py`**, **`tools/spawnretry.py`** -> `lib.proc`; **`tools/units/sharedfiles.py`** ->
  `lib.text` + `lib.project.splits`; **`tools/unitutil.py`** -> `lib.repo` + `lib.units` + `lib.report` (shim until `mt.py` moves).

## Dead paths inside kept tools

* **`claims.py`'s herdr pane probing** (`herdr_exe`, `herdr_panes`, `herdr_read`, `herdr_close`, `resolve_pane`, `panes_for_claim`,
  `pane_probe`, the `--pane` flag; 50 references). Evidence: the pi harness and its pane tool were replaced by Claude Code on
  2026-09-29; live lanes are read from `~/.claude/sessions` (`slots.live_runs`). History: "a pane whose content moves is a worker
  that is alive, whatever its ack file says" becomes "a session whose pid is alive" in `lib.lanes.sessions.live_runs`. **Deleted
  in WP3e** (owner: delete, no attic): `status`/`timeout`/`release` read `lib.lanes.sessions`, the brief's ack line lost `--pane`.
* **`queue.py`'s and `brief.py`'s proposal/attribution paths** (71 + 145 references: `attribution-queue.json`, `proposals`,
  `proposal_by_label`, `covered_by_registered`, `is_proposal`, `render_proposal`, the TU/data/pool seam notes, the
  `stale`/`covered`/`written` states) and the **`--count` address stride** (`spread_picks`, `system_hints`). Deleted in WP3e
  (owner, 2026-10-04, `docs/pipeline.md` 10.10): the registered units are the queue (not-`Matching`), `--count N` claims lanes
  that share no owner header or module, and `--cluster <module|header>` gives header-sharing units one lane.
  `tools/units/queue_selftest.py` (a delegator to `queue.py --selftest`) went with them.
* **`MAIN/.pi/bin/applybranch.sh`, `landbranch.sh`, `mergelane.py`, `union.py`** (untracked) - deleted by the orchestrator (question 2 ruled); the tracked mentions went with `agents/policy: name land.py land --branch as the one landing path and retire the tiler lines`, `agents/merger: point the registration union at land.py resolve`, `docs/pipeline: name land.py land --branch in the plan, pipeline and profile-test docs` and `tools/units: drop the .pi/bin script mentions from the land, unionguard and unionresolve prose`. Evidence:
  `land.py land --branch` + `resolve` + `unionresolve` are the tracked implementation (its docstring says so); the scripts still end
  in `git add -A` (the sweep that bit twice) and reference `AGENTS.md`. Replacement: `land.py`; `CLAUDE.md` step 3 rewritten.

## Kept but dormant

* **`tools/rso/inventory.py`, `tools/rso/symbols.py`**: no caller; the RSO splitter blocker (`docs/rso-modules.md`) parks the work.
  Kept, with a spec marked dormant, because the seed maps they produced are tracked.
* **`tools/units/worktreehook.py`**: a prototype (its own header); kept outside the gate's tiers - **question 4**.
* **`tools/flags/variants/camellia.py`**: data for `tryvar`; every variant is a rejected candidate (its header). Kept as the record of
  what was tried; the spec of `tryvar` points at it.
