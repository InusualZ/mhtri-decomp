#!/usr/bin/env python3
"""Plan the migration of `Type_name(Type* self, ...)` free functions to `Type::name` members (rule 13).

docs/plan.md section 6.5 rule 13: a free function named `<Type>_<name>` whose first parameter is the type's own
`self` is a member function spelled the C way. This tool is the read-only planner for fixing one: it lists, per
function, where it is declared and defined, every reference in `src/` and `include/` (comment-aware), the
member signature to write, the mangled name the compiler will then emit, and the map row it must rename.

    python tools/units/methodize.py NetworkSingleTcp             # the plan for one type
    python tools/units/methodize.py --all                        # every type that has a finding
    python tools/units/methodize.py NetworkSingleTcp --batch out.txt   # a `symedit.py rename-batch` input
    python tools/units/methodize.py NetworkSingleTcp --exact     # confirm each mangling with the real compiler
    python tools/units/methodize.py --selftest

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

SYMBOLS = os.path.join("config", "RMHE08", "symbols.txt")
CTOR_LIKE_RE = re.compile(r"^(?:construct|ctor|dtor|destruct|destroy|delete)(?:$|[A-Z_0-9])", re.I)
MAP_ROW_RE = re.compile(r"^(\S+)\s*=\s*([.\w]+):(0x[0-9A-Fa-f]+);?\s*(.*)$")


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
    """`ret Type::name(params) [const]` - the member to declare in the class and define."""
    return "%s %s::%s(%s)%s" % (f["ret"] or "void", f["owner"], f["method"], ", ".join(f["params"]),
                                " const" if f["const_self"] else "")


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
                m = MAP_ROW_RE.match(line.strip())
                if m and m.group(1) == head:
                    rows[head] = {"section": m.group(2), "address": m.group(3), "attrs": m.group(4)}
    return rows


def exact_mangling(f: dict) -> str | None:
    """The compiler's name for the member, or None: forward-declare what the signature names and compile it."""
    idents = set()
    for chunk in list(f["params"]) + [f["ret"]]:
        idents.update(re.findall(r"[A-Za-z_]\w*", chunk))
    known = set(mg._PRIMITIVE_CODES) | mg._QUALIFIERS | {"unsigned", "signed", "long", "short", "int", "char", "void"}
    fwd = "".join("struct %s;\n" % i for i in sorted(idents - known - {f["owner"]}) if i[:1].isupper())
    params = ", ".join(f["params"])
    snippet = ("%sstruct %s;\n%sstruct %s { %s %s(%s)%s; };\n%s %s::%s(%s)%s"
               % (fwd, f["owner"], "", f["owner"], f["ret"] or "void", f["method"], params,
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
        out.append({"name": name, "type": first["owner"], "method": first["method"],
                    "signature": member_signature(first), "declarations": decls, "definitions": defs,
                    "references": references(sources, name, skip), "mangled": mangled, "mangled_how": how,
                    "map_row": rows.get(name), "note": note})
    return out


def render(entries: list[dict]) -> str:
    lines: list[str] = []
    for e in entries:
        lines.append("%s -> %s::%s" % (e["name"], e["type"], e["method"]))
        lines.append("  member:      %s" % e["signature"])
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
            lines.append("  mangled:     %s%s" % ("~" if e["mangled_how"] != "exact" else "", e["mangled"]))
            if e["mangled_how"] != "exact":
                lines.append("               (%s; confirm with --exact before renaming the map row)"
                             % e["mangled_how"])
        if e["note"]:
            lines.append("  note:        %s" % e["note"])
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
        elif e["mangled_how"] == "exact":
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
            "int Tcp_raw(Tcp* self);\nTcp* Tcp_getInstance(void);\n")
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
              [x["name"] for x in e], ["Tcp_construct", "Tcp_send"])
        send = e[1]
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
        check("the plan for another type is empty", plan(tmp, "Nope"), [])
        check("--all covers every type with a finding", [x["name"] for x in plan(tmp, None)],
              ["Tcp_construct", "Tcp_send"])
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
        e[1]["mangled_how"] = "exact"
        b2 = batch_text(e)
        check("a compiler-confirmed name is an active `old new` row",
              [l for l in b2.splitlines() if l and not l.startswith("#")], ["Tcp_send send__3TcpFPCUcl"])
        check("the batch file is what symedit's parser reads",
              [tuple(l.split("#")[0].split()[:2]) for l in b2.splitlines() if l.split("#")[0].strip()],
              [("Tcp_send", "send__3TcpFPCUcl")])
        check("the tree was not touched (no file written by the planner)",
              sorted(os.listdir(os.path.join(tmp, "src", "mod"))), ["other.cpp", "tcp.cpp"])
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
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if not args.type and not args.all:
        ap.error("name a type, or pass --all")
    root = args.root or repo_root()
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
