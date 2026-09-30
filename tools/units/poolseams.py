#!/usr/bin/env python3
"""poolseams.py - the literal pool as TU-seam evidence: which registered units are ONE original translation unit.

The premise (docs/pool-seams.md, playbook idea 94, measured 2026-09-30): MWCC emits **one literal pool per
translation unit**, one entry per distinct value, in first-use order, and `mwldeppc` does not merge pools across
objects.  So a pool literal (an `.sdata2` float/double, an `.sdata` string) whose address is read by two registered
units means those units are **one** original TU that the registry has cut into pieces - a *fold candidate*, and a
unit that is a partial pool of that TU can never reproduce its `.sdata2`/`.sdata` alone.  The converse: the same
value at two addresses inside what a registry unit treats as one TU cannot happen (the compiler would have reused
the first entry), so such a unit already spans more than one TU.

This module turns that into data.  It reads nothing itself: the references come from `datagap.census` (the one
relocation reader over the registered units' TARGET objects), the claims from `splits.txt`, the symbol types from
the map, the values (optional) from the retail DOL.  Everything below is a pure function of those, so the
`--selftest` fixtures need no build tree.

    python tools/units/poolseams.py                 # the census: groups of registered units that share a pool
    python tools/units/poolseams.py --unit <unit>   # the group a unit belongs to (or "no pool-sharing group")
    python tools/units/poolseams.py --json out.json
    python tools/units/poolseams.py --selftest

(`datagap.py --pool-seams` is the same census; `tudiscover.py at`, `datagap.py` deferral classes, `flipcheck.py`,
`sectiongap.py` and `brief.py` consume `group_of`.)

What is and is not evidence (measured exceptions, docs/pool-seams.md section 4):

* **literal** - an `.sdata2` object of 4 or 8 bytes, or an `.sdata` string: an edge.
* **non-literal** - any other `.sdata2`/`.sdata` object, and every `.data`/`.bss`/`.sbss`/`.rodata` object: a named
  global, a table or a variable.  Several TUs reference those legitimately; never an edge.
* a literal whose value is the int->float magic `0x43300000_80000000` / `0x43300000_00000000` is still a per-TU pool
  entry (the linker does not synthesise it), so it stays an edge and is only *counted* (`magic`).
* a unit that holds one value at two pool addresses **spans several TUs** (`coarse`): its group is reported, but its
  pool is not a single run, so its adjacency/order verdicts are demoted.
"""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
for _p in (TOOLS, HERE):
    if _p not in sys.path:
        sys.path.insert(0, _p)

LITERAL_SECTIONS = (".sdata2", ".sdata")
MAGIC = (bytes.fromhex("4330000080000000"), bytes.fromhex("4330000000000000"))
#: a text gap this small between two group members is alignment, not foreign code
GAP_SLACK = 0x40


# ---- what is a literal ---------------------------------------------------------------------------------------------

def data_kind(entry: dict) -> str:
    """The `data:` tag of a map row (`float`, `double`, `string`, ...), or ''."""
    line = entry.get("line") or ""
    i = line.find(" data:")
    if i < 0:
        return ""
    return line[i + 6:].split()[0] if line[i + 6:].split() else ""


def is_literal(entry: dict) -> bool:
    """True for an object MWCC pools per TU: an `.sdata2` 4/8-byte scalar or an `.sdata` string."""
    sec, size = entry.get("section"), int(entry.get("size") or 0)
    if sec == ".sdata2":
        return size in (4, 8)
    if sec == ".sdata":
        return data_kind(entry) == "string"
    return False


def literal_table(symbols: dict) -> dict[int, dict]:
    """`{address: entry}` for the map's literal rows (the map is keyed by name; a pool is keyed by address)."""
    return {e["address"]: e for e in symbols.values() if is_literal(e)}


# ---- edges ---------------------------------------------------------------------------------------------------------

def owner_of(ranges: dict, section: str, address: int) -> str | None:
    for start, end, unit in ranges.get(section, ()):
        if start <= address < end:
            return unit
    return None


