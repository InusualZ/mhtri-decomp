"""Apply the lanes' integrator requests in one batch: renames + sweep, owner-header declarations, STOPGAP removal.
Spec: docs/tools/spec/integrate.md. CLI: python tools/units/integrate.py [--requests F..|--lane SLUG..] [--names F]
[--base B] [--branch B] [--dry-run] [--no-build] [--no-commit] [--json]; tests: tools/tests/units/test_integrate.py."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import tempfile
import time
from dataclasses import dataclass, field

from tools.lib import cscan
from tools.lib import names as libnames
from tools.lib import objcompare
from tools.lib import outbox as liboutbox
from tools.lib import proc
from tools.lib import report as libreport
from tools.lib import repo as librepo
from tools.lib import requests as R
from tools.lib import text as libtext
from tools.lib import units as libunits
from tools.lib.git import Git
from tools.lib.project import Ownership, Splits

TOOLS_TREE = str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file()))
SOURCE_SUFFIXES = (".c", ".cpp", ".cp", ".cc", ".h", ".hpp")
BUILD = os.path.join("build", "RMHE08")
#: Identifiers that are never a type a header has to see (keywords, the project's scalar typedefs).
BUILTIN_TYPES = (set(libnames.PRIMITIVE_CODES) | libnames.QUALIFIERS
                 | {"unsigned", "signed", "extern", "static", "inline", "__inline", "u8", "s8", "u16", "s16", "u32",
                    "s32", "u64", "s64", "f32", "f64", "BOOL", "size_t", "va_list", "wchar_t", "NULL", "volatile"})
_FWD_RE = re.compile(r"\b(struct|class|union)\s+([A-Za-z_]\w*)\s*;")
_GUARD_END_RE = re.compile(r"(?m)^[ \t]*#[ \t]*endif\b[^\n]*\n?\s*\Z")
_CPP_CLOSE_RE = re.compile(r"(?m)^[ \t]*#[ \t]*ifdef[ \t]+__cplusplus[ \t]*\r?\n[ \t]*\}[^\n]*\r?\n[ \t]*#[ \t]*endif[^\n]*\r?\n")
_CPP_OPEN_RE = re.compile(r"(?m)^[ \t]*#[ \t]*ifdef[ \t]+__cplusplus[ \t]*\r?\n[ \t]*extern[ \t]+\"C\"[ \t]*\{[^\n]*\r?\n"
                          r"[ \t]*#[ \t]*endif[^\n]*\r?\n")
_PLAIN_LINKAGE_RE = re.compile(r"(?m)^[ \t]*extern[ \t]+\"C\"[ \t]*\{[^\n]*\n")
_ERROR_FILE_RE = re.compile(r"#\s+File:\s*(\S+)")
_ERROR_LINE_RE = re.compile(r"#\s+(\d+):\s?(.*)")
_UNTYPED_LINE_RE = re.compile(r"^[ \t]*/\*\s*untyped\s*:[^\n]*\*/[ \t]*$")


# --- small utilities ------------------------------------------------------------------------------------------

def rel(root: str, path: str) -> str:
    return os.path.relpath(path, root).replace("\\", "/")


def run_tool(script: str, *args: str, cwd: str, timeout: float | None = None):
    """Another tool, by subprocess (the layering rule: a tool never imports a tool it is not allowed to)."""
    return proc.run([sys.executable, os.path.join(TOOLS_TREE, script), *args], cwd=cwd, timeout=timeout)


def source_files(root: str) -> list[str]:
    out = []
    for sub in ("src", "include"):
        for base, dirs, files in os.walk(os.path.join(root, sub)):
            dirs[:] = sorted(d for d in dirs if d not in ("__pycache__",))
            out.extend(os.path.join(base, f) for f in sorted(files) if f.endswith(SOURCE_SUFFIXES))
    return out


class Files:
    """The tree's source texts, read lazily and edited in memory; `flush` writes the changed ones byte-exactly."""

    def __init__(self, root: str):
        self.root = root
        self.texts: dict[str, str] = {}
        self.original: dict[str, str | None] = {}
        self._all: list[str] | None = None
        self._declared: dict[str, tuple] = {}

    def all(self) -> list[str]:
        if self._all is None:
            self._all = [rel(self.root, p) for p in source_files(self.root)]
        return self._all

    def exists(self, r: str) -> bool:
        return r in self.texts and self.texts[r] is not None or os.path.isfile(os.path.join(self.root, r))

    def get(self, r: str) -> str:
        if r not in self.texts:
            p = os.path.join(self.root, r)
            t = libtext.read_text(p) if os.path.isfile(p) else None
            self.texts[r] = t
            self.original[r] = t
        return self.texts[r] or ""

    def set(self, r: str, text: str) -> None:
        self.get(r)
        self.texts[r] = text
        if r not in (self._all or []) and self._all is not None:
            self._all.append(r)

    def declared(self, r: str) -> set[str]:
        """`declared_in` of the file's current text, cached on the text."""
        t = self.get(r)
        hit = self._declared.get(r)
        if hit is None or hit[0] is not t:
            hit = (t, declared_in(t) if t else set())
            self._declared[r] = hit
        return hit[1]

    def resolve(self, inc: str, includer: str) -> str | None:
        for base in (os.path.dirname(includer), os.path.join(self.root, "include"), os.path.join(self.root, "src")):
            cand = os.path.normpath(os.path.join(base, inc))
            r = rel(self.root, cand)
            if self.texts.get(r) is not None or os.path.isfile(cand):
                return cand
        return None

    def read_abs(self, path: str) -> str:
        r = rel(self.root, path)
        return libtext.read_text(path) if r.startswith("..") else self.get(r)

    def closure(self, r: str) -> list[str]:
        """The file and everything it includes (quoted includes, resolved like `-i include -i src`), as rel paths."""
        start = os.path.join(self.root, r)
        return [rel(self.root, p) for p in cscan.include_closure(start, self.resolve, self.read_abs)]

    def sees(self, r: str, name: str) -> bool:
        """Whether file `r` (or a header it reaches) declares or defines `name`."""
        return any(not x.startswith("..") and name in self.declared(x) for x in self.closure(r))

    def changed(self) -> list[str]:
        return sorted(r for r, t in self.texts.items() if t is not None and t != self.original.get(r))

    def flush(self) -> list[str]:
        out = self.changed()
        for r in out:
            libtext.atomic_write(os.path.join(self.root, r), self.texts[r])
        return out


# --- the plan ------------------------------------------------------------------------------------------------------

@dataclass
class Item:
    """One request's verdict: `applied` (with its ops), `judgement`, `deferred` or `reverted`, and why."""
    req: R.Request
    cls: str
    reason: str
    state: str = "planned"
    why: str = ""
    resolved: list = field(default_factory=list)
    renames: list = field(default_factory=list)          # (map name, new name)
    rewrites: list = field(default_factory=list)         # (source spelling, map/source name) - stale spellings
    decls: list = field(default_factory=list)            # DeclOp
    config: str | None = None
    notes: list = field(default_factory=list)
    excluded: dict = field(default_factory=dict)         # op name -> why it is no longer applied

    def live_decls(self) -> list:
        return [d for d in self.decls if d.name not in self.excluded]

    def exclude(self, op_name: str, why: str) -> None:
        """Stop applying one declaration; a request left with nothing to apply becomes judgement."""
        self.excluded.setdefault(op_name, why)
        # a stale spelling's rewrite goes with its declaration: renamed, the lane's local declaration would clash
        # with the owner header's (different) one
        self.rewrites = [(o, n) for o, n in self.rewrites if n != op_name]
        if not self.live_decls() and not self.renames and not self.rewrites and not self.config:
            self.state, self.why = "judgement", "; ".join("%s: %s" % kv for kv in self.excluded.items())

    def to_dict(self) -> dict:
        return {"id": self.req.id, "kind": self.req.kind, "class": self.cls, "reason": self.reason,
                "excluded": self.excluded,
                "state": self.state, "why": self.why, "renames": self.renames, "rewrites": self.rewrites,
                "decls": [d.to_dict() for d in self.decls], "notes": self.notes,
                "targets": [r.to_dict() for r in self.resolved]}


@dataclass
class DeclOp:
    rid: str
    name: str
    map_name: str
    address: int | None
    section: str | None
    owner: str | None
    header: str
    cpp: bool
    hint: str | None = None
    data: bool = False
    owned_by_lane: bool = False
    implicit: bool = False          # a rename's declaration: moved only when the lane declared it locally
    move: bool = False              # a decl-move: a band header's declaration of an owned symbol is removed too
    leaf: bool = False              # declared in its leaf header `include/<module>/<symbol>.h` (the full one clashed)
    force: bool = False             # a rename's declaration the build proved a caller needs
    full_header: str | None = None  # the owner's full header, when `leaf` moved the declaration out of it
    result: dict = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {"name": self.name, "map_name": self.map_name, "header": self.header, "owner": self.owner,
                "cpp_linkage": self.cpp, "data": self.data, "leaf": self.leaf, **self.result}


DATA_SECTIONS = (".data", ".bss", ".sbss", ".sdata", ".rodata", ".sdata2", ".sbss2")


def lane_units_of(outbox_dir: str, slug: str) -> list[str]:
    path = os.path.join(outbox_dir, slug + ".json")
    if not os.path.exists(path):
        return []
    return liboutbox.load_outbox(path).units()


def live_units(main: str) -> set[str]:
    """Units a live lane holds: `claims.py list --json` rows not yet merged (by subprocess - one implementation)."""
    p = run_tool("tools/units/claims.py", "list", "--json", cwd=main, timeout=120)
    try:
        rows = json.loads(p.stdout or "[]")
    except json.JSONDecodeError:
        return set()
    return units_of_rows(rows)


def units_of_rows(rows) -> set[str]:
    """Every unit an unmerged `claims.py list --json` row holds: its own `unit` (not the `(unregistered)`
    placeholder) and its `units` - the unit set a cluster claim or a spawned lane's slot lock records."""
    if isinstance(rows, dict):
        rows = rows.get("claims") or rows.get("rows") or []
    out: set[str] = set()
    for r in rows:
        if not isinstance(r, dict) or r.get("merged"):
            continue
        if r.get("unit") and r.get("unit") != "(unregistered)":
            out.add(r["unit"])
        out.update(u for u in r.get("units") or [] if isinstance(u, str) and u)
    return out


