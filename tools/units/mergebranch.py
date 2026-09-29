#!/usr/bin/env python3
"""Bring `main` into a held lane branch, resolve the conflicts **by class**, prove the result, commit it.

    python tools/units/mergebranch.py resolve [--branch B] [--dry-run] [--json]
    python tools/units/mergebranch.py status
    python tools/units/mergebranch.py selftest

Run it **inside the lane's own worktree**: it merges `main` into the current branch and commits the
resolution there, so the lane keeps working and the orchestrator still lands one branch with `land.py`.

**Why this exists.** A proposal branch is cut from main and works for hours; by the time it is finished
main has moved, so `land.py` refuses ("it will not union a header") and the lane has to merge by hand.
Measured 2026-09-27: three lanes, ~1.5 h of hand work, and **content lost twice** - not from a hard
conflict but from the two failure modes this tool is built against:

* **`git apply --3way` is the wrong tool.** It refuses with "does not match index" as soon as the working
  tree differs from the index, and it resolves a *conflict-marked* merge in whatever way it fancies. The
  correct primitive is `git merge-file -p --diff3 ours base theirs`, which is a real three-way merge and
  names the three sides explicitly.
* **Never infer "already resolved" from the absence of markers.** After a crash that leaves a file checked
  out from main, a marker-based guard skips it - and `src/…`, driven that way, silently dropped 157 header
  lines and a whole unit registration. This tool records the conflicted-path list **before** touching
  anything and requires every path on it to be *verifiably* represented, by content.
* **A merge in progress is not a dirty tree.** The merge itself stages every auto-merged file and leaves
  the conflicted paths unmerged, so a `status --porcelain` guard run *before* the merge is detected
  refuses the re-run this tool's own failure message tells the operator to make (2026-09-29: a resolution
  that crashed on an add/add path could not be resumed at all). The merge is checked **first**, and a
  resume is allowed exactly when this tool's state file describes it and nothing *outside* the merge has
  been touched.
* **An add/add path has no three-way base, and that is a class, not an accident.** A path added on both
  sides (a re-home on each side, or two files landing on one name) has no blob in the merge base, so
  `git merge-file` cannot run on it at all. Each side is diffed against the base of the path it was
  **renamed from** - for a re-home that pre-rename path is the true three-way base - and the side that is
  the **superset** wins; with no base to derive, the two copies are compared whole. Neither being the
  superset is a refusal, and whichever side was taken is named in the output and in the commit.

**Resolution is per class, because each class has exactly one correct rule:**

| conflicted path | rule |
|---|---|
| `config/**/symbols.txt` | main's file, then the branch's own rename pairs re-applied as **exact row replacements** - a symbol map is address-ordered, so a textual union reorders it and renames nothing |
| `src/**` | whichever side already carries the other side's work (the branch's edit there is usually a rename sweep, and main's file may already hold it). If neither does, a **comment-only** delta (identical code after `stylelint.strip`) keeps main's block and records the branch's dropped paragraph; otherwise **refuse** and name what is missing |
| an **add/add** path (no blob in the merge base) | each side diffed against the base of the path *it was renamed from*; with no such base, the **superset** of the two copies - and the side taken is always named, never guessed |
| an unsplit band header (`include/unsplit/*`) | a three-way union, then the **rule-2 address sweep**: a declaration whose address is inside a registered `.text` range belongs to that unit's header, and where it should move is reported |
| anything else | a three-way union |

Every resolution is then checked before the commit: no conflict markers, each conflicted path's branch
side present (the **code** for a comment-only resolution), every `fn_XXXXXXXX`/`lbl_XXXXXXXX` the branch
sources still *call* still in the map (a name that is part of a unit file's **path**, or one the map cannot
resolve at its address, is not a symbol reference - both false positives cost a hand merge on 2026-09-28),
and - when the tooling is importable - `land.py`'s own pre-flight rows (rule 7 growth, band ownership,
registration) plus the affected units' compile, which is the only check that sees a `NonMatching` unit's
object.
"""
from __future__ import annotations

import argparse
import contextlib
import io
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_TOOLS = os.path.dirname(HERE)                       # tools/
# `stylelint.strip` is the one comment/literal stripper in the repository: import it rather than write a
# second one (a second stripper drifts and the two then disagree about what a comment is).  A comment-only
# `src/**` conflict is resolved by comparing the stripped *code*.
if HERE not in sys.path:
    sys.path.insert(0, HERE)
