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
import json
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

# A `configure.py` that already declares the `auto` lib: the second batch must extend this block's
# object list, not append a second `"lib": "auto"` block (a duplicate lib silently changes the flags
# every earlier unit was built with).
FIXTURE_CONF_AUTO = (
    "config.progress_categories = [\n"
    '    ProgressCategory("auto", "Auto (bulk attribution)"),\n'
    "]\n\n"
    "config.libs = [\n"
    "    {\n"
    '        "lib": "auto",\n'
    '        "mw_version": "Wii/1.3",\n'
    '        "cflags": cflags_main,\n'
    '        "progress_category": "auto",\n'
    '        "objects": [\n'
    '            Object(NonMatching, "auto/old.c"),\n'
    "        ],\n"
    "    },\n"
    "]\n"
)

# The same, with another lib whose object list comes first: the anchor must pick the auto block, not
# the first `"objects": [` in the file.
FIXTURE_CONF_MULTI = (
    "config.progress_categories = [\n"
    '    ProgressCategory("auto", "Auto (bulk attribution)"),\n'
    "]\n\n"
    "config.libs = [\n"
    "    {\n"
    '        "lib": "main",\n'
    '        "mw_version": "Wii/1.3",\n'
    '        "cflags": cflags_main,\n'
    '        "objects": [\n'
    '            Object(NonMatching, "main.cpp"),\n'
    "        ],\n"
    "    },\n"
    "    {\n"
    '        "lib": "auto",\n'
    '        "mw_version": "Wii/1.3",\n'
    '        "cflags": cflags_main,\n'
    '        "progress_category": "auto",\n'
    '        "objects": [\n'
    '            Object(NonMatching, "auto/old.c"),\n'
    "        ],\n"
    "    },\n"
    "]\n"
)

# An auto lib whose object list is written inline: there is no line to insert under, so `apply` must
# refuse instead of producing invalid Python or a duplicate block.
FIXTURE_CONF_INLINE = FIXTURE_CONF_AUTO.replace(
    '        "objects": [\n            Object(NonMatching, "auto/old.c"),\n        ],',
    '        "objects": [],')


def fake_an(funcs, soft=(), must_link=(), source_names=()):
    """`an` with only the keys the partitioner reads, in the shape `tudiscover` produces."""
    return {
        "ordered": ["f%d" % i for i in range(len(funcs))],
        "addr": [a for a, _ in funcs],
        "size": [s for _, s in funcs],
        "soft": list(soft),
        "must_link": list(must_link),
        "source_names": list(source_names),
    }


