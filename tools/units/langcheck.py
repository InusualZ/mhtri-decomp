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

import argparse
import json
import os
import re
import struct
import sys

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

# MWCC's C++ mangling appends the argument list after `__` (`fn__Fv`, `Pl_Skill_ck__FP4_PLWUs`), and
# the `__F`/`__Q` form is what every mangled name in this image uses. `__start`, `__init_data`,
# `_savegpr_21` and `lbl_80594DE0` are C/EABI spellings and deliberately do not match.
MANGLE_RE = re.compile(r"__(F|Q)")

# `configure.py` tokens this tool needs: the lib a block belongs to, its cflags variable, and one
# Object registration (with an optional inline `cflags=` override).
CONF_TOKEN_RE = re.compile(
    r'"lib":\s*"(?P<lib>[^"]+)"'
    r'|"cflags":\s*(?P<cflags>[A-Za-z_]\w*)'
    r'|Object\(\s*(?P<flag>[A-Za-z_]\w*)\s*,\s*"(?P<path>[^"]+)"(?P<rest>[^)]*)\)')
INLINE_CFLAGS_RE = re.compile(r"cflags\s*=\s*([A-Za-z_]\w*)")


# --------------------------------------------------------------------------------------------------
# the pure rule
# --------------------------------------------------------------------------------------------------
def mangled(name: str) -> bool:
    """MWCC's C++ mangling puts the argument list after `__` (`fn__Fv`, `Pl_Skill_ck__FP4_PLWUs`)."""
    return "__" in name and MANGLE_RE.search(name) is not None


def ext_lang(ext: str) -> str | None:
    """The language an extension implies - `None` for an extension this project does not build."""
    e = ext.lower()
    if e in CXX_EXT:
        return "c++"
    if e in C_EXT:
        return "c"
    return None


def classify(mangled_defined, mangled_undefined, sources) -> dict:
    """The verdict from the three evidence sets. Pure, so the selftest can drive it directly.

    `mangled_defined`/`mangled_undefined` are symbol names; `sources` is the list of `__FILE__` names
    the object references or defines. Confidence is about the *evidence*, not the odds:

    * `high`   - **conclusive**: a mangled definition (the unit's own symbol) or a `__FILE__` string;
    * `medium` - **suggestive**: only mangled names on the *referenced* side (`Panic`,
       `get_now_areano__Fv`). A C unit can call a mangled function by declaring it with the map's
       spelling, so this is reported (`suggested: True`) but is **not** a verdict: it must not drive a
       rename or an extension change (`conclusive: False`). `auto/800FD520_fn_800FD520` calls mangled
       `SetRootMtxTrans__...` and matched at 100.00 % as `.c`.
    * `low`    - nothing mangled and no source string: C is the default, not a measurement.

    `conclusive` is True only when the unit's own name or a `__FILE__` string decides the language;
    `suggested` is True when the only C++ evidence is a mangled callee.
    """
    md = sorted(set(mangled_defined))
    mu = sorted(set(mangled_undefined))
    srcs = sorted(set(sources))
    cpp_src = [s for s in srcs if os.path.splitext(s)[1].lower() in CXX_EXT]
    c_src = [s for s in srcs if os.path.splitext(s)[1].lower() == ".c"]
    evidence = []
    if md:
        evidence.append({"kind": "mangled-defined", "detail": md[0], "count": len(md)})
    if cpp_src:
        evidence.append({"kind": "source-cpp", "detail": ", ".join(cpp_src), "count": len(cpp_src)})
    if mu:
        evidence.append({"kind": "mangled-undefined", "detail": mu[0], "count": len(mu)})
    if c_src:
        evidence.append({"kind": "source-c", "detail": ", ".join(c_src), "count": len(c_src)})
    conclusive = bool(md or cpp_src or c_src)
    if md or cpp_src:
        lang, confidence = "c++", "high"
    elif c_src:
        # a `.c` __FILE__ is conclusive C and outranks a mangled callee (the counter-example's shape)
        lang, confidence = "c", "high"
    elif mu:
        # a mangled *callee* only: report it, but it does not decide the language (see the docstring)
        lang, confidence = "c++", "medium"
    else:
        lang, confidence = "c", "low"
    return {
        "lang": lang,
        "confidence": confidence,
        "conclusive": conclusive,
        "suggested": bool(mu) and not conclusive,
        "evidence": evidence,
        "sources": srcs,
        "conflict": len(srcs) > 1,
        "mangled_defined": md,
        "mangled_undefined": mu,
        "cpp_sources": cpp_src,
        "c_sources": c_src,
    }


