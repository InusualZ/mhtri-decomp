#!/usr/bin/env python3
"""Plan the migration of `Type_name(Type* self, ...)` free functions to `Type::name` members (rule 13).

docs/plan.md section 6.5 rule 13: a free function named `<Type>_<name>` whose first parameter is the type's own
`self` is a member function spelled the C way; the **static form** (owner ruling 2026-09-29) is the same name with
no `Type* self` (`GameSpyInterfaceThread_getInstance(void)`), a **static member**: the plan then reads
`static R name(args);` in the class, `R Type::name(args)` for the definition, callers `Type::name(...)`, and the
compiler's mangling is the plain member one with no `this` and no `C` (`getInstance__22GameSpyInterfaceThreadFv`).
Each plan entry says `kind: member` or `kind: static`. This tool is the read-only planner for fixing one: it lists, per
function, where it is declared and defined, every reference in `src/` and `include/` (comment-aware), the
member signature to write, the mangled name the compiler will then emit, and the map row it must rename.

    python tools/units/methodize.py NetworkSingleTcp             # the plan for one type
    python tools/units/methodize.py --all                        # every type that has a finding
    python tools/units/methodize.py NetworkSingleTcp --batch out.txt   # a `symedit.py rename-batch` input
    python tools/units/methodize.py NetworkSingleTcp --exact     # confirm each mangling with the real compiler
    python tools/units/methodize.py --class NetworkTcp --fn networkPeer_send=send --fn networkPeer_recv=recv \
        --unit Network/network_socket_streams.cpp --batch out.txt      # explicit mapping (below)
    python tools/units/methodize.py --map networkPeer_send=NetworkTcp::send --map-file map.json
    python tools/units/methodize.py --selftest

Explicit mapping: when the free functions do not share the class's prefix (`networkPeerStream_*` -> `NetworkByteStream`,
`networkPeer_*` -> Tcp or Udp by address order), name each `old=Class::member` (`--map`, repeatable; `--map-file` is a
JSON object of the same pairs; `--class C --fn old=member` is the short form). Each function is found by its exact name
(no prefix or `Type* self` naming needed): a first parameter of type `Class*`/`const Class*`/`Class&` is the `this`
(kind member), anything else makes it a static member. The plan adds, per function, the exact edits it can determine
(`edits`: the definition header rewritten to `Class::member(...)`, a declaration to move into the class, every call
site rewritten to `obj->member(...)` / `Class::member(...)`) and writes the rename-batch. The mangling is estimated by
`mangle.py`; with `--unit <src path>` (or `--obj <file.o>`) it is VERIFIED against the function symbols of that built
object: a name the object defines is `verified`, and a Class::member the object defines under a *different* mangling
refuses the plan (exit 2, no batch written). An object built before the migration only carries the old names and
leaves the estimate unconfirmed (say so, rebuild, rerun). Two mappings that mangle to one name are refused.

It **never edits source or the map**. A lane does the source half (declare in the class, define `Type::name`,
sweep the call sites) and measures it (playbook 60: `this` arrives in r3 like the old `self`, so a non-virtual
method should be codegen-neutral, but the declaration set is a codegen input); the orchestrator applies the
map half with `python tools/symbols/symedit.py rename-batch <file>`.

How the findings are found: it calls `stylelint.rule13_findings` over `src/` and `include/` (one rule, one
implementation), so a function the lint exempts with `/* free: <reason> */` is not planned. The mangled name
is `mangle.estimate_member_mangling` (a pure-text estimate, marked `~`) unless `--exact` compiles the member
through `mangle.mangle` (needs the build tree's compiler). **In the batch file only an exact name is an active
rename**; an estimate is written as a comment, because a wrong map name un-pairs the symbol in objdiff. A
constructor/destructor-shaped name (`construct`, `ctor`, `dtor`, `destruct`, `init`-less) is flagged: its
mangling is `__ct__`/`__dt__`, not the method form, so no rename is proposed for it.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))

import mangle as mg  # noqa: E402
import stylelint as sl  # noqa: E402
from tools.lib import project as _project  # noqa: E402  (the map line parser)

SYMBOLS = os.path.join("config", "RMHE08", "symbols.txt")
CTOR_LIKE_RE = re.compile(r"^(?:construct|ctor|dtor|destruct|destroy|delete)(?:$|[A-Z_0-9])", re.I)
CONFIRMED = ("exact", "verified")


def repo_root() -> str:
    p = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return p.stdout.strip() if p.returncode == 0 and p.stdout.strip() else os.getcwd()


# --------------------------------------------------------------------------------------------------
# the plan
# --------------------------------------------------------------------------------------------------
def collect(root: str) -> tuple[list[dict], list[sl.Source]]:
    """Every rule-13 finding under `src/` and `include/` (exempted declarations excluded) and the sources read."""
    sl.set_rule13_context(root)
    sources = list(sl.all_sources(root))
    sources += [sl.Source(path, sl.rel_of(root, path), sl.read_text(path)) for path in sl.header_files(root)]
    findings: list[dict] = []
    for src in sources:
        findings.extend(sl.rule13_findings(src))
    return findings, sources


def member_signature(f: dict) -> str:
    """`[static] ret Type::name(params) [const]` - the member to declare in the class and define."""
    return "%s%s %s::%s(%s)%s" % ("static " if f.get("static") else "", f["ret"] or "void", f["owner"],
                                  f["method"], ", ".join(f["params"]), " const" if f["const_self"] else "")


def references(sources: list[sl.Source], name: str, skip: set) -> list[dict]:
    """Every use of `name` in comment- and string-stripped code, minus the declaration/definition lines."""
    rx = re.compile(r"\b%s\b" % re.escape(name))
    out = []
    for src in sources:
        for m in rx.finditer(src.code):
            line = src.line_of(m.start())
            if (src.rel, line) in skip:
                continue
            after = src.code[m.end():m.end() + 40].lstrip()
            out.append({"file": src.rel, "line": line, "call": after.startswith("("),
                        "text": src.line_text(line).strip()[:140]})
    return out


def map_rows(root: str, names: set) -> dict:
    """`{name: {"section", "address", "attrs"}}` for exactly `names` - streamed, the file is never printed."""
    path = os.path.join(root, SYMBOLS)
    rows: dict = {}
    if not names or not os.path.isfile(path):
        return rows
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            head = line.split(" ", 1)[0]
            if head in names:
                e = _project.parse_line(line.strip())
                if e is not None and e.name == head:
                    rows[head] = {"section": e.section, "address": "0x%08X" % e.address,
                                  "attrs": e.line.partition(";")[2].strip()}
    return rows


def exact_mangling(f: dict) -> str | None:
    """The compiler's name for the member, or None: forward-declare what the signature names and compile it."""
    idents = set()
    for chunk in list(f["params"]) + [f["ret"]]:
        idents.update(re.findall(r"[A-Za-z_]\w*", chunk))
    known = set(mg._PRIMITIVE_CODES) | mg._QUALIFIERS | {"unsigned", "signed", "long", "short", "int", "char", "void"}
    fwd = "".join("struct %s;\n" % i for i in sorted(idents - known - {f["owner"]}) if i[:1].isupper())
    params = ", ".join(f["params"])
    snippet = ("%sstruct %s;\n%sstruct %s { %s%s %s(%s)%s; };\n%s %s::%s(%s)%s"
               % (fwd, f["owner"], "", f["owner"], "static " if f.get("static") else "", f["ret"] or "void",
                  f["method"], params,
                  " const" if f["const_self"] else "", f["ret"] or "void", f["owner"], f["method"], params,
                  " const" if f["const_self"] else ""))
    try:
        names, _ = mg.mangle(snippet)
    except SystemExit:
        return None
    return names[0] if len(names) == 1 else None


