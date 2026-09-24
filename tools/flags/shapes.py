#!/usr/bin/env python3
"""Source-shape generators for `shapesearch.py` - the source-side analogue of `variants/<lib>.py`.

A *shape* is a rewrite of one function's body that leaves the function's meaning alone (or is at worst a
compile error) but changes the IR the allocator and the peephole see. The levers here are the ones the
matching playbook names: declaration order and types (rows 18, 20, 38), named temporaries, casts and
signedness, statement order, compound assignment vs assignment, field form vs pointer arithmetic, dead
copies (row 35), the switch tail and `default`-first shapes (rows 34, 37), condition/branch form,
ternaries, and the loop shape (row 19).

Nothing in here compiles or scores anything: every generator is a pure `body -> [(name, new_body)]`
function over the text between a function's braces, so it can be unit-tested without a compiler. The
driver (`shapesearch.py`) owns the compile/score/dedupe loop.

The transforms are deliberately textual, not a C parser: they must never silently mangle the source into
something the compiler accepts but that means something else, so each one only fires on a narrow pattern
and a rewrite that does not apply yields no variant at all. A generator that produces an uncompilable
variant is not a bug (the driver reports the compile failure and moves on); a generator that produces a
*semantically different* variant is, so the aggressive rewrites (`break` -> `return C`) are opt-in by
generator name.
"""
import re
from dataclasses import dataclass

# --------------------------------------------------------------------------------------------------
# lexical helpers: strings, chars, comments, brace matching, statement splitting
# --------------------------------------------------------------------------------------------------

_IDENT = re.compile(r"[A-Za-z_]\w*")
_CONTROL = {
    "if", "else", "for", "while", "do", "switch", "case", "default", "return", "break", "continue",
    "goto", "sizeof", "typedef", "struct", "union", "enum",
}
_SCALAR_TYPES = ["s8", "s16", "s32", "s64", "u8", "u16", "u32", "u64", "int", "unsigned int", "long",
                 "char", "short", "f32", "f64", "float", "double"]


def _skip_quote(src, i):
    """`src[i]` is a quote; return the index just past the closing quote (or len)."""
    q = src[i]
    i += 1
    while i < len(src):
        if src[i] == "\\":
            i += 2
            continue
        if src[i] == q:
            return i + 1
        i += 1
    return len(src)


def _skip_comment(src, i):
    """`src[i:i+2]` starts a comment; return the index just past it."""
    if src.startswith("//", i):
        j = src.find("\n", i)
        return len(src) if j < 0 else j
    j = src.find("*/", i + 2)
    return len(src) if j < 0 else j + 2


def strip_comments(text):
    """Remove comments, preserving everything else (used for normalised source dedupe)."""
    out = []
    i = 0
    while i < len(text):
        c = text[i]
        if c in "\"'":
            j = _skip_quote(text, i)
            out.append(text[i:j])
            i = j
        elif text.startswith("/*", i) or text.startswith("//", i):
            i = _skip_comment(text, i)
        else:
            out.append(c)
            i += 1
    return "".join(out)


def norm_code(text):
    """Comment- and whitespace-insensitive form of a source text, for cheap candidate dedupe.

    Two variants that differ only in comments or spacing compile to the same object, so collapsing them
    here saves a compile each.
    """
    return re.sub(r"\s+", " ", strip_comments(text)).strip()


def match_brace(src, i):
    """`src[i] == '{'`; return the index of the matching '}', or None."""
    depth = 0
    while i < len(src):
        c = src[i]
        if c in "\"'":
            i = _skip_quote(src, i)
            continue
        if src.startswith("/*", i) or src.startswith("//", i):
            i = _skip_comment(src, i)
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


