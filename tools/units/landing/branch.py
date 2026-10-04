"""`land --branch` apply and `resolve`: the branch's merge-base diff, the registration union, the resolve helpers.
Spec: docs/tools/spec/landing.md. CLI: none (a module of the `land.py` gate)."""
from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile

from tools.lib.lanes import naming
from tools.lib.lanes import teardown
import tools.units.unionguard as ug
import tools.units.unionresolve as ur
from tools.units.landing.common import git, run


# --------------------------------------------------------------------------------------------------
# The registration append-conflict resolver (docs/plan.md 7.5; the 2026-09-26 merger class).
#
# Eight of thirteen live branches conflict with `main` on exactly one class: sibling bands claim adjacent
# address ranges, so both sides append a block at the same anchor in `config/RMHE08/splits.txt` and a line
# in `configure.py`'s `Object(...)` list.  The resolution is the pure append-union - and it is the one
# union that is safe: `unionguard` proves both sides only *inserted*, and `unionresolve` asserts the
# invariants a wrong union breaks silently.
#
# `resolve` runs in the batch's worktree or a **scratch tree**, never in MAIN.  `land --branch` applies the
# branch's own delta inside MAIN (that is the landing) but uses the same union, which is why the union
# lives in `_union_conflicts` with the MAIN guard in `resolve_conflicts` around it.
#
# Staging is explicit (`git add -- <the two scoped paths>`), never `git add -A`: that sweep - the tail of
# an earlier landing script's tail - staged `d910.json` and has bitten the campaign twice.
# --------------------------------------------------------------------------------------------------

UNION_SCOPE = ur.UNION_SCOPE


def _resolve_result(ok: bool, reason: str, **extra) -> dict:
    out = {"ok": ok, "reason": reason}
    out.update(extra)
    return out


def _tree_text(tree: str, ref: str, rel: str) -> str:
    """`git show <ref>:<rel>` read as text, or "" when the path is absent at that ref."""
    p = run(["git", "show", "%s:%s" % (ref, rel)], tree)
    return p.stdout if p.returncode == 0 else ""


