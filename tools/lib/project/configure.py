"""The `configure.py` registry: libs, `Object(...)` rows and cflags groups, read without running the file.
Spec: docs/tools/spec/lib-project.md. CLI: none (library)."""
from __future__ import annotations

import ast
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from tools.lib.repo import VERSION  # `config.version` and the `args.version` default evaluate to it
#: The `args.*` values `configure.py` sees with no command-line flags (its argparse defaults).
DEFAULT_ARGS = {"version": VERSION, "debug": False, "warn": None, "map": False, "non_matching": False,
                "verbose": False, "progress": True, "mode": "configure", "no_asm": False}
#: The three flag spellings; any other first argument is kept as its source text.
FLAGS = ("Matching", "NonMatching", "Equivalent")


class _Unknown(Exception):
    """An expression the evaluator does not model; the statement holding it is skipped."""


@dataclass(frozen=True)
class Unresolved:
    """A lib field or `Object` option the evaluator could not resolve (an undefined group, say): its source
    text is kept, so one unknown value never drops the lib or the object that spells it."""
    text: str


@dataclass(frozen=True)
class Flag:
    """An `Object(...)`'s first argument: its spelling and whether it links (Matching, or a true MatchingFor)."""
    spelling: str
    linked: bool


@dataclass(frozen=True)
class ObjectRow:
    """One registered `Object(flag, path, **options)` with what its lib gives it."""
    path: str
    flag: str                         # "Matching" / "NonMatching" / "Equivalent", or the expression's text
    linked: bool                      # what the build links: Matching, or a MatchingFor naming this version
    lib: str | None
    mw_version: str | None            # the object's own `mw_version=`, else the lib's
    lib_cflags: str | None            # the lib's cflags group name, None when the lib spells an expression
    cflags_name: str | None           # the effective group name: the object's `cflags=<Name>`, else lib_cflags
    cflags: tuple[str, ...]           # the effective flags, resolved
    options: tuple[tuple[str, str], ...]   # every keyword argument as (name, source text)
    line: int                         # 1-based line of the `Object(` call

    @property
    def matching(self) -> bool:
        return self.flag == "Matching"

    def option(self, name: str) -> str | None:
        return dict(self.options).get(name)


@dataclass(frozen=True)
class Lib:
    """One `config.libs` entry."""
    name: str
    mw_version: str | None
    cflags_name: str | None
    cflags: tuple[str, ...]
    progress_category: str | None
    objects: tuple[ObjectRow, ...]
    line: int


@dataclass(frozen=True)
class ObjectCall:
    """An `Object(flag, "path"...)` spelled in a text (a whole file or a fragment); `closed` when the call
    ends on that line with no keyword arguments (the `Object(kind, "unit")` one-line shape)."""
    flag: str
    path: str
    line: int
    closed: bool

    def normalised(self) -> str:
        return 'Object(%s, "%s")' % (self.flag, self.path)


_CALL_RE = re.compile(r"(?<![\w.])Object\(\s*([A-Za-z_]\w*)\s*,\s*\x00(\d+)\x00(\s*\))?")
_QUOTE_RE = re.compile(r"(?i)(?<![\w])([rbuf]{0,2})('''|\"\"\"|'|\")")


def _skeleton(text: str) -> tuple[str, list[str]]:
    """`text` with every comment dropped and every string literal replaced by `\\0<index>\\0` (its newlines kept
    after the marker, so line numbers survive), plus the literals - a lexer for one question, not Python's.
    An unterminated single-quoted literal ends at its line; an unterminated triple-quoted one at the text's end."""
    out: list[str] = []
    strings: list[str] = []
    i, n = 0, len(text)
    while i < n:
        ch = text[i]
        if ch == "#":
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        m = _QUOTE_RE.match(text, i) if ch in "'\"rbufRBUF" else None
        if m is None or (m.group(1) and i > 0 and (text[i - 1].isalnum() or text[i - 1] == "_")):
            out.append(ch)
            i += 1
            continue
        quote, raw = m.group(2), "r" in m.group(1).lower()
        j = m.end()
        while j < n:
            if text[j] == "\\" and not raw:
                j += 2
                continue
            if text[j] == "\\" and raw:
                j += 2
                continue
            if text.startswith(quote, j):
                j += len(quote)
                break
            if text[j] == "\n" and len(quote) == 1:
                break
            j += 1
        literal = text[i:min(j, n)]
        out.append("\x00%d\x00" % len(strings) + "\n" * literal.count("\n"))
        strings.append(literal)
        i = j
    return "".join(out), strings