def plan(root: str, type_name: str | None, exact: bool = False) -> list[dict]:
    """One entry per distinct function of `type_name` (all types when None), in type then name order."""
    findings, sources = collect(root)
    by_name: dict = {}
    for f in findings:
        if type_name is None or f["owner"] == type_name:
            by_name.setdefault(f["token"], []).append(f)
    rows = map_rows(root, set(by_name))
    out = []
    for name in sorted(by_name, key=lambda n: (by_name[n][0]["owner"], n)):
        fs = by_name[name]
        first = fs[0]
        decls = [{"file": f["file"], "line": f["line"]} for f in fs if not f["has_body"]]
        defs = [{"file": f["file"], "line": f["line"]} for f in fs if f["has_body"]]
        skip = {(f["file"], f["line"]) for f in fs}
        note = None
        mangled, how = first["mangled"], "estimated"
        if CTOR_LIKE_RE.match(first["method"]):
            mangled, how = None, "none"
            note = ("constructor/destructor-shaped: the compiler spells `__ct__`/`__dt__`, so this is a "
                    "constructor/destructor, not a plain member - decide it by hand")
        elif exact:
            ex = exact_mangling(first)
            if ex:
                mangled, how = ex, "exact"
            else:
                how = "estimated (the exact compile failed)" if mangled else "none"
        if mangled is None and note is None:
            note = "no mangling estimate (a shape the estimator will not guess) - rerun with --exact"
        is_static = bool(first.get("static"))
        out.append({"name": name, "type": first["owner"], "method": first["method"],
                    "kind": "static" if is_static else "member",
                    "call_form": ("%s::%s(...)" % (first["owner"], first["method"])) if is_static
                    else "obj->%s(...)" % first["method"],
                    "signature": member_signature(first), "declarations": decls, "definitions": defs,
                    "references": references(sources, name, skip), "mangled": mangled, "mangled_how": how,
                    "map_row": rows.get(name), "note": note})
    return out


