"""lib.cscan: the lexer and its three views, brackets, struct definitions and fields, declarations, includes."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile

from tools.lib import cscan, testing

TIER = "fixture"


def test_strip(c):
    code, comm = cscan.strip("a /* goto */ b\nc // goto\nd \"goto\" 'x'\n")
    c.check("block comment blanked in code", "goto" in code, False)
    c.check("line comment and string blanked", code.count("goto"), 0)
    c.check("comment text kept in the comment view", comm.count("goto"), 2)
    c.check("the comment view blanks the literal", '"goto"' in comm, False)
    c.check("both views keep length", (len(code), len(comm)), (len(code), len(code)))
    c.check("newlines preserved", code.count("\n"), 3)
    c.check("code outside comments and literals is untouched", (code[0], code[13], code[15]), ("a", "b", "c"))
    esc = 'char* s = "a \\" goto b";\ngoto x;\n'
    c.check("an escaped quote does not end the literal", cscan.strip(esc)[0].count("goto"), 1)
    unterminated = "char c = 'a;\ngoto x;\n"
    c.check("an unterminated literal stops at the newline", cscan.strip(unterminated)[0].count("goto"), 1)
    c.check("an escaped newline in a literal is blanked in strip (length kept)",
            cscan.strip('"a\\\nb" x')[0], "       x")
    c.check("a literal opener inside a comment is still comment", cscan.strip("/* ' */ x")[0], "        x")
    c.check("a comment opener inside a literal is still literal", cscan.strip('"/*" x')[0], "     x")
    c.check("an unterminated block comment runs to the end", cscan.strip("x /* y\nz")[0], "x     \n ")
    c.check("a trailing backslash at EOF belongs to the literal", cscan.strip('"ab\\')[0], "    ")
    c.check("spans name each kind", [s.kind for s in cscan.spans("/*a*/ //b\n\"c\" 'd'")],
            ["block", "line", "string", "char"])


def test_strip_comments_and_remove(c):
    src = 'struct Fake; // Nope\n/* a\nb */ char* s = "Also\\\nFake";\nint x;\n'
    clean = cscan.strip_comments(src)
    c.check("strip_comments keeps length", len(clean), len(src))
    c.check("strip_comments keeps every newline, even an escaped one", clean.count("\n"), src.count("\n"))
    c.check("strip_comments blanks comments and literals", ("Nope" in clean, "Also" in clean), (False, False))
    c.check("remove_comments drops both comment forms", cscan.remove_comments("a /* x */ b // y\nc"), "a  b \nc")
    c.check("remove_comments keeps a literal holding a comment opener",
            cscan.remove_comments('s = "/* no */"; // yes'), 's = "/* no */"; ')


def test_brackets_and_preproc(c):
    code = "f(a, (b)) { if (x) { y; } }"
    c.check("match_paren", cscan.match_paren(code, 1), 8)
    c.check("match_brace", cscan.match_brace(code, 10), len(code) - 1)
    c.check("unmatched is -1", (cscan.match_brace("{ {", 0), cscan.match_paren("(", 0)), (-1, -1))
    masked = cscan.mask_preproc("#define X \\\n  { \\\n  }\nint y;\n")
    c.check("a directive and its continuations are blanked, newlines kept", masked.split("\n")[3], "int y;")
    c.check("... the continuation lines too", masked.split("\n")[1].strip(), "")


def test_struct_defs_and_fields(c):
    t = cscan.Text("/* size: 0x8 */\nstruct A {\n    u32 x;\n    struct { u8 b; } in;\n};\n"
                   "typedef struct {\n    u8 y;\n} B;\nclass C : public A { int z; };\n")
    defs = cscan.struct_defs(t)
    c.check("names: an anonymous body takes the name after it; `class C : base {` is not a STRUCT_RE match",
            [d.name for d in defs], ["A", "in", "B"])
    c.check("lines", [(d.line, d.end_line) for d in defs], [(2, 5), (4, 4), (6, 8)])
    a = defs[0]
    c.check("fields at the top level only (the nested body is one chunk)",
            [t.code[s:e].strip() for s, e in cscan.fields(t.code, a.open, a.close)], ["u32 x", "struct { u8 b; } in"])
    c.check("to_dict is the legacy shape", list(a.to_dict()), ["name", "start", "open", "close", "line", "end_line"])
    c.check("line_of / line_text / span_lines", (t.line_of(0), t.line_of(len("/* size: 0x8 */\n")),
                                                 t.line_text(2), t.span_lines(0, 17)), (1, 2, "struct A {", (1, 2)))


def test_declared_name(c):
    for seg, want in ((" void foo(int a)", "foo"), (" void (*cb)(void)", "cb"), (" int (*getf(int))(void)", "getf"),
                      (" u32 table[4] = {0}", "table"), (" int x = 3", "x"), ("  ", None)):
        c.check("declared_name(%r)" % seg, cscan.declared_name(seg), want)


def test_function_declarations(c):
    text = ('extern "C" {\nvoid* alloc(u32 size);\n}\nnamespace ns {\nclass K {\n    void m(void* p) { (void*)p; }\n};\n}\n'
            "static int f(int a) {\n    g((void*)a);\n    return 0;\n}\n#define M(x) foo(x)\nvoid (*cb)(void*) = 0;\n"
            "static_assert(sizeof(void*) == 4);\n")
    decls = cscan.function_declarations(cscan.Text(text))
    c.check("prototype in a linkage block, method, definition - not the call, the macro, the initializer, the "
            "static_assert", [d.name for d in decls], ["alloc", "m", "f"])
    alloc, m, f = decls
    c.check("a prototype has no body", (alloc.body, "body" in alloc.to_dict()), (None, False))
    c.check("a definition carries its body pair", f.body is not None and text[f.body[0]] == "{" and
            text[f.body[1]] == "}", True)
    c.check("params and return text", (alloc.params, alloc.ret.strip(), alloc.line), ("u32 size", "void*", 2))


def test_includes_and_closure(c):
    text = '#include "a.h"\n  #  include <b.h>\n// #include "c.h"\n/* see `#include "d.h"` */\n#include"e.h"\n'
    c.check("includes: every directive at a line start, in order", cscan.includes(text), ["a.h", "b.h", "e.h"])
    c.check("includes: quoted only", cscan.includes(text, angle=False), ["a.h", "e.h"])
    with tempfile.TemporaryDirectory() as tmp:
        inc = os.path.join(tmp, "include")
        os.makedirs(os.path.join(inc, "m"))
        files = {"m/top.h": '#include "m/a.h"\n#include "m/b.h"\n#include "missing.h"\n',
                 "m/a.h": '#include "m/b.h"\n', "m/b.h": '#include "m/top.h"\n'}
        for rel, body in files.items():
            with open(os.path.join(inc, rel), "w", encoding="utf-8") as fh:
                fh.write(body)
        start = os.path.join(inc, "m", "top.h")
        order = cscan.include_closure(start, lambda name, _includer: cscan.resolve_include(name, [inc]))
        c.check("closure: depth first, start first, each once, the cycle stops",
                [os.path.relpath(p, inc).replace(os.sep, "/") for p in order], ["m/top.h", "m/a.h", "m/b.h"])
        c.check("resolve_include: None outside the bases", cscan.resolve_include("missing.h", [inc]), None)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
