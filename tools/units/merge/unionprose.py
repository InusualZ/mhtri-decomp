"""The one union rule: code hunks union, prose hunks take the superset, mixed warns; the one diff3 hunk reader.
Spec: docs/tools/spec/merge.md. CLI: none (module)."""
from __future__ import annotations

from dataclasses import dataclass

#: The three marker heads a `--diff3` conflict carries (`=======` is the separator, not a marker).
MARKERS = ("<<<<<<<", "|||||||", ">>>>>>>")


# --- the diff3 hunk --------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Hunk:
    """One conflict region of a `--diff3` merge: the three sides' lines (line endings kept)."""
    ours: tuple[str, ...]
    base: tuple[str, ...]
    theirs: tuple[str, ...]

    @property
    def has_base(self) -> bool:
        """A non-blank base line: one or both sides changed existing content (not a disjoint addition)."""
        return any(line.strip() for line in self.base)


def segments(text: str):
    """The merge result as a sequence of plain lines (`str`) and `Hunk`s, in order.

    The one reading of the markers: a hunk opens at `<<<<<<<`, its base starts at `|||||||`, theirs at
    `=======`, and it closes at `>>>>>>>` (a hunk left unclosed at the end of the text ends there).
    """
    source = text.splitlines(keepends=True)
    i = 0
    while i < len(source):
        if not source[i].startswith("<<<<<<<"):
            yield source[i]
            i += 1
            continue
        sides: dict[str, list[str]] = {"ours": [], "base": [], "theirs": []}
        mode = "ours"
        i += 1
        while i < len(source) and not source[i].startswith(">>>>>>>"):
            if source[i].startswith("|||||||"):
                mode = "base"
            elif source[i].startswith("======="):
                mode = "theirs"
            else:
                sides[mode].append(source[i])
            i += 1
        i += 1                                           # the `>>>>>>>` line (or past the end)
        yield Hunk(tuple(sides["ours"]), tuple(sides["base"]), tuple(sides["theirs"]))


def has_base_region(text: str) -> bool:
    """True iff any conflict hunk in a `--diff3` merge output has a non-empty base section."""
    return any(isinstance(s, Hunk) and s.has_base for s in segments(text))


def markers_in(lines: list[str]) -> list[tuple[int, str]]:
    """`[(1-based line, marker)]` for every conflict marker line left in `lines`."""
    found = []
    for n, line in enumerate(lines, 1):
        head = line.lstrip()
        for m in MARKERS:
            if head.startswith(m + " ") or head.rstrip() == m:
                found.append((n, m))
    return found


# --- the classes and the superset ------------------------------------------------------------------------------

def prose_line(line: str, in_block: bool) -> tuple[bool, bool]:
    """`(is_prose, still_in_block)` for one line, given the incoming block-comment state (line-local, no tokenizer)."""
    if not line.strip():
        return True, in_block                     # a blank line never decides a class
    if in_block:
        return True, "*/" not in line             # any line inside a block comment is prose
    head = line.lstrip()
    if head.startswith("//"):
        return True, False
    if head.startswith("/*"):
        return True, "*/" not in line
    if head.startswith("*"):
        return True, False                        # a ` * ...` continuation (or a `*/`)
    return False, False


def hunk_class(ours: list[str], theirs: list[str], in_block: bool) -> str:
    """`"code"`, `"prose"` or `"mixed"` for one conflict hunk (blank lines are neutral), at the region level."""
    has_prose = has_code = False
    for side in (ours, theirs):
        state = in_block                     # each side starts from the same incoming comment state
        for line in side:
            is_prose, state = prose_line(line, state)
            if is_prose:
                has_prose = True
            else:
                has_code = True
    if not has_prose:
        return "code"
    return "mixed" if has_code else "prose"


def union_side(ours: list[str], theirs: list[str]) -> list[str]:
    """The append-union of one hunk: ours first, then theirs' lines ours does not already carry."""
    merged = list(ours)
    for line in theirs:
        if line not in merged:
            merged.append(line)
    return merged


def branch_only_additions(base: list[str], branch: list[str]) -> list[str]:
    """The non-blank lines `branch` has and `base` has not - one side's additions."""
    return [l for l in branch if l.strip() and l not in base]


def missing_from(candidate: list[str], wanted: list[str]) -> list[str]:
    """The lines of `wanted` the `candidate` copy does not carry, in order."""
    return [l for l in wanted if l not in candidate]


@dataclass(frozen=True)
class Cover:
    """Each side's additions over a shared base and whether the other side carries them (the superset test)."""
    ours_add: tuple[str, ...]
    theirs_add: tuple[str, ...]
    ours_has_theirs: bool
    theirs_has_ours: bool


def cover(base: list[str], ours: list[str], theirs: list[str]) -> Cover:
    """The one superset computation `prose_superset` (a hunk) and `addadd_choice` (a whole file) decide on."""
    ours_add = branch_only_additions(base, ours)
    theirs_add = branch_only_additions(base, theirs)
    return Cover(tuple(ours_add), tuple(theirs_add), not missing_from(ours, theirs_add),
                 not missing_from(theirs, ours_add))


