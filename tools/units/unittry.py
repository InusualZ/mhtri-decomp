#!/usr/bin/env python3
"""Try source edits on a unit's real files and keep each one only when the unit's score rises and no row drops.
Spec: docs/tools/spec/unittry.md. CLI: unittry.py <unit> (--spec F | --replace OLD NEW | --replace-files A B |
--sites RE REPL | --permute FN) [--count N|--all] [--guard return-break] [--split-init] [--hill] [--restore] [--sdata2]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import time
from dataclasses import dataclass, field

from tools.lib import cscan, declperm, pendingmap, repo, report, text, units

#: The scratch directory under `build/tmp/` (the object and the report each trial is scored from).
SCRATCH = "unittry"
#: A score change smaller than this is no change.
EPS = 1e-6
#: The only semantic guard a site sweep knows.
GUARDS = ("return-break",)


class Refused(Exception):
    """An edit that cannot be applied as written (the trial is skipped, nothing is compiled)."""


# --------------------------------------------------------------------------------------------------
# the trials: what a spec file or the CLI asks for
# --------------------------------------------------------------------------------------------------

@dataclass
class Replace:
    """A literal replacement: `count` occurrences exactly (None: every occurrence, at least one)."""
    old: str
    new: str
    count: int | None = 1
    file: str | None = None


@dataclass
class Sites:
    """A regex sweep: every match is tried on its own, the improving ones kept."""
    pattern: str
    repl: str
    guard: str | None = None
    function: str | None = None
    literal: bool = False
    file: str | None = None


@dataclass
class Permute:
    """The orders of a block of consecutive statements in one function (`block` text, `lines`, or the leading
    declarations)."""
    function: str
    block: str | None = None
    lines: tuple[int, int] | None = None
    split_init: bool = False
    hill: bool = False
    cap: int = 120
    max_evals: int = 200
    seed: int = 0
    file: str | None = None


@dataclass
class Trial:
    name: str
    replaces: list[Replace] = field(default_factory=list)
    sites: Sites | None = None
    permute: Permute | None = None

    @property
    def kind(self) -> str:
        return "sites" if self.sites else "permute" if self.permute else "replace"


def _options(arg: str) -> dict:
    """`key=value` and bare `flag` words of a block header -> {key: value or True}."""
    out = {}
    for word in arg.split():
        k, _sep, v = word.partition("=")
        out[k.replace("_", "-")] = v if _sep else True
    return out


def _count(value) -> int | None:
    if value in (None, True):
        return 1
    if value == "all":
        return None
    try:
        n = int(value)
    except (TypeError, ValueError):
        raise SystemExit("tryvar: count=%r: want a number or `all`" % (value,))
    if n < 1:
        raise SystemExit("tryvar: count=%d: want at least 1" % n)
    return n


def _span(value) -> tuple[int, int]:
    """`A-B` (or `[A, B]`) -> (A, B), 1-based and inclusive."""
    try:
        a, b = (int(x) for x in (value if isinstance(value, (list, tuple)) else str(value).split("-", 1)))
    except (TypeError, ValueError):
        raise SystemExit("tryvar: lines=%r: want A-B (1-based, inclusive)" % (value,)) from None
    return a, b


def _permute_from(opts: dict, block: str | None) -> Permute:
    if not opts.get("function") or opts.get("function") is True:
        raise SystemExit("tryvar: a permute block needs function=NAME")
    return Permute(function=opts["function"], block=block or None,
                   lines=_span(opts["lines"]) if opts.get("lines") else None,
                   split_init=bool(opts.get("split-init")), hill=bool(opts.get("hill")),
                   cap=int(opts.get("max", 120)), max_evals=int(opts.get("max-evals", 200)),
                   seed=int(opts.get("seed", 0)), file=opts.get("file") if opts.get("file") is not True else None)


def parse_blocks(text_: str) -> list[Trial]:
    """Trials from the block format (`//@@ trial`, `//@@ replace`, `//@@ sites`, `//@@ permute`, `//@@ with`).

    Every body is verbatim text - no escaping of any kind; a body loses its one trailing newline. A `replace` or
    `sites` block is followed by its `with` block. Edits after a `//@@ trial NAME` belong to that trial (several
    replaces are one candidate); an edit block before any `trial` line is a trial of its own."""
    trials: list[Trial] = []
    current: Trial | None = None           # the open `//@@ trial`
    pending = None                         # (kind, opts, body) of an edit waiting for its `with`

    def target(kind: str) -> Trial:
        if current is not None:
            if current.sites or current.permute or (kind != "replace" and current.replaces):
                raise SystemExit("tryvar: trial %r: a sites sweep or a permutation is a trial on its own"
                                 % current.name)
            return current
        t = Trial("%s-%d" % (kind, len(trials) + 1))
        trials.append(t)
        return t

    try:
        blocks = text.marker_blocks(text_)
    except ValueError as exc:
        raise SystemExit("tryvar: %s (want trial, replace, sites, permute, with)" % exc) from None
    for kind, arg, body in blocks:
        if pending and kind != "with":
            raise SystemExit("tryvar: a `//@@ %s` block has no `//@@ with` block after it" % pending[0])
        if kind == "trial":
            if not arg:
                raise SystemExit("tryvar: `//@@ trial` needs a name")
            if any(t.name == arg for t in trials):
                raise SystemExit("tryvar: trial %r is named twice" % arg)
            current = Trial(arg)
            trials.append(current)
        elif kind in ("replace", "sites"):
            pending = (kind, _options(arg), body)
        elif kind == "with":
            if not pending:
                raise SystemExit("tryvar: `//@@ with` comes before any `replace`/`sites` block")
            ekind, opts, old = pending
            pending = None
            t = target(ekind)
            file_ = opts.get("file") if opts.get("file") is not True else None
            if ekind == "replace":
                t.replaces.append(Replace(old, body, _count("all" if opts.get("all") else opts.get("count")), file_))
            else:
                guard = opts.get("guard")
                if guard is True or (guard and guard not in GUARDS):
                    raise SystemExit("tryvar: trial %r: unknown guard %r (known: %s)" % (t.name, guard,
                                                                                        ", ".join(GUARDS)))
                fn = opts.get("function")
                t.sites = Sites(old if opts.get("literal") else old.strip("\n"), body, guard or None,
                                fn if fn is not True else None, bool(opts.get("literal")), file_)
        elif kind == "permute":
            t = target("permute")
            t.permute = _permute_from(_options(arg), body)
        else:
            raise SystemExit("tryvar: unknown block `//@@ %s` (want trial, replace, sites, permute, with)" % kind)
    if pending:
        raise SystemExit("tryvar: the last `//@@ %s` block has no `//@@ with` block" % pending[0])
    empty = [t.name for t in trials if not (t.replaces or t.sites or t.permute)]
    if empty:
        raise SystemExit("tryvar: trial(s) with no edit: %s" % ", ".join(empty))
    return trials


def parse_json(data) -> list[Trial]:
    """Trials from a JSON spec: a list (or `{"trials": [...]}`) of `{name, replace: [{old, new, count, file}]}`,
    `{name, sites: {regex, repl, guard, function, literal, file}}` or `{name, permute: {function, block, lines,
    split_init, hill, max, max_evals, seed, file}}`."""
    items = data.get("trials") if isinstance(data, dict) else data
    if not isinstance(items, list):
        raise SystemExit("tryvar: a JSON spec is a list of trials or {\"trials\": [...]}")
    out = []
    for i, item in enumerate(items, 1):
        if not isinstance(item, dict):
            raise SystemExit("tryvar: trial %d is not an object" % i)
        t = Trial(str(item.get("name") or "trial-%d" % i))
        kinds = [k for k in ("replace", "sites", "permute") if k in item]
        if len(kinds) != 1:
            raise SystemExit("tryvar: trial %r: want exactly one of replace, sites, permute" % t.name)
        if "replace" in item:
            reps = item["replace"] if isinstance(item["replace"], list) else [item["replace"]]
            for r in reps:
                if isinstance(r, list) and len(r) == 2:
                    r = {"old": r[0], "new": r[1]}
                if not isinstance(r, dict) or not isinstance(r.get("old"), str) or not isinstance(r.get("new"), str):
                    raise SystemExit("tryvar: trial %r: a replace is {old, new[, count, file]}" % t.name)
                t.replaces.append(Replace(r["old"].replace("\r\n", "\n"), r["new"].replace("\r\n", "\n"),
                                          _count(r.get("count")), r.get("file")))
        elif "sites" in item:
            s = item["sites"]
            if not isinstance(s, dict) or not isinstance(s.get("regex"), str) or not isinstance(s.get("repl"), str):
                raise SystemExit("tryvar: trial %r: sites is {regex, repl[, guard, function, literal, file]}" % t.name)
            if s.get("guard") and s["guard"] not in GUARDS:
                raise SystemExit("tryvar: trial %r: unknown guard %r" % (t.name, s["guard"]))
            t.sites = Sites(s["regex"], s["repl"], s.get("guard"), s.get("function"), bool(s.get("literal")),
                            s.get("file"))
        else:
            p = item["permute"]
            if not isinstance(p, dict):
                raise SystemExit("tryvar: trial %r: permute is an object" % t.name)
            opts = {k.replace("_", "-"): v for k, v in p.items() if k != "block"}
            t.permute = _permute_from(opts, (p.get("block") or "").replace("\r\n", "\n").rstrip("\n") or None)
        out.append(t)
    names = [t.name for t in out]
    if len(set(names)) != len(names):
        raise SystemExit("tryvar: a JSON spec names a trial twice")
    return out


def load_spec(path: str) -> list[Trial]:
    with open(path, encoding="utf-8-sig", newline="") as fh:
        body = fh.read().replace("\r\n", "\n")
    if path.lower().endswith(".json"):
        return parse_json(json.loads(body))
    return parse_blocks(body)


# --------------------------------------------------------------------------------------------------
# the files: the accepted state, the candidate, the byte-for-byte restore
# --------------------------------------------------------------------------------------------------

class Workspace:
    """The files a run may write: the unit's source and the headers it may edit.

    `orig` holds each file's bytes and mtime as the run found it; `cur` the LF text of the accepted state. A write
    keeps the file's line endings; `restore` puts the accepted state back and a file whose accepted text is its
    original text gets its original bytes and mtime, so ninja sees nothing changed."""

    def __init__(self, allowed: dict[str, str]):
        self.allowed = allowed            # abs path -> why it may be written ("source", "own header", ...)
        self.orig: dict[str, tuple[bytes, int]] = {}
        self.cur: dict[str, str] = {}

    def text(self, path: str) -> str:
        if path not in self.cur:
            raw = open(path, "rb").read()
            self.orig[path] = (raw, os.stat(path).st_mtime_ns)
            self.cur[path] = raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n")
        return self.cur[path]

    def _write(self, path: str, lf: str) -> None:
        raw, mtime = self.orig[path]
        if lf == raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n"):
            if open(path, "rb").read() != raw:
                text.atomic_write(path, raw)
            os.utime(path, ns=(mtime, mtime))
            return
        out = lf.replace("\n", "\r\n") if b"\r\n" in raw else lf
        text.atomic_write(path, out.encode("utf-8", "surrogateescape"))

    def write(self, files: dict[str, str]) -> None:
        """Write a candidate: `files` over the accepted state."""
        for path, lf in files.items():
            self.text(path)
            self._write(path, lf)

    def accept(self, files: dict[str, str]) -> None:
        self.cur.update(files)

    def restore(self, original: bool = False) -> None:
        """Put the accepted state (or, `original`, the state the run found) back on disk."""
        for path in self.cur:
            if original:
                self.cur[path] = self.orig[path][0].decode("utf-8", "surrogateescape").replace("\r\n", "\n")
            self._write(path, self.cur[path])

    def changed(self) -> list[str]:
        return [p for p in self.cur
                if self.cur[p] != self.orig[p][0].decode("utf-8", "surrogateescape").replace("\r\n", "\n")]


def include_graph(root: str) -> dict[str, set[str]]:
    """{header: the files under `src/` that include it directly} (comments removed, in-tree headers only)."""
    rev: dict[str, set[str]] = {}
    src = os.path.join(root, "src")
    for dirpath, _dirs, files in os.walk(src):
        for fn in files:
            if not fn.endswith((".c", ".cpp", ".h", ".hpp", ".inc")):
                continue
            path = os.path.normpath(os.path.join(dirpath, fn))
            for name in report.includes_of(path):
                hit = report.resolve_include(name, dirpath, root)
                if hit:
                    rev.setdefault(os.path.normcase(hit), set()).add(path)
    return rev


def other_includers(header: str, source: str, root: str, graph=None) -> list[str]:
    """The source files other than `source` whose include closure reaches `header`."""
    graph = graph if graph is not None else include_graph(root)
    seen, todo, out = set(), [os.path.normcase(header)], set()
    while todo:
        h = todo.pop()
        for inc in graph.get(h, ()):
            key = os.path.normcase(inc)
            if key in seen:
                continue
            seen.add(key)
            if inc.endswith((".c", ".cpp")):
                if key != os.path.normcase(source):
                    out.add(inc)
            else:
                todo.append(key)
    return sorted(out)


def resolve_file(spec: str | None, unit, root: str, ws: Workspace, shared_ok: bool, graph_cache: dict) -> str:
    """The absolute path an edit's `file` names (None: the unit's source), refused unless it is the source or a
    header of the source's include closure that no other unit reaches (`shared_ok` lifts the last condition)."""
    if not spec:
        return os.path.normpath(unit.source)
    cands = [os.path.join(root, spec), os.path.join(root, "src", spec)]
    path = next((os.path.normpath(c) for c in cands if os.path.isfile(c)), None)
    if path is None:
        raise SystemExit("tryvar: file=%s: no such file under the tree or src/" % spec)
    if path in ws.allowed:
        return path
    if os.path.normcase(path) == os.path.normcase(os.path.normpath(unit.source)):
        return os.path.normpath(unit.source)
    closure = {os.path.normcase(os.path.normpath(p)) for p in report.source_closure(unit.source, root)}
    if os.path.normcase(path) not in closure:
        raise SystemExit("tryvar: file=%s is not in %s's include closure - edits stay in the unit's own files"
                         % (spec, unit.spelling))
    if "graph" not in graph_cache:
        graph_cache["graph"] = include_graph(root)
    others = other_includers(path, unit.source, root, graph_cache["graph"])
    if others and not shared_ok:
        raise SystemExit("tryvar: file=%s is shared: %d other unit(s) include it (%s%s) and are not re-measured; "
                         "--shared-header allows it" % (spec, len(others), ", ".join(
                             report.rel_path(o, root) for o in others[:3]), ", ..." if len(others) > 3 else ""))
    ws.allowed[path] = "shared header (%d other units)" % len(others) if others else "own header"
    return path