# --------------------------------------------------------------------------------------------------
# explicit mapping: `old=Class::member`, no prefix convention needed
# --------------------------------------------------------------------------------------------------
def parse_mapping(pairs: list[str], class_name: str | None = None, fns: list[str] | None = None) -> dict:
    """`{old: (Class, member)}` from `old=Class::member` pairs plus `--class C --fn old=member`."""
    out: dict = {}
    for text in pairs:
        old, sep, tgt = text.partition("=")
        cls, sep2, member = tgt.partition("::")
        if not (sep and sep2 and old.strip() and cls.strip() and member.strip()):
            raise ValueError("bad --map %r (want old=Class::member)" % text)
        out[old.strip()] = (cls.strip(), member.strip())
    for text in fns or []:
        old, sep, member = text.partition("=")
        if not (sep and class_name and old.strip() and member.strip()):
            raise ValueError("bad --fn %r (want old=member, with --class)" % text)
        out[old.strip()] = (class_name, member.strip())
    return out


def _call_sites(src: "sl.Source", name: str, skip: set) -> list[tuple[int, int, list[str]]]:
    """`(start, end, args)` of every call `name(args)` in code (offsets into the file), minus `skip` lines."""
    out = []
    for m in re.finditer(r"\b%s\b\s*\(" % re.escape(name), src.code):
        if (src.rel, src.line_of(m.start())) in skip:
            continue
        open_pos = m.end() - 1
        depth, close = 0, None
        for i in range(open_pos, len(src.code)):
            c = src.code[i]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    close = i
                    break
        if close is None:
            continue
        chunks = sl._split_parameters(src.code, open_pos + 1, close) if close > open_pos + 1 else []
        args = [src.text[o:o + len(c)].strip() for c, o in chunks]
        out.append((m.start(), close + 1, args))
    return out


def _receiver(arg: str) -> str:
    """`obj->` / `obj.` text for a `this` argument expression."""
    if arg == "this":
        return "this->"
    if arg.startswith("&") and re.fullmatch(r"[A-Za-z_][\w.\[\]>-]*", arg[1:]):
        return arg[1:] + "."
    if re.fullmatch(r"[A-Za-z_][\w.\[\]>-]*(?:\([^()]*\))?", arg):
        return arg + "->"
    return "(%s)->" % arg


