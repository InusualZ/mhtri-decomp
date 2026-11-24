"""Spell a regsim plan's declaration order into a function's source, semantics preserved, or refuse with the reason.
Spec: docs/tools/spec/lib-regsimorder.md. CLI: none (library; `tools/units/applyorder.py` drives it)."""
from __future__ import annotations

import re
from dataclasses import dataclass

from tools.lib import cscan, localedit, regsim

#: type words of a local that can move across a statement (no constructor to run in place): the project's scalars
SCALARS = frozenset(("u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64", "f32", "f64", "int", "char", "short", "long",
                     "float", "double", "unsigned", "signed", "BOOL", "bool", "const", "register", "volatile"))


class Refused(Exception):
    """A plan the source cannot take as written; `reason` says why (the row's BADLINE text)."""

    def __init__(self, reason: str):
        super().__init__(reason)
        self.reason = reason


class Nothing(Refused):
    """The plan's order is the source's own (no scope changes): the row is SAME."""


class Few(Refused):
    """No scope lists two locals the plan could swap and no block local needs a hoist: the row is FEWFS."""


@dataclass
class Spelling:
    """One way to write a plan: `label` names the strategy, `text` is the whole new (LF) source, `summary` says what moved."""
    label: str
    text: str
    summary: str


# --- the source's locals ------------------------------------------------------------------------------------------

def _keys(locals_: list) -> list[tuple[str, int]]:
    seen: dict[str, int] = {}
    out = []
    for loc in locals_:
        n = seen.get(loc.name, 0)
        seen[loc.name] = n + 1
        out.append((loc.name, n))
    return out


def remap(mapping: dict, old_text: str, new_text: str, function: str) -> dict:
    """`mapping` ({vreg: SourceLocal} of `old_text`) for `new_text`: every local is found again by its name and the
    number of earlier locals of that name, so a line split or a hoisted declaration keeps each vreg on its local."""
    old = regsim.source_locals(old_text, function)
    new = dict(zip(_keys(regsim.source_locals(new_text, function)), regsim.source_locals(new_text, function)))
    keys = dict(zip(range(len(old)), _keys(old)))
    out = {}
    for v, loc in mapping.items():
        if loc not in old:
            raise Refused("the plan's local `%s` is not a declaration of the source as it stands" % loc.name)
        key = keys[old.index(loc)]
        if key not in new:
            raise Refused("`%s` lost its declaration while the source was rewritten" % loc.name)
        out[v] = new[key]
    return out


def _lines(text: str) -> list[str]:
    return text.split("\n")


def _indent(line: str) -> str:
    return line[:len(line) - len(line.lstrip())]


def split_declarators(text: str, function: str, mapping: dict, vregs) -> tuple[str, dict, int]:
    """`text` with every multi-name line (`s32 a, b;`) that declares a local of `vregs` written one name per line
    (`s32 a;` / `s32 b;`: the same declarations in the same order), the remapped `mapping` and how many lines were split."""
    rows = sorted({mapping[v].line for v in vregs if mapping[v].shared > 1}, reverse=True)
    if not rows:
        return text, mapping, 0
    lines = _lines(text)
    for n in rows:
        ds = regsim.declarators(lines[n - 1])
        if len(ds) < 2:
            raise Refused("line %d declares several names that cannot be read apart: write them one per line by hand" % n)
        lines[n - 1:n] = [_indent(lines[n - 1]) + d.text() for d in ds]
    new = "\n".join(lines)
    return new, remap(mapping, text, new, function), len(rows)


def is_scalar(prefix: str) -> bool:
    """True for a pointer or a type spelled with scalar words only (a local that has no constructor to run)."""
    return "*" in prefix or all(w in SCALARS for w in re.findall(r"[A-Za-z_]\w*", prefix))


# --- engine A: each scope's declarations reordered among their own lines -----------------------------------------------

def _scope_groups(decl: list[int], mapping: dict, locs: list) -> list[tuple[int, list[int], list[int]]]:
    """[(scope, old order, plan order)] for the scopes that list at least two locals, in textual order of the old order."""
    out = []
    for sc in sorted({mapping[v].scope for v in decl}):
        new = [v for v in decl if mapping[v].scope == sc]
        if len(new) < 2:
            continue
        old = sorted(new, key=lambda v: locs.index(mapping[v]))
        out.append((sc, old, new))
    return out


