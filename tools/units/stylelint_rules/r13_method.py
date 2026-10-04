"""Rule 13: a method is a member - `<Type>_<name>(<Type>* self, ...)` and its static form, unless marked `free:`.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import os
import re

from tools.lib import cscan
from tools.lib import names as libnames
from tools.units.stylelint_rules.common import (
    HEADERS, HEADER_SUFFIXES, SRC, SUFFIXES, Source, _finding, _mask_preproc, _split_parameters, _untyped_marker,
    function_declarations, read_text, rel_of, type_defs,
)
from tools.units.stylelint_rules.r09_mangled import RULE9_MANGLED_RE


# --------------------------------------------------------------------------------------------------
# rule 13: a method is a member - `<Type>_<name>(<Type>* self, ...)` is `<Type>::<name>` spelled the C way
# --------------------------------------------------------------------------------------------------
# A free function named `<Type>_<name>` whose first parameter is `<Type>*`, `const <Type>*` or `<Type>&`, for a
# class or struct the project defines, is a member function written the C way. MWCC then emits the *mangled*
# name (`send__16NetworkSingleTcpFPCUcl`), so the map row has to carry that mangling, the class lists the
# method and call sites use `obj->name(...)`. The exemption is a per-declaration `/* free: <reason> */` marker
# (retail C linkage evidenced, or an SDK C struct); a file can never exempt itself.
#
# Scope: `.cpp`-family sources and the headers a `.cpp` reaches (a C file has no members). The type must have
# a definition the tool can see. The definition registry is built once per tree (`Rule13Context`) and applies
# to both sides of a `--diff`, so a class added by the batch cannot turn an old free function into an "added"
# finding - only text the batch adds can.
RULE13_MARKER_RE = re.compile(r"\bfree\s*:\s*([^\n]*)")
# The reason must say which case it is: evidenced C linkage, or a plain C struct from an SDK API.
RULE13_REASON_RE = re.compile(
    r"\bC[- ]linkage\b|\bunmangled\b|\brelocation\b|\bdump\b|\bevidence|\bSDK\b|\bC struct\b|\bretail\b", re.I)
RULE13_FIRST_PARAM_RE = re.compile(
    r"^\s*(?P<pre>(?:(?:const|volatile|register|struct|class)\s+)*)(?P<type>[A-Za-z_]\w*)\s*"
    r"(?P<mid>(?:const\s*|volatile\s*)*)(?P<decl>[*&])\s*(?P<post>(?:(?:const|volatile)\s+)*)"
    r"(?:[A-Za-z_]\w*)?\s*$")
# A `<Type>_ctor(Other* out)` / `_dtor` / `_construct` / `_destruct` whose first parameter points at a *different*
# type is a C-style SDK helper that initialises `Other` (`VEC3_ctor(MHTRI_PAD_VEC3*)`, `MTX34_ctor(MHTRI_MTX34*)`),
# not a static member of `Type`: measured 2026-09-29, these two are the only such shapes in the tree.
RULE13_CTOR_HELPER_RE = re.compile(r"^(?:ctor|dtor|construct|destruct)$")
RULE13_CPP_SUFFIXES = (".cpp", ".cp", ".cc")


class Rule13Context:
    """The tree-wide facts rule 13 needs: the class/struct names the project defines under a C++ reach.

    `types` holds every name defined in a `.cpp`-family source or in a header a `.cpp` includes (directly or
    transitively); `headers` is the set of those headers' repo-relative paths - a header only a `.c` file
    includes has no members to move a function into, so it is out of scope.
    """

    def __init__(self, types: set, headers: set):
        self.types = types
        self.headers = headers


_RULE13_CTX: "Rule13Context | None" = None
_RULE13_CACHE: dict = {}


def _resolve_include(root: str, includer_rel: str, inc: str) -> str | None:
    """The repo-relative path an `#include` names, or None (an SDK/system header this tree lacks)."""
    hit = cscan.resolve_include(inc, [os.path.join(root, b) for b in (HEADERS, os.path.dirname(includer_rel), SRC)])
    return os.path.relpath(hit, root).replace(os.sep, "/") if hit else None


def build_rule13_context(root: str) -> Rule13Context:
    """Scan `src/` and `include/` once: the header set a `.cpp` reaches and the types defined there."""
    files = {}
    sig = []
    for top in (SRC, HEADERS):
        for dirpath, dirnames, filenames in os.walk(os.path.join(root, top)):
            dirnames[:] = sorted(d for d in dirnames if d != "__pycache__")
            for name in sorted(filenames):
                if name.endswith(SUFFIXES):
                    path = os.path.join(dirpath, name)
                    files[rel_of(root, path)] = path
                    sig.append((path, os.stat(path).st_mtime_ns))
    key = (root, tuple(sig))
    if key in _RULE13_CACHE:
        return _RULE13_CACHE[key]
    texts: dict = {}

    def text_of(rel: str) -> str:
        if rel not in texts:
            texts[rel] = read_text(files[rel])
        return texts[rel]

    reach: set = set()
    todo = [r for r in files if r.endswith(RULE13_CPP_SUFFIXES)]
    seen = set(todo)
    while todo:
        rel = todo.pop()
        for name in cscan.includes(text_of(rel)):
            inc = _resolve_include(root, rel, name)
            if inc is not None and inc in files and inc not in seen:
                seen.add(inc)
                reach.add(inc)
                todo.append(inc)
    types: set = set()
    for rel in seen:
        types.update(n for n, _line in type_defs(Source(files[rel], rel, text_of(rel))))
    _RULE13_CACHE.clear()
    _RULE13_CACHE[key] = ctx = Rule13Context(types, reach)
    return ctx


def set_rule13_context(root: str | None) -> None:
    """Make `lint_source` read rule 13's type registry from `root` (None: per-file types only)."""
    global _RULE13_CTX
    _RULE13_CTX = build_rule13_context(root) if root else None


