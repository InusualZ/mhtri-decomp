#!/usr/bin/env python3
"""Derive the campaign's progress from the repository: what is covered, what is registered, what is next.

    python tools/units/ledger.py [--json]           # totals, per-module table, units at >= 80 %
    python tools/units/ledger.py next [N]           # the next N unclaimed symbols, in address order
    python tools/units/ledger.py unit <unit>        # one unit: ranges, symbols, per-symbol score

Nothing here writes and nothing is stored: the loop's state *is* the repository. `splits.txt` says which
addresses a unit owns, `configure.py` says which units are registered and how they are built,
`build/RMHE08/config.json` says which split object holds a given address, and `build/RMHE08/report.json` says
what each unit scores. A ledger file would be a fourth copy of all that, and it would be the wrong one the
first time a subagent edited the repo without updating it.

Metric: report version 2, where a *function* entry carries `fuzzy_match_percent` and an entry **without**
that key is 0 %, not 100 % (`complete_code_percent` is not a score). The bar is 80 on that per-symbol number,
which is what step 3 of `docs/plan.md` closes a symbol against; playbook 15 pins it to the objdiff version in
`configure.py`, so a tool bump means re-baselining.

`next` is what step 1 of the campaign consumes - an address, and the split object step 2 has to disassemble.
Both come out of the same repository state the other tools read, so a fresh session resumes with no history.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import sys

from tools.lib import names as libnames

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = "RMHE08"
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import symbolpreflight as preflight  # noqa: E402  (shares its symbols.txt / splits.txt / configure.py parsers)

BAR = 80.0
SCORE_KEY = "fuzzy_match_percent"
TEXT_BLOCK = 0x10000
REPORT_PATH = os.path.join(ROOT, "build", GAME, "report.json")
CONFIG_PATH = os.path.join(ROOT, "build", GAME, "config.json")
SOURCES = (
    os.path.join(ROOT, "config", GAME, "splits.txt"),
    os.path.join(ROOT, "configure.py"),
)
# dtk names a split object after the section and the address it starts at: auto_03_802AE0C4_text.o
OBJECT_RE = re.compile(r"^auto_\d+_([0-9a-fA-F]{8})_(\w+)$")
GENERATED_RE = libnames.GENERATED["ledger"]


def read_json(path: str) -> dict | None:
    if not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def report_name(unit: str) -> str:
    """`Camellia/camellia.c` (splits.txt, configure.py) -> `main/Camellia/camellia` (report.json)."""
    return "main/" + os.path.splitext(unit)[0]


def bare(unit: str) -> str:
    """Either spelling of a unit, without its extension and the report's `main/` prefix."""
    return os.path.splitext(unit)[0].removeprefix("main/")


def stale(report_path: str, sources: tuple[str, ...] = SOURCES) -> bool:
    """True when the report predates the split or the config it describes - the one stale input here.

    `report.json` is a build output, so it can describe the repository as it was a session ago; every score
    in it is then one registration out of date (the repo has been bitten by this before).
    """
    if not os.path.exists(report_path):
        return True
    newest = max((os.path.getmtime(path) for path in sources if os.path.exists(path)), default=0.0)
    return os.path.getmtime(report_path) < newest


class Objects:
    """Which split object holds an address - dtk's config.json, or the object file names as a fallback.

    Only the `auto_*` scaffolding is interesting here: a registered unit's addresses are covered by its
    `splits.txt` range and its object is `build/RMHE08/obj/<unit>.o`. An object runs until the next one in
    its section starts, or its recorded size - the last object in a section has no neighbour to measure it.
    """

    def __init__(self, path: str = CONFIG_PATH) -> None:
        self.ranges: dict[str, list[tuple[int, int | None, str]]] = {}
        config = read_json(path)
        if config:
            named = [(unit.get("name", ""), unit.get("code_size", 0) + unit.get("data_size", 0)) for unit in config["units"]]
        else:
            named = [(os.path.splitext(os.path.basename(p))[0], 0) for p in _object_paths()]
        entries = []
        for name, size in named:
            match = OBJECT_RE.match(name)
            if match:
                entries.append(
                    (
                        "." + match.group(2),
                        int(match.group(1), 16),
                        size,
                        os.path.join(ROOT, "build", GAME, "obj", name + ".o"),
                    )
                )
        for section, address, size, obj in sorted(entries):
            self.ranges.setdefault(section, []).append([address, address + size if size else None, obj])
        for section, ranges in self.ranges.items():
            # An object without a recorded size runs until the next one in its section starts.
            for index in range(len(ranges) - 1):
                if ranges[index][1] is None:
                    ranges[index][1] = ranges[index + 1][0]
            self.ranges[section] = [tuple(rng) for rng in ranges]

    def covering(self, section: str, address: int) -> str | None:
        for start, end, obj in self.ranges.get(section, ()):
            if start <= address and (end is None or address < end):
                return obj
        return None


