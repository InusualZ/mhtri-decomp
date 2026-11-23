"""tywidth: the range guard refuses widths that cannot hold the values a local takes (the `u8 id = 0x2092` and
`u8 base_c = 0x400` truncations of the retired sweep), and the sweep keeps a width only when the score rises and the size
does not move away from the target.  Mutation: dropping the constant check lets `u8` through for 0x2092; dropping
`size_ok` keeps the 436 B variant."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

from tools.lib import testing
from tools.flags import tywidth

TIER = "fixture"

HEADER = """
struct Rec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x02 */ u16 id_0x02;
    /* +0x04 */ s16 delta_0x04;
    /* +0x06 */ s32 big_0x06;
};
extern u8 small_tbl[6];
u16 get_count(void);
s32 get_signed(void);
"""


def plan(body, header=HEADER, params="Rec* r", loose=False):
    """The plan of a one-function source: {name: (allowed, refused-types)}."""
    src = "void f(%s) {\n%s\n}\n" % (params, body)
    lines = src.split("\n")
    loc = tywidth.find_def(lines, "f")
    index = tywidth.TypeIndex.from_texts([header])
    return {p["name"]: (p["allowed"], sorted(p["refused"])) for p in
            tywidth.plan_function(lines, loc[0], loc[1], index, loose_sources=loose)}


def test_literals(c):
    c.check("hex, decimal, suffix, negative, char", [tywidth.lit_value(x) for x in ("0x400", "590", "7u", "-3", "'A'", "x")],
            [0x400, 590, 7, -3, 65, None])
    c.check("comments are blanked, lines kept", tywidth.strip_comments("a /* x\ny */ b // z\nc"), "a     \n     b \nc")


def test_constants(c):
    p = plan("    u8 base_c;\n    base_c = 0x400;")
    c.check("0x400 cannot live in a byte: u8/s8 refused", [t for t in ("u8", "s8") if t in p["base_c"][0]], [])
    c.check("... u16/s16/s32 hold it", sorted(p["base_c"][0]), ["s16", "s32", "u16", "u32"])
    p = plan("    u16 help;\n    help = 0x1EE;\n    if (help == 0x1F9) {\n    }")
    c.check("help 0x1EE..0x1F9: no byte type", sorted(set(p["help"][1]) & {"u8", "s8"}), ["s8", "u8"])
    p = plan("    u16 v;\n    if (v == 0xFFFF) {\n    }")
    c.check("a compare with 0xFFFF refuses s16 (it would never be equal) and the bytes", sorted(p["v"][1]), ["s16", "s8", "u8"])
    p = plan("    s32 i;\n    for (i = 0; i < 200; i++) {\n    }")
    c.check("a loop bound of 200 refuses s8", "s8" in p["i"][1] and "u8" not in p["i"][1], True)
    p = plan("    s32 n;\n    n = -1;")
    c.check("a negative constant refuses the unsigned types", sorted(set(p["n"][1]) & {"u8", "u16", "u32"}), ["u16", "u32", "u8"])
    p = plan("    u8 id;\n    id = (r->kind_0x00 < 20) ? 0x2092 : 0x209B;")
    c.check("a ternary of two sprite ids (the eft050 truncation): u8 and s8 refused, u16 kept",
            (sorted(set(p["id"][1]) & {"u8", "s8"}), "u16" in p["id"][0]), (["s8", "u8"], True))
    c.check("... the current type is never offered", "u8" in p["id"][0], False)
    p = plan("    s32 w;\n    switch (w) {\n    case 300:\n        break;\n    }")
    c.check("a case label counts as a compared constant", "u8" in p["w"][1] and "s16" in p["w"][0], True)


def test_sources(c):
    p = plan("    s32 a;\n    a = r->id_0x02;")
    c.check("a u16 field: s16 and the bytes refused, u16/s32/u32 hold it", sorted(p["a"][1]), ["s16", "s8", "u8"])
    p = plan("    s32 a;\n    a = r->kind_0x00;")
    c.check("a u8 field: s8 refused, s16/u8/u16 hold it", sorted(p["a"][1]), ["s8"])
    p = plan("    s32 a;\n    a = r->delta_0x04;")
    c.check("an s16 field: every unsigned type refused", sorted(p["a"][1]), ["s8", "u16", "u32", "u8"])
    p = plan("    s32 a;\n    a = small_tbl[2];")
    c.check("an array element has the array's element type", sorted(p["a"][1]), ["s8"])
    p = plan("    s32 a;\n    a = get_count();")
    c.check("a call has its declared return type", sorted(p["a"][1]), ["s16", "s8", "u8"])
    p = plan("    s32 a;\n    a = (s16)(r->big_0x06 / 3);")
    c.check("a cast bounds an unbounded expression by the cast type", sorted(p["a"][1]), ["s8", "u16", "u32", "u8"])
    p = plan("    s32 a;\n    a = r->big_0x06 + 1;")
    c.check("an expression with no declared bound allows only the 32-bit types",
            sorted(p["a"][1]), ["s16", "s8", "u16", "u8"])
    c.check("... --loose-sources only checks literals", sorted(plan("    s32 a;\n    a = r->big_0x06 + 1;", loose=True)["a"][1]), [])
    p = plan("    s32 a;\n    s16 b = 4;\n    a = b;")
    c.check("a local's declared width is its range", sorted(p["a"][1]), ["s8", "u16", "u32", "u8"])
    p = plan("    u32 roll;\n    roll = seed % 100;", params="u16 seed")
    c.check("`x % 100` is 0..99: every width holds it", p["roll"][1], [])
    p = plan("    s32 m;\n    m = seed & 0xFF;", params="u16 seed")
    c.check("`x & 0xFF` is 0..255: s8 refused only", p["m"][1], ["s8"])


def test_not_assignment(c):
    p = plan("    s32 a;\n    if (a == r->kind_0x00) {\n    }\n    a = 3;")
    c.check("`==` is a compare (a u8 field), not an assignment", sorted(p["a"][1]), ["s8"])
    p = plan("    s32 a;\n    a=r->id_0x02;")
    c.check("`a=x` without spaces is an assignment", sorted(p["a"][1]), ["s16", "s8", "u8"])


def test_size_rule(c):
    c.check("equal stays equal", tywidth.size_ok(100, 100, 100), True)
    c.check("a variant that leaves the target size is refused", tywidth.size_ok(100, 96, 100), False)
    c.check("closer is kept", tywidth.size_ok(96, 100, 100), True)
    c.check("closer from the other side is kept", tywidth.size_ok(110, 104, 100), True)
    c.check("farther from the other side is refused", tywidth.size_ok(104, 110, 100), False)


def test_sweep(c):
    """A fake compiler: the score and size of the function depend on the width of `a` and `i` only."""
    src = "void f(Rec* r) {\n    s32 a;\n    s32 i;\n    a = r->kind_0x00;\n    i = 0;\n}\n".split("\n")
    loc = tywidth.find_def(src, "f")
    index = tywidth.TypeIndex.from_texts([HEADER])
    state = {"lines": None, "calls": []}

    def write(ls):
        state["lines"] = list(ls)

    def measure():
        ls = state["lines"] or src
        a = ls[1].split()[0]
        i = ls[2].split()[0]
        state["calls"].append((a, i))
        pct = {"s32": 90.0, "u8": 99.0, "s8": 100.0, "u16": 95.0, "s16": 92.0, "u32": 90.0}[a]
        pct += {"s32": 0.0, "s16": 2.0, "u8": 1.0}.get(i, 0.0)
        size = 100 if i == "s32" else 96
        return (min(pct, 100.0), 100, size)

    state["lines"] = list(src)
    cur, base, best, log, skipped = tywidth.sweep(src, loc[0], loc[1], index, measure, write)
    c.check("s8 holds nothing a u8 field gives: it is never compiled", any(a == "s8" for a, _ in state["calls"]), False)
    c.check("the guard counted the refused width", skipped >= 1, True)
    c.check("a: u8 raises the score at the target size and is kept", cur[1].strip(), "u8 a;")
    c.check("i: s16 raises the row but leaves the 100 B target size, so s32 stays", cur[2].strip(), "s32 i;")
    c.check("the source left on disk is the kept one", state["lines"], cur)
    c.check("the log names the kept change only", [x.split()[0] for x in log], ["a:s32->u8"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