# --------------------------------------------------------------------------------------------------
# the object oracle
# --------------------------------------------------------------------------------------------------
def elf_symbols(path: str) -> list[dict]:
    """Every symbol in an ELF32 big-endian object, **including undefined and `STT_FILE`**.

    `unitutil.read_elf` drops `shndx == 0` (undefined) and keeps but does not label `STT_FILE`; both
    matter here - the undefined names *are* the relocation targets (the strongest callee evidence),
    and the FILE symbol is the circular one this tool must not read. Values are section-relative.
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


def object_verdict(target: str, labels=None, dol=None) -> dict:
    """The verdict for one target object, with its evidence. `lang=None` when there is no object.

    Falls back to the module-level oracle for the map/DOL when the caller does not have them; that is
    the only I/O besides reading the object.
    """
    if not os.path.exists(target):
        return {"lang": None, "confidence": "none", "conclusive": False, "suggested": False,
                "evidence": [], "sources": [], "conflict": False, "mangled_defined": [],
                "mangled_undefined": [], "cpp_sources": [], "c_sources": [], "target": target,
                "error": "no target object"}
    if labels is None or dol is None:
        labels, dol = oracle()
    defined, undefined = object_names(target)
    sources = string_sources(defined | undefined, labels, dol)
    v = classify([n for n in defined if mangled(n)], [n for n in undefined if mangled(n)], sources)
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
    """The literal `"-x y"` tokens reachable from a cflags variable, following `*cflags_x` spreads.

    Not a full Python evaluator: it is enough to find `-lang`, which no `if f != "…"` filter in this
    file removes. Recursion is bounded so a cycle cannot hang the tool.
    """
    seen = set(seen or ())
    if not name or name in seen:
        return []
    seen.add(name)
    m = re.search(r"(?m)^%s\s*=\s*\[(.*?)\n\]" % re.escape(name), text, re.S)
    if not m:
        return []
    body = m.group(1)
    tokens = [t for t in re.findall(r'"([^"]*)"', body)]
    for ref in set(re.findall(r"\*\s*([A-Za-z_]\w*)", body)):
        tokens.extend(cflags_tokens(text, ref, seen))
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


def unit_verdict(main: str, path: str, labels=None, dol=None) -> dict:
    """The full verdict for one registered path: language, evidence, current extension and `-lang`.

    `path` is the object path as registered (`auto/800CCFB0_fn_800CCFB0.c`); the target object is the
    split object under `build/RMHE08/obj/`, and the extension says what the build currently does.
    """
    main = _root(main)
    ext = os.path.splitext(path)[1]
    ext_l = ext_lang(ext)
    target = os.path.join(main, "build", "RMHE08", "obj", os.path.splitext(path)[0] + ".o")
    v = object_verdict(target, labels, dol)
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
    return "; ".join(bits) or "no evidence"


def evidence_text(v: dict) -> str:
    """The one-line evidence summary - shared by the report and the brief's language cell."""
    return _evidence_text(v)


def language_cell(v: dict | None) -> str:
    """The brief's `| language |` cell: the verdict, the confidence and the evidence, one line."""
    if not v or not v.get("lang"):
        return "not on record (no target object to read)"
    if v.get("suggested"):
        return "**C++** (suggested - mangled callee only, not conclusive: %s)" % _evidence_text(v)
    return "**%s** (%s: %s)" % ("C++" if v["lang"] == "c++" else "C", v["confidence"], _evidence_text(v))


# --------------------------------------------------------------------------------------------------
# report
# --------------------------------------------------------------------------------------------------
def sweep(main: str) -> dict:
    """Every registered unit's verdict, plus the ones whose extension or `-lang` disagrees."""
    main = _root(main)
    conf = open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace").read()
    labels, dol = oracle(main)
    rows, cache = [], {}
    for reg in registered_units(main):
        name = reg["cflags"] or ""
        if name not in cache:
            cache[name] = cflags_lang(main, name, conf)
        v = unit_verdict(main, reg["path"], labels, dol)
        v["lib"] = reg["lib"]
        v["cflags"] = reg["cflags"]
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
        warn += "  <- suggested C++ (mangled callee only, not conclusive)"
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
              "derives from it; %d more are only *suggested* C++ (mangled callees) and must not be "
              "renamed on that alone; %d lib(s) set `-lang` in cflags."
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
        print("  suggested only (mangled callees - reported, never renamed on this alone): %d" % n_sug, file=out)
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
        return
    total = len(s["units"])
    print("%d registered unit(s); oracle=%s" % (total, "map+DOL" if s["oracle"] else "unavailable"), file=out)
    sp = s["split"]
    print("  conclusive c++   %d" % sp["conclusive_cpp"], file=out)
    print("  suggested c++    %d   (mangled callees only - reported, not a verdict)" % sp["suggested"], file=out)
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
        v = unit_verdict(main_dir, match["path"], labels, dol)
        v["lib"], v["cflags"] = match["lib"], match["cflags"]
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
        import io
        s = sweep(ROOT)
        check("sweep covers every registered unit", len(s["units"]), len(registered_units(ROOT)))
        check("the sweep is JSON-serialisable (no tuple keys)", json.dumps(s)[:1], "{")
        check("the sweep reports the known 800CCFB0 extension disagreement",
              any(r["path"].endswith("800CCFB0_fn_800CCFB0.c") for r in s["extension_disagreements"]), True)
        check("800CCFB0's verdict is C++/high in the sweep",
              [(r["lang"], r["confidence"]) for r in s["units"]
               if r["path"].endswith("800CCFB0_fn_800CCFB0.c")], [("c++", "high")])
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

        # the counter-example: auto/800FD520_fn_800FD520 calls mangled SetRootMtxTrans__... and was
        # reconstructed as `.c`, matching at 100.00 % with all 26 relocations identical. Its mangled
        # callee must stay a suggestion and must not appear in the rename list.
        cx = os.path.join(ROOT, "build", "RMHE08", "obj", "auto", "800FD520_fn_800FD520.o")
        if os.path.exists(cx):
            v = object_verdict(cx, labels, dol)
            check("the 800FD520 counter-example is C++/medium", (v["lang"], v["confidence"]), ("c++", "medium"))
            check("the 800FD520 counter-example is not conclusive", (v["conclusive"], v["suggested"]), (False, True))
            check("800FD520 does not drive an extension rename",
                  any(r["path"].endswith("800FD520_fn_800FD520.c") for r in s["extension_disagreements"]),
                  False)
            check("800FD520 is listed as suggested",
                  any(r["path"].endswith("800FD520_fn_800FD520.c") for r in s["suggested"]), True)

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
