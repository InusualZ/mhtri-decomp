"""Generated-name and MWCC mangling predicates, once.
Spec: docs/tools/spec/lib-names.md. CLI: none (library)."""
from __future__ import annotations

import re

#: The generated-name schemes, each written once. `default`: the placeholders dtk and MWCC emit for a
#: symbol nobody named. `rule7`: section 6.5 rule 7's spellings (`fn_`/`lbl_`/`loc_` + hex, any length).
#: `map`: every address-derived placeholder that can sit in a map (dtk, Ghidra's `FUN_`, `sub_`, `dtor_`,
#: the exception-table labels, and a bare `unkNN`). `ledger`: the ledger's "carries no information yet"
#: test (a stem with or without `_`, any hex tail, including a bare stem).
GENERATED = {
    "default": re.compile(r"^(?:(?:fn|lbl|loc|jumptable|pad|gap)_[0-9A-Fa-f]+|@(?:etb|eti)_[0-9A-Fa-f]+|@\d+)$"),
    "rule7": re.compile(r"^(?:fn|lbl|loc)_[0-9A-Fa-f]+$"),
    "map": re.compile(r"^(?:fn_|lbl_|dtor_|FUN_|sub_|loc_|@etb_|@eti_)[0-9A-Fa-f]{6,8}$|^(?:unk|unk_)[0-9A-Fa-f]+$"),
    "ledger": re.compile(r"^(?:fn|lbl|unk|sub|loc|jump|jtbl|jumptable|gap)_?[0-9a-fA-F]*$"),
}
#: The `_XXXXXXXX` address tail a generated name carries.
ADDRESS_TAIL = re.compile(r"_([0-9A-Fa-f]{8})$")
#: MWCC's argument-list suffix: `__F<args>`, `__Q<n>` (a qualified owner), `__ct`/`__dt`. `__start`,
#: `_savegpr_14` and `lbl_8058B290` are C/EABI spellings and do not match.
MANGLE_SUFFIX = re.compile(r"__(?:F[A-Za-z0-9]|Q\d|ct|dt)")
#: The looser test for "carries a C++ argument list or owner": `__F` or `__Q` anywhere.
MANGLED = re.compile(r"__(F|Q)")

#: The parameter codes the project's scalar typedefs (`include/types.h`) expand to under MWCC's mangling.
PRIMITIVE_CODES = {
    "u8": "Uc", "s8": "Sc", "u16": "Us", "s16": "s", "u32": "Ul", "s32": "l", "u64": "Ux", "s64": "x",
    "f32": "f", "f64": "d", "BOOL": "i", "int": "i", "char": "c", "short": "s", "long": "l",
    "float": "f", "double": "d", "bool": "b", "void": "v",
    "unsigned char": "Uc", "signed char": "Sc", "unsigned short": "Us", "unsigned int": "Ui",
    "unsigned": "Ui", "unsigned long": "Ul", "long long": "x", "unsigned long long": "Ux",
    "short int": "s", "long int": "l", "unsigned short int": "Us", "unsigned long int": "Ul",
}
QUALIFIERS = {"const", "volatile", "register", "struct", "class", "enum"}


def is_generated(name: str, scheme: str = "default") -> bool:
    """Whether `name` is an address-derived placeholder under `scheme` (`default`, `rule7`, `map`)."""
    return bool(name) and bool(GENERATED[scheme].match(name))


def address_of(name: str) -> int | None:
    """The address a `<stem>_XXXXXXXX` name spells, or None."""
    m = ADDRESS_TAIL.search(name or "")
    return int(m.group(1), 16) if m else None


def is_mangled(name: str) -> bool:
    """Whether `name` carries MWCC's C++ argument list or owner qualifier (`fn__Fv`, `x__Q24nw4r2dbFv`)."""
    return "__" in name and MANGLED.search(name) is not None


def linkage_stem(name: str) -> str:
    """The identifier a mangled name is built from: everything before the `__<args>` suffix.

    A name never starts with its suffix: a constructor/destructor (`__ct__5FooFv`) keeps its `__ct`/`__dt`,
    so every constructor does not share the empty stem.
    """
    m = next((m for m in MANGLE_SUFFIX.finditer(name) if m.start() > 0), None)
    return name[:m.start()] if m else name


def peel_tokens(name: str) -> set[str]:
    """Every `<digits><exactly that many chars>` component of a mangling that starts like an identifier
    (`Q34nw4r4math4VEC3` -> `nw4r`, `math`, `VEC3`)."""
    out: set[str] = set()
    i, n = 0, len(name)
    while i < n:
        m = re.match(r"\d+", name[i:])
        if m:
            count = int(m.group(0))
            j = i + len(m.group(0))
            token = name[j:j + count]
            if len(token) == count and re.match(r"[A-Za-z_]", token or " "):
                out.add(token)
                i = j + count
                continue
        i += 1
    return out


def param_code(text: str) -> str | None:
    """The MWCC mangling of one parameter declaration (`const u8* data` -> `PCUc`), or None."""
    t = text.split("=")[0].strip()
    if not t or "(" in t or "[" in t or "::" in t or "<" in t or "..." in t:
        return None
    toks = re.findall(r"[A-Za-z_]\w*|\*|&", t)
    base: list[str] = []
    base_const = False
    levels: list[list] = []
    seen_decl = False
    for tok in toks:
        if tok in ("*", "&"):
            levels.append([tok, False])
            seen_decl = True
        elif tok in ("const", "volatile"):
            if levels:
                levels[-1][1] = levels[-1][1] or tok == "const"
            elif tok == "const":
                base_const = True
        elif tok in QUALIFIERS:
            continue
        elif not seen_decl:
            base.append(tok)
    words = base[:]
    prim = PRIMITIVE_CODES.get(" ".join(words))
    if prim is None and len(words) >= 2:
        prim = PRIMITIVE_CODES.get(" ".join(words[:-1]))
        if prim is not None or not levels:
            words = words[:-1]
    if prim is None:
        if len(words) != 1:
            return None
        prim = "%d%s" % (len(words[0]), words[0])
    code = ("C" if base_const else "") + prim
    for ch, const in levels:
        code = ("C" if const else "") + ("P" if ch == "*" else "R") + code
    return code


def estimate_member_mangling(type_name: str, method: str, rest_params: list[str],
                             const_self: bool = False) -> str | None:
    """`method__<len>Type[C]F<params>` for `Type::method(rest_params...)`, or None when unspellable.

    `rest_params` excludes the `self`; empty (or a lone `void`) is `Fv`. A repeated class-typed parameter
    is refused (the compiler back-references it).
    """
    params = [p.strip() for p in rest_params if p.strip() and p.strip() != "void"]
    codes: list[str] = []
    for p in params:
        c = param_code(p)
        if c is None:
            return None
        if any(ch.isdigit() for ch in c) and c in codes:
            return None
        codes.append(c)
    return "%s__%d%s%sF%s" % (method, len(type_name), type_name, "C" if const_self else "",
                              "".join(codes) if codes else "v")


def estimate_static_mangling(type_name: str, method: str, params: list[str]) -> str | None:
    """`method__<len>TypeF<params>` for `static Type::method(params...)` (no `this`, no cv-qualifier)."""
    return estimate_member_mangling(type_name, method, params, const_self=False)
