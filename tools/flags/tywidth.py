#!/usr/bin/env python3
"""Sweeps the integer width of a function's locals, skipping widths that cannot hold the values. Spec: docs/tools/spec/tywidth.md.
CLI: python tools/flags/tywidth.py <unit> <function> [--plan] [--loose-sources] [--dry-run] | --rows CSV --out FILE."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import re
import subprocess
import time

from tools.lib import repo

#: The widths a local is tried at, in sweep order (the retired `tyall.py` order).
TYPES = ("s16", "u16", "u8", "s8", "s32", "u32")
#: The inclusive value range of each integer type the sweep knows.
RANGES = {"s8": (-128, 127), "u8": (0, 255), "s16": (-32768, 32767), "u16": (0, 65535),
          "s32": (-2 ** 31, 2 ** 31 - 1), "u32": (0, 2 ** 32 - 1), "bool": (0, 1)}
ALIAS = {"int": "s32", "unsigned": "u32", "char": "s8", "short": "s16", "BOOL": "s32"}
TYPE_RE = r"(?:s8|u8|s16|u16|s32|u32|int|unsigned|char|short|bool|BOOL)"
LOCAL_RE = re.compile(r"^(\s+)(s32|u32|s16|u16|u8|s8|int)\s+([A-Za-z_]\w*)(\s*=\s*[^;]*)?;\s*$")
LIT = r"(?:0[xX][0-9A-Fa-f]+|\d+)[uUlL]*"
CMP = r"(?:==|!=|<=|>=|<|>)"


def canon(name):
    """The sweep's own spelling of a C integer type name (`int` is `s32`), or None."""
    name = ALIAS.get(name, name)
    return name if name in RANGES else None


def lit_value(text):
    """The value of an integer or character literal, or None."""
    text = text.strip()
    m = re.fullmatch(r"(-?)(0[xX][0-9A-Fa-f]+|\d+)[uUlL]*", text)
    if m:
        v = int(m.group(2), 16) if m.group(2)[:2].lower() == "0x" else int(m.group(2))
        return -v if m.group(1) else v
    m = re.fullmatch(r"'(\\?.)'", text)
    return ord(m.group(1)[-1]) if m else None


def strip_comments(text):
    """`text` with `/* */` and `//` comments blanked (newlines kept, so line numbers hold)."""
    text = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


class TypeIndex:
    """Declared integer widths by name: struct fields, globals, array elements and function returns, read from a set
    of texts (the unit's source and the headers it includes)."""

    DECL = re.compile(r"^\s*(?:(?:extern|static|const|volatile|register|unsigned)\s+)*(%s)\s+([^;(){}]*?)\s*;" % TYPE_RE)
    RET = re.compile(r"^\s*(?:(?:extern(?:\s+\"C\")?|static|inline)\s+)*(%s)\s+\**([A-Za-z_][\w:]*)\s*\(" % TYPE_RE)

    def __init__(self):
        self.names = {}
        self.returns = {}

    def add_text(self, text):
        for line in strip_comments(text).split("\n"):
            m = self.RET.match(line)
            if m and canon(m.group(1)):
                self.returns.setdefault(m.group(2).split("::")[-1], set()).add(canon(m.group(1)))
                continue
            m = self.DECL.match(line)
            if not (m and canon(m.group(1))):
                continue
            for piece in m.group(2).split(","):
                d = re.match(r"\s*\**\s*([A-Za-z_]\w*)", piece)
                if d:
                    self.names.setdefault(d.group(1), set()).add(canon(m.group(1)))

    @classmethod
    def from_texts(cls, texts):
        idx = cls()
        for t in texts:
            idx.add_text(t)
        return idx

    def range_of(self, kinds):
        """The union range of a set of declared types (None when empty)."""
        if not kinds:
            return None
        return (min(RANGES[k][0] for k in kinds), max(RANGES[k][1] for k in kinds))


