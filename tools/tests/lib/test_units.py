"""lib.units: every spelling is one unit, the Unit value type, the compile command and the compile, the target object."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import json
import os
import subprocess
import tempfile
from pathlib import Path

from tools.lib import testing, units

TIER = "fixture"

SPELLINGS = ("Pl/pl_act", "Pl/pl_act.cpp", "src/Pl/pl_act.cpp", "./src/Pl/pl_act.cpp", "main/Pl/pl_act",
             "build/RMHE08/src/Pl/pl_act.o", "build/RMHE08/obj/Pl/pl_act.o", "src\\Pl\\pl_act.cpp", "/Pl/pl_act/")


def test_every_spelling_is_one_key(c):
    c.check("every spelling of duplication.md (g) has one stem", {units.stem(s) for s in SPELLINGS}, {"Pl/pl_act"})
    c.check("... and one report name", {units.report_name(s) for s in SPELLINGS}, {"main/Pl/pl_act"})
    c.check("a dotted library directory keeps its dots", units.stem("Runtime.PPCEABI.H/abort_exit.c"),
            "Runtime.PPCEABI.H/abort_exit")
    c.check("normalize keeps the extension and strips pasted prefixes",
            [units.normalize(s) for s in ("src/Pl/pl_act.cpp", "build/RMHE08/src/Pl/pl_act", "./Pl/pl_act",
                                          "main/Pl/pl_act", "src/src/Pl/pl_act.c")],
            ["Pl/pl_act.cpp", "Pl/pl_act", "Pl/pl_act", "Pl/pl_act", "Pl/pl_act.c"])
    c.check("the top-level unit `main` survives normalize", (units.normalize("main"), units.normalize("main.cpp")),
            ("main", "main.cpp"))
    c.check("object paths per side", (units.obj_rel("Pl/pl_act"), units.target_rel("src/Pl/pl_act.c")),
            (os.path.join("build", "RMHE08", "src", "Pl", "pl_act.o"), os.path.join("build", "RMHE08", "obj", "Pl", "pl_act.o")))
    c.check("with_ext defaults to .cpp", (units.with_ext("a/b"), units.with_ext("a/b.c")), ("a/b.cpp", "a/b.c"))
    c.check("unquote merges a quoted value", units.unquote(['-pragma', '"cats', 'off"', '-O4']),
            ["-pragma", "cats off", "-O4"])


def test_unit_value_type(c):
    with testing.FixtureTree() as tree:
        tree.add_unit("Camellia/camellia.c", lib="crypto", flag="Matching")
        tree.add_unit("Pl/pl_act.cpp")
        tree.add_unit("main.cpp")
        tree.write("src/Other/camellia.cpp", "/* a second camellia */\n")
        root = str(tree.root)
        for spec in ("Pl/pl_act", "src/Pl/pl_act.cpp", "main/Pl/pl_act", "build/RMHE08/src/Pl/pl_act.o", "pl_act"):
            c.check("%s resolves" % spec, units.Unit.resolve(spec, root).key, "Pl/pl_act")
        u = units.Unit.resolve("Camellia/camellia", root)
        c.check("the extension is inferred from src/", (u.ext, u.spelling, u.language), (".c", "Camellia/camellia.c", "c"))
        c.check("paths are under the tree",
                (u.source, u.obj_ours, u.obj_target),
                (os.path.join(root, "src", "Camellia", "camellia.c"),
                 os.path.join(root, "build", "RMHE08", "src", "Camellia", "camellia.o"),
                 os.path.join(root, "build", "RMHE08", "obj", "Camellia", "camellia.o")))
        c.check("registration: lib and flag come from configure.py", (u.lib, u.flag, u.module), ("crypto", "Matching", "Camellia"))
        top = units.Unit.resolve("main", root)
        c.check("a lone `main` is the top-level unit", (top.key, top.module, top.report_name), ("main", "", "main/main"))
        c.check("... and so is its qualified spelling", units.Unit.resolve("src/main.cpp", root).key, "main")
        c.raises("a bare stem two units share is refused", SystemExit, units.Unit.resolve, "camellia", root)
        c.raises("no spec with several units is refused", SystemExit, units.Unit.resolve, None, root)
        c.raises("a unit without source is refused", SystemExit, units.Unit.resolve, "Pl/nope", root)
        c.check("list() is every unit with source", sorted(x.key for x in units.Unit.list(root)),
                ["Camellia/camellia", "Other/camellia", "Pl/pl_act", "main"])


def test_source_spelling(c):
    with testing.FixtureTree() as wt, testing.FixtureTree() as main:
        wt.add_unit("NHTTP/bgnend.c")
        main.write("src/Only/inmain.c", "")
        r = (str(wt.root), str(main.root))
        c.check("the registration names the extension", units.source_spelling("src/NHTTP/bgnend", r), "NHTTP/bgnend.c")
        c.check("the file under MAIN's src/ is the fallback", units.source_spelling("Only/inmain", r), "Only/inmain.c")
        c.check("nothing known is .cpp", units.source_spelling("X/y", r), "X/y.cpp")
        c.check("an explicit extension is kept", units.source_spelling("X/y.c", r), "X/y.c")
        c.check("an explicit source path loses its tree prefix",
                units.source_spelling("ignored", r, source=os.path.join(str(wt.root), "src", "A", "b.c")), "A/b.c")


def test_command_rewrites(c):
    with tempfile.TemporaryDirectory() as tmp:
        main, wt = os.path.join(tmp, "main"), os.path.join(tmp, "wt")
        for d in (os.path.join(main, "include"), os.path.join(main, "build", "RMHE08", "include"), os.path.join(wt, "include")):
            os.makedirs(d)
        tokens = ["sjiswrap.exe", "build/compilers/mwcceppc.exe", "-i", "build/RMHE08/include", "-i", "include",
                  "-MMD", "-c", "src/Pl/pl_act.cpp", "-o", "build/RMHE08/src/Pl", "&&", "python",
                  "tools/elf/objalign.py", "build/RMHE08/src/Pl/sibling.o"]
        out, obj = units.rewrite(tokens, "Pl/pl_act", main, wt)
        incs = [out[i + 1] for i, t in enumerate(out) if t == "-i"]
        c.check("the worktree's include/ comes first even when MAIN lists its generated include first",
                incs, [os.path.join(wt, "include"), os.path.join(main, "build", "RMHE08", "include")])
        c.check("-c is the worktree's source", out[out.index("-c") + 1], os.path.join(wt, "src", "Pl", "pl_act.cpp"))
        c.check("the object is in the worktree's -o directory", obj, os.path.join(wt, "build/RMHE08/src/Pl", "pl_act.o"))
        c.check("a chained helper is pointed at this object", out[-1], os.path.abspath(obj))
        c.check("`cmd /c` is a switch, never a path", units.absolutize(["cmd", "/c", "x"], main), ["cmd", "/c", "x"])
        c.check("a sibling's line is retargeted (-o, -lang)",
                units.retarget(["-lang=c++", "-o", "build/RMHE08/src/X"], "Pl/u.c"),
                ["-lang=c", "-o", os.path.join("build", "RMHE08", "src", "Pl")])


def _ninja(lines_by_tree: dict):
    def run(argv, cwd=None, **kw):
        if argv[:3] == ["ninja", "-t", "commands"]:
            return subprocess.CompletedProcess(argv, 0, lines_by_tree.get((cwd, argv[3]), ""), "")
        raise AssertionError(argv)
    return run


def test_unit_tokens(c):
    with testing.FixtureTree() as wt:
        wt.add_unit("Pl/sib.cpp")
        wt.add_unit("Pl/new.cpp")
        w, m = str(wt.root), "MAIN"
        line = "sjiswrap.exe mwcceppc.exe -lang=c++ -c src/Pl/x.cpp -o build/RMHE08/src/Pl\n"
        c.check("MAIN's line first", units.unit_tokens(m, w, "Pl/new.cpp",
                                                       _ninja({(m, "build/RMHE08/src/Pl/new.o"): line}))[1], "main")
        c.check("then the worktree's", units.unit_tokens(m, w, "Pl/new.cpp",
                                                         _ninja({(w, "build/RMHE08/src/Pl/new.o"): line}))[1], "worktree")
        toks, src = units.unit_tokens(m, w, "Pl/new.cpp", _ninja({(m, "build/RMHE08/src/Pl/sib.o"): line}))
        c.check("then a same-lib sibling, retargeted", (src, toks[toks.index("-o") + 1]),
                ("sibling Pl/sib (same lib)", os.path.join("build", "RMHE08", "src", "Pl")))
        c.raises("an unregistered unit with no line is refused", SystemExit, units.unit_tokens, m, w, "Q/q.cpp", _ninja({}))


def test_compile(c):
    with tempfile.TemporaryDirectory() as tmp:
        main, wt = os.path.join(tmp, "main"), os.path.join(tmp, "wt")
        os.makedirs(os.path.join(wt, "src", "Pl"))
        Path(wt, "src", "Pl", "u.cpp").write_text("int x;\n", encoding="utf-8")
        tokens = ["mwcceppc.exe", "-c", "src/Pl/u.cpp", "-o", "build/RMHE08/src/Pl"]
        obj = os.path.join(wt, "build/RMHE08/src/Pl", "u.o")
        c.check("dry run compiles nothing", units.compile("Pl/u.cpp", main, wt, dry_run=True, tokens=tokens)["object"], obj)

        def writes(argv, **kw):
            Path(obj).write_bytes(b"\x7fELF")
            return subprocess.CompletedProcess(argv, 0, "", "")

        res = units.compile("Pl/u.cpp", main, wt, runner=writes, tokens=tokens)
        c.check("a compile that writes the object is fresh", (res["compiled"], res["fresh"], res["bytes"]), (True, True, 4))
        res = units.compile("Pl/u.cpp", main, wt, runner=lambda argv, **kw: subprocess.CompletedProcess(argv, 0, "", ""),
                            tokens=tokens)
        c.check("rc 0 without an object is refused (the old object was deleted first)",
                (res["compiled"], "DIRECTORY" in res["error"]), (False, True))
        src = Path(wt, "src", "Pl", "u.cpp")
        Path(obj).write_bytes(b"x")
        os.utime(obj, (1_000_000, 1_000_000))
        c.check("an object older than its source is stale", units.object_is_fresh(obj, str(src))[0], False)


def test_run_tokens_scratch_redirects_the_helpers(c):
    """A scratch compile (`mangle`, `tryvar`, `frame`) must not let the chained helpers rewrite the tree's real object:
    that write, outside ninja, is what leaves the object dirty for every later `ninja` run."""
    with tempfile.TemporaryDirectory() as tmp:
        scratch = os.path.join(tmp, "scratch")
        tokens = ["cmd", "/c", "mwcceppc.exe", "-c", "src/Pl/u.cpp", "-o", "build/RMHE08/src/Pl", "&&", "python",
                  "tools\\elf\\objalign.py", "build\\RMHE08\\src\\Pl\\u.o", "&&", "python", "tools\\elf\\objextab.py",
                  "build\\RMHE08\\src\\Pl\\u.o"]
        seen = []

        def runner(argv, **kw):
            seen.append(list(argv))
            Path(scratch, "u.o").write_bytes(b"\x7fELF")
            return subprocess.CompletedProcess(argv, 0, "", "")

        rc, _out, obj = units.run_tokens(tokens, tmp, scratch_dir=scratch, runner=runner)
        want = os.path.abspath(os.path.join(scratch, "u.o"))
        c.check("the scratch compile writes its object to the scratch dir", (rc, os.path.abspath(obj)), (0, want))
        c.check("... and both chained helpers follow it, never the real build/RMHE08 object",
                [seen[0][i + 1] for i, t in enumerate(seen[0]) if t.endswith(("objalign.py", "objextab.py"))],
                [want, want])


def test_target_resolution(c):
    with testing.FixtureTree() as main, testing.FixtureTree() as wt:
        main.add_symbol("fn_80001000", ".text", 0x80001000, 0x20)
        main.add_symbol("fn_80002004", ".text", 0x80002004, 0x10)
        wt.add_symbol("renamed_fn", ".text", 0x80001000, 0x20)
        main.add_object("auto_fn_80001000_text.o", b"\x7fELF")
        main.add_object("auto_03_80002000_text.o", b"\x7fELF")
        main.write("build/RMHE08/config.json", json.dumps({"units": [
            {"name": "auto_03_80002000_text", "code_size": 0x20, "object": "build/RMHE08/obj/auto_03_80002000_text.o"}]}))
        m, w = str(main.root), str(wt.root)
        path, note = units.proposal_target(w, m, "renamed_fn")
        c.check("a renamed symbol finds the retired object by address",
                os.path.basename(path or ""), "auto_fn_80001000_text.o")
        path, note = units.proposal_target(w, m, "fn_80002004")
        c.check("the run covering the address is the fallback", os.path.basename(path or ""), "auto_03_80002000_text.o")
        c.check("an unknown symbol says why", units.proposal_target(w, m, "nope")[0], None)
        wt.add_object("Pl/u.o", b"\x7fELF")
        c.check("this tree's split object wins", units.resolve_target(w, m, "Pl/u.cpp", "x")[1], "worktree-split")
        c.check("with no split object anywhere, the retired object", units.resolve_target(m, m, "Pl/v.cpp", "fn_80001000")[1],
                "auto-fallback")
        c.check("the map follows the invocation", units.resolve_map(w, m)[1], "worktree-map")


def test_object_facts(c):
    from tools.lib.binary.build import ElfBuilder
    stwu, mflr, blr = 0x9421FFE0, 0x7C0802A6, 0x4E800020          # stwu r1,-0x20(r1) / mflr r0 / blr
    text = b"".join(w.to_bytes(4, "big") for w in (mflr, blr, stwu, blr, blr))
    b = ElfBuilder().section(".text", text).section(".data", b"\0" * 8)
    b.symbol("leaf", ".text", 0x0, 8, bind="global", type="func")
    b.symbol("framed", ".text", 0x8, 8, bind="global", type="func")
    b.symbol("tail", ".text", 0x10, 4, bind="local", type="func")
    b.symbol("table", ".data", 0x0, 8, bind="global", type="object")
    b.symbol("extern_fn")
    with tempfile.TemporaryDirectory() as tmp:
        obj = os.path.join(tmp, "u.o")
        Path(obj).write_bytes(b.build())
        c.check("frames: every defined function in address order, the stwu frame or None",
                units.frames(obj), [("leaf", 8, None), ("framed", 8, 0x20), ("tail", 4, None)])
        c.check("function_names follows frames", units.function_names(obj), ["leaf", "framed", "tail"])
        c.check("text_size is the .text section's size", units.text_size(obj), len(text))


def test_flag_helpers(c):
    tokens = ["sjiswrap.exe", "build\\compilers\\Wii\\1.3\\mwcceppc.exe", "-O4,p", "-inline", "auto", "-pragma",
              '"cats', 'off"', "-lang=c", "-MMD", "-c", "src/a.c", "-o", "build/RMHE08/src"]
    head, flags, tail = units.split_flags(tokens)
    c.check("split_flags: wrapper+compiler / flags / -MMD tail",
            (head, tail), (tokens[:2], ["-MMD", "-c", "src/a.c", "-o", "build/RMHE08/src"]))
    c.check("family groups -O levels and valued options", [units.family(t) for t in ("-O3", "-O4,p", "-inline", "-x")],
            ["-O", "-O", "-inline", None])
    c.check("override_flags drops the same family (and its value, quoted or not), then appends",
            units.override_flags(flags, "-O3 -inline noauto -pragma x"), ["-lang=c", "-O3", "-inline", "noauto", "-pragma", "x"])
    c.check("override_flags with nothing extra is the identity", units.override_flags(flags, ""), flags)
    c.check("quiet drops the ### banner", units.quiet("### banner\nerror: x\n### more"), "error: x")
    c.check("with_compiler_version swaps the version directory",
            units.with_compiler_version(head, "1.0")[1], "build\\compilers\\Wii\\1.0\\mwcceppc.exe")
    c.check("... and a cross-family spec swaps the family too",
            units.with_compiler_version(head, "GC/3.0a3")[1], "build\\compilers\\GC\\3.0a3\\mwcceppc.exe")
    c.check("drop_unknown_option removes the rejected option and its value",
            units.drop_unknown_option(["-a", "-gccinc", "-inline", "auto"], "Unknown option '-inline'"), ["-a", "-gccinc"])
    c.check("... and answers None when the log names none", units.drop_unknown_option(["-a"], "fine"), None)
    with tempfile.TemporaryDirectory() as tmp:
        d = Path(tmp, "Wii")
        for v in ("1.3", "1.0"):
            (d / v).mkdir(parents=True)
        c.check("available_versions lists the compiler's siblings",
                units.available_versions([str(d / "1.3" / "mwcceppc.exe")]), ["1.0", "1.3"])
    with testing.FixtureTree() as tree:
        tree.add_unit("Pl/a.c")
        u = units.Unit.resolve("Pl/a", str(tree.root))
        line = " ".join(tokens) + "\n"
        got = units.split_command(u, _ninja({(str(tree.root), "build/RMHE08/src/Pl/a.o"): line}))
        c.check("split_command is split_flags of the unit's own tree's ninja line", got[0], head)


def test_compile_result(c):
    ok = units.CompileResult("o.o", compiled=True, fresh=False, bytes=4, sections={}, log="")
    c.check("a CompileResult reads as the old dict, falsy fields kept", dict(ok),
            {"object": "o.o", "compiled": True, "fresh": False, "bytes": 4, "sections": {}, "log": ""})
    c.check("... with .get and `in`", (ok.get("error"), "error" in ok, ok["bytes"]), (None, False, 4))
    c.raises("... and a KeyError for an absent key", KeyError, lambda: ok["error"])
    dry = units.CompileResult("o.o", command=["cc"], dry_run=True)
    c.check("a dry run keeps the old key order", list(dict(dry)), ["command", "object", "dry_run"])
    bad = units.CompileResult("o.o", compiled=False, error="boom")
    c.check("a failure", dict(bad), {"object": "o.o", "compiled": False, "error": "boom"})


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
