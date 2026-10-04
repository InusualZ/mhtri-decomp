#!/usr/bin/env python3
"""Resolve names in symbols.txt against the runtime dump's Dolphin symbol map.
Spec: docs/tools/spec/dumpmap.md. CLI: dumpmap.py lookup ADDR|NAME | join [--kind K] [--section S] [--limit N] [--json] [--stats-only]
[--file F] [--dump F] [--member M] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import collections
import json
import os
import re
import sys
import zipfile
from tools.lib import names as libnames

from tools.lib.project import SymbolMap  # the one symbols.txt parser
from tools.lib.project.ownership import SYMBOLS_REL

HERE = os.path.dirname(os.path.abspath(__file__))

DEFAULT_DUMP = os.environ.get("MHTRI_DUMP_SYMBOLS", "D:/WiiExperiment/DumpSymbols.zip")

# `NAME ADDR FLAG`, the flag glued to the next entry's name when the dumper lost a newline.
ENTRY_RE = re.compile(r"(?P<name>\S.*?)\s+(?P<addr>[0-9a-fA-F]{8})\s+(?P<flag>[fl])")
ZZ_RE = re.compile(r"^zz_[0-9a-fA-F]{6,8}_?$")
FUN_RE = re.compile(r"^FUN_[0-9a-fA-F]{6,8}$")
# Ghidra's auto-names: `<kind>_<address>`.  The address tail is checked separately as well, because
# `s_<text>_<addr>` and `u_MonsterHunter3_8056f4a0` carry more than the address.
GHIDRA_RE = re.compile(r"^(?:word|byte|dword|qword|u|s|FLOAT|DOUBLE|BOOL|BYTE|switchdataD|switchdata"
                       r"|DAT|PTR|LAB|UNK|off|field|zz|FUN)_[0-9a-fA-F]{6,8}_?$")
# The character set symbols.txt actually uses (plus a leading `$`, which MWCC emits).
USABLE_RE = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$]*$")


def is_generated(name: str) -> bool:
    return libnames.is_generated(name, "map")


def base_name(name: str) -> str:
    """The name without the demangled argument list and without MWCC's `__F...` mangling suffix."""
    n = name.split("(", 1)[0]
    i = n.find("__F")
    if i > 0:
        n = n[:i]
    return n


def is_placeholder(name: str, address: int) -> bool:
    """True when the dump's name is not a real name: `zz_`, `FUN_`, or a Ghidra auto-name.

    Ghidra labels unnamed data `s_<text>_<addr>`, `FLOAT_<addr>`, `u_<addr>`, `switchdataD_<addr>`
    and unnamed code `FUN_<addr>`.  The `<prefix>_<8hex>` shape covers most of them; the extra
    address-tail test catches `s_<text>_<addr>` and the dumper's `zz_005b988_` spelling, and is
    what tells a real `_savefpr_14` apart from a generated name.  A `word_<addr>` that does not
    match its own address is still a Ghidra name (the dumper attached it to the wrong address),
    so the prefix list is checked unconditionally.
    """
    if ZZ_RE.match(name) or FUN_RE.match(name) or GHIDRA_RE.match(name):
        return True
    tail = name.rsplit("_", 1)[-1]
    if len(tail) >= 6 and re.fullmatch(r"[0-9a-fA-F]{6,8}", tail):
        try:
            return (int(tail, 16) & 0xFFFFFF) == (address & 0xFFFFFF)
        except ValueError:
            return False
    return False


def parse_map_text(text: str) -> list[dict]:
    """Parse the Dolphin `.map` member into entries (a line may hold one or two of them)."""
    out = []
    for lineno, line in enumerate(text.splitlines(), 1):
        i = 0
        while i < len(line):
            m = ENTRY_RE.match(line, i)
            if not m:
                break
            name = m.group("name").strip()
            address = int(m.group("addr"), 16)
            head = name.split("(", 1)[0]
            out.append({
                "name": name,
                "address": address,
                "flag": m.group("flag"),
                "lineno": lineno,
                "mangled": "__F" in name,
                "placeholder": is_placeholder(name, address),
                "clean": base_name(name),
                "signature": name[len(head):] or "",
            })
            i = m.end()
    return out