# --------------------------------------------------------------------------------------------------
# functions, statements, the return->break guard
# --------------------------------------------------------------------------------------------------

def _qualified(decl) -> str:
    m = re.search(r"((?:[A-Za-z_]\w*\s*::\s*)*)$", decl.ret.rstrip())
    return re.sub(r"\s+", "", m.group(1) if m else "") + decl.name


def function_body(lf: str, function: str) -> tuple[int, int]:
    """`(open, close)`: the offsets of `function`'s body braces. `function` is a plain name, a `::`-qualified name
    or a mangled name (`name__...`, matched by the part before `__`); multi-line signatures are read whole
    (`lib.cscan.function_declarations`). `Refused` when it is absent or names more than one definition."""
    defs = [d for d in cscan.function_declarations(cscan.Text(lf)) if d.body and d.body[1] > 0]

    def pick(name: str):
        exact = [d for d in defs if _qualified(d) == name]
        if exact:
            return exact
        return [d for d in defs if d.name == name or _qualified(d).endswith("::" + name)]

    hits = pick(function)
    if not hits and "__" in function.lstrip("_"):
        base = re.match(r"(_*[A-Za-z0-9]+?)__", function)
        hits = pick(base.group(1)) if base else []
    if not hits:
        raise Refused("no definition of %s in the file" % function)
    if len(hits) > 1:
        raise Refused("%s names %d definitions (%s): qualify it" % (
            function, len(hits), ", ".join("%s line %d" % (_qualified(d), d.line) for d in hits)))
    return hits[0].body


