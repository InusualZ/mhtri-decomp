"""`land` and `land --branch`: gate -> stage -> commit -> release, one answer line, and the landing log.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import contextlib
import sys
import time

from tools.lib.lanes import landlog
from tools.lib.lanes import naming
from tools.lib.lanes import registry
from tools.units.landing import gate
from tools.units.landing import release
from tools.units.landing import state
from tools.units.landing.base import base_dirty_paths, ledger_line, ledger_numbers, record_base, summary
from tools.units.landing.branch import (_sweep_resolve_helpers, apply_branch, claim_unit_for_branch,
    resolve_helper_state, undo_apply, units_from_branch)
from tools.units.landing.common import changed_status, git, kinds_from_failures, outside_batch, read_base, scratch_paths
from tools.units.landing.message import (clear_land_message, land_message_path, message_body_with_subject,
    message_error, write_land_message)
from tools.units.landing.rows.tree import (branch_error, preflight_foreign, preflight_report, require_clean_tree,
    tolerate_scratch)
from tools.units.landing.stage import (commit_pathspec, foreign_warning, land_decision, land_stageable,
    looks_already_applied, stage_batch, staged_elsewhere)


# --- the landing log: every `land` attempt is one line of `.pi/land-log.jsonl` (`lib.lanes.landlog`) ----------

def _attempt(allow: dict | None = None) -> dict:
    """What one attempt records while it runs; an exception leaves the outcome `error`. `allow` is the invocation's
    allowances (`state.allowances`), recorded whatever the outcome."""
    return {"outcome": "error", "row": None, "conflicts": [], "units": [], "commit": None, "allow": dict(allow or {}),
            "warnings": []}


def _refused(rec: dict, line: str, row: str, outcome: str = "refused") -> int:
    """Print the REFUSED answer line and record which guard or gate row refused; -> the exit status 1."""
    print(line)
    rec.update(outcome=outcome, row=row)
    return 1


def failing_row(problems: list[str]) -> str | None:
    """The first failing gate row's name, from `verify`'s `problems` (`"<row> [KIND]: ..."`)."""
    return problems[0].split(" [", 1)[0] if problems else None


def _log(main: str, branch: str, rec: dict, t0: float) -> None:
    try:
        landlog.append(main, landlog.Attempt(branch, rec["outcome"], round(time.time() - t0, 1),
                                             refused_row=rec["row"], conflicts=tuple(rec["conflicts"]),
                                             units=tuple(rec["units"]), commit=rec["commit"],
                                             allow=rec.get("allow") or {},
                                             warnings=tuple(rec.get("warnings") or ())))
    except OSError as exc:                         # the log is evidence; it never changes a landing's answer
        print("WARNING: the landing log was not written (%s)" % exc, file=sys.stderr)


def land_branch(main: str, branch: str, units: list[str] | None = None, base: str | None = None,
                no_build: bool = False, allow_regression: list[str] | None = None,
                check_outbox: bool = True, release_claims: bool = True,
                subject: str | None = None, no_selftests: bool = False) -> int:
    """The one-command landing: clean tree -> record-base -> apply+union -> gate -> commit -> release.

    Idempotent and loud: every refusal prints one `REFUSED <branch> | <reason>` line (stdout) and leaves
    MAIN exactly as it was found - the apply is undone whenever the landing did not reach a commit, so a
    refused landing is never a half-landing.  The registered unit(s) come from the branch's registration
    diff when `--units` is not given, so a unit renamed at registration is landed under its real name.
    Every attempt appends one line to the landing log.
    """
    t0, rec = time.time(), _attempt(state.allowances(allow_regression, check_outbox, no_selftests))
    try:
        return _land_branch(main, branch, rec, units, base, no_build, allow_regression, check_outbox,
                            release_claims, subject, no_selftests)
    finally:
        _log(main, branch, rec, t0)


def _land_branch(main, branch, rec, units, base, no_build, allow_regression, check_outbox, release_claims,
                 subject, no_selftests) -> int:
    norm = [naming.norm_unit(u.strip("/")) for u in (units or []) if u.strip()]
    rec["units"] = norm
    bad_message = message_error(subject)
    if bad_message:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (branch, bad_message), "--message")
    bad_branch = branch_error(main)
    if bad_branch:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (branch, bad_branch), "HEAD is main")
    if not registry.branch_exists(main, branch):
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | no such branch" % branch, "branch exists")
    dirty = require_clean_tree(main)
    if dirty:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (branch, dirty), "clean tree")
    merge_base = base or git(["merge-base", "main", branch], main).strip()
    if not norm:
        norm = units_from_branch(main, branch, merge_base)
        rec["units"] = norm
    if not norm:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | could not read the branch's registered unit(s) from its configure.py/"
                             "splits.txt diff; pass --units explicitly" % branch, "units from the branch")
    # Read the resolve-helper state now, while `branch` still exists: the landing below may release and
    # delete it, and the containment proof is against `branch`'s pre-land tip.
    helper_redundant, helper_refused = resolve_helper_state(main, branch)
    record_base(main, norm)                # on the clean tree, BEFORE the pick; snapshots the base refs
    head_before = git(["rev-parse", "HEAD"], main).strip()
    ok, why, applied_base = apply_branch(main, branch, base=merge_base, conflicts=rec["conflicts"])
    if not ok:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s (the apply was undone; main is unchanged)" % (branch, why), "apply",
                        outcome="conflict" if rec["conflicts"] else "refused")
    code = _land(main, norm, rec, None, no_build, allow_regression, check_outbox=check_outbox,
                 release_claims=release_claims, subject=subject, branch=branch, no_selftests=no_selftests)
    if code != 0 and git(["rev-parse", "HEAD"], main).strip() == head_before:
        # the gate refused before committing: undo the apply so a refused landing is not a half-landing
        undo_apply(main, applied_base, branch)
        print("NOTE: the apply was undone - main is back at %s" % head_before[:8], file=sys.stderr)
    if code == 0:
        # the branch landed: its resolve helper (if any) is debris now. Delete only what is provably
        # contained; refuse loudly on anything else rather than leaving a `land/*` ref and hoping.
        _sweep_resolve_helpers(main, branch, helper_redundant, helper_refused)
    return code


def land(main: str, units: list[str], base: str | None, no_build: bool,
         allow_regression: list[str] | None = None, check_outbox: bool = True,
         release_claims: bool = True, subject: str | None = None,
         already_applied: bool = False, branch: str | None = None,
         no_selftests: bool = False) -> int:
    """The one command: gate -> stage the batch's files -> commit -> release, one answer line on stdout.

    The failure mode this closes: `verify`'s output was piped (`| tail -3`), the exit status was lost, and a
    batch whose gate had *failed* was committed by hand - twice, leaving a partial source on `main` while
    `ok` stayed green (the unit is `NonMatching`). So the gate's verdict is now the decision, not a report:

    * a red gate never reaches `git commit` (`land_decision`), and `verify` removes any stale message. A
      refusal NAMES the failing check and what it printed - a bare `the gate failed` is not actionable, and a
      reader who cannot see which gate failed cannot tell a real defect from a passing check (2026-09-25);
    * the refusal also names each check's KIND and its remedy. A GATE failure says "the gate failed"; a
      BOOKKEEPING failure (the batch is fine, the landing's own state is stale) says so and never "the gate
      failed", so a reader does not mistake a released branch or a stale base for a bad batch;
    * a worker branch a `--force` release parked at `refs/rescue/<slug>` is restored by the gate
      (`restore_rescued_branch`), because the gate only needs the work to exist as commits;
    * a batch applied before `record-base` (so its paths are in the base's dirty snapshot) is named as such,
      and `--already-applied` lands it rather than making the caller revert and re-record;
    * an empty or whitespace-only `--message` is refused before the gate runs (`message_error`): the empty
      shell substitution that expanded `$(cat /tmp/msg1.txt)` must not silently land the fallback subject;
    * a HEAD that is not `main` is refused before the gate runs (`branch_error`): a land run off `main` moves
      the wrong ref, and every later merge-base and branch diff is computed against a stale `main`;
    * the commit uses the gate's own message, so there is no separate `git commit -F` to get wrong;
    * the commit is `git commit -F msg -- <the batch's paths>`: no pathspec means the whole index, which
      swept another stream's staged edit into a land twice on 2026-09-23 (`85f3d4b5`, `d50fdd32`). Paths the
      index holds but the batch does not are left staged, and a warning names them;
    * the gate log goes to **stderr** and stdout carries exactly one answer line, so `tail -1` is the answer
      whether or not the exit status survived the pipe;
    * the exit status *is* the answer: 0 landed, 1 refused (or landed with an incomplete teardown);
    * every attempt appends one line to the landing log (`.pi/land-log.jsonl`), with every allowance the
      invocation granted (`state.allowances`: the `--allow-*` values travel on `state`, set by the CLI - the one
      path - and `--allow-regression`/`--no-outbox`/`--no-selftests` as parameters).

    Releasing runs after the commit, never before: until `main` has the commits, the worker's branch is their
    only copy.
    """
    t0, rec = time.time(), _attempt(state.allowances(allow_regression, check_outbox, no_selftests))
    try:
        return _land(main, units, rec, base, no_build, allow_regression, check_outbox, release_claims, subject,
                     already_applied, branch, no_selftests)
    finally:
        _log(main, branch or "", rec, t0)


def _land(main: str, units: list[str], rec: dict, base: str | None, no_build: bool,
          allow_regression: list[str] | None = None, check_outbox: bool = True, release_claims: bool = True,
          subject: str | None = None, already_applied: bool = False, branch: str | None = None,
          no_selftests: bool = False) -> int:
    norm_units = [naming.norm_unit(u.strip("/")) for u in units]
    rec["units"] = norm_units
    bad_message = message_error(subject)
    if bad_message:
        # a refusal must not leave a message `git commit -F .git/land_msg.txt` could pick up (2026-09-23)
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (",".join(norm_units), bad_message), "--message")
    bad_branch = branch_error(main)
    if bad_branch:
        # same rule as the message guard: a refusal leaves no committable message and touches nothing
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (",".join(norm_units), bad_branch), "HEAD is main")
    # pre-flight: a foreign path ALREADY in the tree is reported - with a likely cause - before the gate's
    # expensive build.  The same information the post-build refusal prints, delivered a build earlier; a
    # foreign path cannot disappear during the build, so an early refusal loses nothing and saves minutes.
    foreign_now = preflight_foreign(main)
    if foreign_now:
        clear_land_message(main)
        print(preflight_report(main, foreign_now), file=sys.stderr)
        return _refused(rec, "REFUSED %s | %d path(s) outside the batch are already in the tree before the build; "
                             "the post-build gate would refuse them too - clear the tree first"
                             % (",".join(norm_units), len(foreign_now)), "pre-flight (paths outside the batch)")
    gate_failures: list[str] = []
    with contextlib.redirect_stdout(sys.stderr):
        gate_ok = gate.verify(main, norm_units, base, dry_run=False, no_build=no_build,
                              allow_regression=allow_regression, check_outbox=check_outbox,
                              release_claims=False, problems=gate_failures, branch=branch,
                              no_selftests=no_selftests, warnings=rec.setdefault("warnings", [])) == 0
    rows = changed_status(main)
    outside = outside_batch([path for _code, path in rows], main=main)
    scratch = scratch_paths(outside)
    foreign = [p for p in outside if p not in scratch]
    if scratch:
        # the fix for the `d910.json` self-contradiction: the gate used to refuse this path and (via the
        # landing flow's `git add -A`) have staged it, so its own refusal was its own doing. Tolerated scratch
        # is named, de-indexed and left in the tree; the batch lands.
        print(tolerate_scratch(main, scratch), file=sys.stderr)
    if foreign:
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | paths outside the batch appeared during the build: %s"
                             % (",".join(norm_units), ", ".join(foreign)), "paths outside the batch (after the build)")
    base_dirty = base_dirty_paths(main)
    stageable = land_stageable(norm_units, rows, base_dirty, main=main)
    if gate_ok and looks_already_applied(rows, base_dirty, main=main):
        # the batch was applied to the tree *before* `record-base` ran, so the base's dirty snapshot recorded
        # its own edits as foreign and `land_stageable` excluded them (all of them, or just the shared files).
        # This is the 2026-09-26 case (b): the gate printed "READY: every check passed" and `land` then
        # refused with "no batch path is stageable", and the batch had to be committed by hand. Name the
        # ordering and the two ways out instead.
        if already_applied:
            print("NOTE: --already-applied: the batch was applied before `record-base`, so the base's dirty "
                  "snapshot recorded its own edits as foreign - staging the batch's paths anyway.",
                  file=sys.stderr)
            stageable = land_stageable(norm_units, rows, set(), main=main)
        else:
            clear_land_message(main)
            return _refused(rec, "REFUSED %s | the batch is already applied: every changed path was dirty when "
                                 "`record-base` ran, so the gate excluded it as foreign work and has nothing to "
                                 "stage. Revert the batch and re-record the base (`land.py record-base`) with a "
                                 "clean tree, or land it as-is with --already-applied." % ",".join(norm_units),
                            "already applied")
    action, why = land_decision(gate_ok, stageable, gate_failures, kinds_from_failures(gate_failures))
    if action != "commit":
        clear_land_message(main)
        return _refused(rec, "REFUSED %s | %s" % (",".join(norm_units), why),
                        failing_row(gate_failures) or "nothing to stage")
    msg_file = land_message_path(main)
    if subject is not None:
        # the guard above means subject is a real one here, never the empty shell substitution
        write_land_message(main, message_body_with_subject(open(msg_file, encoding="utf-8").read(), subject))
    stage_batch(main, stageable)
    foreign = staged_elsewhere(main, stageable)
    if foreign:
        print(foreign_warning(foreign), file=sys.stderr)
    # A pathspec, never a bare `git commit`: that takes the whole index and is how another stream's staged
    # edit landed under the batch's message twice on 2026-09-23.
    p = commit_pathspec(main, msg_file, stageable)
    if p.returncode != 0:
        clear_land_message(main)
        tail = ((p.stderr or p.stdout) or "").strip().splitlines()
        return _refused(rec, "REFUSED %s | git commit failed: %s" % (",".join(norm_units), tail[-1] if tail else ""),
                        "git commit", outcome="error")
    sha = git(["rev-parse", "--short", "HEAD"], main).strip()
    rec.update(outcome="landed", commit=sha, row=None)
    teardown, incomplete = [], []
    if release_claims:
        # A unit renamed at registration keeps the pre-registration claim key while the gate compiled its
        # registered name; the branch is the claim's identity, so release the key the branch records.
        release_key = claim_unit_for_branch(main, branch) if branch else None
        for u in norm_units:
            out = release.release_unit(release_key or u, main)
            if out.get("complete"):
                teardown.append(u)
            else:
                incomplete.append("%s (%s)" % (u, out.get("refused") or "a teardown step failed"))
    before_ledger, after_ledger = read_base(main).get("ledger") or {}, ledger_numbers(main)
    ledger = summary(before_ledger, after_ledger)
    tail = ""
    if teardown:
        tail += " | teardown %s" % ",".join(teardown)
    if incomplete:
        tail += " | TEARDOWN INCOMPLETE: %s" % "; ".join(incomplete)
        rec["row"] = "claim released (teardown incomplete)"
    # the short ledger delta goes on its OWN line above the answer line: a long batch's LANDED line lists every
    # unit and is cut by `cut -c1-N` before it reaches its numbers. The LANDED line stays as it was.
    print(ledger_line(before_ledger, after_ledger))
    print("LANDED %s %s | %s%s" % (sha, ",".join(norm_units), ledger, tail))
    return 1 if incomplete else 0
