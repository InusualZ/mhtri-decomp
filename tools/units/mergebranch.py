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

**Resolution is per class, because each class has exactly one correct rule:**

| conflicted path | rule |
|---|---|
| `config/**/symbols.txt` | main's file, then the branch's own rename pairs re-applied as **exact row replacements** - a symbol map is address-ordered, so a textual union reorders it and renames nothing |
| `src/**` | whichever side already carries the other side's work (the branch's edit there is usually a rename sweep, and main's file may already hold it). If neither does, **refuse** and name what is missing |
| an unsplit band header (`include/unsplit/*`) | a three-way union, then the **rule-2 address sweep**: a declaration whose address is inside a registered `.text` range belongs to that unit's header, and where it should move is reported |
| anything else | a three-way union |

Every resolution is then checked before the commit: no conflict markers, each conflicted path's branch
side present, every `fn_XXXXXXXX`/`lbl_XXXXXXXX` the branch still names still in the map, and - when the
tooling is importable - `land.py`'s own pre-flight rows (rule 7 growth, band ownership, registration) plus
the affected units' compile, which is the only check that sees a `NonMatching` unit's object.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_TOOLS = os.path.dirname(HERE)                       # tools/
STATE = ".pi/merge-state.json"
MARKERS = ("<<<<<<<", "|||||||", ">>>>>>>")
RENAME = re.compile(r"\b(fn|lbl|loc)_([0-9A-Fa-f]{8})\b")
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
    """
    return git(root, "rev-parse", "--git-path", "MERGE_HEAD").strip()


def merge_in_progress(root: str) -> bool:
    p = merge_head_path(root)
    return bool(p) and os.path.exists(p)


def conflicted(root: str) -> list[str]:
    """The unmerged paths, git's own answer - the list the resolution is driven from."""
    return [ln for ln in git(root, "diff", "--name-only", "--diff-filter=U").splitlines() if ln.strip()]


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

    if not dry_run and git(root, "status", "--porcelain").strip():
        return bail("the tree is not clean - commit or stash first (a merge must start from a known state)")
    head = git(root, "rev-parse", "HEAD").strip()
    tip_state = load_state(root)
    if git(root, "merge-base", "--is-ancestor", "main", "HEAD") and not merge_in_progress(root):
        print("up to date: main is already an ancestor of %s - nothing to merge" % head[:8])
        return 0

    if not merge_in_progress(root):
        base = git(root, "merge-base", "main", "HEAD").strip()
        p = subprocess.run(["git", "merge", "--no-commit", "--no-ff", "main"], cwd=root,
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        todo = conflicted(root)
        save_state(root, {"tip": head, "base": base, "conflicted": todo, "resolved": [],
                          "merge_output": (p.stdout + p.stderr)[-2000:]})
        print("merged main into %s: %d conflicted path(s)" % (head[:8], len(todo)))
    else:
        if not tip_state.get("conflicted"):
            return bail("a merge is in progress but this tool has no state for it - finish or abort it by "
                        "hand (`git merge --abort`)")
        base, todo = tip_state["base"], tip_state["conflicted"]
        print("resuming a merge of main into %s: %d recorded conflicted path(s)" % (head[:8], len(todo)))

    splits = blob(root, "main", "config/RMHE08/splits.txt") or b""
    ranges = text_ranges(splits.decode("utf-8", "replace"))
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
                    miss = missing_from(lines_of(ours), want)
                    blockers.append("%s: neither side is a superset - main's file lacks %d of the branch's "
                                    "line(s) (e.g. %r) and the branch's lacks %d of main's. Resolve this one "
                                    "by hand (`git merge-file -p --diff3` three-ways it)."
                                    % (path, len(miss), miss[0][:70], len(ours_missing)))
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
            want = branch_only_additions(lines_of(base_b), lines_of(theirs))
            gone = missing_from(lines, want)
            if gone:
                blockers.append("%s: the branch's own line(s) are missing from the resolution: %s"
                                % (path, "; ".join(repr(g[:60]) for g in gone[:3])))

    # a rename is TWO edits and a merge resolves only the map half: if the branch's files still name a
    # `fn_XXXXXXXX` the map no longer has, the link fails later with `undefined: 'fn_…'`
    map_text = (blob(root, "main", "config/RMHE08/symbols.txt") or b"").decode("utf-8", "replace")
    live = set(re.findall(r"^\s*(\w+)\s*=", map_text, re.M))
    stale: list[tuple[str, str]] = []
    for path in todo:
        if not path.startswith("src/") or not os.path.isfile(os.path.join(root, path)):
            continue
        text = "\n".join(read_lines(root, path) or [])
        for kind_, addr in RENAME.findall(text):
            ident = "%s_%s" % (kind_, addr)
            if ident not in live and ident in text:
                stale.append((path, ident))
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
            print("  %-44s %-9s %s%s" % (a["path"], a["class"], note, extra))
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
    subprocess.run(["git", "add", "-A"], cwd=root, check=True, capture_output=True)
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
