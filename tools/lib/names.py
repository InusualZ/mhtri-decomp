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
#: Section 6.5 rule 7 as the owner ruled it (2026-10-05): a generator's stem plus eight hex digits **anywhere** in an
#: identifier or a path component - `view_fn_80041234`, `fn_80041234__FPv`, `fn_800FD864_fx` and `dtor_8005E5E8` all
#: count. dtk's `fn_`/`lbl_`/`loc_`/`zz_` and the `dtor_` the map carries.
RULE7_STEM_RE = re.compile(r"(?:fn|lbl|loc|dtor|zz)_[0-9A-Fa-f]{8}")
#: The DOL's address span: the live map's lowest and highest symbol are 0x80004000 and 0x8079D7F0 (2026-10-05), so
#: [0x80004000, 0x80800000) holds every address a name can be derived from and nothing below the image - a flag value
#: such as `0x80000000` (`quest_flag_80000000_ck`) is not an address.
DOL_SPAN = (0x80004000, 0x80800000)
#: An eight-hex-digit run that reads like a DOL address inside an identifier: it starts with `8`, is not preceded by a
#: digit (so it is not the tail of a longer number) and not followed by a hex digit (so `Eft8030EffectSlot` - eight
#: characters `8030Effe` and then a `c` - is not one).
ADDRESS_RUN_RE = re.compile(r"(?<![0-9])8[0-7][0-9A-Fa-f]{6}(?![0-9A-Fa-f])")


def generated_name_kind(name: str) -> str | None:
    """Rule 7's verdict on one identifier or path component: `generated` (a `RULE7_STEM_RE` stem anywhere in it),
    `address` (an `ADDRESS_RUN_RE` run inside `DOL_SPAN` - `Panel805482CC`, `s_80276B58`, `Helper_80147CE0`, the
    `MHTRI_ENEMY_FN_80128204_H` guard of a generated header), or None. A numeric literal is never passed here: the
    caller tokenises identifiers only (`0x80276B58` is a number, not a name)."""
    if not name:
        return None
    if RULE7_STEM_RE.search(name):
        return "generated"
    for m in ADDRESS_RUN_RE.finditer(name):
        if DOL_SPAN[0] <= int(m.group(0), 16) < DOL_SPAN[1]:
            return "address"
    return None


def generated_path_components(path: str) -> list[tuple[str, bool]]:
    """`[(component, is_dir)]` for every component of `path` that `generated_name_kind` flags: each directory and
    the file's stem (the name without its last extension). One reading for stylelint's rule-7 file-name findings
    and the gate's new-unit row (`src/fn_8004CAD8/psvec.h` -> `[("fn_8004CAD8", True)]`)."""
    parts = [p for p in (path or "").replace("\\", "/").split("/") if p]
    out = []
    for i, part in enumerate(parts):
        is_dir = i < len(parts) - 1
        comp = part if is_dir else part.rsplit(".", 1)[0]
        if generated_name_kind(comp):
            out.append((comp, is_dir))
    return out


#: The `_XXXXXXXX` address tail a generated name carries.
ADDRESS_TAIL = re.compile(r"_([0-9A-Fa-f]{8})$")
#: MWCC's argument-list suffix: `__F<args>`, `__Q<n>` (a qualified owner), `__ct`/`__dt`. `__start`,
#: `_savegpr_14` and `lbl_8058B290` are C/EABI spellings and do not match.
MANGLE_SUFFIX = re.compile(r"__(?:F[A-Za-z0-9]|Q\d|ct|dt)")
#: The looser test for "carries a C++ argument list or owner": `__F` or `__Q` anywhere.
MANGLED = re.compile(r"__(F|Q)")

#: The parameter codes the project's scalar typedefs (`src/types.h`) expand to under MWCC's mangling.
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


def owner_stem(name: str) -> str:
    """The identifier and its owner qualifier without the argument list: `setTevKColor__6MHchar` for
    `setTevKColor__6MHcharFUl...`, `fn` for `fn__Fv`, `get__Q24nw4r2db` for `get__Q24nw4r2dbCFv`; a name with no
    argument list is itself. Two spellings with one owner stem are one function mangled two ways (or once in C)."""
    i = name.find("__", 1)
    while i > 0:
        j = i + 2
        rest = name[j:]
        if rest[:1] == "F":
            return name[:i]
        m = re.match(r"Q(\d)", rest)
        k = None
        if m:
            k, count = j + 2, int(m.group(1))
            for _ in range(count):
                d = re.match(r"\d+", name[k:])
                if not d:
                    k = None
                    break
                k += len(d.group()) + int(d.group())
        else:
            d = re.match(r"\d+", rest)
            if d:
                k = j + len(d.group()) + int(d.group())
        if k is not None and k <= len(name) and name[k:k + 1] in ("F", "C"):
            return name[:k]
        i = name.find("__", i + 1)
    return name


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