def plan(requests: list[R.Request], ownership: Ownership, decisions: dict, live: set[str],
         lane_units: set[str], status: dict) -> list[Item]:
    items = []
    lane_stems = {R.unit_stem(u) for u in lane_units}
    for req in requests:
        cls, reason = R.classify(req, decisions)
        it = Item(req, cls, reason)
        items.append(it)
        st = status.get(req.id, {}).get("status")
        if st in ("applied", "rejected"):
            it.state, it.why = "done", "status sidecar: %s" % st
            continue
        it.resolved = R.resolve(req, ownership, decisions) if req.kind in ("rename", "decl", "decl-move") else []
        if cls != "mechanical":
            it.state, it.why = "judgement", reason
            continue
        if req.kind == "config":
            it.config = req.proposed
            it.state = "apply"
            continue
        owner_live = R.live_owner(it.resolved, live)
        if owner_live:
            it.state, it.why = "deferred", "owner %s has a live lane (claims)" % owner_live
            continue
        problems, news = [], []
        for r in it.resolved:
            news.append(None)
            if r.current is None:
                problems.append("%s: %s" % (r.target.symbol, "; ".join(r.notes) or "not in the map"))
                continue
            if R.is_member(r.current):
                problems.append("%s is the member %s now: call it through its owner (`obj->%s(...)`)"
                                % (r.target.symbol, r.current, r.current.split("__")[0]))
                continue
            if r.header is None:
                problems.append("%s: no owner header (%s)" % (r.current, "; ".join(r.notes) or r.owner_state))
                continue
            decided = R.decision_for(r.target, decisions)
            want = decided or r.target.proposed_name
            if want and want in (r.current, R.source_name(r.current)):
                continue                                   # the map already carries the name
            if not (req.kind == "rename" or libnames.is_generated(r.current, "map")):
                continue                                   # a plain declaration: no rename asked
            if not want:
                problems.append("%s needs a name: the request spells none - supply it with --names" % r.current)
            elif R.mangled(want):
                problems.append("%s -> %s is a mangling: the class work comes first (judgement)" % (r.current, want))
            elif req.confidence == "guess" and not decided:
                problems.append("%s -> %s is a GUESS: apply it only through a --names decision" % (r.current, want))
            elif want in ownership.symbols:
                problems.append("%s -> %s: the new name is already in the map" % (r.current, want))
            else:
                news[-1] = want
        if problems:
            judged = [x for x in problems if "is a mangling" in x or "is the member" in x]
            it.state = "judgement" if judged else "deferred"
            it.why = "; ".join(problems)
            continue
        for r, new in zip(it.resolved, news):
            if new:
                it.renames.append((r.current, new))
            final = new or R.source_name(r.current)
            spelled = r.target.symbol
            if spelled != final and _ident(spelled) and spelled != r.current and R.source_name(r.current) != spelled:
                it.rewrites.append((spelled, final))
            hint = r.target.prototype
            # the prototype spells the filed name or the lane's proposed one; a --names decision may have chosen a
            # third (L2 round 2: `isCircleListBusy` filed, `testAndSet611b` decided - the declaration named the wrong
            # function and every caller failed)
            for old in dict.fromkeys(x for x in (spelled, r.target.proposed_name, r.current) if x and _ident(x)):
                if hint and old != final:
                    hint = re.sub(r"\b%s\b" % re.escape(old), final, hint)
            it.decls.append(DeclOp(req.id, final, new or r.current, r.address, r.section, r.owner, r.header,
                                   cpp=libnames.is_mangled(r.current), hint=hint,
                                   data=(r.section or "") in DATA_SECTIONS,
                                   owned_by_lane=bool(r.owner and R.unit_stem(r.owner) in lane_stems),
                                   implicit=req.kind == "rename", move=req.kind == "decl-move"))
        it.state = "apply"
    return items


def _ident(s: str) -> bool:
    return bool(re.match(r"^[A-Za-z_]\w*$", s or ""))


# --- declaration editing ---------------------------------------------------------------------------------------

def _line_bounds(text: str, a: int, b: int) -> tuple[int, int]:
    """`(start of a's line, end of b's line incl. its newline)`."""
    s = text.rfind("\n", 0, a) + 1
    e = text.find("\n", b)
    return s, (len(text) if e < 0 else e + 1)


def statement_span(text: str, t: cscan.Text, decl_pos: int, name: str) -> tuple[int, int] | None:
    """The `[start, end)` of the declaration statement holding `decl_pos`, widened to whole lines when the lines
    hold nothing else (a trailing comment goes with it), plus a standalone `untyped:` marker line right above."""
    code = cscan.mask_preproc(t.code)
    start = max(code.rfind(";", 0, decl_pos), code.rfind("{", 0, decl_pos), code.rfind("}", 0, decl_pos)) + 1
    end = code.find(";", decl_pos)
    if end < 0:
        return None
    end += 1
    ls, le = _line_bounds(text, start + (len(code[start:end]) - len(code[start:end].lstrip())), end - 1)
    first = start + (len(code[start:end]) - len(code[start:end].lstrip()))
    before = code[ls:first]
    after = code[end:le]
    if before.strip() or after.strip():
        return first, end
    # an `untyped:` marker on its own line right above goes with the declaration it exempts
    prev_end = ls - 1
    if prev_end > 0:
        ps = text.rfind("\n", 0, prev_end) + 1
        if _UNTYPED_LINE_RE.match(text[ps:prev_end].rstrip("\r")):
            ls = ps
    return ls, le


def find_prototypes(text: str, name: str) -> list[tuple[int, int, str]]:
    """Every prototype (no body) of function `name` and every `extern` declaration of variable `name` in `text`,
    as `(start, end, statement text)`; never a definition."""
    if name not in text:
        return []
    t = cscan.Text(text)
    out = []
    for d in cscan.function_declarations(t):
        if d.name == name and d.body is None:
            span = statement_span(text, t, d.pos, name)
            if span:
                out.append((span[0], span[1], text[span[0]:span[1]]))
    masked = cscan.mask_preproc(t.code)
    for m in re.finditer(r"\bextern\b[^;{}()]*\b%s\b[^;{}()]*;" % re.escape(name), masked):
        if "(" in m.group(0):
            continue
        span = statement_span(text, t, m.start() + m.group(0).index(name), name)
        if span and not any(s <= span[0] < e for s, e, _ in out):
            out.append((span[0], span[1], text[span[0]:span[1]]))
    return sorted(out)


def definition_prototype(text: str, name: str, data: bool) -> str | None:
    """The declaration a definition of `name` in `text` implies: `ret name(params);` for a function, `extern type
    name[dims];` for a variable at file scope."""
    if name not in text:
        return None
    t = cscan.Text(text)
    if not data:
        for d in cscan.function_declarations(t):
            if d.name == name and d.body is not None:
                ret = re.sub(r"\s+", " ", cscan.mask_preproc(t.code)[d.ret_pos:d.pos]).strip()
                ret = re.sub(r"^(?:extern\s+\"?C?\"?\s*|static\s+)", "", ret).strip()
                params = re.sub(r"\s+", " ", text[d.params_pos:d.params_pos + len(d.params)]).strip()
                params = re.sub(r"/\*.*?\*/", "", params).strip()
                return "%s %s(%s);" % (ret, name, params or "void")
        return None
    code = cscan.mask_preproc(t.code)
    depth = 0
    for m in re.finditer(r"(?m)[{}]|^[ \t]*([A-Za-z_][\w \t\*]*?)\b%s\b[ \t]*((?:\[[^\]]*\])*)[ \t]*(?:=[^;]*)?;"
                         % re.escape(name), code):
        tok = m.group(0)
        if tok == "{":
            depth += 1
        elif tok == "}":
            depth -= 1
        elif depth == 0 and m.group(1) is not None and "extern" not in m.group(1) and "typedef" not in m.group(1):
            typ = re.sub(r"\s+", " ", m.group(1)).strip()
            typ = re.sub(r"^static\s+", "", typ)
            return "extern %s %s%s;" % (typ, name, m.group(2) or "")
    return None


def needed_types(proto: str, name: str) -> list[str]:
    """The type names a prototype uses (its return type and parameter types, not the parameter names)."""
    m = re.match(r"^\s*(?:extern\s+)?(.*?)\b%s\b\s*(\((.*)\))?" % re.escape(name), proto)
    if not m:
        return []
    groups = [m.group(1)]
    if m.group(3):
        for p in cscan._split_top(m.group(3).replace(",", ";")):
            p = p.strip()
            if not p or p == "void" or p == "...":
                continue
            ids = re.findall(r"[A-Za-z_]\w*", re.sub(r"\[[^\]]*\]", "", p))
            ids = [i for i in ids if i not in ("const", "volatile", "struct", "class", "union", "enum")]
            if len(ids) >= 2:
                ids = ids[:-1]                     # the last identifier is the parameter's name
            groups.append(" ".join(ids))
    out = []
    for g in groups:
        for i in re.findall(r"[A-Za-z_]\w*", g):
            if i not in BUILTIN_TYPES and i not in out and i not in ("struct", "class", "union", "enum"):
                out.append(i)
    return out


def visible_names(files: Files, header: str) -> set[str]:
    """Every type-ish name the header's include closure declares (definitions, typedefs, forward declarations)."""
    names: set[str] = set()
    if not files.exists(header):
        return names
    for r in files.closure(header):
        if r.startswith(".."):
            continue
        clean = cscan.strip_comments(files.get(r))
        names |= {d.name for d in cscan.declared_names(clean)}
        names |= {m.group(2) for m in _FWD_RE.finditer(clean)}
    return names


def type_keyword(files: Files, name: str) -> str | None:
    """`class`/`struct`/`union` for a type the tree defines (the keyword a forward declaration must use)."""
    rx = re.compile(r"\b(class|struct|union)\s+%s\b\s*[:{]" % re.escape(name))
    for r in files.all():
        if r.startswith("include/"):
            t = files.get(r)
            if name in t:
                m = rx.search(cscan.strip_comments(t))
                if m:
                    return m.group(1)
    return None


_CPP_ONLY_RE = re.compile(r"\bclass\s+\w+\s*[:{;]|\bpublic\s*:|\btemplate\s*<|\bnamespace\b")


def c_header(text: str, owner: str | None) -> bool:
    """Whether a header is (also) read by C: its owner is a `.c` unit, or it guards `__cplusplus` and declares no
    C++-only construct (a class, `public:`, a template, a namespace)."""
    if (owner or "").endswith(".c"):
        return True
    code = cscan.strip_comments(text)
    return "__cplusplus" in code and not _CPP_ONLY_RE.search(code)


def guard_name(header: str) -> str:
    stem = header[len("include/"):] if header.startswith("include/") else header
    return "MHTRI_" + re.sub(r"[^A-Za-z0-9]", "_", os.path.splitext(stem)[0]).upper() + "_H"


def new_header_text(header: str, owner: str | None, nl: str = "\n") -> str:
    g = guard_name(header)
    lines = ["/*",
             " * Declarations for the symbols `src/%s` owns that other units use (docs/plan.md 6.5, rule 2)." % owner,
             " */",
             "#ifndef %s" % g, "#define %s" % g, "", '#include "types.h"', "", "#endif /* %s */" % g, ""]
    return nl.join(lines)