_LABEL = re.compile(r"^\s*(?:(?:case\b[^:]*|default)\s*:(?!:)\s*)+")
_LOOP = re.compile(r"\b(?:for|while|do)\b")
_SWITCH = re.compile(r"^\s*(?:else\s+)?switch\b")
_TAIL_OK = re.compile(r"^(?:\s|\}|return\s*;)*$")


def _head_kind(head: str) -> str:
    head = _LABEL.sub("", head)
    if _SWITCH.match(head):
        return "switch"
    if re.match(r"^\s*(?:else\s+)?(?:for|while|do)\b", head):
        return "loop"
    return "block"


def return_break_safe(lf: str, pos: int, body: tuple[int, int]) -> tuple[bool, str]:
    """Whether `break;` at `pos` (a `return;` site) does what `return;` did: the innermost breakable construct is
    a switch, nothing enclosing it is a loop or another switch, and only closing braces or a `return;` follow that
    switch to the end of the function. Conservative: a refusal can be an equivalent site, never the reverse."""
    code = cscan.mask_preproc(cscan.strip(lf)[0])
    stack: list[tuple[int, str]] = []
    stmt, paren = body[0] + 1, 0
    for i in range(body[0] + 1, pos):
        c = code[i]
        if c == "(":
            paren += 1
        elif c == ")":
            paren -= 1
        elif paren == 0 and c == "{":
            stack.append((i, _head_kind(code[stmt:i])))
            stmt = i + 1
        elif paren == 0 and c == "}":
            if stack:
                stack.pop()
            stmt = i + 1
        elif paren == 0 and c == ";":
            stmt = i + 1
    own = _LABEL.sub("", code[stmt:pos])
    if _LOOP.search(own):
        return False, "the site is the body of a brace-less loop"
    breakable = [(p, k) for p, k in stack if k in ("switch", "loop")]
    if not breakable:
        return False, "not inside a switch"
    if breakable[-1][1] != "switch":
        return False, "the innermost breakable construct is a loop"
    if len(breakable) > 1:
        return False, "the switch sits inside another %s" % breakable[-2][1]
    close = cscan.match_brace(code, breakable[-1][0])
    if close < 0 or close > body[1]:
        return False, "the switch's closing brace was not found"
    tail = code[close + 1:body[1]]
    if not _TAIL_OK.match(tail):
        line = lf.count("\n", 0, close) + 1
        return False, "code follows the switch that ends on line %d" % line
    return True, ""


