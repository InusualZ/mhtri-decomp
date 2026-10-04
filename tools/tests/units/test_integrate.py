"""integrate.py: planning against a fixture map, the declaration edits on a fixture tree, the build-log reader, blame."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile

from tools.lib import requests as R
from tools.lib import testing
from tools.lib.project import Ownership
from tools.units import integrate as ig

TIER = "fixture"

SYMBOLS = "\n".join([
    "fn_8051E864 = .text:0x8051E864; // type:function size:0x40",
    "SOGetHostID = .text:0x8051F000; // type:function size:0x8",
    "fn_80060000 = .text:0x80060000; // type:function size:0x8",
    "setCircleMode__24NetworkSessionManagerPatFUc = .text:0x803DF144; // type:function size:0x10",
    "updateNetworkPat = .text:0x80419CF0; // type:function size:0x10",
    "fn_80070000 = .text:0x80070000; // type:function size:0x8",
]) + "\n"
SPLITS = ("SO/soi.cpp:\n\t.text       start:0x8051E000 end:0x80520000\n"
          "g3d/anm.cpp:\n\t.text       start:0x80060000 end:0x80061000\n"
          "Network/mgr.cpp:\n\t.text       start:0x803D7000 end:0x803E0000\n"
          "Network/lib.cpp:\n\t.text       start:0x80419000 end:0x8041A000\n"
          "Lane/lane.cpp:\n\t.text       start:0x80070000 end:0x80071000\n")


def req(kind, symbol, proposed="", evidence="x", n=1):
    return R.normalise_legacy({"kind": kind, "symbol": symbol, "proposed": proposed, "evidence": evidence}, "lane-a", n)


def test_plan(c):
    own = Ownership.from_texts(SYMBOLS, SPLITS)
    reqs = [req("rename", "fn_8051E864", "SOInit", n=1),
            req("rename", "fn_80060000", "getGlyphWidth (GUESS)", n=2),
            req("decl", "fn_803DF144", "declare in the owner header", n=3),
            req("decl", "SOGetHostID", "u32 SOGetHostID(void);", n=4),
            req("decl", "fn_80419CF0", "declare in the owner header", n=5),
            req("field", "system_w +0x7D4", "u8 net_active", n=6)]
    items = ig.plan(reqs, own, {}, set(), set(), {})
    st = {it.req.id: it.state for it in items}
    c.check("an evidenced rename applies", (st["lane-a#1"], items[0].renames), ("apply", [("fn_8051E864", "SOInit")]))
    c.check("a GUESS without a decision is deferred", st["lane-a#2"], "deferred")
    c.check("a lane's fn_ that is a member now is judgement", st["lane-a#3"], "judgement")
    c.check("... and says how to call it", "obj->setCircleMode" in items[2].why, True)
    c.check("a decl of a named symbol applies with its prototype",
            (st["lane-a#4"], items[3].decls[0].header, items[3].decls[0].hint),
            ("apply", "include/SO/soi.h", "u32 SOGetHostID(void);"))
    c.check("a stale spelling is rewritten to the map's name, no rename", (items[4].renames, items[4].rewrites),
            ([], [("fn_80419CF0", "updateNetworkPat")]))
    c.check("a field change is judgement", st["lane-a#6"], "judgement")
    items = ig.plan(reqs[:2], own, R.load_decisions(None) | {"fn_80060000": "getGlyphWidth"}, set(), set(), {})
    c.check("a decision applies the GUESS", items[1].renames, [("fn_80060000", "getGlyphWidth")])
    items = ig.plan(reqs[:1], own, {}, {"SO/soi"}, set(), {})
    c.check("an owner with a live lane defers", (items[0].state, items[0].why),
            ("deferred", "owner SO/soi.cpp has a live lane (claims)"))
    items = ig.plan(reqs[:1], own, {}, set(), set(), {"lane-a#1": {"status": "applied"}})
    c.check("an applied request (sidecar) is done", items[0].state, "done")


def write(root, rel, text):
    p = os.path.join(root, rel)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def read(root, rel):
    with open(os.path.join(root, rel), encoding="utf-8", newline="") as fh:
        return fh.read()


SOI_H = ("#ifndef SOI_H\n#define SOI_H\n#include \"types.h\"\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"
         "s32 SOSend(s32 fd);\n\n#ifdef __cplusplus\n}\n#endif\n#endif\n")
LANE = ("/* the unit */\n#include \"types.h\"\n#include \"Lane/lane.h\"\n\nextern \"C\" {\n\n"
        "/* STOPGAP (pilot rule 1): owners SO/soi.cpp. */\ns32 SOInit(void);   /* SOInit */\nu32 SOGetHostID(void);\n\n}\n\n"
        "/* STOPGAP-BEGIN(lane-a#9) */\nvoid buildProfile(Profile* p);\n/* STOPGAP-END(lane-a#9) */\n\n"
        "void f(void)\n{\n    SOInit();\n    SOGetHostID();\n    buildProfile(0);\n}\n")


def test_apply_decls(c):
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "include/types.h", "typedef int s32;\ntypedef unsigned int u32;\n")
        write(tmp, "include/SO/soi.h", SOI_H)
        write(tmp, "include/Lane/lane.h", "class Profile;\n")
        write(tmp, "src/SO/soi.cpp", '#include "SO/soi.h"\nextern "C" u32 SOGetHostID(void) { return 0; }\n')
        write(tmp, "src/Lane/lane.cpp", LANE)
        write(tmp, "src/Other/other.cpp", "u32 SOGetHostID(void);\nvoid g() { SOGetHostID(); }\n")
        files = ig.Files(tmp)
        ops = [ig.DeclOp("lane-a#1", "SOInit", "SOInit", 0x8051E864, ".text", "SO/soi.cpp", "include/SO/soi.h",
                         cpp=False, implicit=True),
               ig.DeclOp("lane-a#4", "SOGetHostID", "SOGetHostID", 0x8051F000, ".text", "SO/soi.cpp",
                         "include/SO/soi.h", cpp=False, hint="u32 SOGetHostID(void);")]
        fails = ig.apply_decls(files, ops, {"src/Lane/lane.cpp"}, {"lane-a#9"})
        files.flush()
        soi, lane = read(tmp, "include/SO/soi.h"), read(tmp, "src/Lane/lane.cpp")
        c.check("nothing refused", fails, {})
        c.check("the declarations land inside the header's extern \"C\" region, address order",
                soi.index("u32 SOGetHostID(void);") > soi.index("s32 SOInit(void);") > soi.index("SOSend")
                and soi.index("u32 SOGetHostID(void);") < soi.rindex("#ifdef __cplusplus"), True)
        c.check("the owner's definition is the prototype's authority", ops[1].result["source"], "the owner's definition")
        c.check("the lane's declaration fills in when the owner defines nothing", ops[0].result["source"],
                "the lane's declaration (src/Lane/lane.cpp)")
        c.check("the emptied legacy stopgap block is gone (linkage block and its STOPGAP comment)",
                ("extern \"C\"" in lane, "STOPGAP (pilot" in lane), (False, False))
        c.check("the STOPGAP-BEGIN block of an applied request is gone", "STOPGAP-BEGIN" in lane, False)
        c.check("the owner header is included where the lane last included", lane.split("\n")[3],
                '#include "SO/soi.h"   /* SOInit, SOGetHostID */')
        c.check("a declaration outside the lane's scope is left", read(tmp, "src/Other/other.cpp").startswith("u32 SOGet"),
                True)
        c.check("the unit header comment is untouched", lane.startswith("/* the unit */\n"), True)
        c.check("no blank-line run is left behind", "\n\n\n" in lane, False)
        again = ig.Files(tmp)
        c.check("a second pass is a no-op", (ig.apply_decls(again, ops, {"src/Lane/lane.cpp"}, set()), again.changed()),
                ({}, []))


def test_new_header_and_refusals(c):
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "include/types.h", "typedef int s32;\n")
        write(tmp, "include/Net/param.h", "typedef struct Param { s32 a; } Param;\n")
        write(tmp, "include/C/clib.h", "#ifdef __cplusplus\nextern \"C\" {\n#endif\nvoid z(void);\n#ifdef __cplusplus\n}\n"
                                       "#endif\n")
        write(tmp, "src/ef/core.cpp", "void* work_alloc(unsigned long size) { return 0; }\n")
        write(tmp, "src/Lane/lane.cpp", '#include "types.h"\nvoid* work_alloc(unsigned long size);\n'
                                        's32 cinit(Param* p);\nvoid f() { work_alloc(4); cinit(0); }\n')
        files = ig.Files(tmp)
        ops = [ig.DeclOp("a#1", "work_alloc", "work_alloc__FUl", 0x800CF7A8, ".text", "ef/core.cpp",
                         "include/ef/core.h", cpp=True),
               ig.DeclOp("a#2", "cinit", "cinit", 0x80001000, ".text", "C/clib.c", "include/C/clib.h", cpp=False)]
        fails = ig.apply_decls(files, ops, {"src/Lane/lane.cpp"}, set())
        files.flush()
        core = read(tmp, "include/ef/core.h")
        c.check("a missing owner header is created from the template", core.startswith("/*\n * Declarations for the "
                                                                                      "symbols `src/ef/core.cpp`"), True)
        c.check("... with its guard and the C++-linkage declaration outside any extern \"C\"",
                ("#ifndef MHTRI_EF_CORE_H" in core, "void* work_alloc(unsigned long size);" in core,
                 "extern \"C\"" in core), (True, True, False))
        c.check("a C header cannot be handed a type it does not see", fails.get("a#2", [""])[0].startswith(
            "cinit: the prototype names Param, which include/C/clib.h cannot see"), True)
        c.check("... and the refused declaration stays in the lane", "s32 cinit(Param* p);" in read(tmp, "src/Lane/lane.cpp"),
                True)


def test_text_helpers(c):
    c.check("definition_prototype: a function", ig.definition_prototype(
        "#pragma x on\nstatic u8 get(u8 i)\n{\n    return i;\n}\n", "get", False), "u8 get(u8 i);")
    c.check("definition_prototype: a variable", ig.definition_prototype("u8 game_mutex[0x18];\n", "game_mutex", True),
            "extern u8 game_mutex[0x18];")
    c.check("definition_prototype: a local is not a definition",
            ig.definition_prototype("void f() {\n    u8 game_mutex[4];\n}\n", "game_mutex", True), None)
    c.check("needed_types drops parameter names and builtins",
            ig.needed_types("NetworkSocket* make(NetworkSocket* s, const u32* v, Param p);", "make"),
            ["NetworkSocket", "Param"])
    c.check("c_header: a guarded header with a class is C++-only", ig.c_header(
        "#ifdef __cplusplus\n#endif\nclass A { public: int x; };\n", "N/a.cpp"), False)
    c.check("c_header: a .c owner's header is C", ig.c_header("", "N/a.c"), True)
    orig = "/* a */\nint x;\n\n/* b */\nint y;\n"
    c.check("an orphaned comment of a removed declaration goes", ig.drop_new_orphan_comments(
        "/* a */\nint x;\n\n/* b */\n\n", orig), "/* a */\nint x;\n\n\n")
    c.check("a blank run the file already had stays", ig.collapse_blank_runs("a\n\n\nb\n", "a\n\n\nb\n"), "a\n\n\nb\n")
    c.check("a blank run the edit made collapses", ig.collapse_blank_runs("a\n\n\nb\n", "a\n\nx\n\nb\n"), "a\n\nb\n")
    c.check("an include goes after the last top-level include",
            ig.add_includes('#include "a.h"\n#include "b.h"\nvoid f() {\n#include "c.inc"\n}\n', {"S/s.h": ["x"]}),
            '#include "a.h"\n#include "b.h"\n#include "S/s.h"   /* x */\nvoid f() {\n#include "c.inc"\n}\n')


LOG = """[1/3] MWCC build/RMHE08/src/A/a.o
### mwcceppc.exe Compiler:
#    File: src\\A\\a.cpp
# ---------------------------
#      12:     x = foo(1);
#   Warning:        ^
#   (10317) implicit arithmetic conversion
### mwcceppc.exe Compiler:
#    File: src\\A\\a.cpp
# ---------------------------
#      40:     block = MEMAlloc(heap, 4);
#   Error:                         ^
#   (10159) function call 'MEMAlloc(void *, long)' does not match
FAILED: build/RMHE08/src/A/a.o
"""


def test_build_log_and_blame(c):
    errs = ig.parse_build_errors(LOG)
    c.check("only the error block is read, with its message", errs, [{"file": "src/A/a.cpp", "line": 40,
            "text": "    block = MEMAlloc(heap, 4);", "message": "function call 'MEMAlloc(void *, long)' does not match"}])
    c.check("the failed objects", ig.failed_objects(LOG), ["build/RMHE08/src/A/a.o"])
    it = ig.Item(req("decl", "MEMAlloc"), "mechanical", "x", state="apply")
    op = ig.DeclOp(it.req.id, "MEMAlloc", "MEMAlloc", 1, ".text", "OS/h.c", "include/OS/h.h", cpp=False)
    op.result = {"prototype": "void* MEMAlloc(Heap* h, u32 n);"}
    it.decls = [op]
    other = ig.Item(req("decl", "Other", n=2), "mechanical", "x", state="apply")
    oop = ig.DeclOp(other.req.id, "Other", "Other", 2, ".text", "B/b.c", "include/B/b.h", cpp=False)
    oop.result = {"prototype": "void Other(void);", "includes_added": ["src/Z/z.cpp"]}
    other.decls = [oop]
    ops, hit = ig.culprits([it, other], errs, ["build/RMHE08/src/A/a.o"])
    c.check("a declaration named on the error line is blamed, nothing else", ([(i.req.id, n) for i, n in ops], hit),
            ([("lane-a#1", "MEMAlloc")], set()))
    ig.exclude_everywhere([it, other], "MEMAlloc", "the build")
    c.check("a request left with nothing to apply becomes judgement", it.state, "judgement")
    c.check("dedupe keeps a decl request's op over a rename's implicit one",
            [o.implicit for o in ig.dedupe_ops([ig.DeclOp("r#1", "x", "x", 1, ".text", "u", "h", cpp=False, implicit=True),
                                                ig.DeclOp("r#2", "x", "x", 1, ".text", "u", "h", cpp=False)])], [False])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
