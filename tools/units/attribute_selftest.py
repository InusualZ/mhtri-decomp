"""Self-test for `tools/units/attribute.py` - the partitioner, the cap and the transactional apply.

Three things here can silently cost a batch, so each gets its own block of checks:

* `segments` - which pieces a region is cut into, and whether the two repair rules (too small joins its
  neighbour, too large is split and flagged) do what they claim. Pure functions over an `an`-shaped
  dict, so they need no DOL.
* `cap_batch` - the `--max-total-bytes` arithmetic (roadmap 7.14), including the boundary the plan
  cares about: a batch that lands *exactly* on the cap must pass, one byte over must not.
* `apply`'s write phase - validate first, temp file + rename, exact restore (roadmap 7.20). These run
  against a fixture layout in a temp directory, so the real `config/RMHE08/splits.txt` and
  `configure.py` are never read or written.

    python tools/units/attribute_selftest.py
    python tools/units/attribute.py --selftest          (the same checks)
"""

from __future__ import annotations

import contextlib
import io
import os
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import attribute as at  # noqa: E402

# A fixture that looks like the real files where it matters: a claimed `.text` range to collide with,
# and both anchors `configure.py` needs. Written to a temp directory, never to the repo.
FIXTURE_SPLITS = "main/foo.c:\n\t.text       start:0x80001000 end:0x80001100\n"
FIXTURE_CONF = ("config.progress_categories = [\n]\n\nconfig.libs = [\n]\n")


def fake_an(funcs, soft=(), must_link=()):
    """`an` with only the keys the partitioner reads, in the shape `tudiscover` produces."""
    return {
        "ordered": ["f%d" % i for i in range(len(funcs))],
        "addr": [a for a, _ in funcs],
        "size": [s for _, s in funcs],
        "soft": list(soft),
        "must_link": list(must_link),
    }