def _rule13_in_scope(rel: str, ctx: "Rule13Context | None") -> bool:
    rel = rel.replace("\\", "/")
    if rel.endswith(RULE13_CPP_SUFFIXES):
        return True
    if rel.endswith(HEADER_SUFFIXES):
        return ctx is None or rel in ctx.headers
    return False


def _rule13_self_type(first_chunk: str) -> tuple[str, bool] | None:
    """`(Type, const_self)` when the first parameter is `Type*`, `const Type*` or `Type&`, else None."""
    m = RULE13_FIRST_PARAM_RE.match(first_chunk)
    if not m:
        return None
    return m.group("type"), bool(re.search(r"\bconst\b", m.group("pre")))


def _rule13_prefix_type(name: str, types: set) -> str | None:
    """The longest defined type `T` with `name == T + "_" + rest` (rest non-empty), or None."""
    best = None
    for i, ch in enumerate(name):
        if ch == "_" and 0 < i < len(name) - 1 and name[:i] in types:
            best = name[:i]
    return best


def _rule13_scan(src: Source) -> tuple[list[dict], set]:
    """`(findings, static_like_names)` for one file; the static form's findings carry `static=True`."""
    ctx = _RULE13_CTX
    if not _rule13_in_scope(src.rel, ctx):
        return [], set()
    types = ctx.types if ctx is not None else {n for n, _l in type_defs(src)}
    if not types:
        return [], set()
    code = _mask_preproc(src.code)
    findings: list[dict] = []
    static_like: set = set()
    for d in function_declarations(src):
        name = d["name"]
        if "::" in name or RULE9_MANGLED_RE.match(name):
            continue
        chunks = _split_parameters(code, d["params_pos"], d["params_pos"] + len(d["params"]))
        first = chunks[0][0] if chunks else ""
        owner = _rule13_prefix_type(name, types)
        if owner is None:
            continue
        selfy = _rule13_self_type(first)
        is_static = selfy is None or selfy[0] != owner
        marker = _untyped_marker(src, d["start_line"], d["end_line"], RULE13_MARKER_RE)
        marked = marker is not None and marker.strip() and RULE13_REASON_RE.search(marker)
        if is_static:
            # no `Type* self`: `getInstance(void)` / `setNotifyValue(u32)` - a static member spelled the C way
            method = name[len(owner) + 1:]
            if RULE13_CTOR_HELPER_RE.match(method) and re.search(r"[*&]", first) and (
                    selfy is None or selfy[0] != owner):
                continue                                     # a C-style helper that initialises another type
            static_like.add(name)
            if marked:
                continue
            params = [c.strip() for c, _o in chunks] if first.strip() not in ("", "void") else []
            mangled = None
            try:
                mangled = libnames.estimate_static_mangling(owner, method, params)
            except Exception:                                # the estimate is a courtesy, never a failure
                mangled = None
            sig = "static %s %s::%s(%s)" % (d["ret"].strip() or "auto", owner, method, ", ".join(params))
            detail = ("`%s` is the static member `%s::%s` spelled the C way (no `%s` self) - declare `%s;` in the "
                      "class, define `%s::%s`, call it `%s::%s(...)`, and rename the map row to %s "
                      "(`python tools/units/methodize.py %s`); or mark a genuine C function "
                      "`/* free: <retail C linkage evidenced|SDK C struct> */`"
                      % (name, owner, method, owner, sig, owner, method, owner, method,
                         "`%s` (estimated)" % mangled if mangled else "the compiler's mangling", owner))
            findings.append(dict(_finding(src, 13, d["line"], detail, token=name), symbol=name,
                                 owner=owner, method=method, params=params, const_self=False, static=True,
                                 ret=d["ret"].strip(), mangled=mangled, has_body=bool(d.get("body"))))
            continue
        if marked:
            continue
        method = name[len(owner) + 1:]
        rest = [c.strip() for c, _o in chunks[1:]]
        mangled = None
        try:
            mangled = libnames.estimate_member_mangling(owner, method, rest, const_self=selfy[1])
        except Exception:                                    # the estimate is a courtesy, never a failure
            mangled = None
        sig = "%s %s::%s(%s)%s" % (d["ret"].strip() or "auto", owner, method, ", ".join(rest),
                                   " const" if selfy[1] else "")
        detail = ("`%s` is `%s::%s` spelled the C way (first parameter is the `%s` self) - declare `%s` in the "
                  "class, define `%s::%s`, call it `obj->%s(...)`, and rename the map row to %s "
                  "(`python tools/units/methodize.py %s`); or mark a genuine C function "
                  "`/* free: <retail C linkage evidenced|SDK C struct> */`"
                  % (name, owner, method, owner, sig, owner, method, method,
                     "`%s` (estimated)" % mangled if mangled else "the compiler's mangling", owner))
        findings.append(dict(_finding(src, 13, d["line"], detail, token=name), symbol=name,
                             owner=owner, method=method, params=rest, const_self=selfy[1], static=False,
                             ret=d["ret"].strip(), mangled=mangled, has_body=bool(d.get("body"))))
    return findings, static_like


def rule13_findings(src: Source) -> list[dict]:
    """Rule 13 for one file: every `<Type>_<name>(<Type>* self, ...)` free function without a `free:` marker."""
    return _rule13_scan(src)[0]


def rule13_static_like(src: Source) -> set:
    """The `<Type>_<name>` names with no `self` in the file - the static-member form of rule 13."""
    return _rule13_scan(src)[1]