def index_by_address(entries: list[dict]) -> dict[int, list[dict]]:
    index: dict[int, list[dict]] = collections.defaultdict(list)
    for e in entries:
        index[e["address"]].append(e)
    return index


def load_dump(path: str = DEFAULT_DUMP, member: str | None = None) -> tuple[dict[int, list[dict]], str]:
    """Read the dump's `.map` member into an address index.  Raises SystemExit on a bad path."""
    if not os.path.exists(path):
        raise SystemExit("dump not found: %s (see docs/memory-dump.md; override with --dump)" % path)
    with zipfile.ZipFile(path) as z:
        members = z.namelist()
        if member is None:
            maps = [n for n in members if n.lower().endswith(".map")]
            if not maps:
                raise SystemExit("no .map member in %s: %s" % (path, ", ".join(members)))
            member = maps[0]
        if member not in members:
            raise SystemExit("member %r not in %s: %s" % (member, path, ", ".join(members)))
        text = z.read(member).decode("latin-1")
    return index_by_address(parse_map_text(text)), member


def map_rows(path: str) -> list[dict]:
    """The symbols.txt side, via the one map parser (never read the 4.5 MB file directly)."""
    rows = [e.to_dict() for e in SymbolMap(path).rows()]
    rows.sort(key=lambda r: (r["address"], r["section"], r["name"]))
    return rows


def choose_proposal(row: dict, entries: list[dict]) -> str | None:
    """The name to propose for one map row, or None when the map is already right."""
    real = [e for e in entries if not e["placeholder"]]
    if not real:
        return None
    mb = base_name(row["name"])
    for e in real:
        if e["name"] == row["name"] or e["name"].lower() == row["name"].lower() or e["clean"] == mb:
            return None
    for e in real:  # the map's own mangling convention first: `all_reset__Fv`, not `all_reset`
        if e["mangled"] and USABLE_RE.match(e["name"]):
            return e["name"]
    for e in real:
        if USABLE_RE.match(e["clean"]):
            return e["clean"]
    return None


def classify(row: dict, entries: list[dict], name_index: dict, proposal_counts: dict,
             dump_counts: dict | None = None) -> dict:
    """Classify one map row against the dump's entries at its address."""
    address = row["address"]
    real = [e for e in entries if not e["placeholder"]]
    sigs = [e["signature"] for e in real if e["signature"]]
    out = {
        "section": row["section"],
        "address": address,
        "map_name": row["name"],
        "map_type": row.get("type", ""),
        "map_size": row.get("size", 0),
        "dump_names": [e["name"] for e in entries],
        "dump_flags": [e["flag"] for e in entries],
        "signature": sigs[0] if sigs else "",
        "proposed_name": None,
        "confidence": "high",
        "subkind": "",
        "warnings": [],
        "reason": "",
        "conflict_with": [],
    }
    if not real:
        out.update(kind="confirm", subkind="placeholder",
                   reason="dump has no name here (zz_/address-embedded placeholder)")
        return out
    mb = base_name(row["name"])
    if any(e["name"] == row["name"] or e["name"].lower() == row["name"].lower() or e["clean"] == mb
           for e in real):
        out.update(kind="confirm", subkind="name", reason="map name already matches the dump")
        return out
    generated = is_generated(row["name"])
    proposed = choose_proposal(row, entries)
    if proposed is None:
        out.update(kind="rename", subkind="demangled", confidence="review",
                   reason="dump name is demangled (%s) - the map's convention wants its mangled form"
                          % real[0]["clean"])
        return out
    out["proposed_name"] = proposed
    others = [x for x in name_index.get(proposed, [])
              if not (x["name"] == row["name"] and x["section"] == row["section"]
                      and x["address"] == address)]
    if others:
        out.update(kind="conflict", subkind="taken",
                   conflict_with=[{"section": x["section"], "address": x["address"], "name": x["name"]}
                                  for x in others],
                   reason="the dump name is already used in the map by another symbol")
        return out
    if proposal_counts.get(proposed, 0) > 1:
        out.update(kind="conflict", subkind="ambiguous",
                   reason="the same dump name is proposed for %d addresses - ambiguous"
                          % proposal_counts[proposed])
        return out
    if generated:
        out.update(kind="rename", subkind="generated", confidence="high", reason="generated name")
    else:
        out.update(kind="rename", subkind="differing", confidence="review",
                   reason="map name is a real name that differs from the dump - confirm the rename")
    out["warnings"] = warnings_for(row, entries, proposed, dump_counts or {})
    return out