def plan_mapping(root: str, mapping: dict, obj_names: list[str] | None = None,
                 exact: bool = False) -> tuple[list[dict], list[str]]:
    """`(entries, errors)` for an explicit `{old: (Class, member)}` mapping; entries are `plan()`-shaped plus `edits`.

    `obj_names` is the function symbol list of the unit's built object (verification), or None to skip it.
    """
    sl.set_rule13_context(root)
    sources = list(sl.all_sources(root))
    sources += [sl.Source(path, sl.rel_of(root, path), sl.read_text(path)) for path in sl.header_files(root)]
    sl.set_rule13_context(None)
    errors: list[str] = []
    rows = map_rows(root, set(mapping))
    entries: list[dict] = []
    for old in mapping:
        cls, member = mapping[old]
        decls, defs, first, edits, skip = [], [], None, [], set()
        found = []
        for src in sources:
            code = sl._mask_preproc(src.code)
            for d in sl.function_declarations(src):
                if d["name"] != old:
                    continue
                chunks = sl._split_parameters(code, d["params_pos"], d["params_pos"] + len(d["params"]))
                found.append((src, d, [c.strip() for c, _o in chunks]))
        if not found:
            errors.append("%s: no declaration or definition found in src/ or include/" % old)
            continue
        for src, d, chunks in found:
            first_p = chunks[0] if chunks else ""
            selfy = sl._rule13_self_type(first_p)
            is_member = selfy is not None and selfy[0] == cls
            rest = chunks[1:] if is_member else ([] if first_p in ("", "void") else chunks)
            const_self = bool(is_member and selfy[1])
            ret = d["ret"].strip()
            sig = {"ret": ret or "void", "owner": cls, "method": member, "params": rest, "const_self": const_self,
                   "static": not is_member}
            if first is None:
                first = sig
            elif (first["params"], first["ret"], first["static"], first["const_self"]) != (
                    rest, sig["ret"], sig["static"], const_self):
                errors.append("%s: declaration and definition disagree (%s:%d)" % (old, src.rel, d["line"]))
            line = d["line"]
            skip.add((src.rel, line))
            tail = " const" if const_self else ""
            has_body = bool(d.get("body"))
            (defs if has_body else decls).append({"file": src.rel, "line": line})
            if has_body:
                edits.append({"kind": "define", "file": src.rel, "line": line,
                              "new": "%s %s::%s(%s)%s" % (ret, cls, member, ", ".join(rest), tail)})
            else:
                edits.append({"kind": "declare", "file": src.rel, "line": line,
                              "new": "%s%s %s(%s)%s;  // move into %s, delete this free declaration"
                                     % ("static " if not is_member else "", ret, member, ", ".join(rest), tail, cls)})
        mangled, how = None, "estimated"
        if CTOR_LIKE_RE.match(member):
            note = "constructor/destructor-shaped member name: decide its `__ct__`/`__dt__` spelling by hand"
        else:
            note = None
            mangled = mg.estimate_member_mangling(cls, member, first["params"], const_self=first["const_self"])
            if exact:
                ex = exact_mangling(first)
                if ex:
                    mangled, how = ex, "exact"
            if mangled is None:
                note = "no mangling estimate (a shape the estimator will not guess) - rerun with --exact"
        refs = references(sources, old, skip)
        for src in sources:
            for start, end, args in _call_sites(src, old, skip):
                if first["static"]:
                    new = "%s::%s(%s)" % (cls, member, ", ".join(args))
                elif args:
                    new = "%s%s(%s)" % (_receiver(args[0]), member, ", ".join(args[1:]))
                else:
                    continue
                edits.append({"kind": "call", "file": src.rel, "line": src.line_of(start), "new": new,
                              "old": src.text[start:end]})
        entries.append({"name": old, "type": cls, "method": member,
                        "kind": "static" if first["static"] else "member",
                        "call_form": ("%s::%s(...)" % (cls, member)) if first["static"] else "obj->%s(...)" % member,
                        "signature": member_signature(first), "declarations": decls, "definitions": defs,
                        "references": refs, "mangled": mangled, "mangled_how": how, "map_row": rows.get(old),
                        "note": note, "edits": edits})
    seen: dict = {}
    for e in entries:
        if e["mangled"]:
            if e["mangled"] in seen:
                errors.append("%s and %s both mangle to %s" % (seen[e["mangled"]], e["name"], e["mangled"]))
            seen.setdefault(e["mangled"], e["name"])
    if obj_names is not None:
        errors.extend(verify_names(entries, obj_names))
    return entries, errors