def include_closure(root, rel):
    """The unit source `rel` and every `#include "..."` file reachable from it under `src/`, as repo-relative paths."""
    seen, todo = [], [rel]
    while todo:
        cur = todo.pop()
        if cur in seen:
            continue
        path = os.path.join(root, cur)
        if not os.path.isfile(path):
            continue
        seen.append(cur)
        try:
            text = open(path, encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for inc in re.findall(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
            for cand in (os.path.join("src", inc), os.path.join(os.path.dirname(cur), inc)):
                cand = cand.replace("\\", "/")
                if os.path.isfile(os.path.join(root, cand)):
                    todo.append(os.path.normpath(cand).replace("\\", "/"))
                    break
    return seen


def split_top(text, sep):
    """`text` split at depth-0 occurrences of the one character `sep`."""
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == sep and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    return out


def expr_range(expr, index, env, depth=0):
    """The (lo, hi) values `expr` can take, or None when it cannot be bounded from declarations.  Handles literals,
    casts, ternaries, `% N`, `& N`, identifiers/fields/array elements/calls with a declared integer type."""
    e = expr.strip()
    while e.startswith("(") and e.endswith(")") and _balanced(e[1:-1]):
        e = e[1:-1].strip()
    if not e or depth > 8:
        return None
    v = lit_value(e)
    if v is not None:
        return (v, v)
    m = re.match(r"^\((%s)\)\s*(.+)$" % TYPE_RE, e, re.S)
    if m and canon(m.group(1)):
        return RANGES[canon(m.group(1))]
    q = split_top(e, "?")
    if len(q) == 2:
        c = split_top(q[1], ":")
        if len(c) == 2:
            a, b = expr_range(c[0], index, env, depth + 1), expr_range(c[1], index, env, depth + 1)
            return None if a is None or b is None else (min(a[0], b[0]), max(a[1], b[1]))
        return None
    m = re.match(r"^(.+?)\s*%%\s*(%s)$" % LIT, e, re.S)
    if m and lit_value(m.group(2)):
        n = lit_value(m.group(2))
        left = expr_range(m.group(1), index, env, depth + 1)
        return (0, n - 1) if left and left[0] >= 0 else (-(n - 1), n - 1)
    m = re.match(r"^(.+?)\s*&\s*(%s)$" % LIT, e, re.S)
    if m and lit_value(m.group(2)) is not None and lit_value(m.group(2)) >= 0:
        return (0, lit_value(m.group(2)))
    if re.search(r"[-+*/|^<>]|&&|\|\|", re.sub(r"\([^()]*\)|\[[^\[\]]*\]|->|\.", "", e)):
        return None
    name = re.findall(r"[A-Za-z_]\w*", re.sub(r"\([^()]*\)|\[[^\[\]]*\]", "", e))
    if not name:
        return None
    ident = name[-1]
    if re.search(r"\w\s*\(.*\)$", e) and not e.startswith("("):
        fn = re.match(r"^[\w:.>\-]*?([A-Za-z_]\w*)\s*\(", e)
        if fn:
            return index.range_of(index.returns.get(fn.group(1)))
        return None
    if ident in env:
        return env[ident]
    return index.range_of(index.names.get(ident))


def _balanced(text):
    depth = 0
    for ch in text:
        depth += (ch == "(") - (ch == ")")
        if depth < 0:
            return False
    return depth == 0


def facts_for(body, var, index, env):
    """What constrains `var`'s width: `(consts, sources, unresolved)` where `consts` is [(value, line_offset, why)],
    `sources` is [(lo, hi, line_offset, why)] (the range of each typed expression stored into or compared with it)
    and `unresolved` is [(line_offset, why)] (a stored expression that cannot be bounded)."""
    consts, sources, unresolved = [], [], []
    esc = re.escape(var)
    for off, raw in enumerate(body):
        line = raw.strip()
        for m in re.finditer(r"(?<![\w.>])%s\s*(=|\+=|-=|\*=|/=|%%=|&=|\|=|\^=|<<=|>>=)\s*([^;]*)" % esc, line):
            op, rhs = m.group(1), m.group(2)
            if op == "=" and rhs.startswith("="):
                continue
            if op in ("<<=", ">>="):
                continue
            rng = expr_range(rhs, index, env)
            why = "%s %s %s" % (var, op, rhs.strip())
            if op == "=" and rng is not None:
                if rng[0] == rng[1]:
                    consts.append((rng[0], off, why))
                else:
                    sources.append((rng[0], rng[1], off, why))
            elif op == "=":
                unresolved.append((off, why))
            elif lit_value(rhs) is not None:
                consts.append((lit_value(rhs), off, why))
            else:
                unresolved.append((off, why))
        for m in re.finditer(r"(?<![\w.>])%s\s*(%s)\s*([^;&|){]*?)(?=\s*(?:&&|\|\||\)|;|\{|$))" % (esc, CMP), line):
            rhs = m.group(2).strip()
            v = lit_value(rhs)
            why = "%s %s %s" % (var, m.group(1), rhs)
            if v is not None:
                consts.append((v, off, why))
            else:
                rng = expr_range(rhs, index, env)
                if rng is not None:
                    sources.append((rng[0], rng[1], off, why))
        for m in re.finditer(r"(%s)\s*(%s)\s*(?<![\w.>])%s(?!\w)" % (LIT, CMP, esc), line):
            consts.append((lit_value(m.group(1)), off, "%s %s %s" % (m.group(1), m.group(2), var)))
        if re.search(r"\bswitch\s*\(\s*%s\s*\)" % esc, line):
            for later in body[off:]:
                for c in re.findall(r"\bcase\s+(%s)\s*:" % LIT, later):
                    consts.append((lit_value(c), off, "case %s" % c))
    return consts, sources, unresolved


def allowed_types(consts, sources, unresolved, candidates=TYPES, loose_sources=False):
    """`(allowed, refused)`: the candidate types that hold every constant and typed source, and `{type: reason}`
    for the rest.  An expression that cannot be bounded allows only the 32-bit types unless `loose_sources`."""
    allowed, refused = [], {}
    for t in candidates:
        lo, hi = RANGES[t]
        bad = None
        for v, off, why in consts:
            if not lo <= v <= hi:
                bad = "%s: %d is outside %s" % (why, v, t)
                break
        if bad is None and not loose_sources:
            for slo, shi, off, why in sources:
                if slo < lo or shi > hi:
                    bad = "%s: source range %d..%d is outside %s" % (why, slo, shi, t)
                    break
            if bad is None and unresolved and t not in ("s32", "u32"):
                bad = "%s: value not bounded by a declaration, %s is narrower than 32 bits" % (unresolved[0][1], t)
        if bad:
            refused[t] = bad
        else:
            allowed.append(t)
    return allowed, refused


def find_def(lines, fn):
    """`(first line, last line)` of the definition of `fn` in `lines`, or None."""
    key = fn.split("__")[0] if ("__" in fn and not fn.startswith("__")) else fn
    key = key.split("::")[-1]
    cands = [i for i, l in enumerate(lines)
             if re.search(r"(^|[\s:*&])" + re.escape(key) + r"\s*\(", l) and not l.rstrip().endswith(";")
             and not l.lstrip().startswith(("*", "/", "//"))]
    defn = None
    for i in cands:
        seg = "\n".join(lines[i:i + 6])
        head = seg.split("{")[0]
        if "{" in seg and ";" not in head and not re.match(r"\s+", lines[i]):
            defn = i
            break
    if defn is None:
        return None
    depth, started = 0, False
    for j in range(defn, len(lines)):
        for ch in lines[j]:
            if ch == "{":
                depth += 1
                started = True
            elif ch == "}":
                depth -= 1
        if started and depth == 0:
            return defn, j
    return None


def local_env(lines, top, end, index):
    """The declared ranges of the function's parameters and locals, by name."""
    env = {}
    sig = " ".join(lines[top:top + 6]).split("{")[0]
    inner = sig[sig.find("(") + 1:sig.rfind(")")] if "(" in sig else ""
    for p in split_top(inner, ","):
        m = re.match(r"^\s*(?:const\s+)?(%s)\s+\**([A-Za-z_]\w*)" % TYPE_RE, p)
        if m and canon(m.group(1)):
            env[m.group(2)] = RANGES[canon(m.group(1))]
    for i in range(top + 1, end + 1):
        m = re.match(r"^\s+(?:register\s+)?(%s)\s+(.*?);" % TYPE_RE, lines[i])
        if m and canon(m.group(1)):
            for piece in m.group(2).split(","):
                d = re.match(r"\s*\**\s*([A-Za-z_]\w*)", piece)
                if d:
                    env[d.group(1)] = RANGES[canon(m.group(1))]
    return env


def plan_function(lines, top, end, index, loose_sources=False, candidates=TYPES):
    """One entry per integer local declaration: `{line, indent, type, name, init, allowed, refused}`."""
    env = local_env(lines, top, end, index)
    out = []
    body = lines[top + 1:end + 1]
    for li in range(top + 1, end + 1):
        m = LOCAL_RE.match(lines[li])
        if not m or "(" in (m.group(4) or ""):
            continue
        ind, ty, name, init = m.group(1), m.group(2), m.group(3), m.group(4) or ""
        # a name re-declared in an inner block is analysed as one variable: the union of the facts is the safe side
        facts = facts_for(body, name, index, {k: v for k, v in env.items() if k != name})
        allowed, refused = allowed_types(*facts, candidates=candidates, loose_sources=loose_sources)
        out.append({"line": li, "indent": ind, "type": canon(ty) or ty, "name": name, "init": init,
                    "allowed": [t for t in allowed if t != canon(ty)], "refused": refused})
    return out


def size_ok(prev_ours, new_ours, target):
    """A kept variant's size must equal the target's, or move closer to it, never away."""
    return abs(new_ours - target) <= abs(prev_ours - target)


def sweep(lines, top, end, index, measure, write, loose_sources=False, candidates=TYPES):
    """Try each allowed width of each local in order against `measure() -> (pct, target, ours)` after `write(lines)`;
    keep a width only when the score rises and the size does not move away from the target.  Returns
    `(lines, base, best, log, skipped)` where `skipped` counts widths the range guard refused."""
    base = measure()
    if base is None:
        return lines, None, None, [], 0
    cur, best, log, skipped = list(lines), base, [], 0
    for item in plan_function(cur, top, end, index, loose_sources, candidates):
        li = item["line"]
        skipped += sum(1 for t in candidates if t in item["refused"] and t != item["type"])
        pick = None
        for nt in [t for t in candidates if t in item["allowed"]]:
            trial = list(cur)
            trial[li] = "%s%s %s%s;" % (item["indent"], nt, item["name"], item["init"])
            write(trial)
            r = measure()
            if r and r[0] > (pick[1][0] if pick else best[0]) + 1e-6 and size_ok(best[2], r[2], base[1]):
                pick = (trial, r, nt)
        if pick:
            cur, best = pick[0], pick[1]
            log.append("%s:%s->%s %.3f (%dB)" % (item["name"], item["type"], pick[2], best[0], best[2]))
    write(cur)
    return cur, base, best, log, skipped


def measure_symbol(root, unit, fn):
    r = subprocess.run([sys.executable, os.path.join(root, "tools", "units", "recompile.py"), unit, "--measure", fn],
                       capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=root)
    m = re.search(r"measure .*?: ([\d.]+)% \(official report metric; target (\d+) B, ours (\d+) B", r.stdout)
    return (float(m.group(1)), int(m.group(2)), int(m.group(3))) if m else None


def source_of(root, unit):
    for ext in (".cpp", ".c"):
        rel = "src/%s%s" % (unit, ext)
        if os.path.exists(os.path.join(root, rel)):
            return rel
    return None


def write_retry(path, data):
    for _ in range(20):
        try:
            with open(path, "wb") as fh:
                fh.write(data)
            return
        except OSError:
            time.sleep(1)
    raise OSError("cannot write %s" % path)


def run_function(root, unit, fn, plan_only=False, dry_run=False, loose_sources=False, out=print):
    rel = source_of(root, unit)
    if rel is None:
        out("%s: no source" % unit)
        return None
    path = os.path.join(root, rel)
    raw = open(path, "rb").read()
    text = raw.decode("utf-8")
    crlf = "\r\n" in text
    lines = text.replace("\r\n", "\n").split("\n")
    loc = find_def(lines, fn)
    if loc is None:
        out("%s %s: definition not found" % (unit, fn))
        return None
    index = TypeIndex.from_texts(open(os.path.join(root, f), encoding="utf-8", errors="replace").read()
                                 for f in include_closure(root, rel))
    if plan_only:
        for item in plan_function(lines, loc[0], loc[1], index, loose_sources):
            out("line %d  %s %s: try %s" % (item["line"] + 1, item["type"], item["name"], ",".join(item["allowed"]) or "-"))
            for t, why in item["refused"].items():
                out("    skip %s  (%s)" % (t, why))
        return None

    def write(ls):
        t = "\n".join(ls)
        write_retry(path, (t.replace("\n", "\r\n") if crlf else t).encode("utf-8"))

    try:
        cur, base, best, log, skipped = sweep(lines, loc[0], loc[1], index,
                                              lambda: measure_symbol(root, unit, fn), write, loose_sources)
    except BaseException:
        write_retry(path, raw)
        raise
    if dry_run:
        write_retry(path, raw)
    out("%s %s %s -> %s %s (guard skipped %d widths)" % (unit, fn[:50], base, best, log, skipped))
    return base, best, log


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="?", help="unit, e.g. ef/eft050")
    ap.add_argument("function", nargs="?", help="symbol to measure and sweep")
    ap.add_argument("--plan", action="store_true", help="print the widths each local may take and why the rest are skipped")
    ap.add_argument("--dry-run", action="store_true", help="measure, then restore the source")
    ap.add_argument("--loose-sources", action="store_true",
                    help="check only literal constants, not the declared widths of the expressions stored")
    ap.add_argument("--rows", help="remaining_functions.csv-style rows `size,unit,function,pct` to sweep in turn")
    ap.add_argument("--out", help="append the one-line results here (with --rows)")
    args = ap.parse_args()
    root = str(repo.repo_root())
    if args.rows:
        sink = open(args.out, "a") if args.out else None

        def emit(s):
            print(s, flush=True)
            if sink:
                sink.write(s + "\n")
                sink.flush()

        for line in open(args.rows):
            size, unit, rest = line.strip().split(",", 2)
            fn, pct = rest.rsplit(",", 1)
            fn = fn.replace("\\", "")
            if fn.startswith(("__sinit", "gTRK")) or not pct.replace(".", "").isdigit() or float(pct) in (0.0, 100.0):
                continue
            run_function(root, unit, fn, dry_run=args.dry_run, loose_sources=args.loose_sources, out=emit)
        return 0
    if not (args.unit and args.function):
        ap.error("a unit and a function (or --rows)")
    run_function(root, args.unit, args.function, plan_only=args.plan, dry_run=args.dry_run,
                 loose_sources=args.loose_sources)
    return 0


if __name__ == "__main__":
    sys.exit(main())
