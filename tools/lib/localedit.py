"""Write the named local a regsim plan asks for into the source: a field-address walker or a hoisted call result.
Spec: docs/tools/spec/lib-localedit.md. CLI: none (library; tools/mwcc-debugger/regsim.py `run --apply-local` is the front end)."""
from __future__ import annotations

import os
import re
from dataclasses import dataclass, field
from pathlib import Path

from tools.lib import cextract, cscan, objcompare, regsim

R_PPC_REL24 = 10

_DECLARE = re.compile(r"declare r(\d+)\((.*?)\) as a local at declaration position (\d+)")
_FOLD = re.compile(r"fold r(\d+)\((.*?)\) into local r(\d+)\((.*?)\)")
_ADDI = re.compile(r"^addi r(\d+),r(\d+),(-?(?:0x[0-9A-Fa-f]+|\d+))$")
_FIELD = re.compile(r"/\*\s*\+0x([0-9A-Fa-f]+)\s*\*/\s*([^;/]+);")
_QUALIFIERS = re.compile(r"\b(?:extern|static|inline|const|volatile|struct|class)\b|\"[^\"]*\"")


class Refused(Exception):
    """An edit the tool will not write: `reason` says why, `manual` is the exact edit for the lane to make."""

    def __init__(self, reason: str, manual: str = ""):
        super().__init__(reason)
        self.reason = reason
        self.manual = manual


@dataclass
class Edit:
    """One edit of a plan: `declare` a temp as a local at `position`, or `fold` it into the local `target`."""
    kind: str
    vreg: int
    label: str
    position: int | None = None
    target: int | None = None
    target_label: str = ""


def parse_edit(edits) -> Edit | None:
    """The first edit string of a plan as an `Edit`, None for a plan with none."""
    for text in edits:
        m = _DECLARE.fullmatch(text)
        if m:
            return Edit("declare", int(m.group(1)), "r%s(%s)" % (m.group(1), m.group(2)), position=int(m.group(3)))
        m = _FOLD.fullmatch(text)
        if m:
            return Edit("fold", int(m.group(1)), "r%s(%s)" % (m.group(1), m.group(2)), target=int(m.group(3)),
                        target_label=m.group(4))
    return None


@dataclass
class NewLocal:
    """A local the edit adds to the declaration run: its name, declaration line and the block it is declared in."""
    vreg: int
    name: str
    line: str
    scope: int


@dataclass
class Change:
    """What an applied edit comes to: the exact `(old block, new block)` replacements, the new local (if the plan
    adds one to the declaration run) and a line saying what was done."""
    blocks: list[tuple[str, str]] = field(default_factory=list)
    new_local: NewLocal | None = None
    summary: str = ""


# --- the source's types ------------------------------------------------------------------------------------------

def closure(source: str, src_root: str) -> list[str]:
    """The source and every quoted include it reaches, as paths (includes resolve beside the includer, then at `src_root`)."""
    def resolve(name: str, includer: str) -> str | None:
        return cscan.resolve_include(name, [os.path.dirname(includer), src_root])
    return cscan.include_closure(source, resolve)


def read(path: str) -> str:
    with open(path, encoding="utf-8", newline="") as fh:
        return fh.read()


@dataclass(frozen=True)
class Field:
    offset: int
    type: str
    name: str
    dims: str


def struct_fields(files: list[str], struct: str) -> list[Field]:
    """The `/* +0xNN */ TYPE NAME[DIMS];` fields of `struct STRUCT { ... }` in the first of `files` that defines it."""
    for path in files:
        try:
            text = read(path)
        except OSError:
            continue
        t = cscan.Text(text)
        for m in re.finditer(r"\b(?:struct|class)\s+%s\b[^;{]*\{" % re.escape(struct), t.code):
            close = cscan.match_brace(t.code, m.end() - 1)
            if close < 0:
                continue
            out = []
            for f in _FIELD.finditer(text[m.end():close]):
                d = re.match(r"(.*?)(\w+)((?:\s*\[[^\]]*\])*)\s*$", f.group(2).strip(), re.S)
                if d and d.group(1).strip():
                    out.append(Field(int(f.group(1), 16), d.group(1).strip(), d.group(2), d.group(3).replace(" ", "")))
            if out:
                return out
    return []


def struct_of(type_text: str) -> str | None:
    """`Q_MySet` from `Q_MySet*` / `const Q_MySet *` (a pointer to a class), None for anything else."""
    t = _QUALIFIERS.sub(" ", type_text).strip()
    m = re.fullmatch(r"([A-Za-z_]\w*)\s*\*", t.replace(" *", "*"))
    return m.group(1) if m else None