def scoped(text: str, function: str, decl: list[int], mapping: dict) -> Spelling:
    """The plan's order inside each scope: the scope's declaration lines are slots and each slot takes the declaration
    the plan puts there. An initialiser makes it an assignment at its own line (the plain declarations go to the first
    slot) when the lines cannot simply be swapped. `Nothing`/`Few` when there is nothing to write, `Refused` when a move
    would change what a name sees, run an initialiser at another time or construct a class local elsewhere."""
    if any(v not in mapping for v in decl):
        raise Refused("not every local of the plan maps to a declaration in the source")
    locs = regsim.source_locals(text, function)
    groups = _scope_groups(decl, mapping, locs)
    if not groups:
        raise Few("no scope lists two locals of the plan")
    changed = [g for g in groups if g[1] != g[2]]
    if not changed:
        raise Nothing("the plan's order is the source's own")
    notes: list[str] = []
    wide = [v for _sc, _old, new in changed for v in new if mapping[v].shared > 1]
    if wide:
        text, mapping, count = split_declarators(text, function, mapping, wide)
        locs = regsim.source_locals(text, function)
        changed = [g for g in _scope_groups(decl, mapping, locs) if g[1] != g[2]]
        notes.append("split %d multi-name line(s)" % count)
    lines = _lines(text)
    edits: dict[int, str | None] = {}
    for sc, old, new in changed:
        fixed = [v for v in old if mapping[v].kind == "for"]
        for v in fixed:
            if old.index(v) != new.index(v):
                raise Refused("`%s` is a `for` header variable and the plan declares it elsewhere: it needs a hoist"
                              % mapping[v].name)
        old_rest, new_rest = [v for v in old if v not in fixed], [v for v in new if v not in fixed]
        if old_rest == new_rest:
            continue
        edits.update(_scope_edits(lines, sc, old_rest, new_rest, mapping, locs))
        notes.append("scope %d: %s" % (sc, " ".join(mapping[v].name for v in new_rest)))
    out = []
    for n, line in enumerate(lines, 1):
        if n in edits:
            if edits[n] is not None:
                out.append(edits[n])
        else:
            out.append(line)
    return Spelling("scoped", "\n".join(out), "; ".join(notes))


def _scope_edits(lines: list[str], scope: int, old: list[int], new: list[int], mapping: dict,
                 locs: list) -> dict[int, str | None]:
    """`{line number: new text (None: the line goes)}` for one scope's reorder; picks the line swap when it is safe,
    else the plain declarations at the first slot with the initialisers left as assignments."""
    slots = sorted(mapping[v].line for v in old)
    first, last = slots[0], slots[-1]
    between = [n for n in range(first, last + 1) if n not in slots and lines[n - 1].strip()]
    names = {mapping[v].name: v for v in old}
    if between and any(not is_scalar(_prefix(lines[mapping[v].line - 1])) for v in old if old.index(v) != new.index(v)):
        raise Refused("a class-typed local would move across a statement (lines %d-%d)" % (first, last))
    why = _swap_refusal(lines, old, new, mapping, names, slots, between)
    if not why:
        return {slot: lines[mapping[v].line - 1] for slot, v in zip(slots, new)}
    return _plain_edits(lines, old, new, mapping, slots, why)


def _prefix(line: str) -> str:
    ds = regsim.declarators(line)
    return ds[0].prefix if ds else ""


def _swap_refusal(lines, old, new, mapping, names, slots, between) -> str:
    """Why the declaration lines cannot just trade places ('' when they can)."""
    rank = {v: i for i, v in enumerate(new)}
    for v in old:
        init = mapping[v].init
        if not init:
            continue
        for name, w in names.items():
            if w != v and re.search(r"\b%s\b" % re.escape(name), init) and rank[w] > rank[v]:
                return "the initialiser of `%s` uses `%s`, which the plan declares after it" % (mapping[v].name, name)
    inits = [v for v in old if mapping[v].init]
    if inits:
        if [v for v in new if v in inits] != inits:
            return "the plan swaps two initialised locals, which would swap their calls"
        if between:
            return "an initialised local would move across the statement at line %d" % between[0]
    for v in old:
        moved_to = slots[new.index(v)]
        here = mapping[v].line
        if moved_to > here and any(re.search(r"\b%s\b" % re.escape(mapping[v].name), lines[n - 1])
                                   for n in between if here < n < moved_to):
            return "`%s` is used before the line the plan moves its declaration to" % mapping[v].name
    return ""


def _plain_edits(lines, old, new, mapping, slots, why) -> dict[int, str | None]:
    """All the plain declarations at the first slot in the plan's order, each initialiser an assignment at its own line."""
    first = slots[0]
    indent = _indent(lines[first - 1])
    plains = []
    out: dict[int, str | None] = {}
    for v in new:
        raw = lines[mapping[v].line - 1]
        if mapping[v].init:
            if raw.strip().startswith("const "):
                raise Refused("`%s` is a `const` with an initialiser and %s" % (mapping[v].name, why))
            if mapping[v].init.startswith("{"):
                raise Refused("`%s` has a brace initialiser and %s" % (mapping[v].name, why))
            if not is_scalar(_prefix(raw)):
                raise Refused("`%s` has a class type: splitting its initialiser changes its construction (%s)"
                              % (mapping[v].name, why))
        ds = regsim.declarators(raw)
        if len(ds) != 1:
            raise Refused("line %d is not a one-name declaration" % mapping[v].line)
        plains.append(indent + ds[0].text(plain=True))
    for v in old:
        n = mapping[v].line
        out[n] = (_indent(lines[n - 1]) + "%s = %s;" % (mapping[v].name, mapping[v].init)) if mapping[v].init else None
    head = "\n".join(plains)
    out[first] = head + ("\n" + out[first] if out[first] is not None else "")
    return out


# --- engines B, C: block-scope locals declared at function scope in the plan's order (idea 324) --------------------

