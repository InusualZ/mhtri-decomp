#!/usr/bin/env python3
"""The one rule for reading a subprocess' text: say the codec, never inherit the locale's.

**The incident this closes (F34).** `land.run` did `subprocess.run(..., text=True, errors="replace")`
with no `encoding=`.  `text=True` without an explicit codec decodes with `locale.getpreferredencoding(False)`
- `cp1252` on this host - while the file it was compared against is read as UTF-8.  `AGENTS.md`'s prose
carries an em dash, so `git show HEAD:AGENTS.md` decoded as cp1252 spelled `â€"` where the file said `—`,
`agents_md_real_change` never matched, and the landing gate refused every landing with "main's tree is not
clean: M AGENTS.md".  The refusal was *false*: the file is byte-equal to HEAD once the LOCAL-ONLY block is
cut.  `PYTHONUTF8=1 agents_md_real_change(MAIN)` returned False while the default returned True - one
non-ASCII byte in the compared text was the whole difference.

The trap is silent and host-dependent: the same code is correct on a UTF-8 locale (Linux CI) and wrong on
this one, and it only bites when the compared text is non-ASCII.  A fixture whose AGENTS.md is pure ASCII
passes either way, which is how `require_clean_tree`'s existing test missed it.

So the rule is mechanical and this module is where it is written down:

* **every** text-mode subprocess call (`text=True` / `universal_newlines=True`) passes `encoding="utf-8"`
  explicitly (and `errors="replace"`, so a stray byte in a tool's output cannot raise
  `UnicodeDecodeError` halfway through a gate);
* `TEXT_KWARGS` is that pair, for callers that want to spell it once;
* `trap_sites(root)` finds the call sites that break the rule, for the selftest that refuses to let one
  come back.

`trap_sites` is an AST scan, not a grep: a call spans several lines, and the keyword may sit on any of them
(`subprocess.run([...], cwd=..., capture_output=True,\n text=True)` is the shape almost all of these have).
"""
from __future__ import annotations

import ast
import os
import subprocess
import sys

#: The pair every text-mode subprocess call in this repo passes.  `errors="replace"` as well as the
#: codec: a tool that prints one byte outside UTF-8 must not abort a gate with a decode error.
TEXT_KWARGS = {"encoding": "utf-8", "errors": "replace"}

#: The keyword names that turn a subprocess call into text mode.
TEXT_KEYWORDS = ("text", "universal_newlines")


def run(args, cwd=None, **kwargs):
    """`subprocess.run` with the codec decided here rather than by the host's locale.

    A thin convenience for callers that do not need to spell `TEXT_KWARGS`; it adds nothing else, so a
    caller that wants `capture_output`/`timeout`/`runner` still passes them through.
    """
    kwargs.setdefault("capture_output", True)
    for key, value in TEXT_KWARGS.items():
        kwargs.setdefault(key, value)
    return subprocess.run(args, cwd=cwd, **kwargs)


def _is_text_on(node: ast.AST) -> bool:
    """Whether a keyword argument's value is the literal `True` that turns text mode on.

    The **literal** is required, on purpose.  `text` is a common field name (`dict(text=...)`, a dataclass
    with a `text=` field, `write_elf(text=b"...")`), and this scan is an AST walk that cannot tell a
    constructor from a subprocess call; the nine `Proposal(text=...)`-shaped calls in `tools/units/` were
    false findings when the rule accepted any non-`False` value.  Every real text-mode call in this tree
    spells `text=True` literally, so requiring it costs no coverage and buys a scan that never asks a
    dataclass to pin a codec.  A `text=<expression>` is not claimed, the same way a `**` splat is not.
    """
    return isinstance(node, ast.Constant) and node.value is True


def call_traps(node: ast.AST) -> str | None:
    """Why `node` (an `ast.Call`) breaks the rule, or None when it does not.

    A `**kwargs` splat suppresses the finding: the keywords are not visible here, so this scan cannot
    claim either way, and a false FAIL would be worse than a missed one (there is exactly one such call
    in the tree, `selftest.py`'s runner, and it passes the codec through its own literal dict).
    """
    if not isinstance(node, ast.Call):
        return None
    keywords = {kw.arg: kw.value for kw in node.keywords if kw.arg}
    if any(kw.arg is None for kw in node.keywords):
        return None                      # a `**` splat: the keywords are not visible to this scan
    if "kwargs" in keywords:             # the same thing spelled through a dict
        return None
    text_kw = [name for name in TEXT_KEYWORDS if name in keywords and _is_text_on(keywords[name])]
    if not text_kw:
        return None
    if "encoding" in keywords:
        return None
    return "text mode (%s=True) without an explicit encoding=" % "/".join(text_kw)


def module_traps(tree: ast.AST) -> list[tuple[int, str]]:
    """`[(line, reason)]` for every text-mode subprocess call in one parsed module."""
    found = []
    for node in ast.walk(tree):
        reason = call_traps(node)
        if reason:
            found.append((getattr(node, "lineno", 0), reason))
    return sorted(found)


def trap_sites(root: str, subdir: str = "tools", skip_selftests: bool = False) -> list[str]:
    """`["<relpath>:<line>: <reason>"]` for every breaking call site under `root/<subdir>`.

    Two kinds of file are not scanned, both because they are not this repo's code to fix: the `m2c`
    submodule and the vendored `mwcc-debugger` (which carries a `PROVENANCE.md`).  `skip_selftests` is
    available for a scan that should ignore test scaffolding; the default scans everything, because a
    selftest compares text too (the trap cost a session in one).
    """
    out: list[str] = []
    vendored = ("tools/m2c/", "tools/mwcc-debugger/")
    base = os.path.join(root, subdir)
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames[:] = [d for d in dirnames if d != "__pycache__"]
        for name in sorted(filenames):
            if not name.endswith(".py"):
                continue
            if skip_selftests and name.endswith("_selftest.py"):
                continue
            path = os.path.join(dirpath, name)
            rel = path.replace("\\", "/")
            if any("/%s" % v in rel or rel.endswith("/" + v.rstrip("/")) for v in vendored):
                continue
            try:
                with open(path, encoding="utf-8", errors="replace") as fh:
                    tree = ast.parse(fh.read(), filename=path)
            except (OSError, SyntaxError) as exc:       # a broken file is not this scan's finding
                out.append("%s: unreadable by the scanner: %s" % (rel, exc))
                continue
            for line, reason in module_traps(tree):
                out.append("%s:%d: %s" % (rel, line, reason))
    return out


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
                    'subprocess.run(["git", "show", "HEAD:AGENTS.md"], cwd=".",\n'
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
    import sys
    if "--selftest" in sys.argv:
        raise SystemExit(selftest())
    for site in trap_sites(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))):
        print(site)