def declared_type(name: str, text: str, function: str) -> str | None:
    """The declared type of the parameter or local `name` of `function`, as written."""
    decl = regsim.find_definition(text, function)
    if decl is None:
        return None
    for chunk, _ in cscan.split_params(decl.params, 0, len(decl.params)):
        m = re.match(r"(.*?)\b%s\s*$" % re.escape(name), chunk.strip(), re.S)
        if m and m.group(1).strip():
            return m.group(1).strip()
    for loc in regsim.source_locals(text, function):
        if loc.name == name:
            m = re.match(r"(.*?)\b%s\b" % re.escape(name), loc.text.strip())
            return m.group(1).strip() if m else None
    return None


def callee_return_type(name: str, files: list[str]) -> str | None:
    """The return type `name` is declared with in one of `files` (qualifiers dropped), None when none declares it."""
    for path in files:
        try:
            t = cscan.Text(read(path))
        except OSError:
            continue
        for d in cscan.function_declarations(t):
            if d.name == name:
                ret = " ".join(re.sub(r"\b(?:extern|static|inline)\b|\"[^\"]*\"", " ", d.ret).split())
                if ret and ret != "void":
                    return ret
    return None


def function_span(text: str, function: str) -> tuple[int, int] | None:
    """(open brace, close brace) offsets of `function`'s body in `text`."""
    decl = regsim.find_definition(text, function)
    return decl.body if decl and decl.body else None


# --- the temp is `base + field offset` in a counted loop: a walking pointer ---------------------------------------

def pointer_name(field_name: str, taken: set[str]) -> str:
    """`part` for `parts_0x02` (`<field>_ptr` for a name that is not a plural); a free identifier."""
    stem = re.sub(r"_0x[0-9A-Fa-f]+$", "", field_name)
    name = stem[:-1] if stem.endswith("s") and len(stem) > 2 else stem + "_ptr"
    while name in taken:
        name += "_p"
    return name


_FOR = re.compile(r"\bfor\s*\(\s*(\w+)\s*=\s*0\s*;\s*\1\s*<\s*[^;()]+;\s*\1\s*\+\+\s*\)")


def walker(g: regsim.Graph, edit: Edit, instrs: list, text: str, function: str, source: str, src_root: str,
           mapping: dict, plan_decl: list[int]) -> Change:
    """`declare` of a temp defined by `addi T, L, K` where `L` is a pointer local or parameter to a class with a field
    at +K: the loop `for (IV = 0; IV < N; IV++)` that reads `&L->field[IV]` takes a pointer local that walks the
    array - `for (IV = 0, p = L->field; IV < N; p++, IV++)` with `p` at the sites. `Refused` otherwise."""
    line = g.defs.get(edit.vreg, "")
    m = _ADDI.match(line)
    if not m or int(m.group(1)) != edit.vreg:
        raise Refused("the temp's first PCode line is `%s`, not an `addi T,L,K` of a local plus a field offset" % line,
                      _manual_declare(g, edit, line))
    base, off = int(m.group(2)), int(m.group(3), 0)
    base_name = g.nodes[base].name if base in g.nodes else ""
    if not base_name or base_name.startswith("@"):
        raise Refused("the base of `%s` is not a named local or parameter" % line, _manual_declare(g, edit, line))
    type_text = declared_type(base_name, text, function)
    struct = struct_of(type_text or "")
    if struct is None:
        raise Refused("`%s` is declared `%s`, not a pointer to a class" % (base_name, type_text), _manual_declare(g, edit, line))
    fields = [f for f in struct_fields(closure(source, src_root), struct) if f.offset == off]
    if len(fields) != 1:
        raise Refused("%s has %d field(s) at +0x%X (the `/* +0x%X */` comments)" % (struct, len(fields), off, off),
                      _manual_declare(g, edit, line))
    fld = fields[0]
    elem = fld.type
    span = function_span(text, function)
    t = cscan.Text(text)
    body = t.code[span[0] + 1:span[1]]
    taken = set(re.findall(r"[A-Za-z_]\w*", body)) | set(re.findall(r"[A-Za-z_]\w*", text[:span[0]].rsplit("\n", 1)[-1]))
    name = pointer_name(fld.name, taken)
    site = r"&\s*%s\s*->\s*%s\s*\[\s*(\w+)\s*\]" % (re.escape(base_name), re.escape(fld.name))
    loops = []
    for fm in _FOR.finditer(body):
        iv = fm.group(1)
        brace = body.find("{", fm.end())
        if brace < 0 or body[fm.end():brace].strip():
            continue      # a braceless body: refuse below rather than guess its extent
        close = cscan.match_brace(body, brace)
        users = [s for s in re.finditer(site, body[brace:close]) if s.group(1) == iv]
        if users:
            loops.append((fm, brace, close, iv))
    manual = ("declare `%s* %s;` in the order the plan gives, and walk it: `for (%s = 0, %s = %s->%s; ...; %s++, %s++)`"
              " with `%s` where `&%s->%s[%s]` was written" % (elem, name, "IV", name, base_name, fld.name, name, "IV",
                                                              name, base_name, fld.name, "IV"))
    if len(loops) != 1:
        raise Refused("%d braced counted loop(s) `for (IV = 0; IV < N; IV++)` read `&%s->%s[IV]` (one is needed)"
                      % (len(loops), base_name, fld.name), manual)
    fm, brace, close, iv = loops[0]
    head_old = body[fm.start():fm.end()]
    head_new = re.sub(r"=\s*0\s*;", "= 0, %s = %s->%s;" % (name, base_name, fld.name), head_old, count=1)
    head_new = re.sub(r"%s\s*\+\+\s*\)$" % re.escape(iv), "%s++, %s++)" % (name, iv), head_new)
    # the loop's own lines in the file (the body view and the text share offsets)
    abs_start = span[0] + 1 + fm.start()
    abs_end = span[0] + 1 + close + 1
    first_line, last_line = t.line_of(abs_start), t.line_of(abs_end - 1)
    old_block = "\n".join(text.split("\n")[first_line - 1:last_line])
    new_block = _splice(old_block, head_old, head_new, site, name, iv)
    scope = _scope_of_neighbours(edit.vreg, plan_decl, mapping)
    local = NewLocal(edit.vreg, name, "%s* %s;" % (elem, name), scope)
    return Change([(old_block, new_block)], local,
                  "declare `%s* %s` (the pointer `%s->%s`) and walk it in the loop over `%s`" % (elem, name, base_name,
                                                                                           fld.name, iv))


