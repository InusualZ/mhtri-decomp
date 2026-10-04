"""recordmerge: one case per rule (splice into the filler, key per struct, compare the declaration) and per guard."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.text import line_ending
from tools.units import recordmerge as rm

TIER = "fixture"

# Small, hand-checked fixtures. The real-world acceptance case - reproducing the `_AINPC_W` merge that was done by
# hand - depends on the repo's own history; it was run against `089491a7b` and is recorded in the tool's commit.
BASE = """\
struct Small {
    /* +0x000 */ u8 unused_0x000[0x00C - 0x000];
    /* +0x00C */ u32 field_0x00C;
};

typedef struct _REC {
    /* +0x000 */ u8 unused_0x000[0x004 - 0x000];
    /* +0x004 */ u8 field_0x004;
    /* +0x005 */ u8 unused_0x005[0x008 - 0x005];
    /* +0x008 */ s16 field_0x008;
    /* +0x00A */ u8 unused_0x00A[0x010 - 0x00A];
} _REC;
"""

OTHER = """\
struct Small {
    /* +0x000 */ u8 unused_0x000[0x00C - 0x000];
    /* +0x00C */ u32 field_0x00C;
};

typedef struct _REC {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x004 - 0x001];
    /* +0x004 */ u8 field_0x004[3];
    /* +0x007 */ u8 field_0x007;
    /* +0x008 */ s16 field_0x008;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unused_0x00B[0x010 - 0x00B];
} _REC;
"""

# `active` spliced into the 0x000 filler; the 0x004 declaration conflict taken from the other (which shrinks the
# 0x005 filler to 0x007); `field_0x007` then spliced into that shrunken filler; the 0x00A filler split. Small must
# come out untouched - the per-struct rule.
WANT_REC = [
    "u8 active;",
    "u8 unused_0x001[0x004 - 0x001];",
    "u8 field_0x004[3];",
    "u8 field_0x007;",
    "s16 field_0x008;",
    "u8 field_0x00A;",
    "u8 unused_0x00B[0x010 - 0x00B];",
]
WANT_BASE_KEPT = [
    "u8 field_0x004;",
    "u8 field_0x007;",
]

ONE = """\
typedef struct _ONE {
    /* +0x000 */ u8 unused_0x000[0x002 - 0x000];
    /* +0x002 */ u32 tail;
} _ONE;
"""


def codes(text: str, group: str) -> list[str]:
    return [m.code for m in rm.group_map(text)[group].members]


def wants(c, name: str, plan, fragment: str) -> None:
    c.expect(name, any(fragment in u for u in plan.unresolved),
             "no unresolved entry contains %r; unresolved: %r" % (fragment, plan.unresolved))


def test_merge(c):
    plan = rm.merge(BASE, OTHER)
    c.check("clean merge applies", plan.ok, True)
    c.check("verification found nothing", plan.violations, [])
    c.check("_REC merged members", codes(plan.text, "_REC"), WANT_REC)
    c.check("Small untouched (per-struct keying)", codes(plan.text, "Small"), codes(BASE, "Small"))
    by_name = {d.name: d for d in plan.groups}
    c.check("one conflict", len(by_name["_REC"].conflicts), 1)
    c.check("conflict member", by_name["_REC"].conflicts[0]["member"], "+0x004 field_0x004")
    c.check("conflict resolution", by_name["_REC"].conflicts[0]["resolution"], "taken from the other")
    c.check("conflict covers the 0x005 filler", by_name["_REC"].conflicts[0]["covers"], ["+0x005 unused_0x005"])
    c.check("the covered filler is shrunk, not dropped", by_name["_REC"].dropped, ["unused_0x005 shrunk to +0x007"])
    c.check("three members spliced", len(by_name["_REC"].added), 3)

    plan = rm.merge(BASE, OTHER, take="base")
    c.check("take=base still applies", plan.ok, True)
    kept = [m.code for m in rm.group_map(plan.text)["_REC"].members if m.off in (0x004, 0x007)]
    c.check("take=base keeps the base's members", kept, WANT_BASE_KEPT)
    c.check("take=base records the resolution", [x["resolution"] for x in plan.groups[1].conflicts],
            ["the base's kept"])

    again = rm.merge(plan.text, plan.text)
    c.check("idempotent merge applies", again.ok, True)
    c.check("idempotent merge is a no-op", [d.added + d.conflicts + d.dropped for d in again.groups], [[], []])
    c.check("idempotent text is unchanged", again.text, plan.text)


def test_refusals(c):
    inside_named = """\
