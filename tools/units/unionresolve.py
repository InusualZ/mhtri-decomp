#!/usr/bin/env python3
"""The union resolver and its invariant assertions, for `land.py`.

Eight of the thirteen live branches conflict with `main` on **one** class, and it is not a defect: two
sibling bands register *adjacent address ranges*, so their `config/RMHE08/splits.txt` blocks and
`configure.py` `Object(...)` lines append at the *same anchor*.  That is an add/add conflict whose
resolution is the pure **append-union** - ours block then theirs - which is also the address order both
files require.  There are no header conflicts in this class.

The union is **per hunk, not per file** (`unionprose.union_markers`, the one rule shared with
`mergebranch.py`): an additive declaration block still appends, but a **comment paragraph both sides
rewrote** takes the **superset** side.  This module's `union_text` used to append every hunk, which is the
same prose-union defect `mergebranch` was fixed for (`7099e70d9`) - it duplicated the paragraph
mid-sentence and reintroduced the older side's generated names (2026-09-29: `include/unsplit/lobby.h` and
`include/lobby/fn_801F3294.h`).  `land.py` refuses a header conflict before it reaches this union anyway,
but `union_text`/`union_file` are public and a hand `union_file <header>` was a live path to the defect;
now a prose hunk with no superset is `blocked` and `land._union_conflicts` refuses it rather than writing
the placeholder.

This module is the land path's resolver, called by `land.py` directly, and it contains **no staging at
all**: the caller stages the two scoped paths explicitly (`git add -- <paths>`), never the whole index
(a `git add -A` sweep has bitten this campaign twice).

It also carries the assertions a wrong union breaks *silently* - the reason the union is not trusted:

* **no duplicate unit key** in the merged `splits.txt`;
* **no duplicate `Object()` line** in the merged `configure.py`;
* **no overlapping `.text`/`extab`/`extabindex` range anywhere** in the merged `splits.txt`;
* **every unit `main` registered is still present** in both files (the union only adds; it must never
  drop a registration `main` already had, which is what an apply that took one side's whole file does).

`land.resolve_conflicts` is the only caller: it unions the conflicted working-tree text in memory, runs
`check_union` on the result *before* writing anything, and refuses on any violation - so a bad union never
touches the tree.

    python tools/units/unionresolve.py --selftest
"""
from __future__ import annotations

import argparse
import collections
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
# `unionprose` is the one union rule (code hunks union, prose hunks take the superset), shared with
# `mergebranch.py`: `union_text` had the same prose-union defect `mergebranch.union_markers` was fixed
# for, so the classification lives once and both import it rather than drift apart.
for _path in (TOOLS, HERE):
    if _path not in sys.path:
        sys.path.insert(0, _path)

from units import unionprose as up  # noqa: E402

# The only two paths a registration append-conflict may touch.  A conflicted header or a `src/**` file is
# a *different* class (a real content conflict) and is never unioned here: a plain union of a HEADER
# stacks two `#ifdef __cplusplus }` closings and drops an `#endif`.
UNION_SCOPE = ("configure.py", "config/RMHE08/splits.txt")

# The sections the merged map must never overlap in.  splits.txt also carries `.data`/`.rodata`/... rows;
# the task names these three because they are where adjacent bands meet and where a bad union shows first.
OVERLAP_SECTIONS = (".text", "extab", "extabindex")

# A unit header is an unindented line ending in `:` (`Sections:` is the file's own legend, not a unit), and
# every range is an indented `section start:0x.. end:0x..` line - the shape `land._split_rows` and
# `stylelint._parse_splits` parse.
_UNIT_RE = re.compile(r"^(\S.*):\s*$")
_RANGE_RE = re.compile(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)")
# `Object(<kind>, "<unit>")` on one line, optional comma/whitespace - `recompile.OBJECT_RE`'s shape.
_OBJECT_RE = re.compile(r"Object\(\s*([A-Za-z_]\w*)\s*,\s*\"([^\"]+)\"\s*\)")


