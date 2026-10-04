#!/usr/bin/env python3
"""Compile a playbook demo with the real MWCC and test its EXPECT lines (the library of `ideas.py demo-check`).
Spec: docs/tools/spec/ideas_demo.md. CLI: none (library)."""
import ast
import os
import re
import shlex
import subprocess
import tempfile

DEFAULT_MWCC = "Wii/1.3"
NUM = r"(?:0x[0-9a-fA-F]+|\d+)"
EXPECT_FORMS = (
    re.compile(r"^(contains|absent)\s+\S+$"),
    re.compile(r"^size\s+\S+\s+%s$" % NUM),
    re.compile(r"^section\s+\S+\s+%s$" % NUM),
    re.compile(r"^order\s+\S+\s+\S+\s+<\s+\S+$"),
    re.compile(r"^reloc\s+\S+$"),
    re.compile(r"^seq\s+\S+(?:\s+\S+)*?(?:\s+in\s+\S+)?$"),
    re.compile(r"^count\s+\S+\s+%s(?:\s+in\s+\S+)?$" % NUM),
    re.compile(r"^insn\s+\S+\s+\S+(?:\s+in\s+\S+)?$"),
    re.compile(r"^(?:no)?bytes\s+\S+\s+(?:[0-9a-fA-F]{2}\s*)+$"),
)
EXPECT_HELP = ("contains|absent <x>, size <sym> <bytes>, section <name> <bytes>, order <section> <A> < <B>, "
               "reloc <sym>, seq <mnemonic>... [in <func>], count <mnemonic> <n> [in <func>], "
               "insn <mnemonic> <operands> [in <func>], bytes|nobytes <section> <hex>")


def expect_ok(line):
    return any(f.match(line) for f in EXPECT_FORMS)


def num(s):
    return int(s, 0)


# ---- flags -------------------------------------------------------------------------------------------------------

def base_flags(root):
    """The token list of configure.py's `cflags_base` (read from the source, never a hand copy)."""
    with open(os.path.join(root, "configure.py"), encoding="utf-8") as f:
        tree = ast.parse(f.read())
    names = {"config.version": "RMHE08", "version_num": "0"}
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_base" for t in node.targets):
            if not isinstance(node.value, ast.List):
                break
            toks = []
            for el in node.value.elts:
                if isinstance(el, ast.Constant):
                    s = el.value
                elif isinstance(el, ast.JoinedStr):
                    s = ""
                    for part in el.values:
                        if isinstance(part, ast.Constant):
                            s += part.value
                        else:
                            s += names.get(ast.unparse(part.value), "0")
                else:
                    continue
                toks.extend(shlex.split(s))
            return toks + ["-DNDEBUG=1"]
    raise SystemExit("ideas_demo: could not read cflags_base from configure.py")


def demo_command(root, info, src, outdir):
    """The compile argv for a demo: base cflags with the demo's FLAGS replacing same-family flags."""
    from tools.lib import proc, units
    proc.install_spawn_retry()  # a launch Windows refuses transiently (WinError 5) is retried
    flags = units.override_flags(base_flags(root), info.get("FLAGS", ""))
    mw = info.get("MWCC", DEFAULT_MWCC)
    comp = os.path.join(root, "build", "compilers", *mw.split("/"), "mwcceppc.exe")
    wrap = os.path.join(root, "build", "tools", "sjiswrap.exe")
    return [wrap, comp] + flags + ["-c", src, "-o", outdir], comp


def objdump_exe(root):
    return os.path.join(root, "build", "binutils", "powerpc-eabi-objdump.exe")


# ---- the object model --------------------------------------------------------------------------------------------

SYM_RE = re.compile(r"^([0-9a-f]{8}) (.{7}) (\S+)\t([0-9a-f]{8}) (.+)$")
FUNC_RE = re.compile(r"^[0-9a-f]{8} <(.+)>:$")
INSN_RE = re.compile(r"^\s+[0-9a-f]+:\t(?:[0-9a-f]{2} ){4}\t(\S+)\s*(\S*)")
RELOC_RE = re.compile(r"^\s+[0-9a-f]+: (R_\w+)\t(\S+?)(?:[+-]0x[0-9a-f]+)?$")
SECT_RE = re.compile(r"^\s*\d+ (\S+)\s+([0-9a-f]{8})\s")
CONTENTS_RE = re.compile(r"^Contents of section (\S+):$")
HEXROW_RE = re.compile(r"^ [0-9a-f]{4,} (.*)$")