def insert_declarations(text: str, cpp_block: str, c_block: str) -> str:
    """`text` with the C++-linkage declarations before the closing include guard and the C-linkage ones inside
    the header's `extern "C"` region (one is opened before the guard when the header has none)."""
    nl = libtext.line_ending(text)
    if c_block:
        closes = list(_CPP_CLOSE_RE.finditer(text))
        plain = list(_PLAIN_LINKAGE_RE.finditer(text))
        if closes:
            pos = closes[-1].start()
            text = text[:pos] + ("" if text[:pos].endswith(nl + nl) else nl) + c_block + nl + text[pos:]
        elif plain:
            open_m = plain[-1]
            close = cscan.match_brace(cscan.strip_comments(text), text.index("{", open_m.start()))
            ls = text.rfind("\n", 0, close) + 1
            text = text[:ls] + nl + c_block + text[ls:]
        else:
            wrapped = (nl.join(["#ifdef __cplusplus", 'extern "C" {', "#endif", ""]) + nl + c_block + nl
                       + nl.join(["#ifdef __cplusplus", "}", "#endif", ""]))
            text = _insert_before_guard(text, wrapped, nl)
    if cpp_block:
        opens = list(_CPP_OPEN_RE.finditer(text))
        plain = list(_PLAIN_LINKAGE_RE.finditer(text))
        if opens:
            pos = opens[0].start()
            text = text[:pos] + cpp_block + nl + text[pos:]
        elif plain:
            pos = plain[0].start()
            text = text[:pos] + cpp_block + nl + text[pos:]
        else:
            text = _insert_before_guard(text, cpp_block, nl)
    return text


def _insert_before_guard(text: str, block: str, nl: str) -> str:
    m = _GUARD_END_RE.search(text)
    if m:
        pos = m.start()
        lead = "" if text[:pos].endswith(nl + nl) else nl
        return text[:pos] + lead + block + nl + text[pos:]
    return text + ("" if text.endswith(nl) else nl) + nl + block


def decl_block(ops: list[DeclOp], nl: str) -> str:
    lines = []
    for op in sorted(ops, key=lambda o: (o.address or 0, o.name)):
        where = ("%s 0x%08X" % (op.section, op.address)) if op.data and op.address is not None else \
            ("0x%08X" % op.address if op.address is not None else op.map_name)
        lines.append("/* %s */" % where)
        if op.result.get("marker"):
            lines.append(op.result["marker"])
        lines.append(op.result["prototype"])
    return nl.join(lines) + nl


def drop_empty_linkage(text: str) -> str:
    """Remove `extern "C"` blocks (either spelling) that hold no code any more - only comments and blank lines.
    The block is found in the text (the code view blanks the `"C"`), its emptiness judged on the code view."""
    changed = True
    while changed:
        changed = False
        code = cscan.strip_comments(text)
        for m in list(_CPP_OPEN_RE.finditer(text))[::-1]:
            if code[m.start():m.end()].strip() == "" or "extern" not in code[m.start():m.end()]:
                continue                                     # the opener itself sits in a comment
            close = _CPP_CLOSE_RE.search(text, m.end())
            if close and not code[m.end():close.start()].strip():
                text = text[:m.start()] + text[close.end():]
                changed = True
                break
        if changed:
            continue
        for m in list(_PLAIN_LINKAGE_RE.finditer(text))[::-1]:
            if "extern" not in code[m.start():m.end()]:
                continue
            open_pos = text.index("{", m.start())
            close = cscan.match_brace(code, open_pos)
            if close > 0 and not code[open_pos + 1:close].strip():
                _s, e = _line_bounds(text, close, close)
                text = text[:m.start()] + text[e:]
                changed = True
                break
    return text


_STOPGAP_HEAD_RE = re.compile(r"/\*\s*(?:PILOT\s+)?STOPGAP\b(?!-)")


def drop_orphan_stopgap_comments(text: str) -> str:
    """Remove a comment that opens with `STOPGAP` / `PILOT STOPGAP` (a legacy stopgap's header) when the next
    non-blank line is not code it could describe (EOF, an `#include`/`#pragma`/`#if`, another comment). A comment
    that only mentions the word (a unit header's paragraph) is never touched."""
    while True:
        changed = False
        for sp in cscan.spans(text):
            if sp.kind != "block" or not _STOPGAP_HEAD_RE.match(text, sp.start):
                continue
            ls, le = _line_bounds(text, sp.start, sp.end - 1)
            if text[ls:sp.start].strip() or text[sp.end:le].strip():
                continue
            rest = text[le:].lstrip()
            if not rest or rest.startswith(("#include", "#pragma", "#if", "/*", "//")):
                text = text[:ls] + text[le:]
                changed = True
                break
        if not changed:
            return text


def _next_line(text: str, pos: int) -> str:
    e = text.find("\n", pos)
    return text[pos:len(text) if e < 0 else e].strip()


def drop_new_orphan_comments(text: str, original: str) -> str:
    """Remove a comment that stands on its own lines and described a declaration this edit removed: in `original`
    the next line was code, now it is a blank line, the end of a linkage block or the end of the file. A comment
    whose text is not unique in `original` is left alone (it cannot be traced)."""
    while True:
        changed = False
        for sp in cscan.spans(text):
            if sp.kind != "block":
                continue
            ls, le = _line_bounds(text, sp.start, sp.end - 1)
            if text[ls:sp.start].strip() or text[sp.end:le].strip():
                continue
            body = text[sp.start:sp.end]
            if original.count(body) != 1:
                continue
            o_end = original.index(body) + len(body)
            o_next = _next_line(original, original.find("\n", o_end) + 1 if "\n" in original[o_end:] else len(original))
            if not o_next or o_next.startswith(("/*", "//", "#")):
                continue
            n_next = _next_line(text, le)
            if n_next == "" or n_next.startswith(("#ifdef __cplusplus", "#endif", "}")) or le >= len(text):
                text = text[:ls] + text[le:]
                changed = True
                break
        if not changed:
            return text


def collapse_blank_runs(text: str, original: str) -> str:
    """Collapse a run of two or more blank lines the edit created (its neighbour lines were not that far apart in
    `original`) to one blank line; a run the file already had is left alone."""
    nl = libtext.line_ending(text)
    flat_old = original.replace("\r\n", "\n")
    for m in reversed(list(re.finditer(r"(?:\r?\n){3,}", text))):
        ps = text.rfind("\n", 0, m.start()) + 1
        prev_line = text[ps:m.start()].replace("\r", "")
        ne = text.find("\n", m.end())
        next_line = text[m.end():ne if ne >= 0 else len(text)].replace("\r", "")
        gap = m.group(0).replace("\r", "")
        if (prev_line + gap + next_line) in flat_old:
            continue
        text = text[:m.start()] + nl + nl + text[m.end():]
    return text


def _strip_declarations(inner: str) -> str:
    """`inner` without its function prototypes and `extern` variable declarations (what a STOPGAP block stands in
    for); what is left - a typedef the code casts through, a helper type - is code the unit still uses."""
    t = cscan.Text(inner)
    spans = []
    for d in cscan.function_declarations(t):
        if d.body is None:
            span = statement_span(inner, t, d.pos, d.name)
            # `typedef s32 (*Getter)(...)` reads as a prototype of `Getter` to cscan: it is a type the code casts
            # through, not a declaration of a symbol
            if span and not re.match(r"\s*typedef\b", cscan.strip_comments(inner[span[0]:span[1]])):
                spans.append(span)
    masked = cscan.mask_preproc(t.code)
    for m in re.finditer(r"\bextern\b[^;{}()]*;", masked):
        span = statement_span(inner, t, m.start() + len("extern"), "")
        if span and not any(s <= span[0] < e for s, e in spans):
            spans.append(span)
    for s, e in sorted(spans, reverse=True):
        inner = inner[:s] + inner[e:]
    return inner


def remove_stopgap_blocks(text: str, ids: set[str]) -> tuple[str, list[str], list[int]]:
    """Delete every `STOPGAP-BEGIN(<id>)..STOPGAP-END(<id>)` block whose id is in `ids`: its markers and its
    declarations go; any other code inside (a typedef a call site casts through - the L2 batch's
    `CircleInfoSetSender`, whose removal broke the build) stays where it was, unwrapped."""
    done, points = [], []
    for rid, s, e in sorted(R.stopgap_blocks(text), key=lambda x: -x[1]):
        if rid in ids:
            block = text[s:e]
            b = R.STOPGAP_BEGIN_RE.search(block)
            f = R.STOPGAP_END_RE.search(block, b.end())
            rest = _strip_declarations(block[b.end():f.start()])
            if cscan.strip_comments(rest).strip():
                rest = rest.lstrip("\r\n").rstrip(" \t")
                nl = libtext.line_ending(text)
                keep = rest if rest.endswith("\n") else rest + nl
            else:
                keep = ""
            text = text[:s] + keep + text[e:]
            done.append(rid)
            points.append(s)
    return text, done, points


def leaf_header(op: "DeclOp") -> str:
    """The leaf header of `op`'s symbol: `include/<the owner header's directory>/<symbol>.h` (section 6.5 rule 2's
    one other owner spelling - for an owner whose full header clashes with a consumer)."""
    base = os.path.dirname(op.header).replace("\\", "/") or "include"
    return "%s/%s.h" % (base, op.name)


def adopt_leaf(files: Files, op: "DeclOp") -> None:
    """Point `op` at its leaf header when one already declares the symbol (an earlier pass's fallback), so a rerun
    never declares it a second time in the clashing full header."""
    lp = leaf_header(op)
    if not op.leaf and lp != op.header and files.exists(lp) and op.name in files.declared(lp):
        op.full_header, op.header, op.leaf = op.header, lp, True


def decl_sites(files: Files, op: "DeclOp", scope_files: set[str]) -> list[tuple]:
    """Every prototype/`extern` of `op.name` outside its header: `(file, start, end, statement, in_scope)`, where
    `in_scope` says the integrator may remove it (the lane's files, a lane-owned symbol, a decl-move's band)."""
    owner_src = "src/%s" % op.owner if op.owner else None
    sites = []
    for r in files.all():
        if r == op.header:
            continue
        t = files.get(r)
        if op.name not in t:
            continue
        in_scope = ((r in scope_files and r != owner_src) or (op.owned_by_lane and r != owner_src)
                    or (op.move and (r.startswith("include/unsplit/") or r == owner_src)))
        for s, e, stmt in find_prototypes(t, op.name):
            sites.append((r, s, e, stmt, in_scope))
    return sites