def hoisted(text: str, function: str, decl: list[int], mapping: dict, with_for: bool = False) -> Spelling:
    """Every local the plan names declared at function scope, in the plan's order (`localedit.hoist_order`: an
    initialiser stays at its place as an assignment, a name declared twice and a `const` initialised stay in their
    block). `with_for` hoists the `for` header variables as well (`for (u16 i = 0;` becomes `for (i = 0;` with `u16 i;`
    declared up top) when each one's name is unique in the function."""
    if any(v not in mapping for v in decl):
        raise Refused("not every local of the plan maps to a declaration in the source")
    notes: list[str] = []
    if with_for:
        text, mapping, n = _hoist_for_headers(text, function, decl, mapping)
        notes.append("hoisted %d `for` variable(s)" % n)
    wide = [v for v in decl if mapping[v].shared > 1]
    if wide:
        text, mapping, count = split_declarators(text, function, mapping, wide)
        notes.append("split %d multi-name line(s)" % count)
    try:
        change = localedit.hoist_order(decl, mapping, text, function)
    except localedit.Refused as exc:
        raise Refused(exc.reason) from None
    for old, new in change.blocks:
        text = text.replace(old, new, 1)
    return Spelling("hoist+for" if with_for else "hoist", text, "; ".join([change.summary] + notes))


def _hoist_for_headers(text: str, function: str, decl: list[int], mapping: dict) -> tuple[str, dict, int]:
    fors = [v for v in decl if mapping[v].kind == "for"]
    if not fors:
        raise Refused("the plan names no `for` header variable")
    names = [loc.name for loc in regsim.source_locals(text, function)]
    d = regsim.find_definition(text, function)
    if d is None:
        raise Refused("the function is not defined in the source")
    t = cscan.Text(text)
    open_pos, close_pos = d.body
    body = t.code[open_pos:close_pos]
    brace_line = t.line_of(open_pos)
    lines = _lines(text)
    if not lines[brace_line - 1].rstrip().endswith("{"):
        raise Refused("the function's `{` is not at the end of its line: write the order by hand")
    edits, decls = {}, []
    for v in fors:
        name = mapping[v].name
        if names.count(name) != 1:
            raise Refused("`%s` is declared more than once in the function: its `for` header stays" % name)
        pat = re.compile(r"\bfor\s*\(\s*(?P<type>[A-Za-z_][\w:]*(?:\s+[\w:]+)*?)(?P<sep>\s+\**\s*|\s*\*+\s*)%s\s*=" %
                         re.escape(name))
        m = pat.search(body)
        if not m or m.group("type").split()[0] == "const" or "const" in m.group("type").split():
            raise Refused("the header of `%s` cannot be split from its type (`const` or not a plain `T %s =`)" % (name, name))
        init_text = body[m.end():body.find(";", m.end())]
        if regsim.split_top_level(init_text) != [init_text]:
            raise Refused("the `for` header of `%s` declares several names" % name)
        start = open_pos + m.start()
        edits[start] = (open_pos + m.end(), "for (%s =" % name)
        decls.append("%s%s%s;" % (m.group("type"), m.group("sep"), name))
    out = text
    for start in sorted(edits, reverse=True):
        end, repl = edits[start]
        out = out[:start] + repl + out[end:]
    lines = _lines(out)
    indent = _indent(lines[brace_line]) if len(lines) > brace_line and lines[brace_line].strip() else "    "
    lines[brace_line:brace_line] = [indent + x for x in decls]
    new = "\n".join(lines)
    return new, remap(mapping, text, new, function), len(decls)


# --- the candidates of one plan -----------------------------------------------------------------------------------

def spellings(decl: list[int], mapping: dict, text: str, function: str, edits=()) -> tuple[list[Spelling], list[tuple[str, Refused]]]:
    """Every distinct way to write the plan's order: the per-scope reorder, the function-scope hoist of its block
    locals and the hoist that takes the `for` variables along. A vreg of the plan that is no declared local (the new
    local an edit asks for) is left out, and so is the plan's own `edits`: the order of the others is what gets written
    (the summary says so). -> (spellings, [(strategy, why not)])."""
    skipped = [v for v in decl if v not in mapping]
    decl = [v for v in decl if v in mapping]
    found: list[Spelling] = []
    refused: list[tuple[str, Refused]] = []
    tries = [("scoped", lambda: scoped(text, function, decl, mapping))]
    if any(mapping[v].scope > 0 for v in decl):
        tries.append(("hoist", lambda: hoisted(text, function, decl, mapping)))
    if any(mapping[v].kind == "for" for v in decl):
        tries.append(("hoist+for", lambda: hoisted(text, function, decl, mapping, True)))
    for label, make in tries:
        try:
            s = make()
        except Refused as exc:
            refused.append((label, exc))
            continue
        if skipped or edits:
            s.summary += "; the plan's edit (%s) is not written" % ", ".join(
                [e.split(" as ")[0] for e in edits] or ["r%d" % v for v in skipped])
        if s.text != text and all(s.text != f.text for f in found):
            found.append(s)
    return found, refused