def statement_units(block: str) -> list[str]:
    """The statements of a block of whole lines (each ending in `\\n`), a statement running to the line where the
    parenthesis and brace depth is back to zero and the code ends in `;` or `}`. Blank and comment-only lines go
    with the statement after them (the last one's when none follows). `Refused` when the block does not end at a
    statement boundary or holds an `else`."""
    code = cscan.strip_comments(block)
    units_, cur, pending = [], [], []
    depth, pos = 0, 0
    for raw in block.splitlines(keepends=True):
        c = code[pos:pos + len(raw)]
        pos += len(raw)
        depth += c.count("(") + c.count("{") - c.count(")") - c.count("}")
        stripped = c.strip()
        if re.match(r"^\}?\s*else\b", stripped):
            raise Refused("the block holds an `else`: its statements cannot move apart")
        if not stripped and not cur:
            pending.append(raw)
            continue
        cur.append(raw)
        if depth == 0 and stripped.endswith((";", "}")):
            units_.append("".join(pending + cur))
            cur, pending = [], []
    if cur or depth:
        raise Refused("the block does not end at a statement boundary")
    if pending:
        if not units_:
            raise Refused("the block holds no statement")
        units_[-1] += "".join(pending)
    return units_


def _flat(unit_text: str) -> str:
    return "    " + " ".join(cscan.strip_comments(unit_text).split())


def declared_name(unit_text: str) -> str | None:
    d = declperm.declared(_flat(unit_text))
    return d[0] if d else None


def dependencies(units_: list[str]) -> tuple[tuple[int, ...], ...]:
    """Per statement, the earlier ones whose declared name it mentions (it cannot move before them)."""
    names = [declared_name(u) for u in units_]
    out = []
    for i, u in enumerate(units_):
        flat = _flat(u)
        d = declperm.declared(flat)
        used = set(re.findall(r"[A-Za-z_]\w*", d[1] if d else flat))
        out.append(tuple(j for j in range(i) if names[j] and names[j] in used))
    return tuple(out)


def split_initialiser(unit_text: str) -> tuple[str, str] | None:
    """`T x = f();` -> (`T x;`, `x = f();`), each a whole line at the statement's indent, or None when the
    statement is no splittable initialised declaration (`const`, a reference, an array or a brace initialiser)."""
    flat = _flat(unit_text)
    d = declperm.declared(flat)
    if not d or not d[1]:
        return None
    name, init = d
    head = flat[:flat.index("=")]
    if re.search(r"\bconst\b|&|\[|\bstatic\b", head) or init.lstrip().startswith("{"):
        return None
    code = cscan.strip_comments(unit_text)
    m = re.search(r"\b%s\s*=" % re.escape(name), code)
    if not m:
        return None
    indent = re.match(r"[ \t]*", unit_text.lstrip("\n")).group(0)
    eq = m.end()
    decl_line = indent + unit_text[:m.start()].strip() + " " + name + ";\n"
    rest = unit_text[eq:].rstrip("\n").lstrip(" ")
    return decl_line, indent + name + " = " + rest + "\n"


def _line_offsets(lf: str) -> list[int]:
    return [0] + [m.end() for m in re.finditer("\n", lf)]


def find_block(lf: str, p: Permute) -> tuple[int, int]:
    """`(start, end)` of the block to permute inside the function's body: the `block` text (exact lines, else the
    same lines with any indent, once), `lines` A-B, or the leading run of declarations."""
    body = function_body(lf, p.function)
    starts = _line_offsets(lf)
    body_first = lf.index("\n", body[0]) + 1 if "\n" in lf[body[0]:body[1]] else body[0] + 1
    if p.lines:
        a, b = p.lines
        if not 1 <= a <= b < len(starts):
            raise Refused("lines %d-%d are outside the file" % (a, b))
        start, end = starts[a - 1], starts[b]
        if start < body[0] or end > body[1] + 1:
            raise Refused("lines %d-%d are not inside %s's body" % (a, b, p.function))
        return start, end
    if p.block:
        inner = lf[body_first:body[1]]
        want = p.block.rstrip("\n") + "\n"
        hits = [m.start() for m in re.finditer(re.escape(want), inner)]
        hits = [h for h in hits if h == 0 or inner[h - 1] == "\n"]
        if len(hits) > 1:
            raise Refused("the block occurs %d times in %s" % (len(hits), p.function))
        if not hits:
            pat = "".join(r"[ \t]*" + re.escape(l.strip()) + r"[ \t]*\n" for l in want.splitlines())
            hits = [m.start() for m in re.finditer(r"(?m)^" + pat, inner)]
            if len(hits) != 1:
                raise Refused("the block is %s in %s" % ("not found" if not hits else
                                                         "found %d times (indent-free)" % len(hits), p.function))
            end = re.match(pat, inner[hits[0]:]).end()
            return body_first + hits[0], body_first + hits[0] + end
        return body_first + hits[0], body_first + hits[0] + len(want)
    pos, end = body_first, body_first
    count = 0
    while count < declperm.DEFAULT_MAX_LINES:
        nxt = lf.find("\n", pos)
        if nxt < 0 or nxt >= body[1]:
            break
        line = lf[pos:nxt + 1]
        if not declperm.declared(_flat(line)):
            break
        pos = end = nxt + 1
        count += 1
    if count < 2:
        raise Refused("%s has no leading run of two or more declarations" % p.function)
    return body_first, end


def permutations(lf: str, p: Permute) -> tuple[int, int, list[str], list[tuple[str, str]], tuple]:
    """`(start, end, units, candidates, deps)`: the block's span, its statements and `[(name, block text)]` for
    every order to try (the identity left out, unless `split_init` makes it a different text)."""
    start, end = find_block(lf, p)
    units_ = statement_units(lf[start:end])
    if len(units_) < 2 and not p.split_init:
        raise Refused("the block holds fewer than two statements")
    if p.split_init:
        decls, tail, n_split = [], [], 0
        for u in units_:
            sp = split_initialiser(u)
            if sp:
                decls.append(sp[0])
                tail.append(sp[1])
                n_split += 1
            elif declared_name(u) and not declperm.declared(_flat(u))[1]:
                decls.append(u)
            else:
                tail.append(u)
        if not n_split:
            raise Refused("no initialised declaration in the block can be split")
        ords, _sampled = declperm.orders(len(decls), p.cap, p.seed)
        cands = [("split-" + declperm.name_of(o)[6:], "".join(decls[i] for i in o) + "".join(tail))
                 for o in [tuple(range(len(decls)))] + ords]
        return start, end, decls + tail, cands, ()
    deps = dependencies(units_)
    ords, _sampled = declperm.orders(len(units_), p.cap, p.seed, deps)
    return start, end, units_, [(declperm.name_of(o), "".join(units_[i] for i in o)) for o in ords], deps