def verify_names(entries: list[dict], names: list[str]) -> list[str]:
    """Confirm each estimate against the built object's function symbols; a differing mangling is an error."""
    errors: list[str] = []
    have = set(names)
    for e in entries:
        if not e["mangled"]:
            continue
        if e["mangled"] in have:
            e["mangled_how"] = "verified"
            continue
        tag = "__%d%s" % (len(e["type"]), e["type"])
        cands = sorted(n for n in have if n.startswith(e["method"] + "__") and tag in n)
        if cands:
            errors.append("%s: the object defines %s but the estimate is %s (mangling mismatch, refusing)"
                          % (e["name"], ", ".join(cands), e["mangled"]))
        elif e["name"] in have:
            e["note"] = ((e["note"] + "; ") if e["note"] else "") + \
                "the object still carries the old name (built before the migration): estimate not verified"
        else:
            e["note"] = ((e["note"] + "; ") if e["note"] else "") + \
                "the object defines neither the old name nor a %s::%s: estimate not verified" % (e["type"], e["method"])
    return errors


def object_for_unit(root: str, unit: str) -> str:
    """`build/RMHE08/src/<unit minus extension>.o` for a source path like `Network/x.cpp` (or `src/Network/x.cpp`)."""
    unit = unit.replace("\\", "/")
    if unit.startswith("src/"):
        unit = unit[4:]
    return os.path.join(root, "build", "RMHE08", "src", os.path.splitext(unit)[0] + ".o")


def render(entries: list[dict]) -> str:
    lines: list[str] = []
    for e in entries:
        lines.append("%s -> %s::%s" % (e["name"], e["type"], e["method"]))
        lines.append("  member:      %s  [kind: %s; call as %s]" % (e["signature"], e["kind"], e["call_form"]))
        for kind, key in (("declared", "declarations"), ("defined", "definitions")):
            for d in e[key]:
                lines.append("  %-12s %s:%d" % (kind + ":", d["file"], d["line"]))
        calls = [r for r in e["references"] if r["call"]]
        other = [r for r in e["references"] if not r["call"]]
        lines.append("  references:  %d call site(s), %d other" % (len(calls), len(other)))
        for r in calls + other:
            lines.append("    %s:%d%s  %s" % (r["file"], r["line"], "" if r["call"] else " (not a call)",
                                              r["text"]))
        row = e["map_row"]
        lines.append("  map row:     %s"
                     % ("%s = %s:%s %s" % (e["name"], row["section"], row["address"], row["attrs"]) if row
                        else "none - the name is not in the map (no rename to apply)"))
        if e["mangled"]:
            lines.append("  mangled:     %s%s" % ("" if e["mangled_how"] in CONFIRMED else "~", e["mangled"]))
            if e["mangled_how"] in CONFIRMED:
                lines.append("               (%s)" % e["mangled_how"])
            else:
                lines.append("               (%s; confirm with --exact before renaming the map row)"
                             % e["mangled_how"])
        if e["note"]:
            lines.append("  note:        %s" % e["note"])
        for ed in e.get("edits", []):
            lines.append("  edit %-8s %s:%d  %s" % (ed["kind"], ed["file"], ed["line"], ed["new"]))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n" if lines else "no rule-13 findings for that selection.\n"


