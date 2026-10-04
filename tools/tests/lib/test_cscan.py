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


def test_type_definitions_and_members(c):
    text = ("/* struct Fake { int no; }; */\ntypedef struct {\n    void (*init)(void* self);\n    int (*tick)(int);\n"
            "    u32 rtti_00;\n} Vtbl;\nclass Base : public Root {\npublic:\n    Vtbl* vtbl; /* +0x00 */\n"
            "    u8 pad[4] = {0};\n    void inl(void) { int x; x = 1; }\n    virtual;\n};\nstruct { int anon; };\n"
            'const char* s = "struct Str { int q; };";\n')
    defs = cscan.type_definitions(text)
    c.check("names: a typedef'd anonymous body takes its alias, a base clause is read, an unnamed one and a definition "
            "inside a comment or a literal are not", [n for n, _m in defs], ["Vtbl", "Base"])
    vt = dict(defs)["Vtbl"]
    c.check("function-pointer members", [(m.name, m.fn_ptr) for m in vt],
            [("init", True), ("tick", True), ("rtti_00", False)])
    c.check("a function pointer's declarator is its text with `(*name)(` cut out (vtableaudit's shape)",
            vt[0].decl.replace(" ", ""), "voidvoid*self)")
    base = dict(defs)["Base"]
    c.check("members: an initializer keeps the name before `=`; an inline body does not end a chunk, so the next `;` "
            "names its last identifier (a known quirk, kept)", [m.name for m in base], ["vtbl", "pad", "x"])
    c.check("... and a member's declarator starts at the previous `;` or the brace, an access label included (kept)",
            base[0].decl.split(), ["public:", "Vtbl*"])
    c.check("a bare access label or `virtual` is never a member",
            [m.name for m in cscan.members(" public: ; virtual ; int a; ")], ["a"])


def test_split_params_and_calls(c):
    code = "f(a, g(b, c), d[1, 2]) + f( ) + f(x"
    c.check("split_params: top-level commas only, with absolute offsets",
            cscan.split_params(code, 2, 21), [("a", 2), (" g(b, c)", 4), (" d[1, 2]", 13)])
    c.check("split_params: an empty range is one empty chunk", cscan.split_params(code, 2, 2), [("", 2)])
    calls = cscan.calls(code, "f")
    c.check("calls: each whole-word call with its matched paren; the unmatched one is skipped",
            [(k.start, k.end) for k in calls], [(0, 22), (25, 29)])
    c.check("calls: the arguments are split_params chunks; `f( )` has one blank chunk",
            ([a for a, _o in calls[0].args], [a for a, _o in calls[1].args]), (["a", " g(b, c)", " d[1, 2]"], [" "]))
    c.check("calls: `ff(` and `f_(` are other names", cscan.calls("ff(1); f_(2); f (3);", "f")[0].start, 14)


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


def test_rewrite_identifiers(c):
    pairs = {"fn_80001000": "doThing", "lbl_80002000": "thing_table"}
    text = ('#include "Mod/fn_80001000.h"\n'
            '#include <Mod/fn_80001000.h>\n'
            'void fn_80001000(void);   /* the `fn_80001000` call */\n'
            'x = lbl_80002000[3] + fn_80001000__Fv + @fn_80001000;\n'
            'log("fn_80001000 failed"); c = \'f\';\n'
            '// see src/Mod/fn_80001000.c and fn_80001000.\n'
            'y = s.fn_80001000; z = fn_800010001;\n')
    new, counts = cscan.rewrite_identifiers(text, pairs)
    lines = new.split("\n")
    c.check("a quoted include path is left", lines[0], '#include "Mod/fn_80001000.h"')
    c.check("an angle include is left", lines[1], "#include <Mod/fn_80001000.h>")
    c.check("the code token is rewritten, the comment mention is kept by default", lines[2],
            "void doThing(void);   /* the `fn_80001000` call */")
    c.check("a data label is rewritten; a mangling and an @-spelling are not", lines[3],
            "x = thing_table[3] + fn_80001000__Fv + @fn_80001000;")
    c.check("a string literal is never rewritten", lines[4], 'log("fn_80001000 failed"); c = \'f\';')
    c.check("a member spelling and a longer hex are left", lines[6], "y = s.fn_80001000; z = fn_800010001;")
    c.check("the counts", counts["fn_80001000"], {"code": 1, "comment": 0, "kept_comment": 2, "skipped": 6})
    new2, counts2 = cscan.rewrite_identifiers(text, pairs, comments=True)
    lines2 = new2.split("\n")
    c.check("with comments, a comment mention is rewritten", lines2[2], "void doThing(void);   /* the `doThing` call */")
    c.check("... but a path in a comment never is", lines2[5], "// see src/Mod/fn_80001000.c and doThing.")
    c.check("... and the counts move", (counts2["fn_80001000"]["comment"], counts2["fn_80001000"]["kept_comment"]),
            (2, 0))
    crlf = "a = fn_80001000;\r\nb = 1;\r\n"
    c.check("line endings are kept", cscan.rewrite_identifiers(crlf, pairs)[0], "a = doThing;\r\nb = 1;\r\n")
    c.check("an untouched text comes back as is", cscan.rewrite_identifiers("int x;\n", pairs)[0], "int x;\n")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