def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # --- names -------------------------------------------------------------------------------------
    check("placeholder: plain", at.placeholder("fn_80040598", 0x80040598, False),
          "auto/80040598_fn_80040598.c")
    check("placeholder: mangled symbol", at.placeholder("Pl_Skill_ck__FP4_PLWUs", 0x80270F50, True),
          "auto/80270F50_Pl_Skill_ck__FP4_PLWUs.cpp")
    check("placeholder: leading digit", at.placeholder("9lives", 0x80000000, False),
          "auto/80000000_u9lives.c")
    check("placeholder: illegal characters", at.placeholder("a b*c", 0x1, False), "auto/00000001_a_b_c.c")
    check("placeholder: truncated", len(at.placeholder("x" * 200, 0x1, False)),
          len("auto/00000001_") + at.NAME_MAX + len(".c"))

    check("mangled: C name", at.mangled("fn_802784A8"), False)
    check("mangled: C++ free function", at.mangled("fn__Fv"), True)
    check("mangled: C++ method", at.mangled("Pl_Skill_ck__FP4_PLWUs"), True)
    check("mangled: identifier with __ inside", at.mangled("my__thing"), False)

    # --- seams -------------------------------------------------------------------------------------
    # six 100-byte functions at 0x1000: cuts are `boundary before function c`, so 1..5 are interior
    FUNCS = [(0x1000 + 0x64 * i, 0x64) for i in range(6)]

    an = fake_an(FUNCS, soft=[(3, 3, 4.0, "pool", "shared pool")])
    check("seams: a narrow strong observation pins its cut", sorted(at.interior_seams(an, 0, 6)), [3])
    check("seams: ends are not interior",
          sorted(at.interior_seams(fake_an(FUNCS, soft=[(0, 0, 4.0, "pool", "x")]), 0, 6)), [])
    check("seams: a wide observation is not a seam",
          sorted(at.interior_seams(fake_an(FUNCS, soft=[(1, 5, 4.0, "pool", "x")]), 0, 6)), [])
    check("seams: a weak kind is not a seam",
          sorted(at.interior_seams(fake_an(FUNCS, soft=[(3, 3, 4.0, "weak-kind", "x")]), 0, 6)), [])
    check("seams: an anchor vetoes a cut inside it",
          sorted(at.interior_seams(fake_an(FUNCS, soft=[(3, 3, 4.0, "pool", "x")],
                                          must_link=[(2, 3, "anchor")]), 0, 6)), [])

    # --- segments ----------------------------------------------------------------------------------
    def seg(soft=(), must_link=(), lo=0, hi=6, min_bytes=0x200, max_bytes=0x4000):
        an = fake_an(FUNCS, soft=soft, must_link=must_link)
        return [(a, b, note) for a, b, _why, note in at.segments(an, lo, hi, min_bytes, max_bytes)]

    check("segments: no evidence is one unit", seg(), [(0, 6, None)])
    check("segments: a pinned seam splits", seg(soft=[(3, 3, 4.0, "pool", "x")], min_bytes=0),
          [(0, 3, None), (3, 6, None)])
    check("segments: too-small pieces merge (first piece has no left neighbour)",
          seg(soft=[(1, 1, 4.0, "pool", "x")]), [(0, 6, None)])
    check("segments: min-bytes 0 keeps every seam",
          seg(soft=[(1, 1, 4.0, "pool", "x")], min_bytes=0), [(0, 1, None), (1, 6, None)])
    check("segments: over the cap is split and flagged",
          seg(max_bytes=0x64), [(0, 1, "capped at --max-bytes, seam is a guess"),
                                (1, 2, "capped at --max-bytes, seam is a guess"),
                                (2, 3, "capped at --max-bytes, seam is a guess"),
                                (3, 4, "capped at --max-bytes, seam is a guess"),
                                (4, 5, "capped at --max-bytes, seam is a guess"),
                                (5, 6, None)])
    check("segments: an anchor is never cut, even by the cap",
          [c for c in [b for _a, b, _n in seg(soft=[(2, 2, 4.0, "pool", "x")],
                                             must_link=[(1, 4, "anchor")], max_bytes=0x64)]
           if 1 < c < 4], [])
    check("segments: the cap still cuts where the anchor allows it",
          [(a, b) for a, b, _n in seg(must_link=[(1, 4, "anchor")], max_bytes=0x64)],
          [(0, 1), (1, 5), (5, 6)])

    # --- the cap arithmetic (roadmap 7.14) ----------------------------------------------------------
    def cap_prop(unit, addr, size):
        return {"unit": unit, "text": [addr, addr + size], "bytes": size}

    BATCH = [cap_prop("auto/a.c", 0x1000, 100), cap_prop("auto/b.c", 0x2000, 100),
             cap_prop("auto/c.c", 0x3000, 100)]
    kept, refused, detail = at.cap_batch(BATCH, 0)
    check("cap: 0 disables the cap", (len(kept), len(refused), detail), (3, 0, None))
    kept, refused, detail = at.cap_batch(BATCH, 250)
    check("cap: the longest prefix that fits is kept", [k["unit"] for k in kept], ["auto/a.c", "auto/b.c"])
    check("cap: the tail is refused", [r["unit"] for r in refused], ["auto/c.c"])
    check("cap: the overflow is exact", detail["overflow"], 50)
    check("cap: the refusal names the candidate", detail["unit"], "auto/c.c")
    check("cap: the refusal carries the range", detail["text"], [0x3000, 0x3064])
    check("cap: the unregistered count is reported", detail["remaining"], 1)
    check("cap: the claimed total is reported", detail["claimed"], 200)
    kept, refused, detail = at.cap_batch(BATCH, 300)
    check("cap: a batch exactly at the cap passes", (len(kept), refused, detail), (3, [], None))
    kept, refused, detail = at.cap_batch(BATCH, 299)
    check("cap: one byte under the cap refuses the last candidate", (len(kept), detail["overflow"]), (2, 1))
    kept, refused, detail = at.cap_batch(BATCH, 99)
    check("cap: a cap below the first candidate keeps nothing",
          (kept, len(refused), detail["overflow"]), ([], 3, 1))
    check("cap: a single candidate over the cap is refused whole",
          at.cap_batch([cap_prop("auto/big.c", 0x1000, 5000)], 4096)[2]["overflow"], 904)

    # --- the cap on the real command line ----------------------------------------------------------
    parser = at.build_parser()
    check("cli: the cap is the plan's 0.5 MB by default", at.CAP_DEFAULT, 0x80000)
    check("cli: apply carries the cap",
          parser.parse_args(["apply", "0x1000", "0x2000"]).max_total_bytes, at.CAP_DEFAULT)
    check("cli: plan carries the cap",
          parser.parse_args(["plan", "0x1000", "0x2000"]).max_total_bytes, at.CAP_DEFAULT)
    check("cli: the cap takes a hex value",
          parser.parse_args(["apply", "0x1000", "0x2000", "--max-total-bytes", "0x100000"])
          .max_total_bytes, 0x100000)
    check("cli: 0 disables the cap",
          parser.parse_args(["apply", "0x1000", "0x2000", "--max-total-bytes", "0"]).max_total_bytes, 0)
    check("cli: --selftest needs no subcommand", parser.parse_args(["--selftest"]).selftest, True)

    # --- apply: the fixture ------------------------------------------------------------------------
    def proposal(unit, addr, size, fns):
        name = "fn_%08X" % addr
        fns[name] = {"addr": addr, "size": size, "scope": ""}
        return {"unit": unit, "text": [addr, addr + size], "bytes": size, "cxx": False, "count": 1,
                "functions": [{"name": name, "address": addr, "size": size}],
                "seam": None, "seam_note": None, "runs": {}}

    def fixture(root, splits=FIXTURE_SPLITS, conf=FIXTURE_CONF):
        root = Path(root)
        (root / "config").mkdir(parents=True, exist_ok=True)
        (root / "config" / "splits.txt").write_text(splits, encoding="utf-8", newline="")
        (root / "configure.py").write_text(conf, encoding="utf-8", newline="")
        return at.Layout(root / "config" / "splits.txt", root / "configure.py", root / "src")

    def read(path):
        return open(path, encoding="utf-8", newline="").read()

    def temps(root):
        return sorted(str(p) for p in Path(root).rglob("*" + at.TMP_SUFFIX))

    def run(props, layout, **kw):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = at.apply(props, layout=layout, **kw)
        return code, buf.getvalue()

    # --- apply: the cap refuses a candidate before anything is written ------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/a.c", 0x80002000, 0x100, fns),
                 proposal("auto/b.c", 0x80002100, 0x100, fns),
                 proposal("auto/c.c", 0x80002200, 0x100, fns)]
        code, out = run(props, layout, cap=0x280)
        check("cap: apply refuses the candidate past the cap", "refused: auto/c.c" in out, True)
        check("cap: apply says how far over it is", "128 B over the cap" in out, True)
        check("cap: apply says how many bytes it claims", "claims 512 B of .text in 2 unit(s)" in out, True)
        splits = read(layout.splits)
        check("cap: the refused unit is not in splits.txt", "auto/c.c" in splits, False)
        check("cap: the refused range is not claimed", "start:0x80002200" in splits, False)
        check("cap: no stub for the refused unit", (layout.src / "auto/c.c").exists(), False)
        check("cap: the kept units are registered",
              ("auto/a.c:" in splits and "auto/b.c:" in splits), True)
        check("cap: the kept stubs exist", (layout.src / "auto/a.c").exists(), True)
        check("cap: no temp files left", temps(tmp), [])

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/d.c", 0x80002000, 0x100, fns)]
        code, out = run(props, layout, cap=1)
        check("cap: nothing fits -> exit 1", code, 1)
        check("cap: nothing fits -> the message says so", "nothing written" in out, True)
        check("cap: nothing fits -> splits.txt untouched", read(layout.splits), FIXTURE_SPLITS)
        check("cap: nothing fits -> configure.py untouched", read(layout.configure), FIXTURE_CONF)
        check("cap: nothing fits -> no stub", (layout.src).exists(), False)
        check("cap: nothing fits -> no temp files", temps(tmp), [])

    # --- apply: validation refuses before the write phase -------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        clash = [proposal("auto/x.c", 0x80001050, 0x100, fns)]        # inside main/foo.c's range
        code, out = run(clash, layout, cap=0)
        check("validate: an overlap with a claimed range is refused", "overlaps main/foo.c" in out, True)
        check("validate: exit 1", code, 1)
        check("validate: the refusal says nothing was touched", "nothing was touched" in out, True)
        check("validate: splits.txt unchanged", read(layout.splits), FIXTURE_SPLITS)
        check("validate: configure.py unchanged", read(layout.configure), FIXTURE_CONF)
        check("validate: no stub written", (layout.src).exists(), False)

        ghost = proposal("auto/y.c", 0x80003000, 0x100, fns)
        del fns[ghost["functions"][0]["name"]]                        # a function the map never had
        code, out = run([ghost], layout, cap=0, fns=fns)
        check("validate: an unresolvable function is refused",
              "not a .text function in the symbol map" in out, True)
        check("validate: a stale address is refused",
              bool(at.validate([dict(ghost, functions=[{"name": "fn_80003000", "address": 0x80003100,
                                                        "size": 0x100}])], layout, fns)), True)

        drifted = dict(proposal("auto/z.c", 0x80004000, 0x100, fns), text=[0x80004000, 0x80004500])
        check("validate: a range that is not the span of its functions is refused",
              bool(at.validate([drifted], layout, fns)), True)
        check("validate: a unit already registered elsewhere is refused",
              bool(at.validate([proposal("main/foo.c", 0x80002000, 0x100, fns)], layout, fns)), True)
        check("validate: an unknown section is refused",
              bool(at.validate([dict(proposal("auto/w.c", 0x80005000, 0x100, fns),
                                     runs={".nope": {}})], layout, fns)), True)
        check("validate: a unit path that escapes src/ is refused",
              any("escapes src/" in e for e in
                  at.validate([proposal("../../evil.c", 0x80005000, 0x100, fns)], layout, fns)), True)
        check("validate: an absolute unit path is refused",
              any("escapes src/" in e for e in
                  at.validate([proposal(str(Path(tmp) / "evil.c"), 0x80005000, 0x100, fns)],
                              layout, fns)), True)
        check("validate: an unreadable shared file is reported, not a traceback",
              bool(at.validate([proposal("auto/q.c", 0x80005000, 0x100, fns)],
                               at.Layout(Path(tmp) / "gone.txt", layout.configure, layout.src), fns)),
              True)

        # the anchors: a configure.py without `config.libs = [` cannot be registered into
        layout_bad = fixture(Path(tmp) / "noanchor", conf="config.progress_categories = [\n]\n")
        check("validate: a missing configure.py anchor is refused",
              any("anchor is missing" in e for e in
                  at.validate([proposal("auto/v.c", 0x80002000, 0x100, fns)], layout_bad, fns)), True)

    # --- apply: a dry run touches nothing -----------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/a.c", 0x80002000, 0x100, fns),
                 proposal("auto/b.c", 0x80002100, 0x100, fns)]
        code, out = run(props, layout, dry_run=True, cap=0)
        check("dry run: exit 0", code, 0)
        check("dry run: it names the writes", "would write 4 file(s)" in out, True)
        check("dry run: it says nothing was written", "dry run: nothing written" in out, True)
        check("dry run: splits.txt unchanged", read(layout.splits), FIXTURE_SPLITS)
        check("dry run: configure.py unchanged", read(layout.configure), FIXTURE_CONF)
        check("dry run: no stub written", (layout.src).exists(), False)
        check("dry run: no temp files", temps(tmp), [])

    # --- apply: the happy path, then idempotence and CRLF -------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/a.c", 0x80002000, 0x100, fns),
                 proposal("auto/b.c", 0x80002100, 0x100, fns)]
        code, out = run(props, layout, cap=0)
        check("apply: exit 0", code, 0)
        check("apply: it reports what it wrote",
              "wrote 2 split block(s), 2 configure object(s), 2 stub source(s)" in out, True)
        splits, conf = read(layout.splits), read(layout.configure)
        check("apply: both blocks landed", ("auto/a.c:" in splits and "auto/b.c:" in splits), True)
        check("apply: the .text lines carry the ranges",
              ("start:0x80002000 end:0x80002100" in splits
               and "start:0x80002100 end:0x80002200" in splits), True)
        check("apply: the pre-existing block survives", "main/foo.c:" in splits, True)
        check("apply: configure.py gained the objects",
              ('"auto/a.c"' in conf and '"auto/b.c"' in conf), True)
        check("apply: the auto progress category was declared", 'ProgressCategory("auto"' in conf, True)
        check("apply: the stubs exist",
              ((layout.src / "auto/a.c").exists() and (layout.src / "auto/b.c").exists()), True)
        check("apply: the stub names its range",
              "0x80002000..0x80002100" in read(layout.src / "auto/a.c"), True)
        check("apply: no temp files left", temps(tmp), [])
        code, out = run(props, layout, cap=0)
        check("apply: re-applying the same batch is a no-op", read(layout.splits).count("auto/a.c:"), 1)
        check("apply: a no-op re-apply writes no stub twice",
              read(layout.configure).count('"auto/a.c"'), 1)
        check("apply: a no-op re-apply says what it skipped",
              "skipped 2 split block(s) already in splits.txt" in out, True)

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp, splits=FIXTURE_SPLITS.replace("\n", "\r\n")), {}
        run([proposal("auto/e.c", 0x80004000, 0x100, fns)], layout, cap=0)
        text = read(layout.splits)
        check("apply: a CRLF splits.txt stays CRLF", "\n" not in text.replace("\r\n", ""), True)
        check("apply: the new block uses CRLF", "\r\n" in text and "auto/e.c:\r\n" in text, True)

    # --- apply: rollback on a mid-write failure -----------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/a.c", 0x80002000, 0x100, fns),
                 proposal("auto/b.c", 0x80002100, 0x100, fns)]
        order: list[str] = []

        def flaky(src, dst):
            order.append(Path(dst).name)
            if Path(dst).name == "b.c":              # the 4th write: splits, configure, a.c, b.c
                raise OSError("injected failure")
            os.replace(src, dst)

        code, out = run(props, layout, cap=0, rename=flaky)
        check("rollback: the failure is reported", "rolled back" in out, True)
        check("rollback: exit 1", code, 1)
        check("rollback: it says nothing was registered", "nothing was registered" in out, True)
        check("rollback: the write order is splits, configure, stubs",
              order, ["splits.txt", "configure.py", "a.c", "b.c"])
        check("rollback: splits.txt restored byte for byte", read(layout.splits), FIXTURE_SPLITS)
        check("rollback: configure.py restored byte for byte", read(layout.configure), FIXTURE_CONF)
        check("rollback: the earlier stub was deleted", (layout.src / "auto/a.c").exists(), False)
        check("rollback: the failed stub never landed", (layout.src / "auto/b.c").exists(), False)
        check("rollback: the created directory was removed", (layout.src).exists(), False)
        check("rollback: no temp files left", temps(tmp), [])

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/a.c", 0x80002000, 0x100, fns)]

        def first(src, dst):
            raise OSError("injected failure on the very first rename")

        code, out = run(props, layout, cap=0, rename=first)
        check("rollback: a failure on the first write leaves both files alone",
              (read(layout.splits), read(layout.configure)), (FIXTURE_SPLITS, FIXTURE_CONF))
        check("rollback: a failure on the first write cleans its temp file", temps(tmp), [])

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