def prose_superset(ours: list[str], base: list[str], theirs: list[str]) -> tuple[str | None, str]:
    """Which side of a rewritten prose hunk carries the other's content: `(side|None, why)`.

    `ours` is main's copy and `theirs` the branch's; `why` names the side taken and the evidence. Neither side
    carrying the other is reported (`None`), never unioned.
    """
    cv = cover(base, ours, theirs)
    if cv.theirs_has_ours and cv.ours_has_theirs:
        return "ours", ("both copies carry the other's prose additions (%d main-side, %d branch-side) - "
                        "kept main's copy" % (len(cv.ours_add), len(cv.theirs_add)))
    if cv.theirs_has_ours:
        return "theirs", ("the branch's copy carries all %d line(s) main's side adds - the branch's copy "
                          "is the superset" % len(cv.ours_add))
    if cv.ours_has_theirs:
        return "ours", ("main's copy carries all %d line(s) the branch's side adds - main's copy is the "
                        "superset" % len(cv.theirs_add))
    return None, ("neither copy carries the other's prose additions (main's side lacks %d line(s), the "
                  "branch's lacks %d) - no superset is derivable"
                  % (len(missing_from(ours, list(cv.theirs_add))), len(missing_from(theirs, list(cv.ours_add)))))


def addadd_choice(base_lines: list[str], ours: list[str], theirs: list[str]) -> tuple[str | None, str]:
    """Which side of an add/add path carries the other's work: `("ours" | "theirs" | None, why)`.

    `base_lines` is the pre-rename base (`[]` when none is derivable: the copies are compared whole). A tie of
    mutual containment keeps the copy that is the literal superset, else main's; no superset is a refusal.
    """
    cv = cover(base_lines, ours, theirs)
    if cv.ours_has_theirs and cv.theirs_has_ours:
        if not missing_from(ours, theirs) and missing_from(theirs, ours):
            return "ours", ("both copies carry the other's additions and main's keeps every line the "
                             "branch's has - kept main's copy")
        if not missing_from(theirs, ours) and missing_from(ours, theirs):
            return "theirs", ("both copies carry the other's additions and the branch's keeps every "
                               "line main's has - kept the branch's copy")
        return "ours", ("both copies carry the other's additions (%d main-side, %d branch-side) - kept "
                         "main's copy" % (len(cv.ours_add), len(cv.theirs_add)))
    if cv.ours_has_theirs:
        return "ours", ("main's copy carries all %d line(s) only the branch's copy adds - main's copy "
                         "is the superset" % len(cv.theirs_add))
    if cv.theirs_has_ours:
        return "theirs", ("the branch's copy carries all %d line(s) only main's copy adds - the "
                           "branch's copy is the superset" % len(cv.ours_add))
    miss_ours = missing_from(ours, list(cv.theirs_add))
    miss_theirs = missing_from(theirs, list(cv.ours_add))
    return None, ("neither copy is a superset: main's lacks %d line(s) the branch's adds (e.g. %r) and the "
                  "branch's lacks %d of main's (e.g. %r) - no base and no superset, so this one is the "
                  "author's call (`git merge-file -p --diff3` with the pre-rename path as the base)"
                  % (len(miss_ours), (miss_ours[0][:50] if miss_ours else ""),
                     len(miss_theirs), (miss_theirs[0][:50] if miss_theirs else "")))


# --- the union -------------------------------------------------------------------------------------------------

def union_markers(text: str, path: str = "") -> tuple[str, int, list[dict]]:
    """Union a `--diff3` merge result by hunk class: `(merged_text, conflict_hunks, decisions)`.

    One decision per hunk: `{"hunk", "class", "took", "why", "blocked"?|"warning"?}`. A code hunk (or a prose
    insertion with an empty base) unions; a prose or mixed hunk takes the superset side; a mixed hunk with no
    superset unions and warns (naming `path`); a prose hunk with no superset is `blocked` (the caller refuses).
    """
    out: list[str] = []
    conflicts = 0
    decisions: list[dict] = []
    in_block = False
    for seg in segments(text):
        if not isinstance(seg, Hunk):
            out.append(seg)
            _, in_block = prose_line(seg, in_block)
            continue
        conflicts += 1
        ours, base, theirs = list(seg.ours), list(seg.base), list(seg.theirs)
        klass = hunk_class(ours, theirs, in_block)
        decision: dict = {"hunk": conflicts, "class": klass, "took": None, "why": ""}
        if klass == "code" or (klass == "prose" and not seg.has_base):
            chosen = union_side(ours, theirs)
            decision["why"] = ("additive declaration block - unioned (ours then theirs)" if klass == "code"
                               else "prose insertion with no base lines - unioned")
        else:
            side, why = prose_superset(ours, base, theirs)
            if side is not None:
                chosen = ours if side == "ours" else theirs
                decision.update(took=side, why=why)
            elif klass == "mixed":
                chosen = union_side(ours, theirs)
                decision["why"] = why
                decision["warning"] = ("%s: a conflict region mixes comment and code and no side carries "
                                       "the other's content (%s) - unioned as before, check it by hand"
                                       % (path or "<unknown file>", why))
            else:
                chosen = list(ours)                      # a placeholder; the caller refuses to commit it
                decision.update(blocked=True, why=why)
        out.extend(chosen)
        for chosen_line in chosen:
            _, in_block = prose_line(chosen_line, in_block)
        decisions.append(decision)
    return "".join(out), conflicts, decisions
