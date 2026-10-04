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


def test_live_units(c):
    rows = [{"unit": "Network/A", "merged": False},                                     # an old row: no `units`
            {"unit": "(unregistered)", "merged": False},
            {"unit": "lane/net2-l2", "registered": False, "units": ["Network/B", "Network/C"], "merged": False},
            {"unit": "cluster/net", "units": ["Network/D"], "merged": True}]
    c.check("live units: a row's unit and every unit a lane's unit set holds; merged rows and the placeholder "
            "are not", ig.units_of_rows(rows), {"Network/A", "lane/net2-l2", "Network/B", "Network/C"})
    own = Ownership.from_texts(SYMBOLS, SPLITS)
    items = ig.plan([req("rename", "fn_8051E864", "SOInit", n=1)], own, {}, ig.units_of_rows(
        [{"unit": "lane/x", "registered": False, "units": ["SO/soi"], "merged": False}]), set(), {})
    c.check("... and a spawned lane's unit set defers its requests", items[0].state, "deferred")


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


# --- the 2026-10-04 pilot gaps ------------------------------------------------------------------------------------

def test_stopgap_keeps_types(c):
    text = ("/* STOPGAP-BEGIN(a#4) */\ntypedef u32 (*CircleInfoSetSender)(NetworkInstance* i, u32 id);\n"
            "/* STOPGAP-END(a#4) */\n/* STOPGAP-BEGIN(a#5) */\nextern \"C\" u32 sendReqCircleListLayer(void* s);\n"
            "extern u8 lbl_flag;\n/* STOPGAP-END(a#5) */\nvoid f(void);\n")
    new, done, _points = ig.remove_stopgap_blocks(text, {"a#4", "a#5"})
    c.check("a STOPGAP block's declarations go, a typedef the code casts through stays (the L2 CircleInfoSetSender "
            "break)", (new, sorted(done)),
            ("typedef u32 (*CircleInfoSetSender)(NetworkInstance* i, u32 id);\nvoid f(void);\n", ["a#4", "a#5"]))


def test_rename_declaration_from_its_stopgap(c):
    """The L2 batch's 9 renames: the lane declared each renamed callee in a STOPGAP block; step 0 removed the block, so
    the rename's declaration found no lane site, was skipped, and every caller failed to compile."""
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "include/types.h", "typedef unsigned int u32;\n")
        write(tmp, "include/Net/io.h", "#ifndef IO_H\n#define IO_H\n#include \"types.h\"\n#endif\n")
        write(tmp, "src/Net/io.cpp", '#include "Net/io.h"\n')                    # the owner: no body to read
        write(tmp, "src/Lane/pat.cpp", '#include "types.h"\n\n/* STOPGAP-BEGIN(a#3) */\n'
                                      'u32 sendReqCircleKick(u32 self, const char* user);\n/* STOPGAP-END(a#3) */\n\n'
                                      "u32 f(void) { return sendReqCircleKick(0, 0); }\n")
        files = ig.Files(tmp)
        op = ig.DeclOp("a#3", "sendReqCircleKick", "sendReqCircleKick", 0x80402BCC, ".text", "Net/io.cpp",
                       "include/Net/io.h", cpp=True, implicit=True)
        c.check("before: a caller would see no declaration once the block goes", ig.needs_declaration(
            ig.Files(tmp), op), False)
        fails = ig.apply_decls(files, [op], {"src/Lane/pat.cpp"}, {"a#3"})
        files.flush()
        io_h, pat = read(tmp, "include/Net/io.h"), read(tmp, "src/Lane/pat.cpp")
        c.check("the rename's declaration is written to the owner header", (fails, op.result.get("skipped"),
                "u32 sendReqCircleKick(u32 self, const char* user);" in io_h), ({}, None, True))
        c.check("... from the lane's STOPGAP spelling, named as such", op.result.get("source"),
                "the lane's declaration (src/Lane/pat.cpp, its STOPGAP block)")
        c.check("... and the caller includes the owner header", '#include "Net/io.h"' in pat, True)
        again = ig.DeclOp("a#3", "sendReqCircleKick", "sendReqCircleKick", 0x80402BCC, ".text", "Net/io.cpp",
                          "include/Net/io.h", cpp=True, implicit=True)
        f2 = ig.Files(tmp)
        ig.apply_decls(f2, [again], {"src/Lane/pat.cpp"}, set())
        c.check("a rename whose callers all see a declaration is still skipped", again.result.get("skipped", "")
                .startswith("no declaration in the lane's scope"), True)


