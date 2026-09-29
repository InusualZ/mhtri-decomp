#!/usr/bin/env python3
"""The one union rule: **code hunks union, prose hunks take the superset**, shared by every resolver.

`tools/units/mergebranch.py` (the lane's `main`-into-branch merge) and `tools/units/unionresolve.py`
(the landing path's resolver, called by `land.py`) both union a `--diff3` conflict, and they carried the
**same** defect independently: a comment paragraph both sides rewrote was unioned line-by-line, which
duplicated the prose mid-sentence (`/* … /* …`) and reintroduced the older side's generated names.  The
merger lane hit it twice in one merge on 2026-09-29 (`include/unsplit/lobby.h`,
`include/lobby/fn_801F3294.h`) and resolved by hand; `mergebranch` was fixed in `7099e70d9`, but
`unionresolve.union_text` still had the old append-union.  Fixing one and not the other is exactly how the
two drift, so the classification and the superset rule live here once, and both import it.

The distinction is **per hunk, not per file**:

* an **additive declaration block** - one side's lines are additions to the other's - unions as before
  (`ours`, then `theirs`), which every registration merge depends on;
* a **comment paragraph both sides rewrote** takes the **superset** side - the copy that already carries
  every non-blank line the other side changed relative to the shared base - never a union;
* a **mixed** comment-and-code hunk prefers the superset when one is derivable and otherwise keeps the
  union and **warns**, naming the file; a prose hunk with no superset is **blocked**, never unioned.

`prose_line` is deliberately line-local, not a C tokenizer: a line is prose when it is blank, starts with
`//`, `/*` or `*`, or lies inside an open block comment (the paragraph's opening `/*` is frequently
*before* the conflict hunk, so that state is carried in).  That is exactly the shape that separates the two
classes.

    python tools/units/unionprose.py --selftest
"""
from __future__ import annotations

import argparse
import sys


def prose_line(line: str, in_block: bool) -> tuple[bool, bool]:
    """`(is_prose, still_in_block)` for one line of a conflict, given the incoming comment state.

    The union rule has to tell an **additive declaration block** (one side's lines are additions to the
    other's - union is correct) from a **comment paragraph both sides rewrote** (union duplicates the prose
    and reintroduces the other side's old generated names).  A line is prose when it is blank, starts with
    `//`, `/*` or `*`, or lies inside an open block comment (the "inside one" case: the paragraph's opening
    `/*` is frequently *before* the conflict hunk, so the state is carried in).  This is deliberately
    line-local, not a C tokenizer - it is exactly the shape that separates the two classes.
    """
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
    """`"code"`, `"prose"` or `"mixed"` for one conflict hunk (blank lines are neutral).

    Classified at the **region** level, not per side: a hunk where the two copies carry a comment line
    *and* a code line is `mixed` (the caller prefers a superset for it and warns otherwise); a hunk with
    no comment line is a declaration/body conflict - today's union target; a hunk with no code line is
    prose, and union is exactly what must not happen to it.
    """
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
    """The non-blank lines the branch has and the base has not - its side of the merge, for the proof."""
    return [l for l in branch if l.strip() and l not in base]


def missing_from(candidate: list[str], wanted: list[str]) -> list[str]:
    """The lines of `wanted` the `candidate` copy does not carry, in order."""
    return [l for l in wanted if l not in candidate]


def prose_superset(ours: list[str], base: list[str], theirs: list[str]) -> tuple[str | None, str]:
    """Which side of a rewritten **prose** hunk carries the other's content: `(side|None, why)`.

    A comment paragraph both sides rewrote must not be unioned (2026-09-29: a merger lane hit this twice in
    one merge - `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h` - and the append-union appended
    one side's lines to the other's, duplicating the paragraph mid-comment and reintroducing the old
    generated names).  The correct side is the **superset**: the copy that already carries every non-blank
    line the other side changed relative to the shared base - the same additions/superset rule
    `mergebranch.addadd_choice` uses, applied to one hunk.  `ours` is the copy on `main` and `theirs` the
    copy on the branch (the convention throughout), and `why` always names the side taken and the evidence,
    so the choice is auditable rather than inferred.  Neither side carrying the other is **not** a guess: it
    is reported, never unioned.
    """
    ours_add = branch_only_additions(base, ours)
    theirs_add = branch_only_additions(base, theirs)
    theirs_has_ours = not missing_from(theirs, ours_add)
    ours_has_theirs = not missing_from(ours, theirs_add)
    if theirs_has_ours and ours_has_theirs:
        return "ours", ("both copies carry the other's prose additions (%d main-side, %d branch-side) - "
                        "kept main's copy" % (len(ours_add), len(theirs_add)))
    if theirs_has_ours:
        return "theirs", ("the branch's copy carries all %d line(s) main's side adds - the branch's copy "
                          "is the superset" % len(ours_add))
    if ours_has_theirs:
        return "ours", ("main's copy carries all %d line(s) the branch's side adds - main's copy is the "
                        "superset" % len(theirs_add))
    return None, ("neither copy carries the other's prose additions (main's side lacks %d line(s), the "
                  "branch's lacks %d) - no superset is derivable"
                  % (len(missing_from(ours, theirs_add)), len(missing_from(theirs, ours_add))))