def find_definition(src, name):
    """Locate the definition of `name`: return (start, open_idx, close_idx) or None.

    A forward declaration ends in `;` before any `{`, so it is skipped. The first definition wins. A
    mangled map name (`Pl_bari_ck__FP4_PLWl`) is also tried as its unmangled prefix, because the source
    spells the C++ definition `Pl_bari_ck` while the symbol map carries the mangling.
    """
    cands = [name]
    for sep in ("__F", "__"):
        if sep in name and name.split(sep)[0] not in cands:
            cands.append(name.split(sep)[0])
    for cand in cands:
        pat = re.compile(r"(?<![\w.>])" + re.escape(cand) + r"\s*\(")
        for m in pat.finditer(src):
            i = m.end() - 1                      # at the '('
            # walk to the matching ')'
            depth = 0
            while i < len(src):
                c = src[i]
                if c in "\"'":
                    i = _skip_quote(src, i)
                    continue
                if src.startswith("/*", i) or src.startswith("//", i):
                    i = _skip_comment(src, i)
                    continue
                if c == "(":
                    depth += 1
                elif c == ")":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            if i >= len(src):
                continue
            # skip anything between ')' and the body ('const', 'noexcept', whitespace, comments)
            j = i + 1
            while j < len(src):
                if src.startswith("/*", j) or src.startswith("//", j):
                    j = _skip_comment(src, j)
                    continue
                if src[j].isspace():
                    j += 1
                    continue
                break
            if j < len(src) and src[j] == "{":
                close = match_brace(src, j)
                if close is not None:
                    return m.start(), j, close
    return None


@dataclass
class Statement:
    start: int   # offset within the body text
    end: int     # offset just past the statement
    text: str

    def stripped(self):
        return strip_comments(self.text).strip()

    def is_decl(self):
        s = self.stripped()
        if not s or s[0] in "{}":
            return False
        first = _IDENT.match(s)
        if first and first.group(0) in _CONTROL:
            return False
        # a declaration has a type-ish head then an identifier, and no '(' before the '=' or ';'
        head = re.match(r"^(?:(?:const|volatile|static|register|unsigned|signed|struct|class)\s+)*"
                        r"(?:[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*\s*\*+\s*)?"
                        r"[A-Za-z_]\w*\s*(?:\[[^\]]*\])?\s*[A-Za-z_]\w*\s*(?:=|;|,|\[)", s)
        if not head:
            return False
        return "(" not in head.group(0).split("=")[0]

    def is_simple(self):
        """A statement that is not a control-flow block - safe to reorder with a disjoint sibling."""
        s = self.stripped()
        first = _IDENT.match(s)
        return not (first and first.group(0) in _CONTROL)


def split_statements(body):
    """Top-level statements of a function body, with their offsets in `body`.

    Splits on `;` at bracket depth 0 and on a `}` that returns to depth 0 when what follows is not a
    `;`/`,`/`)` (i.e. an `if (...) { ... }` with no trailing semicolon). Comments and whitespace attach
    to the statement that follows them.
    """
    out = []
    i = 0
    start = 0
    depth = 0
    n = len(body)
    while i < n:
        c = body[i]
        if c in "\"'":
            i = _skip_quote(body, i)
            continue
        if body.startswith("/*", i) or body.startswith("//", i):
            i = _skip_comment(body, i)
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0 and c == "}":
                j = i + 1
                while j < n and body[j].isspace():
                    j += 1
                if j >= n or body[j] not in ";,)]}":
                    out.append(Statement(start, i + 1, body[start:i + 1]))
                    start = j
        elif c == ";" and depth == 0:
            out.append(Statement(start, i + 1, body[start:i + 1]))
            start = i + 1
        i += 1
    if start < n and body[start:].strip():
        out.append(Statement(start, n, body[start:]))
    return out


def _decl_map(body):
    """{variable: type} for the top-level declarations of a body (best effort)."""
    out = {}
    for st in split_statements(body):
        if not st.is_decl():
            continue
        s = st.stripped().rstrip(";")
        m = re.match(r"^(?:(?:const|volatile|static|register)\s+)*"
                     r"((?:unsigned\s+|signed\s+)?[A-Za-z_]\w*(?:\s*\*+)?)\s+"
                     r"([A-Za-z_]\w*)", s)
        if m:
            out[m.group(2)] = m.group(1).strip()
    return out


def _leading_decls(statements):
    """Indices of the leading run of declaration statements."""
    idx = []
    for i, st in enumerate(statements):
        if st.is_decl():
            idx.append(i)
        elif st.stripped():
            break
    return idx


