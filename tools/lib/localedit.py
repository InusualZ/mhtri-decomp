"""Write the named local a regsim plan asks for into the source: a field-address walker or a hoisted call result.
Spec: docs/tools/spec/lib-localedit.md. CLI: none (library; tools/mwcc-debugger/regsim.py `run --apply-local` is the front end)."""
from __future__ import annotations

import os
import re
from dataclasses import dataclass, field
from pathlib import Path

from tools.lib import cextract, cscan, declperm, objcompare, regsim

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


_SAVE_REST = re.compile(r"^_(?:save|rest)[gf]pr_\d+$")


def call_symbols(obj: str, symbol: str) -> list[str] | None:
    """The callee of each `bl` of `symbol` in `obj`, in code order (a symbol name per R_PPC_REL24 relocation); the
    register save/restore helpers of the prologue and epilogue (`_savegpr_27`) are not calls of the source."""
    syms = objcompare.section_symbols(obj, ".text")
    rows, _ = objcompare.reloc_rows(obj)
    if symbol not in syms or rows is None:
        return None
    off, size = syms[symbol]
    return [s for o, s, typ, _ in rows.get(".text", [])
            if off <= o < off + size and typ == R_PPC_REL24 and not _SAVE_REST.match(s)]


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
    return hoist_call(bl.index(crossed[0]), callees, manual, text, function, source, src_root, local_name)


def hoist_call(rank: int, callees: list[str], manual: str, text: str, function: str, source: str, src_root: str,
               local_name: str | None = None, assign_only: bool = False) -> Change:
    """The `rank`-th call of the function (`callees`: the object's callee per `bl`, code order) evaluated first into a
    local: `T v = callee(args);` on the line before its statement, or with `assign_only` `v = callee(args);` (the
    declaration `T v;` then goes into the caller's declaration run: the change carries it as a `NewLocal`), and `v` in
    its place. `Refused` unless the callee has one source name, the source's calls match the object's, and the
    statement is a plain one that may be reordered."""
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
    if assign_only:
        own = re.fullmatch(r"\s*(\w+)\s*=\s*%s\s*;\s*" % re.escape(call_text), stmt)
        if declperm.declared("    " + stmt.strip()):
            raise Refused("the call is the initialiser of a declaration (`%s`): its lines are the declaration run the plan "
                          "reorders, so the edit is written by hand" % stmt.strip(), manual)
        also = " (a second local beside `%s`, which the statement already assigns it to)" % own.group(1) if own else ""
        new_block = "%s\n%s%s = %s;\n%s" % (context, indent, local, call_text, new_stmt)
        return Change([(old_block, new_block)], NewLocal(0, local, "%s %s;" % (ret, local), 0),
                      "evaluate `%s` first into the local `%s %s`, assigned before its statement%s" % (call_text, ret,
                                                                                                      local, also))
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


# --- the other temps a plan declares: a constant, a call result copied out of r3, a sum ---------------------------

_LI = re.compile(r"^li r(\d+),(-?(?:0x[0-9A-Fa-f]+|\d+))$")
_MR_R3 = re.compile(r"^mr r(\d+),r3$")
_ADD = re.compile(r"^add r(\d+),r(\d+),r(\d+)$")
_INT_WORDS = frozenset(("u8", "s8", "u16", "s16", "u32", "s32", "int", "short", "char", "long", "unsigned", "signed",
                        "BOOL", "bool"))
_UNSIGNED_WORDS = frozenset(("u32", "unsigned"))
_CONSTANT_NAMES = {0: "zero", 1: "one"}
_STORE = re.compile(r"^st")


def _taken_names(t: "cscan.Text", span: tuple[int, int], text: str) -> set[str]:
    """Every identifier of the function (its head and body): a new local's name avoids them."""
    head = text[:span[0]].rsplit("\n", 1)[-1]
    return set(re.findall(r"[A-Za-z_]\w*", t.code[span[0]:span[1]])) | set(re.findall(r"[A-Za-z_]\w*", head))


def _free_name(stem: str, taken: set[str]) -> str:
    name = stem
    while name in taken:
        name += "_v"
    return name


def _statement_start(t: "cscan.Text", pos: int, manual: str) -> int:
    """The 1-based line of offset `pos` when its statement starts there and a value may be computed before it: the line
    follows one ending `;`, `{` or `}`, and is not an `else`/`while`/`for`/`do` header (evaluated more than once or
    conditionally). `Refused` otherwise."""
    line = t.line_of(pos)
    code_lines = t.code.split("\n")
    if re.match(r"\s*(else|while|for|do)\b", code_lines[line - 1]):
        raise Refused("the use is in a `%s` header, evaluated more than once or conditionally"
                      % code_lines[line - 1].split()[0], manual)
    prev = next((x for x in reversed(code_lines[:line - 1]) if x.strip()), "")
    if not re.search(r"[;{}]\s*$", prev):
        raise Refused("the statement is not the first after a `;`, `{` or `}` line (a braceless body?)", manual)
    return line