def union_text_full(text: str, path: str = "") -> tuple[str, int, list[dict]]:
    """Union a `--diff3` merge result **by hunk class**; `(merged, hunks, decisions)`.

    The rule is `unionprose.union_markers`, the one implementation shared with `mergebranch.py`: an
    **additive declaration block** (the registration class this module exists for) unions as before, but a
    **comment paragraph both sides rewrote** takes the **superset** side instead of being appended line-by-
    line - which duplicated the prose mid-sentence and reintroduced the older side's generated names (the
    2026-09-29 merger lane, `include/unsplit/lobby.h` and `include/lobby/fn_801F3294.h`).  A `--diff3`
    conflict carries an extra `|||||||` base section; it is used for the superset proof and never emitted.
    A file with no markers is returned unchanged with a hunk count of zero, so a caller can tell "resolved
    something" from "nothing to do".

    `decisions` is one dict per hunk (`unionprose.union_markers`' shape): a hunk with `blocked` must be
    refused by the caller, never written - `land._union_conflicts` does exactly that.
    """
    return up.union_markers(text, path)


def union_text(text: str) -> tuple[str, int]:
    """`union_text_full` without the decisions: `(merged, hunks_resolved)`.

    Kept for callers that only need the text; the landing path uses `union_text_full` so it can refuse a
    `blocked` prose hunk instead of writing the placeholder.
    """
    merged, hunks, _decisions = union_text_full(text)
    return merged, hunks


def union_file(path: str) -> int:
    """Union-resolve `path` in place; the number of hunks resolved (0 = it had no markers).

    A `blocked` hunk (a prose paragraph neither side is the superset of) is **not written**: the file is
    left alone and a `REFUSED` line is printed, because writing the placeholder would silently drop one
    side's prose.  Every non-code hunk is reported so which rule fired is visible.
    """
    with open(path, encoding="utf-8", newline="") as fh:
        text = fh.read()
    merged, hunks, decisions = union_text_full(text, path)
    if hunks:
        for d in decisions:
            if d["class"] != "code":
                tag = "BLOCKED" if d.get("blocked") else "took %s" % d["took"]
                print("    %-32s hunk %d %-5s %-9s %s" % (path, d["hunk"], d["class"], tag, d["why"]))
    if hunks and any(d.get("blocked") for d in decisions):
        print("    %-32s REFUSED - a prose hunk has no superset; resolve it by hand" % path)
        return 0
    if hunks:
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(merged)
    return hunks


def split_units(text: str) -> list[str]:
    """The unit keys of a `splits.txt` text, in file order (the order is the address order)."""
    out: list[str] = []
    for line in text.splitlines():
        if line.startswith("Sections:"):
            continue
        m = _UNIT_RE.match(line)
        if m:
            out.append(m.group(1).strip())
    return out


def split_ranges(text: str) -> list[tuple[str, str, int, int]]:
    """`(unit, section, start, end)` for every range in a `splits.txt` text."""
    rows: list[tuple[str, str, int, int]] = []
    cur: str | None = None
    for line in text.splitlines():
        if line.startswith("Sections:"):
            continue
        m = _UNIT_RE.match(line)
        if m:
            cur = m.group(1).strip()
            continue
        r = _RANGE_RE.match(line)
        if r and cur:
            rows.append((cur, r.group(1), int(r.group(2), 16), int(r.group(3), 16)))
    return rows


def configure_objects(text: str) -> list[str]:
    """Every `Object(kind, "unit")` as a normalised string, in file order."""
    return ["Object(%s, \"%s\")" % (m.group(1), m.group(2)) for m in _OBJECT_RE.finditer(text)]


def object_names(text: str) -> list[str]:
    """Every unit name a `configure.py` text registers via `Object(...)`, in file order."""
    return [m.group(2) for m in _OBJECT_RE.finditer(text)]


def duplicate_unit_keys(units: list[str]) -> list[str]:
    """The unit keys a merged `splits.txt` names more than once (the append-union's classic breakage)."""
    counts = collections.Counter(units)
    return sorted(unit for unit, n in counts.items() if n > 1)


def duplicate_objects(objects: list[str]) -> list[str]:
    """The `Object()` lines a merged `configure.py` names more than once."""
    counts = collections.Counter(objects)
    return sorted(obj for obj, n in counts.items() if n > 1)