# --------------------------------------------------------------------------------------------------
# applying an edit to the accepted state
# --------------------------------------------------------------------------------------------------

def _lines_of(lf: str, needle: str) -> list[int]:
    out, i = [], lf.find(needle)
    while i >= 0:
        out.append(lf.count("\n", 0, i) + 1)
        i = lf.find(needle, i + max(1, len(needle)))
    return out


def apply_replaces(ws: Workspace, paths: list[str], reps: list[Replace]) -> dict[str, str]:
    """The candidate files for a list of replacements, each applied to the text as the earlier ones left it.
    `Refused` when an `old` occurs another number of times than its `count` asks (the lines are named)."""
    files: dict[str, str] = {}
    for path, r in zip(paths, reps):
        lf = files.get(path, ws.text(path))
        n = lf.count(r.old) if r.old else 0
        if not n or (r.count is not None and n != r.count):
            where = _lines_of(lf, r.old) if r.old else []
            raise Refused("%s: pattern occurs %d time(s)%s, want %s: %r" % (
                os.path.basename(path), n, " (lines %s)" % ", ".join(map(str, where[:12])) if where else "",
                "at least one (all)" if r.count is None else "exactly %d" % r.count, r.old[:60]))
        files[path] = lf.replace(r.old, r.new)
    return files


# --------------------------------------------------------------------------------------------------
# the measurement: compile into the tool's scratch, score with the official report, the .sdata2 order
# --------------------------------------------------------------------------------------------------

@dataclass
class Measurement:
    ok: bool
    functions: dict = field(default_factory=dict)          # name -> (percent, target size)
    fuzzy: float | None = None
    error: str = ""
    sdata2: str | None = None                              # same / differs / absent / None (not checked)
    seconds: float = 0.0
    report: str | None = None

    @property
    def matched(self) -> float:
        return sum(p / 100.0 * s for p, s in self.functions.values())


def sdata2_state(obj: str, target: str) -> str:
    """`same` when the object's `.sdata2` bytes are the target's, else `differs@0xN` / `absent` / `extra`."""
    from tools.lib.binary.elf import Elf
    a, b = Elf.read(target).section_bytes(".sdata2"), Elf.read(obj).section_bytes(".sdata2")
    if a is None and b is None:
        return "same"
    if a is None:
        return "extra"
    if b is None:
        return "absent"
    if a == b:
        return "same"
    first = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
    return "differs@0x%x" % first + ("" if len(a) == len(b) else "(0x%x/0x%x)" % (len(b), len(a)))


def report_score(target: str, unit_name: str, tmpdir: str, objdiff: str, cwd: str):
    """The official metric (`lib.report.score`, the one `measure.py` and `unitscore --measure` use), its report
    written under the tool's own `tmpdir` and deleted before each run -> `score(obj) -> (functions, fuzzy, path)`."""
    def score(obj: str):
        rep = report.score(target, obj, unit_name, tmpdir, objdiff=objdiff, cwd=cwd)
        unit = report.first_unit(rep)
        funcs = {}
        for f in unit.get("functions") or []:
            size = f.get("size")
            funcs[f.get("name")] = (float(report.score_of(f)), int(size) if str(size).isdigit() else 0)
        return funcs, (unit.get("measures") or {}).get("fuzzy_match_percent"), rep.path
    return score


class Measurer:
    """One compile of the unit's real source into the scratch directory, then one score. The object is deleted
    before the compile and must be newer than the trial's start, so a stale object is never scored."""

    def __init__(self, cmd: list[str], cwd: str, objdir: str, target: str, score, sdata2: bool = False):
        self.cmd, self.cwd, self.objdir, self.target, self.score, self.sdata2 = cmd, cwd, objdir, target, score, sdata2

    def __call__(self) -> Measurement:
        t0 = time.time()
        rc, log, obj = units.run_tokens(self.cmd, self.cwd, scratch_dir=self.objdir)
        if rc != 0:
            return Measurement(False, error=units.quiet(log).strip()[-800:] or "compile failed (rc %d)" % rc,
                               seconds=time.time() - t0)
        if os.path.getmtime(obj) < t0 - 1:
            return Measurement(False, error="the object %s did not move: refusing a stale score" % obj,
                               seconds=time.time() - t0)
        try:
            funcs, fuzzy, path = self.score(obj)
        except report.ReportError as exc:
            return Measurement(False, error=str(exc)[:800], seconds=time.time() - t0)
        m = Measurement(True, funcs, fuzzy, seconds=time.time() - t0, report=path)
        if self.sdata2:
            m.sdata2 = sdata2_state(obj, self.target)
        return m


# --------------------------------------------------------------------------------------------------
# the verdict and the run
# --------------------------------------------------------------------------------------------------

def compare(before: Measurement, after: Measurement) -> tuple[list, list]:
    """`(up, down)`: `[(name, before %, after %)]` rows that rose / fell; a row the candidate lost is a fall."""
    up, down = [], []
    for name, (p0, _s) in before.functions.items():
        p1 = after.functions.get(name, (None, 0))[0]
        if p1 is None or p1 < p0 - EPS:
            down.append((name, p0, p1))
        elif p1 > p0 + EPS:
            up.append((name, p0, p1))
    for name, (p1, _s) in after.functions.items():
        if name not in before.functions and p1 > EPS:
            up.append((name, None, p1))
    return up, down