def _splice(old_block: str, head_old: str, head_new: str, site: str, name: str, iv: str) -> str:
    """`old_block` (whole lines) with the loop's header and its `&L->f[IV]` sites rewritten."""
    out = old_block.replace(head_old, head_new, 1)
    return re.sub(site, lambda s: name if s.group(1) == iv else s.group(0), out)


def _scope_of_neighbours(vreg: int, decl: list[int], mapping: dict) -> int:
    """The block a new local is declared in: its neighbours' in the plan's order (`Refused` when they differ)."""
    i = decl.index(vreg)
    near = [mapping[v].scope for v in (decl[i - 1] if i else None, decl[i + 1] if i + 1 < len(decl) else None)
            if v is not None and v in mapping]
    if not near or len(set(near)) != 1:
        if len(near) == 2 and near[0] != near[1]:
            raise Refused("the plan's position lies between declarations of two blocks")
        return near[0] if near else 0
    return near[0]


def _manual_declare(g: regsim.Graph, edit: Edit, line: str) -> str:
    return ("name the temp %s (first seen `%s`) as a local declared at position %s of the plan's order, initialised where "
            "that value is computed" % (edit.label, line, edit.position))


# --- the value crosses a call in ours and not in retail: evaluate the call first --------------------------------

def flatten(dump_text: str) -> list[tuple[str, tuple[str, ...]]]:
    """The instructions of a backend dump in dump order."""
    return [ins for _, _, block in regsim.parse_blocks(dump_text) for ins in block]


def call_symbols(obj: str, symbol: str) -> list[str] | None:
    """The callee of each `bl` of `symbol` in `obj`, in code order (a symbol name per R_PPC_REL24 relocation)."""
    syms = objcompare.section_symbols(obj, ".text")
    rows, _ = objcompare.reloc_rows(obj)
    if symbol not in syms or rows is None:
        return None
    off, size = syms[symbol]
    return [s for o, s, typ, _ in rows.get(".text", []) if off <= o < off + size and typ == R_PPC_REL24]


def local_name_for(callee: str, taken: set[str]) -> str:
    """A name for a call result: the callee's, minus a trailing `_get`/`Get` (`hunter_rank_points_get` ->
    `hunter_rank_points`); a free identifier."""
    name = re.sub(r"(_get|Get)$", "", callee) or callee
    while name in taken or name == callee:
        name += "_value"
    return name