def enclosing_block(code: str, lo: int, pos: int) -> tuple[int, int]:
    """(open, close) offsets of the innermost `{ }` of `code` that holds `pos`, counted from offset `lo`."""
    stack: list[int] = []
    for i in range(lo, pos):
        if code[i] == "{":
            stack.append(i)
        elif code[i] == "}" and stack:
            stack.pop()
    if not stack:
        raise Refused("the use is outside every block of the function")
    return stack[-1], cscan.match_brace(code, stack[-1])


def assign_before_first(t: "cscan.Text", text: str, span: tuple[int, int], sites: list[tuple[int, int]], name: str,
                        value: str, manual: str, unchanged: list[str] | None = None) -> list[tuple[str, str]]:
    """`name = value;` on the line before the first of `sites` (offset spans of `text`), and `name` at every site: one
    `(old, new)` block over the lines first..last. `Refused` unless every site lies in the block that holds the first
    one with no `case`/`default` label between (so the assignment runs before every use), and, for `unchanged`
    (variables the value is built from), none of them is written between the first and the last site."""
    first, last = sites[0][0], sites[-1][1]
    line1 = _statement_start(t, first, manual)
    open_, close = enclosing_block(t.code, span[0], first)
    if last > close:
        raise Refused("the uses are not all inside the block of the first one", manual)
    if open_ == span[0] and declperm.declared("    " + t.line_text(line1).strip()):
        raise Refused("the first use is a function-scope declaration's initialiser (`%s`): its lines are the declaration "
                      "run the plan reorders, so the edit is written by hand" % t.line_text(line1).strip(), manual)
    if re.search(r"\b(?:case|default)\b|\bgoto\b", t.code[first:last]):
        raise Refused("a `case`/`default` label or a goto lies between the uses: the assignment may not run first", manual)
    for var in unchanged or []:
        if re.search(r"\b%s\b\s*(?:[-+*/%%&|^]|<<|>>)?=(?!=)|(?:\+\+|--)\s*%s\b|\b%s\s*(?:\+\+|--)" % ((re.escape(var),) * 3),
                     t.code[first:last]):
            raise Refused("`%s` is written between the first and the last use" % var, manual)
    line2 = t.line_of(max(first, last - 1))
    lines = text.split("\n")
    starts = [0] + [m.end() for m in re.finditer("\n", text)]
    base = starts[line1 - 1]
    segment = "\n".join(lines[line1 - 1:line2])
    out, cursor = [], 0
    for s0, s1 in sites:
        out.append(segment[cursor:s0 - base])
        out.append(name)
        cursor = s1 - base
    out.append(segment[cursor:])
    indent = re.match(r"\s*", lines[line1 - 1]).group()
    new = "%s%s = %s;\n%s" % (indent, name, value, "".join(out))
    if text.count(segment) != 1:
        raise Refused("the lines of the uses are not unique in the file", manual)
    return [(segment, new)]


def _uses_of(instrs: list, vreg: int) -> list[int]:
    pat = re.compile(r"r%d$" % vreg)
    return [i for i, (_, ops) in enumerate(instrs) if any(pat.fullmatch(o) for o in ops)]


def _writes(instr: tuple, vreg: int) -> bool:
    """Whether the dump instruction `(mnemonic, operands)` writes register `vreg` (its first operand, unless it is a
    store, a compare or a branch)."""
    mnemonic, ops = instr
    return bool(ops) and ops[0] == "r%d" % vreg and not re.match(r"st|cmp|b|mt|tw", mnemonic)


def _body(text: str, function: str):
    span = function_span(text, function)
    if span is None:
        raise Refused("the definition of %s was not found in the source" % function)
    return cscan.Text(text), span


