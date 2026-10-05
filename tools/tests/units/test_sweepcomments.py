"""sweepcomments on a temp git repo: each pass rewrites only comment text (code, strings and `#include` lines stay),
a second run is a no-op, the `__LINE__` lock keeps the line count (the mutation: without the macro the line drops),
protected text is never removed, an inherited header goes only when its facts survive, and the census counts what is
left."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
from collections import Counter

from tools.lib import testing
from tools.units import sweepcomments as sw

TIER = "fixture"

SPLITS = ("Sections:\n\t.text type:code\n\n"
          "mod/a.c:\n\t.text       start:0x80001000 end:0x80001100\n\n"
          "mod/b.cpp:\n\t.text       start:0x80001100 end:0x80001200\n")
SYMBOLS = "fn_80001000 = .text:0x80001000; // type:function size:0x100\n"
A_C = """/*
 * mod/a.c - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * The layout is in include/mod/a.h and include/mod/gone.h; see docs/matching.md row 42 and
 * `docs/matching.md` section 7.  Registered from
 * `proposal/80001000_fn_80001000.cpp`.
 *
 * PHASE 4 (docs/splits/phase4, window d).  Recut of fn_80000F00.cpp: its functions whose address lies in this range,
 * in address order; the rest of the range keeps its original bytes.  1 of 2 functions have a body here.
 *
 * TABLE (round 3).  Inventory: `ledger.py unit auto/80001000_fn_80001000.c`; neighbour auto/80001010_x.c.
 */
#include "mod/a.h"
static const char *s = "include/mod/a.h (round 3)";
int x; /* +0x10 */ /* include/mod/a.h */
// trailing run: include/mod/a.h
// second line (2026-09-25)
"""
A_H = "/* mod/a.h - the header. */\nint y;\n"
LOCKED = """/*
 * mod/b.cpp - locked.
 *
 * PHASE 4.
 * stays.
 */
#define CHECK(p) panic(__LINE__, (p))
void f(int *p) { CHECK(p); }
"""
INHERITED = """/* mod/c.c - the unit, holding fn_80001000. */
/* ---- header inherited from src/mod/old.c (written against its pre-phase-4 range) ---- */
/*
 * mod/old.c - fn_80001000 at 0x80001000.
 */
/* ---- header inherited from src/mod/old2.c (written against its pre-phase-4 range) ---- */
/*
 * mod/old2.c - only_here_fact_name.
 */
