#!/usr/bin/env python3
"""Self-test for `tools/unitutil.py`'s tree resolution - the caller names the tree, and a fixture is a tree.

    python tools/unitutil_selftest.py

Two rules, both the module's own, pinned here because a fixture is the shape every `*_selftest.py` here
uses and a fixture is *not* a git worktree:

* `repo_root(start=PATH)` roots the walk at `PATH` and never at this file's directory, so a caller can name
  a tree unambiguously. Without `start` the invocation's own tree wins - its git worktree when there is one,
  else the `cwd` when the `cwd` is a tree at all - and only a cwd that is not a tree falls back to this
  file's directory (the packaged-copy case).
* `resolve_unit(spec, root=PATH)` resolves a unit **under `PATH`**, so a fixture reads its own `src/` and
  `build/` instead of silently resolving the real tree from this module's `ROOT`.

The incident this closes: `tools/objdiff/unitscore_selftest.py` had to `git init` its fake repository.
Without a worktree `repo_root()` fell back to the tool's own directory, so the fixture was scored against
the real build - a fixture that silently reads the wrong tree. The fix is the `cwd`-that-is-a-tree rule and
`resolve_unit(spec, root=)`.
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import unitutil as uu  # noqa: E402


def _ok(label, got, want, failures):
    if got == want:
        print("ok    " + label)
        return failures
    print("FAIL  %s\n        got:  %r\n        want: %r" % (label, got, want))
    return failures + 1


def _raises(fn) -> bool:
    try:
        fn()
        return False
    except SystemExit:
        return True


def _fixture(root_dir: str, name: str = "fx") -> str:
    """A fake repository under `root_dir` - `configure.py`, one lib with a source, one version dir. No git."""
    d = os.path.join(root_dir, name)
    os.makedirs(os.path.join(d, "src", "demo"))
    os.makedirs(os.path.join(d, "build", "RMHE08", "obj", "demo"))
    os.makedirs(os.path.join(d, "build", "RMHE08", "src", "demo"))
    open(os.path.join(d, "configure.py"), "w", encoding="utf-8").write("config.libs = []\n")
    open(os.path.join(d, "src", "demo", "unit.cpp"), "w", encoding="utf-8").write("int f(void);\n")
    return d


def selftest() -> int:
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        fx = _fixture(tmp)
        real = uu.ROOT

        # `start` names the tree; a nested start walks up to it, never to this file's directory
        failures = _ok("repo_root(start=fixture) is the fixture",
                       os.path.normcase(uu.repo_root(start=fx)), os.path.normcase(fx), failures)
        failures = _ok("... a nested start walks up to the fixture, not to this repo",
                       os.path.normcase(uu.repo_root(start=os.path.join(fx, "src", "demo"))),
                       os.path.normcase(fx), failures)

        # `resolve_unit(root=)` resolves under the fixture - the unit's own paths are the fixture's
        u = uu.resolve_unit("demo/unit", root=fx)
        failures = _ok("resolve_unit(root=) finds the fixture's source",
                       os.path.normcase(os.path.dirname(u.src)),
                       os.path.normcase(os.path.join(fx, "src", "demo")), failures)
        failures = _ok("... its object", os.path.normcase(u.obj),
                       os.path.normcase(os.path.join(fx, "build", "RMHE08", "src", "demo", "unit.o")),
                       failures)
        failures = _ok("... its split target", os.path.normcase(u.target),
                       os.path.normcase(os.path.join(fx, "build", "RMHE08", "obj", "demo", "unit.o")),
                       failures)
        failures = _ok("... and the objdiff unit name", u.name, "main/demo/unit", failures)

        # top-level units: `src/<file>.cpp`, spelled like a nested unit without the directory
        os.makedirs(os.path.join(fx, "src", "other"))
        open(os.path.join(fx, "src", "main.cpp"), "w", encoding="utf-8").write("int g(void);\n")
        open(os.path.join(fx, "src", "mh3_pad.cpp"), "w", encoding="utf-8").write("int h(void);\n")
        bs = os.path.join(fx, "build", "RMHE08")
        for spec in ("main", "main/main", "main.cpp", "src/main.cpp", "src/main", "build/RMHE08/src/main.o",
                     "build/RMHE08/obj/main.o", "main\\main"):
            t = uu.resolve_unit(spec, root=fx)
            failures = _ok("top-level %r -> main/main" % spec, (t.name, t.lib, t.file), ("main/main", "", "main"),
                           failures)
        t = uu.resolve_unit("mh3_pad", root=fx)
        failures = _ok("top-level name", t.name, "main/mh3_pad", failures)
        failures = _ok("... its source", os.path.normcase(t.src),
                       os.path.normcase(os.path.join(fx, "src", "mh3_pad.cpp")), failures)
        failures = _ok("... its object", os.path.normcase(t.obj),
                       os.path.normcase(os.path.join(bs, "src", "mh3_pad.o")), failures)
        failures = _ok("... its split target", os.path.normcase(t.target),
                       os.path.normcase(os.path.join(bs, "obj", "mh3_pad.o")), failures)
        failures = _ok("... its object dir", os.path.normcase(t.obj_dir),
                       os.path.normcase(os.path.join(bs, "src")), failures)
        failures = _ok("a top-level unit is listed", "main/main" in [u.name for u in uu.list_units(fx)], True,
                       failures)
        failures = _ok("nested with extension", uu.resolve_unit("demo/unit.cpp", root=fx).name, "main/demo/unit",
                       failures)
        failures = _ok("nested bare stem", uu.resolve_unit("unit", root=fx).name, "main/demo/unit", failures)

        # a stem shared by two units is refused, listing both - never guessed
        open(os.path.join(fx, "src", "other", "unit.cpp"), "w", encoding="utf-8").write("int k(void);\n")
        open(os.path.join(fx, "src", "unit.cpp"), "w", encoding="utf-8").write("int m(void);\n")
        try:
            uu.resolve_unit("unit", root=fx)
            msg = ""
        except SystemExit as e:
            msg = str(e)
        failures = _ok("ambiguous stem refused, all candidates listed",
                       all(n in msg for n in ("main/demo/unit", "main/other/unit", "main/unit")), True, failures)
        failures = _ok("... a qualified spec still resolves", uu.resolve_unit("other/unit", root=fx).name,
                       "main/other/unit", failures)
        failures = _ok("... and the top-level one by src path", uu.resolve_unit("src/unit.cpp", root=fx).name,
                       "main/unit", failures)

        # without `root=` the same spec resolves the REAL tree, which has no demo/unit: the fixture's own
        # unit must not be reachable by accident (the pre-fix silent wrong-tree read)
        failures = _ok("the real tree has no demo/unit (a fixture cannot leak into it)",
                       _raises(lambda: uu.resolve_unit("demo/unit")), True, failures)

        # the invocation rule, in a real subprocess: cwd is the fixture (NOT a git worktree) and is a tree
        code = "import sys; sys.path.insert(0, %r); import unitutil; print(unitutil.repo_root())" % HERE
        p = subprocess.run([sys.executable, "-c", code], cwd=fx, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        failures = _ok("a non-git cwd that IS a tree wins (the invocation rule)",
                       os.path.normcase(p.stdout.strip()), os.path.normcase(fx), failures)

        # ... while a cwd that is NOT a tree still falls back to this tree (the packaged-copy case)
        plain = os.path.join(tmp, "not-a-tree")
        os.makedirs(plain)
        p = subprocess.run([sys.executable, "-c", code], cwd=plain, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        failures = _ok("a cwd that is not a tree still falls back to this tree",
                       os.path.normcase(p.stdout.strip()), os.path.normcase(real), failures)

        failures = _input_fallback(tmp, failures)
    return failures


def _input_fallback(tmp: str, failures: int) -> int:
    """`resolve_input`: a build/orig input is read from the tree, else from MAIN by path (never the reverse, never written)."""
    def norm(p):
        return os.path.normcase(os.path.abspath(p))

    def put(root, rel, data=b"x"):
        path = os.path.join(root, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as fh:
            fh.write(data)
        return path

    def git(cwd, *args):
        subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", "-c", "commit.gpgsign=false"] + list(args), cwd=cwd,
                       capture_output=True, check=True)

    main = os.path.join(tmp, "main")
    wt = os.path.join(tmp, "wt")
    os.makedirs(main)
    put(main, "configure.py", b"config.libs = []\n")
    git(main, "init", "-q")
    git(main, "add", "configure.py")
    git(main, "commit", "-q", "-m", "init")
    git(main, "worktree", "add", "-q", wt)
    failures = _ok("main_tree(worktree) is the primary checkout (the git common dir's parent)", norm(uu.main_tree(wt) or ""), norm(main), failures)
    failures = _ok("main_tree(a tree that is not in git) is None", uu.main_tree(os.path.join(tmp, "not-a-tree")), None, failures)
    rel = os.path.join("orig", "RMHE08", "sys", "main.dol")
    failures = _ok("nothing anywhere -> the tree's own path (the error names it)", norm(uu.resolve_input(rel, wt)), norm(os.path.join(wt, rel)), failures)
    in_main = put(main, rel)
    failures = _ok("only MAIN has it -> MAIN's path", norm(uu.resolve_input(rel, wt)), norm(in_main), failures)
    in_wt = put(wt, rel)
    failures = _ok("both have it -> the tree's own", norm(uu.resolve_input(rel, wt)), norm(in_wt), failures)
    failures = _ok("MAIN itself resolves to itself (no self-fallback loop)", norm(uu.resolve_input(rel, main)), norm(in_main), failures)
    os.remove(in_wt)
    # a directory input with a probe: an empty tree dir does not hide MAIN's dump
    put(main, os.path.join("build", "RMHE08", "asm", "a", "u.s"))
    os.makedirs(os.path.join(wt, "build", "RMHE08", "asm"))

    def has_s(d):
        return any(n.endswith(".s") for _dp, _dirs, names in os.walk(d) for n in names)
    failures = _ok("an empty local dir fails the probe -> MAIN's dump", norm(uu.resolve_input(os.path.join("build", "RMHE08", "asm"), wt, has_s)),
                   norm(os.path.join(main, "build", "RMHE08", "asm")), failures)
    failures = _ok("the same dir with the default probe (exists) stays local", norm(uu.resolve_input(os.path.join("build", "RMHE08", "asm"), wt)),
                   norm(os.path.join(wt, "build", "RMHE08", "asm")), failures)
    # $MHTRI_MAIN names MAIN for the tree the tools serve (ROOT); a fixture root never takes it
    alt = os.path.join(tmp, "alt")
    alt_file = put(alt, "only/in/alt.bin")
    old = os.environ.get("MHTRI_MAIN")
    os.environ["MHTRI_MAIN"] = alt
    try:
        failures = _ok("MHTRI_MAIN is MAIN for the tools' own tree", norm(uu.main_tree(uu.ROOT) or ""), norm(alt), failures)
        failures = _ok("... and resolve_input reads it", norm(uu.resolve_input(os.path.join("only", "in", "alt.bin"), uu.ROOT)), norm(alt_file), failures)
        failures = _ok("... but a fixture root ignores it", norm(uu.main_tree(wt) or ""), norm(main), failures)
    finally:
        if old is None:
            del os.environ["MHTRI_MAIN"]
        else:
            os.environ["MHTRI_MAIN"] = old
    return failures


def main() -> int:
    failures = selftest()
    print("%s: %d failure(s)" % ("FAILED" if failures else "passed", failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