def multi_unit_addresses(records: list[dict], ranges: dict) -> dict[tuple, dict]:
    """`{(section, address): {units, owner, entry-ish}}` for every data address two or more registered units touch.

    A unit touches an address when its target object references it (a census record) or claims it (`splits.txt`
    owner: the object *defines* it, so the census does not list it).
    """
    by: dict[tuple, dict] = {}
    for r in records:
        slot = by.setdefault((r["section"], r["address"]),
                             {"section": r["section"], "address": r["address"], "name": r["name"],
                              "size": r["size"], "units": {}, "owner": None})
        slot["units"][r["unit"]] = slot["units"].get(r["unit"], 0) + r["sites"]
    for (section, address), slot in by.items():
        own = owner_of(ranges, section, address)
        if own:
            slot["owner"] = own
            slot["units"].setdefault(own, 0)
    return {k: v for k, v in by.items() if len(v["units"]) >= 2}


def pool_edges(records: list[dict], ranges: dict, lits: dict[int, dict], value_of=None):
    """`(edges, excluded)`: the literal pool-sharing addresses and, per class, the multi-unit ones that are not.

    `edges` is `{address: {section, name, size, units, owner, magic}}`; `excluded` is `{class: count}` with the
    classes `non-literal:<section>` (a named global/table/variable several TUs may read).  `value_of(address, size)`
    returns the literal's bytes (or None) and only serves the `magic` tag.
    """
    edges: dict[int, dict] = {}
    excluded: dict[str, int] = {}
    lits = lits or {}
    for (section, address), slot in sorted(multi_unit_addresses(records, ranges).items()):
        if section in LITERAL_SECTIONS and address in lits:
            raw = value_of(address, slot["size"]) if value_of else None
            edges[address] = {**slot, "magic": bool(raw) and raw in MAGIC}
        else:
            key = "non-literal:%s" % section
            excluded[key] = excluded.get(key, 0) + 1
    return edges, excluded


# ---- groups --------------------------------------------------------------------------------------------------------

def text_spans(ranges: dict) -> dict[str, list[tuple[int, int]]]:
    out: dict[str, list[tuple[int, int]]] = {}
    for start, end, unit in ranges.get(".text", ()):
        out.setdefault(unit, []).append((start, end))
    return {u: sorted(v) for u, v in out.items()}


def components(edges: dict[int, dict]) -> list[set[str]]:
    """Connected components of the units over the pool-sharing addresses (union-find)."""
    parent: dict[str, str] = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for e in edges.values():
        units = sorted(e["units"])
        for u in units[1:]:
            parent[find(u)] = find(units[0])
    groups: dict[str, set[str]] = {}
    for u in list(parent):
        groups.setdefault(find(u), set()).add(u)
    return [g for g in groups.values() if len(g) >= 2]


def adjacency(members: set[str], spans: dict[str, list], all_ranges: list[tuple[int, int, str]]) -> dict:
    """How the group's `.text` ranges sit in address order.

    `adjacent` - nothing registered between the first and last range (a gap no larger than `GAP_SLACK` counts as
    alignment); `gap` - only unregistered bytes between (`gap_bytes`); `interleaved` - registered code of units
    outside the group lies between members (`between`, `foreign_bytes`).  A TU's `.text` is contiguous, so such a
    unit is part of the fold too (it shares no literal with the rest, but it sits inside the span) - the group's
    `closure` names it.
    """
    mine = sorted((s, e, u) for u in members for s, e in spans.get(u, ()))
    if not mine:
        return {"kind": "no-text", "between": [], "gap_bytes": 0, "lo": 0, "hi": 0}
    lo, hi = mine[0][0], max(e for _s, e, _u in mine)
    inside = [(s, e, u) for s, e, u in all_ranges if u not in members and s < hi and e > lo]
    foreign = sorted({u for _s, _e, u in inside})
    foreign_bytes = sum(min(e, hi) - max(s, lo) for s, e, _u in inside)
    covered = sum(min(e, hi) - max(s, lo) for s, e, _u in mine if s < hi and e > lo)
    gap = max(0, (hi - lo) - covered - foreign_bytes)
    kind = "interleaved" if foreign else ("gap" if gap > GAP_SLACK * max(1, len(mine) - 1) else "adjacent")
    return {"kind": kind, "between": foreign, "gap_bytes": gap, "lo": lo, "hi": hi,
            "foreign_bytes": foreign_bytes, "member_bytes": covered}