def overlapping_ranges(rows: list[tuple[str, str, int, int]],
                       sections: tuple[str, ...] = OVERLAP_SECTIONS) -> list[tuple]:
    """Every pair of ranges in a named section that overlap: `(section, unit_a, sa, ea, unit_b, sb, eb)`.

    Ranges are half-open (`start <= addr < end`), so two rows that merely *touch* (`a.end == b.start`) are
    the correct seam, not an overlap; only `next.start < previous.end` is a defect.
    """
    by_section: dict[str, list[tuple[int, int, str]]] = {}
    for unit, section, start, end in rows:
        if section in sections:
            by_section.setdefault(section, []).append((start, end, unit))
    out: list[tuple] = []
    for section, spans in by_section.items():
        spans.sort()
        for k in range(1, len(spans)):
            prev_start, prev_end, prev_unit = spans[k - 1]
            start, end, unit = spans[k]
            if start < prev_end:
                out.append((section, prev_unit, prev_start, prev_end, unit, start, end))
    return out


def check_union(main_splits: str, merged_splits: str,
                main_configure: str, merged_configure: str) -> list[str]:
    """Every silent breakage of a wrong union, as readable lines (empty = the union is sound).

    The four assertions are deliberately about the *merged* files against **`main`'s** own text, because
    the failure they exist for is the one a green build cannot see: a union that drops `main`'s sibling
    registration, or stacks a unit twice, or lets two bands claim one range - none of which stops `ninja`
    from relinking the old DOL.
    """
    problems: list[str] = []
    for unit in duplicate_unit_keys(split_units(merged_splits)):
        problems.append("duplicate unit key in the merged splits.txt: %s" % unit)
    for obj in duplicate_objects(configure_objects(merged_configure)):
        problems.append("duplicate Object() line in the merged configure.py: %s" % obj)
    for section, ua, sa, ea, ub, sb, eb in overlapping_ranges(split_ranges(merged_splits)):
        problems.append("overlapping %s range in the merged splits.txt: %s 0x%X-0x%X and %s 0x%X-0x%X"
                        % (section, ua, sa, ea, ub, sb, eb))
    merged_units = set(split_units(merged_splits))
    for unit in split_units(main_splits):
        if unit not in merged_units:
            problems.append("a unit main registered is missing from the merged splits.txt: %s" % unit)
    merged_objects = set(configure_objects(merged_configure))
    for obj in configure_objects(main_configure):
        if obj not in merged_objects:
            problems.append("a unit main registered is missing from the merged configure.py: %s" % obj)
    return problems


# --------------------------------------------------------------------------------------------------
# self-test: pure text, no git and no repository state.
# --------------------------------------------------------------------------------------------------