def _union_conflicts(tree: str, branch: str, base: str | None = None, paths: list[str] | None = None,
                     commit: bool = False, runner=subprocess.run) -> dict:
    """Union the in-scope registration conflict in `tree` (the MAIN guard is in `resolve_conflicts`).

    Three ordered gates, any of which refuses: **scope** (every conflicted path must be `configure.py` or
    `config/RMHE08/splits.txt`), **unionguard** (a disjoint addition, not a delete/rename/overlap), and
    the **invariants** (`ur.check_union`).  The union is computed in memory and the invariants asserted
    *before* anything is written, so a union that duplicates a key, overlaps a range or drops a
    registration never reaches the tree.  `ur.union_text_full` also reports a **prose** hunk whose two
    sides neither carried the other (a comment paragraph both sides rewrote, with no superset): unioning
    it would duplicate the prose and reintroduce generated names, so it is refused here rather than
    written - the scope gate already keeps the registration class (`configure.py`/`splits.txt`) a pure
    declaration union.
    """
    try:
        stages = ug.unmerged(tree)
    except RuntimeError as exc:
        return _resolve_result(False, "%s is not a git tree: %s" % (tree, exc))
    conflicted = sorted(paths if paths is not None else stages)
    if not conflicted:
        return _resolve_result(False, "no unmerged path in %s - nothing to resolve" % tree)
    foreign = [p for p in conflicted if p not in UNION_SCOPE]
    if foreign:
        return _resolve_result(
            False,
            "conflict outside the registration scope: %s - a header or `src/**` conflict is a real "
            "content conflict; unioning it stacks `#ifdef`/`#endif` and shifts struct offsets. Resolve "
            "it by hand." % ", ".join(foreign), scope=list(UNION_SCOPE))
    if base:
        ours_renames = ug.rename_sets(tree, base, branch)
        theirs_renames = ug.rename_sets(tree, base, "main")
    else:
        ours_renames = theirs_renames = (set(), set())
    unsafe = []
    for path in conflicted:
        finding = ug.classify(tree, path, stages.get(path, {}), ours_renames, theirs_renames)
        if finding["unsafe"]:
            unsafe.append("%s (%s)" % (path, ", ".join(finding["reasons"])))
    if unsafe:
        return _resolve_result(
            False,
            "unionguard refused, the conflict is not a disjoint addition: %s - resolve by hand, do not "
            "union them" % "; ".join(unsafe))

    # Compute every union in memory first; a violation must not touch the tree.
    merged_texts: dict[str, str] = {}
    for path in conflicted:
        work = os.path.join(tree, *path.replace("/", os.sep).split(os.sep))
        try:
            with open(work, encoding="utf-8", newline="") as fh:
                text = fh.read()
        except OSError as exc:
            return _resolve_result(False, "cannot read %s: %s" % (path, exc))
        merged, hunks, decisions = ur.union_text_full(text, path)
        if hunks == 0 or "<<<<<<<" in merged or ">>>>>>>" in merged:
            return _resolve_result(False, "%s carries no conflict block to union" % path)
        blocked = [d for d in decisions if d.get("blocked")]
        if blocked:
            return _resolve_result(
                False,
                "%s carries a prose conflict hunk with no superset (hunk %d: %s) - unioning it would "
                "duplicate the prose and reintroduce generated names; resolve it by hand"
                % (path, blocked[0]["hunk"], blocked[0]["why"]))
        merged_texts[path] = merged

    main_splits = _tree_text(tree, "main", "config/RMHE08/splits.txt")
    main_configure = _tree_text(tree, "main", "configure.py")
    merged_splits = merged_texts.get("config/RMHE08/splits.txt", main_splits)
    merged_configure = merged_texts.get("configure.py", main_configure)
    violations = ur.check_union(main_splits, merged_splits, main_configure, merged_configure)
    if violations:
        # The union looked plausible but broke an invariant - the case a green build cannot see. Refuse
        # and name every violation; nothing was written.
        return _resolve_result(False, "the union would break %d invariant(s): %s"
                               % (len(violations), "; ".join(violations)), violations=violations)

    for path, merged in merged_texts.items():
        work = os.path.join(tree, *path.replace("/", os.sep).split(os.sep))
        with open(work, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(merged)
    # the two scoped paths, explicitly - never `git add -A`.
    git(["add", "--", *conflicted], tree)
    committed = None
    if commit:
        p = runner(["git", "commit", "-q", "-m",
                    "resolve: union the registration append-conflict on %s" % branch], cwd=tree,
                   capture_output=True, text=True, encoding="utf-8", errors="replace")
        if p.returncode != 0:
            tail = ((p.stdout or "") + (p.stderr or "")).strip().splitlines()
            return _resolve_result(False, "the union is sound but the commit failed: %s"
                                   % (tail[-1] if tail else "unknown error"), paths=conflicted)
        committed = git(["rev-parse", "--short", "HEAD"], tree).strip()
    return _resolve_result(True, "union-resolved %d path(s)" % len(conflicted), paths=conflicted,
                           commit=committed)


def resolve_conflicts(tree: str, main_tree: str, branch: str, base: str | None = None,
                      paths: list[str] | None = None, commit: bool = False,
                      runner=subprocess.run) -> dict:
    """`_union_conflicts`, but it refuses to run inside MAIN - the batch's worktree or a scratch tree only.

    MAIN must stay clean while a branch is resolved: the merger lane's whole advantage is that resolving
    two registrations never touches the gate's own tree, and a gate that applied a diff inside MAIN is what
    left MAIN mid-conflict on 2026-09-26.
    """
    if os.path.abspath(tree) == os.path.abspath(main_tree):
        return _resolve_result(False, "refusing to resolve inside MAIN - use the batch's worktree or a "
                                     "scratch tree (`git worktree add`), never the gate's own tree")
    return _union_conflicts(tree, branch, base, paths=paths, commit=commit, runner=runner)


def scratch_resolve(main_tree: str, branch: str, base: str | None = None, commit: bool = True,
                    runner=subprocess.run) -> dict:
    """`resolve_conflicts` on a fresh scratch worktree: `git worktree add` + `git merge main`.

    A temporary worktree on a temporary branch at the branch's tip (so the branch itself is not disturbed
    and `main` stays clean), then `git merge --no-commit main` - exactly the merger lane's hand operation,
    with ours = the branch.  A clean merge has no conflict to resolve and is reported as such; a conflicted
    one goes through `resolve_conflicts`.  The scratch worktree is removed on refusal and kept on success,
    so the caller can fast-forward the branch (`git branch -f <branch> <scratch-branch>`).
    """
    if git(["rev-parse", "--verify", "-q", branch], main_tree, check=False).strip() == "":
        return _resolve_result(False, "no such branch: %s" % branch)
    base = base or git(["merge-base", "main", branch], main_tree).strip()
    tmp = tempfile.mkdtemp(prefix="land-resolve-")
    slug = re.sub(r"[^A-Za-z0-9._-]+", "-", branch.split("/", 1)[-1])
    scratch_branch = "land/resolve-%s-%d" % (slug, os.getpid())
    suffix = 1
    while git(["rev-parse", "--verify", "-q", scratch_branch], main_tree, check=False).strip():
        suffix += 1
        scratch_branch = "land/resolve-%s-%d-%d" % (slug, os.getpid(), suffix)
    p = runner(["git", "worktree", "add", "-b", scratch_branch, tmp, branch], cwd=main_tree,
               capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        return _resolve_result(False, "git worktree add failed: %s"
                               % ((p.stderr or p.stdout or "").strip().splitlines() or [""])[-1])

    def discard(reason: str, **extra) -> dict:
        runner(["git", "worktree", "remove", "--force", tmp], cwd=main_tree, capture_output=True)
        runner(["git", "branch", "-D", scratch_branch], cwd=main_tree, capture_output=True)
        return _resolve_result(False, reason, **extra)

    merge = runner(["git", "merge", "--no-commit", "main"], cwd=tmp, capture_output=True, text=True, encoding="utf-8",
                   errors="replace")
    if merge.returncode == 0 and not ug.unmerged(tmp):
        return discard("the branch merges `main` cleanly - no registration conflict to resolve")
    if not ug.unmerged(tmp):
        tail = ((merge.stdout or "") + (merge.stderr or "")).strip().splitlines()
        return discard("`git merge main` failed without a conflict: %s"
                       % (tail[-1] if tail else "unknown error"))
    result = resolve_conflicts(tmp, main_tree, branch, base=base, commit=commit, runner=runner)
    if not result.get("ok"):
        return discard(result.get("reason"), **{k: v for k, v in result.items()
                                                if k not in ("ok", "reason")})
    result["worktree"] = tmp
    result["scratch_branch"] = scratch_branch
    return result


# --------------------------------------------------------------------------------------------------
# The resolve helper's teardown: `scratch_resolve` parks a registration union on a
# `land/resolve-<slug>-<pid>` helper branch (in its own scratch worktree) so the caller can fast-forward
# the worker branch onto it.  Once the branch lands, the helper is debris - and a `land/*` ref left
# behind is a false statement about the branch's state (two were left from 2026-09-26).  The landing
# deletes the helpers it can *prove* redundant - the helper's tip is already contained by the branch it
# resolved or by `main` - and refuses loudly on any other, because a hand fix made on the helper
# (2026-09-26: the `u32 mode` repair lived only on `land/resolve-8030681c-...-31048`) would be the only
# copy.  Nothing is deleted until the landing has succeeded.
# --------------------------------------------------------------------------------------------------

def resolve_helper_slug(branch: str) -> str:
    """The slug `scratch_resolve` names a branch's helper with (`worker/x` -> `x`)."""
    return re.sub(r"[^A-Za-z0-9._-]+", "-", branch.split("/", 1)[-1])


def resolve_helper_refs(main: str, branch: str) -> list[str]:
    """Every `land/resolve-<slug>-<pid>[-<n>]` helper ref that `branch` owns, matched by exact slug.

    The trailing `-<pid>[-<n>]` is what keeps `worker/foo` from claiming `worker/foo-bar`'s helper: a
    bare prefix match would sweep a sibling's resolution.
    """
    pat = re.compile(r"^refs/heads/land/resolve-%s-[0-9]+(-[0-9]+)?$" % re.escape(resolve_helper_slug(branch)))
    out = git(["for-each-ref", "--format=%(refname)", "refs/heads/land/"], main, check=False)
    return [line.strip() for line in out.splitlines() if pat.match(line.strip())]


def _is_ancestor(main: str, ancestor: str, descendant: str) -> bool:
    """True when `ancestor` is reachable from `descendant` - the one containment proof the sweep uses."""
    return run(["git", "merge-base", "--is-ancestor", ancestor, descendant], main).returncode == 0


def resolve_helper_state(main: str, branch: str) -> tuple[list[str], list[str]]:
    """Classify `branch`'s resolve helpers -> (provably redundant, refused).

    Redundant: the helper's tip is contained by `branch` or by `main`, so deleting it cannot lose the
    work.  Refused: anything else - the helper may hold a hand fix the branch never took.
    """
    redundant: list[str] = []
    refused: list[str] = []
    for ref in resolve_helper_refs(main, branch):
        tip = git(["rev-parse", ref], main, check=False).strip()
        if tip and (_is_ancestor(main, tip, branch) or _is_ancestor(main, tip, "main")):
            redundant.append(ref)
        else:
            refused.append(ref)
    return redundant, refused


def _resolve_helper_worktree(main: str, ref: str) -> str | None:
    """The worktree a helper ref is checked out in, or None."""
    out = git(["worktree", "list", "--porcelain"], main, check=False)
    wt = None
    for line in out.splitlines():
        if line.startswith("worktree "):
            wt = line[len("worktree "):].strip()
        elif line.startswith("branch ") and wt and line[len("branch "):].strip() == ref:
            return wt
    return None


def delete_resolve_helper(main: str, ref: str) -> str:
    """Remove a redundant helper's scratch worktree and delete its branch; -> the branch name deleted."""
    branch = ref[len("refs/heads/"):]
    wt = _resolve_helper_worktree(main, ref)
    if wt and os.path.isdir(wt):
        teardown.remove_worktree(wt, main)
    git(["worktree", "prune"], main, check=False)
    git(["branch", "-D", branch], main)
    return branch


def _sweep_resolve_helpers(main: str, branch: str, redundant: list[str], refused: list[str]) -> None:
    """Delete the provably-redundant helper(s) and name every refusal, in the landing output."""
    for ref in redundant:
        try:
            delete_resolve_helper(main, ref)
            print("    resolve helper %s deleted (its resolution landed with %s)" % (ref, branch))
        except SystemExit as exc:
            print("    REFUSING to delete resolve helper %s: %s" % (ref, exc), file=sys.stderr)
    for ref in refused:
        print("    REFUSING to delete resolve helper %s: its tip is not contained by %s or main - it may "
              "hold the only copy; fast-forward %s to it and re-run" % (ref, branch, branch),
              file=sys.stderr)


# --------------------------------------------------------------------------------------------------
# The one-command landing: `land.py land --branch worker/<slug>`.
# --------------------------------------------------------------------------------------------------

def claim_unit_for_branch(main: str, branch: str) -> str | None:
    """The registry key whose claim records `branch`, or None.

    A unit renamed at registration keeps the *pre-registration* claim key while the name the gate compiles
    and the outbox lookup use differ.  The branch is the claim's identity, so the key is read from the
    branch rather than re-derived from the unit path - which is what makes the release of a renamed unit
    work.
    """
    try:
        registry = registry.load(main)
    except Exception:
        return None
    for key, record in registry.items():
        if isinstance(record, dict) and record.get("branch") == branch:
            return key
    return None


def units_from_branch(main: str, branch: str, base: str) -> list[str]:
    """The units a branch registers, read from the branch's own registration diff.

    `land --branch` must know which objects to compile, and a registration rename means the branch name
    cannot always be turned back into the registered unit (the outbox slug is the pre-registration name).
    The branch's diff is authoritative: its added `Object(...)` lines and `splits.txt` unit headers name the
    units it registers, in file order.  Returns normalised (extensionless) unit names, de-duplicated.
    """
    p = run(["git", "diff", base, branch, "--", "configure.py", "config/RMHE08/splits.txt"], main)
    found: list[str] = []
    for line in (p.stdout or "").splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        body = line[1:]
        found.extend(naming.norm_unit(n) for n in ur.object_names(body))
        found.extend(naming.norm_unit(u) for u in ur.split_units(body))
    seen: set[str] = set()
    out: list[str] = []
    for unit in found:
        unit = unit.strip("/")
        if unit and unit not in seen:
            seen.add(unit)
            out.append(unit)
    return out


def apply_branch(main: str, branch: str, base: str | None = None, conflicts: list[str] | None = None
                 ) -> tuple[bool, str, str]:
    """Apply the branch's own delta to MAIN with three-way; resolve a registration conflict.

    -> (ok, reason, base).  `git diff --binary <merge-base> <branch>` is exactly the branch's own work (a
    merged-in `main` cancels out), which is why it is used instead of a cherry-pick: cherry-picking a
    branch that merged `main` silently drops the work the merge carried.  A clean apply is done; a conflict is sent to the scoped union,
    and a refusal undoes the apply so the tree is exactly as it was found. `conflicts`, when given, receives the
    paths the apply left unmerged (the landing log's `conflicts`).
    """
    base = base or git(["merge-base", "main", branch], main).strip()
    patch = subprocess.run(["git", "diff", "--binary", base, branch], cwd=main,
                           capture_output=True).stdout
    if not patch.strip():
        return False, "the branch has no diff against %s - nothing to land" % base[:8], base
    ap = subprocess.run(["git", "apply", "-3", "-"], cwd=main, input=patch, capture_output=True)
    stages = ug.unmerged(main)
    if not stages:
        if ap.returncode == 0:
            return True, "applied cleanly", base
        tail = ap.stderr.decode("utf-8", "replace").strip().splitlines()
        return False, "git apply failed: %s" % (tail[-1] if tail else "unknown error"), base
    if conflicts is not None:
        conflicts.extend(sorted(stages))
    result = _union_conflicts(main, branch, base, paths=sorted(stages))
    if not result.get("ok"):
        ug.cleanup_applied(main, base, branch)
        return False, result.get("reason"), base
    return True, "applied with the registration union (%s)" % result.get("reason"), base


def undo_apply(main: str, base: str, branch: str) -> None:
    """Undo `apply_branch` (`unionguard.cleanup_applied`): a refused landing leaves MAIN exactly as it was found."""
    ug.cleanup_applied(main, base, branch)