# --------------------------------------------------------------------------------------------------
# generators: each is body -> [(name, new_body)]
# --------------------------------------------------------------------------------------------------

def _gen_decl_order(body, limit=60):
    """Swap adjacent leading declarations, and rotate the whole run (playbook 18)."""
    sts = split_statements(body)
    decls = _leading_decls(sts)
    out = []
    for a, b in zip(decls, decls[1:]):
        if len(out) >= limit:
            break
        sa, sb = sts[a], sts[b]
        if a + 1 != b:
            continue
        new = body[:sa.start] + sb.text + sa.text + body[sb.end:]
        out.append(("decl_swap_%d_%d" % (a, b), new))
    if len(decls) >= 2:
        order = decls
        texts = [sts[i].text for i in order]
        lo, hi = sts[order[0]].start, sts[order[-1]].end
        for name, rot in (("decl_rotate_fwd", texts[1:] + texts[:1]),
                          ("decl_rotate_back", texts[-1:] + texts[:-1])):
            out.append((name, body[:lo] + "".join(rot) + body[hi:]))
    if len(decls) >= 2:
        # move each declaration to the front / back of the run
        for k, i in enumerate(decls):
            if len(out) >= limit:
                break
            texts = [sts[j].text for j in decls]
            lo, hi = sts[decls[0]].start, sts[decls[-1]].end
            if k:
                out.append(("decl_front_%d" % i, body[:lo] + texts[k] + "".join(
                    texts[:k] + texts[k + 1:]) + body[hi:]))
            if k != len(decls) - 1:
                out.append(("decl_back_%d" % i, body[:lo] + "".join(
                    texts[:k] + texts[k + 1:]) + texts[k] + body[hi:]))
    return out


_TYPE_RE = re.compile(r"^(?P<pre>\s*(?:(?:const|volatile|static|register)\s+)*)"
                      r"(?P<type>(?:unsigned\s+|signed\s+)?[A-Za-z_]\w*(?:\s*\*+)?)\s+"
                      r"(?P<name>[A-Za-z_]\w*)(?P<rest>\s*(?:\[[^\]]*\])?\s*(?:=|;|,|\[))")
_ALTS = {
    "s8": ["s16", "s32", "u8", "u16", "u32"],
    "s16": ["s32", "s8", "u16", "u32", "s64"],
    "s32": ["u32", "s16", "s8", "s64"],
    "u8": ["s8", "s16", "u16", "u32", "s32"],
    "u16": ["s16", "u32", "u8", "s32"],
    "u32": ["s32", "u16", "s8"],
    "int": ["unsigned int", "short", "long"],
    "unsigned int": ["int", "u32", "s32"],
    "char": ["u8", "s8", "s16"],
    "short": ["s16", "int", "s32"],
    "long": ["int", "s32", "s64"],
}


def _gen_decl_type(body, limit=200):
    """Change a local's declared type, and toggle `volatile` (rows 18, 38, 20)."""
    sts = split_statements(body)
    out = []
    for i in _leading_decls(sts):
        st = sts[i]
        m = _TYPE_RE.match(st.stripped())
        if not m:
            continue
        pre, typ, name = m.group("pre"), m.group("type"), m.group("name")
        idx = st.text.find(typ)
        if idx < 0:
            continue
        for alt in _ALTS.get(typ, []):
            if len(out) >= limit:
                return out
            new_stmt = st.text[:idx] + alt + st.text[idx + len(typ):]
            out.append(("decl_type_%s_%s" % (name, alt.replace(" ", "_")),
                        body[:st.start] + new_stmt + body[st.end:]))
        if "volatile" not in pre:
            out.append(("decl_volatile_%s" % name,
                        body[:st.start] + st.text[:idx] + "volatile " + st.text[idx:] + body[st.end:]))
        else:
            vi = st.text.find("volatile")
            out.append(("decl_unvolatile_%s" % name,
                        body[:st.start] + st.text[:vi] + st.text[vi + len("volatile "):] + body[st.end:]))
    return out


_CAST_RE = re.compile(r"\((s8|s16|s32|s64|u8|u16|u32|u64|int|unsigned int|char|short)\)")
_PTRCAST_RE = re.compile(r"\((u8|s8|u16|s16|u32|s32)\s*\*\)")