class Obj:
    """What `objdump -d -r -t -h` says about an object."""

    def __init__(self, text):
        self.text = text
        self.sections = {}      # name -> size
        self.symbols = []       # (name, section, addr, size)
        self.funcs = {}         # name -> [mnemonic]
        self.insns = {}         # name -> [(mnemonic, operands)]
        self.relocs = []        # target names
        self.contents = {}      # section name -> lowercase hex string
        cur = None
        csec = None
        for line in text.splitlines():
            m = CONTENTS_RE.match(line)
            if m:
                csec = m.group(1)
                self.contents.setdefault(csec, "")
                continue
            m = HEXROW_RE.match(line)
            if m and csec is not None:
                self.contents[csec] += m.group(1).split("  ")[0].replace(" ", "")
                continue
            m = SECT_RE.match(line)
            if m:
                self.sections[m.group(1)] = int(m.group(2), 16)
                continue
            m = SYM_RE.match(line)
            if m and m.group(3) != "*ABS*":
                self.symbols.append((m.group(5).strip(), m.group(3), int(m.group(1), 16), int(m.group(4), 16)))
                continue
            m = FUNC_RE.match(line)
            if m:
                cur = m.group(1)
                self.funcs.setdefault(cur, [])
                self.insns.setdefault(cur, [])
                continue
            m = INSN_RE.match(line)
            if m and cur is not None:
                self.funcs[cur].append(m.group(1))
                self.insns[cur].append((m.group(1), m.group(2)))
                continue
            m = RELOC_RE.match(line)
            if m:
                self.relocs.append(m.group(2))
        self.mnemonics = {x for v in self.funcs.values() for x in v}

    def names(self, sym):
        """Symbols matching `sym` exactly or as a C++ mangling prefix."""
        return [s for s in self.symbols if s[0] == sym or s[0].startswith(sym + "__")]

    def func(self, name):
        for k, v in self.funcs.items():
            if k == name or k.startswith(name + "__"):
                return v
        return None


def evaluate(obj, line):
    """(ok, why) for one EXPECT line against an `Obj`."""
    w = line.split()
    op = w[0]
    if op in ("contains", "absent"):
        x = w[1]
        found = x in obj.mnemonics or bool(obj.names(x)) or any(r == x or r.startswith(x + "__") for r in obj.relocs)
        if op == "contains":
            return found, "" if found else "`%s` is not an instruction, symbol or relocation target" % x
        return not found, "" if not found else "`%s` is present" % x
    if op == "size":
        ms = obj.names(w[1])
        if not ms:
            return False, "no symbol `%s`" % w[1]
        got = ms[0][3]
        return got == num(w[2]), "" if got == num(w[2]) else "`%s` is %d (0x%X) bytes, want %d" % (w[1], got, got, num(w[2]))
    if op == "section":
        if w[1] not in obj.sections and num(w[2]) != 0:
            return False, "no section `%s` (have: %s)" % (w[1], ", ".join(sorted(obj.sections)))
        got = obj.sections.get(w[1], 0)
        return got == num(w[2]), "" if got == num(w[2]) else "`%s` is %d (0x%X) bytes, want %d" % (w[1], got, got, num(w[2]))
    if op == "order":
        sec, a, b = w[1], w[2], w[4]
        addr = {}
        for k in (a, b):
            ms = [s for s in obj.names(k) if s[1] == sec]
            if not ms:
                return False, "no symbol `%s` in section `%s`" % (k, sec)
            addr[k] = ms[0][2]
        ok = addr[a] < addr[b]
        return ok, "" if ok else "`%s` is at 0x%X, `%s` at 0x%X (want %s first)" % (a, addr[a], b, addr[b], a)
    if op == "reloc":
        ok = any(r == w[1] or r.startswith(w[1] + "__") for r in obj.relocs)
        return ok, "" if ok else "no relocation names `%s`" % w[1]
    if op == "insn":
        scope = w[-1] if len(w) > 3 and w[-2] == "in" else None
        pool = [t for n, v in obj.insns.items() if scope is None or n == scope or n.startswith(scope + "__") for t in v]
        if scope and not any(n == scope or n.startswith(scope + "__") for n in obj.insns):
            return False, "no function `%s`" % scope
        ok = (w[1], w[2]) in pool
        return ok, "" if ok else "no `%s %s` instruction%s" % (w[1], w[2], " in `%s`" % scope if scope else "")
    if op in ("bytes", "nobytes"):
        want = "".join(w[2:]).lower()
        have = obj.contents.get(w[1])
        if have is None:
            return op == "nobytes", "" if op == "nobytes" else "no contents for section `%s`" % w[1]
        hit = want in have
        ok = hit if op == "bytes" else not hit
        return ok, "" if ok else "section `%s` %s the bytes %s" % (w[1], "lacks" if op == "bytes" else "holds", want)
    if op in ("seq", "count"):
        scope = None
        if "in" in w[1:] and w[-2] == "in":
            scope, w = w[-1], w[:-2]
        if scope:
            ins = obj.func(scope)
            if ins is None:
                return False, "no function `%s`" % scope
        else:
            ins = [m for v in obj.funcs.values() for m in v]
        if op == "count":
            got = ins.count(w[1])
            return got == num(w[2]), "" if got == num(w[2]) else "`%s` occurs %d times, want %s" % (w[1], got, w[2])
        it = iter(ins)
        for m in w[1:]:
            if not any(x == m for x in it):
                return False, "`%s` does not follow the earlier mnemonics in order" % m
        return True, ""
    return False, "unknown EXPECT `%s`" % line