def needs_declaration(files: Files, op: "DeclOp", all_files: bool = False):
    """Whether a file other than the owner's source and header calls `op.name` without seeing a declaration of it
    (`all_files`: the list of those files, for the report)."""
    owner_src = "src/%s" % op.owner if op.owner else None
    out = []
    for r in files.all():
        if r in (op.header, owner_src):
            continue
        t = files.get(r)
        if op.name not in t or not _code_refers(t, op.name) or files.sees(r, op.name):
            continue
        if not all_files:
            return True
        out.append(r)
    return out if all_files else False


def apply_decls(files: Files, ops: list[DeclOp], scope_files: set[str], applied_ids: set[str],
                mangle_check=None) -> dict[str, list[str]]:
    """Edit the tree for every DeclOp: declare in the owner header (prototype from the owner's definition, the
    request or the lane's local declaration, in that order of authority - the definition is what the owner
    compiles), remove the foreign declarations in scope, add the owner header's include where a referrer no longer
    sees a declaration, and drop the STOPGAP blocks of the applied requests. Returns `{rid: [failure]}`."""
    failures: dict[str, list[str]] = {}
    touched: set[str] = set()
    by_header: dict[str, list[DeclOp]] = {}

    def refuse(op: DeclOp, why: str) -> None:
        """A rename's implicit declaration that cannot move is a note, not a refusal: the rename still applies and
        the lane's (renamed) local declaration stays where it is. One that a caller NEEDS (`needed_by`) is a
        refusal: without it the rename leaves the caller with no declaration."""
        if op.implicit and not op.result.get("needed_by"):
            if why not in op.result.setdefault("not_moved", []):
                op.result["not_moved"].append(why)
        else:
            op.result.setdefault("refused", []).append(why)
            failures.setdefault(op.rid, []).append(why)

    # 0. explicit STOPGAP blocks of the applied requests go first (their declarations go with them, and are kept as
    #    the lane's spelling of a prototype nothing else spells)
    removed: list[tuple[str, str]] = []
    for r in files.all():
        t = files.get(r)
        if "STOPGAP-BEGIN" in t:
            removed += [(r, t[s:e]) for rid, s, e in R.stopgap_blocks(t) if rid in applied_ids]
            new, done, _points = remove_stopgap_blocks(t, applied_ids)
            if done:
                files.set(r, new)
                touched.add(r)
    for op in ops:
        op.result = {}
    for op in ops:
        owner_src = "src/%s" % op.owner if op.owner else None
        adopt_leaf(files, op)
        sites = decl_sites(files, op, scope_files)
        if op.implicit and not any(x[4] for x in sites) and not op.force:
            # a rename's declaration: moved when the lane declared it locally, and written to the owner header when
            # a caller would otherwise see none (its STOPGAP block went in step 0 - the L2 batch's 9 renames)
            if not needs_declaration(files, op):
                op.result["skipped"] = "no declaration in the lane's scope to move, and every caller sees one"
                continue
            op.result["needed_by"] = needs_declaration(files, op, all_files=True)
        proto, source = None, None
        if owner_src and files.exists(owner_src):
            proto = definition_prototype(files.get(owner_src), op.name, op.data)
            source = "the owner's definition" if proto else None
        if not proto and op.hint:
            proto, source = op.hint.strip(), "the request"
        lane_site = next((x for x in sites if x[4]), None) or (sites[0] if sites else None)
        if lane_site is None:
            lane_site = next(((r, s, e, stmt, False) for r, block in removed
                              for s, e, stmt in find_prototypes(block, op.name)), None)
        marker = None
        if lane_site:
            lines = lane_site[3].strip("\r\n").splitlines()
            if lines and _UNTYPED_LINE_RE.match(lines[0]):
                marker = lines[0].strip()
            if not proto:
                proto = cscan.remove_comments("\n".join(l for l in lines if not _UNTYPED_LINE_RE.match(l)))
                proto = re.sub(r"\s+", " ", proto).strip()
                proto = re.sub(r'^extern\s+"C"\s+', "", proto)
                if not op.data:
                    proto = re.sub(r"^extern\s+", "", proto)
                source = "the lane's declaration (%s%s)" % (lane_site[0], "" if lane_site in sites else
                                                              ", its STOPGAP block")
        if not proto:
            refuse(op, "%s: no prototype (the request, the owner and the lane spell none)" % op.name)
            continue
        if op.data and not proto.startswith("extern"):
            proto = "extern " + proto
        if "void*" in proto.replace(" *", "*") and not marker and owner_src and files.exists(owner_src):
            marker = _definition_marker(files.get(owner_src), op.name)
        header_text = files.get(op.header) if files.exists(op.header) else None
        already = header_text is not None and op.name in files.declared(op.header)
        op.result.update({"prototype": proto, "source": source, "marker": marker, "already_declared": already})
        if not already:
            if op.owner and op.owner.endswith(".c") and ("&" in proto or "::" in proto):
                refuse(op, "%s: a C++ prototype for the C owner %s" % (op.name, op.owner))
                continue
            visible = visible_names(files, op.header) if header_text is not None else set()
            missing = [n for n in needed_types(proto, op.name) if n not in visible]
            fwd, bad = [], []
            for n in missing:
                kw = type_keyword(files, n)
                ptr_use = re.search(r"\b%s\b\s*[\*&]" % re.escape(n), proto) is not None
                c_consumed = c_header(header_text or "", op.owner)
                if kw and ptr_use and not c_consumed:
                    fwd.append("%s %s;" % (kw, n))
                else:
                    why = ("not a type the tree defines" if not kw else "a by-value use" if not ptr_use else
                           "a C-consumable header cannot forward-declare it without changing the prototype")
                    bad.append("%s: the prototype names %s, which %s cannot see (%s)" % (op.name, n, op.header, why))
            for why in bad:
                refuse(op, why)
            if bad:
                continue
            op.result["forward"] = fwd
            by_header.setdefault(op.header, []).append(op)
        op.result["sites_removed"] = []
        for r, s, e, _stmt, in_scope in sorted(sites, key=lambda x: (x[0], -x[1])):
            if in_scope:
                t = files.get(r)
                files.set(r, t[:s] + t[e:])
                touched.add(r)
                op.result["sites_removed"].append(r)
        op.result["kept_sites"] = sorted({x[0] for x in sites if not x[4]})

    # 1. the declarations, per header
    for header, hops in by_header.items():
        text = files.get(header) if files.exists(header) else new_header_text(header, hops[0].owner)
        nl = libtext.line_ending(text) if text else "\n"
        fwd = sorted({f for o in hops for f in o.result.get("forward", [])})
        cpp_ops = [o for o in hops if o.cpp]
        c_ops = [o for o in hops if not o.cpp]
        text = insert_declarations(text, decl_block(cpp_ops, nl) if cpp_ops else "",
                                   decl_block(c_ops, nl) if c_ops else "")
        if fwd:
            text = _insert_forward(text, fwd, nl)
        files.set(header, text)
        if mangle_check:
            for o in cpp_ops:
                got = mangle_check(header, o)
                o.result["mangled"] = got
                if got and got != o.map_name:
                    o.result.setdefault("refused", []).append("mangles to %s" % got)
                    failures.setdefault(o.rid, []).append("%s: the declaration mangles to %s, the map says %s"
                                                          % (o.name, got, o.map_name))

    # 2. cleanups where something was removed
    for r in sorted(touched):
        t = files.get(r)
        t = drop_empty_linkage(t)
        t = drop_orphan_stopgap_comments(t)
        t = drop_new_orphan_comments(t, files.original.get(r) or "")
        t = collapse_blank_runs(t, files.original.get(r) or "")
        files.set(r, t)

    # 3. includes: every referrer that no longer sees a declaration of the name gets the owner header
    want: dict[str, dict[str, list[str]]] = {}
    for op in ops:
        if op.rid in failures or op.result.get("skipped") or "prototype" not in op.result:
            continue
        inc = op.header[len("include/"):] if op.header.startswith("include/") else op.header
        for r in files.all():
            if r == op.header:
                continue
            t = files.get(r)
            if op.name not in t or not _code_refers(t, op.name) or files.sees(r, op.name):
                continue
            want.setdefault(r, {}).setdefault(inc, []).append(op.name)
            op.result.setdefault("includes_added", []).append(r)
    for r, incs in want.items():
        files.set(r, add_includes(files.get(r), incs))
    return failures


def _definition_marker(text: str, name: str) -> str | None:
    t = cscan.Text(text)
    for d in cscan.function_declarations(t):
        if d.name == name and d.body is not None:
            ln = t.line_of(d.ret_pos) - 1
            if ln >= 1 and _UNTYPED_LINE_RE.match(t.line_text(ln)):
                return t.line_text(ln).strip()
    return None


def _insert_forward(text: str, fwd: list[str], nl: str) -> str:
    have = {m.group(0) for m in _FWD_RE.finditer(cscan.strip_comments(text))}
    add = [f for f in fwd if f not in have]
    if not add:
        return text
    incs = list(cscan.INCLUDE_RE.finditer(text))
    pos = _line_bounds(text, incs[-1].start(), incs[-1].end())[1] if incs else 0
    return text[:pos] + nl + nl.join(add) + nl + text[pos:]


def _code_refers(text: str, name: str) -> bool:
    code = cscan.mask_preproc(cscan.strip_comments(text))
    return re.search(r"(?<![\w@$.])%s\b" % re.escape(name), code) is not None


_VAR_DECL_RE = re.compile(r"(?m)^[ \t]*(?:extern\b[^;{}()]*?|[A-Za-z_][\w \t\*]*?)\b([A-Za-z_]\w*)[ \t]*"
                          r"(?:\[[^\]]*\])*[ \t]*(?:=[^;{}]*)?;")


def declared_in(text: str) -> set[str]:
    """Every function (prototype or definition) and every file-scope or `extern` variable `text` declares."""
    t = cscan.Text(text)
    out = {d.name for d in cscan.function_declarations(t)}
    code = cscan.mask_preproc(t.code)
    depth, at = 0, 0
    for m in _VAR_DECL_RE.finditer(code):
        seg = code[at:m.start()]
        depth += seg.count("{") - seg.count("}")
        at = m.start()
        if depth == 0 or "extern" in m.group(0):
            out.add(m.group(1))
    return out


def add_includes(text: str, incs: dict[str, list[str]]) -> str:
    """`text` with one `#include "<inc>"   /* names */` per header, after the last include of its include block
    (the block before the first top-level brace) - where the hand integrator put them."""
    nl = libtext.line_ending(text)
    lines = []
    for inc, names in incs.items():
        if re.search(r'#[ \t]*include[ \t]*"%s"' % re.escape(inc), text):
            continue
        lines.append('#include "%s"   /* %s */%s' % (inc, ", ".join(dict.fromkeys(names)), nl))
    if not lines:
        return text
    code = cscan.strip_comments(text)
    first_brace = code.find("{")
    incs_found = [m for m in cscan.INCLUDE_RE.finditer(text) if first_brace < 0 or m.start() < first_brace]
    if incs_found:
        pos = _line_bounds(text, incs_found[-1].start(), incs_found[-1].end())[1]
    else:
        guard = re.search(r"(?m)^[ \t]*#[ \t]*define[ \t]+\w+[^\n]*\n", text)
        pos = guard.end() if guard else 0
    return text[:pos] + "".join(lines) + text[pos:]


