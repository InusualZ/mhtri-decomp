"""The land gate: one command that runs the batch checklist and refuses to let a bad batch through.
Spec: docs/tools/spec/landing.md. CLI: land.py record-base|verify|land|resolve|integrate (flags in the spec)."""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

from tools.units.landing import common
from tools.units.landing import gate
from tools.units.landing.base import record_base
from tools.units.landing.branch import resolve_conflicts, scratch_resolve
from tools.units.landing.common import SELF_REPO, git
from tools.units.landing.flow import land, land_branch
from tools.units.landing.rows.tree import caller_branch_error
from tools.units.landing.state import set_allow_orphan, set_allow_rule10, set_allow_rule12, set_unit_renames


INTEGRATE = os.path.join(SELF_REPO, "tools", "units", "integrate.py")


def integrate_command(argv: list[str]) -> list[str]:
    """`land.py integrate ARGS` is `integrate.py ARGS`, unchanged: the integrator is its own tool (applying requests
    is not landing - the batch it builds lands through `land --branch` like any other), so this is a forward, by
    subprocess, never an import (the layering rule)."""
    return [sys.executable, INTEGRATE] + list(argv)


def main() -> int:
    if sys.argv[1:2] == ["integrate"]:
        return subprocess.run(integrate_command(sys.argv[2:])).returncode
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    rb = sub.add_parser("record-base", help="record main's HEAD as the batch base")
    rb.add_argument("--json", action="store_true")
    rb.add_argument("--units", default=None,
                    help="comma-separated batch units to compile and snapshot (the add-only row's base)")
    v = sub.add_parser("verify", help="run the batch checklist (never commits; use `land` for that)")
    v.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    v.add_argument("--units", default=None, help="comma-separated units in this batch")
    v.add_argument("--dry-run", action="store_true", help="run the cheap checks only; touch nothing")
    v.add_argument("--no-build", action="store_true", help="skip the split/link/ok/baseline steps")
    v.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                   help="skip the outbox/branch checks (an orchestrator-only batch); release still runs, "
                        "add --no-release to skip that too. `--no-worker-units` is the old alias and no "
                        "longer turns the teardown off")
    v.add_argument("--no-release", action="store_true", dest="no_release",
                   help="do not release the batch's claims")
    v.add_argument("--allow-regression", action="append", default=[],
                   help="unit whose measured regression is authorised by a rule (recorded in the message); repeatable")
    v.add_argument("--allow-rule12", action="append", default=[], metavar="TOKEN",
                   help="rule-12 token (the unowned data symbol an `extern` names) a landing accepts "
                        "deliberately, with the claim already scheduled; repeatable, recorded in the "
                        "landing log, never a key in a file")
    v.add_argument("--allow-orphan", action="append", default=[], metavar="ADDR",
                   help="hex address of a data object the data-closure row accepts as deliberately "
                        "unclaimed (see `land --allow-orphan`); repeatable, recorded in the log")
    v.add_argument("--no-selftests", action="store_true", dest="no_selftests",
                   help="skip the all-tool-selftests row (the fast path; `python tools/selftest.py "
                        "--changed` is the narrower lane loop)")
    v.add_argument("--json", action="store_true")
    ld = sub.add_parser("land", help="gate + stage + commit + release; one answer line, exit status is the answer")
    ld.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    ld.add_argument("--units", default=None, help="comma-separated units in this batch")
    ld.add_argument("--branch", default=None,
                    help="land a whole branch: refuse a dirty tree, record-base, apply the branch with the "
                         "registration union, gate, commit and release (derives --units from the branch's "
                         "registration diff when --units is omitted)")
    ld.add_argument("--no-build", action="store_true", help="skip the baseline step")
    ld.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                    help="skip the outbox/branch checks (release still runs)")
    ld.add_argument("--no-release", action="store_true", dest="no_release",
                    help="commit without releasing the batch's claims")
    ld.add_argument("--allow-regression", action="append", default=[],
                    help="unit whose measured regression is authorised by a rule; repeatable")
    ld.add_argument("--allow-rule10", action="append", default=[],
                    help="rule-10 violation key a landing accepts deliberately, e.g. "
                         "`run:.data:805FB0F8` for the Pat vtable the owner ruled stays claimed while "
                         "its slots are written; repeatable, recorded in the landing log, never a key "
                         "in a file")
    ld.add_argument("--unit-rename", action="append", default=[], metavar="OLD=NEW",
                    help="a registered unit this batch renames (git mv): its base-snapshot findings follow the "
                         "new name instead of reading as a new unit's additions; recorded in the landing log")
    ld.add_argument("--allow-orphan", action="append", default=[], metavar="ADDR",
                    help="hex address of a data object (or a byte inside a shrunk claim) the data-closure "
                         "row accepts as deliberately unclaimed; repeatable, recorded in the landing log, "
                         "never a key in a file, and an address that matches nothing keeps the refusal")
    ld.add_argument("--allow-rule12", action="append", default=[], metavar="TOKEN",
                    help="rule-12 token (the unowned data symbol an `extern` names) a landing accepts "
                         "deliberately, with the claim already scheduled; repeatable, recorded in the "
                         "landing log, never a key in a file")
    ld.add_argument("--no-selftests", action="store_true", dest="no_selftests",
                    help="skip the all-tool-selftests row (the fast path; `python tools/selftest.py "
                         "--changed` is the narrower lane loop)")
    ld.add_argument("--message", default=None, help="override the gate message's subject line")
    ld.add_argument("--already-applied", action="store_true", dest="already_applied",
                    help="the batch was applied before `record-base` ran, so its paths are in the base's "
                         "dirty snapshot; stage them anyway (otherwise `land` refuses and says so)")
    sub.add_parser("integrate", help="forward to tools/units/integrate.py (apply the lanes' integrator requests "
                                     "in one batch on an integrate/<date> branch); every argument passes through")
    rs = sub.add_parser("resolve", help="union-resolve a branch's registration append-conflict in a "
                                         "scratch tree (never in MAIN); exit status is the answer")
    rs.add_argument("--branch", required=True, help="the worker branch to resolve")
    rs.add_argument("--worktree", default=None,
                    help="a worktree mid-merge on --branch to resolve in (default: a fresh scratch "
                         "worktree, `git merge main`)")
    rs.add_argument("--main", default=None, help="MAIN worktree (default: resolved with git)")
    rs.add_argument("--base", default=None, help="the merge base (default: git merge-base main <branch>)")
    rs.add_argument("--no-commit", action="store_true",
                    help="resolve and stage the scoped paths but do not commit")
    rs.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    main = common.main_root(common.worktree_root())
    if args.cmd == "record-base":
        # manual entry point: it reads MAIN's tree, so it must refuse a caller that is not in MAIN on main
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED record-base | %s" % bad_branch)
            return 1
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        data = record_base(main, units)
        print(json.dumps(data, indent=2) if args.json else "base %s (%s)" % (data["base"], data["subject"]))
        return 0
    if args.cmd == "verify":
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED verify | %s" % bad_branch)
            return 1
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        set_allow_rule12(args.allow_rule12)
        set_allow_orphan(args.allow_orphan)
        return gate.verify(main, units, args.base, args.dry_run, args.no_build, args.allow_regression,
                      check_outbox=not args.no_outbox, release_claims=not args.no_release,
                      no_selftests=args.no_selftests)
    if args.cmd == "land":
        if args.branch:
            units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
            set_allow_rule10(args.allow_rule10)
            set_allow_rule12(args.allow_rule12)
            set_unit_renames(args.unit_rename, units)
            set_allow_orphan(args.allow_orphan)
            return land_branch(main, args.branch, units=units, base=args.base, no_build=args.no_build,
                               allow_regression=args.allow_regression,
                               check_outbox=not args.no_outbox, release_claims=not args.no_release,
                               subject=args.message, no_selftests=args.no_selftests)
        if not args.units:
            print("REFUSED | land needs --units a,b or --branch worker/<slug>")
            return 1
        units = [u.strip() for u in args.units.split(",") if u.strip()]
        set_allow_rule10(args.allow_rule10)
        set_allow_rule12(args.allow_rule12)
        set_unit_renames(args.unit_rename, units)
        set_allow_orphan(args.allow_orphan)
        return land(main, units, args.base, args.no_build, args.allow_regression,
                    allow_rule10=args.allow_rule10,
                    check_outbox=not args.no_outbox, release_claims=not args.no_release,
                    subject=args.message, already_applied=args.already_applied,
                    no_selftests=args.no_selftests)
    if args.cmd == "resolve":
        main = args.main or common.main_root(common.worktree_root())
        if args.worktree:
            base = args.base or git(["merge-base", "main", args.branch], main).strip()
            result = resolve_conflicts(args.worktree, main, args.branch, base=base,
                                       commit=not args.no_commit)
        else:
            result = scratch_resolve(main, args.branch, base=args.base, commit=not args.no_commit)
        if args.json:
            print(json.dumps(result, indent=2))
        elif result.get("ok"):
            tail = ""
            if result.get("scratch_branch"):
                tail = (" | scratch branch %s in %s (fast-forward the worker branch with `git branch -f "
                        "%s %s` when it is not checked out)"
                        % (result["scratch_branch"], result.get("worktree"), args.branch,
                           result["scratch_branch"]))
            print("RESOLVED %s | %s%s" % (args.branch, result.get("reason"), tail))
        else:
            print("REFUSED %s | %s" % (args.branch, result.get("reason")))
        return 0 if result.get("ok") else 1
    ap.print_help()
    return 0


#: The gate's tests (`--selftest` forwards to them until WP6 retires the flag).
TESTS = ("tools/tests/units/test_land.py",)


def selftest() -> int:
    """`land.py --selftest`: run the re-homed test modules (`tools/tests/units/test_land.py` and
    `tools/tests/units/landing/`), one subprocess each; the first failure's exit status is the answer."""
    tests = list(TESTS) + sorted(
        "tools/tests/units/landing/" + f for f in os.listdir(os.path.join(SELF_REPO, "tools", "tests", "units", "landing"))
        if f.startswith("test_") and f.endswith(".py"))
    code = 0
    for rel in tests:
        p = subprocess.run([sys.executable, os.path.join(SELF_REPO, *rel.split("/"))])
        code = code or p.returncode
    return code
