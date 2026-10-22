"""integrate.py, the 2026-10-22 gaps (docs/pipeline.md 16.5): a C++-typed declaration goes to a leaf header, a leaf
header named after a renamed symbol's old stem follows the rename, a request whose row main already named uses main's
name, a float parameter placed differently from the owner's prototype is reported, scratch stays inside the tree, and
`--gate-ref` judges the gate pre-run against the named ref."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile
import unittest.mock as mock

from tools.lib import requests as R
from tools.lib import testing
from tools.lib.project import Ownership
from tools.units import integrate as ig

TIER = "fixture"


def write(root, rel, text):
    p = os.path.join(root, *rel.split("/"))
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def read(root, rel):
    with open(os.path.join(root, *rel.split("/")), encoding="utf-8", newline="") as fh:
        return fh.read()


TYPES = "typedef int s32;\ntypedef float f32;\ntypedef unsigned char u8;\n"
#: An owner header C units read too: guarded `extern "C"`, no C++ construct (`c_header` is True).
EM_H = ('#ifndef EM_H\n#define EM_H\n#include "types.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n\n'
        "s32 em_count(void);\n\n#ifdef __cplusplus\n}\n#endif\n#endif\n")
VEC_H = ("#ifndef VEC_H\n#define VEC_H\n#include \"types.h\"\nnamespace nw4r {\nnamespace math {\n"
         "struct VEC3 { f32 x; f32 y; f32 z; };\n}\n}\n#endif\n")


def test_cpp_types_go_to_a_leaf_header(c):
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "src/types.h", TYPES)
        write(tmp, "src/enemy/em.h", EM_H)
        write(tmp, "src/nw4r/math/vec.h", VEC_H)
        write(tmp, "src/enemy/em.cpp", '#include "enemy/em.h"\n#include "nw4r/math/vec.h"\n'
                                       "void em_pos_set(const nw4r::math::VEC3& v) {}\n")
        write(tmp, "src/Lane/lane.cpp", '#include "types.h"\n#include "nw4r/math/vec.h"\n\n'
                                        "void em_pos_set(const nw4r::math::VEC3& v);\n\n"
                                        "void f(nw4r::math::VEC3& v) { em_pos_set(v); }\n")
        files = ig.Files(tmp)
        op = ig.DeclOp("a#1", "em_pos_set", "em_pos_set__FRCQ34nw4r4math4VEC3", 0x80010000, ".text", "enemy/em.cpp",
                       "src/enemy/em.h", cpp=True)
        fails = ig.apply_decls(files, [op], {"src/Lane/lane.cpp"}, set())
        files.flush()
        c.check("a nw4r:: prototype the C-read owner header cannot see is not refused: it goes to the leaf header",
                (fails, op.header, op.leaf, op.result.get("leaf_includes")),
                ({}, "src/enemy/em_pos_set.h", True, ["src/nw4r/math/vec.h"]))
        leaf, em_h, lane = read(tmp, "src/enemy/em_pos_set.h"), read(tmp, "src/enemy/em.h"), read(tmp, "src/Lane/lane.cpp")
        c.check("... the leaf includes the type's header and declares it with C++ linkage, guarded",
                ('#include "nw4r/math/vec.h"' in leaf, "void em_pos_set(const nw4r::math::VEC3& v);" in leaf,
                 'extern "C"' in leaf, "#ifndef MHTRI_ENEMY_EM_POS_SET_H" in leaf), (True, True, False, True))
        c.check("... the owner header is untouched", em_h, EM_H)
        c.check("... and the lane's local declaration became the leaf's include",
                ('#include "enemy/em_pos_set.h"' in lane, "void em_pos_set(const" in lane), (True, False))
        again = ig.Files(tmp)
        op2 = ig.DeclOp("a#1", "em_pos_set", "em_pos_set__FRCQ34nw4r4math4VEC3", 0x80010000, ".text", "enemy/em.cpp",
                        "src/enemy/em.h", cpp=True)
        c.check("a rerun adopts the leaf and changes nothing",
                (ig.apply_decls(again, [op2], {"src/Lane/lane.cpp"}, set()), again.changed()), ({}, []))
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "src/types.h", TYPES)
        write(tmp, "src/C/clib.h", EM_H)
        write(tmp, "src/nw4r/math/vec.h", VEC_H)
        write(tmp, "src/Lane/lane.cpp", "void cpos(const nw4r::math::VEC3* v);\nvoid f() { cpos(0); }\n")
        op = ig.DeclOp("a#2", "cpos", "cpos", 0x80001000, ".text", "C/clib.c", "src/C/clib.h", cpp=False,
                       hint="void cpos(const nw4r::math::VEC3* v);")
        fails = ig.apply_decls(ig.Files(tmp), [op], {"src/Lane/lane.cpp"}, set())
        c.check("a C owner still refuses a C++ type", fails.get("a#2", [""])[0], "cpos: a C++ prototype for the C owner "
                                                                                "C/clib.c")


def test_leaf_header_follows_the_rename(c):
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "src/types.h", TYPES)
        write(tmp, "src/enemy/fn_8011D448.h", "#ifndef MHTRI_ENEMY_FN_8011D448_H\n#define MHTRI_ENEMY_FN_8011D448_H\n"
                                              "void em_hub_tick(void);\n#endif /* MHTRI_ENEMY_FN_8011D448_H */\n")
        write(tmp, "src/enemy/em_a.cpp", '#include "enemy/fn_8011D448.h"\nvoid f(void) { em_hub_tick(); }\n')
        write(tmp, "src/enemy/em_b.cpp", '#include "fn_8011D448.h"   /* em_hub_tick */\n')
        write(tmp, "src/other/x.cpp", '#include "other/fn_8011D448.h"\n')
        write(tmp, "src/other/fn_8011D448.h", "int other;\n")
        write(tmp, "src/enemy/fn_80120000.h", "void u(void);\n")
        write(tmp, "src/enemy/fn_80120000.cpp", '#include "enemy/fn_80120000.h"\n')
        moved, notes = ig.rename_leaf_headers(tmp, [("fn_8011D448", "em_hub_tick"),
                                                    ("fn_80120000", "em_unit_main")])
        c.check("the leaf header named after the old stem is renamed after the new name",
                (sorted(moved), os.path.exists(os.path.join(tmp, "src", "enemy", "fn_8011D448.h"))),
                ([("src/enemy/fn_8011D448.h", "src/enemy/em_hub_tick.h"),
                  ("src/other/fn_8011D448.h", "src/other/em_hub_tick.h")], False))
        c.check("... its guard follows", read(tmp, "src/enemy/em_hub_tick.h").count("MHTRI_ENEMY_EM_HUB_TICK_H"), 3)
        c.check("... and every include that resolves to it is respelled, a same-directory one too, never another "
                "directory's namesake",
                (read(tmp, "src/enemy/em_a.cpp").splitlines()[0], read(tmp, "src/enemy/em_b.cpp").splitlines()[0],
                 read(tmp, "src/other/x.cpp").splitlines()[0]),
                ('#include "enemy/em_hub_tick.h"', '#include "em_hub_tick.h"   /* em_hub_tick */',
                 '#include "other/em_hub_tick.h"'))
        c.check("a unit's own header is left (a registration move), with a note",
                (os.path.exists(os.path.join(tmp, "src", "enemy", "fn_80120000.h")),
                 notes), (True, ["src/enemy/fn_80120000.h is unit enemy/fn_80120000's own header: renaming it is a "
                                 "registration move"]))


SYMBOLS = ("em_hub_tick = .text:0x8011D448; // type:function size:0x40\n"
           "fn_80120000 = .text:0x80120000; // type:function size:0x40\n")
SPLITS = "enemy/em_hub.cpp:\n\t.text       start:0x8011D000 end:0x80121000\n"


def test_main_already_named_it(c):
    own = Ownership.from_texts(SYMBOLS, SPLITS)
    r = R.from_entry({"id": "lane#4", "kind": "rename", "symbol": "fn_8011D448", "proposed_name": "em_hub_update",
                      "prototype": "void em_hub_update(void);", "evidence": "x"})
    it = ig.plan([r], own, {}, set(), set(), {})[0]
    c.check("a request whose generated row main has named since renames nothing: main's name stands",
            (it.state, it.renames), ("apply", []))
    c.check("... it is reported as such", it.notes,
            ["main already named it em_hub_tick (fn_8011D448, the request asked em_hub_update): using em_hub_tick"])
    c.check("... and the lane's spellings (the filed one and its own proposal) follow main's name",
            (it.rewrites, it.decls[0].name, it.decls[0].hint),
            ([("fn_8011D448", "em_hub_tick"), ("em_hub_update", "em_hub_tick")], "em_hub_tick",
             "void em_hub_tick(void);"))
    still = R.from_entry({"id": "lane#5", "kind": "rename", "symbol": "fn_80120000", "proposed_name": "em_hub_draw",
                          "evidence": "x"})
    it = ig.plan([still], own, {}, set(), set(), {})[0]
    c.check("a row main has not named is still renamed", (it.renames, it.notes),
            ([("fn_80120000", "em_hub_draw")], []))


def test_float_order(c):
    c.check("param_kinds: a by-value float is f, a pointer to one is not",
            ig.param_kinds("void f(ENEMY* e, f32 a, const f32* b, s32 c, double d);", "f"), ["g", "f", "g", "g", "f"])
    c.check("no mismatch when the floats sit where the owner has them",
            ig.float_order_mismatch("void f(ENEMY* w, f32 s);", "void f(ENEMY* e, f32 speed);", "f"), None)
    with tempfile.TemporaryDirectory() as tmp:
        write(tmp, "src/types.h", TYPES)
        write(tmp, "src/enemy/em.h", EM_H)
        write(tmp, "src/enemy/em.cpp", '#include "enemy/em.h"\nvoid em_wave(u8* w, f32 amp, s32 n) {}\n')
        write(tmp, "src/Lane/lane.cpp", "void em_wave(u8* w, s32 n, f32 amp);\nvoid f() { em_wave(0, 1, 2.0f); }\n")
        files = ig.Files(tmp)
        op = ig.DeclOp("a#7", "em_wave", "em_wave", 0x80010000, ".text", "enemy/em.cpp", "src/enemy/em.h", cpp=False,
                       hint="void em_wave(u8* w, s32 n, f32 amp);")
        fails = ig.apply_decls(files, [op], {"src/Lane/lane.cpp"}, set())
        c.check("a float placed elsewhere than in the owner's definition is reported, not refused",
                (fails, op.result.get("float_order")),
                ({}, ["em_wave: float parameter(s) at position [3] in `void em_wave(u8* w, s32 n, f32 amp);`, at [2] "
                      "in the owner's `void em_wave(u8* w, f32 amp, s32 n);`"]))
        c.check("... the owner's prototype is what is declared", "void em_wave(u8* w, f32 amp, s32 n);" in
                files.get("src/enemy/em.h"), True)


def test_scratch_inside_the_tree(c):
    with tempfile.TemporaryDirectory() as tmp:
        inside = os.path.join(tmp, ".pi", "tmp", "lane")
        with mock.patch.dict(os.environ, {"TMPDIR": "", "TEMP": inside, "TMP": tempfile.gettempdir()}):
            c.check("a TEMP that points inside the tree is honoured", ig.scratch_dir(tmp), os.path.abspath(inside))
        with mock.patch.dict(os.environ, {"TMPDIR": "", "TEMP": tempfile.gettempdir(), "TMP": tempfile.gettempdir()}):
            got = ig.scratch_dir(tmp)
        c.check("... one outside it is ignored: build/tmp/integrate inside the tree",
                os.path.normcase(got), os.path.normcase(os.path.join(tmp, "build", "tmp", "integrate")))


def _run_with_gate(extra):
    """`integrate.main` on a fixture repository with the build faked green and the gate pre-run recorded; `main` has
    moved one commit past the integrate branch's base. -> (exit, the ref the pre-run was given, the base)."""
    import contextlib
    import io
    import subprocess
    from tools.tests.units.test_integrate import RUN_FILES
    seen = {}
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        fx.init()
        fx.commit(RUN_FILES, "base")
        base = fx.head()
        fx.branch("work", checkout=True)
        fx.checkout("main")
        fx.commit({"CLAUDE.md": "main moved\n"}, "main moves on")
        fx.checkout("work")
        rq = os.path.join(tmp, "lane-a-requests.json")
        with open(rq, "w", encoding="utf-8") as fh:
            fh.write('{"id": "lane-a#1", "kind": "decl", "symbol": "SOGetHostID", "evidence": "x"}\n')
        real_tool = ig.run_tool

        def fake_tool(script, *args, **kw):
            if script.endswith("commitlint.py"):
                return subprocess.CompletedProcess([script], 0, "ok", "")
            return real_tool(script, *args, **kw)

        def fake_gate(root, ref, units):
            seen["ref"] = ref
            return {}
        env = {"GIT_AUTHOR_NAME": "fx", "GIT_AUTHOR_EMAIL": "fx@example.invalid", "GIT_COMMITTER_NAME": "fx",
               "GIT_COMMITTER_EMAIL": "fx@example.invalid"}
        with mock.patch.object(ig, "run_build", lambda root, log: (True, "", 0.0)), \
                mock.patch.object(ig, "run_tool", fake_tool), mock.patch.object(ig, "gate_prerun", fake_gate), \
                mock.patch.object(ig, "objects_same", lambda *a: {"compared": False}), \
                mock.patch.dict(os.environ, env), contextlib.redirect_stdout(io.StringIO()):
            try:
                code = ig.main(["--requests", rq, "--no-claims", "--no-mangle-check", "--branch", "integrate/fx"]
                               + extra, root=str(fx.root))
            except SystemExit as exc:                 # an unknown option is argparse's exit 2
                code = exc.code
    return code, seen.get("ref"), base


def test_gate_ref(c):
    code, ref, base = _run_with_gate([])
    c.check("by default the gate pre-run judges against the integrate branch's base (the cut point)",
            (code, ref), (0, base))
    code, ref, _base = _run_with_gate(["--gate-ref", "main"])
    c.check("--gate-ref main judges it against main, as land.py will", (code, ref), (0, "main"))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