def first_use_order(members: set[str], edges: dict[int, dict], spans: dict, records_by_unit: dict) -> dict:
    """Do the pool runs follow first-use order (= text order)?  `inversions` counts literal pairs that do not.

    Each literal's first user is the member whose `.text` starts lowest among those touching it; sorted by pool
    address the first-user text starts must not go down.  The edges are only the shared addresses, so the check
    is conservative: a violation is a real disagreement, not noise.
    """
    start = {u: spans[u][0][0] for u in members if spans.get(u)}
    rows = []
    for address, e in sorted(edges.items()):
        users = [u for u in e["units"] if u in members and u in start]
        if users:
            rows.append((address, e["section"], min(start[u] for u in users)))
    inversions = 0
    for section in LITERAL_SECTIONS:
        seq = [s for _a, sec, s in rows if sec == section]
        inversions += sum(1 for a, b in zip(seq, seq[1:]) if b < a)
    return {"inversions": inversions, "checked": len(rows)}


def data_interleave(members: set[str], ranges: dict, refs_by_address: dict, section: str = ".data") -> dict:
    """The `.data` contiguity cross-check: does the group's `.data` interleave data that belongs to someone else?

    A TU's `.data` is contiguous.  `claims` are the members' claimed `.data` ranges; `foreign` is every other unit's
    claim inside their span, plus every object inside it that only non-members reference.
    """
    mine = sorted((s, e) for s, e, u in ranges.get(section, ()) if u in members)
    if not mine:
        return {"section": section, "claims": 0, "foreign": [], "lo": 0, "hi": 0}
    lo, hi = mine[0][0], max(e for _s, e in mine)
    foreign = sorted({u for s, e, u in ranges.get(section, ()) if u not in members and s < hi and e > lo})
    for (sec, address), users in refs_by_address.items():
        if sec == section and lo <= address < hi and users and not (users & members):
            foreign.extend(sorted(u for u in users if u not in foreign))
    return {"section": section, "claims": len(mine), "foreign": sorted(set(foreign)), "lo": lo, "hi": hi}


def coarse_units(records: list[dict], ranges: dict, lits: dict[int, dict], value_of) -> dict[str, list[dict]]:
    """Units that hold one `.sdata2` value at two pool addresses - already more than one TU.

    `{unit: [{value, addresses}]}`.  Needs `value_of`; without it nothing is reported.  Only map rows typed
    `float`/`double` count: `.sdata` strings can be initialised `char[]` objects (never pooled) and an untyped
    or `4byte` word can be half of an 8-byte object the map cut in two (two `0xFFFFFFFF` words read by one `lfd`),
    so two equal words there prove nothing.
    """
    if value_of is None:
        return {}
    touch: dict[str, set[int]] = {}
    typed = {a for a, e in lits.items() if e["section"] == ".sdata2" and data_kind(e) in ("float", "double")}
    for r in records:
        if r["address"] in typed:
            touch.setdefault(r["unit"], set()).add(r["address"])
    for start, end, unit in ranges.get(".sdata2", ()):
        touch.setdefault(unit, set()).update(a for a in typed if start <= a < end)
    out: dict[str, list[dict]] = {}
    for unit, addrs in touch.items():
        seen: dict[tuple, list[int]] = {}
        for a in sorted(addrs):
            e = lits[a]
            raw = value_of(a, int(e.get("size") or 0))
            if raw:
                seen.setdefault((e["section"], raw), []).append(a)
        dups = [{"value": k[1].hex(), "addresses": v} for k, v in seen.items() if len(v) > 1]
        if dups:
            out[unit] = dups
    return out


def confidence(adj: dict, order: dict, data: dict, coarse: bool) -> str:
    """high / medium / low, with the reasons that cap it kept in the group's own fields."""
    if adj["kind"] == "interleaved":
        # the units inside the span are pulled in by contiguity; when they outweigh the members the shared literal
        # is more likely a named global scalar than a pool entry
        return "medium" if adj["foreign_bytes"] <= adj["member_bytes"] and not data["foreign"] else "low"
    if adj["kind"] == "adjacent" and not order["inversions"] and not data["foreign"] and not coarse:
        return "high"
    return "medium"