def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # --- the union itself -------------------------------------------------------------------------
    both = ("head\n<<<<<<< ours\nmain block\n=======\nbranch block\n>>>>>>> theirs\ntail\n")
    merged, hunks = union_text(both)
    check("a conflict unions to ours-then-theirs", merged, "head\nmain block\nbranch block\ntail\n")
    check("... and counts the hunk", hunks, 1)
    check("... leaving no marker", "<<<<<<<" in merged or ">>>>>>>" in merged, False)
    clean, n = union_text("no conflict here\n")
    check("a clean file is passed through unchanged", clean, "no conflict here\n")
    check("... with no hunk", n, 0)
    d3 = "a\n<<<<<<< ours\no\n||||||| base\nstale\n=======\nt\n>>>>>>> theirs\nz\n"
    check("a --diff3 conflict drops the base section", union_text(d3)[0], "a\no\nt\nz\n")

    # --- the union by hunk class (the 2026-09-29 prose defect) ------------------------------------
    # (a) an ADDITIVE DECLARATION BLOCK - the registration class this module exists for - still unions,
    # and every decision is `code`, so the landing path's behaviour is unchanged for it.
    additive = ("Sections:\n"
                "anchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n"
                "<<<<<<< ours\n"
                "menu/main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n"
                "||||||| base\n"
                "=======\n"
                "menu/branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n"
                ">>>>>>> theirs\n"
                "menu/tail.cpp:\n\t.text       start:0x80002000 end:0x80003000\n")
    a_merged, a_hunks, a_dec = union_text_full(additive, "config/RMHE08/splits.txt")
    check("an additive declaration block still unions both sides",
          ("menu/main.cpp" in a_merged, "menu/branch.cpp" in a_merged,
           a_merged.index("menu/main.cpp") < a_merged.index("menu/branch.cpp")), (True, True, True))
    check("... it is one code hunk with no side taken and no warning",
          (a_hunks, [d["class"] for d in a_dec], [d["took"] for d in a_dec],
           [d.get("warning") for d in a_dec]), (1, ["code"], [None], [None]))

    # (b) the two REAL conflicts from the recorded revisions (base `1fb32e620`, main `093017eaf`,
    # branch `aa1c85431`): both sides rewrote the same comment paragraph, so the resolution is the
    # branch's superset copy - asserted by ABSENCE as well as equality.
    for rel, conflict, superset, real_path in (
            ("include/unsplit/lobby.h", up.REAL_LOBBY_CONFLICT, up.REAL_LOBBY_SUPERSET,
             "include/unsplit/lobby.h"),
            ("include/lobby/fn_801F3294.h", up.REAL_FN_CONFLICT, up.REAL_FN_SUPERSET,
             "include/lobby/fn_801F3294.h")):
        r_merged, r_hunks, r_dec = union_text_full(conflict, real_path)
        check("%s: every real hunk is prose" % rel, [d["class"] for d in r_dec], ["prose"] * r_hunks)
        check("%s: every prose hunk takes the branch's superset" % rel, [d["took"] for d in r_dec],
              ["theirs"] * r_hunks)
        check("%s: the resolution equals the superset blob region" % rel, r_merged, superset)
        check("%s: NO duplicated prose (no nested `/*`)" % rel, up.nested_comment(r_merged), False)
        check("%s: NO old generated name reintroduced" % rel,
              any(n in r_merged for n in up.OLD_GENERATED_NAMES), False)
        check("%s: NO old `menu/fn_802A6624.*` path reintroduced" % rel,
              ("menu/fn_802A6624.cpp" in r_merged, "include/menu/fn_802A6624.h" in r_merged),
              (False, False))
    # the old (unfixed) union really would have duplicated the prose - the defect is real, not asserted
    def _old_union(text):
        """The pre-fix behaviour: every hunk as ours-then-theirs, base dropped."""
        out, i, src = [], 0, text.splitlines(keepends=True)
        while i < len(src):
            if src[i].startswith("<<<<<<<"):
                ours, theirs, mode = [], [], "ours"
                i += 1
                while i < len(src) and not src[i].startswith(">>>>>>>"):
                    if src[i].startswith("|||||||"):
                        mode = "base"
                    elif src[i].startswith("======="):
                        mode = "theirs"
                    elif mode == "ours":
                        ours.append(src[i])
                    elif mode == "theirs":
                        theirs.append(src[i])
                    i += 1
                out.extend(ours)
                out.extend(theirs)
            else:
                out.append(src[i])
            i += 1
        return "".join(out)

    check("... and the plain union WOULD have duplicated it (the defect is real)",
          up.nested_comment(_old_union(up.REAL_LOBBY_CONFLICT)), True)

    # (c) a MIXED comment+code region: no superset, so it keeps the union and REPORTS what it did.
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
    mix_merged, _mh, mix_dec = union_text_full(mixed, "include/mixed/thing.h")
    check("a mixed region still unions as before", mix_merged,
          "int a;\n/* main's paragraph. */\nint a_main;\n/* the branch's paragraph. */\nint a_lane;\nint z;\n")
    check("... it is reported (mixed, no side, a warning naming the file)",
          (mix_dec[0]["class"], mix_dec[0]["took"], "include/mixed/thing.h" in mix_dec[0].get("warning", "")),
          ("mixed", None, True))

    # (d) a PROSE hunk with no superset is blocked, and `union_file` refuses to write it (never a silent
    # drop of one side's prose).
    prose_no_sup = ("<<<<<<< ours\n"
                    "/* main changed one word. */\n"
                    "||||||| base\n"
                    "/* the base paragraph. */\n"
                    "=======\n"
                    "/* the branch changed another. */\n"
                    ">>>>>>> theirs\n")
    _pns_merged, _ph, pns_dec = union_text_full(prose_no_sup, "include/else/thing.h")
    check("a prose hunk with no superset is blocked", (pns_dec[0]["class"], pns_dec[0].get("blocked")),
          ("prose", True))
    with tempfile.TemporaryDirectory() as tmp:
        victim = os.path.join(tmp, "thing.h")
        with open(victim, "w", encoding="utf-8", newline="") as fh:
            fh.write(prose_no_sup)
        check("... union_file reports no hunk resolved (it did not write)", union_file(victim), 0)
        with open(victim, encoding="utf-8", newline="") as fh:
            check("... and the conflict is left intact for a hand resolution", fh.read(), prose_no_sup)

    # --- duplicate keys / objects -----------------------------------------------------------------
    splits = ("Sections:\n\t.text       type:code align:32\n\n"
              "a.cpp:\n\t.text       start:0x1000 end:0x1100\n\n"
              "a.cpp:\n\t.text       start:0x2000 end:0x2100\n")
    check("a duplicated unit key is found", duplicate_unit_keys(split_units(splits)), ["a.cpp"])
    conf = ('config.libs = [\n'
            '    Object(NonMatching, "a.cpp"),\n'
            '    Object(NonMatching, "a.cpp"),\n]\n')
    check("a duplicated Object() line is found", duplicate_objects(configure_objects(conf)),
          ['Object(NonMatching, "a.cpp")'])
    check("object_names reads the registered unit", object_names(conf), ["a.cpp", "a.cpp"])

    # --- overlaps ---------------------------------------------------------------------------------
    rows = [("a.cpp", ".text", 0x1000, 0x1100), ("b.cpp", ".text", 0x10F0, 0x1200)]
    check("an overlapping .text range is found", len(overlapping_ranges(rows)), 1)
    check("the overlap names both units", overlapping_ranges(rows)[0][1:3], ("a.cpp", 0x1000))
    touch = [("a.cpp", ".text", 0x1000, 0x1100), ("b.cpp", ".text", 0x1100, 0x1200)]
    check("touching ranges are the seam, not an overlap", overlapping_ranges(touch), [])
    other = [("a.cpp", ".data", 0x1000, 0x1100), ("b.cpp", ".data", 0x1050, 0x1200)]
    check("a .data overlap is out of the named sections", overlapping_ranges(other), [])

    # --- check_union: a sound append-union is clean -----------------------------------------------
    main_splits = ("Sections:\n\t.text       type:code align:32\n\n"
                   "anchor.cpp:\n\t.text       start:0x80000000 end:0x80000800\n\n"
                   "main.cpp:\n\t.text       start:0x80001000 end:0x80002000\n")
    branch_add = ("branch.cpp:\n\t.text       start:0x80000800 end:0x80001000\n")
    merged_splits = main_splits.replace("main.cpp:\n", branch_add + "\nmain.cpp:\n")
    main_conf = 'config.libs = [\n    Object(NonMatching, "main.cpp"),\n]\n'
    merged_conf = 'config.libs = [\n    Object(NonMatching, "branch.cpp"),\n    Object(NonMatching, "main.cpp"),\n]\n'
    check("a sound union has no problems", check_union(main_splits, merged_splits, main_conf, merged_conf),
          [])

    # ... and each of the four breakages is caught
    dup = merged_splits + "\nmain.cpp:\n\t.text       start:0x80003000 end:0x80004000\n"
    check("check_union catches a duplicate unit",
          any("duplicate unit key" in p for p in check_union(main_splits, dup, main_conf, merged_conf)), True)
    drop = main_splits.replace("main.cpp:\n", "")
    check("check_union catches a dropped main unit",
          any("missing from the merged splits.txt" in p
              for p in check_union(main_splits, drop, main_conf, merged_conf)), True)
    ov = merged_splits.replace("branch.cpp:\n\t.text       start:0x80000800 end:0x80001000",
                               "branch.cpp:\n\t.text       start:0x80000400 end:0x80001800")
    check("check_union catches an overlap",
          any("overlapping" in p for p in check_union(main_splits, ov, main_conf, merged_conf)), True)
    drop_obj = 'config.libs = [\n    Object(NonMatching, "branch.cpp"),\n]\n'
    check("check_union catches a dropped Object() line",
          any("missing from the merged configure.py" in p
              for p in check_union(main_splits, merged_splits, main_conf, drop_obj)), True)

    if fails:
        print("unionresolve: %d check(s), %d failure(s)" % (checks, len(fails)))
        for f in fails:
            print("  FAIL " + f)
        return 1
    print("unionresolve: %d check(s), 0 failure(s)" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="*", help="conflicted files to union in place")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest or not args.paths:
        return selftest()
    for path in args.paths:
        print("    %-32s union-resolved %d" % (path, union_file(path)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
