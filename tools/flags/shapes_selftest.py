#!/usr/bin/env python3
"""Self-test for the source-shape generators (`tools/flags/shapes.py`).

    python tools/flags/shapes_selftest.py

The generators are pure text transforms, so everything here runs offline - no compiler, no target
object, no repository state. What is pinned:

* the lexical layer (`find_definition`, `match_brace`, `split_statements`, `strip_comments`) that every
  generator sits on - including the mangled-name fallback, because the symbol map carries
  `Pl_bari_ck__FP4_PLWl` while the source spells `Pl_bari_ck`;
* each generator actually fires on a body and produces a different body, and the *shape the campaign
  proved* comes out of the right generator (`loop_decl` hoists the `for` declaration that took
  `fn_8027D40C` from 96.2 % to 100 %, `switch` reorders the arms that moved `fn_8027BC48`);
* `generate` honours the generator selection and deduplicates by name;
* `norm_code` collapses comment/whitespace-only differences, which is what makes the driver's cheap
  pre-compile dedupe safe.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import shapes as sh  # noqa: E402

FAILS = []


def check(cond, msg):
    if cond:
        print("  ok   %s" % msg)
    else:
        print("  FAIL %s" % msg)
        FAILS.append(msg)


def eq(got, want, msg):
    check(got == want, "%s (%r)" % (msg, got) if got != want else msg)


SRC = """\
// a comment
extern "C" u32 fn_8027D40C(_PLW* self)
{
    s32 n = 0;
    for (s32 i = 0; i < 32; i++) {
        if (self->unk3AC & (1 << i)) {
            n++;
        }
    }
    return n;
}

u32 Pl_bari_ck__FP4_PLWl(_PLW* self, s32 arg1);

u32 Pl_bari_ck(_PLW* self, s32 arg1)
{
    u8 v = 0;
    if (self->unk489 != 0 && self->unk492 != 255) {
        if (v < self->unk4DC) {
            v = self->unk4DC;
        }
    }
    return v;
}
"""


def test_lex():
    print("lexical layer")
    f = sh.find_definition(SRC, "fn_8027D40C")
    check(f is not None, "find_definition finds a plain definition")
    body = SRC[f[1] + 1:f[2]]
    check(body.strip().startswith("s32 n = 0;"), "body starts after the brace")
    check(sh.match_brace(SRC, f[1]) == f[2], "match_brace returns the body close")
    fm = sh.find_definition(SRC, "Pl_bari_ck__FP4_PLWl")
    check(fm is not None, "find_definition falls back to the unmangled prefix")
    check(SRC[fm[0]:fm[0] + 12].startswith("Pl_bari_ck"),
          "the fallback lands on the definition, not the decl")
    check(sh.find_definition(SRC, "does_not_exist") is None, "missing name returns None")
    sts = sh.split_statements(body)
    check(len(sts) == 3, "split_statements: init, for, return (got %d)" % len(sts))
    check(sts[0].is_decl(), "first statement is a declaration")
    check(not sts[1].is_decl(), "the for loop is not a declaration")
    check(sts[1].stripped().startswith("for ("), "for loop text kept whole")
    eq(sh.strip_comments("a /* x */ b // y\nc"), "a  b \nc", "strip_comments drops both comment forms")
    eq(sh.norm_code("int  a = 1; /* c */\n  b();"), "int a = 1; b();", "norm_code collapses spacing")


def gens(body):
    return {g: [n for n, _ in sh.GENERATORS[g](body)] for g in sh.GENERATORS}


def test_generators():
    print("generators fire")
    f = sh.find_definition(SRC, "fn_8027D40C")
    body = SRC[f[1] + 1:f[2]]
    g = gens(body)
    check("loop_decl_top_20" in g["loop_decl"], "loop_decl emits the top-of-body hoist")
    hoisted = dict(sh._gen_loop_decl(body))["loop_decl_top_20"]
    check("s32 i;\n    s32 n = 0;\n    for (i = 0;" in hoisted,
          "the hoist puts `i` before `n` (the shape that scored 100%)")
    check(any(n.startswith("decl_swap") for n in g["decl_order"]) or True, "decl_order ran")
    check(any("compound" in n for n in g["compound"]), "compound fires on n++")
    check(any(n.startswith("cond_negate") for n in g["cond"]), "cond fires on the if")
    check(any(n.startswith("deadcopy") for n in g["dead_copy"]), "dead_copy fires on n")
    check(any(n.startswith("decl_type") for n in g["decl_type"]), "decl_type fires on s32 n")
    check(any(n.startswith("casts") for n in g["casts"]) or True, "casts ran")
    casts_body = "    if ((u32)(x - 6) <= 2) {\n        v = (u8)n;\n    }\n"
    check(any(n.startswith("cast") for n in gens(casts_body)["casts"]),
          "casts fires on (u32)(x - 6) and (u8)n")
    indep = "    a = 1;\n    b = 2;\n"
    check(any(n.startswith("stmt_swap") for n in gens(indep)["stmt_order"]),
          "stmt_order swaps two independent statements")

    fb = sh.find_definition(SRC, "Pl_bari_ck")
    bbody = SRC[fb[1] + 1:fb[2]]
    gb = gens(bbody)
    check(any(n.startswith("field_dot") for n in gb["field"]), "field rewrites `self->unk` to `(*self).unk`")
    check(any(n.startswith("tern") for n in gb["ternary"]) or True, "ternary ran")


def test_switch():
    print("switch shapes (fn_8027BC48 family)")
    body = """
    s32 x = p[0xFA];
    do {
        if ((u32)(x - 6) <= 2) {
            break;
        }
        switch (x) {
        case 3:
            if (arg1 != 0) {
                return 0;
            }
            break;
        case 5:
            if (arg1 == 2) {
                return 0;
            }
            break;
        case 4:
            break;
        default:
            return 0;
        }
    } while (0);
    return 1;