def warnings_for(row: dict, entries: list[dict], proposed: str, dump_counts: dict) -> list[str]:
    """Why a reviewer should look twice at a proposal that the rule called high confidence."""
    warns = []
    e = next((x for x in entries if x["name"] == proposed or x["clean"] == proposed), None)
    if len(proposed) < 6:
        warns.append("short name - check it is not a dropped class prefix")
    if re.match(r"^_[0-9a-fA-F]{7,8}[A-Za-z]", proposed):
        warns.append("embeds another address (module-prefixed Dolphin name)")
    if e is not None and e["mangled"] and "function" not in row.get("type", ""):
        warns.append("dump name is a mangled function but the map symbol is not a function")
    if dump_counts.get(proposed, 0) > 1:
        warns.append("the dump uses this name at %d addresses" % dump_counts[proposed])
    return warns


def join(rows: list[dict], dump_index: dict[int, list[dict]]) -> list[dict]:
    """Classify every map row the dump knows, ascending by address."""
    name_index: dict[str, list[dict]] = collections.defaultdict(list)
    for r in rows:
        name_index[r["name"]].append(r)
    counts: dict[str, int] = collections.Counter()
    dump_counts: dict[str, int] = collections.Counter()
    for entries in dump_index.values():
        for e in entries:
            if not e["placeholder"] and USABLE_RE.match(e["clean"]):
                dump_counts[e["clean"]] += 1
    for r in rows:
        entries = dump_index.get(r["address"])
        if not entries:
            continue
        p = choose_proposal(r, entries)
        if p:
            counts[p] += 1
    out = []
    for r in rows:
        entries = dump_index.get(r["address"])
        if not entries:
            continue
        out.append(classify(r, entries, name_index, counts, dump_counts))
    out.sort(key=lambda x: (x["address"], x["section"], x["map_name"]))
    return out


SUMMARY_KEYS = ("rename", "rename_high", "rename_review", "rename_flagged", "confirm",
                "confirm_name", "confirm_placeholder", "conflict", "conflict_taken",
                "conflict_ambiguous", "addresses", "map_symbols", "no_dump_entry")


def summarize(rows: list[dict], total_map: int) -> dict:
    s: dict[str, int] = collections.Counter()
    for r in rows:
        s[r["kind"]] += 1
        s[r["kind"] + "_" + r["subkind"]] += 1
        if r["kind"] == "rename":
            s["rename_" + r["confidence"]] += 1
            if r.get("warnings"):
                s["rename_flagged"] += 1
    s["addresses"] = len(rows)
    s["map_symbols"] = total_map
    s["no_dump_entry"] = total_map - len(rows)
    return {k: s.get(k, 0) for k in SUMMARY_KEYS}


KINDS = ("rename", "confirm", "conflict")


def render_table(rows: list[dict], limit: int | None) -> str:
    lines = ["%-8s %-7s %-21s %-30s %-38s %s"
             % ("kind", "conf", "address", "map name", "proposed name", "dump name(s) / note"),
             "-" * 150]
    shown = rows if limit is None else rows[:limit]
    for r in shown:
        dump = "; ".join(r["dump_names"])
        if r["signature"]:
            dump += " " + r["signature"]
        note = r["reason"]
        if r.get("warnings"):
            note += " | warn: " + "; ".join(r["warnings"])
        if r["conflict_with"]:
            note += " [%s]" % ", ".join("%s:0x%08X %s" % (c["section"], c["address"], c["name"])
                                        for c in r["conflict_with"])
        lines.append("%-8s %-7s %-21s %-30s %-38s %s"
                     % (r["kind"], r["confidence"],
                        "%s:0x%08X" % (r["section"], r["address"]),
                        r["map_name"], r["proposed_name"] or "-", "%s  | %s" % (dump, note)))
    if limit is not None and len(rows) > limit:
        lines.append("... (%d more rows, raise --limit)" % (len(rows) - limit))
    return "\n".join(lines)