def judge(before: Measurement, after: Measurement, sdata2: bool = False) -> tuple[bool, str, list, list]:
    """`(keep, why, up, down)`: keep only a candidate that compiled, dropped no row and raised the unit's matched
    bytes; with `sdata2`, one that turns a target-identical `.sdata2` into a different one is refused too."""
    if not after.ok:
        return False, "failed: " + (after.error.strip().splitlines() or ["no result"])[-1][:160], [], []
    up, down = compare(before, after)
    if down:
        return False, "%d row(s) drop: %s" % (len(down), ", ".join(
            "%s %.2f->%s" % (n, a, "gone" if b is None else "%.2f" % b) for n, a, b in down[:3])), up, down
    if after.matched <= before.matched + EPS:
        return False, "no gain", up, down
    if sdata2 and before.sdata2 == "same" and after.sdata2 != "same":
        return False, ".sdata2 order changed (%s)" % after.sdata2, up, down
    return True, "kept", up, down


@dataclass
class Row:
    trial: str
    name: str
    verdict: str                         # KEPT, best-of, reverted, skip, guarded, failed
    why: str = ""
    delta: float | None = None
    fuzzy: float | None = None
    up: int = 0
    down: int = 0
    sdata2: str | None = None
    seconds: float = 0.0

    def to_json(self) -> dict:
        return dict(self.__dict__)


class Runner:
    """Applies the trials in order to the accepted state, measuring each candidate and keeping the winners."""

    def __init__(self, ws: Workspace, measure, unit, root: str, out=print, sdata2: bool = False,
                 shared_ok: bool = False):
        self.ws, self.measure, self.unit, self.root, self.out = ws, measure, unit, root, out
        self.sdata2, self.shared_ok = sdata2, shared_ok
        self.rows: list[Row] = []
        self.graph: dict = {}
        self.base: Measurement | None = None
        self.cur: Measurement | None = None
        self.done = 0
        self.planned = 0
        self.t0 = time.time()

    def path(self, spec):
        return resolve_file(spec, self.unit, self.root, self.ws, self.shared_ok, self.graph)

    def _progress(self, row: Row) -> None:
        self.done += 1
        el = time.time() - self.t0
        per = el / max(1, self.done)
        left = max(0, self.planned - self.done) * per
        self.out("[%3d/%-3d %s eta %s] %-8s %-28s %s%s" % (
            self.done, max(self.planned, self.done), _clock(el), _clock(left), row.verdict, row.name[:28],
            "%+9.2f B  up %d down %d" % (row.delta, row.up, row.down) if row.delta is not None else "",
            "  " + row.why if row.why and row.verdict not in ("KEPT",) else ""))

    def _skip(self, trial: str, name: str, verdict: str, why: str, counted: bool = False) -> Row:
        row = Row(trial, name, verdict, why)
        self.rows.append(row)
        self.done += 1 if counted else 0
        self.out("            %-8s %-28s %s" % (verdict, name[:28], why))
        return row

    def evaluate(self, trial: str, name: str, files: dict[str, str], keep: bool = True) -> tuple[Row, Measurement]:
        """Write the candidate, measure it, then keep it (when `keep` and it wins) or restore the accepted state."""
        self.ws.write(files)
        m = self.measure()
        ok, why, up, down = judge(self.cur, m, self.sdata2)
        row = Row(trial, name, "KEPT" if ok and keep else ("better" if ok else
                                                            ("failed" if not m.ok else "reverted")),
                  "" if ok else (m.error.splitlines()[-1][:160] if not m.ok and m.error else why),
                  m.matched - self.cur.matched if m.ok else None, m.fuzzy, len(up), len(down), m.sdata2, m.seconds)
        if ok and keep:
            self.ws.accept(files)
            self.cur = m
        else:
            self.ws.restore()
        self.rows.append(row)
        self._progress(row)
        return row, m

    def baseline(self) -> Measurement:
        self.planned += 1
        m = self.measure()
        self.done += 1
        self.base = self.cur = m
        return m

    def execute(self, trials: list[Trial], restore: bool = False) -> bool:
        """`run` with the files put back whatever happens: the accepted state (or, `restore`, the state the run
        found) is written after the last trial, after an error and after Ctrl-C. True when interrupted."""
        try:
            self.run(trials)
        except KeyboardInterrupt:
            return True
        finally:
            self.ws.restore(original=restore)
        return False

    def run(self, trials: list[Trial]) -> None:
        self.planned += sum(1 for t in trials if t.replaces)
        for t in trials:
            try:
                if t.replaces:
                    self.run_replace(t)
                elif t.sites:
                    self.run_sites(t)
                else:
                    self.run_permute(t)
            except Refused as exc:
                self._skip(t.name, t.name, "skip", str(exc))

    def run_replace(self, t: Trial) -> None:
        paths = [self.path(r.file) for r in t.replaces]
        files = apply_replaces(self.ws, paths, t.replaces)
        self.evaluate(t.name, t.name, files)

    def run_sites(self, t: Trial) -> None:
        s = t.sites
        path = self.path(s.file)
        flags = re.M
        pat = re.compile(re.escape(s.pattern) if s.literal else s.pattern, flags)
        lf = self.ws.text(path)
        span = (0, len(lf))
        if s.function:
            body = function_body(lf, s.function)
            span = body
        if pat.match(""):
            raise Refused("the pattern matches the empty string")
        total = sum(1 for m in pat.finditer(lf) if span[0] <= m.start() < span[1])
        if not total:
            raise Refused("the pattern matches nothing%s" % (" in %s" % s.function if s.function else ""))
        self.planned += total
        pos, k = span[0], 0
        while True:
            lf = self.ws.text(path)
            if s.function:
                span = function_body(lf, s.function)
            m = pat.search(lf, pos)
            if not m or m.start() >= span[1]:
                break
            k += 1
            line = lf.count("\n", 0, m.start()) + 1
            name = "%s@L%d" % (t.name, line)
            repl = s.repl if not s.literal else s.repl.replace("\\", "\\\\")
            new_text = m.expand(repl)
            if s.guard == "return-break":
                try:
                    body = function_body_at(lf, m.start())
                    safe, why = return_break_safe(lf, m.start(), body)
                except Refused as exc:
                    safe, why = False, str(exc)
                if not safe:
                    self._skip(t.name, name, "guarded", why, counted=True)
                    pos = m.end()
                    continue
            cand = lf[:m.start()] + new_text + lf[m.end():]
            row, _m = self.evaluate(t.name, name, {path: cand})
            pos = m.start() + len(new_text) if row.verdict == "KEPT" else m.end()

    def run_permute(self, t: Trial) -> None:
        p = t.permute
        path = self.path(p.file)
        lf = self.ws.text(path)
        start, end, units_, cands, deps = permutations(lf, p)
        if p.hill and not p.split_init:
            return self.run_hill(t, path, start, end, units_, deps)
        self.planned += len(cands)
        best = None
        for name, block in cands:
            files = {path: lf[:start] + block + lf[end:]}
            row, m = self.evaluate(t.name, "%s:%s" % (t.name, name), files, keep=False)
            if row.verdict == "better" and (best is None or m.matched > best[1].matched + EPS):
                best = (files, m, row)
            if row.verdict == "better" and all(pc >= 100.0 for pc, _s in m.functions.values()):
                break
        if best:
            self.ws.write(best[0])
            self.ws.accept(best[0])
            self.cur = best[1]
            best[2].verdict = "KEPT"
            self.out("            KEPT     %s (best of %d orders)" % (best[2].name, len(cands)))

    def run_hill(self, t: Trial, path: str, start: int, end: int, units_: list[str], deps) -> None:
        """One statement moved per step from the accepted order; the first strictly better order is kept."""
        p = t.permute
        order = tuple(range(len(units_)))
        seen = {order}
        evals = 0
        self.planned += p.max_evals
        while evals < p.max_evals:
            moved = False
            lf = self.ws.text(path)
            for cand in declperm.neighbours(order, deps):
                if cand in seen:
                    continue
                if evals >= p.max_evals:
                    break
                seen.add(cand)
                evals += 1
                block = "".join(units_[i] for i in cand)
                row, _m = self.evaluate(t.name, "%s:%s" % (t.name, declperm.name_of(cand)),
                                        {path: lf[:start] + block + lf[end:]})
                if row.verdict == "KEPT":
                    order, moved = cand, True
                    break
            if not moved:
                break
        self.planned -= p.max_evals - evals