def object_calls(text: str) -> list[ObjectCall]:
    """Every `Object(flag, "path"` a text spells - a fragment (a diff line, a conflict side) as well as the
    whole file - and never a call inside a comment or a string literal."""
    if "Object" not in text:
        return []
    skeleton, strings = _skeleton(text)
    out: list[ObjectCall] = []
    for m in _CALL_RE.finditer(skeleton):
        try:
            path = ast.literal_eval(strings[int(m.group(2))])
        except (ValueError, SyntaxError):
            continue
        if isinstance(path, str):
            out.append(ObjectCall(m.group(1), path, skeleton.count("\n", 0, m.start()) + 1, m.group(3) is not None))
    return out


class _Namespace(dict):
    """`config` / `args`: attribute reads and writes on a dict."""


class _Func:
    def __init__(self, node: ast.FunctionDef) -> None:
        self.node = node
        body = [s for s in node.body if not (isinstance(s, ast.Expr) and isinstance(s.value, ast.Constant))]
        if len(body) != 1 or not isinstance(body[0], ast.Return) or body[0].value is None:
            raise _Unknown("not a single-return function")
        self.expr = body[0].value


class Configure:
    """`configure.py` evaluated statically: the cflags groups (spreads, filters, appends resolved once), the
    libs (dict literals and the single-return helpers such as `DolphinLib`), and every `Object(...)`."""

    def __init__(self, text: str, path: str = "<configure.py>", version: str = VERSION,
                 args: dict[str, Any] | None = None) -> None:
        self.path = path
        self.text = text
        self.version = version
        self._args = _Namespace(DEFAULT_ARGS, version=version, **(args or {}))
        self._config = _Namespace(version=version, non_matching=bool(self._args.get("non_matching")))
        self._env: dict[str, Any] = {"config": self._config, "args": self._args, "True": True, "False": False,
                                     "None": None}
        self._funcs: dict[str, _Func] = {}
        self._group_names: list[str] = []
        self._libs: list[Lib] = []
        self._loose: list[ObjectRow] = []
        self._lines: list[bytes] | None = None
        self.skipped: list[int] = []          # line numbers of statements the evaluator could not model
        # On an unclosed bracket CPython re-opens `filename` to quote the line: name a file only when it is one.
        self._run(ast.parse(text, filename=path if os.path.isfile(path) else os.devnull).body)
        self._libs = self._build_libs()

    @classmethod
    def load(cls, path: str | os.PathLike, **kw: Any) -> "Configure":
        p = Path(path)
        return cls(p.read_text(encoding="utf-8", errors="replace"), str(p), **kw)

    @classmethod
    def parse(cls, text: str, **kw: Any) -> "Configure":
        return cls(text, **kw)

    # --- the public views ------------------------------------------------------------------------------
    def libs(self) -> list[Lib]:
        return list(self._libs)

    def lib(self, name: str) -> Lib | None:
        return next((lib for lib in self._libs if lib.name == name), None)

    def objects(self) -> list[ObjectRow]:
        """Every registered object, in registration order (an `Object` listed in `config.libs` outside any
        lib entry counts, with `lib=None`)."""
        rows = [o for lib in self._libs for o in lib.objects] + self._loose
        return sorted(rows, key=lambda o: o.line)

    def object(self, path: str) -> ObjectRow | None:
        """The object registered as `path` (exact), else the one with the same extensionless path."""
        objs = self.objects()
        hit = next((o for o in objs if o.path == path), None)
        if hit is None:
            want = os.path.splitext(path.replace("\\", "/"))[0]
            hit = next((o for o in objs if os.path.splitext(o.path)[0] == want), None)
        return hit

    def groups(self) -> dict[str, tuple[str, ...]]:
        """Every `cflags_*` group, resolved, in definition order."""
        return {n: tuple(str(x) for x in self._env[n]) for n in self._group_names
                if isinstance(self._env.get(n), list)}

    def cflags(self, name: str) -> tuple[str, ...] | None:
        """A group by name (`cflags_main`) or a lib by name (`pl`): its resolved flags, or None."""
        groups = self.groups()
        if name in groups:
            return groups[name]
        lib = self.lib(name)
        return lib.cflags if lib else None

    def matching_units(self) -> list[str]:
        """The paths registered `Matching`."""
        return [o.path for o in self.objects() if o.matching]

    def object_line(self, path: str) -> int | None:
        o = self.object(path)
        return o.line if o else None

    def object_line_text(self, path: str) -> str | None:
        line = self.object_line(path)
        return self.text.splitlines()[line - 1] if line else None

    # --- the evaluator ---------------------------------------------------------------------------------
    def _src(self, node: ast.AST) -> str:
        """A node's text as the file spells it (`ast.get_source_segment` without re-splitting the file per call)."""
        if self._lines is None:
            self._lines = [ln.encode("utf-8") for ln in self.text.splitlines(keepends=True)]
        try:
            first, last = node.lineno - 1, node.end_lineno - 1
            if first == last:
                raw = self._lines[first][node.col_offset:node.end_col_offset]
            else:
                raw = (self._lines[first][node.col_offset:] + b"".join(self._lines[first + 1:last])
                       + self._lines[last][:node.end_col_offset])
            return raw.decode("utf-8")
        except (AttributeError, IndexError, TypeError, UnicodeDecodeError):
            return ast.unparse(node)

    def _run(self, body: list[ast.stmt]) -> None:
        for stmt in body:
            try:
                self._stmt(stmt)
            except _Unknown:
                self.skipped.append(stmt.lineno)

    def _stmt(self, stmt: ast.stmt) -> None:
        if isinstance(stmt, ast.FunctionDef):
            try:
                self._funcs[stmt.name] = _Func(stmt)
            except _Unknown:
                pass
            return
        if isinstance(stmt, ast.Assign) and len(stmt.targets) == 1:
            value = self._eval(stmt.value, {})
            self._assign(stmt.targets[0], value)
            return
        if isinstance(stmt, ast.AnnAssign) and stmt.value is not None:
            self._assign(stmt.target, self._eval(stmt.value, {}))
            return
        if isinstance(stmt, ast.AugAssign) and isinstance(stmt.op, ast.Add) and isinstance(stmt.target, ast.Name):
            cur = self._env.get(stmt.target.id)
            add = self._eval(stmt.value, {})
            if isinstance(cur, list) and isinstance(add, (list, tuple)):
                cur.extend(add)
                return
            raise _Unknown("augmented assignment")
        if isinstance(stmt, ast.If):
            self._run(stmt.body if self._eval(stmt.test, {}) else stmt.orelse)
            return
        if isinstance(stmt, ast.Expr) and isinstance(stmt.value, ast.Call):
            call = stmt.value
            if isinstance(call.func, ast.Attribute) and call.func.attr in ("append", "extend"):
                target = self._eval(call.func.value, {})
                if isinstance(target, list) and len(call.args) == 1 and not call.keywords:
                    arg = self._eval(call.args[0], {})
                    if call.func.attr == "append":
                        target.append(arg)
                    elif isinstance(arg, (list, tuple)):
                        target.extend(arg)
                    else:
                        raise _Unknown("extend with a non-list")
                    return
            raise _Unknown("an expression statement")
        raise _Unknown(type(stmt).__name__)

    def _assign(self, target: ast.expr, value: Any) -> None:
        if isinstance(target, ast.Name):
            self._env[target.id] = value
            if target.id.startswith("cflags_") and isinstance(value, list) and target.id not in self._group_names:
                self._group_names.append(target.id)
            return
        if isinstance(target, ast.Attribute) and isinstance(target.value, ast.Name):
            ns = self._env.get(target.value.id)
            if isinstance(ns, _Namespace):
                ns[target.attr] = value
                return
        raise _Unknown("assignment target")

    def _eval(self, node: ast.expr, local: dict[str, Any]) -> Any:
        if isinstance(node, ast.Constant):
            return node.value
        if isinstance(node, ast.Name):
            if node.id in local:
                return local[node.id]
            if node.id in self._env:
                return self._env[node.id]
            if node.id in FLAGS:
                return Flag(node.id, node.id == "Matching")
            raise _Unknown("name %s" % node.id)
        if isinstance(node, (ast.List, ast.Tuple)):
            out: list[Any] = []
            for elt in node.elts:
                if isinstance(elt, ast.Starred):
                    v = self._eval(elt.value, local)
                    if not isinstance(v, (list, tuple)):
                        raise _Unknown("a spread of a non-list")
                    out.extend(v)
                else:
                    out.append(self._eval(elt, local))
            return out if isinstance(node, ast.List) else tuple(out)
        if isinstance(node, ast.Dict):
            d: dict[Any, Any] = {}
            for k, v in zip(node.keys, node.values):
                if k is None:
                    raise _Unknown("a dict spread")
                key = self._eval(k, local)
                try:
                    d[key] = self._eval(v, local)
                except _Unknown:
                    d[key] = Unresolved(self._src(v))
                if key == "cflags" and isinstance(v, ast.Name):
                    d["__cflags_name__"] = v.id
            d["__line__"] = node.lineno
            return d
        if isinstance(node, ast.Attribute):
            base = self._eval(node.value, local)
            if isinstance(base, _Namespace):
                if node.attr in base:
                    return base[node.attr]
                raise _Unknown("attribute %s" % node.attr)
            raise _Unknown("attribute of a value")
        if isinstance(node, ast.JoinedStr):
            parts = []
            for v in node.values:
                if isinstance(v, ast.Constant):
                    parts.append(str(v.value))
                elif isinstance(v, ast.FormattedValue) and v.format_spec is None and v.conversion == -1:
                    parts.append(str(self._eval(v.value, local)))
                else:
                    raise _Unknown("a formatted value")
            return "".join(parts)
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
            a, b = self._eval(node.left, local), self._eval(node.right, local)
            if isinstance(a, list) and isinstance(b, tuple):
                b = list(b)
            try:
                return a + b
            except TypeError as exc:
                raise _Unknown("+") from exc
        if isinstance(node, ast.BoolOp):
            vals = node.values
            if isinstance(node.op, ast.And):
                result: Any = True
                for v in vals:
                    result = self._eval(v, local)
                    if not result:
                        return result
                return result
            result = False
            for v in vals:
                result = self._eval(v, local)
                if result:
                    return result
            return result
        if isinstance(node, ast.IfExp):
            return self._eval(node.body if self._eval(node.test, local) else node.orelse, local)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.Not):
            return not self._eval(node.operand, local)
        if isinstance(node, ast.Compare) and len(node.ops) == 1:
            a, b = self._eval(node.left, local), self._eval(node.comparators[0], local)
            op = node.ops[0]
            if isinstance(op, ast.In):
                return a in b
            if isinstance(op, ast.NotIn):
                return a not in b
            if isinstance(op, ast.Eq):
                return a == b
            if isinstance(op, ast.NotEq):
                return a != b
            if isinstance(op, ast.Is):
                return a is b
            if isinstance(op, ast.IsNot):
                return a is not b
            raise _Unknown("comparison")
        if isinstance(node, ast.ListComp) and len(node.generators) == 1:
            gen = node.generators[0]
            if not isinstance(gen.target, ast.Name) or gen.is_async:
                raise _Unknown("comprehension target")
            out = []
            for item in self._eval(gen.iter, local):
                inner = dict(local, **{gen.target.id: item})
                if all(self._eval(c, inner) for c in gen.ifs):
                    out.append(self._eval(node.elt, inner))
            return out
        if isinstance(node, ast.Call):
            return self._call(node, local)
        raise _Unknown(type(node).__name__)

    def _call(self, node: ast.Call, local: dict[str, Any]) -> Any:
        func = node.func
        if isinstance(func, ast.Name) and func.id == "Object":
            return self._object(node, local)
        if isinstance(func, ast.Name) and func.id in ("str", "list", "tuple", "len") and len(node.args) == 1:
            return {"str": str, "list": list, "tuple": tuple, "len": len}[func.id](self._eval(node.args[0], local))
        if isinstance(func, ast.Attribute) and func.attr == "index" and len(node.args) == 1:
            base = self._eval(func.value, local)
            if isinstance(base, (list, tuple, str)):
                return base.index(self._eval(node.args[0], local))
        if isinstance(func, ast.Name) and func.id in self._funcs:
            fn = self._funcs[func.id]
            a = fn.node.args
            params = [p.arg for p in a.args]
            values = [self._eval(x, local) for x in node.args]
            bound: dict[str, Any] = {}
            if a.vararg is not None:
                bound[a.vararg.arg] = tuple(values[len(params):])
                values = values[:len(params)]
            elif len(values) > len(params):
                raise _Unknown("too many arguments")
            bound.update(zip(params, values))
            for kw in node.keywords:
                if kw.arg is None:
                    raise _Unknown("a keyword spread")
                bound[kw.arg] = self._eval(kw.value, local)
            if any(p not in bound for p in params):
                raise _Unknown("a missing argument")
            result = self._eval(fn.expr, bound)
            if func.id == "MatchingFor" or isinstance(result, bool):
                return Flag(self._src(node), bool(result))
            return result
        raise _Unknown("call")

    def _object(self, node: ast.Call, local: dict[str, Any]) -> dict:
        if len(node.args) < 2:
            raise _Unknown("Object() without a flag and a path")
        flag_node = node.args[0]
        flag = self._eval(flag_node, local)
        if isinstance(flag_node, ast.Name) and flag_node.id in FLAGS:      # `Matching = True` in the file
            flag = Flag(flag_node.id, bool(flag) if not isinstance(flag, Flag) else flag.linked)
        elif not isinstance(flag, Flag):
            flag = Flag(self._src(flag_node), bool(flag))
        path = self._eval(node.args[1], local)
        if not isinstance(path, str):
            raise _Unknown("a non-string path")
        options = []
        kwargs: dict[str, Any] = {}
        cflags_name = None
        for kw in node.keywords:
            if kw.arg is None:
                raise _Unknown("a keyword spread")
            options.append((kw.arg, self._src(kw.value)))
            try:
                kwargs[kw.arg] = self._eval(kw.value, local)
            except _Unknown:
                kwargs[kw.arg] = Unresolved(self._src(kw.value))
            if kw.arg == "cflags" and isinstance(kw.value, ast.Name):
                cflags_name = kw.value.id
        return {"__object__": True, "flag": flag, "path": path, "options": tuple(options), "kwargs": kwargs,
                "cflags_name": cflags_name, "line": node.lineno}

    def _build_libs(self) -> list[Lib]:
        libs = self._config.get("libs")
        if not isinstance(libs, list):
            return []
        def text(value: Any) -> str | None:
            return value if isinstance(value, str) else None

        out = []
        for entry in libs:
            if isinstance(entry, dict) and entry.get("__object__"):     # an Object listed with no lib
                self._loose.append(ObjectRow(
                    path=entry["path"], flag=entry["flag"].spelling, linked=entry["flag"].linked, lib=None,
                    mw_version=text(entry["kwargs"].get("mw_version")), lib_cflags=None,
                    cflags_name=entry["cflags_name"],
                    cflags=tuple(str(x) for x in entry["kwargs"]["cflags"])
                    if isinstance(entry["kwargs"].get("cflags"), (list, tuple)) else (),
                    options=entry["options"], line=entry["line"]))
                continue
            if not isinstance(entry, dict) or not isinstance(entry.get("lib"), str):
                continue
            lib_cflags = entry.get("cflags")
            lib_flags = tuple(str(x) for x in lib_cflags) if isinstance(lib_cflags, (list, tuple)) else ()
            lib_group = entry.get("__cflags_name__")
            objects = entry.get("objects")
            rows = []
            for obj in objects if isinstance(objects, (list, tuple)) else ():
                if not (isinstance(obj, dict) and obj.get("__object__")):
                    continue
                kw = obj["kwargs"]
                own = kw.get("cflags")
                flags = tuple(str(x) for x in own) if isinstance(own, (list, tuple)) else lib_flags
                rows.append(ObjectRow(
                    path=obj["path"], flag=obj["flag"].spelling, linked=obj["flag"].linked, lib=entry["lib"],
                    mw_version=text(kw.get("mw_version", entry.get("mw_version"))), lib_cflags=lib_group,
                    cflags_name=obj["cflags_name"] if "cflags" in kw else lib_group, cflags=flags,
                    options=obj["options"], line=obj["line"]))
            out.append(Lib(entry["lib"], text(entry.get("mw_version")), lib_group, lib_flags,
                           text(entry.get("progress_category")), tuple(rows), entry.get("__line__", 0)))
        return out


def load(path: str | os.PathLike, **kw: Any) -> Configure:
    """`Configure.load(path)`."""
    return Configure.load(path, **kw)