def test_decided_name_rewrites_the_prototype(c):
    own = Ownership.from_texts(SYMBOLS + "fn_803FDC80 = .text:0x803FDC80; // type:function size:0x10\n",
                               SPLITS + "Network/pat.cpp:\n\t.text       start:0x803FD000 end:0x803FE000\n")
    r = R.from_entry({"id": "a#18", "kind": "rename", "symbol": "fn_803FDC80", "proposed_name": "isCircleListBusy",
                      "prototype": "s32 isCircleListBusy(NetworkInstance* self);", "evidence": "x",
                      "confidence": "guess"})
    items = ig.plan([r], own, {"fn_803FDC80": "testAndSet611b"}, set(), set(), {})
    c.check("a --names decision renames the request's prototype too, not only the fn_ spelling",
            (items[0].renames, items[0].decls[0].hint),
            ([("fn_803FDC80", "testAndSet611b")], "s32 testAndSet611b(NetworkInstance* self);"))


def test_already_applied(c):
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "config/RMHE08/symbols.txt", SYMBOLS)
        write(tmp, "config/RMHE08/splits.txt", SPLITS)
        write(tmp, "include/types.h", "typedef unsigned int u32;\n")
        write(tmp, "include/SO/soi.h", SOI_H.replace("s32 SOSend(s32 fd);", "s32 SOSend(s32 fd);\nu32 SOGetHostID(void);"))
        write(tmp, "src/SO/soi.cpp", '#include "SO/soi.h"\nextern "C" u32 SOGetHostID(void) { return 0; }\n')
        write(tmp, "src/Lane/lane.cpp", '#include "SO/soi.h"\nvoid f(void) { SOGetHostID(); }\n')
        own = Ownership.from_texts(SYMBOLS, SPLITS)
        reqs = [req("decl", "SOGetHostID", "u32 SOGetHostID(void);", n=4),
                req("rename", "fn_80070000", "laneHelper", n=5)]
        items = ig.plan(reqs, own, {}, set(), {"Lane/lane"}, {})
        c.check("before: both plan to apply", [i.state for i in items], ["apply", "apply"])
        c.check("a declaration already in the owner header is marked already applied (the tree)",
                (ig.mark_applied(tmp, items, {"src/Lane/lane.cpp"}), items[0].state, items[0].why),
                (1, "done", "already applied (the tree): SOGetHostID in include/SO/soi.h"))
        c.check("... a rename still to do is not", items[1].state, "apply")
        write(tmp, "src/Lane/lane.cpp", '#include "SO/soi.h"\n/* STOPGAP-BEGIN(lane-a#4) */\nu32 SOGetHostID(void);\n'
                                        '/* STOPGAP-END(lane-a#4) */\nvoid f(void) { SOGetHostID(); }\n')
        items = ig.plan(reqs[:1], own, {}, set(), {"Lane/lane"}, {})
        c.check("... while its STOPGAP block is still in the tree, it is not applied", (ig.mark_applied(
            tmp, items, {"src/Lane/lane.cpp"}), items[0].state), (0, "apply"))
        write(tmp, "src/Lane/lane.cpp", '#include "SO/SOGetHostID.h"\nvoid f(void) { SOGetHostID(); }\n')
        write(tmp, "include/SO/soi.h", SOI_H)
        write(tmp, "include/SO/SOGetHostID.h", "#include \"types.h\"\nu32 SOGetHostID(void);\n")
        items = ig.plan(reqs[:1], own, {}, set(), {"Lane/lane"}, {})
        c.check("... and one its leaf header already declares is applied too",
                (ig.mark_applied(tmp, items, set()), items[0].why),
                (1, "already applied (the tree): SOGetHostID in include/SO/SOGetHostID.h"))