def function_body_at(lf: str, pos: int) -> tuple[int, int]:
    """The body of the function definition that holds offset `pos`."""
    for d in cscan.function_declarations(cscan.Text(lf)):
        if d.body and d.body[0] < pos < d.body[1]:
            return d.body
    raise Refused("the site is not inside a function body")


def _clock(seconds: float) -> str:
    seconds = int(seconds)
    return "%02d:%02d" % (seconds // 60, seconds % 60) if seconds < 3600 else "%dh%02d" % (seconds // 3600,
                                                                                         seconds // 60 % 60)


def print_table(rows: list[Row], out=print, top: int = 30, sdata2: bool = False) -> None:
    scored = sorted((r for r in rows if r.delta is not None), key=lambda r: (-r.delta, r.name))
    out("ranking (matched bytes against the state each was tried on; best first)")
    out("  %-3s %-8s %10s %8s %4s %5s %s%s" % ("#", "verdict", "delta B", "unit %", "up", "down",
                                              "sdata2      " if sdata2 else "", "candidate"))
    for i, r in enumerate(scored[:top], 1):
        out("  %-3d %-8s %+10.2f %8s %4d %5d %s%s" % (
            i, r.verdict, r.delta, "%.4f" % r.fuzzy if r.fuzzy is not None else "-", r.up, r.down,
            "%-12s" % (r.sdata2 or "-") if sdata2 else "", r.name))
    if len(scored) > top:
        out("  ... %d more" % (len(scored) - top))
    other = [r for r in rows if r.delta is None]
    if other:
        out("not scored: %s" % ", ".join("%s (%s)" % (r.name, r.verdict) for r in other[:10]) +
            (" ... %d more" % (len(other) - 10) if len(other) > 10 else ""))


# --------------------------------------------------------------------------------------------------
# the CLI
# --------------------------------------------------------------------------------------------------

def cli_trials(args) -> list[Trial]:
    if args.spec:
        trials = load_spec(args.spec)
        if args.only:
            want = set(args.only.split(","))
            missing = want - {t.name for t in trials}
            if missing:
                raise SystemExit("tryvar: --only names no trial: %s" % ", ".join(sorted(missing)))
            trials = [t for t in trials if t.name in want]
        return trials
    count = None if args.all else (args.count if args.count is not None else 1)
    if args.replace or args.replace_files:
        if args.replace_files:
            old, new = (open(p, encoding="utf-8-sig", newline="").read().replace("\r\n", "\n")
                        for p in args.replace_files)
        else:
            old, new = args.replace
        return [Trial("replace", [Replace(old, new, count, args.file)])]
    if args.sites:
        return [Trial("sites", sites=Sites(args.sites[0], args.sites[1], args.guard, args.function, args.literal,
                                           args.file))]
    if args.permute:
        block = None
        if args.block_file:
            block = open(args.block_file, encoding="utf-8-sig", newline="").read().replace("\r\n", "\n").rstrip("\n")
        return [Trial("permute", permute=Permute(args.permute, block, args.lines, args.split_init, args.hill,
                                                 args.max_perms, args.max_evals, args.seed, args.file))]
    raise SystemExit("tryvar: give --spec FILE, --replace OLD NEW, --replace-files A B, --sites RE REPL or "
                     "--permute FN")


def build_measurer(unit, root: str, args, out=print) -> tuple[Measurer, str]:
    """The compile command (the scorer's own resolution: `lib.units.unit_tokens` + `reconcile_flags`) pointed at
    the tool's scratch, and the target (`lib.units.resolve_target`, optionally renamed by `--rename-map`)."""
    main = repo.main_checkout(root)
    scratch = str(repo.scratch(SCRATCH, root) / unit.key.replace("/", "_"))
    tokens, _src = units.unit_tokens(main, root, unit.spelling)
    tokens, notes = units.reconcile_flags(tokens, root, unit.spelling)
    for n in notes:
        out("note: " + n)
    dry = units.compile(unit.spelling, main, root, dry_run=True, tokens=tokens)
    target, kind, note = units.resolve_target(root, main, unit.spelling, unit.file)
    if kind == "missing":
        raise SystemExit("tryvar: " + note)
    if note:
        out("note: " + note)
    if args.rename_map:
        ops = pendingmap.load(args.rename_map)
        pending = os.path.join(scratch, "target.pending.o")
        os.makedirs(scratch, exist_ok=True)
        outcome = pendingmap.apply_file(target, pending, ops)
        out("pending map %s: %s" % (args.rename_map, outcome.summary()))
        if outcome.refused:
            raise SystemExit("tryvar: --rename-map refused %d line(s)" % len(outcome.refused))
        target = pending
    score = report_score(target, unit.report_name, os.path.join(scratch, "report"),
                         report.objdiff_cli(root, main), root)
    return Measurer(dry["command"], main, os.path.join(scratch, "obj"), target, score, args.sdata2), target


def main(argv=None) -> int:
    try:
        return _main(argv)
    except SystemExit as exc:
        if isinstance(exc.code, str):     # a refusal with its reason: "could not run" (2), never a traceback
            print(exc.code, file=sys.stderr)
            return 2
        raise


def _main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", help="the unit (Pl/pl_act, src/menu/menu_result.cpp, ...)")
    src = ap.add_argument_group("the edits")
    src.add_argument("--spec", metavar="FILE", help="trials from a block file (`//@@ trial/replace/sites/permute/"
                                                    "with`) or a .json spec (docs/tools/spec/unittry.md)")
    src.add_argument("--only", metavar="NAMES", help="--spec: run only these trials (comma-separated)")
    src.add_argument("--replace", nargs=2, metavar=("OLD", "NEW"), help="one literal replacement")
    src.add_argument("--replace-files", nargs=2, metavar=("A", "B"),
                     help="one literal replacement read from two files (no shell escaping)")
    src.add_argument("--count", type=int, help="--replace: OLD must occur exactly N times (all N are replaced)")
    src.add_argument("--all", action="store_true", help="--replace: replace every occurrence (at least one)")
    src.add_argument("--sites", nargs=2, metavar=("REGEX", "REPL"),
                     help="try REPL at each REGEX match on its own, keeping the improving sites")
    src.add_argument("--literal", action="store_true", help="--sites: REGEX and REPL are literal text")
    src.add_argument("--guard", choices=GUARDS, help="--sites: skip the sites the guard cannot prove equivalent")
    src.add_argument("--function", metavar="FN", help="--sites: only the matches inside this function")
    src.add_argument("--permute", metavar="FN", help="try the orders of a block of statements in FN")
    src.add_argument("--block-file", metavar="F", help="--permute: the block's lines, verbatim, from a file")
    src.add_argument("--lines", type=_lines, metavar="A-B", help="--permute: the block is these lines")
    src.add_argument("--split-init", action="store_true",
                     help="--permute: split `T x = f();` into `T x;` + `x = f();` and permute the declarations")
    src.add_argument("--hill", action="store_true", help="--permute: hill-climb (one move per step)")
    src.add_argument("--max-perms", type=int, default=120, help="--permute: at most this many orders (sampled)")
    src.add_argument("--max-evals", type=int, default=200, help="--hill: at most this many compiles")
    src.add_argument("--seed", type=int, default=0, help="--permute: the sample's seed")
    src.add_argument("--file", metavar="PATH", help="the file the CLI edit applies to (default: the unit source)")
    run = ap.add_argument_group("the run")
    run.add_argument("--restore", action="store_true",
                     help="put every file back as it was at the end (a measuring run; the table says what won)")
    run.add_argument("--sdata2", action="store_true",
                     help="check .sdata2 against the target's bytes; refuse a candidate that breaks a same order")
    run.add_argument("--shared-header", action="store_true",
                     help="allow editing a header other units include (they are NOT re-measured)")
    run.add_argument("--rename-map", metavar="F", help="score against the target with these pending map edits")
    run.add_argument("--top", type=int, default=30, help="rows of the ranking to print")
    run.add_argument("--json", action="store_true", help="the run record as JSON on stdout (progress on stderr)")
    args = ap.parse_args(argv)

    trials = cli_trials(args)
    root = repo.worktree_root()
    unit = units.Unit.resolve(args.unit, root)
    out = (lambda s: print(s, file=sys.stderr, flush=True)) if args.json else (lambda s: print(s, flush=True))
    ws = Workspace({os.path.normpath(unit.source): "source"})
    measure, target = build_measurer(unit, root, args, out)
    runner = Runner(ws, measure, unit, root, out, args.sdata2, args.shared_header)
    for t in trials:                      # resolve every file before anything is written
        for spec in [r.file for r in t.replaces] + [x.file for x in (t.sites, t.permute) if x]:
            runner.path(spec)
    out("unit %s   %d trial(s)   report %s" % (unit.report_name, len(trials),
                                             report.rel_path(os.path.dirname(measure.objdir), root) + "/report/"))
    base = runner.baseline()
    if not base.ok:
        print("tryvar: the unit as it stands does not measure: %s" % base.error, file=sys.stderr)
        return 1
    out("baseline: unit %.4f %%, %.2f matched bytes, %d row(s)%s (%.1fs a compile)" % (
        base.fuzzy or 0.0, base.matched, len(base.functions), ", .sdata2 %s" % base.sdata2 if args.sdata2 else "",
        base.seconds))
    interrupted = runner.execute(trials, restore=args.restore)
    print_table(runner.rows, out, args.top, args.sdata2)
    kept = [r for r in runner.rows if r.verdict == "KEPT"]
    final = runner.cur
    changed = [report.rel_path(p, root) for p in ws.changed()]
    out("%s: unit %.4f %% -> %.4f %%, %+.2f matched bytes, %d kept%s" % (
        "interrupted" if interrupted else "done", base.fuzzy or 0.0, final.fuzzy or 0.0, final.matched - base.matched,
        len(kept), "; files restored (--restore)" if args.restore else
        ("; written: %s" % ", ".join(changed) if changed else "; no file changed")))
    if args.json:
        print(json.dumps({"unit": unit.report_name, "target": target, "baseline": {
            "fuzzy": base.fuzzy, "matched": base.matched, "sdata2": base.sdata2},
            "final": {"fuzzy": final.fuzzy, "matched": final.matched, "sdata2": final.sdata2},
            "rows": [r.to_json() for r in runner.rows], "kept": [r.name for r in kept],
            "written": [] if args.restore else changed, "interrupted": interrupted}, indent=2))
    return 130 if interrupted else 0


def _lines(value: str) -> tuple[int, int]:
    try:
        a, b = (int(x) for x in value.split("-", 1))
    except ValueError:
        raise argparse.ArgumentTypeError("want A-B (1-based, inclusive): %r" % value) from None
    return a, b


if __name__ == "__main__":
    raise SystemExit(main())
