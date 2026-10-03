# Open questions for the owner

Only decisions the design cannot make alone. Each has a recommendation; the design and migration assume the recommendation
unless ruled otherwise.

1. **RULED 2026-10-03: retired (done).** **Retire `tools/units/attribute.py` and `attribution-queue.json`?** The splits program put every TU edge into `splits.txt`;
   `queue.py next` already hands out registered body-less units from `brief --pool`. The tiler's queue is a snapshot that goes
   stale on every landing (its own docstring, lines 1963-1975). *Recommendation:* retire both once `ledger.py` reports no
   unclaimed `.text` (if any remains, keep `attribute.py plan` read-only until it is zero, then retire). The `tu-boundary-discovery`
   skill and `CLAUDE.md` (one line each) are updated in the same batch.
2. **RULED 2026-10-03: yes (done).** **Make `land.py land --branch` the only landing path and delete `MAIN/.pi/bin/*`?** Two procedures are documented today
   (`CLAUDE.md` step 3 names `applybranch.sh`; `land.py`'s docstring says the resolver was brought into the repo). The scripts end
   in `git add -A` and reference `AGENTS.md`. *Recommendation:* yes; WP4 rewrites `CLAUDE.md` step 3 and `docs/pipeline.md` section 5,
   and the four scripts are deleted from `.pi/bin` by the orchestrator (they are untracked; no commit removes them).
3. **Delete the herdr pane code from `claims.py`?** Fifty references to the pi harness's pane tool; live lanes are read from
   `~/.claude/sessions` since 2026-09-29. *Recommendation:* delete in WP3e; stall detection = ack age + session pid alive.
4. **`worktreehook.py` - keep as a prototype or retire?** The in-session subagent path (`isolation: "worktree"`) is marked
   PROTOTYPE and its payload cannot identify the lane (its docstring). *Recommendation:* keep it, on `lib.lanes`, excluded from
   the fixture tier and from the gate's selftest row (smoke only), until the orchestrator says it is used in production; retire
   otherwise in WP6.
5. **Python conventions and a test runner.** *Recommendation:* Python 3.12, stdlib only (no package manager), `from __future__ import
   annotations`, `@dataclass(frozen=True)` for lib value types, type hints on lib public APIs only, `tools/selftest.py` stays the
   runner (the `Checker` scripts are plain Python, runnable alone). Allowing `pytest` later would be a second runner for the same
   tests; not recommended now.
6. **How long do the compatibility shims live?** `--selftest` flags (73 tools), `unitutil.py`, `freshguard.py`, `reportdiff.py`,
   `subproc.py`, `spawnretry.py`, `sharedfiles.py`, `mwlink_debugger.py`. *Recommendation:* until WP6, then removed in one batch
   with the reference sweep; `mwlink_debugger.py` and `unitutil.py` (imported by the skill's `mt.py`) stay one package longer.
7. **Canonical invocation form.** `python tools/units/land.py ...` (what every doc spells) vs `python -m tools.units.land ...`
   (no prologue line needed). *Recommendation:* both work; docs keep the path form; the prologue line stays (one identical line,
   lint-checked).
8. **Tool renames.** `splitcheck --baseline` keeps the name `splitcheck` (its proposal flags go); `relocaudit` folds into `undefrefs
   --census`; `checklf` folds into `edit.py check`; `escape --edit` folds into `edit.py replace`; `tudiscover`/`dataorder`/`dataseams`/
   `poolseams` keep their names as entry points of `tools/splits/seams/`. *Recommendation:* no other rename; a rename costs a docs
   sweep and buys nothing the spec index does not.
9. **Where retired code goes.** Delete from the tree (git history keeps it) vs an `attic/` directory. *Recommendation:* delete;
   `retired.md` names the commit; an attic is a second place for dead code to rot.
10. **Fixture-tier enforcement.** Should a fixture-tier test that touches the live tree *fail* (recommended: `lib.repo.repo_root()`
    without `start=` raises under `TIER="fixture"`) or only warn? *Recommendation:* fail - the four breakages came from exactly this.
11. **`commitlint`'s `docs/<topic>` members.** They are derived from top-level `docs/*.md` only, so `docs/tools:` needed the entry page
    `docs/tools.md` (added in this batch, mirroring `docs/matching.md`). *Recommendation:* teach `derive_members` that a directory
    `docs/<dir>/` with a `README.md` is a member too; keep the entry page either way (it is the convention's own shape).
12. **The `rso/` tools.** Keep dormant (recommended; their seed maps are tracked) or retire.
13. **The debuggers' location.** `tools/mwlink_debugger.py` becomes the package `tools/mwlink/` (shim kept); `tools/mwcc-debugger/`
    keeps its hyphenated directory (it is not imported as a package). *Recommendation:* as stated; a hyphenated directory cannot
    be a package, which is why `mwcc-debugger` stays script-run.
14. **The selftest row's budget.** Today ~30 s for the gate. The fixture tier should stay under that; the smoke tier may exceed it.
    *Recommendation:* the gate runs the fixture tier always and the smoke tier only when the batch touches `tools/` (the
    `--changed` map decides), with the park list applying to both.