typedef struct _ONE {
    /* +0x000 */ u32 field_0x000;
    /* +0x004 */ u8 tail;
} _ONE;
"""
    plan = rm.merge(inside_named, """\
typedef struct _ONE {
    /* +0x000 */ u32 field_0x000;
    /* +0x002 */ u16 inner;
    /* +0x004 */ u8 tail;
} _ONE;
""")
    wants(c, "inside a named member", plan, "falls inside the base's member 'field_0x000'")
    c.check("inside-named refuses to write", plan.text, None)

    plan = rm.merge(ONE, """\
typedef struct _ONE {
    /* +0x000 */ u8 f0;
    /* +0x001 */ u16 f1;
    /* +0x003 */ u8 tail2;
} _ONE;
""")
    wants(c, "no room in the filler", plan, "needs 2 B but the base's filler")
    c.check("no-room refuses to write", plan.text, None)

    named_base = ONE.replace("/* +0x000 */ u8 unused_0x000[0x002 - 0x000];",
                             "/* +0x000 */ u8 field_0x000;\n"
                             "    /* +0x001 */ u8 unused_0x001[0x002 - 0x001];")
    plan = rm.merge(named_base, """\
typedef struct _ONE {
    /* +0x000 */ u8 renamed_0x000;
    /* +0x001 */ u8 unused_0x001[0x002 - 0x001];
    /* +0x002 */ u32 tail;
} _ONE;
""")
    wants(c, "same-offset rename", plan, "the base calls it 'field_0x000' and the other 'renamed_0x000'")

    # a filler at that offset is the normal case, not a rename: it is spliced like any other field
    plan = rm.merge(ONE, """\
typedef struct _ONE {
    /* +0x000 */ u8 renamed_0x000;
    /* +0x001 */ u8 unused_0x001[0x002 - 0x001];
    /* +0x002 */ u32 tail;
} _ONE;
""")
    c.check("filler -> field is a splice, not a rename", codes(plan.text, "_ONE")[0], "u8 renamed_0x000;")

    plan = rm.merge(ONE, """\
typedef struct _ONE {
    /* +0x000 */ u8 f0;
    /* +0x001 */ u8 f1;
} _ONE;
""")
    wants(c, "last member has no inferable size", plan, "last member of the other's layout")

    plan = rm.merge(ONE, """\
typedef struct _ONE {
    /* +0x000 */ u8 unused_0x000[0x002 - 0x000];
    /* +0x002 */ u32 tail;
} _ONE;

typedef struct _EXTRA {
    /* +0x000 */ u8 x;
} _EXTRA;
""")
    wants(c, "a group only the other has", plan, "the base has 0 groups with that name")


def test_verify(c):
    # verify() is the guard that protects the file, so break its invariant on purpose
    good = rm.merge(BASE, OTHER).text
    broken = good.replace("    /* +0x007 */ u8 field_0x007;\n", "")
    violations = rm.verify(BASE, OTHER, broken)
    c.check("verify catches a dropped member",
            any("is in the other and missing from the merge" in v for v in violations), True)
    c.check("verify is quiet on the good merge", rm.verify(BASE, OTHER, good), [])


def test_write_path(c):
    # refuses while unresolved, preserves CRLF when it does write
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        base = root / "rec.h"
        base.write_bytes(BASE.replace("\n", "\r\n").encode("utf-8"))
        before = base.read_bytes()

        other = root / "other.h"
        other.write_text(ONE, encoding="utf-8")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = rm.main(["--base", str(base), "--other", str(other)])
        c.check("unresolved run exits 1", rc, 1)
        c.check("unresolved run leaves the file alone", base.read_bytes(), before)

        other.write_text(OTHER, encoding="utf-8")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = rm.main(["--base", str(base), "--other", str(other), "--out", str(base)])
        text = base.read_bytes().decode("utf-8")
        c.check("clean run exits 0", rc, 0)
        c.check("the merge preserved the file's CRLF", line_ending(text), "\r\n")
        c.check("the written header is the merged one", codes(text, "_REC"), WANT_REC)
        c.contains("the run says it wrote", buf.getvalue(), "WRITTEN")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