def _gen_casts(body, limit=120):
    """Signedness / width of casts, and add/remove integer casts (rows 18, 38)."""
    out = []
    seen = set()
    for m in _CAST_RE.finditer(body):
        if len(out) >= limit:
            break
        old = m.group(1)
        for alt in _ALTS.get(old, []):
            key = (m.start(), alt)
            if key in seen:
                continue
            seen.add(key)
            out.append(("cast_%d_%s_to_%s" % (m.start(), old, alt.replace(" ", "_")),
                        body[:m.start()] + "(" + alt + ")" + body[m.end():]))
        # drop the cast
        out.append(("cast_%d_remove" % m.start(), body[:m.start()] + body[m.end():]))
    for m in _PTRCAST_RE.finditer(body):
        if len(out) >= limit:
            break
        old = m.group(1)
        alt = {"u8": "s8", "s8": "u8", "u16": "s16", "s16": "u16", "u32": "s32", "s32": "u32"}[old]
        out.append(("ptrcast_%d_%s_to_%s" % (m.start(), old, alt),
                    body[:m.start()] + "(" + alt + " *)" + body[m.end():]))
    return out


def _idents(text):
    return {m.group(0) for m in _IDENT.finditer(strip_comments(text))} - _CONTROL


def _gen_stmt_order(body, limit=120):
    """Swap adjacent independent top-level statements (declaration and body statements alike)."""
    sts = split_statements(body)
    out = []
    for a in range(len(sts) - 1):
        if len(out) >= limit:
            break
        sa, sb = sts[a], sts[a + 1]
        if not (sa.is_simple() and sb.is_simple()):
            continue
        if not sa.stripped() or not sb.stripped():
            continue
        if _idents(sa.text) & _idents(sb.text):
            continue                      # dependent - swapping would change meaning
        new = body[:sa.start] + sb.text + sa.text + body[sb.end:]
        out.append(("stmt_swap_%d_%d" % (a, a + 1), new))
    return out