# --- build, report, gate pre-run ----------------------------------------------------------------------------

def target_snapshot(root: str) -> dict[str, tuple[str, str] | None]:
    """`{unit stem: (raw sha1, objcompare.fingerprint)}` for every registered unit's split target object - the
    gate's drift reading (`objcompare.fingerprint`) plus the raw hash that tells a names-only change apart."""
    splits = os.path.join(root, "config", "RMHE08", "splits.txt")
    out = {}
    for unit in Splits.read(splits).units:
        stem = R.unit_stem(unit)
        path = os.path.join(root, BUILD, "obj", stem + ".o")
        if os.path.exists(path):
            with open(path, "rb") as fh:
                raw = hashlib.sha1(fh.read()).hexdigest()
            out[stem] = (raw, objcompare.fingerprint(path))
        else:
            out[stem] = None
    return out


def drift(before: dict, after: dict) -> dict:
    names_only, content = [], []
    for stem in sorted(set(before) | set(after)):
        b, a = before.get(stem), after.get(stem)
        if b == a:
            continue
        if b is None or a is None or b[1] != a[1]:
            content.append(stem)
        else:
            names_only.append(stem)
    return {"names_only": names_only, "content": content}


def parse_build_errors(output: str) -> list[dict]:
    """`[{file, line, text, message}]` of the compiler's ERROR blocks in a ninja log (MWCC's `# File:` / `#  N: text`
    / `#   Error:` / `#   (NNNNN) message`); a warning block is skipped."""
    out = []
    cur, last, pending = None, None, None
    for line in output.splitlines():
        m = _ERROR_FILE_RE.search(line)
        if m:
            cur = m.group(1).replace("\\", "/")
            continue
        s = line.strip()
        m = _ERROR_LINE_RE.match(s)
        if m and cur:
            last = {"file": cur, "line": int(m.group(1)), "text": m.group(2)}
            continue
        if re.match(r"#\s+Error:", s) and last:
            pending = dict(last, message="")
            out.append(pending)
            continue
        m = re.match(r"#\s+\(\d+\)\s*(.*)", s)
        if m and pending is not None and not pending["message"]:
            pending["message"] = m.group(1)
            pending = None
    return out


def failed_objects(output: str) -> list[str]:
    return sorted(set(m.group(1).replace("\\", "/") for m in re.finditer(r"FAILED: (\S+\.o)", output)))


def run_build(root: str, log: list[str]) -> tuple[bool, str, float]:
    t0 = time.time()
    p = proc.run(["ninja", "-k", "0"], cwd=root, timeout=3600)
    dt = time.time() - t0
    out = (p.stdout or "") + (p.stderr or "")
    log.append("ninja -k 0: exit %d in %.0f s" % (p.returncode, dt))
    return p.returncode == 0, out, dt


def report_moved(before: str | None, after_path: str) -> dict:
    if not before or not os.path.exists(after_path):
        return {"compared": False}
    with open(before, encoding="utf-8") as fh:
        b = json.load(fh)
    with open(after_path, encoding="utf-8") as fh:
        a = json.load(fh)
    cmp_ = libreport.compare(b, a)
    syms = cmp_["symbols"]["moved"]
    return {"compared": True, "symbols_moved": len(syms), "down": sum(1 for r in syms if r["delta"] < 0),
            "up": sum(1 for r in syms if r["delta"] > 0), "drops": cmp_["drops"][:10]}


def snapshot_objects(root: str) -> str | None:
    """A copy of the tree's compiled objects (`build/RMHE08/src/**.o`) as a tree `objsame.py` can read as BASE."""
    src = os.path.join(root, BUILD, "src")
    if not os.path.isdir(src):
        return None
    dst_root = tempfile.mkdtemp(prefix="integrate-objs-")
    for base, _dirs, files in os.walk(src):
        for f in files:
            if f.endswith(".o"):
                p = os.path.join(base, f)
                q = os.path.join(dst_root, BUILD, "src", os.path.relpath(p, src))
                os.makedirs(os.path.dirname(q), exist_ok=True)
                with open(p, "rb") as a, open(q, "wb") as b:
                    b.write(a.read())
    return dst_root


def objects_same(base_tree: str | None, root: str) -> dict:
    """`objsame.py BASE TREE` (by subprocess - one implementation): the compiled objects that differ modulo `@N`,
    split into `names_only` (only relocation-target or symbol-table reasons: the renames) and `code` (a section's
    bytes or size moved)."""
    if not base_tree:
        return {"compared": False}
    p = run_tool("tools/objdiff/objsame.py", base_tree, root, cwd=root, timeout=1200)
    names_only, code, one_sided = [], [], []
    for line in (p.stdout or "").splitlines():
        parts = line.split(None, 2)
        if len(parts) >= 2 and parts[0] == "DIFFER":
            reasons = parts[2] if len(parts) > 2 else ""
            moved = any(w in reasons for w in (" size ", "bytes differ", " only in "))
            (code if moved else names_only).append(parts[1])
        elif len(parts) >= 2 and parts[0] in ("BASE", "TREE", "UNREAD"):
            one_sided.append("%s %s" % (parts[0], parts[1]))
    summary = (p.stdout or "").strip().splitlines()[-1:] or [""]
    return {"compared": True, "summary": summary[0], "names_only": names_only, "code": code,
            "one_sided": one_sided}


def gate_prerun(root: str, base: str, units: list[str]) -> dict:
    out = {}
    p = run_tool("tools/units/stylelint.py", "--diff", base, "--json", cwd=root, timeout=1200)
    try:
        j = json.loads(p.stdout)
        added = j.get("added") or []
        out["stylelint"] = {"exit": p.returncode, "added": sum(a.get("added", 0) for a in added),
                            "rows": added[:12]}
    except (json.JSONDecodeError, AttributeError):
        out["stylelint"] = {"exit": p.returncode, "tail": (p.stdout or p.stderr or "")[-600:]}
    p = run_tool("tools/units/undefrefs.py", "--main", root, "--base", base, *[R.unit_stem(u) for u in units],
                 cwd=root, timeout=1200)
    out["undefrefs"] = {"exit": p.returncode, "tail": "\n".join((p.stdout or p.stderr or "").strip().splitlines()[-4:])}
    p = run_tool("tools/units/vtableaudit.py", "--main", root, "--diff", base, cwd=root, timeout=1200)
    out["vtableaudit"] = {"exit": p.returncode,
                          "tail": "\n".join((p.stdout or p.stderr or "").strip().splitlines()[-3:])}
    return out


# --- the run ---------------------------------------------------------------------------------------------------------

def changed_units(root: str, base: str, extra_files: list[str]) -> list[str]:
    """Registered units whose source changed since `base` (a `src/` file whose stem is a splits.txt unit)."""
    names = Git(root).diff_names(base)
    files = set(names) | set(extra_files)
    units = {R.unit_stem(u) for u in Splits.read(os.path.join(root, "config", "RMHE08", "splits.txt")).units}
    out = []
    for f in sorted(files):
        if f.startswith("src/"):
            stem = os.path.splitext(f[len("src/"):])[0]
            if stem in units:
                out.append(stem)
    return out


def land_command(branch: str, units: list[str], files: list[str]) -> str:
    """The landing line: every changed unit, a module-dir file first (land.py derives the module from the head)."""
    ordered = sorted(units, key=lambda u: (0 if "/" in u else 1, u))
    return "python tools/units/land.py land --branch %s --units %s" % (branch, ",".join(ordered))


def make_branch(root: str, base: str | None, branch: str | None) -> str:
    g = Git(root)
    cur = g.current_branch() or ""
    if branch is None and cur.startswith("integrate/") and not base:
        return cur
    if branch is None:
        day = datetime.date.today().isoformat()
        branch, n = "integrate/%s" % day, 1
        while g.branch_exists(branch):
            n += 1
            branch = "integrate/%s-%d" % (day, n)
    start = base or "HEAD"
    p = g.run("switch", "-c", branch, start)
    if p.returncode != 0:
        raise SystemExit("integrate: cannot create %s from %s: %s" % (branch, start, p.stderr.strip()))
    return branch


def mangle_checker(root: str):
    def check(header: str, op: DeclOp) -> str | None:
        inc = header[len("include/"):]
        snippet = '#include "%s"\n%s' % (inc, op.result["prototype"])
        p = run_tool("tools/units/mangle.py", "--unit", op.owner or "", "--json", snippet, cwd=root, timeout=180)
        try:
            names = json.loads(p.stdout).get("mangled") or []
        except (json.JSONDecodeError, AttributeError):
            return None
        hit = [n for n in names if n.split("__")[0] == op.name]
        return hit[0] if hit else (names[0] if names else None)
    return check


def apply_plan(root: str, items: list[Item], lane_scope: set[str], comments: bool, mangle: bool,
               stage: str = "all") -> dict:
    """Edit the tree for every `apply` item. `stage`: `renames` (map + sweep only), `rest` (declarations, config),
    or `all`. Returns `{applied_files, failures}`."""
    result = {"files": [], "failures": {}}
    todo = [it for it in items if it.state == "apply"]
    if stage in ("renames", "all"):
        pairs = list(dict.fromkeys((o, n) for it in todo for o, n in it.renames))
        if pairs:
            with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, encoding="utf-8") as fh:
                fh.write("".join("%s %s\n" % p for p in pairs))
                path = fh.name
            args = ["rename-batch", path, "--rewrite", "--no-refs"] + (["--comments"] if comments else [])
            p = run_tool("tools/symbols/symedit.py", *args, cwd=root, timeout=600)
            os.unlink(path)
            if p.returncode != 0:
                raise SystemExit("integrate: symedit rename-batch refused:\n%s" % (p.stdout + p.stderr)[-2000:])
            result["rename_log"] = (p.stdout or "").strip().splitlines()[-1:]
    if stage in ("rest", "all"):
        # a lane's stale spelling of a name the map already changed: source only, so the renames stage is the map
        # and its sweep alone (the first commit)
        stale = list(dict.fromkeys((o, n) for it in todo for o, n in it.rewrites))
        if stale:
            with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, encoding="utf-8") as fh:
                fh.write("".join("%s %s\n" % p for p in stale))
                path = fh.name
            p = run_tool("tools/symbols/symedit.py", "rewrite-batch", path, *(["--comments"] if comments else []),
                         cwd=root, timeout=600)
            os.unlink(path)
            result["rewrite_log"] = (p.stdout or "").strip().splitlines()[-1:]
    if stage in ("rest", "all"):
        files = Files(root)
        scope = set(lane_scope)
        for r in files.all():
            t = files.get(r) if r.startswith("src/") else ""
            if "STOPGAP" in t:
                scope.add(r)
        ops = dedupe_ops([op for it in todo for op in it.live_decls()])
        applied_ids = {it.req.id for it in todo}
        result["failures"] = apply_decls(files, ops, scope, applied_ids, mangle_checker(root) if mangle else None)
        for it in todo:
            if it.config:
                cfg = os.path.join(root, "config", "RMHE08", "config.yml")
                result["failures"].update(apply_config(cfg, it))
        result["files"] = files.flush()
    return result


