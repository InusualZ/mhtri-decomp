#!/usr/bin/env python3
"""Shim: the subprocess codec rule (F34) and its trap scan live in tools.lib.proc (removed in WP6).
Spec: docs/tools/spec/lib-proc.md. CLI: subproc.py [--selftest] (prints every trap site under tools/)."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import ast
import os
import subprocess

from tools.lib.proc import TEXT_KEYWORDS, TEXT_KWARGS, call_traps, module_traps, trap_sites  # noqa: F401
from tools.lib.proc import run as _run


def run(args, cwd=None, **kwargs):
    return _run(args, cwd=cwd, **kwargs)


def selftest() -> int:
    """Check the scanner: it must find the trap, and it must not cry wolf on the fixed spelling."""
    import tempfile

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def traps(src: str) -> list[str]:
        return [reason for _line, reason in module_traps(ast.parse(src))]

    check("the F34 spelling is a trap",
          len(traps('import subprocess\n'
                    'subprocess.run(["git", "show", "HEAD:CLAUDE.md"], cwd=".",\n'
                    '               capture_output=True, text=True, errors="replace")\n')), 1)
    check("the fixed spelling is not",
          traps('import subprocess\n'
                'subprocess.run(["git"], capture_output=True, text=True,\n'
                '               encoding="utf-8", errors="replace")\n'), [])
    check("the keywords may sit on the line after text=True",
          len(traps('import subprocess\n'
                    'subprocess.run(["git"], text=True,\n'
                    '               encoding=None)\n')), 0)
    check("a keyword line with no encoding is still a trap",
          len(traps('import subprocess\n'
                    'subprocess.run(["git"], text=True,\n'
                    '               cwd="/tmp")\n')), 1)
    check("universal_newlines is the same trap",
          len(traps('import subprocess\nsubprocess.Popen(["git"], universal_newlines=True)\n')), 1)
    check("binary mode needs no codec",
          traps('import subprocess\nsubprocess.run(["git"], capture_output=True)\n'
                'subprocess.run(["git"], capture_output=True, text=False)\n'), [])
    check("a `**` splat is not claimed either way",
          traps('import subprocess\nkwargs = {}\n'
                'subprocess.run(["git"], text=True, **kwargs)\n'), [])
    check("a runner indirection is scanned like any call",
          len(traps('def go(runner):\n'
                    '    return runner(["git"], capture_output=True, text=True, errors="replace")\n')), 1)
    check("TEXT_KWARGS is the documented pair",
          (TEXT_KWARGS["encoding"], TEXT_KWARGS["errors"]), ("utf-8", "replace"))

    # a text-mode decode that does NOT pin the codec is host-dependent: the em dash is the smallest
    # string that shows it (`cp1252` spells its UTF-8 bytes `a-hat` + euro + quote, three characters where
    # the file has one).  The fixture is byte-level, so the check itself cannot inherit the trap.
    dash = "\u2014"
    utf8_bytes = dash.encode("utf-8")
    code = "import sys; sys.stdout.buffer.write(bytes.fromhex('%s'))" % utf8_bytes.hex()
    out = subprocess.run([sys.executable, "-c", code], capture_output=True,
                         encoding="utf-8", errors="replace")
    check("a UTF-8 byte above ASCII survives a pinned codec", out.stdout, dash)
    check("... one byte sequence, three glyphs under cp1252",
          len(utf8_bytes.decode("cp1252", "replace")), 3)
    with tempfile.TemporaryDirectory() as tmp:
        # the trap itself, demonstrated the way it bit: the same bytes, decoded both ways
        import subprocess as _sp
        with open(os.path.join(tmp, "bytes.bin"), "wb") as fh:
            fh.write(utf8_bytes)
        via_locale = _sp.run([sys.executable, "-c",
                              "import sys; sys.stdout.write(open(sys.argv[1], encoding='cp1252'"
                              ".read())", os.path.join(tmp, "bytes.bin")],
                             capture_output=True, text=True, encoding="utf-8", errors="replace").stdout
        check("... so a locale decode does not equal the file's text", via_locale != dash, True)

    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "tools", "units"))
        with open(os.path.join(tmp, "tools", "units", "bad.py"), "w", encoding="utf-8") as fh:
            fh.write('import subprocess\np = subprocess.run(["git"], text=True)\n')
        with open(os.path.join(tmp, "tools", "units", "good.py"), "w", encoding="utf-8") as fh:
            fh.write('import subprocess\np = subprocess.run(["git"], text=True, encoding="utf-8")\n')
        with open(os.path.join(tmp, "tools", "units", "bad_selftest.py"), "w", encoding="utf-8") as fh:
            fh.write('import subprocess\np = subprocess.run(["git"], text=True)\n')
        sites = trap_sites(tmp)
        check("a tree scan names the file and the line", len(sites), 2)
        check("... naming `bad.py`", any("tools/units/bad.py:2" in s for s in sites), True)
        check("... and not the fixed file", any("good.py" in s for s in sites), False)
        check("... a selftest is scanned by default", any("bad_selftest" in s for s in sites), True)
        check("... and `skip_selftests=True` leaves it out",
              [s for s in trap_sites(tmp, skip_selftests=True) if "bad_selftest" in s], [])
        os.makedirs(os.path.join(tmp, "tools", "m2c"))
        with open(os.path.join(tmp, "tools", "m2c", "vendored.py"), "w", encoding="utf-8") as fh:
            fh.write('import subprocess\np = subprocess.run(["git"], text=True)\n')
        check("... and the m2c submodule is never scanned",
              any("vendored" in s for s in trap_sites(tmp)), False)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        raise SystemExit(selftest())
    for site in trap_sites(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))):
        print(site)