def build_groups(records: list[dict], ranges: dict, lits: dict[int, dict], value_of=None) -> dict:
    """The whole census: `{groups, excluded, edges, units}`.  Pure given the inputs."""
    edges, excluded = pool_edges(records, ranges, lits, value_of)
    spans = text_spans(ranges)
    all_text = list(ranges.get(".text", ()))
    refs_by_address: dict[tuple, set[str]] = {}
    for r in records:
        refs_by_address.setdefault((r["section"], r["address"]), set()).add(r["unit"])
    coarse = coarse_units(records, ranges, lits, value_of)
    groups = []
    for members in components(edges):
        mine = {a: e for a, e in edges.items() if set(e["units"]) & members}
        adj = adjacency(members, spans, all_text)
        order = first_use_order(members, mine, spans, None)
        data = data_interleave(members, ranges, refs_by_address)
        is_coarse = sorted(u for u in members if u in coarse)
        groups.append({
            "units": sorted(members, key=lambda u: (spans.get(u) or [(1 << 40, 0)])[0][0]),
            "text": {u: [list(x) for x in spans.get(u, ())] for u in members},
            "shared": len(mine), "shared_sections": sorted({e["section"] for e in mine.values()}),
            "magic": sum(1 for e in mine.values() if e["magic"]),
            # literals a data-only unit (no `.text`) owns: a constant table other units read - the place a named
            # global scalar, not a pool entry, is most likely (docs/pool-seams.md section 4)
            "data_owned": sum(1 for e in mine.values() if e["owner"] and not spans.get(e["owner"])),
            "addresses": sorted(mine)[:12],
            "adjacency": adj, "order": order, "data": data, "coarse": is_coarse,
            "closure": adj["between"],
            "confidence": confidence(adj, order, data, bool(is_coarse)),
        })
    groups.sort(key=lambda g: (-len(g["units"]), g["adjacency"]["lo"]))
    # every literal address a registered unit touches (reads or claims): the brief/attribute side asks "does a
    # registered unit already touch this literal?" for a range that is not registered yet
    touch: dict[int, list[str]] = {}
    for r in records:
        if r["section"] in LITERAL_SECTIONS and r["address"] in lits:
            touch.setdefault(r["address"], []).append(r["unit"])
    for section in LITERAL_SECTIONS:
        for start, end, unit in ranges.get(section, ()):
            for a in lits:
                if start <= a < end and lits[a]["section"] == section:
                    touch.setdefault(a, []).append(unit)
    return {"groups": groups, "excluded": excluded, "edges": len(edges),
            "units": sum(len(g["units"]) for g in groups), "coarse": coarse,
            "touch": {a: sorted(set(us)) for a, us in touch.items()},
            "edge_classes": edge_classes(groups, edges)}


def edge_classes(groups: list[dict], edges: dict) -> dict:
    """How many shared literal addresses each confidence class explains (the exception budget, docs section 4)."""
    out = {"high": 0, "medium": 0, "low": 0, "magic": 0, "data_owned": 0}
    for g in groups:
        out[g["confidence"]] += g["shared"]
        out["magic"] += g["magic"]
        out["data_owned"] += g["data_owned"]
    out["total"] = len(edges)
    return out


def group_of(census: dict, unit: str) -> dict | None:
    """The group `unit` (no extension) belongs to, or None."""
    unit = os.path.splitext(unit)[0]
    for g in census["groups"]:
        if unit in g["units"]:
            return g
    return None


# ---- rendering -----------------------------------------------------------------------------------------------------

def fold_line(group: dict, unit: str | None = None) -> str:
    """`candidate fold: a + b + c` with the adjacency/confidence tag - the one sentence other tools quote."""
    others = [u for u in group["units"] if u != unit] if unit else list(group["units"])
    adj = group["adjacency"]["kind"]
    return "candidate fold: %s (%d shared pool literal(s), text %s, confidence %s)" % (
        " + ".join(group["units"]) if unit is None else "%s with %s" % (unit, " + ".join(others)),
        group["shared"], adj, group["confidence"])


