"""Decide a translation unit's *language* (C or C++) from evidence, not from our convenience.

`docs/plan.md`, "The language comes from the symbol, not from our convenience" (owner's rule,
2026-09-23). A unit is **C++** only on **conclusive** evidence:

* **its own symbol is mangled** - a *definition* carrying MWCC's `__F`/`__Q` argument-list mangling
  (`SetPosition__Q34nw4r3g3d6CameraFRCQ34nw4r4math4VEC3`, `fn_800CD584__FP9ResHandle`);
* **its panic/log string names a `.cpp`** - the `__FILE__` assert strings are original source names
  (`ef_line.cpp`, `g3d_resanm.cpp`, `menu_message.cpp`). A unit whose *object* references one is a C++
  file even when its `.text` reads like C, and a unit whose object references a `.c` name is C.

A mangled name on the *referenced* side (`get_now_areano__Fv`, `Panic__Q24nw4r2dbFPCciPCce`) is
**suggestive, not conclusive**: it is still reported, with its reason, but it must not raise
confidence to `high` nor drive an extension change by itself. A C translation unit can call a mangled
function - it declares it with the map's spelling - and `auto/800FD520_fn_800FD520` does exactly that:
it calls mangled `SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3` and was reconstructed as
`.c`, matching at 100.00 % with all 26 relocations identical (`auto/803066F0_fn_803066F0.c` is the
same shape).

A third signal is **suggestive, not conclusive** too, and it is cheap and mechanical: an object that
carries the `extab`/`extabindex` sections was compiled as C++ - **C has no exceptions**, so a C
translation unit has no `__eh` records to emit. The confound is a lib whose `cflags` set
`-Cpp_exceptions on` (`cflags_pl`, `cflags_main`, `cflags_g3d`, `cflags_camellia`): that makes a **C**
unit emit `extab` as well, so the signal is only usable when the lib leaves the flag off, which
`cflags_exceptions` resolves from the unit's cflags variable. Even then it is **one-directional** - a
C++ file with no `try`/`catch`/`throw` emits no `extab`, so an object *without* the section evidences
nothing (`Runtime.PPCEABI.H/__init_cpp_exceptions.cpp` is C++ and has none). It therefore joins the
mangled-callee signal as `suggested`: reported with its reason, keeps the extension, and never
renames on its own. On today's tree its decisive hit count is **0** - the `.c` units that carry
`extab` all sit in libs that enable exceptions (`auto`/`main`), where the hint is silent - but it is
decisive for the no-exceptions libs and catches a wrong extension the moment a new unit is registered
there.

That decides three things and none of them is stylistic: the **file extension**, the **`-lang`** the
front-end is run with, and the **name objdiff pairs by** (a C++ definition is mangled unless it is
`extern "C"` - playbook row 42 seen from the other side).

**Why this is a tool and not a judgement call.** The attribution batches picked `.c`/`.cpp` per unit
from a guess, so registered units are the wrong language today: `auto/800CCFB0_fn_800CCFB0` is
`ef_line.cpp` and is registered as `.c`.  `auto/800CCFB0` closed at 99.96 % with its last rows
attributed to "C-vs-C++ front-end, not source shape" - that residual is not source-reachable. The fix
belongs in the promotion pass (rename + extension + one re-split), which is what the sweep report here
feeds.

The extension is not just a hint: `dtk` derives the front-end flag from it and passes `-lang=c` for a
`.c` object, `-lang=c++` for a `.cpp` one (visible in the real compile command). So a wrong extension is
a wrong compiler invocation, and a lib-level `-lang` in `cflags` would be the only way to disagree with
it - which is why `unit_verdict` resolves the cflags variable's text and reports that separately.

**The trap this tool exists to avoid.** Every target object carries a `STT_FILE` symbol whose name is
the *configured* source path (`800CCFB0_fn_800CCFB0.c`, `pl_master.cpp`). dtk's split synthesises it
from `configure.py`, so it is circular: it echoes the extension we chose and is evidence of nothing.
`elf_symbols` returns it, and every consumer here filters `type == 4`/`SHN_ABS` out. (The other
near-miss is `-lang`: it lives in a cflags list, so `unit_verdict` resolves the cflags variable's text
and reports a flag/verdict disagreement separately from an extension one.)

Usage:

    python tools/units/langcheck.py                 # every registered unit, verdict + evidence
    python tools/units/langcheck.py --disagree      # the sweep report: units that disagree
    python tools/units/langcheck.py --unit auto/800CCFB0_fn_800CCFB0
    python tools/units/langcheck.py --json
    python tools/units/langcheck.py --selftest

Read-only: nothing here writes a file, and nothing here re-splits, builds or links.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import struct
import sys
from tools.lib import names as libnames

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def _root(main: str | None = None) -> str:
    return os.path.abspath(main) if main else ROOT


# The extensions dtk/MWCC build as C++ and as C. `.cp` is the one dtk-template already uses for
# `Gecko_ExceptionPPC.cp`; `.c++` is in `unitutil.SOURCE_EXT`.
CXX_EXT = (".cpp", ".cc", ".cxx", ".cp", ".c++")
C_EXT = (".c",)

# The same bare-source-name filter `tudiscover.source_file_label` uses, so the two agree on what counts
# as a `__FILE__` string. A path separator is allowed because some assert strings carry one.
SRCFILE_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_./\\-]*\.(?:c|cpp|cc|cxx|cp|c\+\+)$")

# MWCC's C++ exception tables. C has no exceptions, so their presence in an object is evidence the
# translation unit was compiled as C++ - unless the lib sets `-Cpp_exceptions on` (the confound
# `cflags_exceptions` resolves). `.relaextab`/`.relaextabindex` are the reloc companions, not the
# sections themselves.
EXTAB_SECTIONS = ("extab", "extabindex")


# `configure.py` tokens this tool needs: the lib a block belongs to, its cflags variable, and one
# Object registration (with an optional inline `cflags=` override).
CONF_TOKEN_RE = re.compile(
    r'"lib":\s*"(?P<lib>[^"]+)"'
    r'|"cflags":\s*(?P<cflags>[A-Za-z_]\w*)'
    r'|Object\(\s*(?P<flag>[A-Za-z_]\w*)\s*,\s*"(?P<path>[^"]+)"(?P<rest>[^)]*)\)')
INLINE_CFLAGS_RE = re.compile(r"cflags\s*=\s*([A-Za-z_]\w*)")

# One cflags-list body is read as an ordered stream of literal tokens and spreads, because both `-lang`
# and `-Cpp_exceptions` mean "last one wins". A spread can be `*cflags_x` or a filtered comprehension
# `*[f for f in cflags_x if ...]`; the tokens a filter removes are re-added explicitly by the same list
# where that matters (`cflags_pl` filters `-Cpp_exceptions off` from `cflags_base` and then sets `on`).
_CFLAGS_TOKEN_RE = re.compile(
    r'"([^"]*)"'
    r'|\*\s*([A-Za-z_]\w*)'
    r'|\*\[[^\]]*?\bfor\s+\w+\s+in\s+([A-Za-z_]\w*)')


# --------------------------------------------------------------------------------------------------
# the pure rule
# --------------------------------------------------------------------------------------------------
def mangled(name: str) -> bool:
    return libnames.is_mangled(name)


def ext_lang(ext: str) -> str | None:
    """The language an extension implies - `None` for an extension this project does not build."""
    e = ext.lower()
    if e in CXX_EXT:
        return "c++"
    if e in C_EXT:
        return "c"
    return None


def classify(mangled_defined, mangled_undefined, sources, extab=False, exceptions_on=False) -> dict:
    """The verdict from the evidence sets. Pure, so the selftest can drive it directly.

    `mangled_defined`/`mangled_undefined` are symbol names; `sources` is the list of `__FILE__` names
    the object references or defines; `extab` is whether the object carries `extab`/`extabindex`, and
    `exceptions_on` whether the unit's lib enables `-Cpp_exceptions` (which would make a C unit emit
    `extab` too). Confidence is about the *evidence*, not the odds:

    * `high`   - **conclusive**: a mangled definition (the unit's own symbol) or a `__FILE__` string;
    * `medium` - **suggestive**: only mangled names on the *referenced* side (`Panic`,
       `get_now_areano__Fv`), or an `extab` object in a lib that does not enable exceptions. A C unit
       can call a mangled function by declaring it with the map's spelling, and C cannot emit `extab`
       unless exceptions are on - so both are reported (`suggested: True`) but are **not** a verdict:
       they must not drive a rename or an extension change (`conclusive: False`). In particular the
       `extab` signal is one-directional: its absence is not evidence of C.
    * `low`    - nothing mangled, no source string and no usable `extab`: C is the default, not a
       measurement.

    `conclusive` is True only when the unit's own name or a `__FILE__` string decides the language;
    `suggested` is True when the only C++ evidence is a mangled callee or an `extab` object.
    """
    md = sorted(set(mangled_defined))
    mu = sorted(set(mangled_undefined))
    srcs = sorted(set(sources))
    cpp_src = [s for s in srcs if os.path.splitext(s)[1].lower() in CXX_EXT]
    c_src = [s for s in srcs if os.path.splitext(s)[1].lower() == ".c"]
    # C has no exceptions, so `extab` in an object is C++ evidence - but only when the lib does not set
    # `-Cpp_exceptions on`, which would make a C unit emit it too (the confound this signal must respect).
    extab_signal = bool(extab) and not exceptions_on
    evidence = []
    if md:
        evidence.append({"kind": "mangled-defined", "detail": md[0], "count": len(md)})
    if cpp_src:
        evidence.append({"kind": "source-cpp", "detail": ", ".join(cpp_src), "count": len(cpp_src)})
    if mu:
        evidence.append({"kind": "mangled-undefined", "detail": mu[0], "count": len(mu)})
    if c_src:
        evidence.append({"kind": "source-c", "detail": ", ".join(c_src), "count": len(c_src)})
    if extab_signal:
        evidence.append({"kind": "extab", "detail": "extab/extabindex present and the lib does not enable "
                                                      "-Cpp_exceptions", "count": 1})
    conclusive = bool(md or cpp_src or c_src)
    if md or cpp_src:
        lang, confidence = "c++", "high"
    elif c_src:
        # a `.c` __FILE__ is conclusive C and outranks every suggested signal
        lang, confidence = "c", "high"
    elif extab_signal:
        # C cannot emit extab without -Cpp_exceptions on, and the lib does not set it - C++ (suggested)
        lang, confidence = "c++", "medium"
    elif mu:
        # a mangled *callee* only: report it, but it does not decide the language (see the docstring)
        lang, confidence = "c++", "medium"
    else:
        lang, confidence = "c", "low"
    return {
        "lang": lang,
        "confidence": confidence,
        "conclusive": conclusive,
        "suggested": bool(mu or extab_signal) and not conclusive,
        "evidence": evidence,
        "sources": srcs,
        "conflict": len(srcs) > 1,
        "mangled_defined": md,
        "mangled_undefined": mu,
        "cpp_sources": cpp_src,
        "c_sources": c_src,
        "extab": bool(extab),
        "extab_signal": extab_signal,
        "extab_conflict": bool(extab_signal and c_src),
        "exceptions_on": bool(exceptions_on),
    }


# --------------------------------------------------------------------------------------------------
# the object oracle
# --------------------------------------------------------------------------------------------------
def _elf_sections(path: str) -> list[dict]:
    """Every section header of an ELF32 big-endian object, with `sname` resolved.

    Shared by `elf_symbols` (it needs `.symtab`/`.strtab`) and `object_has_extab` (it needs the section
    names). Values are section-relative; `data` is the raw section content.
    """
    data = open(path, "rb").read()
    if data[:4] != b"\x7fELF":
        raise ValueError("%s is not an ELF object" % path)
    (shoff,) = struct.unpack_from(">I", data, 0x20)
    (shentsize, shnum, shstrndx) = struct.unpack_from(">HHH", data, 0x2E)
    secs = []
    for i in range(shnum):
        o = shoff + i * shentsize
        name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from(
            ">IIIIIIIIII", data, o)
        secs.append(dict(name=name, typ=typ, off=off, size=size, link=link, entsize=entsize,
                         data=data[off:off + size]))
    shstr = secs[shstrndx]["data"]
    for s in secs:
        e = shstr.find(b"\0", s["name"])
        s["sname"] = shstr[s["name"]:e].decode("latin-1")
    return secs


def elf_symbols(path: str) -> list[dict]:
    """Every symbol in an ELF32 big-endian object, **including undefined and `STT_FILE`**.

    `unitutil.read_elf` drops `shndx == 0` (undefined) and keeps but does not label `STT_FILE`; both
    matter here - the undefined names *are* the relocation targets (the strongest callee evidence),
    and the FILE symbol is the circular one this tool must not read. Values are section-relative.
    """
    secs = _elf_sections(path)
    symtab = next((s for s in secs if s["typ"] == 2), None)
    if symtab is None:
        return []
    strtab = secs[symtab["link"]]["data"]
    out = []
    for o in range(0, symtab["size"], symtab["entsize"] or 16):
        nm, val, size, info, other, shndx = struct.unpack_from(">IIIBBH", symtab["data"], o)
        if nm == 0:
            continue
        e = strtab.find(b"\0", nm)
        out.append({"name": strtab[nm:e].decode("latin-1"), "value": val, "size": size,
                    "type": info & 0xF, "shndx": shndx,
                    # SHN_ABS is where MWCC files the source-file (STT_FILE) symbol
                    "file": (info & 0xF) == 4})
    return out


def object_has_extab(path: str) -> bool:
    """True when the object carries an `extab`/`extabindex` section.

    MWCC emits those only for a C++ translation unit - **C has no exceptions** - unless the lib sets
    `-Cpp_exceptions on`, which is the confound the caller resolves (`cflags_exceptions`). A missing or
    unreadable object has no extab and therefore no signal.
    """
    try:
        return any(s["sname"] in EXTAB_SECTIONS for s in _elf_sections(path))
    except (OSError, ValueError, struct.error):
        return False


def object_names(target: str) -> tuple[set, set]:
    """`(defined, undefined)` symbol names of `target`, with the synthesised `STT_FILE` row removed.

    The FILE row is `dtk dol split`'s echo of the *configured* source path - it equals the extension
    we chose, so reading it would make this tool circular (`langcheck`'s docstring has the full note).
    """
    defined, undefined = set(), set()
    for s in elf_symbols(target):
        if s["file"]:
            continue
        if s["shndx"] == 0:
            undefined.add(s["name"])
        else:
            defined.add(s["name"])
    return defined, undefined


_ORACLE: dict = {}


def oracle(main: str | None = None):
    """`(labels, dol)` from the symbol map and the retail image, loaded once - `(None, None)` on failure.

    `labels` is `tudiscover.load_map()`'s label table (name -> {section, addr, size, kind}); `dol` is a
    `tudiscover.Dol`, so `lbl_XXXX` string content can be read back. Neither the graph nor the analysis
    is needed here, which is what keeps `brief.py` cheap.
    """
    key = _root(main)
    if key not in _ORACLE:
        try:
            sys.path.insert(0, os.path.join(key, "tools", "splits"))
            import tudiscover as td
            _fns, labels = td.load_map()
            dol = td.Dol(td.DOL) if os.path.exists(td.DOL) else None
            _ORACLE[key] = (labels, dol, None)
        except Exception as exc:                                    # missing orig/map, or a bad map
            _ORACLE[key] = (None, None, "%s: %s" % (type(exc).__name__, exc))
    labels, dol, err = _ORACLE[key]
    if err:
        return None, None
    return labels, dol


def string_sources(names, labels, dol) -> list[str]:
    """The `__FILE__`-style source names among `names`, read back from the retail image.

    A name qualifies when the map files it as a string (`// … data:string`) and its bytes are a bare
    source-file name. Symbols the map does not know - or a missing map/DOL - simply contribute nothing.
    """
    if not labels or dol is None:
        return []
    out = set()
    for n in names:
        lab = labels.get(n)
        if not lab or lab.get("kind") != "string":
            continue
        text = dol.cstr(lab["addr"], min(max(lab["size"], 4), 256)).decode("latin-1", "replace").strip()
        if SRCFILE_RE.match(text):
            out.add(text)
    return sorted(out)


def object_verdict(target: str, labels=None, dol=None, exceptions_on: bool = True) -> dict:
    """The verdict for one target object, with its evidence. `lang=None` when there is no object.

    Falls back to the module-level oracle for the map/DOL when the caller does not have them; that is
    the only I/O besides reading the object. `exceptions_on` is the unit's lib setting (see
    `cflags_exceptions`): when the lib enables `-Cpp_exceptions` a C unit emits `extab` too, so the
    section is not C++ evidence and the caller passes True. It **defaults to True** - a caller that has
    not resolved the lib flags must not emit the hint (a false positive is worse than no hint);
    `unit_verdict`/`sweep` pass the value they resolved from the cflags variable.
    """
    if not os.path.exists(target):
        return {"lang": None, "confidence": "none", "conclusive": False, "suggested": False,
                "evidence": [], "sources": [], "conflict": False, "mangled_defined": [],
                "mangled_undefined": [], "cpp_sources": [], "c_sources": [], "target": target,
                "extab": False, "extab_signal": False, "extab_conflict": False,
                "exceptions_on": bool(exceptions_on), "error": "no target object"}
    if labels is None or dol is None:
        labels, dol = oracle()
    defined, undefined = object_names(target)
    sources = string_sources(defined | undefined, labels, dol)
    v = classify([n for n in defined if mangled(n)], [n for n in undefined if mangled(n)], sources,
                 extab=object_has_extab(target), exceptions_on=exceptions_on)
    v["target"] = target
    return v


# --------------------------------------------------------------------------------------------------
# configure.py: the registered units, their extension and their cflags
# --------------------------------------------------------------------------------------------------
def registered_units(main: str | None = None) -> list[dict]:
    """Every `Object(...)` in `config.libs`, in registration order, with lib and cflags.

    Read from `configure.py`'s text (like `brief.registered_units`), so a unit whose object has never
    been built still counts and every field the report needs is there: the path as registered (whose
    extension *is* the current language), the lib, and the cflags variable that decides `-lang`.
    """
    path = os.path.join(_root(main), "configure.py")
    text = open(path, encoding="utf-8", errors="replace").read()
    out, lib, cflags = [], None, None
    for m in CONF_TOKEN_RE.finditer(text):
        if m.group("lib"):
            lib, cflags = m.group("lib"), None
        elif m.group("cflags"):
            cflags = m.group("cflags")
        else:
            inline = INLINE_CFLAGS_RE.search(m.group("rest") or "")
            out.append({"path": m.group("path"), "flag": m.group("flag"), "lib": lib,
                        "cflags": inline.group(1) if inline else cflags})
    return out


def cflags_tokens(text: str, name: str, seen=None) -> list[str]:
    """The literal `"-x y"` tokens reachable from a cflags variable, **in source order**.

    Follows `*cflags_x` spreads and filtered `*[f for f in cflags_x if ...]` spreads, because both
    `-lang` and `-Cpp_exceptions` are "last one wins" and the order decides the answer. Not a full
    Python evaluator: a filter's own quoted literals are collected too, which is harmless here (the one
    filter this tool cares about, `-Cpp_exceptions off`, is followed by an explicit `on` wherever it is
    filtered). Recursion is bounded so a cycle cannot hang the tool.
    """
    seen = set(seen or ())
    if not name or name in seen:
        return []
    seen.add(name)
    m = re.search(r"(?m)^%s\s*=\s*\[(.*?)\n\]" % re.escape(name), text, re.S)
    if not m:
        return []
    tokens = []
    for mt in _CFLAGS_TOKEN_RE.finditer(m.group(1)):
        if mt.group(1) is not None:
            tokens.append(mt.group(1))
        else:
            tokens.extend(cflags_tokens(text, mt.group(2) or mt.group(3), seen))
    return tokens


def cflags_lang(main: str | None = None, cflags_name: str | None = None, text: str | None = None) -> str | None:
    """The `-lang` a cflags variable resolves to, normalised to `"c"`/`"c++"`, or `None`.

    Both spellings are accepted (`"-lang c++"` and `"-lang", "c++"`), because `configure.py` uses both
    forms for other option-value pairs. `None` means "no `-lang` anywhere": the extension decides, which
    is every lib today.
    """
    if text is None:
        text = open(os.path.join(_root(main), "configure.py"), encoding="utf-8", errors="replace").read()
    tokens = cflags_tokens(text, cflags_name or "")
    values = []
    for i, tok in enumerate(tokens):
        m = re.match(r"^-lang(?:\s+(\S+))?$", tok)
        if m:
            if m.group(1):
                values.append(m.group(1))
            elif i + 1 < len(tokens):
                values.append(tokens[i + 1])
    if not values:
        return None
    v = values[-1]
    return "c++" if "++" in v else "c"


def cflags_exceptions(main: str | None = None, cflags_name: str | None = None, text: str | None = None) -> str | None:
    """Whether a cflags variable enables MWCC C++ exceptions: `"on"`, `"off"`, or `None` if unmentioned.

    This is the confound the `extab` language signal must respect: with `-Cpp_exceptions on` a **C** unit
    emits `extab`/`extabindex` too, so the section is only evidence of C++ when the lib leaves the flag
    off. Both spellings are accepted (`"-Cpp_exceptions on"` and the two-token form), matching
    `cflags_lang`. `None` means no token at all; the caller treats that as "not known to be off" and the
    signal stays unused (MWCC's default is on, and a false positive is worse than no hint).
    """
    if text is None:
        text = open(os.path.join(_root(main), "configure.py"), encoding="utf-8", errors="replace").read()
    tokens = cflags_tokens(text, cflags_name or "")
    values = []
    for i, tok in enumerate(tokens):
        m = re.match(r"^-Cpp_exceptions(?:\s+(\S+))?$", tok)
        if m:
            if m.group(1):
                values.append(m.group(1))
            elif i + 1 < len(tokens):
                values.append(tokens[i + 1])
    if not values:
        return None
    return "on" if values[-1] == "on" else "off"


def unit_verdict(main: str, path: str, labels=None, dol=None, exceptions_on: bool = True) -> dict:
    """The full verdict for one registered path: language, evidence, current extension and `-lang`.

    `path` is the object path as registered (`auto/800CCFB0_fn_800CCFB0.c`); the target object is the
    split object under `build/RMHE08/obj/`, and the extension says what the build currently does.
    `exceptions_on` is the lib setting that decides whether the `extab` signal is usable; it defaults to
    True (signal off) so an unresolved flag cannot invent a hint - `sweep`/`--unit` pass the value from
    `cflags_exceptions`.
    """
    main = _root(main)
    ext = os.path.splitext(path)[1]
    ext_l = ext_lang(ext)
    target = os.path.join(main, "build", "RMHE08", "obj", os.path.splitext(path)[0] + ".o")
    v = object_verdict(target, labels, dol, exceptions_on=exceptions_on)
    v["path"] = path
    v["extension"] = ext
    v["extension_lang"] = ext_l
    v["extension_agrees"] = (v["lang"] is None) or (v["lang"] == ext_l)
    # `-lang` needs the unit's cflags variable, which only the caller knows; it fills `lang_flag` in.
    v["lang_flag"] = None
    # dtk derives the real `-lang` from the extension (`-lang=c` for a `.c` object, `-lang=c++` for a
    # `.cpp` one), so unless a lib's cflags set `-lang` the extension *is* the front-end flag.
    v["effective_lang"] = None
    return v


# --------------------------------------------------------------------------------------------------
# what a worker is told (brief.py renders this; keep the wording in one place)
# --------------------------------------------------------------------------------------------------
def brief_paragraph(v: dict | None) -> str:
    """The worker-facing statement of the verdict and what it costs, for `brief.py`'s part 1."""
    if not v or not v.get("lang"):
        return ("**The unit's language is not on record** - the target object is missing, so nothing here "
                "could read it. If the source must be C++, put it in the outbox (`docs/plan.md`, \"The "
                "language comes from the symbol\").")
    if v["lang"] == "c++" and v.get("suggested"):
        if v.get("extab_signal"):
            return ("**This unit is probably C++**: its target object carries `extab`/`extabindex` and its "
                    "lib's cflags do not enable `-Cpp_exceptions`, so a C translation unit could not have "
                    "emitted them (%s). That is *suggested*, not conclusive - and one-directional, because a "
                    "C++ file with no `try`/`catch`/`throw` emits no `extab`. **Keep `.c` - and its flag set - "
                    "unless a mangled definition or a `.cpp` `__FILE__` string turns up** (the promotion field "
                    "is prepared for it)." % _evidence_text(v))
        return ("**This unit is probably C++, but the evidence is only suggestive** (%s). A mangled "
                "*callee* does not prove the caller is C++: a C unit can call a mangled function by "
                "declaring it with the map's spelling, and `auto/800FD520_fn_800FD520` calls mangled "
                "`SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3` and was reconstructed as `.c`, "
                "matching at 100.00 %% with all 26 relocations identical. **Keep `.c` unless a mangled "
                "definition or a `.cpp` `__FILE__` string turns up** - only conclusive evidence changes "
                "the language (`docs/plan.md`, \"The language comes from the symbol\")." % _evidence_text(v))
    if v["lang"] == "c++":
        why = _evidence_text(v)
        body = ("**This unit is C++** (%s). Name the file `.cpp`; MWCC mangles a C++ free function "
                "(`fn_800CCFB0` becomes `fn_800CCFB0__FP9ResHandle`), and objdiff pairs by name, so put "
                "`extern \"C\"` on every definition whose name `symbols.txt` spells plainly or it "
                "measures 0 %% (`docs/matching.md` row 42)." % why)
        if v.get("conflict"):
            body += (" **Two source names are referenced from one object** (%s): the split range spans two "
                     "original files, so the boundary needs a re-check before the extension is trusted."
                     % ", ".join("`%s`" % s for s in v["sources"]))
        return body
    why = _evidence_text(v)
    if v["confidence"] == "high":
        return ("**This unit is C** (%s): keep the `.c` extension - a C++ file here would mangle the names "
                "the map spells plainly." % why)
    return ("**The language is not evidenced**: the target object has no mangled symbol and no `__FILE__` "
            "string, so `.c` is the default rather than a measurement. If its data holds a `.cpp` name, say "
            "so in the outbox - the extension is what decides the front-end (`docs/plan.md`, \"The language "
            "comes from the symbol\").")


def _evidence_text(v: dict) -> str:
    bits = []
    for e in v["evidence"]:
        if e["kind"] == "mangled-defined":
            bits.append("mangled definition `%s`" % e["detail"])
        elif e["kind"] == "mangled-undefined":
            bits.append("calls mangled `%s`%s" % (e["detail"], " (+%d more)" % (e["count"] - 1) if e["count"] > 1 else ""))
        elif e["kind"] == "source-cpp":
            bits.append("`__FILE__` string `%s`" % e["detail"])
        elif e["kind"] == "source-c":
            bits.append("`__FILE__` string `%s`" % e["detail"])
        elif e["kind"] == "extab":
            bits.append("`extab`/`extabindex` present (C has no exceptions; the lib leaves `-Cpp_exceptions` off)")
    return "; ".join(bits) or "no evidence"


def evidence_text(v: dict) -> str:
    """The one-line evidence summary - shared by the report and the brief's language cell."""
    return _evidence_text(v)


def language_cell(v: dict | None) -> str:
    """The brief's `| language |` cell: the verdict, the confidence and the evidence, one line."""
    if not v or not v.get("lang"):
        return "not on record (no target object to read)"
    if v.get("suggested"):
        why = "extab in a no-exceptions lib" if v.get("extab_signal") else "mangled callee only"
        return "**C++** (suggested - %s, not conclusive: %s)" % (why, _evidence_text(v))
    return "**%s** (%s: %s)" % ("C++" if v["lang"] == "c++" else "C", v["confidence"], _evidence_text(v))


# --------------------------------------------------------------------------------------------------
# report
# --------------------------------------------------------------------------------------------------
def sweep(main: str) -> dict:
    """Every registered unit's verdict, plus the ones whose extension or `-lang` disagrees."""
    main = _root(main)
    conf = open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace").read()
    labels, dol = oracle(main)
    rows, cache, exc_cache = [], {}, {}
    for reg in registered_units(main):
        name = reg["cflags"] or ""
        if name not in cache:
            cache[name] = cflags_lang(main, name, conf)
        if name not in exc_cache:
            exc_cache[name] = cflags_exceptions(main, name, conf)
        # a lib that enables `-Cpp_exceptions` (or whose setting cannot be read) makes the `extab` signal
        # unusable; only an explicit `off` lets it fire (a false positive is worse than no hint)
        exc_setting = exc_cache[name]
        exceptions_on = exc_setting != "off"
        v = unit_verdict(main, reg["path"], labels, dol, exceptions_on=exceptions_on)
        v["lib"] = reg["lib"]
        v["cflags"] = reg["cflags"]
        v["exceptions"] = exc_setting
        v["lang_flag"] = cache[name]
        v["effective_lang"] = cache[name] or v["extension_lang"]
        v["flag_agrees"] = (v["lang"] is None) or (v["effective_lang"] is None) or (v["effective_lang"] == v["lang"])
        v["unit"] = os.path.splitext(reg["path"])[0]
        rows.append(v)
    ext_dis = [r for r in rows if not r["extension_agrees"] and r.get("conclusive")]
    flag_dis = [r for r in rows if r["effective_lang"] not in (None, r["lang"]) and r.get("conclusive")]
    # a mangled *callee* only: reported, with its reason, but never a rename driver - a C unit can call
    # a mangled function by declaring it with the map's spelling (auto/800FD520_fn_800FD520)
    suggested = [r for r in rows if r.get("suggested")]
    # a `.cpp` file whose object shows no evidence at all: the extension is a deliberate choice, and
    # nothing contradicts it - a note for a human, never a rename
    unevidenced = [r for r in rows if r["lang"] == "c" and r["confidence"] == "low" and r["extension_lang"] == "c++"]
    # the `extab` language signal, measured on the real tree. `extab_signal` is True only when the object
    # carries extab AND the lib does not enable exceptions - the only case where the hint is decisive.
    extab_units = [r for r in rows if r.get("extab")]
    extab_c = [r for r in extab_units if r["extension_lang"] == "c"]
    extab_cpp = [r for r in extab_units if r["extension_lang"] == "c++"]
    extab_other = [r for r in extab_units if r["extension_lang"] is None]
    candidates = [r for r in extab_c if r.get("extab_signal")]           # hint says C++, registration says C
    confounded = [r for r in extab_c if not r.get("extab_signal")]       # extab but lib enables exceptions
    contradictions = [r for r in rows if r["extension_lang"] == "c++" and r["lang"] is not None
                      and not r.get("extab") and not r.get("mangled_defined") and not r.get("mangled_undefined")]
    from collections import Counter
    return {
        "main": main,
        "oracle": labels is not None,
        "units": rows,
        # JSON-safe keys: a tuple key is not serialisable, and the report is machine-read too
        "counts": {"%s/%s" % k: v for k, v in Counter((r["lang"], r["confidence"]) for r in rows).items()},
        "langs": dict(Counter(r["lang"] or "none" for r in rows)),
        "split": {
            "conclusive_cpp": sum(1 for r in rows if r["conclusive"] and r["lang"] == "c++"),
            "conclusive_c": sum(1 for r in rows if r["conclusive"] and r["lang"] == "c"),
            "suggested": len(suggested),
            "c": sum(1 for r in rows if r["lang"] == "c"),
        },
        "extab": {
            "objects": extab_units,          # every object carrying extab/extabindex
            "registered_c": extab_c,         # ... of which the registration says C
            "registered_cpp": extab_cpp,     # ... of which the registration says C++
            "registered_other": extab_other,
            "candidates": candidates,        # decisive: no-exceptions lib, so the hint says C++
            "confounded": confounded,        # lib enables exceptions: the hint is silent
            "contradictions": contradictions,  # registered .cpp, no extab and no mangled symbol
        },
        "extension_disagreements": ext_dis,
        "flag_disagreements": flag_dis,
        "suggested": suggested,
        "unevidenced": unevidenced,
        "conflicts": [r for r in rows if r.get("conflict")],
    }


def render_row(r: dict) -> str:
    lang = r["lang"] or "?"
    ev = _evidence_text(r) if r["lang"] else (r.get("error") or "no object")
    warn = ""
    # only *conclusive* evidence drives an extension change; a suggested unit keeps its extension
    if not r["extension_agrees"] and r.get("conclusive"):
        warn = "  <- extension %s" % r["extension"]
    if r.get("suggested"):
        if r.get("extab_signal"):
            warn += "  <- suggested C++ (extab in a no-exceptions lib, not conclusive)"
        else:
            warn += "  <- suggested C++ (mangled callee only, not conclusive)"
    if r.get("extab_conflict"):
        warn += "  <- extab vs a `.c` __FILE__ string"
    if r.get("conflict"):
        warn += "  <- 2 source files"
    if r.get("lang_flag") and r["lang"] and r["lang_flag"] != r["lang"]:
        warn += "  <- lib -lang %s" % r["lang_flag"]
    return "%-52s %-6s %-7s %s%s" % (r["path"], r["lang"] or "-", r["confidence"], ev, warn)


def report(s: dict, only_disagree: bool = False, out=sys.stdout) -> None:
    if only_disagree:
        # a unit can be both an extension and a `-lang` disagreement (the same dict object): show it once
        rows, seen = [], set()
        for r in s["extension_disagreements"] + s["flag_disagreements"]:
            if id(r) not in seen:
                seen.add(id(r))
                rows.append(r)
    else:
        rows = s["units"]
    print("%-52s %-6s %-7s %s" % ("unit (as registered)", "lang", "conf", "evidence"), file=out)
    for r in rows:
        print(render_row(r), file=out)
    print("", file=out)
    if only_disagree:
        n_dis = len(s["extension_disagreements"])
        n_sug = len(s["suggested"])
        print("SWEEP: %d of %d registered unit(s) disagree with their extension **on conclusive "
              "evidence** (own mangled symbol or `__FILE__` string) and therefore with the `-lang` dtk "
              "derives from it; %d more are only *suggested* C++ (mangled callees or an `extab` object in a "
              "no-exceptions lib) and must not be renamed on that alone; %d lib(s) set `-lang` in cflags."
              % (n_dis, len(s["units"]), n_sug,
                 sum(1 for r in s["units"] if r["lang_flag"])), file=out)
        if s["flag_disagreements"] and len(s["flag_disagreements"]) != len(s["extension_disagreements"]):
            print("  `-lang` disagreements: %d" % len(s["flag_disagreements"]), file=out)
        print("Fix rides the promotion pass (rename + extension + `-lang`, one re-split), not a per-unit "
              "patch: `docs/plan.md`, \"The language comes from the symbol\".", file=out)
        # the list is already conclusive-only (see `sweep`); a mangled callee is not in it
        strong = s["extension_disagreements"]
        print("  conclusive (own mangled symbol or `__FILE__` string): %d" % len(strong), file=out)
        for r in strong:
            print("      %s -> %s   %s" % (r["path"], os.path.splitext(r["path"])[0] + ".cpp",
                                           _evidence_text(r)), file=out)
        print("  suggested only (mangled callees / extab objects - reported, never renamed on this alone): %d"
              % n_sug, file=out)
        for r in s["suggested"]:
            print("      %s   %s" % (r["path"], _evidence_text(r)), file=out)
        if s["unevidenced"]:
            print("  not a disagreement (a `.cpp` the object does not contradict): %d" % len(s["unevidenced"]),
                  file=out)
            for r in s["unevidenced"]:
                print("      %s  (%s)" % (r["path"], r.get("error") or "no evidence"), file=out)
        if s["conflicts"]:
            print("  one object, two source files (boundary needs a re-check): %d" % len(s["conflicts"]), file=out)
            for r in s["conflicts"]:
                print("      %s  %s" % (r["path"], ", ".join(r["sources"])), file=out)
        e = s["extab"]
        if e["candidates"] or e["contradictions"] or e["confounded"]:
            print("  extab signal: %d object(s) carry extab; %d decisive candidate(s), %d confounded "
                  "(lib enables exceptions), %d contradiction(s) (registered .cpp, no extab, no mangled "
                  "symbol). Full breakdown: without `--disagree`."
                  % (len(e["objects"]), len(e["candidates"]), len(e["confounded"]),
                     len(e["contradictions"])), file=out)
        return
    total = len(s["units"])
    print("%d registered unit(s); oracle=%s" % (total, "map+DOL" if s["oracle"] else "unavailable"), file=out)
    sp = s["split"]
    print("  conclusive c++   %d" % sp["conclusive_cpp"], file=out)
    print("  suggested c++    %d   (mangled callees / extab objects - reported, not a verdict)"
          % sp["suggested"], file=out)
    print("  c                %d" % sp["c"], file=out)
    for key in ("high", "medium", "low"):
        n = s["counts"].get("c++/" + key, 0) + s["counts"].get("c/" + key, 0)
        print("  %-6s %d" % (key, n), file=out)
    print("  c++    %d   c    %d%s" % (s["langs"].get("c++", 0), s["langs"].get("c", 0),
                                     ("   unreadable %d" % s["langs"]["none"]) if s["langs"].get("none") else ""),
          file=out)
    print("  extension disagreements (conclusive only): %d   suggested (not renamed): %d   "
          "lib-level `-lang` overrides: %d   unevidenced `.cpp`: %d   two-source-file conflicts: %d"
          % (len(s["extension_disagreements"]), sp["suggested"], sum(1 for r in s["units"] if r["lang_flag"]),
             len(s["unevidenced"]), len(s["conflicts"])), file=out)
    render_extab(s, out)


def render_extab(s: dict, out=sys.stdout) -> None:
    """The `extab` language-signal measurement - the buckets the owner's hint is worth.

    C has no exceptions, so `extab`/`extabindex` in an object means the unit was compiled as C++ **unless
    the lib enables `-Cpp_exceptions`** (`cflags_pl`, `cflags_main`, ...), which lets a C unit emit them
    too. Only the no-exceptions case is decisive, and even then the signal is one-directional: a C++
    file with no `try`/`catch`/`throw` emits no `extab`, so its absence is not evidence of C.
    """
    e = s["extab"]
    print("", file=out)
    print("extab language signal (C has no exceptions; decisive only when the lib does NOT set "
          "-Cpp_exceptions on):", file=out)
    print("  objects carrying extab/extabindex: %d" % len(e["objects"]), file=out)
    print("    of those registered .c:    %d" % len(e["registered_c"]), file=out)
    print("    of those registered .cpp:  %d" % len(e["registered_cpp"]), file=out)
    if e["registered_other"]:
        print("    of those registered other: %d" % len(e["registered_other"]), file=out)
    print("  decisive candidates (registered .c, no-exceptions lib -> hint says C++): %d"
          % len(e["candidates"]), file=out)
    for r in e["candidates"]:
        print("      %s  (lib %s; cflags %s do not enable -Cpp_exceptions)"
              % (r["path"], r["lib"], r["cflags"]), file=out)
    print("  confounded (registered .c, lib enables -Cpp_exceptions -> hint is silent): %d"
          % len(e["confounded"]), file=out)
    if e["confounded"]:
        by_lib = {}
        for r in e["confounded"]:
            by_lib.setdefault(r["lib"], []).append(r["path"])
        for lib in sorted(by_lib):
            print("      %-8s %d" % (lib, len(by_lib[lib])), file=out)
    print("  contradictions (registered .cpp, no extab and no mangled symbol): %d"
          % len(e["contradictions"]), file=out)
    for r in e["contradictions"]:
        print("      %s  (lib %s; no extab - a C++ file with no exceptions emits none, so this is not evidence of C)"
              % (r["path"], r["lib"]), file=out)
    # which libs the signal can use at all - the four no-exceptions libs are where it is prospective
    settings = {}
    for r in s["units"]:
        settings.setdefault(r["lib"], r.get("exceptions"))
    off = sorted(lib for lib, v in settings.items() if v == "off")
    on = sorted(lib for lib, v in settings.items() if v != "off")
    print("  libs that leave it off (signal usable): %s" % (", ".join(off) or "none"), file=out)
    print("  libs that enable/leave-unknown (signal unused): %s" % (", ".join(on) or "none"), file=out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--main", default=None, help="repository root (default: the one holding this file)")
    ap.add_argument("--unit", default=None, help="one registered unit (path, with or without extension)")
    ap.add_argument("--disagree", action="store_true", help="only the units whose language disagrees")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    main_dir = _root(args.main)
    if args.unit:
        want = args.unit.replace("\\", "/").strip("/")
        match = None
        for reg in registered_units(main_dir):
            if reg["path"] == want or os.path.splitext(reg["path"])[0] == os.path.splitext(want)[0]:
                match = reg
                break
        if match is None:
            print("not a registered unit: %s" % args.unit, file=sys.stderr)
            return 1
        conf = open(os.path.join(main_dir, "configure.py"), encoding="utf-8", errors="replace").read()
        labels, dol = oracle(main_dir)
        exc_setting = cflags_exceptions(main_dir, match["cflags"], conf)
        exceptions_on = exc_setting != "off"
        v = unit_verdict(main_dir, match["path"], labels, dol, exceptions_on=exceptions_on)
        v["lib"], v["cflags"] = match["lib"], match["cflags"]
        v["exceptions"] = exc_setting
        v["lang_flag"] = cflags_lang(main_dir, match["cflags"], conf)
        v["effective_lang"] = v["lang_flag"] or v["extension_lang"]
        if args.json:
            print(json.dumps(v, indent=2))
            return 0
        print(render_row(v))
        print("")
        print(brief_paragraph(v))
        return 0
    s = sweep(main_dir)
    if args.json:
        print(json.dumps(s, indent=2))
        return 0
    report(s, only_disagree=args.disagree)
    return 0


# --------------------------------------------------------------------------------------------------
# selftest
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # the mangling rule: the C++ side of the image, and the C/EABI spellings it must not claim
    for yes in ("Panic__Q24nw4r2dbFPCciPCce", "__dl__FPv", "GetResTexSrt__Q44nw4r3g3d6ScnMdl15CopiedMatAccessFb",
                "get_now_areano__Fv", "Pl_Skill_ck__FP4_PLWUs", "__ct__Q44nw4r3g3d6ScnMdl15CopiedMatAccessFPQ34nw4r3g3d6ScnMdlUl"):
        check("mangled: %s" % yes, mangled(yes), True)
    for no in ("fn_800CCFB0", "_savegpr_23", "lbl_80594DE0", "__start", "__init_data", "OSAlarm",
               "lbl_807962F8", "setVector3"):
        check("not mangled: %s" % no, mangled(no), False)

    # the extensions this project builds
    check(".cpp is C++", ext_lang(".cpp"), "c++")
    check(".cp is C++ (Gecko_ExceptionPPC.cp)", ext_lang(".cp"), "c++")
    check(".c is C", ext_lang(".c"), "c")
    check("an unknown extension has no language", ext_lang(".h"), None)

    # the verdict table, one row per combination that matters
    v = classify(["fn__Fv"], [], [])
    check("a mangled definition is C++/high", (v["lang"], v["confidence"]), ("c++", "high"))
    check("a mangled definition is conclusive", (v["conclusive"], v["suggested"]), (True, False))
    v = classify([], [], ["ef_line.cpp"])
    check("a `.cpp` __FILE__ is C++/high", (v["lang"], v["confidence"]), ("c++", "high"))
    check("a `.cpp` __FILE__ is conclusive", (v["conclusive"], v["suggested"]), (True, False))
    v = classify([], ["Panic__Q24nw4r2dbFPCciPCce"], [])
    check("only mangled callees is C++/medium", (v["lang"], v["confidence"]), ("c++", "medium"))
    check("a mangled callee is NOT conclusive", v["conclusive"], False)
    check("a mangled callee is reported as suggested", v["suggested"], True)
    v = classify([], [], ["OSAlarm.c"])
    check("a `.c` __FILE__ is C/high", (v["lang"], v["confidence"]), ("c", "high"))
    check("a `.c` __FILE__ is conclusive", (v["conclusive"], v["suggested"]), (True, False))
    v = classify([], [], [])
    check("no evidence is C/low (the default)", (v["lang"], v["confidence"]), ("c", "low"))
    check("no evidence is not conclusive", (v["conclusive"], v["suggested"]), (False, False))
    v = classify([], [], ["g3d_resanm.cpp", "g3d_resanmamblight.cpp"])
    check("two source names is a conflict", (v["lang"], v["conflict"], len(v["sources"])), ("c++", True, 2))
    v = classify(["fn__Fv"], ["Panic__Q24nw4r2dbFPCciPCce"], ["ef_line.cpp"])
    check("the evidence list carries all three kinds",
          [e["kind"] for e in v["evidence"]],
          ["mangled-defined", "source-cpp", "mangled-undefined"])
    check("a definition plus a callee is conclusive, not suggested",
          (v["conclusive"], v["suggested"]), (True, False))
    v = classify([], [], ["foo.cpp", "foo.c"])
    check("a `.cpp` and a `.c` name together is a conflict, C++ wins the verdict",
          (v["lang"], v["conflict"]), ("c++", True))
    v = classify([], ["get_now_areano__Fv"], ["TPL.c"])
    check("a conclusive `.c` string overrides a mangled callee",
          (v["lang"], v["conclusive"], v["suggested"]), ("c", True, False))
    v = classify(["fn__Fv", "fn__Fv"], [], [])
    check("duplicate evidence is de-duplicated", (v["mangled_defined"], v["evidence"][0]["count"]), (["fn__Fv"], 1))
    check("only `.c` is a C source (`.cp` is C++)",
          (classify([], [], ["a.c"])["c_sources"], classify([], [], ["a.cp", "a.cpp"])["c_sources"]),
          (["a.c"], []))

    # the `extab` language signal: C has no exceptions, so the section is C++ evidence - unless the lib
    # enables `-Cpp_exceptions` and a C unit can emit it too (the confound). Always *suggested*, never
    # conclusive, and one-directional: a C++ file with no try/catch/throw emits no extab.
    v = classify([], [], [], extab=True)
    check("extab in a no-exceptions lib is C++/medium", (v["lang"], v["confidence"]), ("c++", "medium"))
    check("extab alone is NOT conclusive", (v["conclusive"], v["suggested"]), (False, True))
    check("extab is reported as an evidence kind", [e["kind"] for e in v["evidence"]], ["extab"])
    check("the extab evidence says what it read", "extab/extabindex present" in v["evidence"][0]["detail"], True)
    check("extab evidence names the flag it checked", "-Cpp_exceptions" in v["evidence"][0]["detail"], True)
    v = classify([], [], [], extab=True, exceptions_on=True)
    check("an exceptions-ON lib makes extab silent", (v["lang"], v["confidence"]), ("c", "low"))
    check("a confounded extab is not even suggested", (v["conclusive"], v["suggested"], v["extab_signal"]),
          (False, False, False))
    check("a confounded extab is not evidence", v["evidence"], [])
    v = classify([], [], [], extab=False)
    check("no extab is not evidence of C", (v["lang"], v["evidence"]), ("c", []))
    check("no extab leaves the field off", (v["extab"], v["extab_signal"]), (False, False))
    v = classify([], [], ["OSAlarm.c"], extab=True)
    check("a conclusive `.c` string outranks the extab hint", (v["lang"], v["confidence"]), ("c", "high"))
    check("a `.c` string against extab is flagged", v["extab_conflict"], True)
    v = classify([], ["get_now_areano__Fv"], [], extab=True)
    check("extab + a mangled callee stays suggested, not conclusive",
          (v["lang"], v["conclusive"], v["suggested"]), ("c++", False, True))
    check("the extab evidence is listed after the callee", [e["kind"] for e in v["evidence"]],
          ["mangled-undefined", "extab"])
    v = classify(["fn__Fv"], [], [], extab=True)
    check("extab does not change a conclusive C++ verdict", (v["lang"], v["confidence"], v["conclusive"]),
          ("c++", "high", True))

    # the ELF section oracle, on a synthetic object with and without extab - deterministic, so the check
    # runs even in a fresh worktree with no `build/` (where the real-object block below is skipped)
    def make_elf(section_names):
        import struct as _st
        shstr, off = b"\0", {}
        for n in list(section_names) + [".shstrtab"]:
            off[n] = len(shstr)
            shstr += n.encode("latin-1") + b"\0"
        body, content_off = bytearray(), {}
        for n in section_names:
            content_off[n] = 52 + len(body)
            body += b"\0\0\0\0"
        shstr_off = 52 + len(body)
        body += shstr
        shoff = 52 + len(body)
        shdrs = [(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
        for n in section_names:
            shdrs.append((off[n], 1, 0, 0, content_off[n], 4, 0, 0, 4, 0))
        shdrs.append((off[".shstrtab"], 3, 0, 0, shstr_off, len(shstr), 0, 0, 1, 0))
        shnum = len(shdrs)
        hdr = (b"\x7fELF" + bytes([1, 2, 1, 0]) + b"\0" * 8
               + _st.pack(">HHIIIIIHHHHHH", 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40, shnum, shnum - 1))
        out = bytearray(hdr) + body
        for s in shdrs:
            out += _st.pack(">IIIIIIIIII", *s)
        return bytes(out)

    with tempfile.TemporaryDirectory() as tmp:
        with_extab = os.path.join(tmp, "with_extab.o")
        no_extab = os.path.join(tmp, "no_extab.o")
        junk = os.path.join(tmp, "junk.o")
        open(with_extab, "wb").write(make_elf(["extab", "extabindex", ".text"]))
        open(no_extab, "wb").write(make_elf([".text"]))
        open(junk, "wb").write(b"not an ELF\n")
        check("object_has_extab sees the section", object_has_extab(with_extab), True)
        check("object_has_extab is false without it", object_has_extab(no_extab), False)
        check("object_has_extab is false for a missing object", object_has_extab(no_extab + ".nope"), False)
        check("object_has_extab is false for a non-ELF file", object_has_extab(junk), False)
        # object_verdict threads the lib setting end to end; a truthy dummy dol skips the map/DOL oracle
        v = object_verdict(with_extab, {}, object(), exceptions_on=False)
        check("object_verdict fires the extab signal for a no-exceptions lib",
              (v["lang"], v["confidence"], v["conclusive"], v["suggested"]), ("c++", "medium", False, True))
        v = object_verdict(with_extab, {}, object(), exceptions_on=True)
        check("object_verdict silences the signal when the lib enables exceptions",
              (v["lang"], v["confidence"], v["extab_signal"]), ("c", "low", False))
        v = object_verdict(no_extab, {}, object(), exceptions_on=False)
        check("object_verdict does not infer C from a missing extab", (v["lang"], v["evidence"]), ("c", []))

    # the synthesized STT_FILE is the trap: the selftest proves the reader hands it back and that the
    # verdict filter drops it by reading the real target when it is there
    real = os.path.join(ROOT, "build", "RMHE08", "obj", "auto", "800CCFB0_fn_800CCFB0.o")
    if os.path.exists(real):
        syms = elf_symbols(real)
        files = [s["name"] for s in syms if s["file"]]
        check("elf_symbols returns the STT_FILE row", files, ["800CCFB0_fn_800CCFB0.c"])
        check("object_names drops it (no circular evidence from our own extension)",
              "800CCFB0_fn_800CCFB0.c" in object_names(real)[0] or "800CCFB0_fn_800CCFB0.c" in object_names(real)[1],
              False)
        labels, dol = oracle()
        v = object_verdict(real, labels, dol)
        check("the real 800CCFB0 verdict is C++/high", (v["lang"], v["confidence"]), ("c++", "high"))
        check("its __FILE__ evidence is ef_line.cpp", "ef_line.cpp" in v["sources"], True)
        check("its mangled callee is Panic", any(m.startswith("Panic__Q24nw4r2d") for m in v["mangled_undefined"]), True)
        check("a missing object is reported, not guessed", object_verdict(real + ".nope")["lang"], None)
        check("a missing object carries the new flags (no KeyError downstream)",
              (object_verdict(real + ".nope")["conclusive"], object_verdict(real + ".nope")["suggested"]),
              (False, False))

        # the sweep report, end to end, against the real tree - it is what the promotion pass consumes
        #
        # These checks used to name units by PATH (`800CCFB0_fn_800CCFB0.c`), which the promotion pass
        # renamed (`ef/ef_line.cpp`) - so they broke the moment the work they describe succeeded. Look the
        # unit up by the ADDRESS it holds instead: the address never changes, and the assertion then states
        # the current truth rather than a historical path.
        import io

        def unit_at(addr: int) -> str | None:
            """The registered unit whose `.text` range contains `addr`, from `splits.txt`."""
            cur, rng = None, None
            for line in open(os.path.join(ROOT, "config", "RMHE08", "splits.txt"),
                             encoding="utf-8", errors="replace"):
                m = re.match(r"^(\S+):\s*$", line)
                if m:
                    cur = m.group(1)
                    continue
                t = re.match(r"\s*\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
                if t and cur:
                    a, b = int(t.group(1), 16), int(t.group(2), 16)
                    if a <= addr < b:
                        rng = cur
            return rng

        def obj_of(unit: str) -> str:
            return os.path.splitext(os.path.join(ROOT, "build", "RMHE08", "obj",
                                                 *unit.split("/")))[0] + ".o"

        s = sweep(ROOT)
        check("sweep covers every registered unit", len(s["units"]), len(registered_units(ROOT)))
        check("the sweep is JSON-serialisable (no tuple keys)", json.dumps(s)[:1], "{")
        # `800CCFB0` was the known extension disagreement (a `.cpp` __FILE__ registered as `.c`). The
        # promotion pass renamed it to `ef/ef_line.cpp`, which is the fix - so the assertion is now that it
        # is NOT a disagreement, and that its verdict is still the C++ one that drove the rename.
        cccfb0 = unit_at(0x800CCFB0)
        check("the 800CCFB0 unit is still registered (by address, not path)", bool(cccfb0), True)
        check("800CCFB0 is registered as C++ now (the promotion fixed it)", cccfb0.endswith(".cpp"), True)
        check("800CCFB0 is no longer an extension disagreement",
              any(r["path"] == cccfb0 for r in s["extension_disagreements"]), False)
        check("800CCFB0's verdict is still C++/high in the sweep",
              [(r["lang"], r["confidence"]) for r in s["units"] if r["path"] == cccfb0], [("c++", "high")])
        check("a C unit is not in the disagreement list",
              any(r["path"] == "Camellia/camellia.c" for r in s["extension_disagreements"]), False)
        check("the unresolved `-lang` override count is reported",
              all(r["lang_flag"] is None for r in s["units"]), True)
        buf = io.StringIO()
        report(s, only_disagree=True, out=buf)
        check("the sweep report names the promotion pass", "promotion pass" in buf.getvalue(), True)
        check("every extension disagreement is conclusive",
              all(r["conclusive"] for r in s["extension_disagreements"]), True)
        check("the suggested units are reported separately", len(s["suggested"]) > 0, True)
        check("a suggested unit is not in the disagreement list",
              all(not r["conclusive"] for r in s["suggested"]), True)
        check("the report prints the suggested section",
              "suggested only" in buf.getvalue(), True)

        # the extab signal measured on the real tree: the buckets must be internally consistent
        check("the sweep measures the extab signal", "objects" in s["extab"], True)
        check("every extab object is registered .c, .cpp or another buildable extension",
              len(s["extab"]["registered_c"]) + len(s["extab"]["registered_cpp"])
              + len(s["extab"]["registered_other"]), len(s["extab"]["objects"]))
        check("an object with extab in an exceptions-ON lib is not a decisive candidate",
              all(r["exceptions"] == "off" for r in s["extab"]["candidates"]), True)
        check("a decisive candidate is a `.c` unit",
              all(r["extension_lang"] == "c" for r in s["extab"]["candidates"]), True)
        check("a confounded unit's lib enables (or does not state off) exceptions",
              all(r["exceptions"] != "off" for r in s["extab"]["confounded"]), True)
        check("every contradiction is a `.cpp` with no extab and no mangled symbol",
              all(r["extension_lang"] == "c++" and not r["extab"] and not r["mangled_defined"]
                  and not r["mangled_undefined"] for r in s["extab"]["contradictions"]), True)
        check("the extab buckets survive JSON", json.dumps(s["extab"])[:1], "{")
        buf = io.StringIO()
        render_extab(s, out=buf)
        check("the extab report names the decisive bucket", "decisive candidates" in buf.getvalue(), True)

        # the counter-example: the unit at 0x800FD520 calls mangled SetRootMtxTrans__... and was
        # reconstructed as `.c`, matching at 100.00 % with all 26 relocations identical. Its mangled callee
        # must stay a suggestion and must not appear in the rename list. Looked up by ADDRESS: the promotion
        # pass renamed this unit to `ef/fn_800FD520.c` and the check must survive that.
        fd520 = unit_at(0x800FD520)
        cx = obj_of(fd520) if fd520 else ""
        check("the 800FD520 unit is still registered", bool(fd520), True)
        if fd520 and os.path.exists(cx):
            v = object_verdict(cx, labels, dol)
            check("the 800FD520 counter-example is C++/medium", (v["lang"], v["confidence"]), ("c++", "medium"))
            check("the 800FD520 counter-example is not conclusive", (v["conclusive"], v["suggested"]), (False, True))
            check("800FD520 does not drive an extension rename",
                  any(r["path"] == fd520 for r in s["extension_disagreements"]), False)
            check("800FD520 is listed as suggested",
                  any(r["path"] == fd520 for r in s["suggested"]), True)

    # `-lang` resolution: both spellings, and a nested spread
    conf_text = (
        "cflags_a = [\n"
        '    "-O3",\n'
        '    "-lang c++",\n'
        "]\n"
        "cflags_b = [\n"
        "    *cflags_a,\n"
        '    "-func_align",\n'
        '    "4",\n'
        "]\n"
        "cflags_c = [\n"
        '    "-nodefaults",\n'
        '    "-lang",\n'
        '    "c",\n'
        "]\n")
    check("a direct -lang is found", cflags_lang(None, "cflags_a", conf_text), "c++")
    check("a -lang in a spread it resolves", cflags_lang(None, "cflags_b", conf_text), "c++")
    check("the two-token spelling is found", cflags_lang(None, "cflags_c", conf_text), "c")
    check("no -lang is None", cflags_lang(None, "cflags_d", conf_text), None)
    check("a cycle terminates", cflags_lang(None, "nope", "nope = [\n  *nope,\n]\n"), None)

    # `-Cpp_exceptions` resolution: the confound the extab signal must respect. A filtered spread
    # (`*[f for f in cflags_base if ...]`) is the shape `cflags_lobby`/`cflags_rso`/`cflags_pl_skill` use.
    exc_text = (
        "cflags_base = [\n"
        '    "-Cpp_exceptions off",\n'
        "]\n"
        "cflags_on = [\n"
        "    *cflags_base,\n"
        '    "-Cpp_exceptions on",\n'
        "]\n"
        "cflags_filtered = [\n"
        '    *[f for f in cflags_base if f != "-O4,p"],\n'
        '    "-O3",\n'
        "]\n"
        "cflags_filtered_on = [\n"
        '    *[f for f in cflags_base if f not in ("-O4,p", "-Cpp_exceptions off")],\n'
        '    "-Cpp_exceptions on",\n'
        "]\n"
        "cflags_two = [\n"
        '    "-Cpp_exceptions",\n'
        '    "on",\n'
        "]\n")
    check("an explicit -Cpp_exceptions off resolves", cflags_exceptions(None, "cflags_base", exc_text), "off")
    check("a later -Cpp_exceptions on wins", cflags_exceptions(None, "cflags_on", exc_text), "on")
    check("a filtered spread resolves", cflags_exceptions(None, "cflags_filtered", exc_text), "off")
    check("a filtered spread plus an explicit on resolves", cflags_exceptions(None, "cflags_filtered_on", exc_text), "on")
    check("the two-token -Cpp_exceptions spelling resolves", cflags_exceptions(None, "cflags_two", exc_text), "on")
    check("no -Cpp_exceptions token is None", cflags_exceptions(None, "cflags_none", exc_text), None)

    # the registered-unit parser, against a fixture shaped like the real configure.py
    with tempfile.TemporaryDirectory() as tmp:
        open(os.path.join(tmp, "configure.py"), "w").write(
            "config.libs = [\n"
            "    {\n"
            '        "lib": "auto",\n'
            '        "cflags": cflags_main,\n'
            '        "objects": [\n'
            '            Object(NonMatching, "auto/a.c"),\n'
            '            Object(Matching, "auto/b.cpp", cflags=cflags_pl),\n'
            "        ],\n"
            "    },\n"
            "]\n")
        got = registered_units(tmp)
        check("the parser reads both objects in order", [r["path"] for r in got], ["auto/a.c", "auto/b.cpp"])
        check("the lib is carried", [r["lib"] for r in got], ["auto", "auto"])
        check("the lib's cflags is the fallback", got[0]["cflags"], "cflags_main")
        check("an inline cflags wins", got[1]["cflags"], "cflags_pl")
        check("the Matching flag is carried", got[1]["flag"], "Matching")
        check("the registered extension is the current language",
              [ext_lang(os.path.splitext(r["path"])[1]) for r in got], ["c", "c++"])

    # the worker-facing paragraph says the actionable things
    p = brief_paragraph(classify(["fn__Fv"], [], []))
    check("C++ paragraph says .cpp", "`.cpp`" in p, True)
    check("C++ paragraph says extern \"C\"", 'extern "C"' in p, True)
    check("C++ paragraph names row 42", "row 42" in p, True)
    p = brief_paragraph(classify([], ["get_now_areano__Fv"], []))
    check("a suggested C++ says the evidence is only suggestive", "only suggestive" in p, True)
    check("a suggested C++ says only conclusive evidence changes the language", "only conclusive" in p, True)
    check("a suggested C++ cites the 800FD520 counter-example", "800FD520" in p, True)
    check("a suggested C++ says keep .c unless conclusive evidence appears", "Keep `.c`" in p, True)
    p = brief_paragraph(classify([], [], [], extab=True))
    check("an extab-suggested C++ says it is not conclusive", "not conclusive" in p, True)
    check("an extab-suggested C++ says the signal is one-directional", "one-directional" in p, True)
    check("an extab-suggested C++ says keep .c", "Keep `.c`" in p, True)
    p = brief_paragraph(classify([], [], []))
    check("an unevidenced unit says C is the default", "default" in p and "`.c`" in p, True)
    p = brief_paragraph(classify([], [], ["OSAlarm.c"]))
    check("a C verdict cites the .c string", "OSAlarm.c" in p, True)
    p = brief_paragraph(None)
    check("no verdict is reported as such", "not on record" in p, True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