def _object_paths() -> list[str]:
    directory = os.path.join(ROOT, "build", GAME, "obj")
    if not os.path.isdir(directory):
        return []
    return [os.path.join(directory, name) for name in os.listdir(directory) if name.endswith(".o")]


class Ledger:
    """The campaign's state, read out of the repository in one pass."""

    def __init__(
        self,
        report_path: str = REPORT_PATH,
        config_path: str = CONFIG_PATH,
        symbols: tuple[dict, dict, dict] | None = None,
        splits: list[dict] | None = None,
        configured: dict | None = None,
        report: dict | None = None,
    ) -> None:
        """`symbols`/`splits`/`configured`/`report` are injectable so the views can be tested on fixtures."""
        self.by_name, self.by_section, _ = symbols if symbols is not None else preflight.load_symbols()
        self.splits = splits if splits is not None else preflight.load_splits()
        if configured is not None:
            self.configured, self.libs = configured, []
        else:
            self.configured, self.libs = preflight.load_configure()
        self.report = report if report is not None else (read_json(report_path) or {"units": [], "measures": {}})
        self.objects = Objects(config_path)
        # The report is a *build* output, so it can describe a repository one registration out of date.
        self.stale = report is None and stale(report_path)
        self.scores: dict[str, dict[str, float]] = {
            unit["name"]: {
                function["name"]: function.get(SCORE_KEY, 0.0) for function in unit.get("functions", ())
            }
            for unit in self.report["units"]
        }

    # -- reading the map -------------------------------------------------------------------------

    def symbols(self, kind: str = "function") -> list[dict]:
        """Every symbol of a kind, in campaign order (ascending address; a section's own order next).

        `data` skips the `extab`/`extabindex`/`.ctors`/`.dtors` fragments on purpose: those belong to the
        code unit that owns them and are claimed as ranges with it, never one by one. They are still
        counted in `totals()`.
        """
        out = []
        for section, entries in self.by_section.items():
            if kind == "data" and section in preflight.FRAGMENT_SECTIONS:
                continue
            for entry in entries:
                is_function = (entry.get("type") or "") == "function"
                if kind == "function" and not is_function:
                    continue
                if kind == "data" and is_function:
                    continue
                out.append(entry)
        out.sort(key=lambda entry: (entry["address"], entry["section"]))
        return out

    def owner(self, entry: dict) -> dict | None:
        return preflight.covering(self.splits, entry["section"], entry["address"])

    def unit_symbols(self, block: dict, kind: str = "all") -> list[dict]:
        """Every symbol inside one unit's ranges, from the section lists (not from a scan of the whole map)."""
        out = []
        for rng in block["ranges"]:
            for entry in self.by_section.get(rng["section"], ()):
                if not rng["start"] <= entry["address"] < rng["end"]:
                    continue
                if kind == "function" and (entry.get("type") or "") != "function":
                    continue
                out.append(entry)
        out.sort(key=lambda entry: (entry["address"], entry["section"]))
        return out

    def score(self, owner: dict, entry: dict) -> float:
        return self.scores.get(report_name(owner["unit"]), {}).get(entry["name"], 0.0)

    # -- the three views -------------------------------------------------------------------------

    def totals(self) -> dict:
        functions = self.symbols("function")
        closed = partial = claimed = 0
        for entry in functions:
            owner = self.owner(entry)
            if owner is None:
                continue
            claimed += 1
            score = self.score(owner, entry)
            closed += score >= BAR
            partial += 0.0 < score < BAR
        measures = self.report.get("measures", {})
        everything = [entry for section in self.by_section.values() for entry in section]
        return {
            "functions": len(functions),
            "data_symbols": len(everything) - len(functions),
            "fragment_symbols": sum(
                1 for entry in everything if entry["section"] in preflight.FRAGMENT_SECTIONS
            ),
            "claimed_functions": claimed,
            "closed": closed,
            "partial": partial,
            "unclaimed": len(functions) - claimed,
            "units_registered": len(self.splits),
            "units_configured": len(self.configured),
            "units_total": measures.get("total_units"),
            "matched_functions": measures.get("matched_functions"),
            "matched_code": measures.get("matched_code"),
            "total_code": measures.get("total_code"),
            "fuzzy_match_percent": measures.get("fuzzy_match_percent"),
            "report_stale": self.stale,
            "text_blocks": self.text_blocks(),
        }

    def modules(self) -> list[dict]:
        """One row per module: the directory of each registered unit, plus the unclaimed remainder."""
        rows: dict[str, dict] = {}
        for block in self.splits:
            module = os.path.dirname(block["unit"]) or block["unit"]
            scores = self.scores.get(report_name(block["unit"]), {})
            row = rows.setdefault(
                module, {"module": module, "units": 0, "symbols": 0, "closed": 0, "scored": 0, "best": 0.0}
            )
            row["units"] += 1
            # Every section, not just .text: a runtime unit's functions live in .init, and counting only
            # .text made the Runtime.PPCEABI.H row look empty while it held five units.
            for entry in self.unit_symbols(block, "function"):
                score = scores.get(entry["name"], 0.0)
                row["symbols"] += 1
                row["closed"] += score >= BAR
                row["scored"] += score > 0.0
                row["best"] = max(row["best"], score)
        unclaimed = [entry for entry in self.symbols("function") if self.owner(entry) is None]
        rows["(unclaimed)"] = {
            "module": "(unclaimed)",
            "units": 0,
            "symbols": len(unclaimed),
            "closed": 0,
            "scored": 0,
            "best": 0.0,
        }
        return sorted(rows.values(), key=lambda row: (row["module"] == "(unclaimed)", row["module"]))

    def units_below_bar(self) -> list[dict]:
        out = []
        for block in self.splits:
            scores = self.scores.get(report_name(block["unit"]), {})
            closed = sum(1 for score in scores.values() if score >= BAR)
            if not scores or closed < len(scores):
                out.append(
                    {
                        "unit": block["unit"],
                        "symbols": len(scores),
                        "closed": closed,
                        "worst": min(scores.values(), default=0.0),
                    }
                )
        return sorted(out, key=lambda row: (row["worst"], row["unit"]))

    def text_blocks(self, block_size: int = TEXT_BLOCK) -> dict:
        """Coarse per-0x10000 view of `.text` so the next unit can be picked by address (plan 7.11).

        A block is *touched* when it holds at least one claimed function and *closed* when one of them is at
        the bar. The largest untouched run is the widest stretch with no claim - the address to work next.
        """
        entries = self.by_section.get(".text", ())
        empty = {"block_size": block_size, "start": 0, "end": 0, "total": 0, "claimed": 0,
                 "closed": 0, "largest_untouched": None}
        if not entries:
            return empty
        start = min(entry["address"] for entry in entries) // block_size * block_size
        end = max(entry["address"] + (entry.get("size") or 0) for entry in entries)
        total = (end - start + block_size - 1) // block_size
        touched, closed = set(), set()
        for entry in entries:
            owner = self.owner(entry)
            if owner is None:
                continue
            block = (entry["address"] - start) // block_size
            touched.add(block)
            if self.score(owner, entry) >= BAR:
                closed.add(block)
        best_len = best_start = 0
        run_start = None
        for block in range(total + 1):
            if block < total and block not in touched:
                if run_start is None:
                    run_start = block
            elif run_start is not None:
                if block - run_start > best_len:
                    best_len, best_start = block - run_start, run_start
                run_start = None
        largest = None
        if best_len:
            largest = {"blocks": best_len, "start": start + best_start * block_size,
                       "end": start + (best_start + best_len) * block_size}
        return {"block_size": block_size, "start": start, "end": end, "total": total,
                "claimed": len(touched), "closed": len(closed), "largest_untouched": largest}

    def next(self, limit: int = 10, kind: str = "function", named_only: bool = False) -> list[dict]:
        out = []
        for entry in self.symbols(kind):
            if self.owner(entry) is not None:
                continue
            if named_only and GENERATED_RE.match(entry["name"]):
                continue
            out.append(
                {
                    "name": entry["name"],
                    "section": entry["section"],
                    "address": entry["address"],
                    "type": entry.get("type") or "",
                    "size": entry.get("size") or 0,
                    "object": self.objects.covering(entry["section"], entry["address"]),
                }
            )
            if len(out) >= limit:
                break
        return out

    def unit(self, name: str) -> dict | None:
        block = next((b for b in self.splits if bare(b["unit"]) == bare(name)), None)
        if block is None:
            return None
        scores = self.scores.get(report_name(block["unit"]), {})
        symbols = []
        for entry in self.unit_symbols(block):
            symbols.append(
                {
                    "name": entry["name"],
                    "section": entry["section"],
                    "address": entry["address"],
                    "type": entry.get("type") or "object",
                    "size": entry.get("size") or 0,
                    "score": scores.get(entry["name"]),
                }
            )
        symbols.sort(key=lambda symbol: symbol["address"])
        return {
            "unit": block["unit"],
            "ranges": block["ranges"],
            "configured": self.configured.get(block["unit"]),
            "report": report_name(block["unit"]),
            "symbols": symbols,
            "closed": sum(1 for symbol in symbols if (symbol["score"] or 0.0) >= BAR),
            "partial": sum(1 for symbol in symbols if 0.0 < (symbol["score"] or 0.0) < BAR),
        }


