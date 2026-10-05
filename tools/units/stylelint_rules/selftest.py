"""The lint's selftest: every rule, the add-only diff and its credits, on fixtures and temporary git trees.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import json
import os
import subprocess
import tempfile

from tools.units.stylelint_rules.api import *  # noqa: F401,F403 - the selftest reads every name the lint has
import tools.units.stylelint_rules.refs as _refs  # the seam the selftest stubs


# --------------------------------------------------------------------------------------------------
# selftest
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def rules_of(text: str, rel: str = "x.c", ownership=None) -> list[tuple[int, int]]:
        return [(f["rule"], f["line"]) for f in lint_source(Source("x.c", rel, text), ownership)]

    def lines_of(text: str, rule: int, rel: str = "x.c", ownership=None) -> list[int]:
        return [f["line"] for f in lint_source(Source("x.c", rel, text), ownership) if f["rule"] == rule]

    # --- stripping --------------------------------------------------------------------------------
    code, comm = strip("a /* goto */ b\nc // goto\nd \"goto\" 'x'\n")
    check("strip: block comment blanked", "goto" in code, False)
    check("strip: line comment blanked", code.count("goto"), 0)
    check("strip: string blanked", code.count("goto"), 0)
    check("strip: comment text kept", comm.count("goto"), 2)
    check("strip: length preserved", len(code), len(comm))
    check("strip: newlines preserved", code.count("\n"), 3)
    multi = "/* a\ngoto\nb */\nx\ngoto done;\n"
    check("strip: multi-line comment keeps line numbers", [f["line"] for f in lint_source(Source("x.c", "x.c", multi)) if f["rule"] == 8], [5])
    esc = 'char* s = "a \\" goto b";\ngoto x;\n'
    check("strip: escaped quote in a literal", lines_of(esc, 8), [2])
    check("strip: unterminated line literal stops at newline",
          lines_of('char c = \'a;\ngoto x;\n', 8), [2])
    check("strip: comment body has the annotation", bool(SIZE_RE.search(strip("/* size: 0x10 */\n")[1])), True)
    check("strip: annotation inside a string is not seen", bool(SIZE_RE.search(strip('"size: 0x10"\n')[1])), False)

    # --- rule 3: size -----------------------------------------------------------------------------
    check("rule3: annotated before the definition", lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 3), [])
    check("rule3: annotated after the closing brace", lines_of("struct A {\n    u32 x;\n};\n/* size: 0x8 */\n", 3), [])
    check("rule3: no annotation is a violation", lines_of("struct A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: anonymous typedef gets the name after the brace",
          lines_of("typedef struct {\n    u32 x;\n} A;\n", 3), [1])
    check("rule3: annotation too far away", lines_of("/* size: 0x8 */\n\n\n\n\n\nstruct A {\n    u32 x;\n};\n", 3), [7])
    check("rule3: a forward declaration is not a definition", lines_of("struct A;\n", 3), [])
    check("rule3: a use in a parameter list is not a definition", lines_of("void f(struct A* a);\n", 3), [])
    check("rule3: two types, one annotation - only the near one is certified",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\nstruct B {\n    u32 y;\n};\n", 3), [5])
    check("rule3: class counts too", lines_of("class A {\n    u32 x;\n};\n", 3), [1])
    check("rule3: annotation in a string does not certify",
          lines_of('const char* s = "size: 0x8";\nstruct A {\n    u32 x;\n};\n', 3), [2])

    # --- rule 4/5: fields -------------------------------------------------------------------------
    check("rule4: canonical +0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: the existing 0x annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* 0x00 */ u32 x;\n};\n", 4), [])
    check("rule4: trailing annotation passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x; /* 0x00 - counter */\n};\n", 4), [])
    check("rule4: missing offset is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4), [3])
    check("rule4: two fields, one annotated",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 x;\n    u32 y;\n};\n", 4), [4])
    check("rule4: function pointer field is a field",
          lines_of("/* size: 0x8 */\nstruct A {\n    void (*cb)(void);\n};\n", 4), [3])
    check("rule5: unk field is a violation",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 5), [3])
    check("rule5: pad_/unused_ pass",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 pad_0x00[4];\n    /* +0x04 */ u8 unused_0x04[4];\n};\n", 5), [])
    check("rule5: a named field passes",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id;\n};\n", 5), [])
    check("rule5: unk in a comment does not count",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 item_id; /* was unk00 */\n};\n", 5), [])
    check("rule5: an array field name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u8 unk00[4];\n};\n", 5), [3])
    check("rule5: a bitfield name is found",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00 : 3;\n};\n", 5), [3])
    check("rule4: only the top-level body counts, not a nested type",
          lines_of("/* size: 0x8 */\nstruct A {\n    struct B { /* +0x00 */ u32 z; } b;\n    /* +0x00 */ u32 x;\n};\n", 4), [])

    # --- rule 6: pointer arithmetic ---------------------------------------------------------------
    check("rule6: the classic field poke",
          lines_of("void f(u8* self) {\n    *(u32*)((u8*)self + 0x1C) = 1;\n}\n", 6), [2])
    check("rule6: (char*)obj + 0x10",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((char*)obj + 0x10);\n}\n", 6), [2])
    check("rule6: decimal offset",
          lines_of("void f(void* obj) {\n    u32 v = *(u32*)((u8*)obj + 4);\n}\n", 6), [2])
    check("rule6: a member access is clean", lines_of("void f(A* self) {\n    self->field = 1;\n}\n", 6), [])
    check("rule6: memset is clean", lines_of("void f(void* p) {\n    memset(p, 0, 0x10);\n}\n", 6), [])
    check("rule6: offsetof is clean", lines_of("u32 n = offsetof(A, x);\n", 6), [])
    check("rule6: a cast with no offset is clean", lines_of("void f(void* p) {\n    u32 v = *(u32*)p;\n}\n", 6), [])
    check("rule6: memset's allowed byte range is clean",
          lines_of("void f(u8* p) {\n    memset((u8*)p + 4, 0, 8);\n}\n", 6), [])
    check("rule6: memcpy's allowed byte range is clean",
          lines_of("void f(u8* p, u8* q) {\n    memcpy(p, (u8*)q + 8, 4);\n}\n", 6), [])
    check("rule6: a cast offset passed to another callee is not allowed",
          lines_of("void f(u8* p) {\n    foo((u8*)p + 4);\n}\n", 6), [2])
    check("rule6: the memset exception does not cover a dereference",
          lines_of("void f(u8* p) {\n    memset(p, 0, *(u32*)((u8*)p + 4));\n}\n", 6), [2])
    check("rule6: inside a comment it is clean",
          lines_of("/* written as *(s16*)((u8*)self + 0x1C) */\nvoid f(void) {}\n", 6), [])
    check("rule6: two sites on one line are two findings",
          lines_of("void f(u8* p) {\n    a = *(u32*)((u8*)p + 4); b = *(u32*)((u8*)p + 8);\n}\n", 6), [2, 2])

    # --- the STOPGAP block: no open request id is a rule-2 finding; the marker exempts nothing ---------
    with tempfile.TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "lane-a-requests.json"), "w", encoding="utf-8") as fh:
            fh.write('{"id": "lane-a#1", "kind": "decl", "symbol": "foo", "evidence": "x"}\n'
                     '{"id": "lane-a#2", "kind": "decl", "symbol": "bar", "evidence": "x"}\n')
        with open(os.path.join(tmp, "lane-a-requests.status.json"), "w", encoding="utf-8") as fh:
            fh.write('{"lane-a#2": {"status": "applied"}}\n')
        set_request_dirs([tmp])
        gap = "/* STOPGAP-BEGIN(%s) */\nvoid foo(void);\n/* STOPGAP-END(%s) */\n"
        msgs = lambda text: [f["detail"] for f in lint_source(Source("x.c", "src/A/a.c", text)) if "STOPGAP" in f["detail"]]
        check("stopgap: a block naming an open request is not a finding", msgs(gap % ("lane-a#1", "lane-a#1")), [])
        check("stopgap: a block naming an applied request is", len(msgs(gap % ("lane-a#2", "lane-a#2"))), 1)
        check("stopgap: a block naming no request at all is", len(msgs(gap % ("lane-z#9", "lane-z#9"))), 1)
        check("stopgap: ... and it is rule 2", [f["rule"] for f in lint_source(Source("x.c", "src/A/a.c",
                                                gap % ("lane-z#9", "lane-z#9"))) if "STOPGAP" in f["detail"]], [2])
        check("stopgap: an unpaired BEGIN is a finding", len(msgs("/* STOPGAP-BEGIN(lane-a#1) */\nint x;\n")), 1)
        check("stopgap: the marker exempts nothing (the fn_ inside still fires rule 7)",
              lines_of((gap % ("lane-a#1", "lane-a#1")).replace("foo", "fn_80001234"), 7, "src/A/a.c"), [2])
        check("stopgap: a header is held to it too",
              len([f for f in lint_source(Source("x.h", "include/A/a.h", gap % ("lane-z#9", "lane-z#9")))
                   if "STOPGAP" in f["detail"]]), 1)
        set_request_dirs(None)

    # --- rule 7: names ----------------------------------------------------------------------------
    check("rule7: fn_ name is a violation", lines_of("void fn_80040598(void) {}\n", 7), [1])
    check("rule7: a named function is clean", lines_of("void Pl_Skill_ck(void) {}\n", 7), [])
    check("rule7: bare unk local is a violation", lines_of("void f(void) {\n    u32 unk4 = 0;\n}\n", 7), [2])
    check("rule7: a field's unk is rule 5's, not rule 7's",
          lines_of("/* size: 0x8 */\nstruct A {\n    /* +0x00 */ u32 unk00;\n};\n", 7), [])
    check("rule7: unk in a comment does not count", lines_of("/* unk4 fn_80040598 */\nvoid f(void) {}\n", 7), [])
    check("rule7: unk in a string does not count", lines_of('const char* s = "unk4";\n', 7), [])
    check("rule7: a name that merely contains unk is clean", lines_of("void f(void) {\n    u32 junk = 0;\n}\n", 7), [])
    check("rule7: fn_ with the wrong digit count is clean", lines_of("void fn_1234(void) {}\n", 7), [])

    # --- rule 7 has no exemption: every generated name in src/ fires, everywhere -------------------
    auto = "src/auto/802B2978_fn_802B2978.c"
    # (the generated file name itself is one more rule-7 finding on line 1 since 2026-10-05)
    check("rule7: a src/auto/ file's own fn_ name fires (the bucket exemption is gone)",
          lines_of("void fn_802B2978(void) {}\n", 7, auto), [1, 1])
    check("rule7: a src/auto/ body calling fn_ fires",
          lines_of("void fn_802B2978(void) {\n    fn_80040598();\n}\n", 7, auto), [1, 1, 2])
    check("rule7: a src/auto/ bare unk local fires too",
          lines_of("void fn_802B2978(void) {\n    u32 unk4 = 0;\n}\n", 7, auto), [1, 1, 2])
    check("rule7: a subdirectory of src/auto/ fires (no path exempts anything)",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/auto/deep/x.c"), [1])
    check("rule7: the same shape under src/Pl/ fires",
          lines_of("void fn_802B2978(void) {}\n", 7, "src/Pl/pl_act.cpp"), [1])
    check("rule7: only rule 7 ever had an exemption - rule 4 still fires under src/auto/",
          lines_of("/* size: 0x8 */\nstruct A {\n    u32 x;\n};\n", 4, auto), [3])
    check("rule7: the EXEMPT table is empty (every path is enforced)", EXEMPT, [])
    check("rule7: the reported exemption list is empty", exemptions(), [])
    check("rule7: rule_enforced is True everywhere, with or without the file",
          [rule_enforced(7, "src/auto/x.c"),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c", "/* stub: only a header comment */\n")),
           rule_enforced(7, "src/Pl/x.c", Source("x.c", "src/Pl/x.c",
                         "/* rule 7 deferred: the map has no name */\nvoid fn_80040598(void) {}\n")),
           rule_enforced(6, "src/auto/x.c"), rule_enforced(7, "src/auto\\x.c")],
          [True, True, True, True, True])

    # --- rule 7 key 2 is gone: a file with no bodies is held to it too ----------------------------
    pl = "src/Pl/pl_act.cpp"
    check("rule7 bodyless: a fn_ prototype fires with no body in the file",
          lines_of("void fn_802B2978(void);\n", 7, pl), [1])
    check("rule7 bodyless: a fn_ call with no body fires",
          lines_of("fn_80040598();\n", 7, pl), [1])
    check("rule7 bodyless: a bare unk declaration fires",
          lines_of("u32 unk4;\n", 7, pl), [1])
    check("rule7 bodyless: adding a body changes nothing about rule 7",
          lines_of("void fn_802B2978(void) {}\n", 7, pl), [1])
    check("rule7 bodyless: rule 6 still fires without a body",
          lines_of("u32 v = *(u32*)((u8*)p + 4);\n", 6, pl), [1])
    check("rule7 bodyless: rule 8 still fires without a body",
          lines_of("goto out;\n", 8, pl), [1])

    # --- rule 7 key 3 is gone: no comment exempts anything ----------------------------------------
    defer = "/* rule 7 deferred: the map has only fn_XXXXXXXX for this range */\n"
    check("rule7 deferred: the comment does not hide a fn_ definition",
          lines_of(defer + "void fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: the comment does not hide a fn_ call",
          lines_of(defer + "void f(void) {\n    fn_80040598();\n}\n", 7, pl), [3])
    check("rule7 deferred: the comment does not hide a bare unk local",
          lines_of(defer + "void f(void) {\n    u32 unk4 = 0;\n}\n", 7, pl), [3])
    check("rule7 deferred: a line-comment spelling is inert too",
          lines_of("// rule 7 deferred: the map has no name\nvoid fn_802B2978(void) {}\n", 7, pl), [2])
    check("rule7 deferred: the spelling inside a string is not even a comment",
          lines_of('const char* s = "rule 7 deferred: x";\nvoid fn_802B2978(void) {}\n', 7, pl), [2])
    check("rule7 deferred: a comment naming a generated symbol is not itself a finding",
          lines_of("/* fn_80040598 unk4 lbl_80010000 */\nvoid f(void) {}\n", 7), [])
    check("rule7 deferred: rules 1-6 and 8 still fire in a file carrying the comment",
          [r for r, _l in rules_of(
              defer
              + "/* size: 0x8 */\nstruct A {\n    u32 x;\n    /* +0x04 */ u32 unk04;\n};\n"
              + "void fn_802B2978(u8* p) {\n    *(u32*)((u8*)p + 4) = 1;\n    goto out;\nout:\n    return;\n}\n",
              pl)],
          [4, 5, 6, 7, 8])
    # The dead-key proof on the real incident file: `src/ef/eft053.cpp` carries a `rule 7 deferred:`
    # comment AND unrenamed `fn_` references, and must still report every one of them. If this ever goes
    # quiet the comment is suppressing again - which is the defect this whole change exists to remove.
    if os.path.exists("src/ef/eft053.cpp"):
        real = [f for f in lint_source(Source("src/ef/eft053.cpp", "src/ef/eft053.cpp",
                                              read_text("src/ef/eft053.cpp"))) if f["rule"] == 7]
        check("rule7 deferred: the real ef/eft053.cpp (comment + generated names) still reports rule 7",
              bool(real), True)
        check("... and every finding is a generated name, not an unk field",
              all(f["detail"].startswith(("auto-generated name", "data label", "bare ", "address-named identifier"))
                  for f in real), True)

    # --- rule 7 exact (owner, 2026-10-05): a stem anywhere in a token, address-named identifiers, headers -------
    def r7_tokens(text: str, rel: str = "src/x.c") -> list:
        return [f["token"] for f in lint_source(Source("x", rel, text)) if f["rule"] == 7]

    check("rule7 exact: a suffix form counts as one token (`fn_X__FPv`)",
          r7_tokens("void fn_80041234__FPv(void);\n"), ["fn_80041234__FPv"])
    check("rule7 exact: a prefixed form counts (`view_fn_X`)", r7_tokens("int view_fn_80041234;\n"),
          ["view_fn_80041234"])
    check("rule7 exact: a tail after the address counts (`fn_X_fx`)", r7_tokens("int fn_800FD864_fx;\n"),
          ["fn_800FD864_fx"])
    check("rule7 exact: dtor_ and zz_ stems count", r7_tokens("void dtor_8005E5E8(void);\nint zz_80123456_;\n"),
          ["dtor_8005E5E8", "zz_80123456_"])
    check("rule7 exact: lower-case hex still counts", r7_tokens("void fn_8004cad8(void);\n"), ["fn_8004cad8"])
    check("rule7 exact: address-named identifiers count (Panel805482CC, s_80276B58, Helper_80147CE0)",
          r7_tokens("struct Panel805482CC;\nchar s_80276B58[4];\nvoid Helper_80147CE0(void);\n"),
          ["Panel805482CC", "s_80276B58", "Helper_80147CE0"])
    check("rule7 exact: a generated header's guard is address-named",
          r7_tokens("#ifndef MHTRI_ENEMY_FN_80128204_H\n#define MHTRI_ENEMY_FN_80128204_H\n#endif\n", "include/e.h"),
          ["MHTRI_ENEMY_FN_80128204_H", "MHTRI_ENEMY_FN_80128204_H"])
    check("rule7 exact: a hex literal is a number, not a name",
          r7_tokens("u32 a = 0x80276B58;\nu32 b = 0x80276B58u;\n"), [])
    check("rule7 exact: an address-shaped value outside the DOL span is not an address (quest_flag_80000000_ck)",
          r7_tokens("int quest_flag_80000000_ck(void);\n"), [])
    check("rule7 exact: eight characters followed by another hex digit are not an address (Eft8030EffectSlot)",
          r7_tokens("struct Eft8030EffectSlot;\n"), [])
    check("rule7 exact: a nine-digit number inside a name is not an address", r7_tokens("int abc180123456;\n"), [])
    check("rule7 exact: an #include path does not count (the file-name finding retires it)",
          r7_tokens('#include "enemy/fn_80128204.h"\n#include <fn_80128204.h>\n'), [])
    check("rule7 exact: a header carries rule 7 like a source",
          r7_tokens("void fn_80041234(void);\n", "include/m/a.h"), ["fn_80041234"])
    check("rule7 exact: the detail is a function of the token alone (identity = rule, file, token)",
          len({(f["token"], f["detail"]) for f in lint_source(Source("x", "src/x.c",
              "void Helper_80147CE0(void);\nvoid g(void) {\n    Helper_80147CE0();\n}\n")) if f["rule"] == 7}), 1)
    _r7_base = lint_source(Source("x", "src/x.c", "void g(void) {\n    Helper_80147CE0();\n}\n"))
    _r7_after = lint_source(Source("x", "src/x.c", "void g(void) {\n    panel_helper();\n}\n"))
    check("rule7 exact: renaming an address-named token removes it and adds nothing",
          (added_identities(_r7_base, _r7_after), sorted(removed_identities(_r7_base, _r7_after))),
          ({}, [(7, "Helper_80147CE0", name_detail("Helper_80147CE0", "address"))]))
    check("rule7 exact: lib.names is the one verdict (generated / address / None)",
          [generated_name_kind(t) for t in ("fn_80041234__FPv", "Panel805482CC", "quest_flag_80000000_ck", "main")],
          ["generated", "address", None, None])

    # --- rule 7 file names (2026-10-05): one finding per generated path component ----------------------------
    def r7_paths(rel: str, origin=None) -> list:
        return [(f["token"], f["line"], f["detail"].split(" ")[1]) for f in
                lint_source(Source("x", rel, "int ok;\n", origin=origin)) if f["rule"] == 7]

    check("rule7 files: a generated unit stem is one finding on line 1", r7_paths("src/DWCi/fn_805113B0.cpp"),
          [("fn_805113B0", 1, "file")])
    check("rule7 files: a generated directory is one finding per file in it",
          r7_paths("include/fn_8004CAD8/psvec.h"), [("fn_8004CAD8", 1, "directory")])
    check("rule7 files: a generated directory and stem are two findings",
          r7_paths("include/fn_8004CAD8/fn_8004CAD8.h"), [("fn_8004CAD8", 1, "directory"), ("fn_8004CAD8", 1, "file")])
    check("rule7 files: an address-named stem counts", r7_paths("src/menu/Panel805482CC.cpp"),
          [("Panel805482CC", 1, "file")])
    check("rule7 files: a lbl_ header stem counts", r7_paths("include/lobby/lbl_806BE340.h"),
          [("lbl_806BE340", 1, "file")])
    check("rule7 files: a named path is clean", r7_paths("src/Network/network_state.cpp"), [])
    check("rule7 files: the path judged is the one the text really had (origin), filed under rel",
          [(f["file"], f["token"]) for f in lint_source(Source("x", "src/m/named.cpp", "int ok;\n",
                                                               origin="src/m/fn_80041234.cpp")) if f["rule"] == 7],
          [("src/m/named.cpp", "fn_80041234")])
    with tempfile.TemporaryDirectory() as tmp:
        def fgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def fput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def fref(branch: str) -> tuple:
            import contextlib as _ctx, io as _io  # noqa: PLC0415 - the selftest imports these further down
            out = _io.StringIO()
            with _ctx.redirect_stdout(out), _ctx.redirect_stderr(_io.StringIO()):
                rc = ref_comparison(tmp, branch, load_ownership(tmp), as_json=True)
            got = json.loads(out.getvalue())
            return rc, sorted((d["rule"], d["file"], d["token"]) for d in got["detail"])

        fgit("init", "-q")
        fgit("checkout", "-q", "-b", "main")
        body = "/* a body large enough for git to pair the rename */\n" + "".join(
            "int named_value_%d = %d;\n" % (i, i) for i in range(30))
        fput("config/RMHE08/symbols.txt", "owned_fn = .text:0x80002000; // type:function size:0x10\n")
        fput("config/RMHE08/splits.txt", "m/other.c:\n\t.text       start:0x80002000 end:0x80002010\n")
        fput("src/m/fn_80041234.cpp", body)
        fput("include/fn_8004CAD8/psvec.h", body)
        fgit("add", "-A")
        fgit("commit", "-q", "-m", "base")
        fgit("checkout", "-q", "-b", "named")
        fgit("mv", "src/m/fn_80041234.cpp", "src/m/menu_cursor.cpp")
        fgit("commit", "-q", "-m", "rename to a named stem")
        check("rule7 files: a git mv to a named stem is credited (no addition)", fref("named"), (0, []))
        fgit("checkout", "-q", "-b", "regen", "main")
        fgit("mv", "src/m/fn_80041234.cpp", "src/m/fn_80049999.cpp")
        fgit("commit", "-q", "-m", "rename to another generated stem")
        check("rule7 files: a git mv to another generated stem adds that name",
              fref("regen"), (1, [(7, "src/m/fn_80049999.cpp", "fn_80049999")]))
        fgit("checkout", "-q", "-b", "moved", "main")
        os.makedirs(os.path.join(tmp, "src", "fn_8004CAD8"), exist_ok=True)
        fgit("mv", "include/fn_8004CAD8/psvec.h", "src/fn_8004CAD8/psvec.h")
        fgit("commit", "-q", "-m", "move a header, keep its generated directory")
        check("rule7 files: a move that keeps the generated directory keeps the identity (no addition)",
              fref("moved"), (0, []))
        fgit("checkout", "-q", "-b", "newunit", "main")
        fput("src/m/fn_8004AAAA.cpp", "int named_value;\n")
        fgit("add", "-A")
        fgit("commit", "-q", "-m", "a new file with a generated stem")
        check("rule7 files: a new file with a generated stem is an addition",
              fref("newunit"), (1, [(7, "src/m/fn_8004AAAA.cpp", "fn_8004AAAA")]))

    # --- rule 7: the data-label half fires on every lbl_/loc_ reference ----------------------------
    lbl = Ownership({"lbl_80010000": [(".data", 0x80010000, "object")],
                     "loc_80010120": [(".data", 0x80010120, "object")],
                     "lbl_80020000": [(".data", 0x80020000, "object")],
                     "lbl_80030000": [(".data", 0x80030000, "object")]},
                    {".data": [(0x80010000, 0x80010200, "mod/a.c"),
                               (0x80020000, 0x80020100, "mod/b.c")]})
    own_lbl = "void f(void) {\n    u32 v = (u32)lbl_80010000;\n}\n"
    check("rule7 lbl: this unit's own lbl_ fires",
          lines_of(own_lbl, 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the finding names the label and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/mod/a.c", own_lbl), lbl) if f["rule"] == 7],
          ["data label `lbl_80010000` - name it from what it holds and where it is used, and rename the "
           "map row"])
    check("rule7 lbl: loc_ is the same unrenamed-data stem and fires",
          lines_of("void f(void) {\n    u32 v = (u32)loc_80010120;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the owning unit's header is covered too",
          lines_of(own_lbl, 7, "src/mod/a.h", lbl), [2])
    check("rule7 lbl: another unit's label fires in this file too (no own/foreign split)",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_80020000;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: the same label in a non-owner file fires",
          lines_of(own_lbl, 7, "src/other/c.c", lbl), [2])
    check("rule7 lbl: an unowned label fires",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_80030000;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: a label the map does not know fires",
          lines_of("void f(void) {\n    u32 v = (u32)lbl_DEADBEEF;\n}\n", 7, "src/mod/a.c", lbl), [2])
    check("rule7 lbl: a bodyless file's lbl_ declaration fires (key 2 is gone)",
          lines_of("extern u32 lbl_80010000;\n", 7, "src/mod/a.c", lbl), [1])
    check("rule7 lbl: no ownership index is needed for the lbl_ half",
          lines_of(own_lbl, 7, "src/mod/a.c", None), [2])
    check("rule7 lbl: the advertised name is counted as a distinct label",
          unique_names([f for f in lint_source(Source("x", "src/mod/a.c", own_lbl), lbl)
                        if f["rule"] == 7])["label_names"], 1)

    # --- rule 8: goto -----------------------------------------------------------------------------
    check("rule8: goto is a violation", lines_of("void f(void) {\n    goto out;\nout:\n    return;\n}\n", 8), [2])
    check("rule8: goto in a comment is clean",
          lines_of("/* the old shape was a goto dispatch */\nvoid f(void) {}\n", 8), [])
    check("rule8: goto in a string is clean", lines_of('const char* s = "goto";\n', 8), [])
    check("rule8: a plain label is not reported by this check", lines_of("void f(void) {\nout:\n    return;\n}\n", 8), [])
    check("rule8: a name containing goto is clean", lines_of("void f(void) {\n    u32 gotot = 1;\n}\n", 8), [])

    # --- rule 9: a mangled symbol is reached through its owner -----------------------------------
    check("rule9: a mangled call is a violation",
          lines_of("void f(void) {\n    get_now_areano__Fv();\n}\n", 9), [2])
    check("rule9: a namespaced mangled call is a violation",
          lines_of("void f(void) {\n    Panic__Q24nw4r2dbFPCciPCce(a, 1, b);\n}\n", 9), [2])
    check("rule9: a class-member mangling is a violation",
          lines_of("void f(void) {\n    move__6MHcharFUs(x, 0);\n}\n", 9), [2])
    check("rule9: an fn_XXXXXXXX call is not a mangling",
          lines_of("void f(void) {\n    fn_80040598();\n}\n", 9), [])
    check("rule9: a member call through its owner is clean",
          lines_of("void f(A* a) {\n    a->method(1);\n}\n", 9), [])
    check("rule9: a namespaced call through its owner is clean",
          lines_of("void f(void) {\n    ns::func(1);\n}\n", 9), [])
    check("rule9: a declaration is a finding too (row 50's other half)",
          lines_of("extern void get_now_areano__Fv(void);\n", 9), [1])
    check("rule9: an extern \"C\" mangled declaration is a finding",
          lines_of('extern "C" void Panic__Q24nw4r2dbFPCciPCce(const char*, int, const char*);\n', 9), [1])
    check("rule9: a mangled name in a comment is clean",
          lines_of("/* call get_now_areano__Fv() through the owner */\nvoid f(void) {}\n", 9), [])
    check("rule9: a mangled name in a string is clean",
          lines_of('const char* s = "get_now_areano__Fv";\n', 9), [])
    check("rule9: an ordinary name with underscores is clean",
          lines_of("void f(void) {\n    get_thing();\n}\n", 9), [])
    check("rule9: a call in an expression is a call",
          lines_of("void f(void) {\n    if (get_now_areano__Fv() == 1) {}\n}\n", 9), [2])
    check("rule9: a definition is a declaration",
          lines_of("void get_now_areano__Fv(void) {\n}\n", 9), [1])
    decl = "extern void get_now_areano__Fv(void);\n"
    call = "void f(void) {\n    get_now_areano__Fv();\n}\n"
    check("rule9: the classifier reads a declaration",
          looks_like_declaration(decl, decl.index("get_now_areano__Fv")), True)
    check("rule9: the classifier reads a call",
          looks_like_declaration(call, call.index("get_now_areano__Fv")), False)
    check("rule9: the classifier reads a return expression as a call",
          looks_like_declaration("return get_now_areano__Fv();\n",
                                 "return get_now_areano__Fv();".index("get_now")), False)

    # --- rule 1: a shared type is defined once (cross-file) ---------------------------------------
    def r1(*files: tuple[str, str]) -> list[tuple[int, str, int]]:
        sources = [Source("%s.c" % name, "src/%s.c" % name, text) for name, text in files]
        return [(f["rule"], f["file"], f["line"]) for f in rule1_findings(sources)]

    dup = "/* size: 0x8 */\nstruct Foo {\n    /* +0x00 */ u32 x;\n};\n"
    check("rule1: the same type in two files is a finding in the extra file",
          r1(("a", dup), ("b", dup)), [(1, "src/b.c", 2)])
    check("rule1: a type defined once is clean", r1(("a", dup)), [])
    check("rule1: three files give two findings, one per extra file",
          [f for _r, f, _l in r1(("a", dup), ("b", dup), ("c", dup))], ["src/b.c", "src/c.c"])
    check("rule1: the message names both files",
          [f["detail"] for f in rule1_findings([Source("x", "src/a.c", dup), Source("y", "src/b.c", dup)])],
          ["type `Foo` is defined in `src/a.c` and again in `src/b.c` - one definition, in the owner's header"])
    check("rule1: the owner is the lexicographically first file",
          [f["detail"] for f in rule1_findings([Source("x", "src/z.c", dup), Source("y", "src/a.c", dup)])],
          ["type `Foo` is defined in `src/a.c` and again in `src/z.c` - one definition, in the owner's header"])
    check("rule1: a union is a type too",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", "/* size: 0x8 */\nunion Foo {\n    /* +0x00 */ u32 x;\n};\n"),
              Source("y", "src/b.c", "/* size: 0x8 */\nunion Foo {\n    /* +0x00 */ u32 x;\n};\n")])],
          [1])
    check("rule1: a forward declaration is not a definition",
          r1(("a", "struct Foo;\n"), ("b", "struct Foo;\n")), [])
    check("rule1: an anonymous typedef gets the name after the brace",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", "typedef struct {\n    u32 x;\n} Foo;\n"),
              Source("y", "src/b.c", "typedef struct {\n    u32 x;\n} Foo;\n")])],
          [1])
    check("rule1: a header is never compared, wherever it lives (src/ since the 2026-10-05 move)",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", dup), Source("h", "src/a.h", dup), Source("i", "include/b.h", dup)])],
          [])
    check("rule1: two different types are not duplicates",
          [f["rule"] for f in rule1_findings([
              Source("x", "src/a.c", dup), Source("y", "src/b.c", dup.replace("Foo", "Bar"))])],
          [])

    # --- rule 2: an extern lives with the TU that owns it -----------------------------------------
    idx = Ownership({"foo": [(".text", 0x1000, "function")], "bar": [(".text", 0x3000, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2: an extern for another unit's symbol is a finding",
          lines_of("extern void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2: the same extern for a symbol the file owns is clean",
          lines_of("extern void foo(void);\n", 2, "src/mod/a.c", idx), [])
    check("rule2: the owner's header is clean too",
          lines_of("extern void foo(void);\n", 2, "src/mod/a.h", idx), [])
    check("rule2: an extern variable for another unit's symbol is a finding",
          lines_of("extern u16 bar[2];\n", 2, "src/other/c.c", idx), [1])
    check("rule2: a symbol not in the map is not a finding",
          lines_of("extern void not_in_map(void);\n", 2, "src/other/c.c", idx), [])
    check("rule2: the missing name is counted as a gap",
          idx.gaps["not in symbols.txt"], 1)
    mid = Ownership({"mid": [(".text", 0x2500, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2: an unsplit symbol with one bracketing module is a finding",
          lines_of("extern void mid(void);\n", 2, "src/other/c.c", mid), [1])
    check("rule2: the unsplit detail names the band by its module, never by a path (a band move keeps identities)",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void mid(void);\n"), mid)
           if f["rule"] == 2],
          ["`mid` has no registered owner - declare it in the `mod` unsplit band header"])
    gap = Ownership({"gap": [(".text", 0x2500, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "other/b.c")]})
    check("rule2: an unowned symbol whose brackets disagree is a finding (no module guessed)",
          lines_of("extern void gap(void);\n", 2, "src/other/c.c", gap), [1])
    check("rule2: the unresolved-band detail names no module and no path",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void gap(void);\n"), gap)
           if f["rule"] == 2],
          ["`gap` has no registered owner - declare it in an unsplit band header (the bracketing bands name "
           "different modules)"])
    check("rule2: no rule-2 detail spells a header path (identity survives the 2026-10-05 move)",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void mid(void);\n"), mid)
           + lint_source(Source("x", "src/other/c.c", "extern void gap(void);\n"), gap)
           if f["rule"] == 2 and ("include/" in f["detail"] or "unsplit/" in f["detail"])], [])
    check("rule2: the unresolved band is not counted as a gap", sum(gap.gaps.values()), 0)
    check("rule2: the unresolved band is reported under the sentinel module",
          sorted(gap.unsplit_modules), [UNSPLIT_UNRESOLVED])
    check("rule2: extern declarations are found by the scanner",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void a(void);\nextern u16 b[2];\nextern void (*c)(int);\n"))], ["a", "b", "c"])
    check("rule2: a function-pointer parameter is not the declared name",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void* f(s32 a, void (*cb)(void));\n"))], ["f"])
    check("rule2: a function returning a function pointer is named",
          [n for n, _p, _l in extern_declarations(Source("x", "x.c",
              "extern void (*f(int))(void);\n"))], ["f"])

    # --- rule 2 extended: plain prototypes in `src/` and foreign declarations in a module header -----
    # The `extern`-keyword-only scanner could not see a plain prototype (the Network scope's real sites),
    # and rule 2 never ran on a non-unsplit header at all.  Both are judged now.
    check("rule2 prototype: a plain prototype for another unit's symbol is a finding",
          lines_of("void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2 prototype: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2 prototype: a prototype for a symbol the file owns is clean",
          lines_of("void foo(void);\n", 2, "src/mod/a.c", idx), [])
    check("rule2 prototype: a prototype inside a body is not a file-scope declaration",
          lines_of("void f(void) {\n    void (*fp)(void);\n}\n", 2, "src/other/c.c", idx), [])
    check("rule2 prototype: a variable statement is not a prototype",
          lines_of("u32 g_var;\n", 2, "src/other/c.c", idx), [])
    check("rule2 prototype: `static` does not exempt the site",
          lines_of("static void foo(void);\n", 2, "src/other/c.c", idx), [1])
    check("rule2 prototype: the extern scanner and the prototype scanner never report one site twice",
          sorted(n for n, _p, _l in declaration_sites(Source("x", "x.c",
              "extern void foo(void);\n"))), ["foo"])

    # `_owns` must accept the owner's *public* header, or extending rule 2 to headers reports the owner's
    # own declarations - ~30 false rows in the Network scope alone (the review note).
    check("_owns: the unit's source", _owns("src/mod/a.c", "mod/a.c"), True)
    check("_owns: the unit's private header", _owns("src/mod/a.h", "mod/a.c"), True)
    check("_owns: the unit's public header", _owns("include/mod/a.h", "mod/a.c"), True)
    check("_owns: a header in a deeper include path", _owns("include/sub/mod/a.h", "mod/a.c"), True)
    check("_owns: another unit's header is not owned", _owns("include/other/a.h", "mod/a.c"), False)
    check("_owns: the proof from the review note",
          _owns("include/NHTTP/NHTTP_bgnend.h", "NHTTP/NHTTP_bgnend.c"), True)

    # an ordinary `include/<module>/*.h` header: a foreign declaration is a finding, the owner's own
    # header and an unowned symbol are not (the module header carries public names the map has not split).
    mhdr = "include/mod/user.h"
    check("rule2 header: another unit's owned symbol is a finding",
          lines_of("void foo(void);\n", 2, mhdr, idx), [1])
    check("rule2 header: the detail names the owner and the fix",
          [f["detail"] for f in lint_source(Source("x", mhdr, "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    # a leaf header `include/<module>/<symbol>.h`: owned by the one unit that defines every symbol it declares
    leaf = "include/mod/foo.h"
    two = Ownership({"foo": [(".text", 0x1000, "function")], "foo2": [(".text", 0x1100, "function")]},
                    {".text": [(0x1000, 0x2000, "mod/a.c")]})
    mixed = Ownership({"foo": [(".text", 0x1000, "function")], "mid": [(".text", 0x2500, "function")]},
                      {".text": [(0x1000, 0x2000, "mod/a.c")]})
    check("leaf header: one symbol of unit U is owned, no finding",
          lines_of("void foo(void);\n", 2, leaf, idx), [])
    check("leaf header: several symbols of the same unit are owned",
          lines_of("extern \"C\" {\nvoid foo(void);\nvoid foo2(void);\n}\n", 2, leaf, two), [])
    check("leaf header: a second unit's symbol makes it foreign (both are findings)",
          lines_of("void foo(void);\nvoid bar(void);\n", 2, leaf, idx), [1, 2])
    check("leaf header: an unowned symbol beside it makes it foreign",
          lines_of("void foo(void);\nvoid mid(void);\n", 2, leaf, mixed), [1])
    check("leaf header: not named for a declared symbol is not a leaf",
          lines_of("void foo(void);\n", 2, "include/mod/other.h", idx), [1])
    check("leaf header: the convention applies to a header anywhere (src/ too: headers move beside sources)",
          lines_of("void foo(void);\n", 2, "src/mod/foo.h", idx), [])
    check("leaf header: a band header is never a leaf",
          lines_of("void foo(void);\n", 2, UNSPLIT + "/foo.h", idx), [1])
    check("leaf header: a forward declaration after the guard and an #include keeps it a leaf",
          lines_of("#ifndef FOO_H\n#define FOO_H\n#include \"types.h\"\nstruct Bar;\nvoid foo(struct Bar* b);\n"
                   "#endif\n", 2, leaf, idx), [])
    check("... because a forward-declared type is no declaration, wherever the directives put it",
          [n for n, _l in header_declarations(Source("x", leaf, "#include \"types.h\"\nstruct Bar;\n"
                                                                 "typedef struct Baz Baz;\nvoid foo(Baz* b);\n"))],
          ["foo"])
    check("... and a second unit's symbol beside the forward declaration still makes it foreign",
          lines_of("#include \"types.h\"\nstruct Bar;\nvoid foo(void);\nvoid bar(void);\n", 2, leaf, idx), [3, 4])
    check("leaf header: path-stem ownership is unchanged",
          lines_of("void foo(void);\nvoid bar(void);\n", 2, "include/mod/a.h", idx), [2])
    check("rule2 header: the owner's own header is clean",
          lines_of("void foo(void);\n", 2, "include/mod/a.h", idx), [])
    check("rule2 header: an `extern` declaration is judged too",
          lines_of("extern u16 bar[2];\n", 2, mhdr, idx), [1])
    check("rule2 header: an unowned symbol is left to the band, not reported here",
          lines_of("void mid(void);\n", 2, mhdr, mid), [])
    check("rule2 header: a definition is not a declaration",
          lines_of("void foo(void) {\n}\n", 2, mhdr, idx), [])
    check("rule2 header: a type forward declaration is not a symbol declaration",
          lines_of("struct Vec;\n", 2, mhdr, idx), [])
    check("rule2 header: the body rules apply to a header too, so a fn_ prototype is rule 7",
          rules_of("void fn_80040598(void);\n", mhdr, idx), [(7, 1)])

    # --- rule 2 in the unsplit band: an owned symbol must not be declared there --------------------
    band = "include/unsplit/mod.h"
    check("rule2 band: another unit's owned symbol declared in the band is a finding",
          lines_of("void foo(void);\n", 2, band, idx), [1])
    check("rule2 band: the detail names the owner and the same fix",
          [f["detail"] for f in lint_source(Source("x", band, "void foo(void);\n"), idx)
           if f["rule"] == 2],
          ["`foo` is owned by `src/mod/a.c` - declare it in that unit's header and #include it"])
    check("rule2 band: the extern \"C\" linkage wrapper is transparent",
          lines_of('extern "C" {\nvoid foo(void);\n}\n', 2, band, idx), [2])
    check("rule2 band: a semicolon in a comment does not split a declaration",
          lines_of("/* a; b */\nvoid foo(void);\n", 2, band, idx), [2])
    check("rule2 band: an unowned symbol stays in the band",
          lines_of("void mid(void);\n", 2, band, mid), [])
    bandmid = Ownership({"mid": [(".text", 0x2500, "function")]},
                        {".text": [(0x1000, 0x2000, "mod/a.c"), (0x3000, 0x4000, "mod/b.c")]})
    check("rule2 band: an unowned name is not turned into a counted gap",
          (lines_of("void mid(void);\n", 2, band, bandmid), sum(bandmid.gaps.values()))[1], 0)
    check("rule2 band: a name not in the map stays in the band",
          lines_of("void not_in_map(void);\n", 2, band, idx), [])
    check("rule2 band: a definition is not a declaration",
          lines_of("void foo(void) {\n}\n", 2, band, idx), [])
    check("rule2 band: a type forward declaration is not a symbol declaration",
          lines_of("struct Vec;\n", 2, band, idx), [])
    check("rule2 band: the body rules apply to the band too, so a fn_ prototype is rule 7",
          rules_of("void fn_80040598(void);\n", band, idx), [(7, 1)])
    check("rule2 band: a plain prototype in a src/ file is now rule 2's too",
          lines_of("void foo(void);\n", 2, "src/other/c.c", idx), [1])

    # --- rule 12: an `extern` of data no registered range claims is the finding ---------------------
    # Owner's ruling 2026-09-28: the unit that reads/writes the bytes claims the range and matches it, so
    # the declaration is the finding - in a `src/` file, a module header, or the unsplit band.
    dat = Ownership({"lbl_8079C7D8": [(".sdata2", 0x8079C7D8, "object")],
                     "maskedUserName": [(".sdata", 0x80793968, "object")],
                     "lbl_805FA908": [(".data", 0x805FA908, "object")],
                     "fn_80010000": [(".text", 0x80010000, "function")],
                     "owned_data": [(".data", 0x1500, "object")]},
                    {".data": [(0x1000, 0x2000, "mod/a.c")],
                     ".sdata2": [(0x80800000, 0x80800100, "mod/a.c")]})
    check("rule12: an extern of unowned data is a finding",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "src/other/c.c", dat), [1])
    check("rule12: the detail names the address the claim covers",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c",
              "extern const u16 lbl_8079C7D8;\n"), dat) if f["rule"] == 12],
          ["`lbl_8079C7D8` is unowned data - no registered range covers `.sdata2:0x8079C7D8`; the unit that "
           "uses it claims the range in its own `splits.txt` and matches the bytes (rule 12)"])
    check("rule12: an extern of OWNED data is clean (the declare-never-define carve-out)",
          lines_of("extern u32 owned_data;\n", 12, "src/other/c.c", dat), [])
    check("rule12: a function declaration is rule 2's, never rule 12's",
          lines_of("extern void fn_80010000(void);\n", 12, "src/other/c.c", dat), [])
    check("rule12: an array of unowned data is a finding too",
          lines_of("extern const char maskedUserName[7];\n", 12, "src/other/c.c", dat), [1])
    check("rule12: a module header is judged (the unowned data is declared there)",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "include/mod/user.h", dat), [1])
    check("rule12: the unsplit band is judged too - the band is not the answer for data a unit uses",
          lines_of("extern const u16 lbl_8079C7D8;\n", 12, "include/unsplit/mod.h", dat), [1])
    check("rule12: a name not in the map is not a finding",
          lines_of("extern u32 not_in_map;\n", 12, "src/other/c.c", dat), [])
    check("rule12: a definition is not a declaration",
          lines_of("const u16 lbl_8079C7D8 = 1;\n", 12, "src/other/c.c", dat), [])
    check("rule12: no map means unchecked, not a crash",
          [f for f in lint_source(Source("x", "src/other/c.c",
                                         "extern const u16 lbl_8079C7D8;\n"), None)
           if f["rule"] == 12], [])
    check("rule12: an unsplit FUNCTION is rule 2's, not rule 12's",
          lines_of("extern void mid(void);\n", 12, "src/other/c.c", mid), [])
    check("rule12 and rule 2 both name an unowned data extern (different remedies)",
          sorted(r for r, _l in rules_of("extern const u16 lbl_8079C7D8;\n", "src/other/c.c", dat)),
          [2, 7, 12])

    # the real map: the ownership lookup resolves a symbol we name, from the tree's own data
    if os.path.exists(os.path.join(".", "config", "RMHE08", "symbols.txt")):
        real = load_ownership(".")
        r = real.resolve("em_act_ck__FP11_ENEMY_WORKUcUc") if real else None
        check("rule2: the real map resolves a named symbol to its owner unit",
              (r or {}).get("kind"), "owned")
        # the owner moves whenever the splits program folds or renames the unit (it was enemy/fn_8012BDF4.cpp
        # before the em_common fold), so pin only the module directory and the language
        own = str((r or {}).get("unit") or "")
        check("rule2: the real map names an enemy unit that owns it",
              own.startswith("enemy/") and own.endswith(".cpp"), True)

    # --- budget aggregation -----------------------------------------------------------------------
    text = ("/* size: 0x8 */\nstruct A {\n    u32 unk00;\n};\n"
            "void f(u8* p) {\n    *(u32*)((u8*)p + 0x4) = 1;\n}\n")
    f1 = lint_source(Source("a.c", "src/A/a.c", text))
    f2 = lint_source(Source("b.c", "src/B/b.c", "void fn_80040598(void) {}\n"))
    b = budget(f1 + f2)
    check("budget: two units", [row["file"] for row in b["units"]], ["src/A/a.c", "src/B/b.c"])
    check("budget: unit row total", b["units"][0]["total"], 3)
    check("budget: rule 5 counted once", b["totals"]["5"], 1)
    check("budget: rule 7 counted once", b["totals"]["7"], 1)
    check("budget: empty rule is zero", b["totals"]["8"], 0)
    check("budget: overall total", b["total"], 4)
    check("budget: units are sorted", b["units"] == sorted(b["units"], key=lambda r: r["file"]), True)
    check("budget: findings count", b["findings"], 4)
    check("budget: unique fn names", b["unique"]["fn_names"], 1)
    check("budget: unique unk fields", b["unique"]["unk_fields"], 1)
    check("budget: unique types", b["unique"]["types"], 0)
    check("budget: unique shared types", b["unique"]["shared_types"], 0)
    check("budget: unique extern symbols", b["unique"]["extern_symbols"], 0)
    check("budget: unique data labels", b["unique"]["label_names"], 0)
    check("budget: unique mangled names", b["unique"]["mangled_names"], 0)
    check("unique: repeated sites collapse", unique_names([
        {"rule": 5, "detail": "field `unk1`"}, {"rule": 5, "detail": "field `unk1`"},
        {"rule": 7, "detail": "bare `unk1` identifier"},
    ])["unk_fields"], 1)

    # --- the band's rule-2 column (`--budget --headers`) -------------------------------------------
    # `lint_all`/`header_rule2_findings` skip `include/unsplit/`, so a band header's owned-declaration
    # findings are in no budget column; `header_rule2_band_findings` is that missing column's source.
    band_src = Source("x", "include/unsplit/mod.h", "void foo(void);\nvoid mid(void);\n")
    band_findings = band_rule2_findings([band_src], idx)
    check("budget band: an owned declaration in the band is a finding",
          [f["rule"] for f in band_findings], [2])
    check("budget band: an unowned name is left alone",
          [f["line"] for f in band_findings], [1])
    check("budget band: a symbol not in the map is left alone",
          band_rule2_findings([Source("x", "include/unsplit/mod.h", "void not_in_map(void);\n")], idx), [])
    shown = budget(band_findings)
    check("budget band: the band header is its own row",
          [r["file"] for r in shown["units"]], ["include/unsplit/mod.h"])
    check("budget band: the r2 column carries the band reading", shown["totals"]["2"], 1)
    check("budget band: without --headers nothing changes",
          [r["file"] for r in budget(f1 + f2)["units"]], ["src/A/a.c", "src/B/b.c"])

    # --- diff comparison --------------------------------------------------------------------------
    before = rule_counts(f1)
    after = rule_counts(f1 + f2)
    d = diff_deltas(before, after)
    check("diff: one rising pair", [(x["rule"], x["file"], x["added"]) for x in d], [(7, "src/B/b.c", 1)])
    check("diff: an unchanged tree is clean", diff_deltas(before, before), [])
    check("diff: a falling count is not an addition",
          diff_deltas({(5, "x"): 3}, {(5, "x"): 1}), [])
    check("diff: a fixed rule still fails for a different one",
          [(x["rule"]) for x in diff_deltas({(5, "x"): 1, (8, "x"): 0}, {(5, "x"): 1, (8, "x"): 1})], [8])
    # the grandfather, proved on a *header* rule-2 count (the behaviour that had to survive the extension):
    # an existing finding never blocks, an added one refuses.
    hdr_before = {(2, "include/mod/a.h"): 2}
    check("diff grandfather: a header's pre-existing rule-2 findings are not additions when untouched",
          diff_deltas(hdr_before, hdr_before), [])
    check("... even when the file is rewritten but the count does not rise",
          diff_deltas(hdr_before, {(2, "include/mod/a.h"): 2}), [])
    check("... and one added rule-2 finding in that header refuses",
          [(x["rule"], x["file"], x["added"]) for x in
           diff_deltas(hdr_before, {(2, "include/mod/a.h"): 3})], [(2, "include/mod/a.h", 1)])

    # --- `--list-added`: which occurrence, not only how many (2026-09-28) -------------------------
    # A UI lane read "+76 rule 7" and had to drop three bodies to learn which of its renames were
    # load-bearing.  The count delta is (rule, file); the detail names the occurrences behind it, excluding
    # the ones the file already carried.  The match is line-independent, so bodies added above a
    # pre-existing finding must not re-list it.
    def _af(rule: int, file: str, line: int, token: str, detail: str = "d") -> dict:
        return {"rule": rule, "file": file, "line": line, "token": token, "detail": detail}
    bfa = [_af(7, "src/Pl/pl_act.cpp", 1, "fn_80040598"),
           _af(11, "src/Pl/pl_act.cpp", 2, "base_fn"),
           _af(12, "src/Pl/pl_act.cpp", 3, "old_data")]
    afa = [_af(7, "src/Pl/pl_act.cpp", 10, "fn_80040598"),   # shifted down by the new body, still old
           _af(7, "src/Pl/pl_act.cpp", 20, "fn_80275B04"),   # added
           _af(11, "src/Pl/pl_act.cpp", 30, "base_fn"),
           _af(11, "src/Pl/pl_act.cpp", 40, "new_fn"),       # added
           _af(12, "src/Pl/pl_act.cpp", 50, "old_data"),
           _af(12, "src/Pl/pl_act.cpp", 60, "new_data")]     # added
    detail = added_finding_detail(diff_deltas(rule_counts(bfa), rule_counts(afa)), afa, bfa)
    check("--list-added names exactly the three added occurrences",
          [(d["rule"], d["line"], d["token"]) for d in detail],
          [(7, 20, "fn_80275B04"), (11, 40, "new_fn"), (12, 60, "new_data")])
    check("... and a shifted pre-existing finding is not re-listed",
          [d["token"] for d in detail if d["token"] in ("fn_80040598", "base_fn", "old_data")], [])
    check("... and an unchanged count names nothing",
          added_finding_detail(diff_deltas(rule_counts(afa), rule_counts(afa)), afa, afa), [])
    # ordering: grouped by rule, then file with its count, biggest offender first (backlog.py's weight).
    many = [_af(7, "src/A/a.cpp", 1, "fn_00000001"), _af(7, "src/A/a.cpp", 2, "fn_00000002"),
            _af(7, "src/B/b.cpp", 3, "fn_00000003")]
    lines = added_detail_lines(added_finding_detail(diff_deltas({}, rule_counts(many)), many, []))
    check("--list-added groups by rule then file, biggest file first",
          [ln for ln in lines if ln.startswith("rule") or ln.startswith("  src")],
          ["rule 7:", "  src/A/a.cpp (2)", "  src/B/b.cpp (1)"])
    check("... with one `rule R <file>:<line> <token>` line per occurrence",
          [ln for ln in lines if ln.startswith("    rule ")],
          ["    rule 7 src/A/a.cpp:1 fn_00000001", "    rule 7 src/A/a.cpp:2 fn_00000002",
           "    rule 7 src/B/b.cpp:3 fn_00000003"])

    # --- `--diff`: the OWNERSHIP OF THE ADDRESS, not the spelling of the name -----------------------
    # The 2026-09-28 incident.  `cb7d49aaa` renamed four map rows and landed the map alone; completing the
    # rename in the referrers then read as "+4 added rule-2 violations", because the base copy's *old*
    # spelling resolves to nothing at all while the corrected one resolves (owned or unsplit).  A credit is
    # granted only for the other half of a rename the **base map already made**, and three conditions bound
    # it: the name is new to the file, the address was owned at base for the same owner (or the same unsplit
    # module), and the file gave up an unmapped name to pay for it.

    ADDR = 0x803D6A98
    site = "include/Network/fn_8041A87C.h"
    finding = {"rule": 2, "file": site, "line": 419,
               "text": "void* GameSpyInterfaceThread_getInstance(void);", "detail": "(detail)",
               "symbol": "GameSpyInterfaceThread_getInstance"}
    new_map = Ownership({"GameSpyInterfaceThread_getInstance": [(".text", ADDR, "function")]},
                        {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    old_map = Ownership({"getGameSpyInterfaceThread": [(".text", ADDR, "function")]},
                        {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    no_row = Ownership({}, {".text": [(0x803D0000, 0x803D8000, "Network/fn_803D3CE8.cpp")]})
    not_yet_registered = Ownership({"GameSpyInterfaceThread_getInstance": [(".text", ADDR, "function")]},
                                   {})
    someone_else = Ownership({"patched_at_this_address": [(".text", ADDR, "function")]},
                             {".text": [(0x803D0000, 0x803D8000, "Network/other.cpp")]})
    check("address view: the map resolves an address to the row's owner, by address",
          new_map.resolution_at(".text", ADDR)["unit"], "Network/fn_803D3CE8.cpp")
    check("... and answers None for an address it carries no row at",
          new_map.resolution_at(".text", 0x803D7000), None)

    # (a) completing an owed rename -> PASS
    check("rename credit: a completion resolves to an address the base map already owned",
          owed_rename_completion(finding, new_map, new_map), True)
    check("... and the base map's row NAME is irrelevant - the address is the identity",
          owed_rename_completion(finding, old_map, new_map), True)
    added_row = {"rule": 2, "file": site, "added": 1, "before": 3, "after": 4}
    check("... so the addition is credited",
          apply_rename_credits([added_row], [finding], new_map, new_map, {site: {"DWCi_htons"}}, {site: 1}),
          ([], {(2, site): 1}))
    check("... and the same holds when the map rename rode the same batch",
          apply_rename_credits([added_row], [finding], old_map, new_map, {site: {"DWCi_htons"}}, {site: 1}),
          ([], {(2, site): 1}))
    check("... reported, never silent",
          rename_credit_lines({(2, site): 1}, {site: {"getGameSpyInterfaceThread"}}),
          ["  ~1 rule 2  %s  (completing a rename the base map already made: same address, same owner at "
           "base; stopped spelling getGameSpyInterfaceThread)" % site])

    # (b) a genuinely new foreign declaration at an address UNOWNED at base -> still REFUSE
    check("rename credit: no row at that address in the base map is not a completion",
          owed_rename_completion(finding, no_row, new_map), False)
    check("... nor is a row whose range the base had not registered (unsplit there)",
          owed_rename_completion(finding, not_yet_registered, new_map), False)
    check("... so the batch still adds a violation",
          apply_rename_credits([added_row], [finding], no_row, new_map, {site: {"DWCi_htons"}},
                               {site: 1}),
          ([added_row], {}))

    # (c) the same address owned at base by a DIFFERENT owner -> still REFUSE
    check("rename credit: an address the base map owned for somebody else is not a completion",
          owed_rename_completion(finding, someone_else, new_map), False)
    check("... so the batch still adds a violation",
          apply_rename_credits([added_row], [finding], someone_else, new_map, {site: {"DWCi_htons"}},
                               {site: 1}),
          ([added_row], {}))

    # (d) both spellings were map rows at each side: the count does not rise, and nothing is credited
    check("rename credit: a count that does not rise is untouched",
          diff_deltas({(2, site): 1}, {(2, site): 1}), [])
    check("... and a credit with nothing to subtract is not reported",
          apply_rename_credits([], [finding], old_map, new_map, {}, {site: 1}), ([], {}))

    # the three bounds, each of which a genuinely new declaration fails
    check("rename credit: a symbol the base copy of the file already declared is never credited",
          rename_credits([finding], old_map, new_map, {site: {finding["symbol"]}}, {site: 1}), {})
    check("... nor is one when the file gave up no unmapped name",
          rename_credits([finding], old_map, new_map, {}, {site: 0}), {})
    two = [finding, dict(finding, symbol="PatInterface_clear")]
    check("... and each credit costs one freed gap",
          rename_credits(two, old_map, new_map, {}, {site: 1}), {(2, site): 1})
    check("... an unrelated rule is never credited",
          apply_rename_credits([{"rule": 7, "file": site, "added": 1, "before": 0, "after": 1}],
                               [dict(finding, rule=7)], old_map, new_map, {}, {site: 1}),
          ([{"rule": 7, "file": site, "added": 1, "before": 0, "after": 1}], {}))
    check("... and a file that also adds a foreign declaration still refuses, by name",
          apply_rename_credits([dict(added_row, added=2, after=5)], [finding], old_map, new_map,
                               {site: {"DWCi_htons"}}, {site: 1}),
          ([dict(added_row, added=1, after=5)], {(2, site): 1}))
    # the gap a credit is paid with is a name the map cannot resolve, read from the file itself
    check("rename credit: an unmapped declaration is a gap",
          unresolved_declarations(Source("x", site, "void getGameSpyInterfaceThread(void);\n"), new_map),
          {"getGameSpyInterfaceThread"})
    check("... and a resolved one is not",
          unresolved_declarations(Source("x", site, "void GameSpyInterfaceThread_getInstance(void);\n"),
                                  new_map), set())
    check("... so completing the rename frees exactly one",
          len(unresolved_declarations(Source("x", site, "void getGameSpyInterfaceThread(void);\n"), new_map)
              - unresolved_declarations(Source("x", site,
                                               "void GameSpyInterfaceThread_getInstance(void);\n"),
                                        new_map)), 1)

    # --- rule 14 (was 10 until 2026-10-05): a codegen pragma belongs to a TU, not to a shared header -
    hdr = "include/stage/fn_802B2AA0.h"

    def pragmas_of(text: str, rel: str = hdr) -> list[int]:
        return [f["line"] for f in codegen_pragma_findings(Source("x", rel, text))]

    check("rule14: peephole in a shared header is a finding", pragmas_of("#pragma peephole off\n"), [1])
    check("rule14: optimization_level in a shared header is a finding",
          pragmas_of("#pragma optimization_level 2\n"), [1])
    check("rule14: fp_contract in a shared header is a finding",
          pragmas_of("#pragma fp_contract on\n"), [1])
    check("rule14: an indented pragma is still found", pragmas_of("    #pragma peephole off\n"), [1])
    check("rule14: a pragma in a .cpp is not reported",
          codegen_pragma_findings(Source("x", "src/enemy/em019_prog.cpp", "#pragma peephole off\n")), [])
    check("rule14: a pragma in a .c is not reported",
          codegen_pragma_findings(Source("x", "src/foo.c", "#pragma peephole off\n")), [])
    check("rule14: a commented pragma is clean", pragmas_of("/* #pragma peephole off */\n"), [])
    check("rule14: a pragma named in a line comment is clean",
          pragmas_of("// #pragma peephole off\n"), [])
    check("rule14: `#pragma once` is not a codegen pragma", pragmas_of("#pragma once\n"), [])
    check("rule14: `#pragma pack` is not a codegen pragma", pragmas_of("#pragma pack(4)\n"), [])
    check("rule14: the finding names the pragma and the fix",
          [f["detail"] for f in codegen_pragma_findings(Source("x", hdr, "#pragma peephole off\n"))],
          ["codegen pragma `#pragma peephole` in a shared header - state it in the `.c`/`.cpp` that "
           "needs it, never in the header"])
    check("rule14: the report is a rule-14 finding (plan 6.5 rule 10 is vtableaudit's)",
          [f["rule"] for f in codegen_pragma_findings(Source("x", hdr, "#pragma peephole off\n"))], [14])
    if os.path.exists(hdr):
        check("rule14: the motivating header is clean after the fix",
              codegen_pragma_findings(Source(hdr, hdr, read_text(hdr))), [])

    # --- rule 15 (2026-10-05): comment hygiene - stale paths and function-comment addresses refuse ---------
    def r15_of(text: str, rel: str = "src/Net/net_a.c", ownership=None) -> list[tuple]:
        return [(f["check"], f["token"], f["line"], f["advisory"])
                for f in lint_source(Source("x", rel, text), ownership) if f["rule"] == 15]

    set_rule15_context(None)
    check("rule15: a retired include/ path in a comment is a refusing finding, the path its token",
          r15_of("/* see include/Net/net.h */\nint a;\n"), [("stale-path", "include/Net/net.h", 1, False)])
    check("rule15: each stale class has its token: .pi/, proposal/, auto/<hex>_, a retired tool named bare",
          sorted(t for c, t, _l, _a in r15_of("/* .pi/notes/x.md, proposal/80001000.cpp,\n"
                                                " * auto/80001000_net.cpp and mergelane */\nint a;\n")),
          [".pi/notes/x.md", "auto/80001000_net.cpp", "mergelane", "proposal/80001000.cpp"])
    check("rule15: code, an `#include` and a string literal are not comment text",
          r15_of('#include "include/x.h"\nconst char* s = "proposal/x.cpp";\nint include_x;\n'), [])
    check("rule15: src/Camellia/ is the vendor's and never read",
          r15_of("/* include/x.h 2026-10-05 */\nint a;\n", rel="src/Camellia/camellia.c"), [])
    with tempfile.TemporaryDirectory() as tmp15:
        for rel in ("tools/units/herdr/README", ".pi/notes/x.md", "src/Net/net.h"):
            os.makedirs(os.path.dirname(os.path.join(tmp15, rel)), exist_ok=True)
            open(os.path.join(tmp15, rel), "w").close()
        retired_live = "/* tools/units/herdr/README */\nint a;\n"
        check("rule15: without a tree a retired tool's path is stale", len(r15_of(retired_live)), 1)
        set_rule15_context(tmp15)
        check("rule15: ... and a path the tree has is live (whitelisted)", r15_of(retired_live), [])
        check("rule15: `.pi/` is never live, even where the scratch exists (it differs by checkout)",
              [c for c, *_ in r15_of("/* .pi/notes/x.md */\nint a;\n")], ["stale-path"])
        check("rule15: a moved include/ path carries the live spelling as its remedy, not in its identity",
              [(f["token"], f.get("remedy")) for f in lint_source(Source("x", "src/a.c", "/* include/Net/net.h */\n"))
               if f["rule"] == 15], [("include/Net/net.h", "the live path is `Net/net.h`")])
        set_rule15_context(None)

    fmap = Ownership({"net_foo": [(".text", 0x80001000, "function")], "net_bar": [(".text", 0x80002000, "function")]},
                     {".text": [(0x80001000, 0x80003000, "Net/net_a.c")]},
                     functions={0x80001000: [("net_foo", 0x40)], 0x80002000: [("net_bar", 0x20)]})

    def addr_of(comment: str, name: str = "net_foo", ownership=fmap) -> list[str]:
        text = "%s\nvoid %s(void) {\n}\n" % (comment, name)
        return [f["detail"] for f in lint_source(Source("x", "src/Net/net_a.c", text), ownership)
                if f["rule"] == 15 and f["check"] == "address"]

    check("rule15: the canonical prefix naming the function's address and size is clean",
          addr_of("/* 0x80001000 (0x40): reset the pool */"), [])
    check("rule15: a wrong size is a finding naming the symbol's size",
          addr_of("/* 0x80001000 (0x44): reset the pool */"),
          ["function comment states 0x80001000 (0x44) above `net_foo`: the symbol's size is 0x40"])
    check("rule15: another symbol's address is a finding naming where the map has the function",
          addr_of("/* 0x80002000 (0x20): reset the pool */"),
          ["function comment states 0x80002000 (0x20) above `net_foo`, which the map has at 0x80001000"])
    check("rule15: an address no function starts at is a finding",
          addr_of("/* 0x80001010: reset the pool */"),
          ["function comment states 0x80001010 above `net_foo`: no function symbol starts there"])
    check("rule15: the legacy ` - ` separator is read too (no size: only the address is judged)",
          (addr_of("/* 0x80001000 - reset */"), len(addr_of("/* 0x80001010 - reset */"))), ([], 1))
    check("rule15: a decimal size is read", addr_of("/* 0x80001000 (64): reset */"), [])
    check("rule15: a comment separated by a blank line is not the function's",
          addr_of("/* 0x80001010 (0x44): data */\n"), [])
    check("rule15: a range or an address list is not the prefix",
          (addr_of("/* 0x80001010..0x80001020: data */"), addr_of("/* 0x80001010/0x80001020 - data */")), ([], []))
    check("rule15: a name the map lacks (a ctor, a source-side name) is judged by the row at the address",
          (addr_of("/* 0x80001000 (0x40): build */", "NetPool"), len(addr_of("/* 0x80001000 (0x4): x */", "NetPool"))),
          ([], 1))
    check("rule15: without the map (or a map built without its rows) the address is not judged",
          (addr_of("/* 0x80001010: x */", ownership=None),
           addr_of("/* 0x80001010: x */", ownership=Ownership(fmap.symbols, fmap.ranges))), ([], []))

    adv = r15_of("/* 2026-10-05: the network lane, round 3; 92.5 % fuzzy match; a 30 % chance */\n"
                 "/* 0x80001000 (0x40): net_foo_reset clears the pool */\nvoid net_foo_reset(void) {\n}\n"
                 "/* 0x80002000: init */\nvoid init(void) {\n}\n")
    check("rule15: the advisory classes - date, lane, round N, a percentage beside a scoring word, a self-name",
          sorted(c for c, _t, _l, a in adv if a), ["date", "lane", "percent", "round N", "self-name"])
    check("rule15: ... every one advisory, none refusing", [c for c, _t, _l, a in adv if not a], [])
    check("rule15: a game probability is not a percentage finding; a short generic name is not a self-name",
          [t for c, t, _l, _a in adv if c in ("percent", "self-name")], ["92.5 % fuzzy", "net_foo_reset"])
    check("rule15: the budget splits the column by class",
          budget([f for f in lint_source(Source("x", "src/Net/net_a.c",
                                                "/* .pi/x.md 2026-10-05 */\nint a;\n"))])["rule15"],
          {"refusing": {"stale-path": 1, "address": 0},
           "advisory": dict({c: 0 for c in ADVISORY}, date=1)})

    # the add-only comparison: a stale path new to a file refuses; an advisory addition and a reflowed old path pass
    with tempfile.TemporaryDirectory() as tmp15:
        def g15(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp15, capture_output=True, check=True)

        def put15(rel: str, text: str) -> None:
            p = os.path.join(tmp15, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def diff15() -> tuple:
            old = os.getcwd()
            os.chdir(tmp15)
            try:
                out = io.StringIO()
                with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                    rc = main(["--diff", "HEAD", "--json"])
            finally:
                os.chdir(old)
            data = json.loads(out.getvalue())
            return rc, [(a["rule"], a["file"], a["added"]) for a in data["added"]], \
                [(d["rule"], d["token"]) for d in data["detail"]]

        import contextlib
        import io
        g15("init", "-q")
        put15("config/RMHE08/symbols.txt", "net_a = .text:0x80001000; // type:function size:0x10\n")
        put15("config/RMHE08/splits.txt", "net/net_a.c:\n\t.text       start:0x80001000 end:0x80001010\n")
        put15("src/net/net_a.c", "/* the pool, see .pi/notes/old.md */\nint a;\n")
        g15("add", "-A")
        g15("commit", "-q", "-m", "base")
        put15("src/net/net_a.c", "/* the pool, see .pi/notes/old.md and proposal/80001000.cpp */\nint a;\n")
        check("rule15 --diff: a stale path new to a file refuses, the old one stays grandfathered",
              diff15(), (1, [(15, "src/net/net_a.c", 1)], [(15, "proposal/80001000.cpp")]))
        put15("src/net/net_a.c", "/* 2026-10-05: the lane's wave 2 (92 % match).\n"
                                 " * the pool, see .pi/notes/old.md */\nint a;\n")
        check("rule15 --diff: advisory additions and a reflowed old path pass (no diff effect)",
              diff15(), (0, [], []))
        set_rule15_context(None)

    # --- rule 11: no `void *` parameter or return type -------------------------------------------
    check("rule11: an unmarked void* parameter is a finding",
          lines_of("void f(void *p) {\n}\n", 11), [1])
    check("rule11: an unmarked void* return type is a finding",
          lines_of("void *f(void) {\n}\n", 11), [1])
    check("rule11: a prototype is judged too", lines_of("void f(void *p);\n", 11), [1])
    check("rule11: a `void **` return is still a void pointer",
          lines_of("void **f(void) {\n}\n", 11), [1])
    check("rule11: `const void *` is a void pointer",
          lines_of("void f(const void *src) {\n}\n", 11), [1])
    check("rule11: two void* parameters are two findings",
          lines_of("void f(void *a, void *b) {\n}\n", 11), [1, 1])
    check("rule11: a parameter and a return are both reported",
          lines_of("void *f(void *p) {\n}\n", 11), [1, 1])
    check("rule11: void alone is clean", lines_of("void f(void) {\n}\n", 11), [])
    check("rule11: a real parameter type is clean",
          lines_of("void f(Vec *out, u32 n) {\n}\n", 11), [])
    check("rule11: a function pointer parameter with a clean signature is clean",
          lines_of("void f(void (*cb)(int)) {\n}\n", 11), [])
    check("rule11: a `void*` local's parenthesised cast is not read as a type",
          lines_of("void f(void) {\n    u32 v = (u32)(void *)p;\n}\n", 11), [])
    check("rule11: a file-scope initializer cast is clean",
          lines_of("void *p = (void *)0;\n", 11), [])
    check("rule11: a parameter on a continuation line is reported on its own line",
          lines_of("void f(s32 a, void *first,\n       void *second);\n", 11), [1, 2])
    check("rule11: a function returning a function pointer is judged",
          lines_of("void (*f(void *self, int n))(void);\n", 11), [1])
    check("rule11: a file-scope void* variable is not a parameter/return",
          lines_of("void *g_buffer;\n", 11), [])
    check("rule11: a static_assert operand is not a parameter list",
          lines_of("static_assert(sizeof(void *) == 4);\n", 11), [])
    check("rule11: a macro body is not a declaration",
          lines_of("#define PTR ((void *)0)\nvoid f(void) {\n}\n", 11), [])
    check("rule11: a call-shaped control block is not a declaration",
          lines_of("void f(void) {\n    while (memcmp(p, (void *)q, 4)) {\n    }\n}\n", 11), [])
    check("rule11: a namespace-scoped definition is judged",
          lines_of("namespace nw4r {\nvoid f(void *p) {\n}\n}\n", 11), [2])
    check("rule11: a class-scoped inline method is judged",
          lines_of("class A {\npublic:\n    void f(void *p) {\n    }\n};\n", 11), [3])

    # the marker: per-declaration, on the declaration or the line above it
    check("rule11 marker: on the line above exempts the declaration",
          lines_of("/* untyped: opaque handle */\nvoid f(void *h) {\n}\n", 11), [])
    check("rule11 marker: trailing on the same line exempts it",
          lines_of("void f(void *h); /* untyped: opaque handle */\n", 11), [])
    check("rule11 marker: a memcpy-shaped byte range is an accepted reason",
          lines_of("/* untyped: memcpy-shaped byte range */\nvoid f(void *dst) {\n}\n", 11), [])
    check("rule11 marker: a caller-owned payload is an accepted reason",
          lines_of("/* untyped: caller-owned payload */\nvoid f(void *data) {\n}\n", 11), [])
    check("rule11 marker: an empty reason is still a finding",
          lines_of("/* untyped: */\nvoid f(void *p) {\n}\n", 11), [2])
    check("rule11 marker: a vague reason is still a finding",
          lines_of("/* untyped: TODO, it is untyped */\nvoid f(void *p) {\n}\n", 11), [2])
    check("rule11 marker: a marker in a string is not a marker",
          lines_of('const char* s = "untyped: opaque handle";\nvoid f(void *p) {\n}\n', 11), [2])
    check("rule11 marker: a marker on an unrelated declaration does not leak",
          lines_of("/* untyped: opaque handle */\nvoid g(void *h);\n\nvoid f(void *p) {\n}\n", 11), [4])
    check("rule11 marker: a trailing marker on the previous declaration does not exempt the next",
          lines_of("void g(void *h); /* untyped: opaque handle */\nvoid f(void *p);\n", 11), [2])
    check("rule11 marker: each declaration needs its own marker",
          lines_of("/* untyped: opaque handle */\nvoid g(void *h);\n"
                   "/* untyped: opaque handle */\nvoid f(void *p);\n", 11), [])

    # the scope note: a `void *` local is out of scope, only counted
    check("rule11 locals: a local void* is not a finding but is counted",
          (lines_of("void f(void) {\n    void *p = 0;\n}\n", 11),
           rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    void *p = 0;\n}\n"))), ([], 1))
    check("rule11 locals: a cast is not counted",
          rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    u32 v = (u32)(void *)p;\n}\n")), 0)
    check("rule11 locals: a parameter is not counted",
          rule11_local_count(Source("x.c", "x.c", "void f(void *p) {\n}\n")), 0)
    check("rule11 locals: a nested block is not counted twice",
          rule11_local_count(Source("x.c", "x.c", "void f(void) {\n    if (1) {\n        void *p = 0;\n    }\n}\n")), 1)

    # --- rule 13: a method is a member -------------------------------------------------------------
    set_rule13_context(None)
    CLS = "struct Tcp {\n    int a;\n};\n\n"

    def r13(text: str, rel: str = "x.cpp") -> list[int]:
        return [f["line"] for f in lint_source(Source("x.cpp", rel, text), None) if f["rule"] == 13]

    check("rule13: `Type_name(Type* self)` is a finding",
          r13(CLS + "s32 Tcp_send(Tcp* self, const u8* d, s32 n) {\n    return 0;\n}\n"), [5])
    check("rule13: a prototype is judged too", r13(CLS + "s32 Tcp_send(Tcp* self);\n"), [5])
    check("rule13: `const Type*` is a finding", r13(CLS + "int Tcp_get(const Tcp* self);\n"), [5])
    check("rule13: `Type&` is a finding", r13(CLS + "int Tcp_get(Tcp& self);\n"), [5])
    check("rule13: an unnamed self is a finding", r13(CLS + "int Tcp_get(Tcp*);\n"), [5])
    check("rule13: `struct Type*` is a finding", r13(CLS + "int Tcp_get(struct Tcp* self);\n"), [5])
    check("rule13: an `inline` free function is a finding like any other",
          r13(CLS + "inline int Tcp_get(Tcp* self) {\n    return self->a;\n}\n"), [5])
    check("rule13: an `extern \"C\"` block in a .cpp is judged (the owner's own case)",
          r13(CLS + 'extern "C" {\ns32 Tcp_send(Tcp* self);\n}\n'), [6])
    check("rule13 static: a `Type_name(void)` / `Type_name(u32)` name with no self is a finding",
          r13(CLS + "Tcp* Tcp_getInstance(void);\nvoid Tcp_setValue(u32 v);\n"), [5, 6])
    check("rule13 static: a different pointer first parameter is a static member, not a self",
          r13(CLS + "struct Other {\n    int b;\n};\nint Tcp_get(Other* self);\n"), [8])
    check("rule13 static: a `Type**` first parameter is not a self, so it is static",
          r13(CLS + "int Tcp_get(Tcp** self);\n"), [5])
    check("rule13 static: the marker exempts it",
          r13(CLS + "/* free: retail C linkage, the dump names it unmangled */\nTcp* Tcp_getInstance(void);\n"), [])
    check("rule13 static: a prefix that is not a defined type is not a finding",
          r13(CLS + "int Nope_getInstance(void);\n"), [])
    check("rule13 static: a C file has no members", r13(CLS + "int Tcp_get(void);\n", "x.c"), [])
    check("rule13 static: a body's call is not a declaration",
          r13(CLS + "void f(void) {\n    Tcp_get();\n}\n"), [])
    check("rule13 static: a static member definition is not a finding",
          r13("struct Tcp {\n    static int get(void);\n};\nint Tcp::get(void) {\n    return 0;\n}\n"), [])
    check("rule13 static: `Type_ctor(Other*)` is a C-style helper, not a finding",
          r13(CLS + "struct Other {\n    int b;\n};\nvoid Tcp_ctor(Other* out);\nvoid Tcp_dtor(Other& out);\n"), [])
    check("rule13 static: `Type_ctor(void)` is a static finding", r13(CLS + "void Tcp_ctor(void);\n"), [5])
    check("rule13 static: `Type_ctor(Type* self)` is still the member finding",
          r13(CLS + "void Tcp_ctor(Tcp* self);\n"), [5])
    check("rule13 static: the detail names the static member, the call form and the mangling",
          [(f["static"], "static u32 Tcp::count()" in f["detail"], "Tcp::count(...)" in f["detail"],
            "count__3TcpFv" in f["detail"])
           for f in lint_source(Source("x.cpp", "x.cpp", CLS + "u32 Tcp_count(void);\n"), None)
           if f["rule"] == 13], [(True, True, True, True)])
    check("rule13: a name whose prefix is not a defined type is not a finding",
          r13(CLS + "int Nope_get(Tcp* self);\n"), [])
    check("rule13: a type the tool cannot see is not a finding", r13("int Tcp_get(Tcp* self);\n"), [])
    check("rule13: a member definition is not a finding",
          r13("struct Tcp {\n    int send(int n);\n};\nint Tcp::send(int n) {\n    return n;\n}\n"), [])
    check("rule13: an already-mangled name is not a finding",
          r13(CLS + "int Tcp_get__FP3Tcp(Tcp* self);\n"), [])
    check("rule13: a C file has no members", r13(CLS + "int Tcp_get(Tcp* self);\n", "x.c"), [])
    check("rule13: a header is judged by lint_source too (with the type in scope)",
          r13(CLS + "int Tcp_get(Tcp* self);\n", "include/mod/a.h"), [5])
    check("rule13: the header walk reports it",
          [f["line"] for f in rule13_findings(Source("a.h", "include/mod/a.h",
                                                    CLS + "int Tcp_get(Tcp* self);\n"))], [5])
    check("rule13: the marker on the line above exempts the declaration",
          r13(CLS + "/* free: retail C linkage, the dump names it unmangled */\nint Tcp_get(Tcp* self);\n"), [])
    check("rule13: a trailing marker exempts its own declaration",
          r13(CLS + "int Tcp_get(Tcp* self); /* free: SDK C struct */\n"), [])
    check("rule13: an SDK C struct is an accepted reason",
          r13(CLS + "// free: SDK C struct\nint Tcp_get(Tcp* self);\n"), [])
    check("rule13: an empty reason is still a finding",
          r13(CLS + "/* free: */\nint Tcp_get(Tcp* self);\n"), [6])
    check("rule13: a vague reason is still a finding",
          r13(CLS + "/* free: it is fine */\nint Tcp_get(Tcp* self);\n"), [6])
    check("rule13: a marker on an unrelated declaration does not leak",
          r13(CLS + "/* free: SDK C struct */\nint Tcp_a(Tcp* self);\n\nint Tcp_b(Tcp* self);\n"), [8])
    check("rule13: a trailing marker on the previous declaration does not exempt the next",
          r13(CLS + "int Tcp_a(Tcp* self); /* free: SDK C struct */\nint Tcp_b(Tcp* self);\n"), [6])
    check("rule13: a marker in a string is not a marker",
          r13(CLS + 'const char* s = "free: SDK C struct";\nint Tcp_get(Tcp* self);\n'), [6])
    check("rule13: two functions are two findings",
          r13(CLS + "int Tcp_a(Tcp* self);\nint Tcp_b(Tcp* self);\n"), [5, 6])
    check("rule13: a body is not a declaration (a call inside a definition)",
          r13(CLS + "void f(Tcp* t) {\n    Tcp_get(t);\n}\n"), [])
    check("rule13: static-like names are collected (they are findings too)",
          sorted(rule13_static_like(Source("x.cpp", "x.cpp", CLS + "Tcp* Tcp_getInstance(void);\n"
                                           "void Tcp_setValue(u32 v);\nint Tcp_get(Tcp* self);\n"))),
          ["Tcp_getInstance", "Tcp_setValue"])
    det = [f for f in lint_source(Source("x.cpp", "x.cpp",
                                         CLS + "s32 Tcp_send(Tcp* self, const u8* d, s32 n);\n"), None)
           if f["rule"] == 13]
    check("rule13: the detail names the type, the method, the member signature and the mangling",
          ("Tcp::send" in det[0]["detail"], "s32 Tcp::send(const u8* d, s32 n)" in det[0]["detail"],
           "send__3TcpFPCUcl" in det[0]["detail"], det[0]["token"]), (True, True, True, "Tcp_send"))

    # the registry: a tree-wide type, C++ reach only, cached per tree
    with tempfile.TemporaryDirectory() as tmp13:
        def put13(rel: str, text: str) -> None:
            path = os.path.join(tmp13, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        put13("include/mod/cls.h", "struct Tcp {\n    int a;\n};\n")
        put13("include/mod/cstruct.h", "struct CTcp {\n    int a;\n};\n")
        put13("include/mod/decl.h", '#include "mod/cls.h"\nint Tcp_get(Tcp* self);\n')
        put13("include/mod/cdecl.h", '#include "mod/cstruct.h"\nint CTcp_get(CTcp* self);\n')
        put13("src/mod/a.cpp", '#include "mod/decl.h"\nint Tcp_get(Tcp* self) {\n    return self->a;\n}\n')
        put13("src/mod/b.c", '#include "mod/cdecl.h"\nint CTcp_get(CTcp* self) {\n    return self->a;\n}\n')
        ctx13 = build_rule13_context(tmp13)
        check("rule13 registry: a type a .cpp reaches through a header is known", "Tcp" in ctx13.types, True)
        check("rule13 registry: a type only a .c file reaches is not", "CTcp" in ctx13.types, False)
        check("rule13 registry: a header only a .c file includes is out of scope",
              sorted(ctx13.headers), ["include/mod/cls.h", "include/mod/decl.h"])
        set_rule13_context(tmp13)
        check("rule13 registry: the declaration in a reachable header is a finding",
              [f["line"] for f in header_rule13_findings(tmp13)], [2])
        check("rule13 registry: the definition in the .cpp is a finding (type from another file)",
              [f["line"] for f in lint_tree(tmp13, [os.path.join(tmp13, "src/mod/a.cpp")], None)
               if f["rule"] == 13], [2])
        check("rule13 registry: the .c file is never a finding",
              [f for f in lint_tree(tmp13, [os.path.join(tmp13, "src/mod/b.c")], None) if f["rule"] == 13], [])
        put13("src/mod/c.cpp", "struct Late {\n    int a;\n};\nint Late_get(Late* self);\n")
        check("rule13 registry: the cache notices a new file",
              "Late" in build_rule13_context(tmp13).types, True)
        set_rule13_context(None)

    # `--diff`: a base finding is grandfathered, an added one refuses (a real temporary repository)
    import contextlib
    import io
    with tempfile.TemporaryDirectory() as tmp:
        def r13git(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)

        def r13put(rel: str, text: str) -> None:
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def r13rev() -> str:
            return subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True, text=True,
                                  encoding="utf-8", errors="replace").stdout.strip()

        def r13diff(base: str) -> tuple[int, dict]:
            old = os.getcwd()
            os.chdir(tmp)
            try:
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf), contextlib.redirect_stderr(io.StringIO()):
                    rc = main(["--diff", base, "--json"])
                return rc, json.loads(buf.getvalue())
            finally:
                os.chdir(old)

        r13git("init", "-q")
        r13git("checkout", "-q", "-b", "main")
        r13put("config/RMHE08/symbols.txt", "Tcp_old = .text:0x80002000; // type:function size:0x10\n")
        r13put("config/RMHE08/splits.txt", "mod/tcp.cpp:\n\t.text       start:0x80002000 end:0x80002010\n")
        r13put("include/mod/tcp.h", "struct Tcp {\n    int a;\n};\nint Tcp_old(Tcp* self);\n")
        r13put("src/mod/tcp.cpp", '#include "mod/tcp.h"\nint Tcp_old(Tcp* self) {\n    return self->a;\n}\n')
        r13git("add", "-A")
        r13git("commit", "-q", "-m", "base")
        base13 = r13rev()
        r13git("checkout", "-q", "-b", "lane")
        r13put("src/mod/tcp.cpp", '#include "mod/tcp.h"\n// a comment edit\nint Tcp_old(Tcp* self) {\n'
                                  "    return self->a;\n}\n")
        r13git("commit", "-q", "-am", "touch the file that already carries rule 13")
        rc_touch, out_touch = r13diff(base13)
        check("rule13 --diff: a file that already carries the finding is grandfathered (exit 0)", rc_touch, 0)
        check("... with no added row", out_touch["added"], [])
        r13put("src/mod/tcp.cpp", '#include "mod/tcp.h"\nint Tcp_old(Tcp* self) {\n    return self->a;\n}\n'
                                  "int Tcp_new(Tcp* self) {\n    return self->a;\n}\n")
        r13git("commit", "-q", "-am", "add a second C-spelled method")
        rc_add, out_add = r13diff(base13)
        check("rule13 --diff: an added `Type_name(Type*)` refuses (exit 1)", rc_add, 1)
        check("... as a rule-13 row on the .cpp",
              [(a["rule"], a["file"], a["added"]) for a in out_add["added"]], [(13, "src/mod/tcp.cpp", 1)])
        r13put("src/mod/tcp.cpp", '#include "mod/tcp.h"\nint Tcp_old(Tcp* self) {\n    return self->a;\n}\n'
                                  "/* free: retail C linkage, the caller's relocation names it unmangled */\n"
                                  "int Tcp_new(Tcp* self) {\n    return self->a;\n}\n")
        r13git("commit", "-q", "-am", "mark the new one")
        rc_mark, out_mark = r13diff(base13)
        check("rule13 --diff: a marked addition does not refuse", (rc_mark, out_mark["added"]), (0, []))
        r13put("include/mod/tcp.h", "struct Tcp {\n    int a;\n};\nint Tcp_old(Tcp* self);\n"
                                    "int Tcp_hdr(Tcp* self);\n")
        r13git("commit", "-q", "-am", "add a header declaration")
        rc_hdr, out_hdr = r13diff(base13)
        check("rule13 --diff: an added header declaration refuses",
              (rc_hdr, [(a["rule"], a["file"]) for a in out_hdr["added"]]), (1, [(13, "include/mod/tcp.h")]))
    set_rule13_context(None)

    # --- end-to-end over the fixtures -------------------------------------------------------------
    check("e2e: rule list is complete", sorted(RULE_NAMES), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
    check("e2e: no rule is declared unchecked", UNCHECKED, [])

    # --- the budget's rule-10 column is vtableaudit's (2026-10-05); the pragma check is rule 14 ------------
    check("rule 10 is an audit rule, never a lint finding", (AUDIT_RULES, 14 in RULE_NAMES), (
        {10: "tools/units/vtableaudit.py"}, True))
    fake = [{"rule": 7, "file": "src/a/x.cpp"}, {"rule": 14, "file": "include/a/x.h"}]
    for f in fake:
        f.setdefault("detail", "")
    b10 = budget(fake, {"src/a/x.cpp": 2, "src/a/y.cpp": 1})
    check("budget: the audit's counts fill the r10 column per file and the total; `findings` stays the lint's",
          (b10["totals"]["10"], b10["totals"]["14"], b10["totals"]["7"], b10["total"], b10["findings"],
           [(u["file"], u["rules"]["10"], u["total"]) for u in b10["units"]], b10["rule10"]),
          (3, 1, 1, 5, 2, [("include/a/x.h", 0, 1), ("src/a/x.cpp", 2, 3), ("src/a/y.cpp", 1, 1)],
           {"source": "tools/units/vtableaudit.py", "violations": 3}))
    check("budget: no audit (None) leaves the column 0 and says so", (budget(fake)["totals"]["10"],
                                                                     budget(fake)["rule10"]), (0, None))
    with tempfile.TemporaryDirectory() as aroot:
        os.makedirs(os.path.join(aroot, "tools", "units"))
        audit = os.path.join(aroot, "tools", "units", "vtableaudit.py")
        with open(audit, "w", encoding="utf-8") as fh:
            fh.write("import json\nprint(json.dumps({'violations': [{'unit': 'a/x.cpp'}, {'unit': 'a/x.cpp'}], "
                     "'references': [{'kind': 'own', 'file': 'src/a/y.cpp'}, {'kind': 'external', 'file': "
                     "'src/a/z.cpp'}], 'unbuilt': []}))\n")
        counts, note = rule10_counts(aroot)
        check("rule10_counts: a run counts against src/<unit>, an own-range write against its file, nothing else",
              counts, {"src/a/x.cpp": 2, "src/a/y.cpp": 1})
        check("... and the note names the audit, the total and the file count",
              ("vtableaudit" in note, "3 violation(s) over 2 file(s)" in note), (True, True))
        with open(audit, "w", encoding="utf-8") as fh:
            fh.write("import sys\nsys.exit(2)\n")
        check("rule10_counts: an audit that cannot read the tree is (None, why), never a crash or a 0 pass",
              rule10_counts(aroot)[0], None)
    check("e2e: findings sort by rule then line",
          rules_of(text), sorted(rules_of(text)))

    # --- `--diff` judges each side by its own map -------------------------------------------------
    # Before this, a rename read as a regression: the base copy still said the old name, the *working* map
    # no longer resolved it, so the base side's rule-2 findings vanished and the delta called them additions
    # (a real batch measured "+62 added rule-2 violations" for a pure rename). Stub the single call that
    # reads REF, so the check needs no repository.
    ref_map = "zzz_selftest_symbol = .text:0x80040598; // type:function size:0x8\n"
    ref_splits = "Pl/pl_act.cpp:\n\t.text\tstart:0x80040598 end:0x800405A0\n"
    real_git_bytes = _refs.git_bytes
    try:
        _refs.git_bytes = lambda root, *a: (
            ref_splits if a[-1].endswith("splits.txt") else ref_map).encode()
        ref_own = load_ownership_at_ref(".", "HEAD")
    finally:
        _refs.git_bytes = real_git_bytes
    check("the ref's own map resolves a name the working map cannot",
          ref_own is not None and ref_own.resolve("zzz_selftest_symbol") is not None, True)
    working = load_ownership(".")
    check("...and the working map really cannot resolve it",
          working is None or working.resolve("zzz_selftest_symbol"), None)
    check("an absent ref map falls back rather than dying", load_ownership_at_ref(".", ""), working)

    # --- `--diff REF`: the ref the comparison actually uses ----------------------------------------
    # `--diff main` in a branch whose main has moved must measure *this batch*, so a ref that is not an
    # ancestor resolves to the merge base and says so. 2026-09-27: a lane diagnosed another lane's landing
    # this way, and the orchestrator had to pass the merge base by hand three times in one night.
    import contextlib
    import io
    with tempfile.TemporaryDirectory() as tmp:
        def sgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)

        def rev(where: str = "HEAD") -> str:
            return subprocess.run(["git", "rev-parse", where], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        sgit("init", "-q")
        sgit("checkout", "-q", "-b", "main")
        open(os.path.join(tmp, "a.txt"), "w").write("1\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "base")
        cut = rev()
        sgit("checkout", "-q", "-b", "lane")
        open(os.path.join(tmp, "b.txt"), "w").write("2\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "the batch's own work")
        sgit("checkout", "-q", "main")
        open(os.path.join(tmp, "c.txt"), "w").write("3\n")
        sgit("add", "-A")
        sgit("commit", "-q", "-m", "another lane landed")
        moved_main = rev()
        sgit("checkout", "-q", "lane")
        check("an ancestor ref is used exactly as given", _resolve_diff_ref(tmp, cut), cut)
        check("... and so is HEAD", _resolve_diff_ref(tmp, "HEAD"), "HEAD")
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            resolved = _resolve_diff_ref(tmp, "main")
        check("a ref that is not an ancestor resolves to the merge base", resolved, cut)
        check("... not to the ref itself", resolved == moved_main, False)
        check("... and the run says which base it used",
              "merge base" in buf.getvalue() and cut[:12] in buf.getvalue(), True)
        check("... so the gate's own ancestor base is silent",
              _resolve_diff_ref(tmp, cut) == cut and buf.getvalue().count("merge base"), 1)

    # --- `--ref BRANCH`: judge a held branch in read-only mode ---------------------------------------
    # 2026-09-28: a claim lane re-implemented the tool's before/after merge in ~40 lines of scratch to
    # prove a `splits.txt` claim cleared a held branch's rows, and the reproduction was only approximately
    # trusted.  `--ref` reads both sides from git, so the branch is never checked out and the working tree
    # is never touched.  The rows must be the rows `--diff` would print with the branch checked out.
    with tempfile.TemporaryDirectory() as tmp:
        def rgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=tmp, capture_output=True, check=True)

        def rput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def rrev(where: str = "HEAD") -> str:
            return subprocess.run(["git", "rev-parse", where], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        rgit("init", "-q")
        rgit("checkout", "-q", "-b", "main")
        rput("config/RMHE08/symbols.txt",
             "owned_fn = .text:0x80002000; // type:function size:0x10\n"
             "unowned_data = .data:0x80003000; // type:object size:0x10\n")
        rput("config/RMHE08/splits.txt", "other/other_unit.c:\n\t.text       start:0x80002000 end:0x80002010\n")
        rput("src/other/other_unit.c", "void owned_fn(void) {}\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "base")
        base_sha = rrev()
        # the held branch adds two declarations that are findings only there
        rgit("checkout", "-q", "-b", "held")
        rput("src/held/held_unit.c", "extern void owned_fn(void);\nextern u8 unowned_data[];\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "held branch adds the declarations")
        # main moves on, so `held` is nobody's ancestor and the merge base is `base_sha`
        rgit("checkout", "-q", "main")
        rput("src/main_moved.c", "int main_moved;\n")
        rgit("add", "-A")
        rgit("commit", "-q", "-m", "main moved on")
        head_before, tree_before = rrev(), sorted(os.listdir(os.path.join(tmp, "src")))
        own = load_ownership(tmp)
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_ref = ref_comparison(tmp, "held", own, as_json=False)
        text_out = out.getvalue()
        check("--ref judges a held branch and fails on a real addition", (rc_ref, "rule 2" in text_out), (1, True))
        check("--ref reports the rule-12 row too", "rule 12" in text_out, True)
        check("--ref names the branch it judged", "held" in text_out, True)
        check("--ref is read-only: HEAD did not move", rrev(), head_before)
        check("... and the working tree is untouched", sorted(os.listdir(os.path.join(tmp, "src"))), tree_before)
        check("... the branch's file was never materialised",
              os.path.exists(os.path.join(tmp, "src/held/held_unit.c")), False)
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_ref_json = ref_comparison(tmp, "held", own, as_json=True)
        ref_data = json.loads(out.getvalue())
        check("--ref --json names the branch and its merge base",
              (ref_data["ref"], ref_data["base"]), ("held", base_sha))
        check("... and lists the added rows exactly",
              sorted((a["rule"], a["file"], a["added"]) for a in ref_data["added"]),
              [(2, "src/held/held_unit.c", 2), (12, "src/held/held_unit.c", 1)])
        # reproduce the rows `--diff <base>` prints with the branch CHECKED OUT - the strongest proof
        rgit("checkout", "-q", "held")
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_diff = main(["--diff", base_sha, "--json"])
            diff_data = json.loads(out.getvalue())
        finally:
            os.chdir(old_cwd)
        check("--ref reproduces --diff's rows for the branch",
              sorted((a["rule"], a["file"], a["added"]) for a in diff_data["added"]),
              sorted((a["rule"], a["file"], a["added"]) for a in ref_data["added"]))
        check("... and the exit codes agree", (rc_ref_json, rc_diff), (1, 1))
        # RUN FROM THE BRANCH'S OWN WORKTREE: the review lane is launched exactly here, and `--ref held`
        # used to compare the branch with itself - `_resolve_diff_ref` saw held as an ancestor of HEAD (it
        # IS HEAD) and returned it - printing "adds no violation over 0 changed file(s)" and exiting 0.
        # That is a false green for every review's lint row (2026-09-28). It must judge the real rows.
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            rc_self = ref_comparison(tmp, "held", own, as_json=False)
        self_text = out.getvalue()
        check("--ref from the branch's own worktree judges the branch, not itself", rc_self, 1)
        check("... and never reports 0 changed files as clean", "over 0 changed file(s)" in self_text, False)
        check("... naming the fork point it used instead", "fork point" in err.getvalue(), True)
        check("... with the same rule-2 row as from MAIN", "rule 2" in self_text, True)
        # an EMPTY branch measures nothing too: refuse, never print "clean"
        rgit("checkout", "-q", "-b", "empty", "main")
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            rc_empty = ref_comparison(tmp, "empty", own, as_json=False)
        check("--ref on a branch with nothing to compare REFUSES (exit 2, not a false clean)", rc_empty, 2)
        check("... saying it judged 0 changed file(s)", "0 changed file(s)" in out.getvalue(), True)
        check("... and pointing at MAIN or --diff",
              ("from MAIN" in out.getvalue() or "--diff" in out.getvalue()), True)
        # here the branch IS HEAD, so `self_ref` is true while the resolved base is the fork point: the
        # reason must come from the base the comparison actually ran against, not from `self_ref` - the
        # old wording contradicted the "judging against the fork point" line printed a moment before
        check("... and the reason comes from the RESOLVED base, not from `self_ref`",
              ("no source file changed between" in out.getvalue()
               and "nothing to compare it against" not in out.getvalue()), True)
        rgit("checkout", "-q", "main")

    # --- `--diff`/`--ref` are rename-stable: a renamed header's rule-10/11/12 findings are not additions --
    # The rename that hit it (`include/fn_80429B94.h` -> `include/Network/network_pat_control.h`, the
    # network-pat lane) alone reported "+6 rule 11 ... (0 -> 6)" and "+1 rule 12 ... (1 -> 2)": the
    # whole-tree header walks read the *base* tree and keyed its findings by the old path, while the
    # per-file walk and the working tree keyed by the new one.  Rule 10 has the same shape.  A rename must
    # measure delta 0 - that is the whole point of the comparison.
    with tempfile.TemporaryDirectory() as tmp:
        def dgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def dput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def drev(where: str = "HEAD") -> str:
            return subprocess.run(["git", "rev-parse", where], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        dgit("init", "-q")
        dgit("checkout", "-q", "-b", "main")
        dput("config/RMHE08/symbols.txt",
             "owned_fn = .text:0x80002000; // type:function size:0x10\n"
             "unowned_data = .data:0x80003000; // type:object size:0x10\n")
        dput("config/RMHE08/splits.txt",
             "other/other_unit.c:\n\t.text       start:0x80002000 end:0x80002010\n")
        dput("src/other/other_unit.c", "void owned_fn(void) {}\n")
        # one finding of each header rule: rule 14 (a codegen pragma), rule 11 (`void *` parameter), rule
        # 12 (`extern` of data no registered range claims).
        dput("include/mod/a.h",
             "#pragma pool\nvoid takes_void_star(void *p);\nextern u8 unowned_data[];\n")
        dgit("add", "-A")
        dgit("commit", "-q", "-m", "base")
        base_sha = drev()
        # a positive control: the base header really carries one finding of each header rule, so a delta 0
        # below is the rename being stable and not the fixture carrying nothing.
        base_own = load_ownership_at_ref(tmp, base_sha)
        base_src = Source("include/mod/a.h", "include/mod/a.h",
                          _refs.git_bytes(tmp, "show", "%s:include/mod/a.h" % base_sha).decode("utf-8"))
        check("... the base header really carried one finding of each header rule",
              sorted(f["rule"] for f in (codegen_pragma_findings(base_src) + rule11_findings(base_src)
                                          + rule12_findings(base_src, base_own))),
              [11, 12, 14])
        # the rename, committed on `lane`, so both comparisons (working tree `--diff`, read-only `--ref`)
        # measure it the same way
        dgit("checkout", "-q", "-b", "lane")
        dgit("mv", "include/mod/a.h", "include/mod/b.h")
        dgit("commit", "-q", "-m", "rename a header that carries all three header findings")
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_diff_rename = main(["--diff", base_sha, "--json"])
            diff_rename = json.loads(out.getvalue())
        finally:
            os.chdir(old_cwd)
        check("--diff on a renamed header measures delta 0 (exit 0)", rc_diff_rename, 0)
        check("... with no added row", diff_rename["added"], [])
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_ref_rename = ref_comparison(tmp, "lane", load_ownership(tmp), as_json=True)
        ref_rename = json.loads(out.getvalue())
        check("--ref on the same rename also measures delta 0", rc_ref_rename, 0)
        check("... with no added row", ref_rename["added"], [])

    # --- the classifier (2026-10-05): a header is a `.h` anywhere, the band root is one constant ------------
    # The owner's ruling moves every header beside its source (`include/<m>/x.h` -> `src/<m>/x.h`) and the band to
    # `src/unsplit/`. A classifier keyed on `include/` would drop every header rule on the move and read the band as
    # an ordinary header; these pin that it does not.
    check("classifier: a header under include/ is a header", is_header("include/mod/a.h"), True)
    check("classifier: a header under src/ is a header", is_header("src/mod/a.h"), True)
    check("classifier: a .cpp is not a header", is_header("src/mod/a.cpp"), False)
    check("classifier: the band is src/unsplit/ (BAND_ROOT)", is_unsplit_header("src/unsplit/mod.h"), True)
    check("classifier: ... and include/unsplit/ still reads as the band (a pre-move ref, the --diff back side)",
          is_unsplit_header("include/unsplit/mod.h"), True)
    check("classifier: any other src/ header is not the band", is_unsplit_header("src/mod/unsplit.h"), False)
    check("classifier: the band reading at the new root (an owned name declared there is rule 2)",
          lines_of("void foo(void);\n", 2, "src/unsplit/mod.h", idx), [1])
    check("classifier: the band at its new root carries no STOPGAP reading (band semantics kept)",
          [f["rule"] for f in lint_source(Source("b", "src/unsplit/mod.h",
                                                 "/* STOPGAP-BEGIN(none) */\n/* STOPGAP-END(none) */\n"))],
          [])
    check("classifier: a rule-2 detail names no band path",
          [f["detail"] for f in lint_source(Source("x", "src/other/c.c", "extern void mid(void);\n"), mid)
           if f["rule"] == 2],
          ["`mid` has no registered owner - declare it in the `mod` unsplit band header"])
    hbody = ("struct Hdr {\n    u32 unk00;\n};\nvoid fn_80040598(void);\nvoid f(void) {\n    goto x;\n}\n"
             "#pragma peephole off\n")
    for hrel in ("include/mod/h.h", "src/mod/h.h"):
        check("classifier: the body rule set runs on %s (3, 4, 5, 7, 8, 14)" % hrel,
              sorted({r for r, _l in rules_of(hbody, hrel)}), [3, 4, 5, 7, 8, 14])
    check("classifier: a .c is never judged by rule 14",
          sorted({r for r, _l in rules_of(hbody, "src/mod/h.c")}), [3, 4, 5, 7, 8])
    import tools.units.stylelint_rules.lint as _lint_mod
    _saved_switch = _lint_mod.HEADER_BODY_RULES_ON
    try:
        _lint_mod.HEADER_BODY_RULES_ON = False
        check("classifier: the recommended header body rules switch off in one place (rule 7 and 14 stay)",
              sorted({r for r, _l in rules_of(hbody, "include/mod/h.h")}), [7, 14])
        check("classifier: ... and the switch never touches a source file",
              sorted({r for r, _l in rules_of(hbody, "src/mod/h.c")}), [3, 4, 5, 7, 8])
    finally:
        _lint_mod.HEADER_BODY_RULES_ON = _saved_switch
    check("classifier: the owner's header beside its source is the owner's (rule 2 clean)",
          lines_of("void foo(void);\n", 2, "src/mod/a.h", idx), [])

    # --- the header move itself: `include/mod/a.h` -> `src/mod/a.h` and the band's rule-2 reading -------------
    # A `git mv` of a header carrying findings of every header-visible rule must measure delta 0 on both
    # comparisons: identities are path-free (rule 2's band detail names a module) and the per-file walk keys the base
    # copy by the path the working tree spells.
    with tempfile.TemporaryDirectory() as tmp:
        def mgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def mput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        mgit("init", "-q")
        mgit("checkout", "-q", "-b", "main")
        mput("config/RMHE08/symbols.txt",
             "owned_fn = .text:0x80002000; // type:function size:0x10\n"
             "unowned_data = .data:0x80003000; // type:object size:0x10\n"
             "mid_fn = .text:0x80002500; // type:function size:0x10\n")
        mput("config/RMHE08/splits.txt",
             "other/other_unit.c:\n\t.text       start:0x80002000 end:0x80002010\n"
             "other/later_unit.c:\n\t.text       start:0x80003000 end:0x80003010\n")
        mput("src/other/other_unit.c", "void owned_fn(void) {}\n")
        mput("src/other/later_unit.c", "void later(void) {}\n")
        mput("src/mod/user.c", "extern void mid_fn(void);\nvoid u(void) { mid_fn(); }\n")
        mput("include/mod/a.h",
             "#pragma pool\nvoid takes_void_star(void *p);\nextern u8 unowned_data[];\nvoid fn_80040598(void);\n"
             "void owned_fn(void);\nstruct S {\n    u32 x;\n};\n")
        mgit("add", "-A")
        mgit("commit", "-q", "-m", "base")
        base_sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True, text=True,
                                  encoding="utf-8", errors="replace").stdout.strip()
        base_rules = sorted({f["rule"] for f in lint_source(
            Source("a", "include/mod/a.h", open(os.path.join(tmp, "include/mod/a.h"), encoding="utf-8").read()),
            load_ownership_at_ref(tmp, base_sha))})
        check("header move: the base header carries rules 2, 3, 4, 7, 11, 12 and 14 (a positive control)",
              base_rules, [2, 3, 4, 7, 11, 12, 14])
        mgit("checkout", "-q", "-b", "lane")
        mgit("mv", "include/mod/a.h", "src/mod/a.h")
        mgit("commit", "-q", "-m", "move a header beside its source")
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_move = main(["--diff", base_sha, "--json"])
            move = json.loads(out.getvalue())
        finally:
            os.chdir(old_cwd)
        check("header move: --diff measures delta 0 (exit 0)", rc_move, 0)
        check("header move: ... no added row", move["added"], [])
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_move_ref = ref_comparison(tmp, "lane", load_ownership(tmp), as_json=True)
        check("header move: --ref measures delta 0 too", (rc_move_ref, json.loads(out.getvalue())["added"]), (0, []))
        # the negative control: the same move that also adds one generated name is refused, naming it
        mgit("checkout", "-q", "-b", "lane2", base_sha)
        mgit("mv", "include/mod/a.h", "src/mod/a.h")
        mput("src/mod/a.h", open(os.path.join(tmp, "src/mod/a.h"), encoding="utf-8").read()
             + "void fn_80041234(void);\n")
        mgit("commit", "-q", "-am", "move and add one generated name")
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            rc_move_bad = ref_comparison(tmp, "lane2", load_ownership(tmp), as_json=True)
        bad = json.loads(out.getvalue())
        check("header move: a move that adds a generated name refuses (negative control)",
              (rc_move_bad, [(d["rule"], d["file"], d["token"]) for d in bad["detail"]]),
              (1, [(7, "src/mod/a.h", "fn_80041234")]))

    # --- `--list-added`: name the findings a `--diff` counts, grouped the way a lane needs them -------
    # 2026-09-28: a UI lane read "+76 rule 7" with no way to learn *which* tokens were added; it dropped
    # three whole bodies to find the load-bearing renames, then re-added them.  The diff below adds one
    # occurrence of each of rules 7, 11 and 12 to a file that already carries findings of all three, so
    # the detail must name the new ones and exclude the old.
    with tempfile.TemporaryDirectory() as tmp:
        def agit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def aput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def arev() -> str:
            return subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        agit("init", "-q")
        agit("checkout", "-q", "-b", "main")
        aput("config/RMHE08/symbols.txt",
             "owned_thing = .text:0x80200000; // type:function size:0x10\n"
             "new_data = .data:0x80400010; // type:object size:0x10\n")
        # `new_data` starts owned by another unit, so the base `extern` is a rule-2 finding, never rule 12.
        # The batch drops that range (making it unowned data): rule 12 rises 0 -> 1 (a genuine addition),
        # and the *same* token now carries a *different* rule-2 complaint - owned-foreign -> unsplit - which
        # the identity keeps distinct (its detail differs), so rule 2 is named too even though its count does
        # not rise.  A count delta could not see that; the token set can.  Exactly one brand-new rule 7 and
        # rule 11 token is added on top (the file's pre-existing `fn_80040598`/`base_fn` are not additions).
        base_splits = ("other/other_unit.c:\n\t.text       start:0x80200000 end:0x80200010\n"
                       "\t.data       start:0x80400000 end:0x80400020\n")
        base_file = ("void fn_80040598(void) {}\n"
                     "void base_fn(void *p);\n"
                     "extern u8 new_data[];\n")
        aput("config/RMHE08/splits.txt", base_splits)
        aput("src/Pl/pl_act.cpp", base_file)
        agit("add", "-A")
        agit("commit", "-q", "-m", "base carries a pre-existing finding of each of rules 7, 11 and 2")
        base_sha = arev()
        aput("config/RMHE08/splits.txt",
             "other/other_unit.c:\n\t.text       start:0x80200000 end:0x80200010\n")
        aput("src/Pl/pl_act.cpp",
             "void fn_80040598(void) {}\n"
             "void fn_80275B04(void) {}\n"
             "void base_fn(void *p);\n"
             "void new_fn(void *q);\n"
             "extern u8 new_data[];\n")
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_plain = main(["--diff", base_sha])
            plain = out.getvalue()
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_list = main(["--diff", base_sha, "--list-added"])
            listed = out.getvalue()
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_json = main(["--diff", base_sha, "--json"])
            data = json.loads(out.getvalue())
        finally:
            os.chdir(old_cwd)
        # the flag is a report only: exit status and the summary a lane/gate reads are untouched.
        check("--list-added leaves the exit status alone", (rc_plain, rc_list, rc_json), (1, 1, 1))
        check("... the absent-flag summary line is byte-identical",
              plain.splitlines()[0],
              "stylelint: the batch adds 4 section 6.5 violation(s) over 1 changed file(s):")
        check("... and its per-rule rows are the ones a lane already reads",
              [ln for ln in plain.splitlines() if ln.startswith("  +")],
              ["  +1 rule 2  src/Pl/pl_act.cpp  (1 -> 1)",
               "  +1 rule 7  src/Pl/pl_act.cpp  (1 -> 2)",
               "  +1 rule 11  src/Pl/pl_act.cpp  (1 -> 2)",
               "  +1 rule 12  src/Pl/pl_act.cpp  (0 -> 1)"])
        check("... the without-flag run carries no detail", "added findings" in plain, False)
        # the rule-2 row is (1 -> 1): the count did not rise, but the *token* `new_data` now carries the
        # unsplit complaint instead of the owned-foreign one - two different complaints about one token,
        # which the identity keeps distinct by construction.
        check("--diff --json names exactly the four added findings",
              [(d["rule"], d["file"], d["line"], d["token"]) for d in data["detail"]],
              [(2, "src/Pl/pl_act.cpp", 5, "new_data"),
               (7, "src/Pl/pl_act.cpp", 2, "fn_80275B04"),
               (11, "src/Pl/pl_act.cpp", 4, "new_fn"),
               (12, "src/Pl/pl_act.cpp", 5, "new_data")])
        check("... and never the pre-existing occurrences",
              [ln for ln in listed.splitlines() if "fn_80040598" in ln or "base_fn" in ln], [])
        check("--list-added prints the same four, grouped by rule",
              [ln for ln in listed.splitlines() if ln.startswith("    rule ")],
              ["    rule 2 src/Pl/pl_act.cpp:5 new_data",
               "    rule 7 src/Pl/pl_act.cpp:2 fn_80275B04",
               "    rule 11 src/Pl/pl_act.cpp:4 new_fn",
               "    rule 12 src/Pl/pl_act.cpp:5 new_data"])
        check("... with the per-file count line", "  src/Pl/pl_act.cpp (1)" in listed, True)
        # a file that only already carries findings: touching it (a comment) adds none, so the exit status
        # stays 0 - a pre-existing finding never blocks.
        aput("config/RMHE08/splits.txt", base_splits)
        aput("src/Pl/pl_act.cpp", "/* touched, adds no finding */\n" + base_file)
        os.chdir(tmp)
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                rc_pre = main(["--diff", base_sha, "--list-added"])
            pre_text = out.getvalue()
        finally:
            os.chdir(old_cwd)
        check("a diff that adds nothing to a file of pre-existing findings exits 0", rc_pre, 0)
        check("... and prints no detail", "added findings" in pre_text, False)

    # --- the count cap is gone: an addition is a *token new to the file*, not a count delta (2026-09-30) --
    # A count delta cannot say which occurrences are new, so the old `--diff` refused EVERY added
    # occurrence of anything in a file that already carried a finding.  A lane at 69 rule-7 findings had to
    # rename five symbols (carrying 445 of its band's 466 references, referrers in another module) merely to
    # earn the right to write a body, and two more lanes lost 920 B of 100 %-measured bodies to rule 12,
    # which has no rename remedy at all.  The judgement is now the **token set difference per (rule, file)**:
    # spelling an already-flagged name 30 more times is not an addition; a token new to the file still is.
    # The exit-code contract (0 clean / 1 additions / 2 nothing measured) and the `+N rule R (before ->
    # after)` rows are unchanged - only what `N` counts.
    with tempfile.TemporaryDirectory() as tmp:
        def cgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def cput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        def crev() -> str:
            return subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True,
                                  text=True, encoding="utf-8", errors="replace").stdout.strip()

        cgit("init", "-q")
        cgit("checkout", "-q", "-b", "main")
        # the base symbol map: `unowned_data` is data no range claims (rule 12); `fn_80040598` is an
        # auto-generated stem (rule 7); `old_name` is owned by `other/other_unit.c`, so declaring it here is
        # a rule-2 finding - the two rules the rename map exists for.
        symbols_txt = ("owned_fn = .text:0x80200000; // type:function size:0x10\n"
                       "unowned_data = .data:0x80400000; // type:object size:0x10\n"
                       "fn_80040598 = .text:0x80201000; // type:function size:0x10\n"
                       "fn_80275B04 = .text:0x80201010; // type:function size:0x10\n"
                       "old_name = .text:0x80200030; // type:function size:0x10\n"
                       "pl_act_step_reset = .text:0x80201020; // type:function size:0x10\n")
        cput("config/RMHE08/symbols.txt", symbols_txt)
        cput("config/RMHE08/splits.txt",
             "other/other_unit.c:\n\t.text       start:0x80200000 end:0x80201000\n"
             "Pl/pl_act.cpp:\n\t.text       start:0x80201000 end:0x80202000\n")
        cput("src/other/other_unit.c", "void owned_fn(void) {}\n")
        base_file = ("void fn_80040598(void) {}\n"
                     "extern u8 unowned_data[];\n"
                     "extern void old_name(void);\n")
        cput("src/Pl/pl_act.cpp", base_file)
        cgit("add", "-A")
        cgit("commit", "-q", "-m", "base: one rule-7, one rule-12 and one rule-2 finding")
        base_sha = crev()
        # the thirty more spellings of the ALREADY-FLAGGED stem the cap used to refuse
        more = "void user(void) {\n" + "".join("    fn_80040598();\n" for _ in range(30)) + "}\n"
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            def crun(*extra: str) -> tuple:
                # the working map is rewritten per scenario; the mtime cache can miss a same-tick write
                _OWNERSHIP_CACHE.clear()
                out = io.StringIO()
                with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                    rc = main(["--diff", base_sha, *extra])
                return rc, out.getvalue()

            # (a) +30 occurrences of an already-flagged token -> PASS (this is the cap being removed)
            cput("src/Pl/pl_act.cpp", base_file + more)
            rc_a, out_a = crun()
            check("(a) 30 more occurrences of an already-flagged token are not additions", rc_a, 0)
            check("... and print no `+N rule R` row", [ln for ln in out_a.splitlines()
                                                    if ln.startswith("  +")], [])

            # (b) the same file gaining a *new* generated token -> REFUSE, naming it
            cput("src/Pl/pl_act.cpp", base_file + more + "void extra(void) { fn_80275B04(); }\n")
            rc_b, out_b = crun("--list-added")
            check("(b) a new generated token in that file refuses", rc_b, 1)
            check("... the row keeps both the new count and the old one",
                  [ln for ln in out_b.splitlines() if ln.startswith("  +")],
                  ["  +1 rule 7  src/Pl/pl_act.cpp  (1 -> 32)"])
            check("... and names the one token that is new, never the 31 that are not",
                  [ln for ln in out_b.splitlines() if ln.strip().startswith("rule 7 src/")],
                  ["    rule 7 src/Pl/pl_act.cpp:36 fn_80275B04"])

            # (c) a rename in that file (old token -> real name) -> PASS, with the count falling
            cput("src/Pl/pl_act.cpp",
                 "void pl_act_step_reset(void) {}\n"
                 "extern u8 unowned_data[];\n"
                 "extern void old_name(void);\n"
                 "void user(void) {\n" + "".join("    pl_act_step_reset();\n"
                                                  for _ in range(30)) + "}\n")
            own_work = load_ownership(tmp)
            after_c = open(os.path.join(tmp, "src/Pl/pl_act.cpp"), encoding="utf-8").read()
            check("(c) the rename really is a falling count",
                  (sum(1 for f in lint_source(Source("src/Pl/pl_act.cpp", "src/Pl/pl_act.cpp",
                                                     _refs.git_bytes(tmp, "show", "%s:src/Pl/pl_act.cpp"
                                                               % base_sha).decode("utf-8")),
                                             own_work) if f["rule"] == 7),
                   sum(1 for f in lint_source(Source("src/Pl/pl_act.cpp", "src/Pl/pl_act.cpp",
                                                     after_c),
                                             own_work) if f["rule"] == 7)), (1, 0))
            rc_c, out_c = crun("--json")
            check("... and measures clean", (rc_c, json.loads(out_c)["added"]), (0, []))

            # the symbol rename the map translates: `old_name` -> `new_name` at the SAME address, own by the
            # same unit.  A count delta sees 1 -> 1 and passes; a naive token set would call it a removal
            # plus an addition and refuse, which is the `+62 rule-2 violations for a pure rename` artefact.
            cput("config/RMHE08/symbols.txt",
                 symbols_txt.replace("old_name = .text:0x80200030",
                                     "new_name = .text:0x80200030"))
            cput("src/Pl/pl_act.cpp", base_file.replace("old_name", "new_name"))
            base_own_c2 = load_ownership_at_ref(tmp, base_sha)
            after_own_c2 = load_ownership(tmp)
            check("(e) rename_map reads the row rename the base declared under the old name",
                  rename_map(base_own_c2, after_own_c2, {"old_name"}), {"old_name": "new_name"})
            rc_rename, out_rename = crun("--json")
            check("... so a pure symbol rename still measures clean (no removal + addition)",
                  (rc_rename, json.loads(out_rename)["added"]), (0, []))
            # the *whole unit* renamed too (`menu/fn_802A6624.cpp` -> `menu/menu_message.cpp`): the range
            # boundaries are identical, so the address is unchanged and the row is still the same symbol.
            # The owner *path* a rule-2 detail spells moves with it, or the same finding reads as new.
            unit_a = Ownership({"old_row": [(".text", 0x80200030, "function")]},
                               {".text": [(0x80200000, 0x80201000, "menu/fn_802A6624.cpp")]})
            unit_b = Ownership({"new_row": [(".text", 0x80200030, "function")]},
                               {".text": [(0x80200000, 0x80201000, "menu/menu_message.cpp")]})
            check("... even when the owning unit file was renamed with it",
                  rename_map(unit_a, unit_b, {"old_row"}), {"old_row": "new_row"})
            check("... and the owner path its detail spells is translated with the unit",
                  renamed_finding({"rule": 2, "file": "x", "line": 1, "token": "old_row",
                                   "detail": "`old_row` is owned by `src/menu/fn_802A6624.cpp`"},
                                  {"old_row": "new_row"},
                                  {"src/menu/fn_802A6624.cpp": "src/menu/menu_message.cpp"}),
                  {"rule": 2, "file": "x", "line": 1, "token": "new_row",
                   "detail": "`new_row` is owned by `src/menu/menu_message.cpp`"})
            # ... but a range the batch moved to another owner is NOT a rename: the boundaries differ
            moved_unit = Ownership({"new_row": [(".text", 0x80200030, "function")]},
                                   {".text": [(0x80200000, 0x80200008, "menu/menu_message.cpp")]})
            check("... while a range that moved is not read as a rename",
                  rename_map(unit_a, moved_unit, {"old_row"}), {})

            # the same rename is NOT credited when the address moved: the map (`rename_map`) keys on the
            # address, so `new_name` at a new address leaves the old declaration's finding in place.
            cput("config/RMHE08/symbols.txt",
                 symbols_txt.replace("old_name = .text:0x80200030",
                                     "new_name = .text:0x80200080"))
            rc_moved, out_moved = crun("--json")
            check("... and an address that moved is not read as a rename",
                  (rc_moved, [a["rule"] for a in json.loads(out_moved)["added"]]), (1, [2]))
            cput("config/RMHE08/symbols.txt", symbols_txt)

            # (d) a NEW file -> every token in it is new -> REFUSE
            cput("src/Pl/pl_act.cpp", base_file)
            cput("src/Pl/brand_new.cpp", "void fn_80ABCDEF(void) {}\n")
            rc_d, out_d = crun("--list-added")
            check("(d) a new file's every token is an addition", rc_d, 1)
            check("... named on the new file",
                  [ln for ln in out_d.splitlines() if ln.strip().startswith("rule 7 src/")],
                  ["    rule 7 src/Pl/brand_new.cpp:1 fn_80ABCDEF"])
            os.remove(os.path.join(tmp, "src/Pl/brand_new.cpp"))

            # (f) base unreadable -> REFUSE, never pass silently.  A comment-only touch adds no token, so
            # with the base readable it exits 0; blinded, the base side contributes no identity at all and
            # every after-side finding is an addition - the fail-closed half of the judgement.
            cput("src/Pl/pl_act.cpp", "/* touched, adds no token */\n" + base_file)
            rc_ok, out_ok = crun()
            check("(f) a token-preserving touch passes when the base is readable", rc_ok, 0)
            real_git_bytes = _refs.git_bytes

            def _blind(root: str, *args: str) -> bytes:
                if args[:1] == ("show",) and args[1].startswith(base_sha) \
                        and args[1].endswith("src/Pl/pl_act.cpp"):
                    raise RuntimeError("selftest: simulated unreadable base object")
                return real_git_bytes(root, *args)

            _refs.git_bytes = _blind
            try:
                rc_blind, _out_blind = crun()
            finally:
                _refs.git_bytes = real_git_bytes
            check("... and REFUSES when the base blob cannot be read", rc_blind, 1)
        finally:
            os.chdir(old_cwd)

    # --- a MOVE is not growth: an identity another file of the batch gave up is credited (2026-09-29) ------
    # `--diff` grandfathered per file, so functions moved from an old file into a new one (a recut) read as
    # additions in the new file though the old one lost the same findings.  The credit is earned by the diff:
    # one per removal from another file, the same (rule, token, detail), never silent.
    with tempfile.TemporaryDirectory() as tmp:
        def mgit(*args: str) -> None:
            subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=tmp, capture_output=True, check=True)

        def mput(rel: str, text: str) -> None:
            p = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        mgit("init", "-q")
        mgit("checkout", "-q", "-b", "main")
        mput("config/RMHE08/symbols.txt",
             "unowned_data = .data:0x80400000; // type:object size:0x10\n"
             "fn_80040598 = .text:0x80201000; // type:function size:0x10\n"
             "fn_80275B04 = .text:0x80201010; // type:function size:0x10\n")
        mput("config/RMHE08/splits.txt", "Pl/pl_act.cpp:\n\t.text       start:0x80201000 end:0x80202000\n")
        old_body = ("void fn_80040598(void) {}\n"
                    "void fn_80275B04(void) {}\n")
        mput("src/Pl/pl_act.cpp", old_body + "extern u8 unowned_data[];\n")
        mgit("add", "-A")
        mgit("commit", "-q", "-m", "base")
        m_base = subprocess.run(["git", "rev-parse", "HEAD"], cwd=tmp, capture_output=True, text=True,
                                encoding="utf-8", errors="replace").stdout.strip()
        old_cwd = os.getcwd()
        os.chdir(tmp)
        try:
            def mrun(*extra: str) -> tuple:
                _OWNERSHIP_CACHE.clear()
                out = io.StringIO()
                with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                    rc = main(["--diff", m_base, *extra])
                return rc, out.getvalue()

            def moved_lines(text: str) -> list:
                return [ln.strip() for ln in text.splitlines() if ln.strip().startswith("moved rule")]

            # (1) a pure move: both rule-7 tokens leave the old file and land in the new one
            mput("src/Pl/pl_act.cpp", "extern u8 unowned_data[];\n")
            mput("src/Pl/pl_new.cpp", old_body)
            rc, out = mrun()
            check("(move 1) a pure move is credited and the verdict is clean", rc, 0)
            check("... reported as moved, naming the old and the new file",
                  moved_lines(out),
                  ["moved rule 7 fn_80040598: src/Pl/pl_act.cpp -> src/Pl/pl_new.cpp",
                   "moved rule 7 fn_80275B04: src/Pl/pl_act.cpp -> src/Pl/pl_new.cpp"])
            rc, out = mrun("--json")
            check("... and the --json payload carries the moves", len(json.loads(out)["moved"]), 2)

            # (2) a move plus one genuinely new finding: only the new one refuses
            mput("src/Pl/pl_new.cpp", old_body + "void extra(void) { fn_80ABCDEF(); }\n")
            rc, out = mrun("--list-added")
            check("(move 2) a move plus one new finding refuses", rc, 1)
            check("... naming only the new token",
                  [ln.strip() for ln in out.splitlines() if ln.strip().startswith("rule 7 src/")],
                  ["rule 7 src/Pl/pl_new.cpp:3 fn_80ABCDEF"])
            check("... the row counts one addition, not three",
                  [ln for ln in out.splitlines() if ln.startswith("  +")],
                  ["  +1 rule 7  src/Pl/pl_new.cpp  (0 -> 3)"])
            check("... and the credited moves are still reported", len(moved_lines(out)), 2)

            # (3) a copy that leaves the original intact: nothing was removed, nothing is credited
            mput("src/Pl/pl_act.cpp", old_body + "extern u8 unowned_data[];\n")
            mput("src/Pl/pl_new.cpp", old_body)
            rc, out = mrun()
            check("(move 3) a copy that leaves the original intact is net growth and refuses", rc, 1)
            check("... with no move credited", moved_lines(out), [])

            # (4) a finding of a different rule leaves: no credit for a rule-7 addition
            mput("src/Pl/pl_act.cpp", old_body)          # the rule-12 extern is gone
            mput("src/Pl/pl_new.cpp", "void fn_80ABCDEF(void) {}\n")
            rc, out = mrun()
            check("(move 4) a removal of another rule earns no credit", (rc, moved_lines(out)), (1, []))

            # (5) net growth across the batch: two files gain a name one file lost - one credit only
            mput("src/Pl/pl_act.cpp", "extern u8 unowned_data[];\n")
            mput("src/Pl/pl_new.cpp", "void fn_80040598(void) {}\n")
            mput("src/Pl/pl_new2.cpp", "void fn_80040598(void) {}\n")
            rc, out = mrun()
            check("(move 5) a name that grew across the batch still refuses (one credit per removal)",
                  (rc, len(moved_lines(out))), (1, 1))

            # (6) a DELETED file's identities are removals: a fold that deletes pl_act.cpp and absorbs its
            # bodies into pl_new.cpp is a move, though the deleted file no longer exists after the batch
            for extra_file in ("src/Pl/pl_new.cpp", "src/Pl/pl_new2.cpp"):
                if os.path.exists(os.path.join(tmp, extra_file)):
                    os.remove(os.path.join(tmp, extra_file))
            mput("src/Pl/pl_act.cpp", old_body + "extern u8 unowned_data[];\n")
            os.remove(os.path.join(tmp, "src/Pl/pl_act.cpp"))
            mput("src/Pl/pl_new.cpp", old_body)
            rc, out = mrun()
            check("(delete 1) delete-and-absorb is credited and the verdict is clean", rc, 0)
            check("... reported as moved from the deleted file",
                  moved_lines(out),
                  ["moved rule 7 fn_80040598: src/Pl/pl_act.cpp -> src/Pl/pl_new.cpp",
                   "moved rule 7 fn_80275B04: src/Pl/pl_act.cpp -> src/Pl/pl_new.cpp"])

            # (7) delete + one genuinely new finding: only the new one refuses
            mput("src/Pl/pl_new.cpp", old_body + "void extra(void) { fn_80ABCDEF(); }\n")
            rc, out = mrun("--list-added")
            check("(delete 2) delete-and-absorb plus a new finding refuses only the new one",
                  (rc, [ln.strip() for ln in out.splitlines() if ln.strip().startswith("rule 7 src/")]),
                  (1, ["rule 7 src/Pl/pl_new.cpp:3 fn_80ABCDEF"]))

            # (8) a deleted file whose content is NOT absorbed anywhere earns nothing for an unrelated add
            mput("src/Pl/pl_new.cpp", "void fn_80ABCDEF(void) {}\n")
            rc, out = mrun()
            check("(delete 3) a new token is not credited by an unrelated deleted file", (rc, moved_lines(out)),
                  (1, []))

            # (9) a pure rename (git pairs it, -M) is no finding at all
            os.remove(os.path.join(tmp, "src/Pl/pl_new.cpp"))
            mput("src/Pl/pl_act.cpp", old_body + "extern u8 unowned_data[];\n")
            mgit("add", "-A")
            mgit("mv", "src/Pl/pl_act.cpp", "src/Pl/pl_ren.cpp")
            rc, out = mrun()
            check("(delete 4) a rename carries its identities: clean, nothing credited", (rc, moved_lines(out)),
                  (0, []))
            mgit("mv", "src/Pl/pl_ren.cpp", "src/Pl/pl_act.cpp")
        finally:
            os.chdir(old_cwd)

    # --- SPLIT credit: a deleted/shrunk file whose bytes went to N absorbers earns up to N credits (2026-09-30) ---
    def _fnd(rule, file, token, line=1):
        return {"rule": rule, "file": file, "line": line, "token": token,
                "detail": "`%s` has no registered owner" % token}

    F, G1, G2, G3 = "src/A/f.cpp", "src/A/g1.cpp", "src/A/g2.cpp", "src/A/g3.cpp"
    absorb = {F: [G1, G2]}

    def _split(rule, before, after, absorbers=absorb):
        fresh0 = added_identities(before, after)
        left, moves = apply_move_credits(fresh0, before, after, absorbers=absorbers)
        return sorted(k[1] for k in left), sorted(m["to"] for m in moves)

    base_f = [_fnd(2, F, "lbl_1")]
    check("(split 1) a deleted file split into two absorbers is credited twice",
          _split(2, base_f, [_fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")]), ([], [G1, G2]))
    check("(split 2) a non-absorber copy of the same token is refused, the absorbers still credited",
          _split(2, base_f, [_fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1"), _fnd(2, G3, "lbl_1")]),
          ([G3], [G1, G2]))
    check("(split 3) the token also new in a third file, absorbers sorted after it: the third still refuses",
          _split(2, base_f, [_fnd(2, "src/A/a0.cpp", "lbl_1"), _fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")]),
          (["src/A/a0.cpp"], [G1, G2]))
    base_1 = [dict(_fnd(1, F, "T"), detail="type `T` is defined in X and again in Y")]
    check("(split 4) a rule-1 duplicate is never credited by absorption",
          _split(1, base_1, [dict(_fnd(1, G1, "T"), detail=base_1[0]["detail"]),
                              dict(_fnd(1, G2, "T"), detail=base_1[0]["detail"])]),
          ([G2], [G1]))
    check("(split 5) a copy that leaves the original intact earns nothing",
          _split(2, base_f, [_fnd(2, F, "lbl_1"), _fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")]), ([G1, G2], []))
    check("(split 6) an identity new in an absorber that F never carried earns nothing",
          _split(2, base_f, [_fnd(2, G1, "lbl_9")]), ([G1], []))
    check("(split 7) no absorber map: the plain one-credit-per-removal rule",
          _split(2, base_f, [_fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")], absorbers={}), ([G2], [G1]))
    _fr, _mv = apply_move_credits(added_identities(base_f, [_fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")]), base_f,
                                  [_fnd(2, G1, "lbl_1"), _fnd(2, G2, "lbl_1")], absorbers=absorb)
    check("(split 8) the gate log names the split and its absorbers",
          [ln.strip() for ln in move_credit_lines(_mv)][1:],
          ["moved (split across 2 absorbers) rule 2: lbl_1: src/A/f.cpp -> src/A/g1.cpp, src/A/g2.cpp"])
    check("... and the --json moved entries carry the split flag", [m.get("split") for m in _mv], [True, True])
    check("file_absorbers maps units to files by stem and skips unknown units",
          file_absorbers({"A/g1": ["A/f", "A/zz"], "A/g2": ["A/f"], "A/g3": ["A/zz"]}, [F], [G1, G2, G3]),
          {F: [G1, G2]})

    # --- MOVE + RENAME: a finding that moved file while the batch renamed its map row (2026-10-04) ---------
    # The L3 incident: `lbl_80794868` -> `frame_counter` in symbols.txt, the rule-12 extern moved from
    # src/main.cpp to include/unsplit/unknown.h; the removal was keyed under the old spelling, the addition under
    # the new, so the move read as +1.
    def _r12(file, token, line=1):
        return {"rule": 12, "file": file, "line": line, "token": token,
                "detail": "`%s` is unowned data - no registered range covers `.sbss:0x80794868`" % token}

    ren = {"lbl_80794868": "frame_counter"}
    M, U = "src/main.cpp", "include/unsplit/unknown.h"

    def _mr(before, after, symbols=ren):
        fresh0 = added_identities(before, after, symbols)
        left, moves = apply_move_credits(fresh0, before, after, symbols)
        return ({k: [f["token"] for f in v] for k, v in left.items()},
                [(m["rule"], m["token"], m["from"], m["to"]) for m in moves])

    check("(move+rename 1) a finding moved to another file under its renamed spelling is credited as a move",
          _mr([_r12(M, "lbl_80794868")], [_r12(U, "frame_counter")]),
          ({}, [(12, "frame_counter", M, U)]))
    check("... and without the rename map it is still one removal plus one addition",
          _mr([_r12(M, "lbl_80794868")], [_r12(U, "frame_counter")], symbols={}),
          ({(12, U): ["frame_counter"]}, []))
    check("(move+rename 2) renamed in place, same file: a rename, neither added nor moved",
          _mr([_r12(M, "lbl_80794868")], [_r12(M, "frame_counter")]), ({}, []))
    check("(move+rename 3) renamed and moved but under another rule: not credited",
          _mr([_r12(M, "lbl_80794868")], [dict(_fnd(2, U, "frame_counter"))]),
          ({(2, U): ["frame_counter"]}, []))
    check("(move+rename 4) a genuinely new finding beside a credited move is still added",
          _mr([_r12(M, "lbl_80794868")], [_r12(U, "frame_counter"), _r12(U, "other_data")]),
          ({(12, U): ["other_data"]}, [(12, "frame_counter", M, U)]))
    check("(move+rename 5) a renamed copy that leaves the original in place earns nothing",
          _mr([_r12(M, "lbl_80794868")], [_r12(M, "frame_counter"), _r12(U, "frame_counter")]),
          ({(12, U): ["frame_counter"]}, []))

    # --- --findings: the listing of the --diff / --budget set, filtered ----------------------------------
    listed = [_af(7, "src/Network/b.cpp", 9, "fn_00000009"), _af(2, "src/Network/a.cpp", 4, "lbl_1"),
              _af(7, "src/Network/a.cpp", 2, "fn_00000002"), _af(7, "src/Pl/p.cpp", 1, "fn_00000001"),
              _af(2, "include/Network/n.h", 3, "x")]
    check("--findings: no filter keeps every finding, sorted by file then line",
          [(f["file"], f["line"]) for f in select_findings(listed)],
          [("include/Network/n.h", 3), ("src/Network/a.cpp", 2), ("src/Network/a.cpp", 4),
           ("src/Network/b.cpp", 9), ("src/Pl/p.cpp", 1)])
    check("--findings --path GLOB --rule N keeps the intersection",
          [f["token"] for f in select_findings(listed, ["src/Network/*"], [7])], ["fn_00000002", "fn_00000009"])
    check("--findings --path with no wildcard is a directory prefix, not a substring",
          [f["file"] for f in select_findings(listed, ["src/Net"])], [])
    check("--findings: repeated --path and --rule are unions",
          len(select_findings(listed, ["src/Pl", "include/*"], [2, 7])), 2)
    _buf = io.StringIO()
    with contextlib.redirect_stdout(_buf):
        print_findings_listing(select_findings(listed, rules=[2]), "budget", True, None, [2])
    _js = json.loads(_buf.getvalue())
    check("--findings --json is the lib.findings schema with each row a Finding",
          (_js["tool"], _js["ok"], [r["token"] for r in _js["rows"]], _js["by_rule"], _js["filters"]["rule"]),
          ("stylelint", False, ["x", "lbl_1"], {"2": 2}, [2]))
    _buf = io.StringIO()
    with contextlib.redirect_stdout(_buf):
        print_findings_listing(select_findings(listed, ["src/Pl"]), "budget", False)
    check("--findings text: one `file:line  rule N  token` line, then the count",
          _buf.getvalue().splitlines(),
          ["src/Pl/p.cpp:1  rule 7  fn_00000001", "stylelint: 1 finding(s) in the budget set (rule 7: 1)"])

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0