def test_narrow(c):
    def items_with(*ops):
        out = []
        for i, op in enumerate(ops):
            it = ig.Item(req("decl", op.name, n=i + 1), "mechanical", "x", state="apply")
            it.decls = [op]
            out.append(it)
        return out
    skipped = ig.DeclOp("lane-a#1", "copyFmpSlot", "copyFmpSlot", 1, ".text", "N/p.cpp", "include/N/p.h", cpp=False,
                        implicit=True)
    skipped.result = {"skipped": "no declaration in the lane's scope to move"}
    wrong = ig.DeclOp("lane-a#2", "getErr", "getErr", 2, ".text", "N/p.cpp", "include/N/p.h", cpp=False)
    wrong.result = {"prototype": "s32 getErr(u32* x);"}
    items = items_with(skipped, wrong)
    errs = [{"file": "src/L/l.cpp", "line": 5, "text": "copyFmpSlot(a);", "message": "undefined identifier 'copyFmpSlot'"},
            {"file": "src/L/l.cpp", "line": 9, "text": "getErr(0, 1);",
             "message": "function call 'getErr(int, int)' does not match"}]
    v = ig.narrow(items, errs, ["build/RMHE08/src/L/l.o"])
    c.check("one round handles every error: the needed declaration is forced, the wrong one excluded",
            (v["forced"], v["excluded"], skipped.force, items[1].state), (["copyFmpSlot"], ["getErr"], True, "judgement"))
    v = ig.narrow(items, errs[:1], ["build/RMHE08/src/L/l.o"])
    c.check("... a forced declaration still undefined reverts its request", (v["reverted"], items[0].state),
            (["lane-a#1"], "reverted"))
    clash = ig.DeclOp("lane-a#3", "em_net_recv", "em_net_recv", 3, ".text", "enemy/em.cpp", "include/enemy/em.h",
                      cpp=False)
    clash.result = {"prototype": "void em_net_recv(void);", "already_declared": False,
                    "includes_added": ["src/L/l.cpp"]}
    items = items_with(clash)
    v = ig.narrow(items, [{"file": "include\\enemy\\em.h", "line": 12, "text": "struct EnemyWork {",
                           "message": "struct/union/enum/class tag 'EnemyWork' redefined", "object": "x"}], ["x"])
    c.check("a redefinition in the owner header moves the declaration to its leaf header",
            (v["leaf"], clash.header, clash.full_header, clash.leaf, items[0].state),
            (["em_net_recv"], "include/enemy/em_net_recv.h", "include/enemy/em.h", True, "apply"))


def test_error_command(c):
    toks = ["cmd", "/c", "mwcceppc.exe", "-O4", "-maxerrors", "1", "-MMD", "-c", "src/a.cpp", "-o", "build/RMHE08/src",
            "&&", "python", "objalign.py"]
    c.check("the diagnostic compile reports every error and writes to scratch, the build untouched",
            ig.error_command(toks, "T"),
            ["cmd", "/c", "mwcceppc.exe", "-O4", "-maxerrors", "0", "-MMD", "-c", "src/a.cpp", "-o", "T"])
    c.check("... a command without -maxerrors gets one", ig.error_command(["cc", "-c", "a", "-o", "b"], "T"),
            ["cc", "-c", "a", "-maxerrors", "0", "-o", "T"])


def test_config_edit(c):
    old = "object: x\nblock_relocations:\n- target: a\n  end: b\n# a comment of the next key\nfill_gaps: true\n"
    it = ig.Item(R.from_entry({"id": "a#42", "kind": "config", "symbol": "updateSession", "evidence": "x",
                               "proposed": "block_relocations: 0x803D7564 and 0x803D7568 (the constant 0x80060034)"}),
                 "mechanical", "x", state="apply")
    it.config = it.req.proposed
    new, added, key, refusal = ig.config_edit(old, it)
    c.check("a prose block lands at the end of its key's block, through the guard's rule",
            (new, added, key, refusal),
            ("object: x\nblock_relocations:\n- target: a\n  end: b\n- source: .text:0x803D7564\n  end: .text:0x803D756C\n"
             "# a comment of the next key\nfill_gaps: true\n", 1, "block_relocations", None))
    c.check("... and a second pass adds nothing", ig.config_edit(new, it)[1], 0)


# --- run(): the refusals and the commit, on a fixture repository with the build faked -------------------------------