def dedupe_ops(ops: list[DeclOp]) -> list[DeclOp]:
    """One DeclOp per (name, header): a `decl` request's op wins over a rename's implicit one (two requests often
    name one symbol - `rename fn_X` and `decl fn_X`)."""
    best: dict[tuple[str, str], DeclOp] = {}
    for op in ops:
        key = (op.name, op.header)
        cur = best.get(key)
        if cur is None or (cur.implicit and not op.implicit) or (not cur.hint and op.hint and
                                                                 cur.implicit == op.implicit):
            best[key] = op
    return [op for op in ops if best.get((op.name, op.header)) is op]


def config_edit(old: str, it: Item) -> tuple[str, int, str | None, str | None]:
    """`(new text, items added, key, refusal)`: the request's relocation items (`lib.requests.config_items`, every form
    a lane files) appended at the end of their key's block, judged by the guard's own rule (`lib.repo.config_change`);
    an item already present is not added again."""
    section = it.req.targets[0].section if it.req.targets else None
    key, items, why = R.config_items(it.config or "", section)
    if not key:
        return old, 0, None, why
    nl = libtext.line_ending(old)
    new, added = old, 0
    for item in items:
        first = item.splitlines()[0][2:].strip()
        if first in new:
            continue
        entry = item.replace("\n", nl)
        lines = new.splitlines(keepends=True)
        at = next((i for i, ln in enumerate(lines) if re.match(r"%s:\s*$" % key, ln.rstrip("\r\n"))), None)
        if at is None:
            new = new.rstrip("\r\n") + nl + key + ":" + nl + entry + nl
        else:
            last = at
            for i in range(at + 1, len(lines)):
                s = lines[i]
                if s[:1] in ("-", " ", "\t"):
                    last = i
                elif s.strip() and not s.startswith("#"):
                    break
            if not lines[last].endswith(("\n", "\r")):
                lines[last] += nl
            lines.insert(last + 1, entry + nl)
            new = "".join(lines)
        added += 1
    verdict = librepo.config_change(old, new)
    if not verdict["ok"]:
        return old, 0, key, "the guard refuses the config change: %s" % verdict["reason"]
    return new, added, key, None


def apply_config(path: str, it: Item) -> dict:
    """Apply one config request to `path` (`config_edit`); `{rid: [why]}` on a refusal."""
    old = libtext.read_text(path)
    new, added, key, refusal = config_edit(old, it)
    if refusal:
        return {it.req.id: [refusal]}
    if added:
        libtext.atomic_write(path, new)
    it.notes.append("config: %d item(s) added under %s" % (added, key) if added else "config: already present")
    return {}


def restore(root: str, base_head: str, created: list[str]) -> None:
    g = Git(root)
    g.run("checkout", base_head, "--", ".")
    for r in created:
        p = os.path.join(root, r)
        if os.path.exists(p) and g.run("ls-files", "--error-unmatch", r).returncode != 0:
            os.remove(p)


def culprits(items: list[Item], errors: list[dict], failed: list[str]) -> tuple[list[tuple[Item, str]], set[str]]:
    """What a failed build is blamed on, most specific first: `[(item, declaration name)]` whose declared name is on a
    compiler error line or message; else the request ids whose rename or stale-spelling rewrite is named there;
    else the declarations whose edits touched a failed object's source or the header it errs in."""
    live = [it for it in items if it.state == "apply"]
    ops = []
    for e in errors:
        hay = e["text"] + " " + e.get("message", "")
        for it in live:
            for d in it.live_decls():
                if re.search(r"\b%s\b" % re.escape(d.name), hay) and d.result.get("prototype"):
                    ops.append((it, d.name))
    if ops:
        return list({(id(it), n): (it, n) for it, n in ops}.values()), set()
    hit = set()
    for e in errors:
        hay = e["text"] + " " + e.get("message", "")
        for it in live:
            if any(re.search(r"\b%s\b" % re.escape(n), hay) for _o, n in it.renames + it.rewrites):
                hit.add(it.req.id)
    if hit:
        return [], hit
    sources = set()
    for f in failed:
        stem = f.replace("\\", "/").split("/src/", 1)[-1][:-2]
        sources |= {"src/%s%s" % (stem, ext) for ext in (".c", ".cpp", ".cp")}
    sources |= {e["file"] for e in errors}
    for it in live:
        for d in it.live_decls():
            touched = (set(d.result.get("sites_removed", [])) | set(d.result.get("includes_added", []))
                       | ({d.header} if d.result.get("prototype") and not d.result.get("already_declared")
                          else set()))
            if touched & sources:
                ops.append((it, d.name))
    return list({(id(it), n): (it, n) for it, n in ops}.values()), set()


def exclude_everywhere(items: list[Item], name: str, why: str) -> None:
    """Stop declaring `name` in every request that would (two requests often name one symbol)."""
    for it in items:
        if it.state == "apply" and any(d.name == name for d in it.decls):
            it.exclude(name, why)


def commit_stage(root: str, subject: str, body: str) -> str | None:
    p = run_tool("tools/git/commitlint.py", "--message", subject, cwd=root, timeout=60)
    if p.returncode != 0:
        raise SystemExit("integrate: commitlint refuses %r: %s" % (subject, (p.stdout + p.stderr).strip()))
    g = Git(root)
    g.run("add", "-A", "src", "include", "config")
    msg = subject + "\n\n" + body.strip() + "\n"
    p = g.run("commit", "-q", "-F", "-", input=msg)
    if p.returncode != 0:
        return None
    return g.head()


def module_of(files: list[str]) -> str:
    counts: dict[str, int] = {}
    for f in files:
        if f.startswith("src/") and "/" in f[4:]:
            m = f[4:].split("/")[0].lower()
            counts[m] = counts.get(m, 0) + 1
    return max(counts, key=lambda k: (counts[k], k)) if counts else "network"


# --- narrowing a failed build ------------------------------------------------------------------------------------

_QUOTED_RE = re.compile(r"'([A-Za-z_~][\w:~]*)")
_CLASH_RE = re.compile(r"redefin|redeclar|already (?:been )?defined|multiply.defined", re.I)
_UNDEFINED_RE = re.compile(r"undefined identifier", re.I)


def object_source(root: str, obj: str) -> str | None:
    """`src/<stem>.<ext>` of a ninja object target `build/RMHE08/src/<stem>.o` (the extension the tree has)."""
    stem = obj.replace("\\", "/").split("/src/", 1)[-1]
    stem = stem[:-2] if stem.endswith(".o") else stem
    for ext in (".cpp", ".c", ".cp", ".cc"):
        if os.path.isfile(os.path.join(root, "src", stem + ext)):
            return "src/%s%s" % (stem, ext)
    return None


def error_command(tokens: list[str], scratch_dir: str) -> list[str] | None:
    """The build's compile command for one object, with every error reported (`-maxerrors 0`: the project's
    `-maxerrors 1` stops MWCC at the first error of an object) and the object written to `scratch_dir` - the build's
    own object and flags are untouched. None when the command has no `-o`."""
    toks = list(tokens)
    if "&&" in toks:
        toks = toks[:toks.index("&&")]
    if "-o" not in toks:
        return None
    if "-maxerrors" in toks and toks.index("-maxerrors") + 1 < len(toks):
        toks[toks.index("-maxerrors") + 1] = "0"
    else:
        toks.insert(toks.index("-o"), "0")
        toks.insert(toks.index("0"), "-maxerrors")
    toks[toks.index("-o") + 1] = scratch_dir
    return toks


def object_errors(root: str, failed: list[str], runner=None) -> tuple[list[dict], list[str]]:
    """Every compiler error of each failed object - one direct compile per object (`error_command`), so a round
    narrows every wrong declaration of an object at once instead of one per round. -> (errors, notes); an error
    carries the `object` it came from."""
    runner = runner or proc.run
    errors, notes = [], []
    for obj in failed:
        stem = obj.replace("\\", "/").split("/src/", 1)[-1]
        stem = stem[:-2] if stem.endswith(".o") else stem
        try:
            tokens = libunits.ninja_command(root, stem)
        except SystemExit as exc:
            notes.append("%s: no compile command (%s)" % (obj, str(exc).splitlines()[0]))
            continue
        tmp = tempfile.mkdtemp(prefix="integrate-errors-")
        try:
            cmd = error_command(tokens, tmp)
            if cmd is None:
                notes.append("%s: the compile command has no -o" % obj)
                continue
            p = runner(cmd, cwd=root, timeout=600)
            found = parse_build_errors((p.stdout or "") + (p.stderr or ""))
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        for e in found:
            e["object"] = obj
        errors += found
    return errors, notes


def _ops_by_name(items: list[Item]) -> dict[str, list[tuple[Item, DeclOp]]]:
    out: dict[str, list[tuple[Item, DeclOp]]] = {}
    for it in items:
        if it.state == "apply":
            for d in it.live_decls():
                out.setdefault(d.name, []).append((it, d))
    return out


def revert(it: Item, why: str) -> None:
    it.state, it.why = "reverted", why


def to_leaf(items: list[Item], name: str) -> list[str]:
    """Move every live declaration of `name` to its leaf header; -> the leaf paths (empty when already there)."""
    moved = []
    for _it, d in _ops_by_name(items).get(name, []):
        if d.leaf:
            continue
        lp = leaf_header(d)
        d.full_header, d.header, d.leaf = d.header, lp, True
        moved.append(lp)
    return sorted(set(moved))


