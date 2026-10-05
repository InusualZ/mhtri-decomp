"""The command line: `--budget`, `--findings`, `--diff REF` and the read-only `--ref BRANCH`.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

from tools.lib.git import Git
from tools.units.stylelint_rules.common import (
    EXEMPT, RULE7_NOTES, Source, UNCHECKED, all_sources, exemptions, read_text, rel_of,
)
from tools.units.stylelint_rules.context import load_ownership
from tools.units.stylelint_rules.r01_shared_type import rule1_findings
from tools.units.stylelint_rules.r13_method import set_rule13_context
from tools.units.stylelint_rules.lint import (
    header_rule12_findings, header_rule2_band_findings, lint_all, lint_tree, rule11_local_total,
    rule13_static_like_total,
)
from tools.units.stylelint_rules.diff import Judgement, added_detail_lines, derive_file_absorbers, judge, renames_of
from tools.units.stylelint_rules.refs import (
    changed_src_files, changed_src_files_between, deleted_src_files, findings_at_ref, findings_of_deleted,
    findings_of_ref, header_rule12_findings_at_ref, load_ownership_at_ref, rule1_findings_at_ref, sources_of_ref,
    unresolved_declarations_at_ref,
)
from tools.units.stylelint_rules.report import (
    budget, print_budget, print_findings, print_findings_listing, rule10_counts, select_findings,
)
import tools.units.stylelint_rules.refs as _refs


# --------------------------------------------------------------------------------------------------
# cli
# --------------------------------------------------------------------------------------------------
SELFTEST_FLAG = ("--selftest",)


def _same_commit(root: str, a: str, b: str) -> bool:
    """Whether two revisions name the same commit - the `--ref <branch>` self-comparison test."""
    try:
        return _refs.git(root, "rev-parse", a).strip() == _refs.git(root, "rev-parse", b).strip()
    except RuntimeError:
        return False


def _fork_point(root: str, branch: str) -> str | None:
    """`branch`'s fork point against `main` - the base `--diff main` would resolve to from MAIN, or None.

    The fallback for `--ref <branch>` run **from the branch's own worktree**: there the branch is an
    ancestor of HEAD by construction (it *is* HEAD), so `_resolve_diff_ref` returns the branch and the
    comparison would judge it against itself.
    """
    return Git(root).fork_point(branch)


def _resolve_diff_ref(root: str, ref: str) -> str:
    """The ref `--diff` actually compares against: REF when it is an ancestor of HEAD, else its merge base.

    `--diff REF` compares the working tree with REF, so when REF is not an ancestor of HEAD the diff also
    contains whatever landed on REF after this branch was cut - which reads as *this* batch's regression.
    Measured 2026-09-27: a lane spent a diagnosis on another lane's landing exactly this way, and the same
    night the orchestrator had to be told `--diff <merge-base>` by hand three times. The merge base is the
    only base that measures *this* batch, so it is selected automatically and named on stderr. The land gate
    passes the batch base, which is an ancestor by construction, so its comparison is untouched.
    """
    if subprocess.run(["git", "merge-base", "--is-ancestor", ref, "HEAD"],
                      cwd=root, capture_output=True).returncode == 0:
        return ref
    base = subprocess.run(["git", "merge-base", ref, "HEAD"], cwd=root,
                          capture_output=True, text=True, encoding="utf-8", errors="replace").stdout.strip()
    if not base:
        print("stylelint: warning: %s is not an ancestor of HEAD and has no merge base with it - comparing "
              "against %s itself" % (ref, ref), file=sys.stderr)
        return ref
    print("stylelint: %s is not an ancestor of HEAD, so the comparison uses the merge base %s - only that "
          "base measures this batch" % (ref, base[:12]), file=sys.stderr)
    return base


def untouched(findings: list[dict], skip: set) -> list[dict]:
    """`findings` minus the files in `skip` - the changed and the deleted paths, whose every rule `lint_source` already
    reports on both sides.

    Every rule of a changed file comes from `lint_source` (headers included since 2026-10-05), so the only whole-tree
    walk a comparison still needs is rule 12 over the **untouched** headers: it is the one reading of an unchanged file
    that a batch can still move, because each side is judged by its own map and a `splits.txt` edit can leave an
    untouched header's `extern` uncovered. The rule 11, 13 and 14 walks were the same text on both sides for an
    untouched file, so they never contributed an identity and are gone.
    """
    return [f for f in findings if f["file"] not in skip]


def ref_comparison(root: str, branch: str, ownership: "Ownership | None", as_json: bool,
                   list_added: bool = False) -> int:
    """Judge a **held branch** read-only: its committed tree against the merge base `--diff` would use.

    `--diff REF` compares the *working tree* with REF, so it cannot judge a branch that is not checked out -
    the `after` side would be whatever the worktree happens to hold, and a lane had to re-implement this
    comparison in ~40 lines of scratch (2026-09-28) to prove a `splits.txt` claim cleared a held branch's
    rows.  `--ref B` reads **both** sides from git objects: `B` is the `after` tree and
    `_resolve_diff_ref(root, B)` (the merge base, resolved exactly as `--diff` resolves a non-ancestor REF)
    is the `before`.  Nothing is checked out and nothing is written; each side is judged by the map it was
    written against, exactly as `--diff` judges them.

    **It can be run from the branch's own worktree, and a self-comparison is repaired or refused.**
    `_resolve_diff_ref(root, branch)` returns the branch itself when the branch is an ancestor of HEAD -
    trivially true when HEAD *is* the branch, which is exactly where a review lane runs (`--ref <the branch
    it is reviewing>`).  The old behaviour then compared the branch with itself and printed "adds no
    section 6.5 violation over 0 changed file(s)" with exit 0: a check that measured nothing, reported as
    clean - a **false green for every review's lint row** (2026-09-28).  When the resolved base equals the
    branch the fork point against `main` is used instead; and if the comparison still finds no changed
    file, it **refuses** with a non-zero exit rather than printing "clean".  The refusal's reason names the
    **resolved** base (never the branch), so it cannot contradict the stderr line that resolved it, and
    **exit 2 keeps meaning "nothing was measured"** - 3 is left unclaimed should a git failure ever need a
    code of its own.
    """
    base = _resolve_diff_ref(root, branch)
    self_ref = _same_commit(root, base, branch)
    if self_ref:
        fork = _fork_point(root, branch)
        if fork:
            print("stylelint: --ref %s resolved to the branch itself (it is HEAD here or an ancestor of "
                  "it); judging against the fork point with main, %s, instead"
                  % (branch, fork[:12]), file=sys.stderr)
            base = fork
    pairs = changed_src_files_between(root, base, branch)
    rels = [after for _before, after in pairs]
    if not rels:
        # A CHECK THAT MEASURES NOTHING MUST NEVER PRINT "CLEAN": that is how the self-comparison above
        # passed a review lane's lint row.  Refuse loudly instead of exiting 0.  The **reason comes from the
        # resolved base**, not from `self_ref`: when the fork point was resolved the comparison *did* run
        # against it, so "there is nothing to compare it against" would contradict the stderr line above.
        why = ("no source file changed between %s and %s" % (base[:12], branch) if base != branch else
               "%s is HEAD here (or an ancestor of it), so there is nothing to compare it against" % branch)
        print("REFUSED: --ref %s judged 0 changed file(s) - %s.\n"
              "  run it from MAIN, or pass the base explicitly: --diff <merge-base %s main>"
              % (branch, why, branch))
        return 2
    base_ownership = load_ownership_at_ref(root, base) or ownership
    after_ownership = load_ownership_at_ref(root, branch) or ownership
    base_findings = findings_at_ref(root, base, pairs, base_ownership)
    base_gaps = unresolved_declarations_at_ref(root, base, pairs, base_ownership)
    # the renames between the base and the branch, base-path -> branch-path: the whole-tree header walks
    # read the *base* side, so a renamed header must be keyed by the path the branch spells (a rename alone
    # is not an addition), exactly as `--diff` does it.
    rename = renames_of(pairs)
    deleted = deleted_src_files(root, base, branch)
    skip = set(rels) | set(deleted)
    before_findings = (
        base_findings
        + rule1_findings_at_ref(root, base, pairs)
        + untouched(header_rule12_findings_at_ref(root, base, base_ownership, rename), skip)
        + findings_of_deleted(root, base, deleted, base_ownership))
    touched = findings_of_ref(root, branch, pairs, after_ownership)
    after_sources = sources_of_ref(root, branch, pairs)
    after_findings = (
        touched
        + rule1_findings_at_ref(root, branch, [])
        + untouched(header_rule12_findings_at_ref(root, branch, after_ownership), skip))
    j = judge(before_findings, after_findings, touched, base_findings, base_ownership, after_ownership, base_gaps,
              after_sources, rename,
              derive_file_absorbers(root, base, branch, pairs, deleted_src_files(root, base, branch)))
    return report_comparison(j, rels, as_json, list_added, {"ref": branch, "base": base}, branch,
                             "stylelint: %s adds no section 6.5 violation over %d changed file(s) (read-only: judged "
                             "against %s, nothing checked out)" % (branch, len(rels), base[:12]))


def report_comparison(j: "Judgement", rels: list[str], as_json: bool, list_added: bool, head: dict, subject: str,
                      clean: str) -> int:
    """Print one comparison's verdict - the JSON payload (`head` first), the `+N rule R` rows with the credit lines
    and `--list-added`'s detail, or the clean line - and return its exit code (1 when anything was added)."""
    if as_json:
        print(json.dumps(dict(head, **{
            "added": j.added, "detail": j.detail, "moved": j.moves, "changed": rels,
            "rename_credits": [{"rule": r, "file": p, "count": n, "stopped_spelling": sorted(j.freed_gaps.get(p, ()))}
                               for (r, p), n in sorted(j.credits.items())],
            "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
            "exempt": exemptions()}), indent=2))
    elif j.added:
        print("stylelint: %s adds %d section 6.5 violation(s) over %d changed file(s):"
              % (subject, sum(a["added"] for a in j.added), len(rels)))
        for a in j.added:
            print("  +%d rule %d  %s  (%d -> %d)" % (a["added"], a["rule"], a["file"], a["before"], a["after"]))
        for line in j.credit_lines:
            print(line)
        if list_added:
            print("  added findings, by rule then file (biggest file first):")
            for line in added_detail_lines(j.detail):
                print(line)
        for num, what in UNCHECKED:
            print("  not checked (cross-file): rule %d - %s" % (num, what))
        for rule, prefix, why in EXEMPT:
            print("  not enforced: rule %d under %s (%s)" % (rule, prefix, why))
        for cond, why in RULE7_NOTES:
            print("  not enforced: rule 7 for %s (%s)" % (cond, why))
    else:
        print(clean)
        for line in j.credit_lines:
            print(line)
    return 1 if j.added else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Lint src/ against docs/plan.md 6.5 (roadmap 7.21).")
    ap.add_argument("--diff", metavar="REF",
                    help="fail only if the working tree adds a violation relative to REF")
    ap.add_argument("--ref", metavar="BRANCH",
                    help="read-only: judge the named branch's committed tree against its merge base, so a "
                         "held branch can be checked without checking it out (same comparison as --diff)")
    ap.add_argument("--budget", action="store_true", help="report the backlog per unit over src/")
    ap.add_argument("--headers", action="store_true",
                    help="with --budget: add the unsplit band's (src/unsplit/) rule-2 reading to the table (the "
                         "declarations of a symbol a registered unit already owns). Additive: without it "
                         "the table is byte-identical to before")
    ap.add_argument("--list-added", action="store_true",
                    help="with --diff/--ref: also name each added finding (rule, file, line and the "
                         "identifier/token), grouped by rule then file, biggest file first")
    ap.add_argument("--findings", action="store_true",
                    help="list each finding (file:line, rule, token) of the --diff set (the added ones) or, "
                         "without --diff, the whole-tree --budget set; --json gives the lib.findings schema")
    ap.add_argument("--path", action="append", metavar="GLOB",
                    help="with --findings: keep a file matching GLOB (fnmatch; no wildcard = a prefix); repeatable")
    ap.add_argument("--rule", action="append", type=int, metavar="N",
                    help="with --findings: keep rule N; repeatable")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    # spelled through a constant: `stylelint.py` is the entry point `tools/selftest.py` discovers (it registers the
    # selftest with lib.cli), and this package module must not look like a second one
    ap.add_argument(*SELFTEST_FLAG, action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        from tools.units.stylelint_rules import selftest as _selftest  # noqa: PLC0415 - only on --selftest
        return _selftest.selftest()

    if args.ref is not None and args.diff is not None:
        ap.error("--ref and --diff are two different comparisons; pass one")
    if (args.path or args.rule) and not args.findings:
        ap.error("--path and --rule filter the --findings listing; pass --findings")
    if args.findings and args.ref is not None:
        ap.error("--findings lists the --diff or the --budget set; --ref has --list-added")

    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True, encoding="utf-8", errors="replace")
    root = root.stdout.strip() if root.returncode == 0 else os.getcwd()
    ownership = load_ownership(root)
    set_rule13_context(root)

    if args.ref is not None:
        return ref_comparison(root, args.ref, ownership, args.json, args.list_added)

    if args.diff is not None:
        args.diff = _resolve_diff_ref(root, args.diff)
        try:
            pairs = changed_src_files(root, args.diff)
            rels = [after for _before, after in pairs]
            # Each side against its own map: the working tree's index cannot resolve a name this batch
            # renamed, so using it here turned every rename into a batch of phantom rule-2 additions.
            base_ownership = load_ownership_at_ref(root, args.diff) or ownership
            base_findings = findings_at_ref(root, args.diff, pairs, base_ownership)
            # the renames in this batch, ref-path -> now-path: the whole-tree header walks below read the
            # *ref* tree, so without this a header renamed by the batch keys its existing finding under the
            # old path and `diff_deltas` reads it as an addition (rule 11's six on one rename, rule 12's
            # one) even though the per-file walks already key by the new path.
            rename = renames_of(pairs)
            # every source of the `before` count is kept as findings, not only counts: `--list-added`
            # subtracts the base side's occurrences, and the count is `rule_counts` of the same list.
            deleted = deleted_src_files(root, args.diff)
            skip = set(rels) | set(deleted)
            before_findings = (
                base_findings
                + rule1_findings_at_ref(root, args.diff, pairs)
                + untouched(header_rule12_findings_at_ref(root, args.diff, base_ownership, rename), skip)
                + findings_of_deleted(root, args.diff, deleted, base_ownership))
            base_gaps = unresolved_declarations_at_ref(root, args.diff, pairs, base_ownership)
        except RuntimeError as exc:
            print("stylelint: %s" % exc, file=sys.stderr)
            return 2
        # `lint_tree` covers the changed files only, which is exactly what a credit may consider: a finding
        # in a file the batch did not touch can never be one of its additions.  Kept as a list (not just
        # its counts) because a credit is a judgement about a *finding* - its symbol and its resolution -
        # and `diff_deltas` alone cannot tell a completed rename from a newly foreign declaration.
        touched = lint_tree(root, [os.path.join(root, a) for _b, a in pairs], ownership)
        # the sources are rebuilt the way `lint_tree` builds them, so `rel` (the key both sides are compared by) is
        # spelled identically
        after_sources = [Source(os.path.join(root, a), rel_of(root, os.path.join(root, a)),
                                read_text(os.path.join(root, a))) for _b, a in pairs]
        after_findings = (
            touched
            + rule1_findings(all_sources(root))
            + untouched(header_rule12_findings(root, ownership), skip))
        j = judge(before_findings, after_findings, touched, base_findings, base_ownership, ownership, base_gaps,
                  after_sources, rename,
                  derive_file_absorbers(root, args.diff, None, pairs, deleted_src_files(root, args.diff)))
        if args.findings:
            print_findings_listing(select_findings(j.detail, args.path, args.rule), "added (--diff %s)" % args.diff,
                                   args.json, args.path, args.rule)
            return 1 if j.added else 0
        return report_comparison(
            j, rels, args.json, args.list_added, {"ref": args.diff}, "the batch",
            "stylelint: no new section 6.5 violation over %d changed file(s) "
            "(rule 2 resolves every extern to an owner or the unsplit band; rule 7 fires on every "
            "auto-generated name; rule 11 fires on every unmarked `void *` parameter/return type; "
            "rule 12 fires on every `extern` of unowned data; only pre-existing findings and a "
            "rename's referrer half are grandfathered)" % len(rels))

    if args.headers and not (args.budget or args.findings):
        ap.error("--headers describes the --budget table; pass --budget (or --findings)")
    findings = lint_all(root, ownership)
    if args.headers:
        # the band's rule-2 column, which `lint_all` cannot see (see `band_rule2_findings`). Only the
        # rule-2 reading is added: the band's rule 12 is already in `lint_all` via `header_rule12_findings`.
        findings = findings + header_rule2_band_findings(root, ownership)
        findings.sort(key=lambda f: (f["rule"], f["file"], f["line"]))
    if args.findings:
        print_findings_listing(select_findings(findings, args.path, args.rule),
                               "budget" + (" + band headers" if args.headers else ""), args.json, args.path,
                               args.rule)
        return 0
    rule10, rule10_note = rule10_counts(root) if (args.json or args.budget) else (None, "")
    if args.json:
        print(json.dumps({"budget": budget(findings, rule10),
                          "rule11_locals": rule11_local_total(root),
                          "rule13_static_like": rule13_static_like_total(root),
                          "rule2_gaps": dict(ownership.gaps) if ownership else {},
                          "rule2_unsplit_modules": (
                              {m: {"sites": n, "symbols": len(ownership.unsplit_symbols.get(m, ()))}
                               for m, n in ownership.unsplit_modules.items()} if ownership else {}),
                          "unchecked": [{"rule": n, "why": w} for n, w in UNCHECKED],
                          "exempt": exemptions()}, indent=2))
    elif args.budget:
        print_budget(findings, ownership, root, rule10, rule10_note)
    else:
        print_findings(findings)
    return 0