def render_group(g: dict, index: int = 0) -> list[str]:
    adj, order, data = g["adjacency"], g["order"], g["data"]
    lines = ["group %d: %d units, %d shared literal(s) [%s]%s, confidence %s" % (
        index, len(g["units"]), g["shared"], ",".join(g["shared_sections"]),
        (", %d int->float magic" % g["magic"]) if g["magic"] else "", g["confidence"])]
    for u in g["units"]:
        spans = g["text"].get(u) or []
        lines.append("    %-34s .text %s" % (u, ", ".join("0x%08X-0x%08X" % tuple(s) for s in spans) or "-"))
    lines.append("    text       %s%s" % (adj["kind"], (" (inside the span, pulled in by contiguity: %s; 0x%X B foreign vs 0x%X B"
                                                        " members)" % (", ".join(adj["between"][:4]), adj["foreign_bytes"],
                                                                       adj["member_bytes"])) if adj["between"]
                                        else (" (+0x%X unregistered)" % adj["gap_bytes"] if adj["kind"] == "gap" else "")))
    lines.append("    pool order %s (%d literal(s) checked)" % (
        "first-use order holds" if not order["inversions"] else "%d inversion(s) against text order" % order["inversions"],
        order["checked"]))
    if data["claims"]:
        lines.append("    .data      %d claim(s) 0x%08X-0x%08X %s" % (
            data["claims"], data["lo"], data["hi"],
            "contiguous" if not data["foreign"] else "INTERLEAVES foreign data of %s - the pool model and the data "
            "model disagree" % ", ".join(data["foreign"][:4])))
    else:
        lines.append("    .data      no claim in the group")
    if g["coarse"]:
        lines.append("    coarse     %s holds one value at two pool addresses: already more than one TU" %
                     ", ".join(g["coarse"][:4]))
    lines.append("    " + fold_line(g))
    return lines


def render_census(c: dict, top: int = 25) -> str:
    groups = c["groups"]
    by_conf: dict[str, int] = {}
    for g in groups:
        by_conf[g["confidence"]] = by_conf.get(g["confidence"], 0) + 1
    lines = ["pool-sharing census: %d group(s) over %d registered unit(s), %d shared literal address(es)"
             % (len(groups), c["units"], c["edges"]),
             "confidence: %s" % ", ".join("%s %d" % kv for kv in sorted(by_conf.items())),
             "shared literals by the group's confidence: %s" %
             ", ".join("%s %d" % (k, c["edge_classes"][k]) for k in ("high", "medium", "low")),
             "  of which int->float magic %d, owned by a data-only unit %d" % (c["edge_classes"]["magic"],
                                                                              c["edge_classes"]["data_owned"]),
             "excluded (multi-unit objects that are not pool literals): %s" %
             (", ".join("%s %d" % kv for kv in sorted(c["excluded"].items())) or "none"),
             "units holding one value at two pool addresses (already several TUs): %d" % len(c["coarse"]), "",
             "  #  units shared  text                      adjacency     confidence  first unit .. last unit"]
    for i, g in enumerate(groups):
        adj = g["adjacency"]
        lines.append("%3d  %5d %6d  0x%08X-0x%08X  %-12s  %-10s  %s .. %s" % (
            i, len(g["units"]), g["shared"], adj["lo"], adj["hi"], adj["kind"], g["confidence"],
            g["units"][0], g["units"][-1]))
    lines.append("")
    for i, g in enumerate(groups[:top]):
        lines.extend(render_group(g, i))
        lines.append("")
    if len(groups) > top:
        lines.append("... %d more group(s) (--top, --json)" % (len(groups) - top))
    return "\n".join(lines).rstrip()


# ---- the tree (readers) --------------------------------------------------------------------------------------------

_CACHE: dict = {}


def load_census(root: str, with_values: bool = True) -> dict:
    """The census for the TARGET objects of `root` (cached per root).  Needs `build/RMHE08/obj` and the map."""
    key = (os.path.abspath(root), with_values)
    if key in _CACHE:
        return _CACHE[key]
    from units import datagap as dg  # noqa: PLC0415 - the census reader; a lazy import keeps this module pure

    ranges = dg.load_claims(root)
    (records, _stats), _n, _read = dg.census(root, None, ranges=ranges)
    symbols = dg.load_data_symbols(root)
    value_of = None
    if with_values:
        dol_path = os.path.join(root, "orig", dg.GAME_DIR, "sys", "main.dol")
        if os.path.exists(dol_path):
            sys.path.insert(0, os.path.join(TOOLS, "splits"))
            import tudiscover as td  # noqa: PLC0415

            dol = td.Dol(dol_path)
            value_of = lambda a, n: dol.read(a, n) if n else None  # noqa: E731
    census = build_groups(records, ranges, literal_table(symbols), value_of)
    _CACHE[key] = census
    return census