def excerpt(obj, line, limit=28):
    """The part of the object an EXPECT is about, for a failure report."""
    w = line.split()
    rows = []
    if w[0] in ("size", "order", "contains", "absent", "reloc"):
        keys = [x for x in w[1:] if x not in ("<",)]
        rows = ["  sym %-8s %-10s +0x%X size 0x%X" % (n, sec, a, sz) for n, sec, a, sz in obj.symbols
                if any(n == k or n.startswith(k + "__") for k in keys)]
    if w[0] in ("bytes", "nobytes"):
        return "  %s: %s" % (w[1], obj.contents.get(w[1], "(none)")[:240])
    if w[0] == "section":
        rows = ["  section %s 0x%X" % kv for kv in obj.sections.items()]
    if w[0] == "insn":
        for name, v in obj.insns.items():
            if len(w) > 3 and w[-2] == "in" and not name.startswith(w[-1]):
                continue
            rows.append("  %s: %s" % (name, "; ".join("%s %s" % t for t in v)))
    if w[0] in ("seq", "count", "contains", "absent"):
        scope = w[-1] if "in" in w[1:] and w[-2] == "in" else None
        for name, ins in obj.funcs.items():
            if scope is None or name.startswith(scope):
                rows.append("  %s: %s" % (name, " ".join(ins)))
    return "\n".join(rows[:limit]) or "  (nothing to show)"


# ---- compile -----------------------------------------------------------------------------------------------------

def compile_demo(root, info, demo_path):
    """(Obj | None, log). Compiles into a scratch directory; the repository is never written."""
    with tempfile.TemporaryDirectory(prefix="ideas-demo-") as d:
        argv, comp = demo_command(root, info, os.path.abspath(demo_path), d)
        if not os.path.isfile(comp):
            return None, "SKIP compiler missing: %s" % os.path.relpath(comp, root)
        p = subprocess.run(argv, cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
        log = (p.stdout or "") + (p.stderr or "")
        o = os.path.join(d, os.path.splitext(os.path.basename(demo_path))[0] + ".o")
        if p.returncode != 0 or not os.path.isfile(o):
            return None, "compile failed (rc %d):\n%s" % (p.returncode, log.strip())
        q = subprocess.run([objdump_exe(root), "-d", "-r", "-t", "-h", "-s", o], capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        if q.returncode != 0:
            return None, "objdump failed: %s" % q.stderr.strip()
        return Obj(q.stdout), log


def check_demo(root, info, demo_path, compile_fn=compile_demo):
    """(status, [report lines]); status is PASS, FAIL or SKIP."""
    obj, log = compile_fn(root, info, demo_path)
    if obj is None:
        return ("SKIP" if log.startswith("SKIP") else "FAIL"), [log]
    report, bad = [], False
    for e in info["EXPECT"]:
        ok, why = evaluate(obj, e)
        if not ok:
            bad = True
            report.append("  FAILED EXPECT: %s\n    %s\n%s" % (e, why, excerpt(obj, e)))
    return ("FAIL" if bad else "PASS"), report