def fake_source(name, lo, hi, funcs, reject=None):
    """An accepted `__FILE__` name spanning functions `lo..hi` of `funcs`, in tudiscover's shape."""
    return {"src": name, "lo": lo, "hi": hi, "reject": reject,
            "start": funcs[lo][0], "end": funcs[hi][0] + funcs[hi][1]}


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
          "proposal/80040598_fn_80040598.c")
    check("placeholder: mangled symbol", at.placeholder("Pl_Skill_ck__FP4_PLWUs", 0x80270F50, True),
          "proposal/80270F50_Pl_Skill_ck__FP4_PLWUs.cpp")
    check("placeholder: leading digit", at.placeholder("9lives", 0x80000000, False),
          "proposal/80000000_u9lives.c")
    check("placeholder: illegal characters", at.placeholder("a b*c", 0x1, False), "proposal/00000001_a_b_c.c")
    check("placeholder: truncated", len(at.placeholder("x" * 200, 0x1, False)),
          len("proposal/00000001_") + at.NAME_MAX + len(".c"))

    check("mangled: C name", at.mangled("fn_802784A8"), False)
    check("mangled: C++ free function", at.mangled("fn__Fv"), True)
    check("mangled: C++ method", at.mangled("Pl_Skill_ck__FP4_PLWUs"), True)
    check("mangled: identifier with __ inside", at.mangled("my__thing"), False)

    # --- language (the extension picks the front-end: dtk turns it into -lang=c / -lang=c++) --------
    # attribute.py reads the verdict from `tools/units/langcheck.py`; these pin the *region* evidence it
    # feeds it, because a new stub has no target object to read.
    check("region language: a mangled name in the region",
          at.region_language({}, {}, ["Pl_Skill_ck__FP4_PLWUs"], 0x1000, 0x1100)["lang"], "c++")
    graph_callee = {"funcs": {"f1": {"calls": ["Panic__Q24nw4r2dbFPCciPCce", "fn_80041E8C"]}}}
    v = at.region_language({}, graph_callee, ["f1"], 0x1000, 0x1100)
    check("region language: a mangled callee is C++/medium", (v["lang"], v["confidence"]), ("c++", "medium"))
    check("region language: a mangled callee of a foreign function is not evidence",
          at.region_language({}, {"funcs": {"other": {"calls": ["Panic__Q24nw4r2dbFPCciPCce"]}}},
                             ["f1"], 0x1000, 0x1100)["lang"], "c")
    an_cpp = {"source_names": [{"src": "ef_line.cpp", "start": 0x1000, "end": 0x1100}]}
    check("region language: a .cpp __FILE__ inside the region",
          at.region_language(an_cpp, {}, ["f1"], 0x1000, 0x1100)["lang"], "c++")
    check("region language: the same .cpp __FILE__ outside is not evidence",
          at.region_language(an_cpp, {}, ["f1"], 0x2000, 0x2100)["lang"], "c")
    check("region language: a .c __FILE__ inside is C/high",
          at.region_language({"source_names": [{"src": "TPL.c", "start": 0x1000, "end": 0x1100}]},
                             {}, ["f1"], 0x1000, 0x1100)["confidence"], "high")
    check("region language: no evidence is C and low",
          (at.region_language({}, {}, ["fn_80040598"], 0x1000, 0x1100)["lang"],
           at.region_language({}, {}, ["fn_80040598"], 0x1000, 0x1100)["confidence"]), ("c", "low"))
    check("region language: a missing source_names key is not a crash",
          at.region_language({}, None, ["fn_80040598"], 0x1000, 0x1100)["lang"], "c")

    langcxx = at.region_language({}, {}, ["Pl_Skill_ck__FP4_PLWUs"], 0x1000, 0x1100)
    stub = {"unit": "auto/800CCFB0_fn_800CCFB0.cpp", "text": [0x800CCFB0, 0x800CD584], "count": 1,
            "bytes": 0x5D4, "seam": None, "seam_note": None, "cxx": True, "runs": {},
            "language": at.lc.classify([], [], ["ef_line.cpp"])}
    check("the placeholder derives C++ from the region verdict",
          at.placeholder("Pl_Skill_ck__FP4_PLWUs", 0x1000, langcxx["lang"] == "c++"),
          "proposal/00001000_Pl_Skill_ck__FP4_PLWUs.cpp")
    check("the stub header states the language and its evidence",
          "Language: C++ (high: `__FILE__` string `ef_line.cpp`)" in at.stub_text(stub), True)
    check("the stub header states an unevidenced C default as such",
          "Language: C (low: no evidence)" in at.stub_text(
              {"unit": "auto/x.c", "text": [0, 4], "count": 1, "seam": None, "seam_note": None,
               "cxx": False, "language": at.region_language({}, {}, ["fn_1"], 0, 4)}), True)
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        at.human([stub])
    check("the plan's human line carries the language verdict",
          "C++ (high: `__FILE__` string `ef_line.cpp`)" in buf.getvalue(), True)

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
    def seg(soft=(), must_link=(), source_names=(), lo=0, hi=6, min_bytes=0x200, max_bytes=0x4000):
        an = fake_an(FUNCS, soft=soft, must_link=must_link, source_names=source_names)
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

    # --- source-file ownership: one file is one TU, never half of one, never a union --------------
    # six 100-byte functions; a name's first referrer is its TU's earliest cut and the next name's is
    # the latest, so the two names' spans stay whole. These are the two live failure modes.
    A = fake_source("a.cpp", 1, 2, FUNCS)
    B = fake_source("b.cpp", 4, 4, FUNCS)
    LONG = fake_source("a.cpp", 1, 4, FUNCS)
    check("sources: a source start is a seam whatever the soft vote's width",
          sorted(at.interior_seams(fake_an(FUNCS, source_names=[B]), 0, 6)), [4])
    check("sources: a rejected name is not a boundary",
          sorted(at.interior_seams(fake_an(FUNCS, source_names=[fake_source("c.cpp", 4, 4, FUNCS,
                                                                            reject="shared")]),
                                    0, 6)), [])
    check("sources: the owner is the latest name at or before the index",
          (at.source_owner(fake_an(FUNCS, source_names=[A, B]), 3),
           at.source_owner(fake_an(FUNCS, source_names=[A, B]), 4),
           at.source_owner(fake_an(FUNCS, source_names=[A, B]), 0)), ("a.cpp", "b.cpp", None))
    # a union of two files is split at the second file's start (the 80063888 case)
    check("segments: a union of two source files is split at the later start",
          seg(source_names=[A, B], min_bytes=0), [(0, 1, None), (1, 4, None), (4, 6, None)])
    # a candidate pool seam inside one file's span does not cut the file in two (the g3d_calcvtx case)
    check("segments: a seam inside one file joins back into one file",
          seg(source_names=[A], soft=[(3, 3, 4.0, "pool", "x")], min_bytes=0, lo=1, hi=6),
          [(1, 6, "one source file (a.cpp): a candidate seam inside it was not taken")])
    # the join is recorded in the note, and the swallowed seam is re-read by tu_probe (below)
    check("segments: the join says which file it joined",
          "g3d" not in seg(source_names=[A], soft=[(3, 3, 4.0, "pool", "x")], min_bytes=0, lo=1,
                           hi=6)[0][2], True)
    # a clean one-file range is untouched
    check("segments: one file with no seam inside is untouched",
          seg(source_names=[A], min_bytes=0, lo=1, hi=4), [(1, 4, None)])
    check("segments: a join that would exceed the cap is left split",
          seg(source_names=[A], soft=[(3, 3, 4.0, "pool", "x")], min_bytes=0, lo=1, hi=6,
              max_bytes=0x12C),
          [(1, 3, None), (3, 6, None)])

    # --- a byte budget is not a TU boundary (2026-09-25) -------------------------------------------
    # The two size repairs used to cut wherever the byte count said, so a proposal could hold half of
    # one file and the cap could put a boundary inside a name. The evidence outranks the byte count now.
    check("segments: the cap ends a piece at the file's own edge, not at a byte",
          seg(source_names=[fake_source("a.cpp", 1, 3, FUNCS)], min_bytes=0, max_bytes=0x190),
          [(0, 1, None), (1, 4, "ends at the edge of source file a.cpp, not at the byte cap"),
           (4, 6, None)])
    check("segments: a cap smaller than the file slides out of it",
          seg(source_names=[LONG], min_bytes=0, max_bytes=0x12C),
          [(0, 1, None),
           (1, 5, "over --max-bytes: the cut is the edge of source file a.cpp, not a byte position"),
           (5, 6, None)])
    check("segments: ... and then no cut lands inside the file at all",
          [c for c in {b for _a, b, _n in seg(source_names=[LONG], min_bytes=0, max_bytes=0x12C)}
           if 1 < c <= 4], [])
    check("segments: the piece that ends at the file edge is not over the cap",
          seg(source_names=[fake_source("a.cpp", 1, 3, FUNCS)], min_bytes=0, max_bytes=0x190)[1][1], 4)
    check("segments: a small piece that starts a source file is never merged away (B is 200 B)",
          seg(source_names=[B], min_bytes=0x200, lo=0, hi=6), [(0, 4, None), (4, 6, None)])
    check("segments: ... the merge still runs where no name starts",
          seg(source_names=[B], soft=[(2, 2, 4.0, "pool", "x")], min_bytes=0x200, lo=0, hi=6),
          [(0, 4, None), (4, 6, None)])

    # --- the TU probe: what the queue entry records so the brief can warn --------------------------
    def probe(source_names, lo=0, hi=6, soft=()):
        an = fake_an(FUNCS, source_names=source_names, soft=soft)
        return at.tu_probe(an, lo, hi)["verdict"]

    check("probe: one whole name is one TU", probe([A], 0, 6), "one-tu")
    check("probe: two whole names are a union", probe([A, B], 0, 6), "multi-tu")
    check("probe: no name is unproven", probe([], 0, 6), "unproven")
    check("probe: a name the range cuts is partial", probe([LONG], 0, 3), "partial")
    check("probe: a candidate seam left inside one file is merged",
          probe([A], 1, 6, soft=[(3, 3, 4.0, "pool", "x")]), "merged")
    check("probe: the sources are listed for the brief",
          at.tu_probe(fake_an(FUNCS, source_names=[A, B]), 0, 6)["sources"], ["a.cpp", "b.cpp"])
    check("probe: the partial file is named",
          at.tu_probe(fake_an(FUNCS, source_names=[LONG]), 0, 3)["partial_source"], "a.cpp")
    # a size-only edge is flagged as the size decision it is: no name bounds the range, so "unproven"
    # would read as if the range were merely unexamined (2026-09-25)
    check("probe: a piece the cap decided is capped, not unproven",
          at.tu_probe(fake_an(FUNCS), 0, 6, "capped at --max-bytes, seam is a guess")["verdict"], "capped")
    check("probe: the same range without a cap note is unproven", probe([], 0, 6), "unproven")
    check("probe: a named file outranks the cap note",
          at.tu_probe(fake_an(FUNCS, source_names=[A]), 0, 6,
                      "capped at --max-bytes, seam is a guess")["verdict"], "one-tu")

    # --- the invariant: two proposals never overlap, and none overlaps a registered range -----------
    def prop(unit, a, b):
        return {"unit": unit, "text": [a, b]}

    check("overlap: adjacent half-open ranges do not overlap",
          at.overlap_report([prop("a", 0x1000, 0x1100), prop("b", 0x1100, 0x1200)]), [])
    check("overlap: one shared byte is an overlap",
          len(at.overlap_report([prop("a", 0x1000, 0x1100), prop("b", 0x10FF, 0x1200)])), 1)
    check("overlap: the report names both ranges",
          "0x00001000..0x00001100" in at.overlap_report([prop("a", 0x1000, 0x1100),
                                                         prop("b", 0x10FF, 0x1200)])[0], True)
    check("overlap: a proposal over a registered unit is reported",
          len(at.overlap_report([prop("a", 0x1000, 0x1100)], [(0x0F00, 0x1080, "main/foo.c")])), 1)
    check("overlap: ... and the registered unit is named",
          "main/foo.c" in at.overlap_report([prop("a", 0x1000, 0x1100)],
                                            [(0x0F00, 0x1080, "main/foo.c")])[0], True)
    check("overlap: a claim that only touches the edge is not an overlap",
          at.overlap_report([prop("a", 0x1100, 0x1200)], [(0x1000, 0x1100, "main/foo.c")]), [])
    check("overlap: a clean batch reports nothing",
          at.overlap_report([prop("a", 0x1000, 0x1100), prop("b", 0x2000, 0x2100)],
                            [(0x3000, 0x3100, "main/foo.c")]), [])
    kept, dropped = at.drop_overlaps([prop("b", 0x10FF, 0x1200), prop("a", 0x1000, 0x1100)])
    check("overlap: the lowest address wins the overlap", [p["unit"] for p in kept], ["a"])
    check("overlap: the loser is not emitted", len(kept), 1)
    check("overlap: the drop is reported, never silent", len(dropped), 1)
    check("overlap: a disjoint batch survives whole",
          len(at.drop_overlaps([prop("a", 0x1000, 0x1100), prop("b", 0x1100, 0x1200)])[0]), 2)
    check("overlap: one that sits on a registered range is dropped",
          len(at.drop_overlaps([prop("a", 0x1000, 0x1100)], [(0x0F00, 0x1080, "main/foo.c")])[0]), 0)
    check("intervals: touching and nested ranges merge into one",
          at.merge_intervals([(0x10, 0x20), (0x20, 0x30), (0x14, 0x18)]), [(0x10, 0x30)])
    check("intervals: a gap stays a gap",
          at.merge_intervals([(0x10, 0x20), (0x21, 0x30)]), [(0x10, 0x20), (0x21, 0x30)])
    check("intervals: coverage needs the whole range",
          (at.interval_covered([(0x10, 0x30)], 0x10, 0x30),
           at.interval_covered([(0x10, 0x30)], 0x20, 0x31)), (True, False))

    # --- propose: the region is half-open on function boundaries, so adjacent regions never overlap -
    def props_for(start, end, claimed=()):
        return at.propose(fake_an(FUNCS), {}, {}, {}, start, end, min_bytes=0, max_bytes=0x4000,
                          claimed=list(claimed))

    whole_end = FUNCS[-1][0] + FUNCS[-1][1]
    check("propose: the whole region is one run", [p["count"] for p in props_for(0x1000, whole_end)], [6])
    check("propose: its range is the span of its functions",
          props_for(0x1000, whole_end)[0]["text"], [0x1000, whole_end])
    cut = FUNCS[3][0]
    left, right = props_for(0x1000, cut), props_for(cut, whole_end)
    check("propose: two adjacent regions tile the functions",
          [p["text"] for p in left] + [p["text"] for p in right], [[0x1000, cut], [cut, whole_end]])
    check("propose: two adjacent regions never overlap", at.overlap_report(left + right), [])
    # a function that straddles `end` belongs to the next region: the region that cuts it drops it (and
    # `main` says so), rather than emitting a range past the region it was asked for
    check("propose: a function ending past `end` is not part of the region",
          [p["count"] for p in props_for(0x1000, FUNCS[2][0] + 0x20)], [2])
    check("propose: ... and the next region does not start inside it either",
          props_for(FUNCS[2][0] + 0x20, whole_end)[0]["text"][0], FUNCS[3][0])
    check("propose: a region over a registered range excludes those functions",
          [p["count"] for p in props_for(0x1000, whole_end, [(FUNCS[1][0], FUNCS[2][0], "main/foo.c")])],
          [1, 4])
    check("propose: ... and nothing it emits overlaps the registered range",
          at.overlap_report(props_for(0x1000, whole_end, [(FUNCS[1][0], FUNCS[2][0], "main/foo.c")]),
                            [(FUNCS[1][0], FUNCS[2][0], "main/foo.c")]), [])
    # a function whose *bytes* run into a claim is a boundary defect, and it must not be proposed: the
    # old start-only test proposed it and the proposal then overlapped a live unit's range
    check("propose: a function whose bytes run into a claim is excluded",
          [p["count"] for p in props_for(0x1000, whole_end,
                                         [(FUNCS[1][0] + 0x10, FUNCS[1][0] + 0x20, "main/foo.c")])],
          [1, 4])

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

    # --- apply: extending an existing lib (a second batch must not append a second block) ------------
    check("extend: lib_objects_anchor finds the auto block",
          at.lib_objects_anchor(FIXTURE_CONF_AUTO, "auto") is not None, True)
    check("extend: lib_objects_anchor is None when the lib is absent",
          at.lib_objects_anchor(FIXTURE_CONF, "auto"), None)
    check("extend: lib_objects_anchor refuses an inline objects list",
          at.lib_objects_anchor(FIXTURE_CONF_INLINE, "auto"), None)
    check("extend: lib_present finds the block", at.lib_present(FIXTURE_CONF_AUTO, "auto"), True)
    check("extend: lib_present is False when the lib is absent", at.lib_present(FIXTURE_CONF, "auto"),
          False)
    once = at.configure_insertion(FIXTURE_CONF_AUTO, ["auto/new.c"])
    check("extend: configure_insertion extends, not appends", once.count('"lib": "auto"'), 1)
    check("extend: the old object survives", '"auto/old.c"' in once, True)
    check("extend: the new object lands after the old one",
          once.index('"auto/old.c"') < once.index('"auto/new.c"'), True)
    check("extend: configure_insertion is idempotent",
          at.configure_insertion(once, ["auto/new.c"]), once)
    check("extend: the progress category is not duplicated", once.count('ProgressCategory("auto"'), 1)

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp, conf=FIXTURE_CONF_AUTO), {}
        props = [proposal("auto/new.c", 0x80002000, 0x100, fns)]
        code, out = run(props, layout, cap=0)
        conf = read(layout.configure)
        check("extend: exit 0", code, 0)
        check("extend: the object line landed", '"auto/new.c"' in conf, True)
        check("extend: exactly one auto lib remains", conf.count('"lib": "auto"'), 1)
        check("extend: the new object is inside the auto objects list",
              conf.index('"auto/new.c"') < conf.index("],", conf.index('"objects": [')), True)
        check("extend: the stub exists", (layout.src / "auto/new.c").exists(), True)
        check("extend: no temp files", temps(tmp), [])
        code, out = run(props, layout, cap=0)
        conf = read(layout.configure)
        check("extend: re-applying adds no object line", conf.count('"auto/new.c"'), 1)
        check("extend: re-applying keeps one auto lib", conf.count('"lib": "auto"'), 1)
        check("extend: re-applying says what it skipped",
              "skipped 1 split block(s) already in splits.txt" in out, True)

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp, conf=FIXTURE_CONF_MULTI), {}
        run([proposal("auto/multi.c", 0x80002000, 0x100, fns)], layout, cap=0)
        conf = read(layout.configure)
        check("extend: the anchor picks the auto lib, not the first", conf.count('"lib": "auto"'), 1)
        check("extend: the other lib is untouched", conf.count('"main.cpp"'), 1)
        check("extend: the new object is in the auto list",
              conf.index('"auto/old.c"') < conf.index('"auto/multi.c"'), True)
        check("extend: the new object is not in the main list",
              conf.index('"auto/multi.c"') < conf.index("],", conf.index('"lib": "main"')), False)

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp, conf=FIXTURE_CONF_AUTO.replace("\n", "\r\n")), {}
        run([proposal("auto/crlf.c", 0x80002000, 0x100, fns)], layout, cap=0)
        conf = read(layout.configure)
        check("extend: a CRLF configure.py stays CRLF", "\n" not in conf.replace("\r\n", ""), True)
        check("extend: the new object landed with CRLF",
              '\r\n' in conf and '"auto/crlf.c"' in conf, True)

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp, conf=FIXTURE_CONF_INLINE), {}
        code, out = run([proposal("auto/inline.c", 0x80002000, 0x100, fns)], layout, cap=0)
        check("extend: an inline objects list is refused", "no multi-line" in out, True)
        check("extend: the refusal touches nothing", read(layout.configure), FIXTURE_CONF_INLINE)
        check("extend: the refusal writes no stub", (layout.src / "auto/inline.c").exists(), False)
        check("extend: the refusal leaves no temp files", temps(tmp), [])

    # --- apply: a new lib is appended only when none exists -----------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("auto/fresh.c", 0x80002000, 0x100, fns)]
        code, out = run(props, layout, cap=0)
        conf = read(layout.configure)
        check("new lib: exit 0", code, 0)
        check("new lib: exactly one auto block is appended", conf.count('"lib": "auto"'), 1)
        check("new lib: the object line landed", '"auto/fresh.c"' in conf, True)
        check("new lib: the block has its own objects list", '"objects": [' in conf, True)
        check("new lib: the progress category was declared", 'ProgressCategory("auto"' in conf, True)
        check("new lib: the stub exists", (layout.src / "auto/fresh.c").exists(), True)
        check("new lib: no temp files", temps(tmp), [])

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

    # --- option A: the proposal queue, not registrations (owner, 2026-09-24) ------------------------
    # A proposal is work to hand out, never a registered unit: `queue` writes the queue and touches none of
    # the four shared files, and the retired registration path (`apply`) refuses unless asked explicitly.
    def capture(fn):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = fn()
        return code, buf.getvalue()

    with tempfile.TemporaryDirectory() as tmp:
        layout, fns = fixture(tmp), {}
        props = [proposal("proposal/a.c", 0x80002000, 0x100, fns),
                 proposal("proposal/b.c", 0x80003000, 0x80, fns)]
        doc = at.queue_doc(props, at.CAP_DEFAULT)
        check("queue: the document is versioned", doc["version"], 1)
        check("queue: carries no timestamp (so the same inputs give the same bytes)",
              any(k in doc for k in ("generated", "timestamp", "generated_at")), False)
        check("queue: carries the input fingerprints",
              all(k in doc for k in ("dol_sha1", "symbols_sha1", "splits_sha1", "configure_sha1")), True)
        check("queue: one entry per proposal", len(doc["units"]), 2)
        check("queue: the total is the sum of the proposals", doc["total_bytes"], 0x180)
        check("queue: an entry carries everything a brief needs",
              sorted(doc["units"][0]), ["bytes", "count", "cxx", "functions", "label", "language",
                                        "runs", "seam", "seam_note", "text", "tu"])
        check("queue: a label is not a src/ path", doc["units"][0]["label"].startswith("proposal/"), True)
        check("queue: a label never claims to be a registered unit",
              any(doc["units"][0]["label"].startswith(p) for p in ("src/", "auto/", "main/")), False)

        qpath = Path(tmp) / "attribution-queue.json"
        before = (read(layout.splits), read(layout.configure))
        code, out = capture(lambda: at.write_queue(props, at.CAP_DEFAULT, path=qpath))
        check("queue: exits 0", code, 0)
        check("queue: writes a parseable document", json.loads(read(qpath))["units"][0]["label"],
              "proposal/a.c")
        check("queue: leaves splits.txt and configure.py byte-identical",
              (read(layout.splits), read(layout.configure)), before)
        check("queue: leaves no temp file behind", temps(tmp), [])
        check("queue: writes no source stub", (layout.src).exists(), False)
        code, out = capture(lambda: at.write_queue(props, at.CAP_DEFAULT, path=qpath, dry_run=True))
        check("queue: a dry run leaves the queue as it was", json.loads(read(qpath))["units"][0]["label"],
              "proposal/a.c")

    # --- the destructive default is opt-in: a rewrite that would drop proposals is refused ----------
    # `queue` writes the whole file for the region it is given, so a sub-region run used to discard every
    # other proposal (`queue 0x8008F8E4 0x80097D40` took a 419-entry queue down to 1). The refusal is a
    # coverage test, because a legitimate re-run does re-cut the same region.
    with tempfile.TemporaryDirectory() as tmp:
        qpath = Path(tmp) / "attribution-queue.json"
        old = [{"label": "proposal/1000_a.c", "text": [0x1000, 0x1100],
                "functions": [{"name": "fa", "address": 0x1000, "size": 0x80},
                              {"name": "fb", "address": 0x1080, "size": 0x70}]},
               {"label": "proposal/2000_b.c", "text": [0x2000, 0x2080],
                "functions": [{"name": "fc", "address": 0x2000, "size": 0x80}]}]
        qpath.write_text(json.dumps({"version": 1, "units": old}), encoding="utf-8")
        first = prop("proposal/1000_a.c", 0x1000, 0x1100)
        second = prop("proposal/2000_b.c", 0x2000, 0x2080)
        check("guard: a rewrite that keeps the whole queue is allowed",
              at.queue_guard(qpath, [first, second], []), None)
        reason = at.queue_guard(qpath, [first], [])
        check("guard: a sub-region rewrite is refused", bool(reason), True)
        check("guard: the refusal says the whole file would be replaced",
              "writes the *whole* queue" in reason, True)
        check("guard: the refusal names what it would drop", "proposal/2000_b.c" in reason, True)
        check("guard: the refusal names the flag that overrides it", "--replace-region" in reason, True)
        check("guard: --replace-region overrides the refusal",
              at.queue_guard(qpath, [first], [], replace_region=True), None)
        check("guard: a range a registered unit now holds is not a loss",
              at.queue_guard(qpath, [first], [(0x2000, 0x2080, "Pl/pl_act.cpp")]), None)
        check("guard: an unreadable queue is not refused over",
              at.queue_guard(Path(tmp) / "gone.json", [first], []), None)
        empty = Path(tmp) / "empty.json"
        empty.write_text(json.dumps({"version": 1, "units": []}), encoding="utf-8")
        check("guard: an empty queue is not refused over", at.queue_guard(empty, [first], []), None)
        # a re-cut of the same region loses nothing even when every label and every boundary moves
        recut = [prop("proposal/1000_z.c", 0x1000, 0x1080), prop("proposal/1080_y.c", 0x1080, 0x1100),
                 prop("proposal/2000_x.c", 0x2000, 0x2080)]
        check("guard: a re-cut of the whole region is allowed", at.queue_guard(qpath, recut, []), None)
        # the last function ends at 0x10F0, so the old entry's `text` tail is padding nobody owns: a new
        # tiling that stops at the function and not at the old range is not a lost proposal
        padded = [prop("proposal/1000_v.c", 0x1000, 0x10F0), prop("proposal/2000_t.c", 0x2000, 0x2080)]
        check("guard: a gap after the last function is not a lost proposal",
              at.queue_guard(qpath, padded, []), None)
        nofns = Path(tmp) / "nofns.json"
        nofns.write_text(json.dumps({"version": 1, "units": [{"label": "proposal/3000_w.c",
                                                                "text": [0x3000, 0x3080]}]}),
                         encoding="utf-8")
        check("guard: an entry with no functions falls back to its range",
              bool(at.queue_guard(nofns, [first], [])), True)
        check("guard: ... and is kept when its range is covered",
              at.queue_guard(nofns, [prop("proposal/3000_w.c", 0x3000, 0x3080)], []), None)

    ap = at.build_parser()
    check("cli: `queue` is a subcommand", ap.parse_args(["queue", "0x0", "0x1"]).cmd, "queue")
    check("cli: `plan` still parses", ap.parse_args(["plan", "0x0", "0x1"]).cmd, "plan")
    check("cli: `apply` defaults to refusing (no --legacy-register)",
          ap.parse_args(["apply", "0x0", "0x1"]).legacy_register, False)
    check("cli: `--legacy-register` is accepted on apply",
          ap.parse_args(["apply", "0x0", "0x1", "--legacy-register"]).legacy_register, True)
    check("cli: `queue` takes --max-total-bytes like the others",
          ap.parse_args(["queue", "0x0", "0x1", "--max-total-bytes", "0x1000"]).max_total_bytes, 0x1000)
    check("cli: `queue` refuses a dropping rewrite by default",
          ap.parse_args(["queue", "0x0", "0x1"]).replace_region, False)
    check("cli: `queue --replace-region` opts in",
          ap.parse_args(["queue", "0x0", "0x1", "--replace-region"]).replace_region, True)
    check("cli: only `queue` carries --replace-region",
          hasattr(ap.parse_args(["plan", "0x0", "0x1"]), "replace_region"), False)

    # determinism: the queue is a pure function of its inputs, so regenerating over an unchanged tree is
    # not a diff. This is what the removed wall-clock timestamp used to break.
    with tempfile.TemporaryDirectory() as tmp:
        props = [proposal("proposal/a.c", 0x80002000, 0x100, {})]
        fp = {"dol_sha1": "d" * 40, "symbols_sha1": "s" * 40,
              "splits_sha1": "p" * 40, "configure_sha1": "c" * 40}
        a = json.dumps(at.queue_doc(props, 0, fp), indent=1)
        b = json.dumps(at.queue_doc(props, 0, fp), indent=1)
        check("queue: the same inputs produce byte-identical documents", a == b, True)
        # and a changed input is visible as a change, naming which input moved
        fp2 = dict(fp, symbols_sha1="t" * 40)
        check("queue: a changed input changes the document",
              json.dumps(at.queue_doc(props, 0, fp2), indent=1) != a, True)
        check("queue: the fingerprints are the whole difference",
              json.loads(a)["symbols_sha1"] != json.loads(json.dumps(at.queue_doc(props, 0, fp2)))["symbols_sha1"],
              True)

    real_fp = at.input_fingerprints()
    # A worktree has no `orig/` (it is gitignored), so the DOL cannot be hashed there. Assert the shape only
    # when the input exists: a selftest that needs the original binary fails in every worktree, which is where
    # most of this repo's work happens.
    if os.path.exists(at.td.DOL):
        check("fingerprints: the DOL is hashed from the real tree",
              bool(real_fp.get("dol_sha1")) and len(real_fp["dol_sha1"]) == 40, True)
    else:
        check("fingerprints: a missing DOL is None, not an exception (worktrees have no orig/)",
              real_fp.get("dol_sha1"), None)
    check("fingerprints: the symbol map is hashed too",
          bool(real_fp.get("symbols_sha1")) and len(real_fp["symbols_sha1"]) == 40, True)
    check("fingerprints: a missing input is None, never an exception",
          at.input_fingerprints(at.Layout(Path(tmp) / "nope", Path(tmp) / "nope2", Path(tmp)))["splits_sha1"],
          None)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