def constant_local(g: regsim.Graph, edit: Edit, instrs: list, text: str, function: str, mapping: dict,
                   plan_decl: list[int], local_name: str | None = None) -> Change:
    """`declare` of a temp defined by `li T,K` (a constant the optimiser keeps in a register across uses): a local
    `s32 zero;` in the plan's place, `zero = K;` before the first statement that assigns the literal `K`, and `zero` as
    the value of every `<lvalue> = K;` statement in that block. `Refused` when no statement assigns `K`, or the
    assignments are not all in the first one's block."""
    line = g.defs.get(edit.vreg, "")
    m = _LI.match(line)
    manual = _manual_declare(g, edit, line)
    if not m or int(m.group(1)) != edit.vreg:
        raise Refused("the temp's first PCode line is `%s`, not `li T,K`" % line, manual)
    k = int(m.group(2), 0)
    t, span = _body(text, function)
    uses = _uses_of(instrs, edit.vreg)
    again = [i for i in uses[1:] if _writes(instrs[i], edit.vreg)]
    if again:
        raise Refused("r%d is written again (`%s %s`): a counter or an offset that changes, not a constant"
                      % (edit.vreg, instrs[again[0]][0], ",".join(instrs[again[0]][1])), manual)
    sites = []
    pos = 0
    for raw in t.code.split("\n"):
        mm = re.match(r"^(\s*)([^=;{}]*[\w\])])\s*=\s*(-?(?:0[xX][0-9A-Fa-f]+|\d+))[uUlL]*\s*;\s*$", raw)
        if mm and span[0] < pos < span[1] and int(mm.group(3), 0) == k and \
                not declperm.declared("    " + raw.strip()) and not re.match(r"\s*(return|typedef)\b", raw):
            sites.append((pos + mm.start(3), pos + mm.end(3)))
        pos += len(raw) + 1
    if not sites:
        raise Refused("no statement `<lvalue> = %d;` in %s: nothing to give the constant's local to" % (k, function), manual)
    name = local_name or _free_name(_CONSTANT_NAMES.get(k, "const_%s" % abs(k) if k >= 0 else "const_m%d" % -k),
                                    _taken_names(t, span, text))
    literal = text[sites[0][0]:sites[0][1]]
    blocks = assign_before_first(t, text, span, sites, name, literal, manual)
    scope = _scope_of_neighbours(edit.vreg, plan_decl, mapping)
    return Change(blocks, NewLocal(edit.vreg, name, "s32 %s;" % name, scope),
                  "name the constant %s as `s32 %s`: `%s = %s;` before the first of %d `= %s;` statement(s) (the dump "
                  "has %d use(s) of the temp)" % (edit.label, name, name, literal, len(sites), literal, len(uses) - 1))


def call_result_local(g: regsim.Graph, edit: Edit, instrs: list, obj: str, symbol: str, text: str, function: str,
                      source: str, src_root: str, mapping: dict, plan_decl: list[int],
                      local_name: str | None = None) -> Change:
    """`declare` of a temp defined by `mr T,r3` (a copy of a call's result): the call is evaluated first into a local
    declared in the plan's place - `v = callee(args);` on the line before its statement and `v` in the call's place.
    The call is the `bl` the copy follows; `Refused` as `hoist_call` does, or when r3 is rewritten between them."""
    line = g.defs.get(edit.vreg, "")
    manual = _manual_declare(g, edit, line)
    if not _MR_R3.match(line):
        raise Refused("the temp's first PCode line is `%s`, not `mr T,r3`" % line, manual)
    def_at = next((i for i, (mn, ops) in enumerate(instrs) if mn == "mr" and ops == ("r%d" % edit.vreg, "r3")), None)
    if def_at is None:
        raise Refused("the copy `%s` is not in the dump" % line, manual)
    back = next((i for i in range(def_at - 1, -1, -1) if instrs[i][0] in ("bl", "bctrl", "blrl")), None)
    if back is None or instrs[back][0] != "bl":
        raise Refused("the copy of r3 does not follow a direct call", manual)
    for mn, ops in instrs[back + 1:def_at]:
        if ops and ops[0] == "r3" and not _STORE.match(mn) and not mn.startswith("cmp"):
            raise Refused("r3 is rewritten between the call and its copy", manual)
    bl = [i for i, (mn, _) in enumerate(instrs) if mn == "bl"]
    callees = call_symbols(obj, symbol)
    if callees is None or len(callees) != len(bl):
        raise Refused("the dump has %d `bl`, the object's %s has %s relocated call(s)"
                      % (len(bl), symbol, "?" if callees is None else len(callees)), manual)
    change = hoist_call(bl.index(back), callees, manual, text, function, source, src_root, local_name, assign_only=True)
    scope = _scope_of_neighbours(edit.vreg, plan_decl, mapping)
    change.new_local = NewLocal(edit.vreg, change.new_local.name, change.new_local.line, scope)
    return change


def _int_type(type_text: str | None) -> str | None:
    """`s32`/`u32` - what a sum of operands of this declared type is - or None when it is not an integer type."""
    words = re.findall(r"[A-Za-z_]\w*", _QUALIFIERS.sub(" ", type_text or ""))
    if not words or "*" in (type_text or "") or any(w not in _INT_WORDS for w in words):
        return None
    return "u32" if any(w in _UNSIGNED_WORDS for w in words) and not any(w in ("u8", "u16") for w in words) else "s32"


