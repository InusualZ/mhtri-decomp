"""Self-test for `tools/units/attribute.py` - the partitioner, not the map.

The risky part of bulk attribution is `segments`: which pieces a region is cut into, and whether the two
repair rules (too small joins its neighbour, too large is split and flagged) do what they claim. Those are
pure functions over an `an`-shaped dict, so they can be tested without the DOL.

    python tools/units/attribute_selftest.py
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import attribute as at  # noqa: E402

FAIL = []


def check(name, got, want):
    if got != want:
        FAIL.append("%s: got %r, want %r" % (name, got, want))


def fake_an(funcs, soft=(), must_link=()):
    """`an` with only the keys the partitioner reads, in the shape `tudiscover` produces."""
    return {
        "ordered": ["f%d" % i for i in range(len(funcs))],
        "addr": [a for a, _ in funcs],
        "size": [s for _, s in funcs],
        "soft": list(soft),
        "must_link": list(must_link),
    }


# --- names -----------------------------------------------------------------------------------------
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

# --- seams -----------------------------------------------------------------------------------------
# six 100-byte functions at 0x1000: cuts are `boundary before function c`, so 1..5 are interior
FUNCS = [(0x1000 + 0x64 * i, 0x64) for i in range(6)]

an = fake_an(FUNCS, soft=[(3, 3, 4.0, "pool", "shared pool")])
check("seams: a narrow strong observation pins its cut", sorted(at.interior_seams(an, 0, 6)), [3])
check("seams: ends are not interior", sorted(at.interior_seams(fake_an(FUNCS, soft=[(0, 0, 4.0, "pool", "x")]), 0, 6)), [])
check("seams: a wide observation is not a seam",
      sorted(at.interior_seams(fake_an(FUNCS, soft=[(1, 5, 4.0, "pool", "x")]), 0, 6)), [])
check("seams: a weak kind is not a seam",
      sorted(at.interior_seams(fake_an(FUNCS, soft=[(3, 3, 4.0, "weak-kind", "x")]), 0, 6)), [])
check("seams: an anchor vetoes a cut inside it",
      sorted(at.interior_seams(fake_an(FUNCS, soft=[(3, 3, 4.0, "pool", "x")],
                                      must_link=[(2, 3, "anchor")]), 0, 6)), [])

# --- segments --------------------------------------------------------------------------------------
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

if FAIL:
    print("FAIL (%d)" % len(FAIL))
    for f in FAIL:
        print("  " + f)
    sys.exit(1)
print("ok - %d checks" % 23)
