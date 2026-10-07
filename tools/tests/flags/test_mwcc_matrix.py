"""mwcc_matrix: `--only-open` filters on the official number (a 100 % row is never kept, a 0.00 % row never dropped -
the `grep -v` trap), and `--one`'s ScratchObject keeps the variant's object aside and restores the unit's real one
byte- and mtime-exact."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import types

from tools.flags import mwcc_matrix as mm
from tools.lib import testing

TIER = "fixture"

ROWS = [("full_fn", 32, 32, 100.0, None, False), ("zero_fn", 40, 36, 0.0, (0, "li r3, 0"), False),
        ("partial_fn", 64, 64, 99.99, (12, "mr r30, r3"), True), ("tiny_fn", 4, 4, 100.0, None, False)]


def test_only_open(c):
    shown, hidden = mm.open_rows(ROWS, True)
    c.check("only the rows below 100 % are kept, 0.00 % included", [r[0] for r in shown], ["zero_fn", "partial_fn"])
    c.check("... and the hidden 100 % rows are counted", hidden, 2)
    c.check("without the flag every row is kept", (len(mm.open_rows(ROWS)[0]), mm.open_rows(ROWS)[1]), (4, 0))
    block = mm.render_block("default", ROWS, True)
    c.check("the table names the open rows and says how many full ones it hid",
            ([ln.split()[0] for ln in block[1:3]], block[3].strip()),
            (["zero_fn", "partial_fn"], "(2 function(s) at 100% not shown: --only-open)"))
    c.contains("a positional fallback keeps its ~ mark", block[2], "99.99%~")


def test_summarize_official(c):
    """`summarize` prints the report's entry score, never the positional `match_percent` (mutation: reading the
    entry as a number falls back to `~` for every row)."""
    import json
    side = lambda pct: {"symbols": [{"name": n, "size": 8, "match_percent": pct, "instructions": [{}]}
                                    for n in ("hit", "unscored")]}
    with testing.FixtureTree() as t:
        path = t.write("d.json", json.dumps({"left": side(11.0), "right": side(12.0)}))
        rows = {r[0]: r for r in mm.summarize(str(path), {"hit": {"fuzzy_match_percent": 87.5, "size": 8},
                                                           "unscored": {"size": 8}})}
        c.check("the entry's score is the printed one, not the positional 11.0", rows["hit"][3:6:2], (87.5, False))
        c.check("an entry with no score key is 0 %, still official", rows["unscored"][3:6:2], (0.0, False))
        rows = {r[0]: r for r in mm.summarize(str(path), {"hit": 55.0})}
        c.check("a bare number is accepted, a function the report lacks falls back with ~",
                (rows["hit"][3], rows["hit"][5], rows["unscored"][3], rows["unscored"][5]), (55.0, False, 11.0, True))


def test_func_align_override(c):
    from tools.lib import units
    flags = ["-O4,p", "-func_align", "4", "-sym", "on", "-W", "off", "-lang=c"]
    c.check("--flags-extra \"-func_align 16\" replaces the earlier value instead of appending a second",
            units.override_flags(flags, "-func_align 16"), ["-O4,p", "-sym", "on", "-W", "off", "-lang=c", "-func_align", "16"])
    c.check("-W error replaces -W off", units.override_flags(flags, "-W error")[-3:], ["-lang=c", "-W", "error"])


def test_scratch_object(c):
    with testing.FixtureTree() as t:
        real = t.write("build/RMHE08/src/g3d/x.o", b"real object")
        os.utime(real, (1_000_000_000, 1_000_000_000))
        unit = types.SimpleNamespace(root=str(t.root), key="g3d/x")
        dest = mm.scratch_path(unit, "1.3__-O4,s")
        c.check("the scratch object lives below build/tmp/matrix/one/<label>/",
                os.path.relpath(dest, str(t.root)).replace("\\", "/"), "build/tmp/matrix/one/1.3__-O4,s/g3d/x.o")
        with mm.ScratchObject(str(real)) as guard:
            real.write_bytes(b"variant object")                 # what the compile wrote over the real path
            kept = guard.keep(dest)
        c.check("the variant's object is kept in the scratch dir", open(kept, "rb").read(), b"variant object")
        c.check("the unit's real object is back, bytes and mtime", (real.read_bytes(), int(os.path.getmtime(real))),
                (b"real object", 1_000_000_000))
        c.check("no backup is left behind", os.path.exists(str(real) + ".matrix-backup"), False)
        os.unlink(real)
        with mm.ScratchObject(str(real)):
            real.write_bytes(b"variant object")
        c.check("a unit with no object before has none after", os.path.exists(real), False)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