def _whole_operand(code: str, start: int, end: int) -> bool:
    """Whether `code[start:end]` (`A + B`) is one operand of the expression around it: not under a tighter operator
    (`x * A + B`, `A + B * 2`), a unary one, a cast or a member access."""
    before, after = code[:start].rstrip(), code[end:].lstrip()
    return not (before and before[-1] in "*/%-+~!.)") and not (after and after[0] in "*/%.(")


def sum_local(g: regsim.Graph, edit: Edit, text: str, function: str, mapping: dict, plan_decl: list[int],
              local_name: str | None = None) -> Change:
    """`declare` of a temp defined by `add T,A,B` with `A` and `B` named integer locals or parameters: a local
    `s32 sum;` in the plan's place, `sum = A + B;` before the first statement that writes `A + B` (or `B + A`) and `sum`
    at every such place in that block. `Refused` for a pointer operand (the sum is then `&p->field[i]`, which has no
    spelling as a plain `A + B`), an operand written between the uses, or no `A + B` in the source."""
    line = g.defs.get(edit.vreg, "")
    manual = _manual_declare(g, edit, line)
    m = _ADD.match(line)
    if not m or int(m.group(1)) != edit.vreg:
        raise Refused("the temp's first PCode line is `%s`, not `add T,A,B`" % line, manual)
    operands = []
    for v in (int(m.group(2)), int(m.group(3))):
        n = g.nodes.get(v)
        if n is None or not n.name or n.name.startswith("@") or n.name == "-":
            raise Refused("an operand of `%s` is not a named local or parameter" % line, manual)
        operands.append(n.name)
    a, b = operands
    t, span = _body(text, function)
    types = [declared_type(x, text, function) for x in operands]
    kinds = [_int_type(x) for x in types]
    if None in kinds:
        bad = operands[kinds.index(None)]
        raise Refused("`%s` is declared `%s`, not an integer: a sum with it is address arithmetic, not a local a "
                      "developer names" % (bad, types[kinds.index(None)]), manual)
    pattern = re.compile(r"(?<![\w.>])(?:%s\s*\+\s*%s|%s\s*\+\s*%s)(?![\w(\[.])" % ((re.escape(a), re.escape(b),
                                                                                       re.escape(b), re.escape(a))))
    sites = [(span[0] + x.start(), span[0] + x.end()) for x in pattern.finditer(t.code[span[0]:span[1]])
             if _whole_operand(t.code, span[0] + x.start(), span[0] + x.end())]
    if not sites:
        raise Refused("`%s + %s` is not written in %s: the sum comes from an address or index expression the source "
                      "spells differently" % (a, b, function), manual)
    name = local_name or _free_name("sum", _taken_names(t, span, text))
    ctype = "u32" if "u32" in kinds else "s32"
    blocks = assign_before_first(t, text, span, sites, name, "%s + %s" % (a, b), manual, [a, b])
    scope = _scope_of_neighbours(edit.vreg, plan_decl, mapping)
    return Change(blocks, NewLocal(edit.vreg, name, "%s %s;" % (ctype, name), scope),
                  "name the sum %s as `%s %s`: `%s = %s + %s;` before the first of %d use(s)"
                  % (edit.label, ctype, name, name, a, b, len(sites)))


def declare(g: regsim.Graph, edit: Edit, instrs: list, text: str, function: str, source: str, src_root: str,
            mapping: dict, plan_decl: list[int], obj: str, symbol: str, local_name: str | None = None) -> Change:
    """The local a `declare` edit asks for, by what defines the temp: `addi T,L,K` a walking pointer (`walker`),
    `li T,K` a constant, `mr T,r3` a call's result, `add T,A,B` a sum. `Refused` (with the edit to make by hand)
    for any other definition."""
    line = g.defs.get(edit.vreg, "")
    mnemonic = line.split(None, 1)[0] if line else ""
    if mnemonic == "li":
        return constant_local(g, edit, instrs, text, function, mapping, plan_decl, local_name)
    if mnemonic == "mr":
        return call_result_local(g, edit, instrs, obj, symbol, text, function, source, src_root, mapping, plan_decl,
                                 local_name)
    if mnemonic == "add":
        return sum_local(g, edit, text, function, mapping, plan_decl, local_name)
    if mnemonic == "addi":
        return walker(g, edit, instrs, text, function, source, src_root, mapping, plan_decl)
    raise Refused("the temp's first PCode line is `%s`: `addi`, `li`, `mr T,r3` and `add` are the definitions a local "
                  "is written for" % line, _manual_declare(g, edit, line))