def narrow(items: list[Item], errors: list[dict], failed: list[str], root: str | None = None) -> dict:
    """Blame every compiler error of a failed build and pick each culprit's remedy, all in one round:

    * a redefinition in an owner header this batch declared into or included (a type the full header redefines
      for the consumer) -> the declarations move to their **leaf** header (`include/<module>/<symbol>.h`);
    * `undefined identifier 'X'` for a rename's target whose declaration was skipped -> the declaration is
      **forced** into the owner header (a caller needs it); a second time, the rename is **reverted**;
    * any other error naming a declared symbol in its message -> that declaration is **excluded** everywhere
      (the owner's prototype and the call site disagree: judgement with the compiler's message);
    * an error naming only a renamed symbol -> the rename is **reverted**;
    * the rest go through `culprits` (the line text, then the edits that touched the failed source).

    -> `{excluded, leaf, forced, reverted, unattributed}`; nothing in the first four means no progress."""
    out = {"excluded": [], "leaf": [], "forced": [], "reverted": [], "unattributed": []}
    rest = []
    for e in errors:
        ops = _ops_by_name(items)
        renamed = {n: it for it in items if it.state == "apply" for _o, n in it.renames + it.rewrites}
        msg = e.get("message", "")
        efile = e.get("file", "").replace("\\", "/")
        where = "%s:%d %s" % (efile, e.get("line", 0), (msg or e.get("text", "").strip())[:120])
        quoted = [q.split("::")[-1] for q in _QUOTED_RE.findall(msg)]
        named = [q for q in dict.fromkeys(quoted) if q in ops or q in renamed]
        if _CLASH_RE.search(msg) and not [q for q in named if q in ops]:
            src = object_source(root, e.get("object", "")) if root else None
            hit = sorted({d.name for lst in ops.values() for _it, d in lst
                          if d.header == efile and not d.leaf and d.result.get("prototype")
                          and (not d.result.get("already_declared") or (src and src in d.result.get("includes_added", [])))})
            if hit:
                for name in hit:
                    if to_leaf(items, name):
                        out["leaf"].append(name)
                continue
        if not named:
            rest.append(e)
            continue
        for q in named:
            if q in ops and _UNDEFINED_RE.search(msg):
                undeclared = [(it, d) for it, d in ops[q] if not d.result.get("prototype") or d.result.get("skipped")]
                if undeclared:
                    for it, d in undeclared:
                        if d.force:
                            revert(it, "the build still finds no declaration of %s: %s" % (q, where))
                            out["reverted"].append(it.req.id)
                        else:
                            d.force = True
                            out["forced"].append(q)
                    continue
            if q in ops:
                exclude_everywhere(items, q, "the build failed on it: " + where)
                out["excluded"].append(q)
            elif q in renamed and renamed[q].state == "apply":
                revert(renamed[q], "the build failed on its rename: " + where)
                out["reverted"].append(renamed[q].req.id)
    if rest:
        ops, hit = culprits(items, rest, failed)
        for it, name in ops:
            why = "; ".join("%s:%d %s" % (e["file"], e["line"], e.get("message") or e["text"].strip()[:80])
                            for e in rest if name in e["text"] + e.get("message", ""))[:300]
            exclude_everywhere(items, name, "the build failed on it: " + (why or "an edit it made is in a failed object"))
            out["excluded"].append(name)
        for it in items:
            if it.req.id in hit and it.state == "apply":
                revert(it, "the build failed on its rename")
                out["reverted"].append(it.req.id)
        if not ops and not hit:
            out["unattributed"] = ["%s:%d %s" % (e["file"], e["line"], e.get("message", "")) for e in rest[:6]]
    for k in ("excluded", "leaf", "forced", "reverted"):
        out[k] = list(dict.fromkeys(out[k]))
    return out


def progressed(verdict: dict) -> bool:
    return any(verdict.get(k) for k in ("excluded", "leaf", "forced", "reverted"))


# --- already applied: the tree (and the sidecar) say a request needs nothing ------------------------------------

def mark_applied(root: str, items: list[Item], scope: set[str]) -> int:
    """Turn every `apply` item the tree already satisfies into `done` ("already applied"), so a rerun never
    re-proposes it: no rename left to do, no stale spelling left in the code, no STOPGAP block of its id, and each
    declaration present in its owner header or its leaf header with no foreign declaration left in scope (a rename's
    declaration may instead be unneeded: every caller sees one); a config request whose items config.yml already
    carries. -> the number marked."""
    files = Files(root)
    stopgap_ids = set()
    for r in files.all():
        text = files.get(r)
        if "STOPGAP-BEGIN" in text:
            stopgap_ids |= {rid for rid, _s, _e in R.stopgap_blocks(text)}
    spelled: dict[str, bool] = {}

    def in_code(name: str) -> bool:
        if name not in spelled:
            spelled[name] = any(name in files.get(r) and _code_refers(files.get(r), name) for r in files.all())
        return spelled[name]

    n = 0
    for it in items:
        if it.state != "apply" or it.renames or it.req.id in stopgap_ids:
            continue
        if any(in_code(old) for old, _new in it.rewrites):
            continue
        why = []
        if it.config:
            cfg = os.path.join(root, "config", "RMHE08", "config.yml")
            old = libtext.read_text(cfg) if os.path.exists(cfg) else ""
            _new, added, key, refusal = config_edit(old, it)
            if refusal or added:
                continue
            why.append("config.yml carries every %s item" % key)
        ok = True
        for d in it.live_decls():
            adopt_leaf(files, d)
            if any(x[4] for x in decl_sites(files, d, scope)):
                ok = False
                break
            if files.exists(d.header) and d.name in files.declared(d.header):
                why.append("%s in %s" % (d.name, d.header))
            elif d.implicit and not needs_declaration(files, d):
                why.append("%s: every caller sees a declaration" % d.name)
            else:
                ok = False
                break
        if ok:
            it.state, it.why = "done", "already applied (the tree): " + ("; ".join(why) or "nothing left to do")
            n += 1
    return n


# --- the run ---------------------------------------------------------------------------------------------------------

def scope_files(root: str, lane_scope: set[str]) -> set[str]:
    """The files a removal may edit: the lane's units plus every source carrying a STOPGAP marker."""
    files = Files(root)
    scope = set(lane_scope)
    for r in files.all():
        if r.startswith("src/") and "STOPGAP" in files.get(r):
            scope.add(r)
    return scope


def file_snapshot(root: str, paths: list[str]) -> dict[str, bytes]:
    out = {}
    for r in paths:
        p = os.path.join(root, r)
        if os.path.isfile(p):
            with open(p, "rb") as fh:
                out[r] = fh.read()
    return out


def write_snapshot(root: str, snap: dict[str, bytes]) -> None:
    for r, data in snap.items():
        libtext.atomic_write(os.path.join(root, r), data)


def head_matches(root: str, snap: dict[str, bytes]) -> list[str]:
    """The snapshot paths whose committed (HEAD) bytes differ from the green tree's - empty when the commits are the
    tree that built."""
    g = Git(root)
    return [r for r, data in sorted(snap.items()) if (g.show("HEAD", r) or b"").replace(b"\r\n", b"\n")
            != data.replace(b"\r\n", b"\n")]


def write_config_patch(root: str, base: str, branch: str) -> str | None:
    """The config.yml change as a patch file under `build/tmp/integrate/` (never a unit commit) -> its path."""
    rel_cfg = librepo.CONFIG_PATH
    p = Git(root).run("diff", base, "--", rel_cfg)
    if p.returncode != 0 or not (p.stdout or "").strip():
        return None
    path = os.path.join(str(librepo.scratch("integrate", root)), "%s-config.patch" % re.sub(r"[^\w.-]+", "-", branch))
    libtext.atomic_write(path, p.stdout if p.stdout.endswith("\n") else p.stdout + "\n")
    return path


def refuse_base(root: str, out: str, log: list[str]) -> dict:
    """The base build's verdict when it failed: the failed objects and their first errors (all of them per object)."""
    failed = failed_objects(out)
    errs, _notes = object_errors(root, failed)
    return {"ok": False, "failed": failed, "errors": (errs or parse_build_errors(out))[:12]}