_COMPOUND_SITE = [
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*\+\+\s*;"),
     ["{n} += 1;", "{n} = {n} + 1;"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*--\s*;"),
     ["{n} -= 1;", "{n} = {n} - 1;"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*\+=\s*1\s*;"),
     ["{n}++;", "{n} = {n} + 1;"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*-=\s*1\s*;"),
     ["{n}--;", "{n} = {n} - 1;"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*=\s*(?P=n)\s*\+\s*(?P<r>.+?);"),
     ["{n} += {r};"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*=\s*(?P=n)\s*-\s*(?P<r>.+?);"),
     ["{n} -= {r};"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*=\s*(?P=n)\s*(?P<o>[*/%&|^])\s*(?P<r>.+?);"),
     ["{n} {o}= {r};"]),
    (re.compile(r"\b(?P<n>[A-Za-z_]\w*)\s*(?P<o>[+\-*/%&|^])=\s*(?P<r>.+?);"),
     ["{n} = {n} {o} {r};"]),
]


def _gen_compound(body, limit=120):
    """Compound assignment vs assignment vs `++`/`--`, at every site in the body (row 18).

    Scans the whole body rather than the top-level statement list, because the lever usually matters on a
    statement nested in a loop or an `if` (`n++;` inside the popcount loop is the shape the campaign hit).
    One variant per site, so a body with ten sites costs ten compiles.
    """
    out = []
    seen = set()
    for rx, forms in _COMPOUND_SITE:
        for m in rx.finditer(body):
            if len(out) >= limit:
                return out
            for form in forms:
                new_text = form.format(**{k: (v or "") for k, v in m.groupdict().items()})
                key = (m.start(), new_text)
                if key in seen:
                    continue
                seen.add(key)
                out.append(("compound_%d_%s" % (m.start(), re.sub(r"\W", "", new_text)[:14]),
                            body[:m.start()] + new_text + body[m.end():]))
    return out


def _split_assignment(text):
    """(head, lhs, rhs) for `head lhs = rhs;` at the top level of a statement, or None."""
    depth = 0
    i = 0
    eq = -1
    while i < len(text):
        c = text[i]
        if c in "\"'":
            i = _skip_quote(text, i)
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "=" and depth == 0 and not (i and text[i - 1] in "=!<>+-*/%&|^") \
                and not text.startswith("==", i):
            eq = i
            break
        i += 1
    if eq < 0:
        return None
    head, lhs, rhs = text[:eq], text[:eq], text[eq + 1:]
    semi = rhs.rstrip()
    if not semi.endswith(";"):
        return None
    rhs = semi[:-1]
    m = re.match(r"^(.*?)([A-Za-z_]\w*)\s*$", lhs, re.S)
    if not m:
        return None
    return text[:m.start()], m.group(2), rhs


def _gen_temps(body, limit=120):
    """Named temporaries: hoist a statement's RHS, or one call argument, into a fresh local (row 18)."""
    sts = split_statements(body)
    decls = _decl_map(body)
    out = []
    for i, st in enumerate(sts):
        if len(out) >= limit:
            break
        s = st.text
        m = re.match(r"^(\s*)return\s+(.+);\s*$", s, re.S)
        if m:
            rhs = m.group(2)
            if re.search(r"[+\-*/%&|^<>]", rhs) or rhs.count("(") > 1:
                out.append(("temp_ret_%d" % i,
                            body[:st.start] + "%ss32 t%d = %s;\n%sreturn t%d;" % (m.group(1), i, rhs, m.group(1), i) + body[st.end:]))
            continue
        parts = _split_assignment(s)
        if not parts:
            continue
        head, lhs, rhs = parts
        if len(rhs.strip()) < 3 or not (re.search(r"[+\-*/%&|^<>?]", rhs) or "(" in rhs):
            continue
        if st.is_decl():
            # `T x = e;` -> `T x; T t = e; x = t;` (head already carries the type)
            pre = "%s%s;\n" % (head, lhs)
            decl = "%s%s t%d = %s;\n" % (head, "", i, rhs)
            asn = "%s%s = t%d;" % (head, lhs, i)
        else:
            typ = decls.get(lhs, "s32")
            pre = ""
            decl = "%s%s t%d = %s;\n" % (head, typ, i, rhs)
            asn = "%s%s = t%d;" % (head, lhs, i)
        out.append(("temp_rhs_%d" % i, body[:st.start] + pre + decl + asn + body[st.end:]))
        # hoist each parenthesised subexpression
        for k, pm in enumerate(re.finditer(r"\(([^()]*)\)", rhs)):
            if len(out) >= limit:
                break
            if not re.search(r"[+\-*/%&|^<>]", pm.group(1)):
                continue
            inner = pm.group(1)
            new_rhs = rhs[:pm.start()] + ("t%d_%d" % (i, k)) + rhs[pm.end():]
            typ2 = decls.get(lhs, "s32")
            sub = "%s%s t%d_%d = %s;\n" % (head if st.is_decl() else head, typ2 if not st.is_decl() else "", i, k, inner)
            out.append(("temp_sub_%d_%d" % (i, k),
                        body[:st.start] + sub + "%s%s = %s;" % (head, lhs, new_rhs) + body[st.end:]))
    return out


_ARROW_RE = re.compile(r"([A-Za-z_][\w.\[\]()]*)\s*->\s*([A-Za-z_]\w*)")
_DOTPTR_RE = re.compile(r"\(\s*\*\s*([A-Za-z_][\w\[\]()]*)\s*\)\s*\.\s*([A-Za-z_]\w*)")


def _gen_field_form(body, limit=80):
    """Field form: `p->f` <-> `(*p).f` (row 20's address-shape lever)."""
    out = []
    for m in list(_ARROW_RE.finditer(body))[:limit]:
        out.append(("field_dot_%d" % m.start(), body[:m.start()] + "(*%s).%s" % (m.group(1), m.group(2)) + body[m.end():]))
    for m in list(_DOTPTR_RE.finditer(body))[:limit]:
        out.append(("field_arrow_%d" % m.start(), body[:m.start()] + "%s->%s" % (m.group(1), m.group(2)) + body[m.end():]))
    return out


def _gen_dead_copy(body, limit=80):
    """Insert dead copies of a live local, to steer the allocator's web priority (row 35)."""
    sts = split_statements(body)
    decls = _decl_map(body)
    if not decls:
        return []
    first_body = next((i for i, st in enumerate(sts) if not st.is_decl() and st.stripped()), None)
    if first_body is None:
        return []
    out = []
    for var, typ in decls.items():
        if "*" in typ:
            continue
        st = sts[first_body]
        ind = re.match(r"\s*", st.text).group(0)
        for tag, decl in (("plain", "%s%s dc_%s = %s;" % (ind, typ, var, var)),
                          ("vol", "%svolatile %s dc_%s = %s;" % (ind, typ, var, var))):
            ins = "%s\n%s(void)dc_%s;\n" % (decl, ind, var)
            out.append(("deadcopy_%s_%s" % (tag, var), body[:st.start] + ins + body[st.start:]))
        # a chain of two dead copies of the competing value
        ins2 = ("%s%s dc_%s = %s;\n%s%s dc2_%s = dc_%s;\n%s(void)dc2_%s;\n"
                % (ind, typ, var, var, ind, typ, var, var, ind, var))
        out.append(("deadcopy_chain_%s" % var, body[:st.start] + ins2 + body[st.start:]))
    return out


_SWITCH_RE = re.compile(r"\bswitch\s*\(")
_LABEL_RE = re.compile(r"\b(default|case\b[^:]*)\s*:")


def _switch_arms(body, switch_at):
    """(open, close, [(label_start, body_start, body_end, label_text)]) for a switch at `switch_at`."""
    i = body.find("{", switch_at)
    if i < 0:
        return None
    close = match_brace(body, i)
    if close is None:
        return None
    inner = body[i + 1:close]
    labels = []
    for m in _LABEL_RE.finditer(inner):
        labels.append([m.start(), m.end(), m.group(1)])
    if not labels:
        return None
    arms = []
    for k, (ls, le, lab) in enumerate(labels):
        end = labels[k + 1][0] if k + 1 < len(labels) else len(inner)
        arms.append((ls, le, end, lab))
    return i, close, arms


def _gen_switch(body, limit=120):
    """Switch shapes: `default` first (row 37), arm order, `break`->`return C` (row 34).

    The `break` -> `return C` rewrite is included here because it is the switch-tail lever, but it is
    only generated for a constant the function already returns, and the driver labels the variant
    `switch_*` so it can be excluded wholesale.
    """
    out = []
    for m in list(_SWITCH_RE.finditer(body))[:8]:
        got = _switch_arms(body, m.start())
        if not got:
            continue
        open_i, close_i, arms = got
        if len(arms) > 5:
            continue
        base = body[open_i + 1:close_i]
        arm_texts = [base[a[0]:a[2]] for a in arms]
        # default first / last
        di = next((k for k, a in enumerate(arms) if a[3].startswith("default")), None)
        if di is not None and di != 0:
            order = [di] + [k for k in range(len(arms)) if k != di]
            out.append(("switch_default_first_%d" % m.start(),
                        body[:open_i + 1] + "".join(arm_texts[k] for k in order) + body[close_i:]))
        if di is not None and di != len(arms) - 1:
            order = [k for k in range(len(arms)) if k != di] + [di]
            out.append(("switch_default_last_%d" % m.start(),
                        body[:open_i + 1] + "".join(arm_texts[k] for k in order) + body[close_i:]))
        # swap adjacent non-default arms
        for k in range(len(arms) - 1):
            if arms[k][3].startswith("default") or arms[k + 1][3].startswith("default"):
                continue
            order = list(range(len(arms)))
            order[k], order[k + 1] = order[k + 1], order[k]
            out.append(("switch_swap_%d_%d" % (k, k + 1),
                        body[:open_i + 1] + "".join(arm_texts[j] for j in order) + body[close_i:]))
        # break -> return C, for every constant return the function already has
        consts = sorted({m2.group(1) for m2 in re.finditer(r"\breturn\s+(-?\d+)\s*;", body)})
        for k, (ls, le, end, lab) in enumerate(arms):
            arm = base[ls:end]
            br = re.search(r"\bbreak\s*;", arm)
            if not br:
                continue
            for c in consts:
                if len(out) >= limit:
                    break
                new_arm = arm[:br.start()] + ("return %s;" % c) + arm[br.end():]
                new_base = base[:ls] + new_arm + base[end:]
                out.append(("switch_break_return_%s_arm%d" % (c, k),
                            body[:open_i + 1] + new_base + body[close_i:]))
    return out


_CMP_FLIP = [(">=", "<="), ("<=", ">="), (">", "<"), ("<", ">"), ("==", "=="), ("!=", "!=")]


def _gen_cond(body, limit=150):
    """Condition shape: negate/swap branches, flip comparison sides, bool forms (rows 34, 37)."""
    out = []
    for m in list(re.finditer(r"\bif\s*\((.*?)\)\s*\{", body, re.S))[:40]:
        cond = m.group(1)
        if len(cond) > 120:
            continue
        neg = _negate_cond(cond)
        if neg:
            out.append(("cond_negate_%d" % m.start(), body[:m.start()] + "if (%s) {" % neg + body[m.end():]))
        flip = _flip_cmp(cond)
        if flip:
            out.append(("cond_flip_%d" % m.start(), body[:m.start()] + "if (%s) {" % flip + body[m.end():]))
        if len(out) >= limit:
            break
    return out


def _negate_cond(cond):
    c = cond.strip()
    if c.startswith("!"):
        return c[1:].strip()
    m = re.match(r"^(.*?)\s*!=\s*(0|0x0)\s*$", c)
    if m:
        return m.group(1).strip()
    m = re.match(r"^(.*?)\s*==\s*(0|0x0)\s*$", c)
    if m:
        return "!" + m.group(1).strip()
    return "!(" + c + ")"


def _flip_cmp(cond):
    for a, b in _CMP_FLIP:
        if a in (">", "<", ">=", "<=") and a in cond:
            parts = cond.split(a, 1)
            if len(parts) == 2:
                return "%s %s %s" % (parts[1].strip(), b, parts[0].strip())
    return None


def _gen_ternary(body, limit=80):
    """Ternary / branchless-threshold shapes (`x > N ? 2 : 1` family, from the pl_act header)."""
    out = []
    for m in list(re.finditer(r"([^?;{}()]+?)\?([^:;{}]+):([^;{}]+)", body))[:limit]:
        c, a, b = m.group(1).strip(), m.group(2).strip(), m.group(3).strip()
        if not c:
            continue
        out.append(("tern_swap_%d" % m.start(), body[:m.start()] + "(%s) ? (%s) : (%s)" % (c, a, b) + body[m.end():]))
        out.append(("tern_neg_%d" % m.start(), body[:m.start()] + "(%s) ? (%s) : (%s)" % (_negate_cond(c), b, a) + body[m.end():]))
    return out


def _gen_loop(body, limit=40):
    """Loop shape: `do { } while (0)`, `for(;;)` vs `while(1)`, prefix vs postfix (row 19)."""
    out = []
    if re.search(r"\bdo\s*\{", body):
        # do { B } while (0);  ->  { B }
        m = re.search(r"\bdo\s*\{", body)
        close = match_brace(body, body.index("{", m.start()))
        if close is not None:
            tail = re.match(r"\s*while\s*\(\s*0\s*\)\s*;", body[close + 1:])
            if tail:
                inner = body[body.index("{", m.start()) + 1:close]
                if "break" not in inner:
                    out.append(("loop_unroll_do", body[:m.start()] + "{" + inner + "}" + body[close + 1 + tail.end():]))
    for m in list(re.finditer(r"\bfor\s*\(\s*;\s*;\s*\)", body))[:4]:
        out.append(("loop_for_to_while_%d" % m.start(), body[:m.start()] + "while (1)" + body[m.end():]))
    for m in list(re.finditer(r"\bwhile\s*\(\s*1\s*\)", body))[:4]:
        out.append(("loop_while_to_for_%d" % m.start(), body[:m.start()] + "for (;;)" + body[m.end():]))
    for m in list(re.finditer(r"\b([A-Za-z_]\w*)\+\+", body))[:20]:
        out.append(("loop_prefix_%d" % m.start(), body[:m.start()] + "++" + m.group(1) + body[m.end():]))
    return out


def _paren_match(text, i):
    """`text[i] == '('`; return the index of the matching ')', or None."""
    depth = 0
    while i < len(text):
        c = text[i]
        if c in "\"'":
            i = _skip_quote(text, i)
            continue
        if text.startswith("/*", i) or text.startswith("//", i):
            i = _skip_comment(text, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


_FOR_DECL_RE = re.compile(r"\bfor\s*\(\s*(?:(?:const|unsigned|signed)\s+)*"
                          r"(?P<type>s8|s16|s32|s64|u8|u16|u32|u64|int|char|short|long|f32|f64)\s+"
                          r"(?P<name>[A-Za-z_]\w*)\s*=\s*(?P<init>[^;]*);")


def _gen_loop_decl(body, limit=60):
    """Hoist a `for`-loop declaration into a separate declaration statement (row 18's web order).

    `for (s32 i = 0; ...)` creates `i`'s live range inside the loop; a separate `s32 i;` before the loop
    (or at the top of the body, where `decl_order` can reorder it against the other locals) creates it in
    a different order, which is enough to flip the allocator's register pair.
    """
    out = []
    for m in list(_FOR_DECL_RE.finditer(body))[:12]:
        open_i = body.rindex("(", m.start(), m.end())
        close_i = _paren_match(body, open_i)
        if close_i is None:
            continue
        typ, name, init = m.group("type"), m.group("name"), m.group("init")
        rest = body[m.end():close_i]                 # ` cond; inc` (the init's ';' was consumed)
        line_start = body.rfind("\n", 0, m.start()) + 1
        indent = re.match(r"[ \t]*", body[line_start:m.start()]).group(0)
        new_for = "for (%s = %s;%s)" % (name, init, rest)
        # (a) declare just before the loop, initialise in the for-init
        pre_decl = indent + "%s %s;\n" % (typ, name)
        out.append(("loop_decl_%d" % m.start(),
                    body[:line_start] + pre_decl + body[line_start:m.start()]
                    + new_for + body[close_i + 1:]))
        # (b) declare + initialise before the loop, empty for-init
        pre_init = indent + "%s %s = %s;\n" % (typ, name, init)
        out.append(("loop_decl_init_%d" % m.start(),
                    body[:line_start] + pre_init + body[line_start:m.start()]
                    + "for (;" + rest + ")" + body[close_i + 1:]))
        # (c) declare at the top of the body, so `decl_order` can place it against the other locals
        head = len(body) - len(body.lstrip())
        lead = body[:head]
        out.append(("loop_decl_top_%d" % m.start(),
                    lead + "%s %s;\n" % (typ, name) + lead[lead.rfind("\n") + 1:]
                    + body[head:m.start()] + new_for + body[close_i + 1:]))
        if len(out) >= limit:
            break
    return out


GENERATORS = {
    "decl_order": _gen_decl_order,
    "loop_decl": _gen_loop_decl,
    "decl_type": _gen_decl_type,
    "casts": _gen_casts,
    "stmt_order": _gen_stmt_order,
    "compound": _gen_compound,
    "temps": _gen_temps,
    "field": _gen_field_form,
    "dead_copy": _gen_dead_copy,
    "switch": _gen_switch,
    "cond": _gen_cond,
    "ternary": _gen_ternary,
    "loop": _gen_loop,
}

# The order the driver tries them in: branch/statement shape first (a residual is usually a branch or a
# colouring), then the allocator levers, then the more speculative ones.
DEFAULT_ORDER = ["switch", "cond", "compound", "stmt_order", "decl_order", "loop_decl", "decl_type",
                 "casts", "temps", "ternary", "loop", "field", "dead_copy"]


def generate(body, names=None, limit_per_gen=400):
    """[(gen, name, new_body)] for every generator in `names` (default: all), deduped by name."""
    out = []
    seen = set()
    for g in (names or DEFAULT_ORDER):
        fn = GENERATORS.get(g)
        if fn is None:
            continue
        for name, new in fn(body)[:limit_per_gen]:
            key = (g, name)
            if key in seen:
                continue
            seen.add(key)
            out.append((g, name, new))
    return out
