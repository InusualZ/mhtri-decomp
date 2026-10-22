#!/usr/bin/env python3
"""The name registry: one shared, first-proposal-wins record of the names lanes give symbols other units own.
Spec: docs/tools/spec/names.md. CLI: names.py lookup ADDR|NAME | propose ADDR NAME --lane L --evidence TEXT [--dry-run]
| list [--lane L] [--pending] | export [-o FILE] [--ref REF] [--registry F] [--root DIR] [--json]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import datetime
import json
import os
import re
from dataclasses import dataclass, field

from tools.lib import names as libnames
from tools.lib import repo as librepo
from tools.lib.git import Git
from tools.lib.project import splits as _splits
from tools.lib.project import symbols as _symbols

#: The registry, tracked beside the map it feeds (docs/tools/spec/names.md, "Why this path").
REGISTRY_REL = "config/RMHE08/name_registry.jsonl"
#: The `.gitattributes` rule the registry needs: two lanes' appends merge as a union (`git merge`, and `land.py`'s
#: `git apply -3`), never a conflict; `resolve` then decides the address first-proposal-wins.
ATTRIBUTE_RULE = "%s  text eol=lf merge=union" % REGISTRY_REL
SYMBOLS_REL = "config/RMHE08/symbols.txt"
SPLITS_REL = "config/RMHE08/splits.txt"
DEFAULT_REF = "main"


def parse_address(text: str) -> int | None:
    """`0x8011D448` / `8011D448` -> the int; None for anything else (a name)."""
    m = re.fullmatch(r"\s*(?:0x)?([0-9A-Fa-f]{8})\s*", text or "")
    return int(m.group(1), 16) if m else None


@dataclass
class Entry:
    """One registry line: a proposed `name` for the row at `section:address`, by `lane`, at `at`."""
    address: int
    name: str
    section: str = ""
    old: str = ""
    lane: str = ""
    evidence: str = ""
    at: str = ""
    order: int = 0                      # the line's position in the read (ties on `at` keep it)
    status: str = ""                    # `winner` / `superseded` / `duplicate` / `name-taken`, set by `resolve`
    winner: str = ""                    # for a loser: the name that won its address (or the address its name won)

    @classmethod
    def from_json(cls, data: dict, order: int) -> "Entry | None":
        addr = parse_address(str(data.get("address") or ""))
        name = str(data.get("name") or "").strip()
        if addr is None or not name:
            return None
        return cls(addr, name, str(data.get("section") or ""), str(data.get("old") or ""), str(data.get("lane") or ""),
                   str(data.get("evidence") or ""), str(data.get("at") or ""), order)

    def to_json(self) -> dict:
        return {"address": "0x%08X" % self.address, "section": self.section, "name": self.name, "old": self.old,
                "lane": self.lane, "evidence": self.evidence, "at": self.at}

    def line(self) -> str:
        return json.dumps(self.to_json(), ensure_ascii=False, sort_keys=False)

    def key(self) -> tuple:
        return (self.address, self.name, self.lane, self.at)


def read_entries(texts: list[str]) -> list[Entry]:
    """Every distinct entry of the registry texts (main's committed copy, then the tree's), in read order; a line that
    is not a JSON object with an address and a name is skipped, and an identical entry read twice counts once."""
    out, seen, order = [], set(), 0
    for text in texts:
        for line in (text or "").splitlines():
            line = line.strip()
            if not line:
                continue
            try:
                data = json.loads(line)
            except ValueError:
                continue
            e = Entry.from_json(data, order) if isinstance(data, dict) else None
            order += 1
            if e is not None and e.key() not in seen:
                seen.add(e.key())
                out.append(e)
    return out


def resolve(entries: list[Entry]) -> list[Entry]:
    """First proposal wins, by `at` then read order: the first entry for an address is its `winner`; a later one with
    the same name is a `duplicate`, with another name `superseded`; a later entry whose name already won another
    address is `name-taken`. -> the entries, each with `status`/`winner` set. Pure."""
    by_addr: dict[int, Entry] = {}
    by_name: dict[str, Entry] = {}
    for e in sorted(entries, key=lambda x: (x.at, x.order)):
        held = by_addr.get(e.address)
        if held is not None:
            e.status, e.winner = ("duplicate" if held.name == e.name else "superseded"), held.name
            continue
        taken = by_name.get(e.name)
        if taken is not None:
            e.status, e.winner = "name-taken", "0x%08X" % taken.address
            continue
        e.status = "winner"
        by_addr[e.address] = e
        by_name[e.name] = e
    return entries


@dataclass
class MapView:
    """The rows of one map text, indexed by name and by address (any section)."""
    label: str
    by_name: dict = field(default_factory=dict)
    by_addr: dict = field(default_factory=dict)

    @classmethod
    def parse(cls, label: str, text: str) -> "MapView":
        v = cls(label)
        for line in (text or "").splitlines():
            row = _symbols.parse_line(line)
            if row is not None:
                v.by_name.setdefault(row.name, []).append(row)
                v.by_addr.setdefault(row.address, []).append(row)
        return v

    def at(self, address: int) -> list:
        return list(self.by_addr.get(address, []))


class Registry:
    """The state `names.py` judges against: main's map (the landed names) and the tree's (a lane's own-row renames),
    the claims, and the registry read from main's committed copy plus the tree's file."""

    def __init__(self, root: str, ref: str = DEFAULT_REF, registry: str | None = None):
        self.root = root
        self.path = registry or os.path.join(root, *REGISTRY_REL.split("/"))
        git = Git(root)
        has_ref = bool(ref) and git.run("rev-parse", "--verify", "-q", ref + "^{commit}").returncode == 0
        self.ref = ref if has_ref else None

        def at_ref(rel: str) -> str | None:
            if not self.ref:
                return None
            data = git.show(self.ref, rel)
            return data.decode("utf-8", "replace") if data is not None else None

        def in_tree(rel_or_path: str) -> str:
            p = rel_or_path if os.path.isabs(rel_or_path) else os.path.join(root, *rel_or_path.split("/"))
            try:
                with open(p, encoding="utf-8", errors="replace", newline="") as fh:
                    return fh.read()
            except OSError:
                return ""

        tree_map = in_tree(SYMBOLS_REL)
        ref_map = at_ref(SYMBOLS_REL)
        self.main_map = MapView.parse(self.ref or "the tree", ref_map if ref_map is not None else tree_map)
        self.tree_map = MapView.parse("the tree", tree_map)
        ref_splits = at_ref(SPLITS_REL)
        self.splits = _splits.Splits.parse(ref_splits if ref_splits is not None else in_tree(SPLITS_REL))
        ref_reg = at_ref(REGISTRY_REL) if registry is None else None
        self.entries = resolve(read_entries([ref_reg or "", in_tree(self.path)]))

    # --- lookups -------------------------------------------------------------------------------------------
    def owner(self, section: str, address: int) -> str | None:
        r = self.splits.covering(section, address) if section else None
        return _splits.stem(r.unit) if r else None

    def winner_at(self, address: int) -> Entry | None:
        return next((e for e in self.entries if e.address == address and e.status == "winner"), None)

    def winner_named(self, name: str) -> Entry | None:
        return next((e for e in self.entries if e.name == name and e.status == "winner"), None)

    def map_rows_named(self, name: str) -> list:
        """The rows called `name` in main's map and the tree's (one per address)."""
        rows = {}
        for view in (self.main_map, self.tree_map):
            for r in view.by_name.get(name, []):
                rows.setdefault((r.section, r.address), r)
        return list(rows.values())

    def row_at(self, address: int):
        """The map row at `address`: main's (a name that landed), else the tree's. None when neither has one."""
        rows = self.main_map.at(address) or self.tree_map.at(address)
        return rows[0] if rows else None

    def lookup(self, query: str) -> dict:
        """What is known about an address or a name: the map rows (main's and the tree's), the owner, every registry
        entry with its status."""
        addr = parse_address(query)
        if addr is not None:
            rows = {(r.section, r.address, r.name): r for v in (self.main_map, self.tree_map) for r in v.at(addr)}
            entries = [e for e in self.entries if e.address == addr]
        else:
            rows = {(r.section, r.address, r.name): r for r in self.map_rows_named(query)}
            entries = [e for e in self.entries if e.name == query]
        out_rows = []
        for (section, address, name), r in sorted(rows.items()):
            where = [v.label for v in (self.main_map, self.tree_map) if any(x.name == name for x in v.at(address))]
            out_rows.append({"name": name, "section": section, "address": "0x%08X" % address, "size": r.size,
                             "generated": libnames.is_generated(name, "map"), "in": where,
                             "owner": self.owner(section, address)})
        return {"query": query, "map": out_rows,
                "registry": [dict(e.to_json(), status=e.status, winner=e.winner) for e in entries]}

    # --- the decision --------------------------------------------------------------------------------------
    def propose(self, address: int, name: str, lane: str, evidence: str, now: str | None = None
                ) -> tuple[str, str, Entry | None]:
        """-> (verdict, message, entry to append). `verdict`: `proposed` (append it), `already` (this name is the
        winner here, or the map carries it: nothing to write) or `refused`. The rules, first refusal wins: a valid,
        non-generated name; a map row at the address; a map row already named (by main or the tree) keeps its name; a
        name the map uses at another address; an address the registry already gave another name (first proposal wins:
        the winner is named); a name the registry already gave another address."""
        if not name or not _symbols.VALID_NAME_RE.fullmatch(name):
            return "refused", "%r is not a name the map can carry" % name, None
        if (libnames.is_generated(name, "map") or libnames.RULE7_STEM_RE.search(name)
                or libnames.ADDRESS_RUN_RE.search(name)):
            return "refused", "%s is a generated or address-named name (section 6.5 rule 7)" % name, None
        if not lane.strip() or not evidence.strip():
            return "refused", "a proposal names its --lane and its --evidence", None
        row = self.row_at(address)
        if row is None:
            return "refused", "no map row at 0x%08X (a name needs a row: split or add it first)" % address, None
        for view in (self.main_map, self.tree_map):
            for r in view.at(address):
                if r.name == name:
                    return "already", "0x%08X is %s in %s's map" % (address, name, view.label), None
                if not libnames.is_generated(r.name, "map"):
                    return ("refused", "%s already named 0x%08X %s - use %s" % (
                        "main" if view is self.main_map else "the tree", address, r.name, r.name), None)
        elsewhere = [r for r in self.map_rows_named(name) if r.address != address]
        if elsewhere:
            r = elsewhere[0]
            return "refused", "%s is already the map's name of %s:0x%08X" % (name, r.section, r.address), None
        won = self.winner_at(address)
        if won is not None:
            if won.name == name:
                return "already", "0x%08X %s was proposed by lane %s at %s" % (address, name, won.lane, won.at), None
            return ("refused", "0x%08X already has the proposal %s (lane %s, %s) - first proposal wins: use %s"
                    % (address, won.name, won.lane, won.at, won.name), None)
        taken = self.winner_named(name)
        if taken is not None:
            return ("refused", "%s is already proposed for 0x%08X (lane %s, %s)"
                    % (name, taken.address, taken.lane, taken.at), None)
        stamp = now or datetime.datetime.now().replace(microsecond=0).isoformat()
        entry = Entry(address, name, row.section, row.name, lane.strip(), " ".join(evidence.split()), stamp,
                      status="winner")
        return "proposed", "0x%08X %s -> %s (lane %s)" % (address, row.name, name, entry.lane), entry

    def append(self, entry: Entry) -> None:
        """Append one line (LF, UTF-8) to the tree's registry file, creating it (and its directory) when absent."""
        os.makedirs(os.path.dirname(self.path), exist_ok=True)
        lead = ""
        if os.path.isfile(self.path) and os.path.getsize(self.path):
            with open(self.path, "rb") as fh:
                fh.seek(-1, os.SEEK_END)
                lead = "" if fh.read(1) == b"\n" else "\n"
        with open(self.path, "a", encoding="utf-8", newline="\n") as fh:
            fh.write(lead + entry.line() + "\n")

    def pending(self) -> list[tuple[Entry, object]]:
        """`[(winner, main's row at its address)]` for every winner main's map does not carry yet."""
        out = []
        for e in self.entries:
            if e.status != "winner":
                continue
            rows = self.main_map.at(e.address)
            if any(r.name == e.name for r in rows):
                continue
            out.append((e, rows[0] if rows else None))
        return out

    def export(self) -> tuple[list[tuple[str, str]], list[str]]:
        """-> (`[(old, new)]` rename pairs for `symedit.py rename-batch`, notes): every pending winner whose row in main's
        map still has a generated name and whose name main's map does not use elsewhere."""
        pairs, notes = [], []
        for e, row in self.pending():
            if row is None:
                notes.append("0x%08X %s: no row at that address in %s's map" % (e.address, e.name, self.main_map.label))
            elif not libnames.is_generated(row.name, "map"):
                notes.append("0x%08X %s: main named it %s - the proposal is stale" % (e.address, e.name, row.name))
            elif self.main_map.by_name.get(e.name):
                other = self.main_map.by_name[e.name][0]
                notes.append("0x%08X %s: the name is main's at 0x%08X" % (e.address, e.name, other.address))
            else:
                pairs.append((row.name, e.name))
        return pairs, notes


# --- the CLI ----------------------------------------------------------------------------------------------------

def _print_lookup(info: dict) -> None:
    if not info["map"] and not info["registry"]:
        print("%s: not in the map, not in the registry" % info["query"])
        return
    for r in info["map"]:
        print("map  %s %s %s size:0x%X owner:%s%s (%s)" % (r["address"], r["section"], r["name"], r["size"],
                                                          r["owner"] or "none", " [generated]" if r["generated"] else "",
                                                          ", ".join(r["in"])))
    for e in info["registry"]:
        extra = (" (won by %s)" % e["winner"]) if e["status"] != "winner" else ""
        print("reg  %s %s %-10s lane %s, %s%s - %s" % (e["address"], e["name"], e["status"], e["lane"], e["at"], extra,
                                                       e["evidence"][:100]))


def main(argv: list[str] | None = None, root: str | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=None, help="the tree (default: the invocation's)")
    ap.add_argument("--ref", default=DEFAULT_REF, help="the ref whose map and registry are the landed state (main)")
    ap.add_argument("--registry", default=None, help="the registry file (default %s in the tree)" % REGISTRY_REL)
    ap.add_argument("--json", action="store_true")
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("lookup", help="what main's map, the tree's map and the registry say about an address or a name")
    p.add_argument("query", nargs="+")
    p = sub.add_parser("propose", help="record a name for the row at an address (first proposal wins)")
    p.add_argument("address")
    p.add_argument("name")
    p.add_argument("--lane", required=True)
    p.add_argument("--evidence", required=True)
    p.add_argument("--dry-run", action="store_true")
    p = sub.add_parser("list", help="every registry entry with its status")
    p.add_argument("--lane", default=None)
    p.add_argument("--pending", action="store_true", help="only winners main's map does not carry yet")
    p = sub.add_parser("export", help="`old new` rename lines of the pending winners (symedit.py rename-batch)")
    p.add_argument("-o", "--output", default=None)
    args = ap.parse_args(argv)
    root = args.root or root or librepo.repo_root()
    reg = Registry(root, args.ref, args.registry)

    if args.cmd == "lookup":
        infos = [reg.lookup(q) for q in args.query]
        if args.json:
            print(json.dumps(infos, indent=1))
        else:
            for info in infos:
                _print_lookup(info)
        return 0
    if args.cmd == "propose":
        addr = parse_address(args.address)
        if addr is None:
            print("REFUSED %s | not an address (0x8XXXXXXX)" % args.address)
            return 2
        verdict, message, entry = reg.propose(addr, args.name, args.lane, args.evidence)
        if verdict == "proposed" and not args.dry_run:
            reg.append(entry)
        if args.json:
            print(json.dumps({"verdict": verdict, "message": message, "entry": entry.to_json() if entry else None,
                              "dry_run": args.dry_run}))
        else:
            print("%s %s%s" % (verdict.upper(), message, " (dry run: nothing written)" if args.dry_run and entry else ""))
        return 1 if verdict == "refused" else 0
    if args.cmd == "list":
        rows = [e for e in reg.entries if not args.lane or e.lane == args.lane]
        if args.pending:
            pend = {id(e) for e, _row in reg.pending()}
            rows = [e for e in rows if id(e) in pend]
        if args.json:
            print(json.dumps([dict(e.to_json(), status=e.status, winner=e.winner) for e in rows], indent=1))
        else:
            for e in sorted(rows, key=lambda x: (x.address, x.at, x.order)):
                print("0x%08X %-10s %s (was %s) lane %s %s%s" % (e.address, e.status, e.name, e.old or "?", e.lane,
                                                                 e.at, (" - won by %s" % e.winner) if e.winner else ""))
            print("%d entr%s" % (len(rows), "y" if len(rows) == 1 else "ies"))
        return 0
    pairs, notes = reg.export()
    text = "".join("%s %s\n" % p for p in pairs)
    if args.output:
        with open(args.output, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    elif not args.json:
        sys.stdout.write(text)
    if args.json:
        print(json.dumps({"pairs": pairs, "notes": notes}, indent=1))
    for n in notes:
        print("note: " + n, file=sys.stderr)
    print("%d rename(s)%s" % (len(pairs), (" written to %s" % args.output) if args.output else ""), file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