def run(args, root: str | None = None) -> int:
    """The batch: plan, base build, apply/build/narrow rounds, then commit the green tree (exit 0) - or refuse with
    exit 1 (a dirty tree, a base that does not compile, a build that never went green). `root`: the tree (default:
    the invocation's)."""
    t_start = time.time()
    root = root or librepo.repo_root()
    main = librepo.main_checkout(root)
    outbox_dir = args.outbox or os.path.join(main, ".pi", "outbox")
    paths = list(args.requests or [])
    for slug in args.lane or []:
        paths.append(os.path.join(outbox_dir, slug + R.REQUESTS_SUFFIX))
    if not paths:
        paths = R.request_files(outbox_dir)
    requests, status, lane_scope, lane_names = [], {}, set(), set()
    for p in paths:
        reqs = R.load_file(p)
        if args.only:
            reqs = [q for q in reqs if q.id in args.only]
        requests += reqs
        st = R.load_status(p)
        status.update(st)
        for u in lane_units_of(os.path.dirname(p), R.slug_of(p)) + list(args.lane_units or []):
            for ext in (".cpp", ".c", ".cp"):
                if os.path.exists(os.path.join(root, "src", R.unit_stem(u) + ext)):
                    lane_scope.add("src/%s%s" % (R.unit_stem(u), ext))
            lane_scope.add("include/%s.h" % R.unit_stem(u))
            lane_names.add(R.unit_stem(u))
    decisions = R.load_decisions(args.names)
    g = Git(root)
    dirty = [x for x in g.status_porcelain("no") if x]
    if dirty and not args.dry_run:
        print("REFUSED integrate | the tree is dirty (%d path(s)); integrate on a clean tree" % len(dirty))
        return 1
    log: list[str] = []
    live = set() if args.no_claims else live_units(main)
    ownership = Ownership.load(root, auto=True)
    items = plan(requests, ownership, decisions, live, lane_names, status)
    summary = {"tree": root, "requests": len(requests), "files": [os.path.basename(p) for p in paths],
               "classes": {c: sum(1 for i in items if i.cls == c) for c in R.CLASSES}}
    summary["already_applied"] = mark_applied(root, items, scope_files(root, lane_scope))
    if args.dry_run:
        return emit(args, items, summary, log, t_start)
    prev_branch = g.current_branch()
    branch = make_branch(root, args.base, args.branch)
    base_head = g.head()
    summary.update({"branch": branch, "base": base_head})
    build_time = 0.0
    if not args.no_build:
        # (b) the base must compile before anything is applied: a base broken by a merge of main is the operator's
        # to fix, and narrowing against it would blame the batch for the base's errors
        ok, out, dt = run_build(root, log)
        build_time += dt
        if not ok:
            summary["base_build"] = refuse_base(root, out, log)
            if branch != prev_branch and prev_branch:
                g.run("switch", prev_branch)
                g.run("branch", "-D", branch)
            summary["final"] = ("REFUSED: the base %s does not compile (%d object(s) failed) - nothing was applied; fix "
                                "the base first (a merge of main is the usual cause)"
                                % (base_head[:10], len(summary["base_build"]["failed"])))
            emit(args, items, summary, log, t_start)
            return 1
        summary["base_build"] = {"ok": True, "seconds": round(dt)}
    report_path = os.path.join(root, BUILD, "report.json")
    before_report = None
    if os.path.exists(report_path):
        before_report = os.path.join(tempfile.gettempdir(), "integrate-report-before-%d.json" % os.getpid())
        with open(report_path, "rb") as src, open(before_report, "wb") as dst:
            dst.write(src.read())
    before_targets = target_snapshot(root)
    before_objects = None if args.no_build else snapshot_objects(root)
    created: list[str] = []
    rounds = []
    untracked_before = {p for _c, p in g.status_porcelain("all") if _c == "??"}
    built_ok: bool | None = False
    try:
        for attempt in range(1 + args.retries):
            if attempt:
                restore(root, base_head, created)        # each round starts from the base; the last one is kept
            res = apply_plan(root, items, lane_scope, not args.no_comments, not args.no_mangle_check)
            created = [f for f in res["files"] if g.run("ls-files", "--error-unmatch", f).returncode != 0]
            fail_now = res["failures"]
            if fail_now:
                for it in items:
                    for d in it.live_decls():
                        if d.result.get("refused"):
                            if d.implicit and d.result.get("needed_by") and it.state == "apply":
                                revert(it, "its callers need a declaration the owner header cannot take: %s"
                                       % "; ".join(d.result["refused"]))
                            else:
                                exclude_everywhere(items, d.name, "; ".join(d.result["refused"]))
                rounds.append({"round": attempt + 1, "apply_refused": sorted(fail_now)})
                continue
            if args.no_build:
                rounds.append({"round": attempt + 1, "build": "skipped"})
                built_ok = None
                break
            ok, out, dt = run_build(root, log)
            build_time += dt
            if ok:
                rounds.append({"round": attempt + 1, "build_ok": True, "seconds": round(dt)})
                built_ok = True
                break
            failed = failed_objects(out)
            errs, notes = object_errors(root, failed)
            errs = errs or parse_build_errors(out)
            verdict = narrow(items, errs, failed, root)
            rounds.append({"round": attempt + 1, "build_ok": False, "seconds": round(dt), "failed": failed,
                           "errors": errs[:12], "narrowed": {k: v for k, v in verdict.items() if v},
                           **({"notes": notes} if notes else {})})
            if not progressed(verdict):
                summary["build_failed_unattributed"] = failed
                break
    except BaseException:
        # a crash mid-apply leaves nothing behind: the tracked files go back to the base, new files are removed
        new_files = [p for _c, p in g.status_porcelain("all") if _c == "??" and p not in untracked_before
                     and p.startswith(("src/", "include/"))]
        restore(root, base_head, new_files)
        raise
    summary["rounds"] = rounds
    summary["build_seconds"] = round(build_time)
    changed = sorted(set(Git(root).diff_names(base_head)) | set(created))
    if built_ok is False:
        # (a) a tree that did not build is never committed: it is left for the operator, and the run fails
        summary["left_in_tree"] = changed
        summary["final"] = ("REFUSED: the build never went green in %d round(s) - NOTHING was committed; the working "
                            "tree holds the last attempt (%d file(s)) for the operator: `git checkout -- .` and "
                            "remove the new files to discard it" % (len(rounds), len(changed)))
        emit(args, items, summary, log, t_start)
        return 1
    after_targets = target_snapshot(root)
    d = drift(before_targets, after_targets)
    units = sorted(set(changed_units(root, base_head, created)) | set(d["content"]) | set(d["names_only"]))
    summary["drift"] = d
    summary["units"] = units
    # a unit header that still describes a removed stopgap is prose the integrator cannot rewrite: name it
    mentions = []
    for f in changed:
        p = os.path.join(root, f)
        if f.startswith("src/") and os.path.isfile(p):
            for n, line in enumerate(libtext.read_text(p).splitlines(), 1):
                if "STOPGAP" in line and "STOPGAP-" not in line:
                    mentions.append("%s:%d" % (f, n))
    summary["stopgap_mentions"] = mentions
    summary["report"] = report_moved(before_report, report_path)
    if not args.no_build:
        summary["objects"] = objects_same(before_objects, root)
    if not args.no_gate and not args.no_build:
        summary["gate"] = gate_prerun(root, base_head, units)
    # (h) the relocation-analysis change is a separate patch, never part of the unit commits
    patch = write_config_patch(root, base_head, branch)
    if patch:
        g.run("checkout", base_head, "--", librepo.CONFIG_PATH)
        summary["config_patch"] = patch
        summary["config_apply"] = ("git apply %s  (then `ninja` + the score check, and commit it alone as "
                                   "`config/analysis: ...`)" % patch.replace("\\", "/"))
    changed = [f for f in changed if f != librepo.CONFIG_PATH]
    if built_ok is None:
        summary["final"] = "--no-build: the result is left in the working tree (a tree that was not built is never committed)"
    elif args.commit:
        snap = file_snapshot(root, changed)
        restore(root, base_head, created)
        apply_plan(root, items, lane_scope, not args.no_comments, False, stage="renames")
        ren_pairs = list(dict.fromkeys(p for it in items if it.state == "apply" for p in it.renames))
        commits = []
        if ren_pairs:
            ok1, _out1, dt1 = run_build(root, log)
            build_time += dt1
            if ok1:
                commits.append(commit_stage(root, "config/symbols: name the %d symbols the integrated requests ask for"
                                            % len(ren_pairs), "\n".join("%s %s" % p for p in ren_pairs)))
            else:
                summary["renames_commit"] = "folded into the second commit: the renames alone do not compile"
        write_snapshot(root, snap)
        mod = module_of(changed)
        if Git(root).diff_names("HEAD") or [f for f in created if os.path.exists(os.path.join(root, f))]:
            subject = ("game/%s: declare the integrated callees in their owners' headers" % mod if commits or not ren_pairs
                       else "game/%s: name the %d integrated symbols and declare the callees in their owners' headers"
                       % (mod, len(ren_pairs)))
            commits.append(commit_stage(root, subject, "\n".join(sorted(changed))))
        summary["commits"] = [c for c in commits if c]
        summary["commits_verified"] = not head_matches(root, snap)
        summary["build_seconds"] = round(build_time)
    summary["land"] = land_command(branch, units, changed)
    if args.write_status:
        for p in paths:
            slug = R.slug_of(p)
            updates = {it.req.id: {"status": _status(it), "note": it.why or it.reason}
                       for it in items if it.req.id.startswith(slug + "#")
                       and (it.state != "done" or it.why.startswith("already applied"))}
            if updates:
                R.write_status(p, updates)
    return emit(args, items, summary, log, t_start)


def _status(it: Item) -> str:
    if it.state == "done":
        return "applied"
    return {"apply": "applied", "judgement": "judgement", "deferred": "deferred", "reverted": "judgement"}.get(
        it.state, "open")


def emit(args, items: list[Item], summary: dict, log: list[str], t_start: float) -> int:
    summary["seconds"] = round(time.time() - t_start)
    groups = {"applied": [i for i in items if i.state == "apply"],
              "judgement": [i for i in items if i.state in ("judgement", "reverted")],
              "deferred": [i for i in items if i.state == "deferred"],
              "already applied": [i for i in items if i.state == "done"]}
    if args.json:
        print(json.dumps({"summary": summary, "items": [i.to_dict() for i in items], "log": log}, indent=1))
        return 0
    print("integrate: %d request(s) from %s - classes %s" % (
        summary["requests"], ", ".join(summary["files"]),
        ", ".join("%s %d" % (k, v) for k, v in summary["classes"].items())))
    for label in ("applied", "judgement", "deferred", "already applied"):
        print("\n## %s (%d)%s" % (label, len(groups[label]), " - dry run: nothing was written" if args.dry_run and
                                    label == "applied" else ""))
        for it in groups[label]:
            what = ", ".join("%s->%s" % p for p in it.renames)
            decl = ", ".join("%s in %s" % (d.name, d.header) for d in it.live_decls())
            if label == "already applied":
                print("- %s [%s] %s" % (it.req.id, it.req.kind, it.why))
                continue
            print("- %s [%s] %s%s%s" % (it.req.id, it.req.kind, what, (" | decl " + decl) if decl else "",
                                        (" | " + it.why) if it.why and label != "applied" else
                                        (" | " + it.reason) if label == "applied" and not what and not decl else ""))
            if label == "applied":
                for name, why in it.excluded.items():
                    print("    judgement: %s not declared - %s" % (name, why[:200]))
                for d in it.live_decls():
                    for why in d.result.get("not_moved", [])[:1]:
                        print("    kept in the lane: %s" % why[:200])
    for key in ("base_build", "rounds", "final", "build_seconds", "drift", "report", "objects", "gate",
                "stopgap_mentions", "units", "config_patch", "config_apply", "renames_commit", "commits",
                "commits_verified", "left_in_tree", "land"):
        if key in summary:
            print("\n%s: %s" % (key, json.dumps(summary[key]) if not isinstance(summary[key], str) else summary[key]))
    print("\n%d s total" % summary["seconds"])
    return 0


def main(argv: list[str] | None = None, root: str | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--requests", nargs="*", help="request files (default: every *-requests.json in the outbox)")
    ap.add_argument("--lane", nargs="*", help="lane slugs whose <slug>-requests.json to read")
    ap.add_argument("--outbox", default=None, help="the outbox directory (default MAIN/.pi/outbox)")
    ap.add_argument("--only", nargs="*", help="request ids to consider (the rest are ignored)")
    ap.add_argument("--names", default=None, help="decisions: `old new` lines naming GUESS / unnamed targets")
    ap.add_argument("--lane-units", nargs="*", help="units whose files a removal may edit (default: the lane outbox)")
    ap.add_argument("--base", default=None, help="the branch to integrate onto (default: the current HEAD)")
    ap.add_argument("--branch", default=None, help="the integrate branch (default integrate/<date>)")
    ap.add_argument("--dry-run", action="store_true", help="classify, resolve and plan; write nothing")
    ap.add_argument("--no-build", action="store_true", help="skip the builds (and so never commit)")
    ap.add_argument("--no-gate", action="store_true", help="skip the stylelint/undefrefs/vtableaudit pre-run")
    ap.add_argument("--no-claims", action="store_true", help="do not ask claims.py for live lanes")
    ap.add_argument("--no-comments", action="store_true", help="the sweep leaves comment mentions alone")
    ap.add_argument("--no-mangle-check", action="store_true", help="skip mangle.py on C++ declarations")
    ap.add_argument("--retries", type=int, default=12,
                    help="narrowing rounds after a refused declaration or a failed build (default 12)")
    ap.add_argument("--no-commit", action="store_true",
                    help="leave the result in the working tree (the default commits it as two commits: renames, the rest)")
    ap.add_argument("--commit", action="store_true", help=argparse.SUPPRESS)
    ap.add_argument("--write-status", action="store_true", help="write each request's verdict to its sidecar")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    args.commit = not args.no_commit and not args.dry_run and not args.no_build
    return run(args, root)


if __name__ == "__main__":
    raise SystemExit(main())