int z;
"""
CONFIGURE = ("cflags = [\n    # Registered from\n    # proposal/80001000_fn_80001000.cpp: the a unit, include/mod/a.h.\n"
             "    \"-O4\",  # see docs/matching.md 17\n    \"include/mod/a.h\",\n]\n")
DOC = "Read `include/mod/a.h` and docs/matching.md 29.\n"
FILES = {"configure.py": CONFIGURE, "config/RMHE08/splits.txt": SPLITS, "config/RMHE08/symbols.txt": SYMBOLS,
         "src/mod/a.c": A_C, "src/mod/a.h": A_H, "src/mod/b.cpp": LOCKED, "src/mod/c.c": INHERITED,
         "src/mod/crlf.c": "/* CRLF\r\n * include/mod/a.h\r\n */\r\nint w;\r\n",
         "src/Camellia/camellia.c": "/* include/mod/a.h */\n", "docs/x.md": DOC, "docs/tooling-requests.md": DOC,
         "docs/splits/phase4/homebutton-carried-notes.md": "live\n"}


def _repo():
    g = testing.GitFixture().init()
    g.commit(FILES, "base")
    return g


def _read(g, rel):
    return (g.root / rel).read_bytes().decode("utf-8")


def test_paths(c):
    g = _repo()
    try:
        root = str(g.root)
        dry = sw.run(root, "paths")
        c.check("a dry run writes nothing", _read(g, "src/mod/a.c"), A_C)
        res = sw.run(root, "paths", apply=True)
        a = _read(g, "src/mod/a.c")
        c.check("include/X becomes the include spelling when src/X exists", "The layout is in mod/a.h and" in a, True)
        c.check("a dangling include/ path stays and is listed",
                ("include/mod/gone.h" in a, ("src/mod/a.c", "include/mod/gone.h") in [tuple(x) for x in
                                                                                      res["dangling_include"]]),
                (True, True))
        c.check("docs/matching.md N becomes playbook N, in both spellings",
                ("see playbook 42 and\n * playbook 7." in a), True)
        c.check("the phase-4 pointer leaves the parenthetical", "STUB (phase 4; no bodies yet)" in a, True)
        c.check("'Registered from proposal/...' goes with its now-empty sentence and line",
                ("Registered" in a, "proposal/" in a), (False, False))
        c.check("a provenance name whose address starts a unit becomes that unit; one inside a unit stays",
                ("ledger.py unit mod/a.c`" in a, "auto/80001010_x.c" in a), (True, True))
        c.check("code, the string literal and the #include line are untouched",
                ('#include "mod/a.h"' in a, '"include/mod/a.h (round 3)"' in a, "int x; /* +0x10 */ /* mod/a.h */"
                 in a), (True, True, True))
        c.check("a // run is rewritten too", "// trailing run: mod/a.h" in a, True)
        c.check("CRLF endings are kept", _read(g, "src/mod/crlf.c"), "/* CRLF\r\n * mod/a.h\r\n */\r\nint w;\r\n")
        c.check("the MPL vendor directory is excluded", _read(g, "src/Camellia/camellia.c"), FILES["src/Camellia/camellia.c"])
        cfg = _read(g, "configure.py")
        c.check("configure.py: comments only, provenance across lines dropped",
                (cfg.count("include/mod/a.h"), "# Registered: the a unit, mod/a.h." in cfg, "# see playbook 17" in cfg),
                (1, True, True))
        c.check("markdown gets the tree path; a record doc is left as written",
                (_read(g, "docs/x.md"), _read(g, "docs/tooling-requests.md")),
                ("Read `src/mod/a.h` and playbook 29.\n", DOC))
        again = sw.run(root, "paths", apply=True)
        c.check("a second run is a no-op", again["files_changed"], 0)
        c.check("the dry run predicted the apply", dry["files_changed"], res["files_changed"])
    finally:
        g.cleanup()


def test_history_and_lock(c):
    g = _repo()
    try:
        root = str(g.root)
        sw.run(root, "paths", apply=True)
        res = sw.run(root, "history", apply=True)
        a = _read(g, "src/mod/a.c")
        c.check("narrative atoms leave the parenthetical", "STUB (no bodies yet)" in a, True)
        c.check("the PHASE 4 recut boilerplate goes, the fact after it stays",
                ("PHASE 4" in a, " * 1 of 2 functions have a body here." in a), (False, True))
        c.check("a label's (round N) goes; a // line's date parenthetical goes",
                ("TABLE.  Inventory" in a, "// second line\n" in a), (True, True))
        c.check("the string literal keeps its (round 3)", '"include/mod/a.h (round 3)"' in a, True)
        b = _read(g, "src/mod/b.cpp")
        c.check("a __LINE__-locked header keeps its line count (the sentence line is blanked)",
                (b.count("\n"), "PHASE 4" in b), (LOCKED.count("\n"), False))
        cc = _read(g, "src/mod/c.c")
        c.check("an inherited header whose facts survive goes; one with a unique fact stays and is listed",
                ("src/mod/old.c" in cc, "only_here_fact_name" in cc, len(res["inherited_kept"])), (False, True, 1))
        c.check("the unit's own header stays the first token", cc.startswith("/* mod/c.c - the unit"), True)
        c.check("a second history run is a no-op", sw.run(root, "history")["files_changed"], 0)
    finally:
        g.cleanup()


def test_lock_mutation(c):
    unlocked = LOCKED.replace("#define CHECK(p) panic(__LINE__, (p))", "#define CHECK(p) panic(7, (p))")
    r = sw.sweep_text(sw.Context("."), "src/mod/b.cpp", unlocked, "history", sw.line_macros([unlocked]))
    c.check("without a __LINE__ macro the emptied line is dropped", r.new.count("\n"), unlocked.count("\n") - 1)
    c.check("the lock covers the lines up to the use", sorted(sw.locked_lines(LOCKED, {"CHECK"}))[:3], [1, 2, 3])
    with_line = "int a;\n#line 40\nint b;\nvoid g(void) { use(__LINE__); }\n"
    c.check("a #line directive starts the locked span", min(sw.locked_lines(with_line, set())), 3)


def test_paren_across_lines(c):
    text = ("/*\n * x (a trial flip, 2026-10-04; the unit stays\n *   NonMatching) and (round 3, kept; pilot L2).\n"
            " */\nint a;\n")
    r = sw.sweep_text(sw.Context("."), "src/x.c", text, "history", set())
    c.check("a dropped atom leaves the kept atoms and their line break (and its comment prefix) untouched",
            r.new, "/*\n * x (a trial flip; the unit stays\n *   NonMatching) and (kept).\n */\nint a;\n")
    lead = "/* handlers (pilot L2, round 2, 0x803D72F4..0x803DF2EC). */\n"
    r = sw.sweep_text(sw.Context("."), "src/x.c", lead, "history", set())
    c.check("two leading atoms go in one deletion (no overlap, nothing left behind)",
            r.new, "/* handlers (0x803D72F4..0x803DF2EC). */\n")


def test_protected_and_roundtrip(c):
    g = sw.c_groups("/* keep size: 0x10 here */\nint a;\n")[0]
    refused, log = [], Counter()
    logical = g.logical()
    sw.apply_edits(g, [sw.Edit(0, len(logical), "", "t")], log, refused)
    c.check("an edit that removes protected text is refused", (len(refused), g.logical()), (1, logical))
    text = ("int a; /* +0x04 */ /* two */\n  // run one\r\n  // run two\n/**/\n/*\n * x\n*/\n"
            "    /* indented\n       body */\n")
    for grp in sw.c_groups(text):
        c.check("the model reproduces %r" % text[grp.start:grp.end][:12], grp.render(), text[grp.start:grp.end])
    py = "x = 1  # tail\n# a\n# b\ns = '# not a comment'\n"
    c.check("Python comments come from tokenize (a # in a string is not one)",
            [py[g.start:g.end] for g in sw.py_groups(py)], ["# tail", "# a\n# b\n"])


def test_if0_fixes_census(c):
    text = "int a;\n#if 0\nint b;\n#if X\nint c;\n#endif\n#endif\nint d;\n#if 0\nint e;\n#else\nint f;\n#endif\n"
    r = sw.if0_text("src/x.c", text, set())
    c.check("an #if 0 block (nested conditional included) is deleted; one with an #else arm is refused",
            (r.new.startswith("int a;\nint d;\n#if 0\n"), len(r.refused)), (True, 1))
    text = "int a;\n\n/* describes the block\n * below */\n#if 0\nint b;\n#endif\n\nint c; /* x */\n"
    c.check("the full-line comment describing the block goes with it, and the doubled blank line",
            sw.if0_text("src/x.c", text, set()).new, "int a;\n\nint c; /* x */\n")
    g = _repo()
    try:
        root = str(g.root)
        saved = sw.RANGE_FIXES
        sw.RANGE_FIXES = (("src/mod/a.h", "mod/a.c", "the header.", "the header for 0x80001000..0x80001100.",
                           (0x80001000, 0x80001100)),
                          ("src/mod/a.c", "mod/b.cpp", "STUB", "x", (0x1, 0x2)))
        try:
            res = sw.run(root, "fixes", apply=True)
        finally:
            sw.RANGE_FIXES = saved
        c.check("a fix that agrees with splits.txt applies; one that does not is refused",
                (_read(g, "src/mod/a.h").startswith("/* mod/a.h - the header for 0x80001000"), len(res["refusals"])),
                (True, 1))
        census = sw.census(root, "src", sw.STALE_MARKERS, whitelist_paths=True)
        c.check("the census counts stale paths in comments, never in the vendor directory or a string",
                (census["totals"].get("include/"), census["totals"].get("proposal/"),
                 "src/Camellia/camellia.c" in census["files"].get("include/", {}),
                 census["totals"].get("docs/splits/phase4")), (6, 2, False, 2))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = sw.TOOL.run(sw.main, ["--root", root, "--list-stale"], parser=sw.build_parser())
        c.check("--list-stale prints a total and exits 0", (rc, "total" in out.getvalue()), (0, True))
    finally:
        g.cleanup()


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