# -- rendering -----------------------------------------------------------------------------------


def render_totals(ledger: Ledger) -> str:
    totals = ledger.totals()
    lines = [
        f"symbols    {totals['functions']} functions, {totals['data_symbols']} data symbols"
        f" ({totals['fragment_symbols']} of them extab/extabindex/ctors/dtors fragments)",
        f"covered    {totals['claimed_functions']} functions in {totals['units_registered']} registered units"
        f" ({totals['units_configured']} configured, {totals['units_total']} split objects in the report)",
        f"closed     {totals['closed']} / {totals['functions']} >= {BAR:.0f} %, {totals['partial']} partial,"
        f" {totals['unclaimed']} unclaimed  (objdiff counts {totals['matched_functions']} matched)",
        f"bytes      {totals['matched_code']} / {totals['total_code']} of .text"
        f" ({totals['fuzzy_match_percent']} % fuzzy)",
    ]
    blocks = totals["text_blocks"]
    if blocks["total"]:
        lines.append(
            f"blocks     {blocks['total']} x {blocks['block_size']:#x} in .text,"
            f" {blocks['claimed']} touched, {blocks['closed']} closed"
        )
        run = blocks["largest_untouched"]
        if run:
            lines.append(
                f"           largest untouched run {run['blocks']} blocks,"
                f" {run['start']:#010x}-{run['end']:#010x}"
            )
    if totals["report_stale"]:
        lines.append(
            "report     STALE - older than splits.txt/configure.py, so the scores below are one session old"
        )
        lines.append("           rm build/RMHE08/report.json && ninja build/RMHE08/report.json")
    lines += [
        f"{'module':<18}{'units':>6}{'symbols':>9}{'closed':>8}{'scored':>8}{'best':>8}",
    ]
    for row in ledger.modules():
        lines.append(
            f"{row['module']:<18}{row['units']:>6}{row['symbols']:>9}{row['closed']:>8}"
            f"{row['scored']:>8}{row['best']:>8.1f}"
        )
    below = ledger.units_below_bar()
    if below:
        lines += ["", "below the bar:"]
        for row in below[:20]:
            lines.append(f"  {row['unit']:<40}{row['closed']:>4}/{row['symbols']:<4} closed, worst {row['worst']:.1f}")
        if len(below) > 20:
            lines.append(f"  ... {len(below) - 20} more")
    return "\n".join(lines)