def union_markers(text: str, path: str = "") -> tuple[str, int, list[dict]]:
    """Union a `--diff3` merge result **by hunk class**: code hunks as before, prose hunks by superset.

    Never applied blindly - a union of two *edits* duplicates both (an `if` block was duplicated that way
    and cost an hour).  It stays right for an **additive declaration block** (a band header's declarations,
    a block in if/else form) and that behaviour is pinned by the selftest.  It is wrong for a **prose region
    both sides rewrote**: appending one side's lines to the other's duplicates the paragraph mid-comment and
    reintroduces the older side's generated names (the 2026-09-29 merger lane).  For a prose hunk the
    resolution is the **superset side**, and the decision is returned so which copy was taken, and why, is
    auditable.  A **mixed** hunk (comment and code lines) prefers the superset when one is derivable and
    otherwise keeps the old union and returns a warning naming the file.  A prose hunk with no superset is
    marked `blocked` - the caller refuses it, it is never unioned.

    Returns `(merged_text, conflict_hunks, decisions)`, one decision per hunk:
    `{"hunk", "class", "took", "why", "blocked"?|"warning"?}`.
    """
    out: list[str] = []
    source = text.splitlines(keepends=True)
    i, conflicts = 0, 0
    decisions: list[dict] = []
    in_block = False
    while i < len(source):
        line = source[i]
        if not line.startswith("<<<<<<<"):
            out.append(line)
            _, in_block = prose_line(line, in_block)
            i += 1
            continue
        # --- one conflict hunk: split it into ours / base / theirs ------------------------------------
        conflicts += 1
        ours: list[str] = []
        base: list[str] = []
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
            elif mode == "base":
                base.append(source[i])
            else:
                theirs.append(source[i])
            i += 1
        if i < len(source):
            i += 1                                       # consume the `>>>>>>>` line
        # --- classify and resolve ---------------------------------------------------------------------
        klass = hunk_class(ours, theirs, in_block)
        decision: dict = {"hunk": conflicts, "class": klass, "took": None, "why": ""}
        if klass == "code" or (klass == "prose" and not any(l.strip() for l in base)):
            # An additive declaration block, or a prose insertion whose base has no lines (both sides only
            # *added*): the union is safe by construction, and is the behaviour merges depend on.  A
            # **mixed** hunk always goes through the superset check (and warns when there is none), because
            # a comment line and a code line at the same anchor is exactly where a blind union is unsafe.
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


# --------------------------------------------------------------------------------------------------
# self-test: pure text, no git and no repository state - the shared rule pinned directly.
# --------------------------------------------------------------------------------------------------

