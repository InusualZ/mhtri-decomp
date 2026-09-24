#!/usr/bin/env python3
"""Derive the mangled name MWCC emits for a C++ declaration, so the map can be renamed to it.

A C++ unit's symbols are mangled. Our symbol map spells them the way dtk does when it cannot demangle
(`fn_800CCCF8`), and the original object's name is the mangled one - so a C++ definition whose name does
not match the map's spelling is invisible to objdiff, and the tempting workaround is `extern "C"`.

**It is not needed.** The map is a build input, not a description of the original's symbol table: rename
the map symbol to the mangled name our source emits and objdiff pairs it by name, the relocations match,
and the front-end stays the one the unit really had (docs/plan.md, "The language comes from the symbol").
This tool produces that spelling. It is the other half of a rename, so it prints the `symedit.py` command
too.

    python tools/units/mangle.py 'void Pl_Skill_ck(_PLW* self, u8 x)'
    python tools/units/mangle.py --unit Pl/pl_act.cpp 'void Pl_Skill_ck(_PLW*, u8)'
    python tools/units/mangle.py --file decls.cpp --json
    python tools/units/mangle.py --selftest

The snippet is a **declaration**; a body is appended when it has none (`{ }`), because only a *defined*
symbol appears in the object's function table. A full definition may be passed instead - it is used
verbatim. Whatever the declaration needs (a struct, a typedef, an include) has to be in the snippet: the
tool compiles it as its own translation unit with the flags of `--unit`.

Only the compiler's **front-end** decides the mangled spelling, so the `--unit` flags matter only in that
they select the compiler version and `-lang=c++`; they are not a claim about the unit being renamed.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402

# Any registered C++ unit will do: its command line carries the compiler version and `-lang=c++`, which is
# all the mangler reads. Kept small and stable rather than "whatever is first".
DEFAULT_UNIT = "Runtime.PPCEABI.H/__ppc_eabi_init.cpp"


# Every snippet in this project needs the scalar types, and `cflags` already pass `-i include`, so the probe
# includes the common header by default. `--no-include` opts out for a snippet that must not see it.
PREAMBLE = '#include "types.h"\n'


def stub_source(snippet: str, preamble: bool = True) -> str:
    """The snippet as a compilable TU: a declaration gets a body, a definition is used verbatim.

    A body is required because `frames()` reads the object's *function* table - a declaration alone emits
    an undefined reference, which is not a function entry, and the tool would report nothing.

    The decision is made on the snippet's **tail**, not on whether it contains a `{` anywhere: a class or
    struct definition in the same snippet carries braces of its own, so "contains a brace" would read
    `struct M { void f(int); }; void M::f(int)` as already-complete and the compile would fail with
    "illegal function definition".
    """
    text = snippet.strip()
    if text.endswith(";"):
        text = text[:-1].rstrip()
    if not text.endswith("}"):
        if "(" in text.rsplit("\n", 1)[-1]:
            text += " { }"                   # complete a function declaration
    return (PREAMBLE if preamble else "") + text + "\n"


def mangle(snippet: str, unit_spec: str | None = None, verbose: bool = False,
           preamble: bool = True) -> tuple[list[str], str]:
    """Compile `snippet` and return (defined function names, compiler output).

    Raises `SystemExit` when the unit's command line cannot be resolved - the same failure `recompile.py`
    raises, and for the same reason: a hand-written command line would drift from what ninja runs.
    """
    unit = unitutil.resolve_unit(unit_spec or DEFAULT_UNIT)
    tokens = unitutil.compile_command(unit)
    with tempfile.TemporaryDirectory() as scratch:
        src = os.path.join(scratch, "mangle_probe.cpp")
        with open(src, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(stub_source(snippet, preamble))
        rc, out, obj = unitutil.run_compile(tokens, scratch_dir=scratch, src=src, verbose=verbose)
        if rc != 0 or not os.path.exists(obj):
            raise SystemExit("the snippet did not compile with %s's command line:\n%s"
                             % (unit.name, unitutil.quiet(out)))
        return unitutil.function_names(obj), out


def rename_command(old: str, new: str) -> str:
    """The other half of the edit, as the command that performs it (never by hand - skill rule)."""
    return "python tools/symbols/symedit.py rename %s %s" % (old, new)


def _print(names: list[str], out: str, old: str | None, as_json: bool) -> int:
    if as_json:
        print(json.dumps({"mangled": names, "rename": rename_command(old, names[0]) if old and names else None},
                         indent=1))
        return 0
    if not names:
        print("no function symbol was defined - is the snippet a declaration the compiler accepted?")
        print(unitutil.quiet(out))
        return 1
    for n in names:
        print(n)
    if old and len(names) == 1:
        print("")
        print("rename the map symbol to it (map + source, one edit):")
        print("  %s" % rename_command(old, names[0]))
    return 0


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # the source shape, without the compiler
    check("stub_source appends a body to a declaration", stub_source("void f(void)", preamble=False),
          "void f(void) { }\n")
    check("stub_source leaves a definition alone", stub_source("void f(void) { }", preamble=False),
          "void f(void) { }\n")
    check("stub_source completes a declaration that ends in a semicolon",
          stub_source("void f(void);", preamble=False), "void f(void) { }\n")
    check("stub_source keeps a trailing body brace", stub_source("int f() { return 1; }", preamble=False),
          "int f() { return 1; }\n")
    # a struct in the same snippet carries braces of its own - the tail decides, not a brace search
    check("stub_source completes a member definition after a class",
          stub_source("struct M { void f(int); };\nvoid M::f(int)", preamble=False),
          "struct M { void f(int); };\nvoid M::f(int) { }\n")
    check("stub_source leaves a class-only snippet alone", stub_source("struct M { int a; };", preamble=False),
          "struct M { int a; }\n")
    check("stub_source includes the project's scalar types by default",
          stub_source("void f(void)").startswith('#include "types.h"'), True)
    check("stub_source can opt out of the preamble",
          stub_source("void f(void)", preamble=False).startswith('#include'), False)
    check("the rename command goes through symedit, not by hand",
          rename_command("fn_800CCCF8", "X__Fv"), "python tools/symbols/symedit.py rename fn_800CCCF8 X__Fv")

    # the real thing: one compile with a unit's real command line
    names, _ = mangle("void mangle_probe_free(int a, unsigned char b)")
    check("a free function yields exactly one defined symbol", len(names), 1)
    check("the name is mangled (argument list appended after __)",
          names[0].startswith("mangle_probe_free__F"), True)
    check("the C spelling is not the emitted name", names[0] != "mangle_probe_free", True)

    # a member function carries its class, which is the case `extern \"C\"` cannot express at all
    cls, _ = mangle("struct MangleProbe { void method(int a); };\nvoid MangleProbe::method(int a)")
    check("a member function is one symbol", len(cls), 1)
    check("a member function's mangling names the class", "MangleProbe" in cls[0], True)
    check("a member function differs from the free-function spelling", cls[0] != names[0], True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("snippet", nargs="?", help="a C++ declaration (a body is appended) or a definition")
    ap.add_argument("--file", default=None, help="read the snippet from this file instead")
    ap.add_argument("--unit", default=None,
                    help="whose command line to compile with (default %s - only the compiler front-end "
                         "affects the mangling)" % DEFAULT_UNIT)
    ap.add_argument("--old", default=None,
                    help="the map symbol being renamed; prints the symedit.py rename command for it")
    ap.add_argument("--no-include", action="store_true",
                    help="do not prepend `#include \"types.h\"` to the probe (it is prepended by default)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if args.file:
        snippet = open(args.file, encoding="utf-8").read()
    elif args.snippet:
        snippet = args.snippet
    else:
        ap.print_help()
        return 2
    names, out = mangle(snippet, args.unit, args.verbose, preamble=not args.no_include)
    return _print(names, out, args.old, args.json)


if __name__ == "__main__":
    sys.exit(main())