def render_summary(s: dict) -> str:
    return ("join: %d map symbols | %d with a dump entry | rename %d (high %d, review %d, flagged %d) "
            "| confirm %d (name %d, placeholder %d) | conflict %d (taken %d, ambiguous %d) | "
            "no dump entry %d"
            % (s.get("map_symbols", 0), s.get("addresses", 0), s.get("rename", 0),
               s.get("rename_high", 0), s.get("rename_review", 0), s.get("rename_flagged", 0),
               s.get("confirm", 0), s.get("confirm_name", 0), s.get("confirm_placeholder", 0),
               s.get("conflict", 0), s.get("conflict_taken", 0), s.get("conflict_ambiguous", 0),
               s.get("no_dump_entry", 0)))


def parse_query(q: str) -> tuple[int | None, str | None]:
    q = q.strip()
    if q.lower().startswith("0x") or re.fullmatch(r"[0-9a-fA-F]{8}", q):
        return int(q, 16), None
    return None, q


def lookup_records(rows: list[dict], dump_index: dict[int, list[dict]],
                   address: int | None, name: str | None) -> list[dict]:
    """One record per address: the dump side and the symbols.txt side together."""
    if address is not None:
        addrs = [address]
    else:
        addrs = sorted({r["address"] for r in rows if r["name"] == name})
        if not addrs:  # maybe the query is a dump name
            addrs = sorted(a for a, es in dump_index.items()
                           if any(e["name"] == name or e["clean"] == name for e in es))
    recs = []
    for a in addrs:
        mine = [r for r in rows if r["address"] == a]
        dumps = [{"name": e["name"], "clean": e["clean"], "flag": e["flag"],
                  "placeholder": e["placeholder"], "mangled": e["mangled"],
                  "signature": e["signature"]} for e in dump_index.get(a, [])]
        if not mine and not dumps:
            continue
        recs.append({
            "address": a,
            "section": mine[0]["section"] if mine else "",
            "map": [{"name": r["name"], "section": r["section"], "type": r.get("type", ""),
                     "size": r.get("size", 0)} for r in mine],
            "dump": dumps,
        })
    return recs


def render_lookup(recs: list[dict], query: str) -> str:
    if not recs:
        return "no match for %r in the map or the dump" % query
    lines = []
    for rec in recs:
        dumps = "; ".join(d["name"] for d in rec["dump"]) or "-"
        sig = next((d["signature"] for d in rec["dump"] if d["signature"]), "")
        maps = "; ".join("%s (%s%s)" % (m["name"], m["type"] or "?",
                                        (", size 0x%X" % m["size"]) if m["size"] else "")
                         for m in rec["map"]) or "-"
        sec = rec["section"] or "?"
        lines.append("0x%08X  %-11s  dump=%-46s  sig=%-14s  map=%s"
                     % (rec["address"], sec, dumps, sig or "-", maps))
    return "\n".join(lines)