def literal_units(root: str) -> dict[int, list[str]]:
    """`{literal address: registered units touching it}` for the tree (empty when unreadable)."""
    try:
        return load_census(root).get("touch", {})
    except Exception:  # noqa: BLE001
        return {}


def pool_seams_for(literals, touch: dict, own_unit: str | None = None) -> dict | None:
    """The registered units that already touch literals a not-yet-registered range reads.

    `literals` are `{address: size}`-free plain addresses of the pool literals the range's functions reference;
    `touch` is `literal_units`' shape.  Returns `{units: [{unit, count, addresses}], shared}` or None.  A literal
    some registered unit touches is one TU's pool entry: the range is a piece of that unit's TU (or the unit is a
    piece of this range's), never an independent TU.
    """
    by_unit: dict[str, list[int]] = {}
    for a in sorted(set(literals)):
        for u in touch.get(a, ()):
            if u != own_unit:
                by_unit.setdefault(u, []).append(a)
    if not by_unit:
        return None
    rows = [{"unit": u, "count": len(v), "addresses": v[:6]} for u, v in sorted(by_unit.items(), key=lambda kv: -len(kv[1]))]
    return {"units": rows, "shared": len({a for v in by_unit.values() for a in v})}


def note_for_unit(root: str, unit: str) -> str | None:
    """One line for a tool that already knows `unit`: the pool-sharing group, or None.  Never raises."""
    try:
        g = group_of(load_census(root), unit)
    except Exception:  # noqa: BLE001 - a guard must not break the tool that calls it
        return None
    return fold_line(g, os.path.splitext(unit)[0]) if g else None


# ---- selftest ------------------------------------------------------------------------------------------------------

def _rec(unit, section, address, name="lbl", size=4, sites=1):
    return {"unit": unit, "section": section, "address": address, "name": name, "size": size, "sites": sites}