# --- idea 324: a plan that lists block-scope locals hoists them to function scope in the plan's order ---------------

def _stays_put(loc: "regsim.SourceLocal", lines: list[str], duplicated: set[str]) -> str:
    """Why `loc` cannot be moved to function scope ('' when it can): its name is declared more than once in the function
    (two blocks each name an `i`), it is a `for` variable, or a `const` with an initialiser."""
    stripped = lines[loc.line - 1].strip()
    if loc.name in duplicated:
        return "`%s` is declared in more than one block" % loc.name
    if stripped.startswith("for") or re.match(r"for\b", stripped):
        return "`%s` is a `for` variable" % loc.name
    if loc.init and stripped.startswith("const "):
        return "`%s` is a `const` with an initialiser" % loc.name
    return ""


def hoist_order(decl: list[int], mapping: dict, text: str, function: str) -> Change:
    """The plan's declaration order `decl` (first declared = highest number) spelled at function scope: every local
    the plan names - the function-scope ones and the block-scope ones - becomes one declaration in one run at the
    first function-scope declaration's line, in the plan's order; an initialiser stays on its own line as an assignment
    (`T v = e;` in a block becomes `v = e;` there, so `T v = f();` keeps its call order). A block-scope local whose
    name is declared twice, a `for` variable and a `const` initialised stay where they are. One `(old, new)` block over
    the touched lines. `Refused` when the plan names a local the source has no declaration for, lists no block-scope
    local, or the edit has nothing to move."""
    if any(v not in mapping for v in decl):
        raise Refused("not every local of the plan maps to a declaration in the source")
    block = [v for v in decl if mapping[v].scope > 0]
    if not block:
        raise Refused("the plan lists no block-scope local: the order alone is `--tryvar`'s")
    lines = text.split("\n")
    seen: dict[str, int] = {}
    for loc in regsim.source_locals(text, function):
        seen[loc.name] = seen.get(loc.name, 0) + 1
    duplicated = {n for n, c in seen.items() if c > 1}
    stay = {v: why for v in block for why in [_stays_put(mapping[v], lines, duplicated)] if why}
    moving = [v for v in block if v not in stay]
    if not moving:
        raise Refused("every block-scope local of the plan stays in its block (%s)" % "; ".join(stay.values()))
    fn_scope = [v for v in decl if mapping[v].scope == 0]
    for v in fn_scope:
        why = _stays_put(mapping[v], lines, set())
        if why:
            raise Refused("%s: write the order by hand" % why)
    members = [v for v in decl if v not in stay]
    if fn_scope:
        first = min(mapping[v].line for v in fn_scope)
    else:
        t = cscan.Text(text)
        brace_line = t.line_of(function_span(text, function)[0])
        if not lines[brace_line - 1].rstrip().endswith("{"):
            raise Refused("the function's `{` is not at the end of its line: write the order by hand")
        first = brace_line + 1
    indent = re.match(r"\s*", lines[first - 1]).group() or "    "

    def plain(v: int) -> str:
        loc = mapping[v]
        raw = lines[loc.line - 1].strip()
        return indent + (raw[:raw.index("=")].rstrip() + ";" if loc.init else raw)

    new: dict[int, str | None] = {}
    for v in members:
        loc = mapping[v]
        new[loc.line] = ("%s%s = %s;" % (re.match(r"\s*", lines[loc.line - 1]).group(), loc.name, loc.init)
                         if loc.init else None)
    lo, hi = min(list(new) + [first]), max(list(new) + [first])
    out: list[str] = []
    for n in range(lo, hi + 1):
        if n == first:
            out.append("\n".join(plain(v) for v in members))
        if n not in new:
            out.append(lines[n - 1])
        elif new[n] is not None:
            out.append(new[n])
    old, result = "\n".join(lines[lo - 1:hi]), "\n".join(out)
    if old == result:
        raise Refused("the plan's order is the source's own")
    if text.count(old) != 1:
        raise Refused("the lines %d-%d are not unique in the file" % (lo, hi))
    names = [mapping[v].name for v in moving]
    return Change([(old, result)], None,
                  "hoist %s to function scope and declare %d local(s) in the plan's order%s"
                  % (", ".join("`%s`" % n for n in names), len(members),
                     "; %s stay in their blocks" % ", ".join(sorted(stay_names(stay, mapping))) if stay else ""))


def stay_names(stay: dict, mapping: dict) -> list[str]:
    return ["`%s`" % mapping[v].name for v in stay]