def batch_text(entries: list[dict]) -> str:
    """A `symedit.py rename-batch` input: an exact mangling is an active row, an estimate is commented out."""
    out = ["# symedit.py rename-batch input from tools/units/methodize.py (old new); apply with",
           "#   python tools/symbols/symedit.py rename-batch <this file> --dry-run",
           "# an active row is a compiler-confirmed mangling; a `#~` row is an estimate - confirm it (--exact)"]
    for e in entries:
        if not e["map_row"] or not e["mangled"]:
            out.append("# %s: %s" % (e["name"], "not in the map" if not e["map_row"] else e["note"]))
        elif e["mangled_how"] in CONFIRMED:
            out.append("%s %s" % (e["name"], e["mangled"]))
        else:
            out.append("#~ %s %s" % (e["name"], e["mangled"]))
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------------------------------
# selftest - a temporary tree, never the real one
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    with tempfile.TemporaryDirectory() as tmp:
        def put(rel: str, text: str) -> None:
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)

        put("include/mod/tcp.h",
            'struct Tcp {\n    int a;\n};\ns32 Tcp_send(Tcp* self, const u8* data, s32 size);\n'
            "void Tcp_construct(Tcp* self);\n/* free: retail C linkage, the dump names it unmangled */\n"
            "int Tcp_raw(Tcp* self);\nTcp* Tcp_getInstance(void);\nvoid Tcp_ctor(Other* out);\n")
        put("src/mod/tcp.cpp",
            '#include "mod/tcp.h"\ns32 Tcp_send(Tcp* self, const u8* data, s32 size) {\n    return size;\n}\n'
            "void Tcp_construct(Tcp* self) {\n}\n"
            "void user(Tcp* t) {\n    Tcp_send(t, 0, 1); // Tcp_send in a comment\n"
            '    const char* s = "Tcp_send";\n    void (*fp)() = (void(*)())&Tcp_send;\n}\n')
        put("src/mod/other.cpp", '#include "mod/tcp.h"\nvoid o(Tcp* t) {\n    Tcp_send(t, 0, 2);\n}\n')
        put("config/RMHE08/symbols.txt",
            "Tcp_send = .text:0x803CE558; // type:function size:0x64\n"
            "Tcp_sender = .text:0x803CE600; // type:function size:0x10\n")
        e = plan(tmp, "Tcp")
        check("the functions of the type, alphabetically, exempt one excluded",
              [x["name"] for x in e], ["Tcp_construct", "Tcp_getInstance", "Tcp_send"])
        send = e[2]
        check("the member signature", send["signature"], "s32 Tcp::send(const u8* data, s32 size)")
        check("the declaration and the definition are told apart",
              ([d["file"] for d in send["declarations"]], [d["file"] for d in send["definitions"]]),
              (["include/mod/tcp.h"], ["src/mod/tcp.cpp"]))
        check("references: comments and strings are not references, the call sites and the address-of are",
              sorted((r["file"], r["line"], r["call"]) for r in send["references"]),
              [("src/mod/other.cpp", 3, True), ("src/mod/tcp.cpp", 8, True), ("src/mod/tcp.cpp", 10, False)])
        check("the map row is looked up by EXACT name (`Tcp_sender` is not it)",
              send["map_row"], {"section": ".text", "address": "0x803CE558", "attrs": "// type:function size:0x64"})
        check("the mangled name is estimated and marked so",
              (send["mangled"], send["mangled_how"]), ("send__3TcpFPCUcl", "estimated"))
        check("a constructor-shaped name proposes no rename and says why",
              (e[0]["mangled"], "constructor" in (e[0]["note"] or "")), (None, True))
        check("a name absent from the map has no map row", e[0]["map_row"], None)
        gi = e[1]
        check("a static-form finding is planned as a static member",
              (gi["kind"], gi["signature"], gi["call_form"], gi["mangled"]),
              ("static", "static Tcp* Tcp::getInstance()", "Tcp::getInstance(...)", "getInstance__3TcpFv"))
        check("a member-form finding says how it is called", (send["kind"], send["call_form"]),
              ("member", "obj->send(...)"))
        check("`Type_ctor(Other*)` is a C-style helper, not planned", "Tcp_ctor" in [x["name"] for x in e], False)
        check("the plan for another type is empty", plan(tmp, "Nope"), [])
        check("--all covers every type with a finding", [x["name"] for x in plan(tmp, None)],
              ["Tcp_construct", "Tcp_getInstance", "Tcp_send"])
        text = render(e)
        check("the human plan names the member, the sites and the map row",
              all(s in text for s in ("Tcp::send", "include/mod/tcp.h:4", "src/mod/other.cpp:3",
                                      "Tcp_send = .text:0x803CE558", "~send__3TcpFPCUcl")), True)
        check("the human plan does not print the whole map (only the row asked for)",
              "Tcp_sender" in text, False)
        b = batch_text(e)
        check("an estimate is a commented row, never an active rename",
              [l for l in b.splitlines() if l and not l.startswith("#")], [])
        check("... and carries the `#~ old new` form", "#~ Tcp_send send__3TcpFPCUcl" in b, True)
        e[2]["mangled_how"] = "exact"
        b2 = batch_text(e)
        check("a compiler-confirmed name is an active `old new` row",
              [l for l in b2.splitlines() if l and not l.startswith("#")], ["Tcp_send send__3TcpFPCUcl"])
        check("the batch file is what symedit's parser reads",
              [tuple(l.split("#")[0].split()[:2]) for l in b2.splitlines() if l.split("#")[0].strip()],
              [("Tcp_send", "send__3TcpFPCUcl")])
        check("the tree was not touched (no file written by the planner)",
              sorted(os.listdir(os.path.join(tmp, "src", "mod"))), ["other.cpp", "tcp.cpp"])

    # explicit mapping: two prefixes, a Tcp/Udp-style duplicated body pair
    with tempfile.TemporaryDirectory() as tmp:
        def put(rel, text):
            path = os.path.join(tmp, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
        put("include/net/peer.h",
            "struct Tcp {\n    int a;\n};\nstruct Udp {\n    int a;\n};\nstruct Stream {\n    int a;\n};\n"
            "s32 netPeer_send(Tcp* self, const u8* data, s32 size);\n"
            "s32 netPeerB_send(Udp* self, const u8* data, s32 size);\n"
            "s32 netPeerC_send(Tcp* self, const u8* data, s32 size);\n"
            "s32 netStream_size(const Stream* s);\nStream* netStream_make(void);\n")
        put("src/net/peer.cpp",
            '#include "net/peer.h"\n'
            "s32 netPeer_send(Tcp* self, const u8* data, s32 size) {\n    return size;\n}\n"
            "s32 netPeerB_send(Udp* self, const u8* data, s32 size) {\n    return size;\n}\n"
            "s32 netStream_size(const Stream* s) {\n    return 1;\n}\n"
            "void user(Tcp* t, Udp* u, Stream* s) {\n    netPeer_send(t, 0, 1);\n    netPeerB_send(&g_udp, 0, 2);\n"
            "    netStream_size(s);\n    netStream_make();\n}\n")
        put("config/RMHE08/symbols.txt",
            "".join("%s = .text:0x8000%04X; // type:function size:0x10\n" % (n, i * 16)
                    for i, n in enumerate(["netPeer_send", "netPeerB_send", "netStream_size", "netStream_make"])))
        mp = parse_mapping(["netPeer_send=Tcp::send", "netPeerB_send=Udp::send", "netStream_size=Stream::size"],
                           "Stream", ["netStream_make=make"])
        check("mapping parse", mp["netStream_make"], ("Stream", "make"))
        ents, errs = plan_mapping(tmp, mp)
        by = {e["name"]: e for e in ents}
        check("no errors on a clean mapping", errs, [])
        check("Tcp/Udp duplicated pair mangles apart",
              (by["netPeer_send"]["mangled"], by["netPeerB_send"]["mangled"]),
              ("send__3TcpFPCUcl", "send__3UdpFPCUcl"))
        check("const self -> C, static form has none",
              (by["netStream_size"]["mangled"], by["netStream_make"]["kind"], by["netStream_make"]["mangled"]),
              ("size__6StreamCFv", "static", "make__6StreamFv"))
        calls = sorted(x["new"] for e in ents for x in e["edits"] if x["kind"] == "call")
        check("call sites are rewritten (member, address-of receiver, static)", calls,
              sorted(["Stream::make()", "s->size()", "t->send(0, 1)", "g_udp.send(0, 2)"]))
        check("a definition edit drops the self",
              [x["new"] for x in by["netPeer_send"]["edits"] if x["kind"] == "define"],
              ["s32 Tcp::send(const u8* data, s32 size)"])
        check("the map row is found by exact name", by["netPeer_send"]["map_row"] is not None, True)
        good = ["send__3TcpFPCUcl", "send__3UdpFPCUcl", "size__6StreamCFv", "make__6StreamFv"]
        ents2, errs2 = plan_mapping(tmp, mp, obj_names=good)
        check("names present in the object are verified", (errs2, {e["mangled_how"] for e in ents2}),
              ([], {"verified"}))
        b = batch_text(ents2)
        check("a verified name is an active batch row",
              sorted(l for l in b.splitlines() if l and not l.startswith("#")),
              sorted(["netPeer_send send__3TcpFPCUcl", "netPeerB_send send__3UdpFPCUcl",
                      "netStream_size size__6StreamCFv", "netStream_make make__6StreamFv"]))
        _e3, errs3 = plan_mapping(tmp, mp, obj_names=["send__3TcpFPCUc"] + good[1:])
        check("a different mangling in the object is refused", len(errs3), 1)
        check("... and names the mismatch", "mismatch" in errs3[0] and "send__3TcpFPCUc" in errs3[0], True)
        ents4, errs4 = plan_mapping(tmp, mp, obj_names=["netPeer_send", "netPeerB_send", "netStream_size",
                                                        "netStream_make"])
        check("a pre-migration object is not a mismatch, just unverified",
              (errs4, {e["mangled_how"] for e in ents4}), ([], {"estimated"}))
        _e5, errs5 = plan_mapping(tmp, {"netPeer_send": ("Tcp", "send"), "netPeerC_send": ("Tcp", "send")})
        check("two mappings mangling to one name are refused", any("both mangle" in x for x in errs5), True)
        _e6, errs6 = plan_mapping(tmp, {"nope": ("Tcp", "x")})
        check("an unknown function is an error", len(errs6), 1)
        check("the explicit plan does not write the tree",
              sorted(os.listdir(os.path.join(tmp, "src", "net"))), ["peer.cpp"])
    sl.set_rule13_context(None)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("type", nargs="?", help="the class/struct whose `Type_name(Type* self)` functions to plan")
    ap.add_argument("--all", action="store_true", help="every type with a rule-13 finding")
    ap.add_argument("--root", default=None, help="the tree to read (default: this git tree)")
    ap.add_argument("--batch", metavar="FILE", help="write a `symedit.py rename-batch` input here")
    ap.add_argument("--exact", action="store_true", help="confirm each mangling with the compiler (needs build/)")
    ap.add_argument("--map", action="append", default=[], metavar="OLD=CLASS::MEMBER",
                    help="explicit mapping (repeatable); no prefix convention needed")
    ap.add_argument("--map-file", metavar="JSON", help="a JSON object {old: \"Class::member\"}")
    ap.add_argument("--class", dest="cls", help="with --fn: the class every --fn maps into")
    ap.add_argument("--fn", action="append", default=[], metavar="OLD=MEMBER")
    ap.add_argument("--unit", help="the unit's source (Network/x.cpp): verify manglings against its built object")
    ap.add_argument("--obj", help="verify against this object file's function symbols")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    root = args.root or repo_root()
    if args.map or args.map_file or args.fn:
        pairs = list(args.map)
        if args.map_file:
            with open(args.map_file, encoding="utf-8") as fh:
                pairs += ["%s=%s" % kv for kv in json.load(fh).items()]
        try:
            mapping = parse_mapping(pairs, args.cls, args.fn)
        except ValueError as ex:
            ap.error(str(ex))
        obj = args.obj or (object_for_unit(root, args.unit) if args.unit else None)
        names = None
        if obj:
            if not os.path.isfile(obj):
                print("the object %s is not built; manglings stay estimates (build it to verify)" % obj)
            else:
                import unitutil
                names = unitutil.function_names(obj)
        entries, errors = plan_mapping(root, mapping, names, exact=args.exact)
        sys.stdout.write((json.dumps(entries, indent=1) + "\n") if args.json else render(entries))
        if errors:
            for e in errors:
                print("REFUSED: " + e)
            return 2
        if args.batch:
            with open(args.batch, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(batch_text(entries))
            print("batch file: %s" % args.batch)
        return 0
    if not args.type and not args.all:
        ap.error("name a type, or pass --all")
    entries = plan(root, None if args.all else args.type, exact=args.exact)
    if args.batch:
        with open(args.batch, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(batch_text(entries))
    if args.json:
        print(json.dumps(entries, indent=1))
    else:
        sys.stdout.write(render(entries))
        if args.batch:
            print("batch file: %s" % args.batch)
    return 0


if __name__ == "__main__":
    sys.exit(main())