def _entry(section, address, size, tag=""):
    return {"name": "lbl_%X" % address, "section": section, "address": address, "size": size, "type": "object",
            "line": "lbl_%X = %s:0x%X; // type:object size:0x%X%s" % (address, section, address, size,
                                                                     (" data:" + tag) if tag else "")}


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # is_literal / data_kind
    check("sdata2 4/8 bytes are literals", (is_literal(_entry(".sdata2", 0x100, 4, "float")),
                                            is_literal(_entry(".sdata2", 0x108, 8, "double"))), (True, True))
    check("an sdata2 table is not", is_literal(_entry(".sdata2", 0x110, 0x20)), False)
    check("an sdata string is a literal, an sdata variable is not",
          (is_literal(_entry(".sdata", 0x200, 3, "string")), is_literal(_entry(".sdata", 0x208, 4, "4byte"))),
          (True, False))
    check(".data and .bss objects never are", (is_literal(_entry(".data", 0x300, 4)), is_literal(_entry(".bss", 0x310, 4))),
          (False, False))
    check("data_kind reads the tag", data_kind(_entry(".sdata", 0x1, 1, "string")), "string")

    # a fixture tree: A B C are one TU (A,B share 0x8000; B,C share 0x8008), D is alone, E shares 0x8020 with A
    ranges = {
        ".text": [(0x1000, 0x1100, "A"), (0x1100, 0x1200, "B"), (0x1200, 0x1300, "C"), (0x1300, 0x1400, "D"),
                  (0x1400, 0x1500, "E")],
        ".sdata2": [(0x8000, 0x8010, "A")],
        ".data": [(0x9000, 0x9010, "A"), (0x9010, 0x9020, "B"), (0x9020, 0x9030, "C")],
    }
    lits = {a: _entry(".sdata2", a, 4, "float") for a in (0x8000, 0x8004, 0x8008, 0x800C, 0x8020, 0x8024)}
    lits[0x8010] = _entry(".sdata2", 0x8010, 8, "double")
    recs = [
        _rec("B", ".sdata2", 0x8000), _rec("B", ".sdata2", 0x8008, sites=2), _rec("C", ".sdata2", 0x8008),
        _rec("D", ".sdata2", 0x8024),                       # a private literal: no edge
        _rec("E", ".sdata2", 0x8020), _rec("A", ".sdata2", 0x8020),
        _rec("A", ".data", 0x9100, "g"), _rec("D", ".data", 0x9100, "g"),   # a shared global: excluded
        _rec("C", ".sdata2", 0x8010, size=8), _rec("B", ".sdata2", 0x8010, size=8),
    ]
    edges, excluded = pool_edges(recs, ranges, lits)
    check("edges are the literals two units touch (owner counts)", sorted(edges), [0x8000, 0x8008, 0x8010, 0x8020])
    check("the owner of an sdata2 address is one of its units", sorted(edges[0x8000]["units"]), ["A", "B"])
    check("a shared global is excluded, by class", excluded, {"non-literal:.data": 1})
    check("components: A B C E are one group, D is not in any",
          sorted(sorted(g) for g in components(edges)), [["A", "B", "C", "E"]])
    c = build_groups(recs, ranges, lits)
    g = c["groups"][0]
    check("one group, units in text order", (len(c["groups"]), g["units"]), (1, ["A", "B", "C", "E"]))
    check("D sits between C and E in text, so the group is interleaved", (g["adjacency"]["kind"], g["adjacency"]["between"]),
          ("interleaved", ["D"]))
    check("a small unit inside the span is pulled in by contiguity: medium, and named in the closure",
          (g["confidence"], g["closure"]), ("medium", ["D"]))
    rng_big = {**ranges, ".text": ranges[".text"][:3] + [(0x1300, 0x1A00, "D"), (0x1A00, 0x1B00, "E")]}
    check("foreign code that outweighs the members is low confidence (more likely a named global)",
          build_groups(recs, rng_big, lits)["groups"][0]["confidence"], "low")
    check("the .data claims A B C are contiguous", (g["data"]["claims"], g["data"]["foreign"]), (3, []))
    check("group_of / fold_line", (group_of(c, "C.cpp") is g, group_of(c, "D")), (True, None))
    check("fold_line names the other units", "B with A + C + E" in fold_line(g, "B") or "B with A + C" in fold_line(g, "B"),
          True)
    # adjacency: only A B C adjacent and order clean -> high
    recs2 = recs[:3] + [_rec("A", ".sdata2", 0x8000, sites=1)]
    c2 = build_groups([r for r in recs2 if r["unit"] != "E"], ranges, lits)
    g2 = c2["groups"][0]
    check("A B C alone are adjacent, ordered, contiguous: high", (g2["units"], g2["adjacency"]["kind"], g2["confidence"]),
          (["A", "B", "C"], "adjacent", "high"))
    # a foreign .data between members disagrees with the pool model
    rng3 = {**ranges, ".data": [(0x9000, 0x9010, "A"), (0x9010, 0x9020, "D"), (0x9020, 0x9030, "C")]}
    g3 = build_groups([r for r in recs2 if r["unit"] != "E"], rng3, lits)["groups"][0]
    check("a foreign .data claim inside the span is reported and caps the confidence", (g3["data"]["foreign"], g3["confidence"]),
          (["D"], "medium"))
    text = "\n".join(render_group(g3))
    check("the render says the models disagree", "INTERLEAVES foreign data of D" in text, True)
    # first-use order: a literal first used by a LATER unit at a LOWER pool address than an earlier unit's is an inversion
    rng4 = {".text": [(0x1000, 0x1100, "P"), (0x1100, 0x1200, "Q")], ".sdata2": []}
    lit4 = {0x8000: _entry(".sdata2", 0x8000, 4, "float"), 0x8004: _entry(".sdata2", 0x8004, 4, "float"),
            0x8008: _entry(".sdata2", 0x8008, 4, "float")}
    ok4 = [_rec("P", ".sdata2", 0x8000), _rec("Q", ".sdata2", 0x8000), _rec("P", ".sdata2", 0x8004),
           _rec("Q", ".sdata2", 0x8004)]
    check("shared literals in first-use order: no inversion",
          build_groups(ok4, rng4, lit4)["groups"][0]["order"]["inversions"], 0)
    bad4 = [_rec("Q", ".sdata2", 0x8000), _rec("P", ".sdata2", 0x8000), _rec("P", ".sdata2", 0x8004),
            _rec("Q", ".sdata2", 0x8004)]
    # address 0x8000 first user = P (text order), 0x8004 first user = P: no inversion either - order is by TEXT
    check("first user is decided by text order, not by the record order",
          build_groups(bad4, rng4, lit4)["groups"][0]["order"]["inversions"], 0)
    # an inversion: Q (later text) is the first user at the lower address, P only at the higher one
    inv = [_rec("Q", ".sdata2", 0x8000), _rec("R", ".sdata2", 0x8000), _rec("P", ".sdata2", 0x8004),
           _rec("R", ".sdata2", 0x8004)]
    rng5 = {".text": [(0x1000, 0x1100, "P"), (0x1100, 0x1200, "Q"), (0x1200, 0x1300, "R")], ".sdata2": []}
    check("a lower pool address first used by a later unit is an inversion",
          build_groups(inv, rng5, lit4)["groups"][0]["order"]["inversions"], 1)
    # magic + coarse need values
    val = {0x8000: bytes.fromhex("4330000080000000"), 0x8004: bytes.fromhex("3f800000"), 0x8008: bytes.fromhex("3f800000")}
    vof = lambda a, n: val.get(a)  # noqa: E731
    recs6 = [_rec("P", ".sdata2", 0x8000), _rec("Q", ".sdata2", 0x8000), _rec("Q", ".sdata2", 0x8004),
             _rec("Q", ".sdata2", 0x8008)]
    c6 = build_groups(recs6, rng4, lit4, vof)
    check("the int->float magic stays an edge and is counted", (c6["groups"][0]["shared"], c6["groups"][0]["magic"]), (1, 1))
    check("one value at two pool addresses in one unit is a coarse unit",
          (sorted(c6["coarse"]), c6["coarse"]["Q"][0]["value"]), (["Q"], "3f800000"))
    check("...and it demotes the group", c6["groups"][0]["confidence"], "medium")
    check("no readers for values: no coarse claim", build_groups(recs6, rng4, lit4)["coarse"], {})
    check("note_for_unit never raises on an unreadable root", note_for_unit("does/not/exist", "A"), None)
    touch = {0x8000: ["A", "B"], 0x8004: ["B"], 0x8008: ["C"]}
    got = pool_seams_for([0x8000, 0x8004, 0x8004, 0x9999], touch)
    check("pool_seams_for names the units touching the range's literals, most-shared first",
          ([(r["unit"], r["count"]) for r in got["units"]], got["shared"]), ([("B", 2), ("A", 1)], 2))
    check("... never the range's own unit, and None when nothing is touched",
          (pool_seams_for([0x8000], touch, "A")["units"][0]["unit"], pool_seams_for([0x9999], touch),
           pool_seams_for([], touch)), ("B", None, None))
    check("the census carries the touch index (every literal a unit reads or claims)",
          sorted(build_groups(recs, ranges, lits)["touch"])[:2], [0x8000, 0x8004])
    check("edge classes add up to the group totals",
          build_groups(recs, ranges, lits)["edge_classes"]["total"], 4)
    check("render_census handles no groups", "0 group(s)" in render_census(build_groups([], {}, {})), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main(argv=None) -> int:
    import argparse
    import json

    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=os.path.dirname(TOOLS), help="the tree to read (default: this tool's tree)")
    ap.add_argument("--unit", help="only the group this unit belongs to")
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--json", help="write the census to this file")
    ap.add_argument("--no-values", action="store_true", help="skip the DOL (no magic tag, no coarse-unit check)")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    census = load_census(args.root, not args.no_values)
    if args.json:
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(census, fh, indent=1, default=list)
        print("wrote %s" % args.json)
    if args.unit:
        g = group_of(census, args.unit)
        print("\n".join(render_group(g)) if g else "%s: no pool-sharing group" % args.unit)
        return 0
    print(render_census(census, args.top))
    return 0


if __name__ == "__main__":
    sys.exit(main())