def render_next(rows: list[dict]) -> str:
    lines = []
    for row in rows:
        lines.append(f"{row['address']:#010x} {row['section']:<8} {row['type']:<9}{row['size']:>6}  {row['name']}")
        if row["object"]:
            lines.append(f"{'':<12}{row['object']}")
    return "\n".join(lines)


def render_unit(unit: dict) -> str:
    lines = [f"{unit['unit']}: {len(unit['symbols'])} symbols, {unit['closed']} closed, {unit['partial']} partial"]
    for rng in unit["ranges"]:
        lines.append(f"  {rng['section']:<10} {rng['start']:#010x}-{rng['end']:#010x}")
    configured = unit["configured"]
    lines.append(
        f"  build: {configured['lib']} / {configured['mw_version']} / {configured['cflags']} / {configured['flag']}"
        if configured
        else "  build: not in configure.py"
    )
    lines.append("")
    for symbol in unit["symbols"]:
        score = "-" if symbol["score"] is None else f"{symbol['score']:.1f}"
        lines.append(
            f"  {symbol['address']:#010x} {symbol['type']:<9}{symbol['size']:>6} {score:>7}  {symbol['name']}"
        )
    return "\n".join(lines)


def selftest() -> int:
    """Fixture check for the byte view (plan 7.11) - no build, no repository state.

    The other views are pinned by `ledger_selftest.py`; this covers what that file predates: the per-0x10000
    block view and the two burn-downs it feeds.
    """
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def sym(name, address, size=0x100):
        return {"name": name, "section": ".text", "address": address, "type": "function", "size": size,
                "lineno": 1}

    symbols = [sym("fn_80040000", 0x80040000), sym("fn_80050000", 0x80050000),
               sym("fn_80070000", 0x80070000), sym("fn_80080000", 0x80080000)]
    by_section = {".text": symbols}
    splits = [{"unit": "Mod/text.c", "ranges": [
        {"section": ".text", "start": 0x80040000, "end": 0x80060000, "rename": None}]}]
    configured = {"Mod/text.c": {"flag": "NonMatching", "path": "Mod/text.c", "lib": "mod",
                                  "mw_version": "Wii/1.3", "cflags": "cflags_base"}}
    report = {"version": 2, "measures": {"matched_code": "0x100", "total_code": "0x500"}, "units": [
        {"name": "main/Mod/text", "functions": [
            {"name": "fn_80040000", "fuzzy_match_percent": 90.0},
            {"name": "fn_80050000", "fuzzy_match_percent": 50.0}]}]}
    missing = os.path.join(ROOT, "build", GAME, "does-not-exist.json")
    ledger = Ledger(symbols=({s["name"]: s for s in symbols}, by_section, {}), splits=splits,
                    configured=configured, report=report, config_path=missing)
    blocks = ledger.text_blocks()
    check("block size", blocks["block_size"], 0x10000)
    check("block origin is aligned down", blocks["start"], 0x80040000)
    check("blocks total", blocks["total"], 5)
    check("blocks touched (claimed)", blocks["claimed"], 2)
    check("blocks closed", blocks["closed"], 1)
    check("largest untouched run", blocks["largest_untouched"],
          {"blocks": 3, "start": 0x80060000, "end": 0x80090000})
    check("totals carries the view", ledger.totals()["text_blocks"], blocks)
    check("a section with no .text has no blocks",
          Ledger(symbols=({}, {}, {}), splits=[], configured={}, report={"units": [], "measures": {}},
                 config_path=missing).text_blocks()["total"], 0)
    if fails:
        print("FAIL (%d)" % len(fails))
        for line in fails:
            print("  " + line)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    parser.add_argument("--selftest", action="store_true", help="run the ledger self-test (plan 7.11)")
    parser.add_argument("--report", default=REPORT_PATH, help="objdiff report (default build/RMHE08/report.json)")
    parser.add_argument("--config", default=CONFIG_PATH, help="dtk config (default build/RMHE08/config.json)")
    sub = parser.add_subparsers(dest="command")

    p = sub.add_parser("next", help="the next unclaimed symbols, in address order")
    p.add_argument("limit", nargs="?", type=int, default=10)
    p.add_argument("--type", choices=("function", "data", "all"), default="function")
    p.add_argument("--named", action="store_true", help="skip generated names (fn_*, lbl_*, unk_*)")

    p = sub.add_parser("unit", help="one unit: ranges, symbols, per-symbol score")
    p.add_argument("name")
    args = parser.parse_args()

    if args.selftest:
        return selftest()

    ledger = Ledger(args.report, args.config)

    if args.command == "next":
        rows = ledger.next(args.limit, args.type, args.named)
        print(json.dumps(rows, indent=2) if args.json else render_next(rows))
        return 0 if rows else 1

    if args.command == "unit":
        unit = ledger.unit(args.name)
        if unit is None:
            sys.exit("no such unit: %s\nregistered units: %s"
                     % (args.name, ", ".join(b["unit"] for b in ledger.splits)))
        print(json.dumps(unit, indent=2) if args.json else render_unit(unit))
        return 0

    if args.json:
        print(
            json.dumps(
                {"totals": ledger.totals(), "modules": ledger.modules(), "below_bar": ledger.units_below_bar()},
                indent=2,
            )
        )
    else:
        print(render_totals(ledger))
    return 0


if __name__ == "__main__":
    sys.exit(main())