RUN_FILES = {
    "configure.py": "", ".gitignore": "build/\n",
    "config/RMHE08/symbols.txt": "SOGetHostID = .text:0x8051F000; // type:function size:0x8\n",
    "config/RMHE08/splits.txt": ("SO/soi.cpp:\n\t.text       start:0x8051E000 end:0x80520000\n"
                                 "Lane/lane.cpp:\n\t.text       start:0x80070000 end:0x80071000\n"),
    "include/types.h": "typedef unsigned int u32;\ntypedef int s32;\n",
    "include/SO/soi.h": SOI_H,
    "src/SO/soi.cpp": '#include "SO/soi.h"\nextern "C" u32 SOGetHostID(void) { return 0; }\n',
    "src/Lane/lane.cpp": ('#include "types.h"\n\n/* STOPGAP-BEGIN(lane-a#1) */\nextern "C" u32 SOGetHostID(void);\n'
                          '/* STOPGAP-END(lane-a#1) */\n\nvoid f(void) { SOGetHostID(); }\n'),
}


def _run(fx, tmp, builds, errors=None):
    """`integrate.main` on the fresh fixture repo `fx` (the request file in `tmp`): `builds` are the verdicts the
    faked `run_build` returns in turn."""
    import contextlib
    import io
    import subprocess
    import unittest.mock as mock
    fx.init()
    fx.commit(RUN_FILES, "base")
    base = fx.git("rev-parse", "HEAD").strip()
    rq = os.path.join(tmp, "lane-a-requests.json")
    with open(rq, "w", encoding="utf-8") as fh:
        fh.write('{"id": "lane-a#1", "kind": "decl", "symbol": "SOGetHostID", "evidence": "x"}\n')
    verdicts = list(builds)

    def fake_build(root, log):
        ok = verdicts.pop(0) if verdicts else True
        return ok, "" if ok else "FAILED: build/RMHE08/src/Other/x.o\n", 0.0

    real_tool = ig.run_tool

    def fake_tool(script, *args, **kw):
        if script.endswith("commitlint.py"):
            return subprocess.CompletedProcess([script], 0, "commitlint: ok", "")
        return real_tool(script, *args, **kw)
    out = io.StringIO()
    env = {"GIT_AUTHOR_NAME": "fx", "GIT_AUTHOR_EMAIL": "fx@example.invalid", "GIT_COMMITTER_NAME": "fx",
           "GIT_COMMITTER_EMAIL": "fx@example.invalid"}
    with mock.patch.object(ig, "run_build", fake_build), mock.patch.object(ig, "run_tool", fake_tool), \
            mock.patch.object(ig, "object_errors", lambda root, failed: (list(errors or []), [])), \
            mock.patch.dict(os.environ, env), contextlib.redirect_stdout(out):
        code = ig.main(["--requests", rq, "--no-claims", "--no-gate", "--no-mangle-check", "--branch", "integrate/fx"],
                       root=str(fx.root))
    return base, code, out.getvalue()


def test_run_refusals_and_commit(c):
    unattributable = [{"file": "src/Other/x.cpp", "line": 3, "text": "y = ;", "message": "expression syntax error",
                       "object": "build/RMHE08/src/Other/x.o"}]
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        base, code, out = _run(fx, tmp, [True, False], unattributable)
        status = fx.git("status", "--porcelain").splitlines()
        c.check("(a) a build that never goes green is REFUSED with a non-zero exit",
                (code, "NOTHING was committed" in out), (1, True))
        c.check("... no commit was made on the integrate branch", fx.git("rev-parse", "HEAD").strip(), base)
        c.check("... and the last attempt is left in the working tree for the operator",
                sorted(line[3:] for line in status), ["include/SO/soi.h", "src/Lane/lane.cpp"])
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        base, code, out = _run(fx, tmp, [False], unattributable)
        c.check("(b) a base that does not compile is refused before anything is applied",
                (code, "the base" in out and "does not compile" in out, fx.git("status", "--porcelain")),
                (1, True, ""))
        c.check("... the integrate branch it cut is removed again",
                (fx.git("branch", "--list", "integrate/fx").strip(), fx.git("rev-parse", "--abbrev-ref", "HEAD").strip()),
                ("", "main"))
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        base, code, out = _run(fx, tmp, [True, True])
        log = fx.git("log", "--format=%s", "%s..HEAD" % base).splitlines()
        c.check("a green build is committed (one commit: no renames)", (code, log),
                (0, ["game/lane: declare the integrated callees in their owners' headers"]))
        c.check("... the committed tree is the tree that built, and the tree is clean",
                ("commits_verified: true" in out, fx.git("status", "--porcelain")), (True, ""))
        c.check("(f) ... and the land line names the changed unit",
                "land.py land --branch integrate/fx --units Lane/lane" in out, True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
