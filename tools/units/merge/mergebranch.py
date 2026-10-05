"""Bring `main` into a held lane branch, resolve the conflicts by class, prove the result, commit it.
Spec: docs/tools/spec/merge.md. CLI: none (module; `tools/units/mergebranch.py` is the entry point, spec/mergebranch.md)."""
from __future__ import annotations

import argparse
import json
import os
import re

from tools.lib import cscan, proc, project as _project, repo, text
from tools.lib.git import Git
from tools.lib.project.ownership import HEADER_SUFFIXES, is_band_header
import tools.units.merge.unionprose as union

# The names `resolve` and the tests use; the implementations live once in `union`.
prose_line = union.prose_line
hunk_class = union.hunk_class
union_side = union.union_side
prose_superset = union.prose_superset
union_markers = union.union_markers
branch_only_additions = union.branch_only_additions
missing_from = union.missing_from
addadd_choice = union.addadd_choice
markers_in = union.markers_in
MARKERS = union.MARKERS

STATE = ".pi/merge-state.json"
RENAME = re.compile(r"\b(fn|lbl|loc)_([0-9A-Fa-f]{8})\b")
# A generated name followed by a file extension is a file name, not a symbol: `fn_805113B0.c` / `.o`.
PATH_EXT = re.compile(r"\.(?:c|cpp|cc|cp|h|hpp|hh|o|obj|d|s|asm|txt|json|map|md)\b")
ADDR_COMMENT = re.compile(r"/\*\s*0x([0-9A-Fa-f]{8})")
DECL_HEAD = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?(?:const\s+)?[A-Za-z_][\w:<> \*&]*?\b\w+\s*\(")


# --- git plumbing (bytes in / bytes out: a merge must never re-encode a file) ----------------------------------

def git(root: str, *args: str, check: bool = False) -> str:
    p = Git(root).run(*args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed: %s" % (" ".join(args), (p.stderr or p.stdout).strip()[:300]))
    return p.stdout if p.returncode == 0 else ""


def blob(root: str, ref: str, path: str) -> bytes | None:
    """A path's exact bytes at a ref, or None when the ref does not carry it (added/deleted)."""
    return Git(root).show(ref, path)


def merge_head_path(root: str) -> str:
    """Where MERGE_HEAD lives for this tree: `rev-parse --git-path`, joined onto `root` (git answers relative to
    the worktree, and a linked worktree's `.git` is a file)."""
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
    """The paths whose working tree differs from the index, deduplicated (an unmerged path is listed once per
    stage by `git diff --name-only`); untracked files are absent."""
    seen: list[str] = []
    for ln in git(root, "diff", "--name-only").splitlines():
        p = ln.strip()
        if p and p not in seen:
            seen.append(p)
    return seen


def cleanliness_blocker(root: str, in_merge: bool, conflicted_paths: list[str], dry_run: bool) -> str | None:
    """The reason this tree may not be touched, or `None`: a fresh merge needs a clean tree; a resume needs
    nothing edited outside the recorded conflicted paths."""
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
    """`destination -> source` for every rename git detects between two refs (`-M`), in one call."""
    try:
        return {new: old for old, new in Git(root).renames(base, ref)}
    except RuntimeError:                                 # GitError: an unknown ref is "no renames"
        return {}


def newline_of(data: bytes) -> str:
    """The ending a blob already uses (`lib.text.line_ending`)."""
    return text.line_ending(data.decode("utf-8", "replace"))


def lines_of(data: bytes) -> list[str]:
    return data.decode("utf-8", "replace").split("\n")


def restore(root: str, path: str, lines: list[str], nl: str) -> None:
    """Write lines back with the file's own line ending (a header may be CRLF while a source is LF)."""
    text.atomic_write(os.path.join(root, path), nl.join(l.rstrip("\r") for l in lines))


def read_lines(root: str, path: str) -> list[str] | None:
    p = os.path.join(root, path)
    if not os.path.isfile(p):
        return None
    with open(p, "rb") as fh:
        return lines_of(fh.read())


# --- pure resolvers ------------------------------------------------------------------------------------------

def rename_pairs(before: list[str], after: list[str]) -> list[tuple[str, str]]:
    """The `(old row, new row)` pairs in two versions of a symbol map, in file order (the i-th removed row with
    the i-th added row, both still map rows; a count mismatch is the caller's to report)."""
    removed = [l for l in before if l.strip() and not l.startswith("#") and l not in set(after)]
    added = [l for l in after if l.strip() and not l.startswith("#") and l not in set(before)]
    pairs = []
    for i in range(min(len(removed), len(added))):
        if "=" in removed[i] and "=" in added[i]:
            pairs.append((removed[i], added[i]))
    return pairs


def apply_renames(main_lines: list[str], pairs: list[tuple[str, str]]) -> tuple[list[str], list[str]]:
    """Re-apply rename pairs onto main's map by exact row replacement: `(lines, unapplied)`."""
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


def text_ranges(splits: str) -> list[tuple[int, int, str]]:
    """`(start, end, unit)` for every `.text` range in a `splits.txt`."""
    return [(r.start, r.end, r.unit) for r in _project.Splits.parse(splits).text_ranges()]


def sweep_band_header(lines: list[str], ranges: list[tuple[int, int, str]]) -> tuple[list[str], list[tuple[str, str]]]:
    """Drop band-header declarations whose address a registered unit owns (rule 2): `(kept, [(decl, owner)])`."""
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
    if is_band_header(path):
        return "band"
    if path.endswith(HEADER_SUFFIXES):
        return "threeway"     # a header merges three-way wherever it lives (`src/**` since the 2026-10-05 move)
    if path.startswith("src/"):
        return "source"
    return "threeway"


def map_symbols(map_text: str) -> tuple[set[str], dict[int, str]]:
    """`(every row's name, address -> name)` for a `symbols.txt`."""
    live: set[str] = set()
    addresses: dict[int, str] = {}
    for line in map_text.splitlines():
        e = _project.parse_line(line)
        if e is not None:
            live.add(e.name)
            addresses[e.address] = e.name
    return live, addresses


def stale_generated_names(source: str, live: set[str], addresses: dict[int, str]) -> list[str]:
    """Every generated name in `source` a rename has made stale - a symbol use, never a path component or file
    name, and only one the map resolves at its address."""
    out: list[str] = []
    for m in RENAME.finditer(source):
        ident = m.group(0)
        if m.start() > 0 and source[m.start() - 1] in "/\\":
            continue                                    # a path component: `DWCi/fn_805113B0.c`
        if PATH_EXT.match(source, m.end()):
            continue                                    # a file name: `fn_805113B0.c` / `fn_805113B0.o`
        if ident in live:
            continue
        if int(m.group(2), 16) not in addresses:
            continue                                    # no map row at this address: not a symbol use
        out.append(ident)
    return out


def stripped_code(source: str) -> str:
    """The file's code with comments and literals blanked (`lib.cscan.strip`) and whitespace collapsed."""
    return re.sub(r"\s+", "", cscan.strip(source)[0])


# --- the resolution ------------------------------------------------------------------------------------------

def state_path(root: str) -> str:
    return str(repo.state("merge-state.json", root))


def save_state(root: str, state: dict) -> None:
    text.atomic_write(state_path(root), json.dumps(state, indent=1, sort_keys=True))


def load_state(root: str) -> dict:
    try:
        with open(state_path(root), "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return {}


def merge_file(root: str, ours: bytes, base: bytes, theirs: bytes) -> tuple[str, int]:
    """A real three-way merge of three byte strings (`git merge-file --diff3`, through `lib.git`)."""
    merged, conflicts = Git(root or None).merge_bytes(ours, base, theirs)
    return merged.decode("utf-8", "replace"), conflicts


def resolve(root: str, branch: str | None, dry_run: bool, as_json: bool) -> int:
    def bail(reason: str, code: int = 1) -> int:
        if as_json:
            print(json.dumps({"verdict": "REFUSED", "reason": reason}))
        else:
            print("REFUSED %s | %s" % (branch or "?", reason))
        return code

    # A merge in progress is checked first: the cleanliness guard is for a fresh merge (`cleanliness_blocker`).
    g = Git(root)
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
        p = g.run("merge", "--no-commit", "--no-ff", "main")
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
        if ours is None or theirs is None:
            # a deletion on either side wins: resurrecting a deleted file is never the merge the author
            # meant, and `git rm` also handles the file already being gone from the working tree
            g.run("rm", "-q", "--", path)
            actions.append({"path": path, "class": "deleted", "note": "deleted on one side"})
            continue
        kind = classification(path)
        action = {"path": path, "class": kind}

        # --- add/add: no blob at this path in the merge base (a map is left to its own class below) ----------
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
            diff = git(root, "diff", base, "HEAD", "--", path).splitlines()
            rem = [l for l in diff if l.startswith("-") and not l.startswith("---")]
            add = [l for l in diff if l.startswith("+") and not l.startswith("+++")]
            pairs = rename_pairs([l[1:] for l in rem], [l[1:] for l in add])
            main_lines = lines_of(ours)
            merged, unapplied = apply_renames(main_lines, pairs)
            action.update(pairs=len(pairs), unapplied=len(unapplied))
            # a row the branch *added* is not a rename and would be dropped silently by a rename-only re-apply
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
            # take whichever side already contains the other's work; refuse when neither does
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
                        # a comment block both sides rewrote: keep main's, record the branch's dropped one
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
            merged_text, conflicts = merge_file(root, ours, base_b, theirs)
            merged, unioned, decisions = union_markers(merged_text, path)
            nl = newline_of(ours)
            if kind == "band":
                kept, dropped = sweep_band_header(merged.split("\n"), ranges)
                merged = "\n".join(kept)
                if dropped:
                    action["moved"] = [{"decl": d[:70], "owner": o} for d, o in dropped]
            restore(root, path, merged.split("\n"), nl)
            action.update(conflicts=conflicts, unioned=unioned)
            if decisions:
                action["hunks"] = decisions
            for d in decisions:
                if d.get("blocked"):
                    blockers.append("%s: prose conflict hunk %d - %s. A prose region both sides rewrote "
                                    "must not be unioned; neither side is the superset, so take the newer "
                                    "side by hand (`git merge-file -p --diff3` three-ways it)."
                                    % (path, d["hunk"], d["why"]))
            warnings = [d["warning"] for d in decisions if d.get("warning")]
            if warnings:
                action["warnings"] = warnings
        actions.append(action)

    # --- the proof, before anything is committed ------------------------------------------------------------
    # an add/add resolution takes one whole side, so it is proved by identity, never a union of the two
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
                if stripped_code("\n".join(lines)) != stripped_code(theirs.decode("utf-8", "replace")):
                    blockers.append("%s: resolved as comment-only but the code no longer matches the "
                                    "branch's" % path)
            else:
                want = branch_only_additions(lines_of(base_b), lines_of(theirs))
                gone = missing_from(lines, want)
                if gone:
                    blockers.append("%s: the branch's own line(s) are missing from the resolution: %s"
                                    % (path, "; ".join(repr(g_[:60]) for g_ in gone[:3])))

    # a rename is TWO edits and a merge resolves only the map half: a branch source still calling a renamed
    # `fn_XXXXXXXX` fails the link later (a path mention is not a call - `stale_generated_names`)
    map_text = (blob(root, "main", "config/RMHE08/symbols.txt") or b"").decode("utf-8", "replace")
    live, addresses = map_symbols(map_text)
    stale: list[tuple[str, str]] = []
    for path in todo:
        if not path.startswith("src/") or not os.path.isfile(os.path.join(root, path)):
            continue
        body = "\n".join(read_lines(root, path) or [])
        stale.extend((path, ident) for ident in stale_generated_names(body, live, addresses))
    if stale:
        blockers.append("the map renamed %d generated name(s) the branch's sources still use (the merge "
                        "resolved the map half only) - e.g. %s in %s. The address's current map name is the "
                        "one to use." % (len(stale), stale[0][1], stale[0][0]))

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
                    hunks = a.get("hunks", [])
                    took = [d for d in hunks if d.get("took")]
                    blocked = [d for d in hunks if d.get("blocked")]
                    if hunks:
                        note = "%d conflict hunk(s): %d unioned, %d prose taken" % (
                            len(hunks), len(hunks) - len(took) - len(blocked), len(took))
                        if blocked:
                            note += ", %d blocked" % len(blocked)
                    else:
                        note = "%d conflict(s), %d unioned" % (a.get("conflicts", 0), a.get("unioned", 0))
                    # a prose hunk names the side it took and why, so the operator audits the choice
                    if took:
                        note += " | " + "; ".join("hunk %d (%s): %s" % (d["hunk"], d["class"], d["why"])
                                                   for d in took)
            extra = ""
            if a.get("moved"):
                extra = " | %d declaration(s) belong elsewhere: %s -> %s" % (
                    len(a["moved"]), a["moved"][0]["decl"][:40], a["moved"][0]["owner"])
            print("  %-44s %-9s %s%s" % (a["path"], "add/add" if a.get("addadd") else a["class"],
                                         note, extra))
        for a in actions:
            for w in a.get("warnings", []):
                print("WARNING  %s" % w)
        for b in blockers:
            print("BLOCKED  %s" % b)
    if blockers:
        print("\nnothing was committed. Fix the item(s) above and re-run `mergebranch.py resolve` "
              "(the state file is at %s, so the re-run resumes rather than restarts)." % STATE)
        return 1

    # --- the lane-side pre-flight the gate would run anyway --------------------------------------------------
    for n in preflight(root, todo, base):
        print("  %s" % n)

    if dry_run:
        print("\ndry run: the resolution is in the tree and NOT committed")
        return 0
    msg = ("merge main into the branch (resolved by class: %s)" % ", ".join(sorted({a["class"] for a in actions}))
           if actions else "merge main into the branch")
    # stage the merge's own paths, never the whole tree (deletions were staged by `git rm` above)
    present = [p for p in todo if os.path.isfile(os.path.join(root, p))]
    if present:
        p = g.run("add", "--", *present)
        if p.returncode != 0:
            return bail("staging the merge's own paths failed: %s" % (p.stderr or p.stdout).strip()[:200])
    p = g.run("commit", "-q", "-m", msg)
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
    """The checks `land.py` would run, run here; imported lazily and never fatal (a tree where the gate's
    tooling is not importable reports it, and a pre-flight that raises is a note, not a crash)."""
    notes = []
    try:
        from tools.units import land
    except Exception as exc:  # pragma: no cover - depends on the tree
        return ["pre-flight skipped (land.py not importable here: %s)" % exc]
    for fn, label in (("band_ownership_warnings", "band-header ownership"),):
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
        p = proc.run(["ninja", *targets], cwd=root)
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


def main(argv: list[str] | None = None, description: str = "") -> int:
    ap = argparse.ArgumentParser(description=description)
    ap.add_argument("--root", default=None, help="the lane's tree (default: the cwd's toplevel)")
    sub = ap.add_subparsers(dest="cmd")
    r = sub.add_parser("resolve", help="merge main into the current branch, resolved by class")
    r.add_argument("--branch", default=None)
    r.add_argument("--dry-run", action="store_true", help="resolve but do not commit")
    r.add_argument("--json", action="store_true")
    s = sub.add_parser("status", help="what this tool has recorded for the tree")
    s.set_defaults(func=None)
    a = ap.parse_args(argv)
    root = a.root or (git(os.getcwd(), "rev-parse", "--show-toplevel").strip() or os.getcwd())
    if not a.cmd:
        ap.print_help()
        return 0
    if a.cmd == "status":
        return status(root)
    return resolve(root, a.branch, a.dry_run, a.json)