# The real region from `include/unsplit/lobby.h` at the recorded revisions (base `1fb32e620`,
# main `093017eaf`, branch `aa1c85431`) - the merge `d21bc02d5` on `worker/menu-num-8725`, where both
# sides rewrote the same comment paragraph and the old append-union duplicated it mid-sentence.
REAL_LOBBY_CONFLICT = (
    "<<<<<<< ours\n"
    "/* `fn_802A7C04`/`fn_802A8EC0`/`fn_802A8ED8`/`menu_cursor_step`/`fn_802A8F50` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/fn_802A6624.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    "||||||| base\n"
    "/* `fn_802A7C04`/`fn_802A8EC0`/`fn_802A8ED8`/`menu_cursor_step`/`fn_802A8F50` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/fn_802A6624.cpp` owns that range\n"
    " * now, so its header `include/menu/fn_802A6624.h` declares them and this header includes it (rule 2).\n"
    "=======\n"
    "/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    ">>>>>>> theirs\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n")
REAL_LOBBY_SUPERSET = (
    "/* `menu_hold_row_draw_by_lsp`/`menu_cursor_step_fixed_tail`/`menu_cursor_step_open_last`/`menu_cursor_step`/`toggle_word_step` (0x802A7C04-0x802A8F50) were\n"
    " * declared here while the menu band had no registered unit.  `menu/menu_message.cpp` owns that range\n"
    " * now, so its header `include/menu/menu_message.h` declares them and this header includes it (rule 2).\n"
    " * mismatch is the `(10505) illegal overloading` this move clears. */\n")

# The old generated names a union reintroduces (the branch's rename sweep removed them).
OLD_GENERATED_NAMES = ("fn_802A7C04", "fn_802A8EC0", "fn_802A8ED8", "fn_802A8F50")

# The real region from `include/lobby/fn_801F3294.h` at the same revisions - the *second* conflict the
# merger lane hit in that one merge.  Its hunk opens **mid-comment** (the paragraph's `/*` is above the
# hunk), which is why the classifier carries the block state in rather than restarting per side.
REAL_FN_CONFLICT = (
    "/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow\n"
    " * view - while its band had no registered unit, and the two spellings could not both be visible\n"
    "<<<<<<< ours\n"
    " * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    "||||||| base\n"
    " * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header\n"
    " * `include/menu/fn_802A6624.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    "=======\n"
    " * ((10505) illegal overloading).  `menu/menu_message.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    ">>>>>>> theirs\n"
    " * (docs/plan.md 6.5 rule 2). */\n")
REAL_FN_SUPERSET = (
    "/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow\n"
    " * view - while its band had no registered unit, and the two spellings could not both be visible\n"
    " * ((10505) illegal overloading).  `menu/menu_message.cpp` owns the address and its header\n"
    " * `include/menu/menu_message.h` (included below) declares the definition's `s32`/`u16` spelling\n"
    " * (docs/plan.md 6.5 rule 2). */\n")


def nested_comment(text: str) -> bool:
    """True when a `/*` opens while a block comment is already open - the duplicated-prose signature."""
    in_block = False
    for line in text.split("\n"):
        j = 0
        while j < len(line):
            if line.startswith("*/", j) and in_block:
                in_block, j = False, j + 2
                continue
            if line.startswith("/*", j):
                if in_block:
                    return True
                in_block, j = True, j + 2
                continue
            j += 1
    return False


def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # (a) an ADDITIVE DECLARATION BLOCK - a registration-shaped hunk - still unions as before.
    additive = ("Sections:\n"
                "anchor.cpp:\n"
                "\t.text       start:0x80000000 end:0x80000800\n"
                "<<<<<<< ours\n"
                "menu/main.cpp:\n"
                "\t.text       start:0x80001000 end:0x80002000\n"
                "||||||| base\n"
                "=======\n"
                "menu/branch.cpp:\n"
                "\t.text       start:0x80000800 end:0x80001000\n"
                ">>>>>>> theirs\n"
                "menu/tail.cpp:\n"
                "\t.text       start:0x80002000 end:0x80003000\n")
    add_merged, add_hunks, add_dec = union_markers(additive, "config/RMHE08/splits.txt")
    check("an additive declaration block still unions both sides", add_merged,
          "Sections:\n"
          "anchor.cpp:\n"
          "\t.text       start:0x80000000 end:0x80000800\n"
          "menu/main.cpp:\n"
          "\t.text       start:0x80001000 end:0x80002000\n"
          "menu/branch.cpp:\n"
          "\t.text       start:0x80000800 end:0x80001000\n"
          "menu/tail.cpp:\n"
          "\t.text       start:0x80002000 end:0x80003000\n")
    check("... one hunk, classified code", (add_hunks, [d["class"] for d in add_dec]), (1, ["code"]))
    check("... no side taken, no warning",
          ([d["took"] for d in add_dec], [d.get("warning") for d in add_dec]), ([None], [None]))
    check("... the union is ours-then-theirs, not a superset", add_merged.count("menu/branch.cpp"), 1)

    # (b) the REAL rewritten paragraph: the superset side is taken, never a union.
    merged, hunks, dec = union_markers(REAL_LOBBY_CONFLICT, "include/unsplit/lobby.h")
    check("the real prose rewrite is one prose hunk", (hunks, [d["class"] for d in dec]), (1, ["prose"]))
    check("... it takes the branch's superset copy", [d["took"] for d in dec], ["theirs"])
    check("... and says so, naming the branch", all("superset" in d["why"] for d in dec), True)
    check("... the resolution IS the superset copy", merged, REAL_LOBBY_SUPERSET)
    check("... no duplicated prose (no nested `/*`)", nested_comment(merged), False)
    check("... no old generated name reintroduced",
          any(n in merged for n in OLD_GENERATED_NAMES), False)

    # the second real conflict opens mid-comment (the `/*` is above the hunk): still prose, still superset
    fn_merged, fn_hunks, fn_dec = union_markers(REAL_FN_CONFLICT, "include/lobby/fn_801F3294.h")
    check("the mid-comment real hunk is prose (the open block is carried in)",
          (fn_hunks, [d["class"] for d in fn_dec]), (1, ["prose"]))
    check("... it takes the branch's superset copy", [d["took"] for d in fn_dec], ["theirs"])
    check("... the resolution IS the superset copy", fn_merged, REAL_FN_SUPERSET)
    check("... and no old file/name is reintroduced",
          ("menu/fn_802A6624.cpp" in fn_merged, "include/menu/fn_802A6624.h" in fn_merged), (False, False))

    # (c) a MIXED comment+code region: no superset here, so it keeps the union and REPORTS it.
    mixed = ("int a;\n"
             "<<<<<<< ours\n"
             "/* main's paragraph. */\n"
             "int a_main;\n"
             "||||||| base\n"
             "int a_base;\n"
             "=======\n"
             "/* the branch's paragraph. */\n"
             "int a_lane;\n"
             ">>>>>>> theirs\n"
             "int z;\n")
    mix_merged, _mix_hunks, mix_dec = union_markers(mixed, "include/mixed/thing.h")
    check("a mixed comment+code region still unions as before", mix_merged,
          "int a;\n/* main's paragraph. */\nint a_main;\n/* the branch's paragraph. */\nint a_lane;\nint z;\n")
    check("... classified mixed", [d["class"] for d in mix_dec], ["mixed"])
    check("... takes no side when there is no superset", [d["took"] for d in mix_dec], [None])
    check("... and WARNS, naming the file",
          ("include/mixed/thing.h" in (mix_dec[0].get("warning") or ""),
           "unioned as before" in (mix_dec[0].get("warning") or "")), (True, True))

    # a mixed region WITH a superset prefers it, with no warning
    mixed_sup = ("<<<<<<< ours\n"
                 "/* main's paragraph. */\n"
                 "int a_main;\n"
                 "||||||| base\n"
                 "int a_base;\n"
                 "=======\n"
                 "/* main's paragraph. */\n"
                 "int a_main;\n"
                 "int a_branch_extra;\n"
                 ">>>>>>> theirs\n")
    ms_merged, _ms_hunks, ms_dec = union_markers(mixed_sup, "include/mixed/thing.h")
    check("a mixed region with a superset prefers it",
          ([d["took"] for d in ms_dec], ms_merged),
          (["theirs"], "/* main's paragraph. */\nint a_main;\nint a_branch_extra;\n"))
    check("... with no warning (the superset is sound)", [d.get("warning") for d in ms_dec], [None])

    # a PROSE region with no superset is blocked, never unioned
    prose_no_sup = ("<<<<<<< ours\n"
                    "/* main changed one word. */\n"
                    "||||||| base\n"
                    "/* the base paragraph. */\n"
                    "=======\n"
                    "/* the branch changed another. */\n"
                    ">>>>>>> theirs\n")
    pns_dec = union_markers(prose_no_sup, "include/else/thing.h")[2]
    check("a prose region with no superset is blocked, not unioned",
          (pns_dec[0]["class"], pns_dec[0].get("blocked"), pns_dec[0]["took"]), ("prose", True, None))

    # no markers -> no change
    check("no markers is no change", union_markers("a\nb\n")[0], "a\nb\n")
    check("... with no hunks", union_markers("a\nb\n")[1], 0)

    if fails:
        print("unionprose: %d check(s), %d failure(s)" % (checks, len(fails)))
        for f in fails:
            print("  FAIL " + f)
        return 1
    print("unionprose: %d check(s), 0 failure(s)" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    return selftest()


if __name__ == "__main__":
    sys.exit(main())