def hoist(g: regsim.Graph, vreg: int, instrs: list, obj: str, symbol: str, text: str, function: str, source: str,
          src_root: str, local_name: str | None = None) -> Change:
    """The temp `vreg` is live across exactly one `bl` in ours and across none in retail: evaluate that call first, into a
    local - `T v = callee(args);` on the line before its statement and `v` in its place. `Refused` unless exactly one
    call lies in the temp's live range, the object's calls match the dump's, the callee's source calls match
    its object calls one for one, and the statement is a plain one that may be reordered."""
    label = g.nodes[vreg].label()
    pat = re.compile(r"\br%d\b" % vreg)
    uses = [i for i, (_, ops) in enumerate(instrs) if any(pat.fullmatch(o) for o in ops)]
    if not uses:
        raise Refused("%s appears in no instruction of the dump" % label)
    first, last = uses[0], uses[-1]
    bl = [i for i, (mn, _) in enumerate(instrs) if mn == "bl"]
    indirect = [i for i in range(first, last) if instrs[i][0] in ("bctrl", "blrl")]
    crossed = [i for i in bl if first < i < last]
    manual = "evaluate the call between %s's first and last use into a named local before the statement that uses it" % label
    if indirect or len(crossed) != 1:
        raise Refused("%s is live across %d direct and %d indirect call(s); one direct call is needed"
                      % (label, len(crossed), len(indirect)), manual)
    callees = call_symbols(obj, symbol)
    if callees is None or len(callees) != len(bl):
        raise Refused("the dump has %d `bl`, the object's %s has %s relocated call(s)"
                      % (len(bl), symbol, "?" if callees is None else len(callees)), manual)
    rank = bl.index(crossed[0])
    callee = callees[rank]
    ident = sorted(cextract.target_identifiers(callee))
    if len(ident) != 1:
        raise Refused("callee %s has no single source name" % callee, manual)
    name = ident[0]
    nth = sum(1 for c in callees[:rank] if c == callee)
    span = function_span(text, function)
    t = cscan.Text(text)
    body_calls = [c for c in cscan.calls(t.code[:span[1]], name) if c.start > span[0]]
    if len(body_calls) != sum(1 for c in callees if c == callee):
        raise Refused("%s is called %d time(s) in the source but %d in the object" % (
            name, len(body_calls), sum(1 for c in callees if c == callee)), manual)
    site = body_calls[nth]
    line_no = t.line_of(site.start)
    lines = text.split("\n")
    stmt = lines[line_no - 1]
    start = t.code.rfind("\n", 0, site.start) + 1
    prefix = t.code[start:site.start]
    kw = re.match(r"\s*(if|return|switch)\b", prefix)
    if re.match(r"\s*(else|while|for|do)\b", prefix):
        raise Refused("the call is in a `%s` header, evaluated more than once or conditionally" % prefix.split()[0], manual)
    inner = prefix[kw.end():] if kw else prefix
    if re.search(r"&&|\|\||\?|,|\+\+|--", inner):
        raise Refused("a sequenced operator or side effect precedes the call in `%s`" % stmt.strip(), manual)
    if line_no < 2 or not re.search(r"[;{}]\s*$", t.code.split("\n")[line_no - 2]):
        raise Refused("the statement is not the first after a `;`, `{` or `}` line (a braceless body?)", manual)
    ret = callee_return_type(name, closure(source, src_root))
    if ret is None:
        raise Refused("no declaration of %s with a return type was found in the unit's includes" % name, manual)
    taken = set(re.findall(r"[A-Za-z_]\w*", t.code[span[0]:span[1]]))
    local = local_name or local_name_for(name, taken)
    call_text = text[site.start:site.end]
    indent = re.match(r"\s*", stmt).group()
    new_stmt = stmt.replace(call_text, local, 1)
    context = lines[line_no - 2]
    old_block = context + "\n" + stmt
    new_block = "%s\n%s%s %s = %s;\n%s" % (context, indent, ret, local, call_text, new_stmt)
    return Change([(old_block, new_block)], None,
                  "evaluate `%s` first: `%s %s = %s;` before its statement" % (call_text, ret, local, call_text))


# --- the fold of a backend temp into a local: no source edit of its own -----------------------------------------

def fold(g: regsim.Graph, edit: Edit) -> Change:
    """`fold T into local L` has no mechanical spelling: it is always `Refused`, with the reason the graph gives -
    a local that is a dead node (the optimiser already propagated its value into the temp) or a live one the lane
    reuses for the temp's value."""
    target = g.nodes.get(edit.target)
    line = g.defs.get(edit.vreg, "?")
    if target is not None and not target.neighbors:
        raise Refused(
            "local %s interferes with nothing (the optimiser propagated it away, it holds no register), so folding the "
            "temp %s (`%s`) into it is not a source change" % (edit.target_label, edit.label, line),
            "the plan wants %s numbered where %s sits in the declaration order; keep %s live past the optimiser "
            "(a use it cannot fold) or move its declaration, then measure" % (edit.label, edit.target_label,
                                                                             edit.target_label))
    raise Refused(
        "folding %s (`%s`) into local %s means reusing that local for the temp's value, which needs the temp's "
        "uses in the source" % (edit.label, line, edit.target_label),
        "assign the value computed by `%s` to local %s where it is computed and read %s where the temp was read"
        % (line, edit.target_label, edit.target_label))