from stylelint import strip  # noqa: E402
STATE = ".pi/merge-state.json"
MARKERS = ("<<<<<<<", "|||||||", ">>>>>>>")
RENAME = re.compile(r"\b(fn|lbl|loc)_([0-9A-Fa-f]{8})\b")
# A generated name followed by a file extension is a file name, not a symbol: `fn_805113B0.c` / `.o`.
PATH_EXT = re.compile(r"\.(?:c|cpp|cc|cp|h|hpp|hh|o|obj|d|s|asm|txt|json|map|md)\b")
# A `symbols.txt` row: the name, its section (`.text`, `@`-prefixed extab, ...) and its address.
MAP_ROW = re.compile(r"^\s*([^\s=]+)\s*=\s*([^:\s]+):(0x[0-9A-Fa-f]+)", re.M)
ADDR_COMMENT = re.compile(r"/\*\s*0x([0-9A-Fa-f]{8})")
DECL_HEAD = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?(?:const\s+)?[A-Za-z_][\w:<> \*&]*?\b\w+\s*\(")
SPLITS_UNIT = re.compile(r"^(\S+):\s*$")
SPLITS_TEXT = re.compile(r"^\s*\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)")


# ---------------------------------------------------------------------------------------------------
# git plumbing (bytes in / bytes out: a merge must never re-encode a file)
# ---------------------------------------------------------------------------------------------------
def git(root: str, *args: str, check: bool = False) -> str:
    p = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if check and p.returncode != 0:
        raise SystemExit("git %s failed: %s" % (" ".join(args), (p.stderr or p.stdout).strip()[:300]))
    return p.stdout if p.returncode == 0 else ""


def blob(root: str, ref: str, path: str) -> bytes | None:
    """A path's exact bytes at a ref, or None when the ref does not carry it (added/deleted)."""
    p = subprocess.run(["git", "show", "%s:%s" % (ref, path)], cwd=root, capture_output=True)
    return p.stdout if p.returncode == 0 else None


def merge_head_path(root: str) -> str:
    """Where MERGE_HEAD lives for this tree.

    Never `os.path.join(root, ".git", ...)`: in a linked worktree (every slot) `.git` is a *file* pointing
    at the real git dir, so that path never exists and an in-progress merge would look like a fresh one.
    `rev-parse --git-path` is the authority, but **git answers relative to the worktree** (`.git/MERGE_HEAD`),
    and `os.path.exists` would then resolve it against the calling process's cwd: a caller that passes
    `--root` (or any programmatic caller, the selftest's fixtures included) saw no merge in progress and
    started a second `git merge` over the one already there.  It is joined onto `root` for that reason.
    """
    p = git(root, "rev-parse", "--git-path", "MERGE_HEAD").strip()
    if p and not os.path.isabs(p):
        p = os.path.join(root, p)
    return p


def merge_in_progress(root: str) -> bool:
    p = merge_head_path(root)
    return bool(p) and os.path.exists(p)


def conflicted(root: str) -> list[str]:
    """The unmerged paths, git's own answer - the list the resolution is driven from."""
    return [ln for ln in git(root, "diff", "--name-only", "--diff-filter=U").splitlines() if ln.strip()]


def worktree_edits(root: str) -> list[str]:
    """The paths whose *working tree* differs from the index, deduplicated (`git diff --name-only`).

    An unmerged path is reported here once per index stage, which is why this is not just a `status`
    wrapper - and why a merge in progress is not the same thing as a dirty tree (see
    `cleanliness_blocker`). Untracked files are deliberately absent: git's own diff does not see them.
    """
    seen: list[str] = []
    for ln in git(root, "diff", "--name-only").splitlines():
        p = ln.strip()
        if p and p not in seen:
            seen.append(p)
    return seen


def cleanliness_blocker(root: str, in_merge: bool, conflicted_paths: list[str], dry_run: bool) -> str | None:
    """The reason this tree may not be touched, or `None` when it may.

    A **fresh** merge must start from a known state, so any dirt refuses it (unchanged).  A **resume** is
    different: the merge in progress is itself the dirt - git stages every auto-merged file and leaves the
    conflicted paths unmerged - so requiring a clean tree refuses the re-run this tool's own message asks
    for.  A merge is only resumed when this tool's state file describes it (checked by the caller), and
    what still has to hold is that nothing *outside* the merge has been edited: an edit to a path the
    merge does not own is a real refusal, and it names the path.
    """
    if dry_run:
        return None
    if not in_merge:
        if git(root, "status", "--porcelain").strip():
            return "the tree is not clean - commit or stash first (a merge must start from a known state)"
        return None
    allowed = set(conflicted_paths)
    stray = [p for p in worktree_edits(root) if p not in allowed]
    if stray:
        return ("a merge is in progress but the tree has changed outside it (%d path(s): %s) - resolve or "
                "stash them first, or `git merge --abort`" % (len(stray), ", ".join(stray[:3])))
    return None


def rename_sources(root: str, base: str, ref: str) -> dict[str, str]:
    """`destination -> source` for every rename git detects between two refs (`-M`), in one call.

    This is the same detector that decided a path was an *add* on that side of the merge, so it is the
    right authority for "which path is this one's pre-rename base".  Called once per ref, lazily, and
    only when an add/add path is actually being resolved.
    """
    out: dict[str, str] = {}
    for line in git(root, "diff", "-M", "--name-status", "--diff-filter=R", base, ref).splitlines():
        parts = line.split("\t")
        if len(parts) >= 3 and parts[0].startswith("R"):
            out[parts[2]] = parts[1]
    return out


def newline_of(data: bytes) -> str:
    return "\r\n" if b"\r\n" in data[:65536] else "\n"


def lines_of(data: bytes) -> list[str]:
    return data.decode("utf-8", "replace").split("\n")


def restore(root: str, path: str, lines: list[str], nl: str) -> None:
    """Write lines back with the file's *own* line ending: `include/**` is CRLF while `src/**` is LF, and
    a scripted rewrite that assumes one silently rewrites the other."""
    text = nl.join(l.rstrip("\r") for l in lines)
    with open(os.path.join(root, path), "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def read_lines(root: str, path: str) -> list[str] | None:
    p = os.path.join(root, path)
    if not os.path.isfile(p):
        return None
    with open(p, "rb") as fh:
        return lines_of(fh.read())


# ---------------------------------------------------------------------------------------------------
# pure resolvers - the logic lives here so the selftest needs no repository
# ---------------------------------------------------------------------------------------------------
def rename_pairs(before: list[str], after: list[str]) -> list[tuple[str, str]]:
    """The `(old row, new row)` pairs in two versions of a symbol map, in file order.

    A rename is a row replacement, never an insertion or a deletion: pair the *i*-th removed line with the
    *i*-th added line and keep only pairs that still look like map rows. Nothing is paired speculatively -
    a `-`/`+` count mismatch is reported by the caller, not guessed at here.
    """
    removed = [l for l in before if l.strip() and not l.startswith("#") and l not in set(after)]
    added = [l for l in after if l.strip() and not l.startswith("#") and l not in set(before)]
    pairs = []
    for i in range(min(len(removed), len(added))):
        if "=" in removed[i] and "=" in added[i]:
            pairs.append((removed[i], added[i]))
    return pairs


def apply_renames(main_lines: list[str], pairs: list[tuple[str, str]]) -> tuple[list[str], list[str]]:
    """Re-apply rename pairs onto main's map by exact row replacement.

    Returns `(lines, unapplied)`. A pair whose old row is gone (main already renamed it) or whose new row
    is already present (applied twice) is not an error to guess about - it is reported, and the caller
    decides. Order is preserved because the row is replaced in place.
    """
    out = list(main_lines)
    unapplied = []
    for old, new in pairs:
        if new in out and old in out:
            unapplied.append(old)            # a real collision: main already gives this row another name
            continue
        if new in out:                       # already applied - main carries the new name
            continue
        if old not in out:                   # main renamed the row to something else entirely
            unapplied.append(old)
            continue
        out[out.index(old)] = new
    return out, unapplied


def markers_in(lines: list[str]) -> list[tuple[int, str]]:
    found = []
    for n, line in enumerate(lines, 1):
        head = line.lstrip()
        for m in MARKERS:
            if head.startswith(m + " ") or head.rstrip() == m:
                found.append((n, m))
    return found


def union_markers(text: str) -> tuple[str, int]:
    """Union a `--diff3` merge result: ours first, then theirs' lines that ours lacks.

    Never applied to a `src/**` block blindly - a union of two *edits* duplicates both (an `if` block was
    duplicated that way and cost an hour). It is right for an additive section (a band header's
    declarations, a block in if/else form) and wrong for a rewritten body, which is why `resolve` classifies
    the path first.
    """
    out: list[str] = []
    source = text.splitlines(keepends=True)
    i, conflicts = 0, 0
    while i < len(source):
        line = source[i]
        if line.startswith("<<<<<<<"):
            conflicts += 1
            ours: list[str] = []
            theirs: list[str] = []
            mode = "ours"
            i += 1
            while i < len(source) and not source[i].startswith(">>>>>>>"):
                if source[i].startswith("|||||||"):
                    mode = "base"
                elif source[i].startswith("======="):
                    mode = "theirs"
                elif mode == "ours":
                    ours.append(source[i])
                elif mode == "theirs":
                    theirs.append(source[i])
                i += 1
            merged = list(ours)
            for l in theirs:
                if l not in merged:
                    merged.append(l)
            out.extend(merged)
        else:
            out.append(line)
        i += 1
    return "".join(out), conflicts


def text_ranges(splits: str) -> list[tuple[int, int, str]]:
    """`(start, end, unit)` for every `.text` range in a `splits.txt`."""
    ranges, cur = [], None
    for line in splits.splitlines():
        m = SPLITS_UNIT.match(line)
        if m:
            cur = m.group(1)
            continue
        m = SPLITS_TEXT.match(line)
        if m and cur:
            ranges.append((int(m.group(1), 16), int(m.group(2), 16), cur))
    return ranges


def sweep_band_header(lines: list[str], ranges: list[tuple[int, int, str]]) -> tuple[list[str], list[tuple[str, str]]]:
    """Drop band-header declarations whose address a registered unit owns (rule 2).

    Returns `(kept, dropped)` with the owner for each drop, because the declaration is not deleted - it
    **moved into that unit's header**, and the reader has to be told where. Rule 2 has no deferral comment
    (unlike rule 7), so "keep it in the band for now" is not an option.
    """
    keep, dropped = [], []
    for line in lines:
        m = ADDR_COMMENT.search(line)
        if m and DECL_HEAD.match(line) and ";" in line:
            a = int(m.group(1), 16)
            owner = next((o for s, e, o in ranges if s <= a < e), None)
            if owner:
                dropped.append((line.strip(), owner))
                continue
        keep.append(line)
    return keep, dropped


def classification(path: str) -> str:
    if os.path.basename(path) == "symbols.txt" and path.startswith("config/"):
        return "map"
    if path.startswith("src/"):
        return "source"
    if path.startswith("include/unsplit/"):
        return "band"
    return "threeway"


def branch_only_additions(base: list[str], branch: list[str]) -> list[str]:
    """The non-blank lines the branch has and the base has not - its side of the merge, for the proof."""
    return [l for l in branch if l.strip() and l not in base]


def missing_from(candidate: list[str], wanted: list[str]) -> list[str]:
    return [l for l in wanted if l not in candidate]


def addadd_choice(base_lines: list[str], ours: list[str], theirs: list[str]) -> tuple[str | None, str]:
    """Which side of an **add/add** path carries the other's work: `("ours" | "theirs" | None, why)`.

    A path added on both sides has no blob of its own in the merge base, so `git merge-file` has no
    three-way base to work from (`merge_file(root, ours, None, theirs)` is a `TypeError`, and the branch
    class that produces it - a re-home on each side - is common, not exotic).  The lane that hit it
    resolved by hand with the rule implemented here: each side is diffed against the base of the path *it
    was renamed from* (for a re-home the pre-rename path **is** the true three-way base), and the side that
    is the **superset** of the other wins.  `base_lines == []` means no such base is derivable, and then
    the two copies are compared whole - the same rule, with nothing to subtract.

    A side is the superset when it contains every line the other side has and it has not.  Neither side
    being the superset is a **refusal**, never a guess: two independent files that landed on one path, or
    two different rename sources, is a decision only the author can make.  `why` always names the side
    taken and the evidence, because "which copy won, and why" has to be readable in the merge commit.
    """
    ours_add = branch_only_additions(base_lines, ours)
    theirs_add = branch_only_additions(base_lines, theirs)
    ours_sup = not missing_from(ours, theirs_add)
    theirs_sup = not missing_from(theirs, ours_add)
    if ours_sup and theirs_sup:
        # Mutual containment of additions: the copies differ only in lines neither side *added* (a
        # deletion, a reorder).  Keep the copy that is the literal superset when exactly one is - a
        # deletion is not a replacement - and otherwise main's, which is the merge's incumbent truth.
        if not missing_from(ours, theirs) and missing_from(theirs, ours):
            return "ours", ("both copies carry the other's additions and main's keeps every line the "
                             "branch's has - kept main's copy")
        if not missing_from(theirs, ours) and missing_from(ours, theirs):
            return "theirs", ("both copies carry the other's additions and the branch's keeps every "
                               "line main's has - kept the branch's copy")
        return "ours", ("both copies carry the other's additions (%d main-side, %d branch-side) - kept "
                         "main's copy" % (len(ours_add), len(theirs_add)))
    if ours_sup:
        return "ours", ("main's copy carries all %d line(s) only the branch's copy adds - main's copy "
                         "is the superset" % len(theirs_add))
    if theirs_sup:
        return "theirs", ("the branch's copy carries all %d line(s) only main's copy adds - the "
                           "branch's copy is the superset" % len(ours_add))
    miss_ours = missing_from(ours, theirs_add)
    miss_theirs = missing_from(theirs, ours_add)
    return None, ("neither copy is a superset: main's lacks %d line(s) the branch's adds (e.g. %r) and the "
                  "branch's lacks %d of main's (e.g. %r) - no base and no superset, so this one is the "
                  "author's call (`git merge-file -p --diff3` with the pre-rename path as the base)"
                  % (len(miss_ours), (miss_ours[0][:50] if miss_ours else ""),
                     len(miss_theirs), (miss_theirs[0][:50] if miss_theirs else "")))


def map_symbols(map_text: str) -> tuple[set[str], dict[int, str]]:
    """`(every row's name, address -> name)` for a `symbols.txt`: the map a generated name must resolve through."""
    live: set[str] = set()
    addresses: dict[int, str] = {}
    for m in MAP_ROW.finditer(map_text):
        live.add(m.group(1))
        addresses[int(m.group(3), 16)] = m.group(1)
    return live, addresses


def stale_generated_names(text: str, live: set[str], addresses: dict[int, str]) -> list[str]:
    """Every generated name in `text` a rename has made stale - and only a real *symbol* reference.

    Two false positives cost a hand merge on 2026-09-28, and both are closed here:

    * a **path** is not a symbol.  A comment naming `src/DWCi/fn_805113B0.c` is the *file name* of a unit
      whose map row is `DWCi_sendControlFrame`; the old scan read it as five stale symbols and blocked the
      merge.  A match preceded by a path separator, or followed by a file extension (`fn_…c` / `fn_…o`),
      is a path component, not a use.
    * a name the map cannot resolve at its address is not a stale symbol - it is prose (or an old file
      name).  A genuinely stale reference (the branch still *calls* a `fn_XXXXXXXX` the map renamed)
      resolves to a map row at that address, so it is still reported.
    """
    out: list[str] = []
    for m in RENAME.finditer(text):
        ident = m.group(0)
        if m.start() > 0 and text[m.start() - 1] in "/\\":
            continue                                    # a path component: `DWCi/fn_805113B0.c`
        if PATH_EXT.match(text, m.end()):
            continue                                    # a file name: `fn_805113B0.c` / `fn_805113B0.o`
        if ident in live:
            continue
        if int(m.group(2), 16) not in addresses:
            continue                                    # no map row at this address: not a symbol use
        out.append(ident)
    return out


def stripped_code(text: str) -> str:
    """The file's **code**, with comments and string/char literals blanked by `stylelint.strip`.

    Whitespace is collapsed because `strip` preserves *positions*: two comments of different lengths leave
    different numbers of spaces, and a comment-only rewrite of a paragraph would otherwise read as a code
    change.  Collapsing also folds string-literal content (the shared stripper blanks it), which is the
    tool's existing notion of `code` - a string-only change is not what this comparison is for.
    """
    return re.sub(r"\s+", "", strip(text)[0])


# ---------------------------------------------------------------------------------------------------
# the resolution
# ---------------------------------------------------------------------------------------------------
def state_path(root: str) -> str:
    return os.path.join(root, STATE)


def save_state(root: str, state: dict) -> None:
    os.makedirs(os.path.dirname(state_path(root)), exist_ok=True)
    with open(state_path(root), "w", encoding="utf-8") as fh:
        json.dump(state, fh, indent=1, sort_keys=True)


def load_state(root: str) -> dict:
    try:
        with open(state_path(root), "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return {}


def merge_file(root: str, ours: bytes, base: bytes, theirs: bytes) -> tuple[str, int]:
    """A real three-way merge of three byte strings, via `git merge-file --diff3`.

    `git apply --3way` is deliberately not used: it needs the index to match the working tree (it refuses
    with "does not match index" otherwise) and it decides conflicts itself.
    """
    with tempfile.TemporaryDirectory() as tmp:
        paths = []
        for name, data in (("ours", ours), ("base", base), ("theirs", theirs)):
            p = os.path.join(tmp, name)
            with open(p, "wb") as fh:
                fh.write(data)
            paths.append(p)
        p = subprocess.run(["git", "merge-file", "-p", "--diff3", *paths],
                           capture_output=True)
        return p.stdout.decode("utf-8", "replace"), p.returncode


def resolve(root: str, branch: str | None, dry_run: bool, as_json: bool) -> int:
    def bail(reason: str, code: int = 1) -> int:
        if as_json:
            print(json.dumps({"verdict": "REFUSED", "reason": reason}))
        else:
            print("REFUSED %s | %s" % (branch or "?", reason))
        return code

    # A merge in progress is checked **first**: the guard below is for a *fresh* merge (it must start from
    # a known state), and running it before this test refused the re-run after a crashed resolution - the
    # re-run this tool's own failure message tells the operator to make.  See `cleanliness_blocker`.
    head = git(root, "rev-parse", "HEAD").strip()
    in_merge = merge_in_progress(root)
    tip_state = load_state(root)
    if in_merge and not tip_state:
        return bail("a merge is in progress but this tool has no state for it - finish or abort it by "
                    "hand (`git merge --abort`)")
    blocker = cleanliness_blocker(root, in_merge, tip_state.get("conflicted", []), dry_run)
    if blocker:
        return bail(blocker)
    if git(root, "merge-base", "--is-ancestor", "main", "HEAD") and not in_merge:
        print("up to date: main is already an ancestor of %s - nothing to merge" % head[:8])
        return 0

    if not in_merge:
        base = git(root, "merge-base", "main", "HEAD").strip()
        p = subprocess.run(["git", "merge", "--no-commit", "--no-ff", "main"], cwd=root,
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        todo = conflicted(root)
        save_state(root, {"tip": head, "base": base, "conflicted": todo, "resolved": [],
                          "merge_output": (p.stdout + p.stderr)[-2000:]})
        print("merged main into %s: %d conflicted path(s)" % (head[:8], len(todo)))
    else:
        base, todo = tip_state["base"], tip_state["conflicted"]
        print("resuming a merge of main into %s: %d recorded conflicted path(s)" % (head[:8], len(todo)))

    splits = blob(root, "main", "config/RMHE08/splits.txt") or b""
    ranges = text_ranges(splits.decode("utf-8", "replace"))
    rename_maps: dict[str, dict[str, str]] = {}
    actions: list[dict] = []
    blockers: list[str] = []

    for path in todo:
        ours = blob(root, "main", path)
        base_b = blob(root, base, path)
        theirs = blob(root, "HEAD", path)
        # a deletion on either side wins: resurrecting a deleted file is never the merge the author meant
        if ours is None or theirs is None:
            # a deletion on either side wins: resurrecting a deleted file is never the merge the author
            # meant, and `git rm` also handles the file already being gone from the working tree
            subprocess.run(["git", "rm", "-q", "--", path], cwd=root, capture_output=True)
            actions.append({"path": path, "class": "deleted", "note": "deleted on one side"})
            continue
        kind = classification(path)
        action = {"path": path, "class": kind}

        # --- add/add: no blob at this path in the merge base -------------------------------------------------
        # The path was added on both sides, so there is no three-way base *here* and `merge_file` cannot
        # run at all (`base_b` is None: its TypeError/AttributeError is what this branch replaces).  The
        # true base of a re-home is the path it was renamed from; when no side names one, the superset of
        # the two copies is the only answer that is not a guess.  A `symbols.txt` is left to its own
        # class below - a map is resolved by row surgery, never by taking a side whole.
        if base_b is None and kind != "map":
            if "main" not in rename_maps:
                rename_maps["main"] = rename_sources(root, base, "main")
            if "HEAD" not in rename_maps:
                rename_maps["HEAD"] = rename_sources(root, base, "HEAD")
            src_ours, src_theirs = rename_maps["main"].get(path), rename_maps["HEAD"].get(path)
            if src_ours and src_theirs and src_ours != src_theirs:
                blockers.append("%s: renamed from %r on main but from %r on the branch - two different "
                                "three-way bases for one path, so there is nothing to diff either copy "
                                "against. This one is the author's call."
                                % (path, src_ours, src_theirs))
                continue
            src = src_ours or src_theirs
            base_add = blob(root, base, src) if src else None
            side, why = addadd_choice(lines_of(base_add) if base_add is not None else [],
                                      lines_of(ours), lines_of(theirs))
            prefix = ("the pre-rename base is %r; " % src) if base_add is not None else \
                     "no pre-rename base is derivable, so the two copies are compared whole; "
            if side is None:
                blockers.append("%s: %s%s" % (path, prefix, why))
                continue
            taken_bytes = ours if side == "ours" else theirs
            taken = lines_of(taken_bytes)
            if kind == "band":
                # a band header still owes the rule-2 address sweep, whichever copy was taken
                kept, dropped = sweep_band_header(taken, ranges)
                taken = kept
                if dropped:
                    action["moved"] = [{"decl": d[:70], "owner": o} for d, o in dropped]
            restore(root, path, taken, newline_of(taken_bytes))
            action.update(addadd=True, took=side, note=prefix + why)
            actions.append(action)
            continue

        if kind == "map":
            # the branch's own rename pairs, re-applied onto main's file as exact row replacements
            rem = [l for l in git(root, "diff", base, "HEAD", "--", path).splitlines()
                   if l.startswith("-") and not l.startswith("---")]
            add = [l for l in git(root, "diff", base, "HEAD", "--", path).splitlines()
                   if l.startswith("+") and not l.startswith("+++")]
            pairs = rename_pairs([l[1:] for l in rem], [l[1:] for l in add])
            main_lines = lines_of(ours)
            merged, unapplied = apply_renames(main_lines, pairs)
            action.update(pairs=len(pairs), unapplied=len(unapplied))
            # a row the branch *added* is not a rename and would be dropped silently by a rename-only
            # re-apply. Every address already has a row in this repository's map, so a proposal lane's
            # map change is renames only (measured: 64 renames, 0 additions) - which is exactly why an
            # addition has to be reported rather than assumed impossible.
            pair_news = {new for _old, new in pairs}
            added_rows = [l for l in lines_of(theirs)
                          if l.strip() and l not in main_lines and l not in pair_news]
            if added_rows:
                blockers.append("%s: the branch added %d map row(s) that are not renames (e.g. %r) - a symbol "
                                "map is address-ordered, so insert them in place by hand rather than "
                                "appending them" % (path, len(added_rows), added_rows[0].strip()[:60]))
            if unapplied:
                blockers.append("%s: %d rename row(s) could not be re-applied onto main (the row is gone or "
                                "the new name is already there) - e.g. %s" % (path, len(unapplied),
                                                                              unapplied[0][:60]))
            restore(root, path, merged, newline_of(ours))
        elif kind == "source":
            # the branch's edit here is usually a rename sweep, and main's file may already carry it, so
            # take whichever side already contains the other's work - and refuse when neither does, rather
            # than union two edits of the same block (the duplicated-`if` mistake)
            want = branch_only_additions(lines_of(base_b), lines_of(theirs))
            if not want:
                restore(root, path, lines_of(theirs), newline_of(theirs))
                action["note"] = "the branch's file is a superset; took it"
            elif not missing_from(lines_of(ours), want):
                restore(root, path, lines_of(ours), newline_of(ours))
                action["note"] = "main's file already carries the branch's edits; took it"
            else:
                ours_missing = missing_from(lines_of(theirs), branch_only_additions(lines_of(base_b),
                                                                                    lines_of(ours)))
                if not ours_missing:
                    restore(root, path, lines_of(theirs), newline_of(theirs))
                    action["note"] = "the branch's file carries main's edits too; took it"
                else:
                    ours_code = stripped_code(ours.decode("utf-8", "replace"))
                    theirs_code = stripped_code(theirs.decode("utf-8", "replace"))
                    if ours_code == theirs_code:
                        # A comment block that BOTH sides rewrote is the one `src/**` conflict with no
                        # superset on either side (2026-09-28: the branch's ".text only" sentence vs
                        # main's "DATA CLAIMED" paragraph).  The *code* is identical, so the delta is
                        # prose: keep main's block - the shared truth the branch's fork predates - and
                        # record the branch's dropped paragraph, never a silent loss.
                        restore(root, path, lines_of(ours), newline_of(ours))
                        dropped = [l.strip() for l in lines_of(theirs) if l.strip() and l not in lines_of(ours)]
                        action["comment_only"] = True
                        action["note"] = ("code is identical after stripping comments - kept main's comment "
                                          "block and dropped the branch's (e.g. %r)" %
                                          (dropped[0][:60] if dropped else ""))
                    else:
                        miss = missing_from(lines_of(ours), want)
                        blockers.append("%s: neither side is a superset - main's file lacks %d of the "
                                        "branch's line(s) (e.g. %r) and the branch's lacks %d of main's. "
                                        "Resolve this one by hand (`git merge-file -p --diff3` three-ways "
                                        "it)." % (path, len(miss), miss[0][:70], len(ours_missing)))
                        continue
        else:
            text, conflicts = merge_file(root, ours, base_b, theirs)
            merged, unioned = union_markers(text)
            nl = newline_of(ours)
            if kind == "band":
                kept, dropped = sweep_band_header(merged.split("\n"), ranges)
                merged = "\n".join(kept)
                if dropped:
                    action["moved"] = [{"decl": d[:70], "owner": o} for d, o in dropped]
            restore(root, path, merged.split("\n"), nl)
            action.update(conflicts=conflicts, unioned=unioned)
        actions.append(action)

    # --- the proof, before anything is committed -----------------------------------------------------
    # an add/add resolution takes one whole side, so it is proved by identity: the path must be *that*
    # side's copy, never a union of the two (a union is the one thing an add/add pair must never become).
    for a in actions:
        if not a.get("addadd") or not os.path.isfile(os.path.join(root, a["path"])):
            continue
        ref, label = ("HEAD", "the branch") if a.get("took") == "theirs" else ("main", "main")
        want = blob(root, ref, a["path"])
        want_lines = lines_of(want) if want is not None else None
        if want_lines is not None and a.get("moved"):
            want_lines, _ = sweep_band_header(want_lines, ranges)   # a band header owes the rule-2 sweep
        if want_lines is not None and want_lines != (read_lines(root, a["path"]) or []):
            blockers.append("%s: the add/add resolution is not %s's copy (it must be one side whole, never "
                            "a union of the two)" % (a["path"], label))

    comment_only = {a["path"] for a in actions if a.get("comment_only")}
    for path in todo:
        if not os.path.isfile(os.path.join(root, path)):
            continue
        lines = read_lines(root, path) or []
        marks = markers_in(lines)
        if marks:
            blockers.append("%s: %d conflict marker(s) left (line %d %s)" % (path, len(marks), marks[0][0],
                                                                            marks[0][1]))
        kind = classification(path)
        theirs, base_b = blob(root, "HEAD", path), blob(root, base, path)
        if kind == "source" and theirs and base_b:
            if path in comment_only:
                # the code is what has to match the branch; the paragraph the resolution kept is named in
                # its action.  Fail closed if the code moved under us since the resolve above.
                if stripped_code("\n".join(lines)) != stripped_code(theirs.decode("utf-8", "replace")):
                    blockers.append("%s: resolved as comment-only but the code no longer matches the "
                                    "branch's" % path)
            else:
                want = branch_only_additions(lines_of(base_b), lines_of(theirs))
                gone = missing_from(lines, want)
                if gone:
                    blockers.append("%s: the branch's own line(s) are missing from the resolution: %s"
                                    % (path, "; ".join(repr(g[:60]) for g in gone[:3])))

    # a rename is TWO edits and a merge resolves only the map half: if the branch's files still *call* a
    # `fn_XXXXXXXX` the map has renamed, the link fails later with `undefined: 'fn_…'`.  Only a real symbol
    # reference counts: a comment naming a unit file's *path* is not one (`stale_generated_names`).
    map_text = (blob(root, "main", "config/RMHE08/symbols.txt") or b"").decode("utf-8", "replace")
    live, addresses = map_symbols(map_text)
    stale: list[tuple[str, str]] = []
    for path in todo:
        if not path.startswith("src/") or not os.path.isfile(os.path.join(root, path)):
            continue
        text = "\n".join(read_lines(root, path) or [])
        stale.extend((path, ident) for ident in stale_generated_names(text, live, addresses))
    if stale:
        blockers.append("the map renamed %d generated name(s) the branch's sources still use (the merge "
                        "resolved the map half only) - e.g. %s in %s. The address's current map name is the "
                        "one to use." % (len(stale), stale[0][1], stale[0][0]))

    resolved = [a["path"] for a in actions]
    if as_json:
        print(json.dumps({"verdict": "BLOCKED" if blockers else "OK", "actions": actions,
                          "blockers": blockers, "conflicted": todo}, indent=1))
    else:
        print("resolved by class:")
        for a in actions:
            note = a.get("note")
            if not note:
                if a["class"] == "map":
                    note = "%d rename row(s) re-applied" % a.get("pairs", 0)
                elif a["class"] == "source":
                    note = "took one side whole"
                else:
                    note = "%d conflict(s), %d unioned" % (a.get("conflicts", 0), a.get("unioned", 0))
            extra = ""
            if a.get("moved"):
                extra = " | %d declaration(s) belong elsewhere: %s -> %s" % (
                    len(a["moved"]), a["moved"][0]["decl"][:40], a["moved"][0]["owner"])
            # an add/add path says which side it took: "which copy won" must be readable in the output
            print("  %-44s %-9s %s%s" % (a["path"], "add/add" if a.get("addadd") else a["class"],
                                         note, extra))
        for b in blockers:
            print("BLOCKED  %s" % b)
    if blockers:
        print("\nnothing was committed. Fix the item(s) above and re-run `mergebranch.py resolve` "
              "(the state file is at %s, so the re-run resumes rather than restarts)." % STATE)
        return 1

    # --- the lane-side pre-flight the gate would run anyway ------------------------------------------
    notes = preflight(root, todo, base)
    for n in notes:
        print("  %s" % n)

    if dry_run:
        print("\ndry run: the resolution is in the tree and NOT committed")
        return 0
    msg = ["merge main into the branch (resolved by class: %s)" % ", ".join(sorted({a["class"] for a in actions}))
           if actions else "merge main into the branch"]
    # stage **the merge's own paths**, not the whole tree: `add -A` here would fold whatever else the
    # lane's worktree happens to carry (an untracked scratch file, its own notes) into the merge commit -
    # and a resumed merge deliberately tolerates untracked files, so it must not stage them.  Deletions
    # were already staged by `git rm` in the loop above.
    present = [p for p in todo if os.path.isfile(os.path.join(root, p))]
    if present:
        p = subprocess.run(["git", "add", "--", *present], cwd=root, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        if p.returncode != 0:
            return bail("staging the merge's own paths failed: %s" % (p.stderr or p.stdout).strip()[:200])
    p = subprocess.run(["git", "commit", "-q", "-m", msg[0]], cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        return bail("the merge commit failed: %s" % (p.stderr or p.stdout).strip()[:200])
    try:
        os.remove(state_path(root))
    except OSError:
        pass
    print("\ncommitted %s" % git(root, "log", "--oneline", "-1").strip()[:70])
    print("next: from MAIN, `python tools/units/land.py land --branch %s` (it derives --units itself)"
          % (branch or git(root, "rev-parse", "--abbrev-ref", "HEAD").strip()))
    return 0


def preflight(root: str, todo: list[str], base: str) -> list[str]:
    """The checks `land.py` would run, run here - a lane should learn its own name before the gate says it.

    Imported lazily and never fatal: this tool has to work in a tree where the unit tooling is not
    importable (the selftest's own fixtures are such a tree), and a pre-flight that cannot run is reported,
    not turned into a crash.
    """
    notes = []
    for extra in (REPO_TOOLS, HERE):
        if extra not in sys.path:
            sys.path.insert(0, extra)
    try:
        import land  # noqa: E402
    except Exception as exc:  # pragma: no cover - depends on the tree
        return ["pre-flight skipped (land.py not importable here: %s)" % exc]
    for fn, label in (("rule7_defer_growth", "rule 7 (generated names added by this merge)"),
                      ("band_ownership_warnings", "band-header ownership")):
        f = getattr(land, fn, None)
        if f is None:
            continue
        try:
            found = f(root, base)
        except BaseException as exc:  # a pre-flight must never kill the tool
            notes.append("%s: could not run (%s)" % (label, exc))
            continue
        notes.append("%s: %s" % (label, "clean" if not found else "FOUND %d: %s" % (len(found), found[:2])))
    units = []
    try:
        units = [u for u in land.units_from_branch(root, "HEAD", base)]
    except BaseException:
        pass
    if units:
        targets = ["build/RMHE08/src/%s.o" % u for u in units]
        p = subprocess.run(["ninja", *targets], cwd=root, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        failed = [l for l in (p.stdout + p.stderr).splitlines() if l.startswith("FAILED")]
        notes.append("compile %d unit(s): %s" % (len(units), "ok" if not failed else
                                                 "FAILED %d - %s" % (len(failed), failed[0][:90])))
        notes.append("stylelint: run `python tools/units/stylelint.py --diff %s` (it now picks the merge "
                     "base itself)" % base[:12])
    return notes


def status(root: str) -> int:
    st = load_state(root)
    if not st:
        print("no merge in progress for this tree (no %s)" % STATE)
        return 0
    print("a merge of main into %s is recorded" % st.get("tip", "?")[:8])
    print("  base          %s" % st.get("base", "?")[:8])
    print("  conflicted    %d path(s): %s" % (len(st.get("conflicted", [])),
                                             ", ".join(st.get("conflicted", [])[:6])))
    print("  markers now   %s" % ("none" if not any(markers_in(read_lines(root, p) or [])
                                                   for p in st.get("conflicted", [])
                                                   if os.path.isfile(os.path.join(root, p))) else "present"))
    print("re-run `mergebranch.py resolve` to continue; it resumes from this state.")
    return 0


# ---------------------------------------------------------------------------------------------------
# selftest
# ---------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    base = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
            "fn_80010040 = .text:0x80010040; // type:function size:0x40",
            "old_name = .text:0x80010080; // type:function size:0x20"]
    branch = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
              "fn_80010040 = .text:0x80010040; // type:function size:0x40",
              "new_name = .text:0x80010080; // type:function size:0x20"]
    pairs = rename_pairs(base, branch)
    check("one rename pairs up", pairs, [(base[2], branch[2])])
    main_map = ["fn_80010000 = .text:0x80010000; // type:function size:0x40",
                "old_name = .text:0x80010080; // type:function size:0x20",
                "added_on_main = .text:0x800100C0; // type:function size:0x10"]
    applied, unapplied = apply_renames(main_map, pairs)
    check("the rename lands on main's map", "new_name = .text:0x80010080; // type:function size:0x20" in applied, True)
    check("... main's own new row is untouched", "added_on_main = .text:0x800100C0; // type:function size:0x10" in applied, True)
    check("... order is preserved (the renamed row did not move)",
          applied.index("new_name = .text:0x80010080; // type:function size:0x20") > applied.index(base[0]), True)
    check("nothing unapplied", unapplied, [])
    check("a pair whose old row is gone is reported, not guessed",
          apply_renames(main_map, [("gone = .text:0x1; // x", "fresh = .text:0x1; // x")])[1],
          ["gone = .text:0x1; // x"])
    check("an already-applied pair is a no-op", apply_renames(applied, pairs)[1], [])

    merged, conflicts = union_markers("a\n<<<<<<< ours\nb\n||||||| base\nc\n=======\nd\ne\n>>>>>>> theirs\nf\n")
    check("the union counts the conflict", conflicts, 1)
    check("... keeps ours and theirs, drops base", merged, "a\nb\nd\ne\nf\n")
    check("... and is idempotent for a line both sides added",
          union_markers("<<<<<<< o\nx\n=======\nx\n>>>>>>> t\n")[0], "x\n")
    check("no markers is no change", union_markers("a\nb\n")[0], "a\nb\n")

    splits = ("menu/arena_result.cpp:\n\t.text       start:0x803B0F98 end:0x803B465C\n\n"
              "enemy/em_pop.cpp:\n\t.text       start:0x803B465C end:0x803B8000\n")
    ranges = text_ranges(splits)
    check("splits.txt text ranges parse", ranges,
          [(0x803B0F98, 0x803B465C, "menu/arena_result.cpp"), (0x803B465C, 0x803B8000, "enemy/em_pop.cpp")])
    header = ["/* band */",
              "s32 fn_803B1000(void);                  /* 0x803B1000 */",
              "void fn_803B521C(s32);                  /* 0x803B521C */",
              "void quest_arena_count_get(u16 index);  /* 0x803B68F0 */",
              "void unowned_thing(void);               /* 0x80700000 */",
              "struct ScreenGeomView { int x; };"]
    kept, dropped = sweep_band_header(header, ranges)
    check("a declaration inside a registered range is swept", len(dropped), 3)
    check("... and the owner is named", [o for _d, o in dropped],
          ["menu/arena_result.cpp", "enemy/em_pop.cpp", "enemy/em_pop.cpp"])
    check("a declaration outside every range stays", "void unowned_thing(void);               /* 0x80700000 */" in kept, True)
    check("... and so does a struct", "struct ScreenGeomView { int x; };" in kept, True)

    check("the map class", classification("config/RMHE08/symbols.txt"), "map")
    check("the source class", classification("src/menu/arena_result.cpp"), "source")
    check("the band class", classification("include/unsplit/menu.h"), "band")
    check("everything else is three-way", classification("configure.py"), "threeway")
    check("branch additions ignore blank lines", branch_only_additions(["a", ""], ["a", "", "b"]), ["b"])
    check("missing lines are reported in order", missing_from(["a"], ["a", "b", "c"]), ["b", "c"])

    # --- the stale-generated-name scan must read a *symbol reference*, not a unit file's path ---------
    # 2026-09-28: a comment naming `src/DWCi/fn_805113B0.c` was read as five stale symbols and blocked a
    # merge whose map row at that address is `DWCi_sendControlFrame` (the file name, not a map row).
    live, addresses = map_symbols("DWCi_sendControlFrame = .text:0x805113B0; // type:function size:0xB8\n"
                                  "some_real_name = .text:0x80004320; // type:function size:0x10\n")
    check("the map's names and addresses parse",
          ("DWCi_sendControlFrame" in live, addresses.get(0x805113B0)),
          (True, "DWCi_sendControlFrame"))
    check("a path in a comment is not a stale symbol",
          stale_generated_names("* see `src/DWCi/fn_805113B0.c` and this unit.", live, addresses), [])
    check("... the bare `/fn_XXXXXXXX.c` form too",
          stale_generated_names("from `DWCi/fn_805113B0.c`", live, addresses), [])
    check("... and an object-file mention (`fn_XXXXXXXX.o`)",
          stale_generated_names("in fn_805113B0.o's relocations", live, addresses), [])
    check("a genuinely stale CALL is still reported",
          stale_generated_names("void f(void) { fn_80004320(); }\n", live, addresses), ["fn_80004320"])
    check("... even when a comment also names its file",
          stale_generated_names("/* fn_80004320.c */\nvoid f(void) { fn_80004320(); }\n",
                                live, addresses), ["fn_80004320"])
    check("a name the map cannot resolve is not reported",
          stale_generated_names("fn_DEADBEEF()", live, addresses), [])
    check("a name still live in the map is not stale",
          stale_generated_names("void f(void) { fn_80004320(); }\n",
                                live | {"fn_80004320"}, addresses), [])
    check("the shared stripper compares code, not comment length",
          stripped_code("/* a longer rewritten paragraph */\nint a;\n") ==
          stripped_code("/* short */\nint a;\n"), True)
    check("... and a real code change is not hidden by it",
          stripped_code("int a_lane;\n") == stripped_code("int a_main;\n"), False)

    # the failure mode this tool exists for, on a real repository: a conflicted file left as main's copy
    with tempfile.TemporaryDirectory() as tmp:
        def qgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)
        qgit("init", "-q")
        qgit("checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"))
        with open(os.path.join(tmp, "src", "f.txt"), "w") as fh:
            fh.write("line one\nline two\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "base")
        cut = git(tmp, "rev-parse", "HEAD").strip()
        qgit("checkout", "-q", "-b", "lane")
        with open(os.path.join(tmp, "src", "f.txt"), "w") as fh:
            fh.write("line one\nline two\nlane adds this\n")
        qgit("commit", "-q", "-am", "the branch's own work")
        branch_blob = blob(tmp, "HEAD", "src/f.txt")
        qgit("checkout", "-q", "main")
        with open(os.path.join(tmp, "src", "f.txt"), "w") as fh:
            fh.write("line one\nmain moved\nline two\n")
        qgit("commit", "-q", "-am", "main moved")
        qgit("checkout", "-q", "lane")
        check("the fixture's branch edit is a real addition",
              branch_only_additions(lines_of(blob(tmp, cut, "src/f.txt")), lines_of(branch_blob)), ["lane adds this"])
        check("... and main's copy lacks exactly that line",
              missing_from(lines_of(blob(tmp, "main", "src/f.txt")), ["lane adds this"]), ["lane adds this"])

    # END TO END, in a real (temporary) repository: the class of defect this tool exists for is a stale
    # branch whose map conflict must keep BOTH sides - and the one it must refuse rather than guess.
    def e2e(src_same_line: bool) -> tuple[int, str, str]:
        with tempfile.TemporaryDirectory() as tmp:
            def qgit(*args: str) -> None:
                subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                                "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                               check=True)

            def put(rel: str, text: str) -> None:
                p = os.path.join(tmp, rel)
                os.makedirs(os.path.dirname(p), exist_ok=True)
                with open(p, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)

            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                    "fn_80001080 = .text:0x80001080; // type:function size:0x40",
                    "fn_800010C0 = .text:0x800010C0; // type:function size:0x40"]
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
            put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
            put("src/f.c", "int a;\nint pad1;\nint pad2;\nint pad3;\nint b;\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            # the branch renames one map row and edits the first source line
            qgit("checkout", "-q", "-b", "lane")
            put("config/RMHE08/symbols.txt",
                "\n".join(rows[:2] + ["lane_renamed = .text:0x80001080; // type:function size:0x40", rows[3]]) + "\n")
            put("src/f.c", "int a_lane;\nint pad1;\nint pad2;\nint pad3;\nint b;\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch's work")
            # main moves: a row inserted one line above the renamed one (so the hunks overlap), and
            # either the same source line (a real conflict) or a different one (a clean merge)
            qgit("checkout", "-q", "main")
            put("config/RMHE08/symbols.txt",
                "\n".join(rows[:2] + ["main_added = .text:0x80001060; // type:function size:0x20", rows[2],
                                      rows[3]]) + "\n")
            put("src/f.c", "int a_main;\nint pad1;\nint pad2;\nint pad3;\nint b;\n" if src_same_line
                else "int a;\nint pad1;\nint pad2;\nint pad3;\nint b_main;\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main moved")
            qgit("checkout", "-q", "lane")
            code = resolve(tmp, "lane", dry_run=True, as_json=False)
            merged = "\n".join(read_lines(tmp, "config/RMHE08/symbols.txt") or [])
            source = "\n".join(read_lines(tmp, "src/f.c") or [])
            log = git(tmp, "log", "--oneline", "-3")
            return code, merged, source, log

    code, merged, source, _log = e2e(src_same_line=False)
    check("a clean source merge resolves", code, 0)
    check("... the branch's rename landed on main's map", "lane_renamed = .text:0x80001080" in merged, True)
    check("... and main's own new row survived it", "main_added = .text:0x80001060" in merged, True)
    check("... in address order (main's row is the lower one)",
          merged.index("main_added") < merged.index("lane_renamed"), True)
    check("... and main's source edit is in the source file too", "b_main" in source, True)

    code2, merged2, _source2, _log2 = e2e(src_same_line=True)
    check("a same-line source conflict BLOCKS rather than picking a side", code2, 1)
    check("... while the map conflict still resolved (the branch's rename was kept)",
          "lane_renamed = .text:0x80001080" in merged2, True)
    check("... and main's row was kept as well", "main_added = .text:0x80001060" in merged2, True)

    # a comment-only `src/**` conflict: both sides rewrote the same header paragraph, the *code* is
    # identical, so neither raw file is a superset. 2026-09-28: this blocked a merge that had to be done
    # by hand.  It must resolve, keep main's block (the claim's current truth), and record the dropped one.
    def e2e_comment() -> tuple[int, str, str]:
        with tempfile.TemporaryDirectory() as tmp:
            def qgit(*args: str) -> None:
                subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                                "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                               check=True)

            def put(rel: str, text: str) -> None:
                p = os.path.join(tmp, rel)
                os.makedirs(os.path.dirname(p), exist_ok=True)
                with open(p, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)

            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40"]
            body = "int a;\nint pad1;\nint b;\n"
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
            put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
            put("src/f.c", "/* the unit note. */\n" + body)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("src/f.c", "/* the branch's `.text`-only sentence. */\n" + body)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch rewrote the note")
            qgit("checkout", "-q", "main")
            put("src/f.c", "/* DATA CLAIMED - this unit's range is its own. */\n" + body)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main rewrote the note")
            qgit("checkout", "-q", "lane")
            code = resolve(tmp, "lane", dry_run=True, as_json=False)
            return code, "\n".join(read_lines(tmp, "src/f.c") or []), git(tmp, "log", "--oneline", "-2")

    code3, source3, _log3 = e2e_comment()
    check("a comment-only source conflict resolves instead of blocking", code3, 0)
    check("... keeping main's comment block", "DATA CLAIMED" in source3, True)
    check("... and dropping the branch's obsolete paragraph", "`.text`-only sentence" not in source3, True)
    check("... while the code is untouched", "int a;\nint pad1;\nint b;" in source3, True)

    # the guard that must NOT be weakened: a full body rewrite on the same line still refuses
    code4, _merged4, _source4, _log4 = e2e(src_same_line=True)
    check("a real code conflict still blocks after the comment-only fix", code4, 1)

    # ------------------------------------------------------------------------------------------------
    # ADD/ADD + THE RESUME GUARD (2026-09-29).  Both defects were found by a merger lane that then did the
    # work by hand.  (1) An add/add path (added on both sides - a re-home on each side, which is common,
    # not exotic) has no blob in the merge base, and `resolve` called `merge_file(root, ours, None, ...)`:
    # a `TypeError`/`AttributeError`, never a resolution.  (2) The `the tree is not clean` guard ran before
    # the merge-in-progress test, so the re-run the tool's own message asks for was refused - the state
    # file and the conflicted list were then driven by hand.
    # ------------------------------------------------------------------------------------------------
    side, why = addadd_choice([], ["a", "b"], ["a", "b", "c"])
    check("an add/add pair with no base blob: the superset side wins", side, "theirs")
    check("... and the choice is explained (which side, and why)", "superset" in why and "branch" in why, True)
    check("... symmetrically (main's copy can be the superset too)",
          addadd_choice([], ["a", "b", "c"], ["a", "b"])[0], "ours")
    check("... and an ambiguous pair is refused, never guessed", addadd_choice([], ["a"], ["b"])[0], None)
    check("... naming what each side lacks",
          "neither copy is a superset" in addadd_choice([], ["a"], ["b"])[1], True)
    abase = ["/* the band */", "int a;", "int b;"]
    check("with a pre-rename base, only each side's own additions are compared",
          addadd_choice(abase, abase + ["main_only"], abase + ["main_only", "branch_only"])[0], "theirs")
    check("... so a copy that only *deletes* is not the superset",
          addadd_choice(abase, abase[:2], abase + ["branch_only"])[0], "theirs")
    check("... and disjoint additions are refused (no superset either way)",
          addadd_choice(abase, abase + ["x"], abase + ["y"])[0], None)
    check("... a tie is broken by the copy that kept every line (a deletion is not a replacement)",
          addadd_choice(abase, abase + ["x"], abase[:2] + ["x"])[0], "ours")
    check("... and that tie-break is symmetric too",
          addadd_choice(abase, abase[:2] + ["x"], abase + ["x"])[0], "theirs")
    check("... identical copies are not a decision at all (main's is kept)",
          addadd_choice(abase, abase, abase)[0], "ours")

    @contextlib.contextmanager
    def fixture():
        """A throwaway repository plus the `qgit`/`put` helpers the fixtures below share."""
        with tempfile.TemporaryDirectory() as tmp:
            def qgit(*args: str, check: bool = True) -> str:
                p = subprocess.run(["git", "-c", "user.email=t@e.invalid", "-c", "user.name=t",
                                    "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True,
                                   text=True, encoding="utf-8", errors="replace")
                if check and p.returncode != 0:
                    raise AssertionError("git %s failed: %s" % (" ".join(args), p.stderr))
                return p.stdout

            def put(rel: str, text: str) -> None:
                path = os.path.join(tmp, rel)
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)

            yield tmp, qgit, put

    def project(qgit, put, rows: list[str]) -> None:
        """The `main`/`lane` pair every fixture here starts from: both sides insert a row in the same place
        of `symbols.txt` and the lane edits `src/f.c`, so the merge has at least one real conflict."""
        qgit("init", "-q")
        qgit("checkout", "-q", "-b", "main")
        put("config/RMHE08/symbols.txt", "\n".join(rows) + "\n")
        put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
        put("src/f.c", "int a;\nint pad1;\nint pad2;\nint b;\n")
        qgit("add", "-A")
        qgit("commit", "-q", "-m", "base")
        qgit("checkout", "-q", "-b", "lane")
        put("config/RMHE08/symbols.txt",
            "\n".join(rows[:2] + ["lane_renamed = .text:0x80001080; // type:function size:0x40"]) + "\n")
        qgit("commit", "-q", "-am", "the branch renames a map row")
        qgit("checkout", "-q", "main")
        put("config/RMHE08/symbols.txt",
            "\n".join(rows[:2] + ["main_added = .text:0x80001060; // type:function size:0x20", rows[2]]) + "\n")
        qgit("commit", "-q", "-am", "main adds a map row")
        qgit("checkout", "-q", "lane")

    def run_quiet(root: str, dry_run: bool) -> tuple[int, str]:
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = resolve(root, "lane", dry_run=dry_run, as_json=False)
        return code, out.getvalue()

    def e2e_addadd_no_base() -> tuple[int, str, str]:
        """Fixture (a): a path added on both sides with **no base blob at all** (no rename to point at),
        main's copy a strict subset of the branch's.  It used to raise before it could decide anything."""
        with fixture() as (tmp, qgit, put):
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("README", "the root\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("src/Pl/new.cpp", "one\ntwo\nthree\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch adds the file")
            qgit("checkout", "-q", "main")
            put("src/Pl/new.cpp", "one\ntwo\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main adds the same path")
            qgit("checkout", "-q", "lane")
            # prove the fixture is the class under test before the tool runs: unmerged, **no stage 1**
            qgit("merge", "--no-commit", "--no-ff", "main", check=False)
            stages = sorted(ln.split()[2] for ln in
                            git(tmp, "ls-files", "-u", "--", "src/Pl/new.cpp").splitlines() if ln.strip())
            check("the fixture is a real add/add: unmerged with no base stage", stages, ["2", "3"])
            qgit("merge", "--abort")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "src/Pl/new.cpp") or [])
            return code, report, merged

    code5, report5, merged5 = e2e_addadd_no_base()
    check("add/add with no base blob resolves instead of crashing", code5, 0)
    check("... taking the superset side's copy", merged5.strip(), "one\ntwo\nthree")
    check("... and reporting which side won, as add/add",
          ("add/add" in report5, "superset" in report5), (True, True))

    def e2e_addadd_rehome() -> tuple[int, str, str, bool]:
        """Fixture (a'), the lane's real case: the **same re-home on both sides**.  Main's rename is
        detected (its copy is nearly the old file), the branch's is not (its copy carries the whole naming
        pass, so git reads it as a *new* file) - which is what makes this add/add rather than a rename.
        The pre-rename path is the true three-way base, and the branch's copy is the superset."""
        with fixture() as (tmp, qgit, put):
            body = "".join("line %d\n" % i for i in range(20))
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("src/Pl/fn_8024F200.cpp", body)
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            qgit("mv", "src/Pl/fn_8024F200.cpp", "src/Pl/pl_act_step.cpp")
            put("src/Pl/pl_act_step.cpp", "main_edit 0\n" + body[len("line 0\n"):]
                + "".join("lane_line_%d\n" % i for i in range(40)))
            qgit("commit", "-q", "-am", "the branch re-homes and adds")
            qgit("checkout", "-q", "main")
            qgit("mv", "src/Pl/fn_8024F200.cpp", "src/Pl/pl_act_step.cpp")
            put("src/Pl/pl_act_step.cpp", "main_edit 0\n" + body[len("line 0\n"):])
            qgit("commit", "-q", "-am", "main re-homes (the pure half)")
            qgit("checkout", "-q", "lane")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "src/Pl/pl_act_step.cpp") or [])
            resurrected = os.path.exists(os.path.join(tmp, "src/Pl/fn_8024F200.cpp"))
            return code, report, merged, resurrected

    code6, report6, merged6, resurrected6 = e2e_addadd_rehome()
    check("a re-home add/add (both sides, one side's rename undetected) resolves", code6, 0)
    check("... by taking the branch's copy (it carries main's edit)",
          ("main_edit 0" in merged6, "lane_line_0" in merged6), (True, True))
    check("... whole - never a union, never markers", "<<<<<<<" not in merged6, True)
    check("... against the pre-rename base git names for it",
          "pre-rename base is 'src/Pl/fn_8024F200.cpp'" in report6, True)
    check("... and the pre-rename path itself is not resurrected", resurrected6, False)

    def e2e_addadd_band() -> tuple[int, str, str]:
        """An add/add **band header**: taking a side whole still owes the rule-2 address sweep, so a
        declaration whose address a registered unit owns is moved out (and named), as in every other
        band resolution."""
        with fixture() as (tmp, qgit, put):
            qgit("init", "-q")
            qgit("checkout", "-q", "-b", "main")
            put("config/RMHE08/splits.txt", "src/f.c:\n\t.text       start:0x80001000 end:0x80001100\n")
            put("README", "the root\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "base")
            qgit("checkout", "-q", "-b", "lane")
            put("include/unsplit/Pl.h", "/* the band */\n"
                "void fn_80001040(void);        /* 0x80001040 */\n"
                "void unowned_thing(void);      /* 0x80700000 */\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "the branch adds the band header")
            qgit("checkout", "-q", "main")
            put("include/unsplit/Pl.h", "/* the band */\n")
            qgit("add", "-A")
            qgit("commit", "-q", "-m", "main adds the same header, empty")
            qgit("checkout", "-q", "lane")
            code, report = run_quiet(tmp, dry_run=True)
            merged = "\n".join(read_lines(tmp, "include/unsplit/Pl.h") or [])
            return code, report, merged

    code7b, report7b, merged7b = e2e_addadd_band()
    check("an add/add band header resolves", code7b, 0)
    check("... keeping the branch's copy's unowned declaration", "unowned_thing" in merged7b, True)
    check("... and sweeping the one a registered unit owns", "fn_80001040" not in merged7b, True)
    check("... naming where it belongs", "belong elsewhere" in report7b and "src/f.c" in report7b, True)

    def e2e_resume(stray: bool) -> dict:
        """Fixture (b): a **mid-merge** tree.  A `--dry-run` leaves exactly what a crash leaves - MERGE_HEAD,
        the state file, the resolved paths written, nothing committed - so the second invocation is the
        tool's own documented re-run.  With `stray`, an edit the merge does not own is made first: that is
        still a dirty tree and must still refuse."""
        with fixture() as (tmp, qgit, put):
            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                    "fn_80001080 = .text:0x80001080; // type:function size:0x40"]
            project(qgit, put, rows)
            code1, first = run_quiet(tmp, dry_run=True)
            crash_state = {"merge_head": merge_in_progress(tmp),
                           "clean": cleanliness_blocker(tmp, True, conflicted(tmp), False)}
            put("scratch.txt", "the lane's own notes\n")          # untracked: not the merge's business
            if stray:
                put("src/f.c", "int a;\nint lane_touched_this;\nint pad2;\nint b;\n")
            code2, second = run_quiet(tmp, dry_run=False)
            out = {"code1": code1, "code2": code2, "first": first, "second": second,
                   "merged": "\n".join(read_lines(tmp, "config/RMHE08/symbols.txt") or []),
                   "still_merging": merge_in_progress(tmp),
                   "scratch_tracked": bool(git(tmp, "ls-files", "scratch.txt").strip()),
                   "scratch_left": os.path.isfile(os.path.join(tmp, "scratch.txt")),
                   "committed": git(tmp, "log", "--oneline", "-2").splitlines()}
            out.update(crash_state)
            return out

    r = e2e_resume(stray=False)
    check("the dry run that leaves the mid-merge tree resolved", r["code1"], 0)
    check("the fixture leaves a merge in progress, as a crash does", r["merge_head"], True)
    check("a tree dirty only because a merge is in progress is not a dirty tree", r["clean"], None)
    check("the second invocation RESUMES instead of refusing the re-run", r["code2"], 0)
    check("... and says so", "resuming" in r["second"], True)
    check("... finishing the merge", r["still_merging"], False)
    check("... with both sides of the map kept",
          ("lane_renamed" in r["merged"], "main_added" in r["merged"]), (True, True))
    check("... and an untracked file is NOT swept into the merge commit", r["scratch_tracked"], False)
    check("... while it is still in the worktree", r["scratch_left"], True)

    s = e2e_resume(stray=True)
    check("an edit the merge does not own still refuses a resume", s["code2"], 1)
    check("... naming the path and the reason",
          ("outside it" in s["second"], "src/f.c" in s["second"]), (True, True))
    check("... and committing nothing", s["still_merging"], True)

    def e2e_foreign_merge() -> tuple[int, str]:
        """A merge this tool did **not** start has no recorded conflicted list to drive, and is not
        resumable: it is refused by that name (`git merge --abort` is the way out)."""
        with fixture() as (tmp, qgit, put):
            rows = ["fn_80001000 = .text:0x80001000; // type:function size:0x40",
                    "fn_80001040 = .text:0x80001040; // type:function size:0x40",
                    "fn_80001080 = .text:0x80001080; // type:function size:0x40"]
            project(qgit, put, rows)
            subprocess.run(["git", "merge", "--no-commit", "--no-ff", "main"], cwd=tmp,
                           capture_output=True)
            return run_quiet(tmp, dry_run=False)

    code7, report7 = e2e_foreign_merge()
    check("a merge this tool did not start is refused, not resumed blindly", code7, 1)
    check("... by name", "no state for it" in report7, True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--root", default=None, help="the lane's tree (default: the cwd's toplevel)")
    sub = ap.add_subparsers(dest="cmd")
    r = sub.add_parser("resolve", help="merge main into the current branch, resolved by class")
    r.add_argument("--branch", default=None)
    r.add_argument("--dry-run", action="store_true", help="resolve but do not commit")
    r.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="what this tool has recorded for the tree")
    s.set_defaults(func=None)
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    root = a.root or (git(os.getcwd(), "rev-parse", "--show-toplevel").strip() or os.getcwd())
    if not a.cmd:
        ap.print_help()
        return 0
    if a.cmd == "status":
        return status(root)
    return resolve(root, a.branch, a.dry_run, a.json)


if __name__ == "__main__":
    sys.exit(main())