def selftest() -> int:
    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # --- parsing ---------------------------------------------------------------
    e = parse_map_text("CntSdRsoTerminate 800406ac f")
    check("one plain entry", [(x["name"], x["address"], x["flag"]) for x in e],
          [("CntSdRsoTerminate", 0x800406AC, "f")])
    e = parse_map_text("kbd_open(unsigned 80040798 f")
    check("demangled arg list kept", (e[0]["name"], e[0]["clean"], e[0]["signature"]),
          ("kbd_open(unsigned", "kbd_open", "(unsigned"))
    e = parse_map_text("zz_00e8888_ 800e8888 fgetInstance() 800e89d8 l")
    check("glued pair splits into two", [(x["name"], x["address"], x["flag"]) for x in e],
          [("zz_00e8888_", 0x800E8888, "f"), ("getInstance()", 0x800E89D8, "l")])
    e = parse_map_text("LexicalCast<b,i>(int 803e24f8 fzz_03e250c_ 803e250c l")
    check("space in a name does not split the entry",
          [(x["name"], x["address"]) for x in e],
          [("LexicalCast<b,i>(int", 0x803E24F8), ("zz_03e250c_", 0x803E250C)])
    text = "a 80000000 fb 80000004 l\nall_reset(void) 800cef9c f\nall_reset__Fv 800cef9c f"
    joined = "".join("%s%08x%s" % (x["name"], x["address"], x["flag"]) for x in parse_map_text(text))
    check("reconstruction is lossless", joined.replace(" ", ""), text.replace("\n", "").replace(" ", ""))
    check("two names at one address",
          len([x for x in parse_map_text(text) if x["address"] == 0x800CEF9C]), 2)

    # --- naming rules ----------------------------------------------------------
    check("zz_ placeholder", is_placeholder("zz_8005b988_", 0x8005B988), True)
    check("zz_ 7-digit placeholder", is_placeholder("zz_005b988_", 0x8005B988), True)
    check("FUN_ placeholder", is_placeholder("FUN_80041148", 0x80041148), True)
    check("address-embedded data name", is_placeholder("s_FVKIN_809b0f00", 0x809B0F00), True)
    check("address-embedded FLOAT", is_placeholder("FLOAT_8079878c", 0x8079878C), True)
    check("Ghidra name attached to another address", is_placeholder("word_807C0156", 0x80040CA8), True)
    check("Ghidra u_ text name", is_placeholder("u_MonsterHunter3_8056f4a0", 0x8056F4A0), True)
    check("real name is not a placeholder", is_placeholder("CntSdRsoTerminate", 0x800406AC), False)
    check("_savefpr_14 is not a placeholder", is_placeholder("_savefpr_14", 0x80456D54), False)
    check("embedded-looking tail that is not the address",
          is_placeholder("thing_123456", 0x80999999), False)
    check("base of a mangled name", base_name("kbd_init__FUc"), "kbd_init")
    check("base of __dl", base_name("__dl__FPv"), "__dl")
    check("base of a demangled name", base_name("cCameraManager::PushWorldUpVector(void)"),
          "cCameraManager::PushWorldUpVector")
    check("base of all_reset(void)", base_name("all_reset(void)"), "all_reset")
    check("generated fn_", is_generated("fn_8004054C"), True)
    check("generated lbl_", is_generated("lbl_8057C82C"), True)
    check("generated dtor_", is_generated("dtor_8005E5E8"), True)
    check("generated @etb_", is_generated("@etb_800066E0"), True)
    check("generated unk50", is_generated("unk50"), True)
    check("a real name is not generated", is_generated("main"), False)
    check("a mangled real name is not generated", is_generated("kbd_init__FUc"), False)
    check("usable plain name", bool(USABLE_RE.match("recvAnsFmpListVersion")), True)
    check("demangled name is not usable", bool(USABLE_RE.match("ns::C::m")), False)
    check("template name is not usable", bool(USABLE_RE.match("LexicalCast<b,i>")), False)

    # --- classify --------------------------------------------------------------
    def row(name, address, section=".text", type_="function", size=4):
        return {"name": name, "address": address, "section": section, "type": type_, "size": size}

    def entries(*specs):
        return parse_map_text("\n".join(specs))

    def one(r, spec, index=None, counts=None, dcounts=None):
        es = entries(spec)
        return classify(r, es, index or {}, counts or {}, dcounts or {})

    c = one(row("fn_8003F4D8", 0x8003F4D8), "recvAnsFmpListVersion 8003f4d8 f")
    check("plain dump name -> high rename", (c["kind"], c["confidence"], c["proposed_name"]),
          ("rename", "high", "recvAnsFmpListVersion"))
    c = one(row("fn_80050674", 0x80050674),
            "scaleMat34W__FPQ34nw4r4math5MTX34PQ34nw4r4math4VEC3 80050674 f")
    check("mangled dump name -> high rename", (c["kind"], c["confidence"], c["proposed_name"]),
          ("rename", "high", "scaleMat34W__FPQ34nw4r4math5MTX34PQ34nw4r4math4VEC3"))
    c = one(row("fn_8003F554", 0x8003F554), "cCameraManager::PushWorldUpVector(void) 8003f554 f")
    check("demangled-only -> review with no proposal",
          (c["kind"], c["confidence"], c["proposed_name"]), ("rename", "review", None))
    check("demangled reason says so", "demangled" in c["reason"], True)
    c = one(row("kbd_init__FUc", 0x8004074C), "kbd_init(unsigned 8004074c f")
    check("map name already right -> confirm", (c["kind"], c["subkind"]), ("confirm", "name"))
    c = one(row("fn_80040598", 0x80040598), "zz_0040598_ 80040598 f")
    check("dump placeholder -> confirm", (c["kind"], c["subkind"]), ("confirm", "placeholder"))
    c = one(row("change_widemode_req__FUc", 0x8003FBE8), "sendReqLayerStart 8003fbe8 f")
    check("real map name that differs -> review rename",
          (c["kind"], c["confidence"], c["subkind"]), ("rename", "review", "differing"))
    c = one(row("fn_80467918", 0x80467918), "cos 80467918 f")
    check("short name is flagged", any("short name" in w for w in c["warnings"]), True)
    c = one(row("fn_8003F4D8", 0x8003F4D8), "recvAnsFmpListVersion 8003f4d8 f")
    check("a plain name is not flagged for having no argument list", c["warnings"], [])
    c = one(row("fn_8003F4D8", 0x8003F4D8), "recvAnsFmpListVersion 8003f4d8 f",
            dcounts={"recvAnsFmpListVersion": 3})
    check("a dump name used at several dump addresses is flagged",
          any("3 addresses" in w for w in c["warnings"]), True)
    c = one(row("lbl_80602198", 0x80602198, ".data", "label"),
            "_80411a88PatInterface_VTable 80602198 l")
    check("a module-prefixed Dolphin name is flagged",
          any("embeds another address" in w for w in c["warnings"]), True)
    c = one(row("kbd_init__FUc", 0x8004074C), "kbd_init(unsigned 8004074c f")
    check("a confirmation carries no warnings", c["warnings"], [])
    taken = {"main": [row("main", 0x8003F218)]}
    c = one(row("__start", 0x80006310, section=".init"), "main 80006310 f", index=taken)
    check("name already used elsewhere -> conflict",
          (c["kind"], c["subkind"], c["conflict_with"][0]["name"]), ("conflict", "taken", "main"))
    c = one(row("fn_8003FC58", 0x8003FC58), "DBClose 8003fc58 f", counts={"DBClose": 2})
    check("same name at two addresses -> ambiguous",
          (c["kind"], c["subkind"]), ("conflict", "ambiguous"))
    c = one(row("fn_8003FC58", 0x8003FC58), "DBClose 8003fc58 f", counts={"DBClose": 1})
    check("one address only -> not ambiguous", c["kind"], "rename")
    sib = {"_savefpr_14": [row("_savefpr_14", 0x80456D3C)]}
    c = one(row("__save_fpr", 0x80456D3C), "_savefpr_14 80456d3c f", index=sib)
    check("sibling at the same address still conflicts",
          (c["kind"], c["subkind"]), ("conflict", "taken"))
    check("case-insensitive confirmation",
          one(row("CntSdRsoTerminate", 0x800406AC), "cntsdrsoTerminate 800406ac f")["kind"], "confirm")

    # --- join / summarize / render --------------------------------------------
    rows = [row("fn_80040598", 0x80040598), row("fn_800406AC", 0x800406AC),
            row("fn_8003F4D8", 0x8003F4D8), row("kbd_init__FUc", 0x8004074C),
            row("lbl_80000000", 0x80000000, ".data", "label", 4)]
    dindex = index_by_address(entries("zz_0040598_ 80040598 f",
                                      "CntSdRsoTerminate 800406ac f",
                                      "recvAnsFmpListVersion 8003f4d8 f",
                                      "kbd_init(unsigned 8004074c f"))
    jr = join(rows, dindex)
    check("join skips addresses the dump does not know", len(jr), 4)
    check("join is ascending by address", [r["address"] for r in jr],
          [0x8003F4D8, 0x80040598, 0x800406AC, 0x8004074C])
    s = summarize(jr, len(rows))
    check("summary counts", (s["rename"], s["confirm"], s["conflict"], s["no_dump_entry"]),
          (2, 2, 0, 1))
    check("summary splits confirmations",
          (s["confirm_name"], s["confirm_placeholder"]), (1, 1))
    check("summary splits rename confidence", (s["rename_high"], s["rename_review"]), (2, 0))
    check("summary counts flagged renames", s["rename_flagged"], 0)
    check("summary string is one line", render_summary(s).count("\n"), 0)
    t = render_table(jr, limit=2)
    check("limit truncates rows", t.count("more rows"), 1)
    check("table names the kinds", all(k in t for k in ("kind", "confirm", "rename")), True)
    check("no-limit table has every row", render_table(jr, None).count("more rows"), 0)

    # --- lookup ---------------------------------------------------------------
    lrows = [row("fn_800406AC", 0x800406AC), row("kbd_init__FUc", 0x8004074C),
             row("fn_80040798", 0x80040798)]
    lindex = index_by_address(entries("CntSdRsoTerminate 800406ac f",
                                      "kbd_init(unsigned 8004074c f",
                                      "kbd_open(unsigned 80040798 f"))
    check("query 0x... is an address", parse_query("0x800406AC"), (0x800406AC, None))
    check("query 800406AC is an address", parse_query("800406AC"), (0x800406AC, None))
    check("query a name", parse_query("CntSdRsoTerminate"), (None, "CntSdRsoTerminate"))
    rec = lookup_records(lrows, lindex, 0x800406AC, None)
    check("lookup by address finds both sides",
          (rec[0]["map"][0]["name"], rec[0]["dump"][0]["name"]),
          ("fn_800406AC", "CntSdRsoTerminate"))
    rec = lookup_records(lrows, lindex, None, "kbd_init__FUc")
    check("lookup by map name resolves to the dump name",
          rec[0]["dump"][0]["clean"], "kbd_init")
    rec = lookup_records(lrows, lindex, None, "kbd_open")
    check("lookup by dump name resolves to the map address",
          (rec[0]["address"], rec[0]["map"][0]["name"]), (0x80040798, "fn_80040798"))
    check("lookup of an unknown name is empty", lookup_records(lrows, lindex, None, "nope"), [])
    check("lookup renders one line per address",
          render_lookup(rec, "kbd_open").count("\n"), 0)
    check("lookup line carries the signature", "(unsigned" in render_lookup(rec, "kbd_open"), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("command", nargs="?", choices=("lookup", "join"))
    ap.add_argument("query", nargs="?", help="lookup: an address or a name")
    ap.add_argument("--file", default=SYMBOLS_REL, help="the symbol map (symbols.txt)")
    ap.add_argument("--dump", default=DEFAULT_DUMP, help="DumpSymbols.zip (or $MHTRI_DUMP_SYMBOLS)")
    ap.add_argument("--member", default=None, help="zip member (default: the first *.map)")
    ap.add_argument("--section", default=None, help="only this section (.text, .data, ...)")
    ap.add_argument("--kind", default="rename,conflict",
                    help="join: rename,confirm,conflict or all (default: rename,conflict)")
    ap.add_argument("--limit", type=int, default=40, help="join: rows to print (default 40)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--stats-only", action="store_true", help="join: print only the summary")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.command:
        ap.print_help()
        return 0

    dump_index, member = load_dump(args.dump, args.member)
    rows = map_rows(args.file)
    if args.section:
        rows = [r for r in rows if r["section"] == args.section]

    if args.command == "lookup":
        if not args.query:
            ap.error("lookup needs an address or a name")
        address, name = parse_query(args.query)
        recs = lookup_records(rows, dump_index, address, name)
        if args.json:
            print(json.dumps({"member": member, "query": args.query, "records": recs}, indent=2))
        else:
            print(render_lookup(recs, args.query))
        return 0 if recs else 1

    jr = join(rows, dump_index)
    s = summarize(jr, len(rows))
    kinds = KINDS if args.kind == "all" else tuple(k.strip() for k in args.kind.split(",") if k.strip())
    bad = [k for k in kinds if k not in KINDS]
    if bad:
        ap.error("unknown --kind %s (use %s or all)" % (", ".join(bad), ", ".join(KINDS)))
    shown = [r for r in jr if r["kind"] in kinds]
    if args.json:
        print(json.dumps({"member": member, "dump": args.dump, "file": args.file,
                          "stats": s, "kinds": list(kinds), "limit": args.limit,
                          "rows": shown[:args.limit]}, indent=2))
        return 0
    print(render_summary(s))
    if not args.stats_only:
        print(render_table(shown, args.limit))
    return 0


if __name__ == "__main__":
    sys.exit(main())