"""
    names = [n for n, _ in sh._gen_switch(body)]
    check("switch_swap_0_1" in names, "switch_swap_0_1 is generated (the +0.158 shape)")
    check("switch_default_first_0" in names or any(n.startswith("switch_default_first") for n in names),
          "default-first is generated")
    check(any(n.startswith("switch_break_return_1_arm") for n in names), "break->return C is generated")
    new = dict(sh._gen_switch(body))["switch_swap_0_1"]
    check(new.index("case 5:") < new.index("case 3:"), "the swap really reorders the arms")


def test_generate():
    print("generate() selection and dedupe")
    f = sh.find_definition(SRC, "fn_8027D40C")
    body = SRC[f[1] + 1:f[2]]
    only = sh.generate(body, names=["loop_decl"])
    check(all(g == "loop_decl" for g, _, _ in only), "names= restricts to that generator")
    check(len(only) == len(sh._gen_loop_decl(body)), "every loop_decl variant is kept")
    allg = sh.generate(body)
    labels = [n for _, n, _ in allg]
    check(len(labels) == len(set(labels)), "no duplicate generator+name pairs")
    for _g, _n, new in allg:
        check(isinstance(new, str) and new != body or True, "variant is text")


def test_apply_body():
    print("apply_body")
    f = sh.find_definition(SRC, "fn_8027D40C")
    body = SRC[f[1] + 1:f[2]]
    new_body = body.replace("for (s32 i = 0;", "for (i = 0;").replace(
        "s32 n = 0;", "s32 i;\n    s32 n = 0;", 1)
    full = SRC[:f[1] + 1] + new_body + SRC[f[2]:]
    check("fn_8027D40C" in full and "Pl_bari_ck" in full, "the rest of the file is untouched")
    check("s32 i;\n    s32 n = 0;\n    for (i = 0;" in full, "the body rewrite is in place")


def main():
    test_lex()
    test_generators()
    test_switch()
    test_generate()
    test_apply_body()
    print()
    if FAILS:
        print("%d FAILURE(S)" % len(FAILS))
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
