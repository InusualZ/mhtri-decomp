#!/usr/bin/env python3
"""Write `tools/units/data-queue.json`: the campaign's unowned data as a queue, not as comments.

docs/plan.md 7.17. `attribute.py`'s data pass currently writes the runs it saw into `splits.txt` as
`# ... claim in the measured data pass` comments, and nothing reads them back. This tool is the writer
for the queue `brief.py` already reads: one entry per **unowned data run**, in the shape the plan
specifies,

    {unit, section, start, end, labels, leak, density, verdict}

with `brief.py`'s `data_queue_entries()` consuming it per unit (it accepts either a bare list or
`{"entries": [...]}`; this writer emits the bare list).

Where the backlog comes from - the same repository state the other tools read, never a stored list:

* the **symbols** (`config/RMHE08/symbols.txt`, through `symbolpreflight.load_symbols`, which goes
  through `symedit`; the file is 4.5 MB and is never printed or pasted),
* the **owners** (`config/RMHE08/splits.txt`): a symbol is backlog when no claimed range covers its
  `(section, address)`,
* the **regions** (`build/RMHE08/config.json`, through `ledger.Objects`): which split object covers an
  unclaimed address, so a run with no referencing unit still gets a stable, honest name,
* the **references** (`build/tmp/tudiscover/graph.json`, the `tudiscover` cache): a data run whose
  symbols are referenced by exactly one registered unit is attributed to that unit (`Pl/pl_act`), which
  is what makes the queue useful to `brief.py`. The cache is read, never rebuilt - a stale stamp only
  drops the reference attribution, it does not trigger the 200-400 s graph walk.

The selection rule (pure, `select`): a symbol is backlog when it is not a function, its section is a
data section (not `.text`, not an `extab`/`extabindex`/`.ctors`/`.dtors` fragment - those travel with
the code unit that owns them), and no `splits.txt` range covers it.

The run grouping (pure, `group_runs`): within one section, symbols are merged while each next address
is at or before the current run's end, so a gap starts a new run and a zero-size label never does.

The verdict is the queue's own, pre-measurement verdict, and it uses the decisions the plan already
made: `never` for linker-generated data (`_rom_copy_info`, `_bss_init_info`, §8.4), `owner-held` for
the TRK interrupt-vector table (the escalation queue keeps it unowned), `not claimed` when the run leaks
across units or a claimed symbol sits inside it (`density < 0.5`), otherwise `proposed`.
`dataclaim.py` (7.8) refines `proposed` with the target-vs-ours section measurement.

    python tools/units/dataqueue.py                 # write the queue and print a summary
    python tools/units/dataqueue.py --dry-run       # report what would be written, write nothing
    python tools/units/dataqueue.py --limit 20      # a preview queue (deterministic prefix)
    python tools/units/dataqueue.py --json          # the queue on stdout, write nothing
    python tools/units/dataqueue.py --selftest

Writing is atomic (temp file + `os.replace`) and idempotent: the same repository state renders the same
bytes, so re-running cannot churn the file.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))

import sharedfiles as sf  # noqa: E402  (the one writer for shared files - docs/plan.md 7.12)
import symbolpreflight as preflight  # noqa: E402  (the splits.txt / symbols.txt parsers)

# dtk's section order (mirrors tudiscover.SECTION_ORDER) - the queue's deterministic sort key.
SECTION_ORDER = [".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data",
                 ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2"]
# Sections the queue never covers: `.text` is code, the four fragments are per-function and per-unit
# side effects that travel with the code unit (docs/plan.md §6.5 / AGENTS.md "playbook 23").
NON_DATA_SECTIONS = (".text",) + tuple(preflight.FRAGMENT_SECTIONS)

# docs/plan.md §8.4: MW ld emits these; they are not a translation unit's data.
LINKER_GENERATED = ("_rom_copy_info", "_bss_init_info")
# The escalation queue keeps the TRK interrupt-vector table unowned; its data must not be proposed.
# The exclusive end address is included too: the map's end marker (`gTRKInterruptVectorTableEnd`) sits on
# it, and claiming the marker separately would be claiming the table.
TRK_VECTOR_TABLE = (0x80004380, 0x800062B4)

QUEUE_REL = os.path.join("tools", "units", "data-queue.json")
GRAPH_REL = os.path.join("build", "tmp", "tudiscover", "graph.json")


def _section_key(section: str) -> tuple:
    return (SECTION_ORDER.index(section) if section in SECTION_ORDER else len(SECTION_ORDER), section)


def _covered(splits: list[dict], section: str, address: int) -> str | None:
    """The unit whose `splits.txt` range covers `(section, address)`, or None - pure."""
    for block in splits:
        for rng in block.get("ranges", ()):
            if rng["section"] == section and rng["start"] <= address < rng["end"]:
                return block["unit"]
    return None


def is_data_section(section: str) -> bool:
    return section not in NON_DATA_SECTIONS


def select(symbols: list[dict], splits: list[dict]) -> list[dict]:
    """The backlog: every non-function symbol in a data section that no claimed range covers."""
    out = []
    for entry in symbols:
        if (entry.get("type") or "") == "function":
            continue
        if not is_data_section(entry.get("section") or ""):
            continue
        if _covered(splits, entry["section"], entry["address"]) is not None:
            continue
        out.append(entry)
    return out


def group_runs(entries: list[dict]) -> list[dict]:
    """Merge backlog symbols into maximal contiguous runs per section, deterministically.

    A symbol ends at `address + max(size, 1)`: a zero-size label must not break a run in two. The next
    symbol joins the run when its address is at or before that end, so any real gap starts a new run.
    """
    by_section: dict[str, list[dict]] = {}
    for entry in entries:
        by_section.setdefault(entry["section"], []).append(entry)
    runs: list[dict] = []
    for section, items in by_section.items():
        current = None
        for entry in sorted(items, key=lambda e: (e["address"], e["name"])):
            end = entry["address"] + max(int(entry.get("size") or 0), 1)
            item = [entry["address"], entry["name"], end]
            if current is not None and entry["address"] <= current["end"]:
                current["end"] = max(current["end"], end)
                current["names"].append(entry["name"])
                current["items"].append(item)
            else:
                current = {"section": section, "start": entry["address"], "end": end,
                           "names": [entry["name"]], "items": [item]}
                runs.append(current)
    runs.sort(key=lambda r: (_section_key(r["section"]), r["start"], r["end"]))
    return runs


def split_by_owner(runs: list[dict], refs: dict[str, set[str]], fn_unit: dict[str, str]) -> list[dict]:
    """Cut each contiguous run where the referencing-unit signature changes.

    A `.sdata2` pool is contiguous but several units share it, so raw contiguity merges unrelated
    units' data into one run and hides the attribution. Splitting where the set of referencing units
    changes recovers the per-unit runs `attribute.py` would propose (`Pl/pl_act`'s `.sdata2` run is one
    such case), while an unreferenced stretch stays a single run.
    """
    out: list[dict] = []
    for run in runs:
        current = None
        for address, name, end in run["items"]:
            sig = frozenset(fn_unit[fn] for fn in refs.get(name, ()) if fn in fn_unit)
            if current is not None and sig == current["sig"]:
                current["end"] = max(current["end"], end)
                current["names"].append(name)
            else:
                current = {"section": run["section"], "start": address, "end": end,
                           "names": [name], "sig": sig}
                out.append(current)
    out.sort(key=lambda r: (_section_key(r["section"]), r["start"], r["end"]))
    return out


def refs_by_symbol(funcs: dict) -> dict[str, set[str]]:
    """`data symbol -> the functions that reference it`, from the tudiscover graph cache."""
    out: dict[str, set[str]] = {}
    for function, record in (funcs or {}).items():
        for ref in record.get("refs") or ():
            out.setdefault(ref, set()).add(function)
    return out


def function_units(symbols: list[dict], splits: list[dict]) -> dict[str, str]:
    """`function symbol -> owning unit path without extension`, from `splits.txt`."""
    out: dict[str, str] = {}
    for entry in symbols:
        if (entry.get("type") or "") != "function":
            continue
        unit = _covered(splits, entry["section"], entry["address"])
        if unit:
            out[entry["name"]] = os.path.splitext(unit)[0]
    return out


def run_unit(run: dict, refs: dict[str, set[str]], fn_unit: dict[str, str]) -> tuple[str, int]:
    """The unit a run belongs to, and how many of its symbols leak across units.

    Exactly one registered unit referencing the run names it; anything else (no reference, or several
    units) leaves the run unnamed here and the covering split region stands in for the future unit.
    """
    units: set[str] = set()
    per_symbol: dict[str, set[str]] = {}
    for name in run["names"]:
        owners = {fn_unit[fn] for fn in refs.get(name, ()) if fn in fn_unit}
        per_symbol[name] = owners
        units |= owners
    unit = next(iter(units)) if len(units) == 1 else region_name(run)
    leak = sum(1 for owners in per_symbol.values() if owners - {unit})
    return unit, leak


def region_name(run: dict) -> str:
    """`auto_09_80790E20_sdata` - the split region, when no registered unit references the run."""
    return "auto/%08X_%s" % (run["start"], run["section"].lstrip("."))


def verdict_for(names: list[str], leak: int, density: float, start: int) -> str:
    """The queue's pre-measurement verdict; see the module docstring for the vocabulary."""
    if any(name in LINKER_GENERATED for name in names):
        return "never"
    if TRK_VECTOR_TABLE[0] <= start <= TRK_VECTOR_TABLE[1]:
        return "owner-held"
    if leak or density < 0.5:
        return "not claimed"
    return "proposed"


def build_entries(symbols: list[dict], splits: list[dict], funcs: dict, cover) -> list[dict]:
    """The whole queue, in the plan's shape, in a deterministic order - pure.

    `cover(section, address)` names the split object covering an address (the ledger's
    `Objects.covering`), so an unattributed run still resolves to the region it lives in.
    """
    refs = refs_by_symbol(funcs)
    fn_unit = function_units(symbols, splits)
    by_section: dict[str, list[dict]] = {}
    for entry in symbols:
        by_section.setdefault(entry["section"], []).append(entry)
    entries = []
    for run in split_by_owner(group_runs(select(symbols, splits)), refs, fn_unit):
        names = run["names"]
        nameset = set(names)
        unit, leak = run_unit(run, refs, fn_unit)
        inside = [e for e in by_section.get(run["section"], ())
                  if run["start"] <= e["address"] < run["end"]]
        filler = sum(1 for e in inside if e["name"] not in nameset)
        labels = len(names)
        density = round(labels / (labels + filler), 2) if labels + filler else 1.0
        entries.append({
            "unit": unit,
            "section": run["section"],
            "start": run["start"],
            "end": run["end"],
            "labels": labels,
            "leak": leak,
            "density": density,
            "verdict": verdict_for(names, leak, density, run["start"]),
        })
    entries.sort(key=lambda e: (_section_key(e["section"]), e["start"], e["end"], e["unit"]))
    return entries


def render(entries: list[dict]) -> str:
    """The file text: sorted keys, so the same entries always render the same bytes."""
    return json.dumps(entries, indent=1, sort_keys=True) + "\n"


def write_queue(path: str, text: str) -> None:
    """Atomic write through the shared-file layer: a crash leaves the old queue or the new one.

    The queue is deterministic LF JSON, so the write is byte-identical to the old temp+`os.replace`;
    routing it through `sharedfiles.Transaction` inherits the exact-bytes rollback and the single
    temp-file primitive the other shared-file writers use (docs/plan.md 7.12).
    """
    tx = sf.Transaction()
    try:
        tx.write(Path(path), text)
    except BaseException:
        tx.rollback()
        raise
    finally:
        tx.cleanup()


# -- the impure edges: read the repository, print a summary --------------------------------------------------

def read_graph_cache(root: str) -> tuple[dict, str]:
    """The tudiscover graph cache, read only when its stamp still matches `symbols.txt`.

    Never rebuild it: the rebuild is the 200-400 s asm walk, and this tool is a queue writer. A stale
    cache costs the reference attribution, not the backlog.
    """
    path = os.path.join(root, GRAPH_REL)
    if not os.path.exists(path):
        return {}, "no graph cache - runs will be named by split region only"
    try:
        data = json.loads(open(path, encoding="utf-8").read())
    except (ValueError, OSError) as exc:
        return {}, "unreadable graph cache (%s)" % exc
    symbols_path = os.path.join(root, "config", "RMHE08", "symbols.txt")
    if os.path.exists(symbols_path):
        digest = hashlib.sha1(open(symbols_path, "rb").read()).hexdigest()
        if (data.get("stamp") or {}).get("symbols") != digest:
            return {}, "graph cache predates symbols.txt - re-run `tudiscover.py cache`"
    return data.get("funcs") or {}, ""


def load_inputs(root: str) -> tuple[list[dict], list[dict], dict, object, str]:
    """(symbols, splits, funcs, cover, warning) from the ledger - the same state the other tools read."""
    import ledger as ledger_mod  # noqa: E402  (kept out of the module import for the pure selftest)

    led = ledger_mod.Ledger()
    symbols = [entry for entries in led.by_section.values() for entry in entries]
    funcs, warning = read_graph_cache(root)
    return symbols, led.splits, funcs, led.objects.covering, warning


def summary(entries: list[dict], path: str, warning: str, registered: set[str] | None = None) -> str:
    """The one screen an operator reads: where the backlog is, and what verdicts it carries."""
    if warning:
        print("WARNING: %s" % warning, file=sys.stderr)
    if not entries:
        return "queue empty - no unowned data"
    sections: dict[str, int] = {}
    verdicts: dict[str, int] = {}
    units: dict[str, int] = {}
    for entry in entries:
        sections[entry["section"]] = sections.get(entry["section"], 0) + 1
        verdicts[entry["verdict"]] = verdicts.get(entry["verdict"], 0) + 1
        units[entry["unit"]] = units.get(entry["unit"], 0) + 1
    bytes_ = sum(entry["end"] - entry["start"] for entry in entries)
    registered = registered or set()
    named = sum(1 for entry in entries if entry["unit"] in registered)
    lines = ["%d run(s), %d bytes, %d attributed to a registered unit" % (len(entries), bytes_, named),
             "  sections: " + ", ".join("%s %d" % (s, n) for s, n in
                                        sorted(sections.items(), key=lambda kv: _section_key(kv[0]))),
             "  verdicts: " + ", ".join("%s %d" % (v, n) for v, n in sorted(verdicts.items())),
             "  units:    " + ", ".join("%s %d" % (u, n) for u, n in
                                        sorted(units.items(), key=lambda kv: (-kv[1], kv[0]))[:8]),
             "  -> " + path]
    return "\n".join(lines)


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def sym(name, section, address, size=4, type_="object"):
        return {"name": name, "section": section, "address": address, "size": size, "type": type_}

    splits = [
        {"unit": "Pl/pl_act.cpp", "ranges": [{"section": ".text", "start": 0x1000, "end": 0x1100,
                                              "rename": None}]},
        {"unit": "main.cpp", "ranges": [{"section": ".text", "start": 0x2000, "end": 0x2100,
                                         "rename": None}]},
    ]

    # --- the selection rule
    fixtures = [
        sym("owned", ".data", 0x100, 4),            # covered by a split -> not backlog
        sym("fn_1000", ".text", 0x1000, 8, "function"),
        sym("frag", "extab", 0x300, 4),             # fragment -> travels with its code unit
        sym("code_label", ".text", 0x1200, 4, "label"),
        sym("free1", ".data", 0x400, 4),
        sym("free2", ".data", 0x404, 4),
    ]
    split_owned = [{"unit": b["unit"], "ranges": list(b["ranges"])} for b in splits]
    split_owned[0]["ranges"].append(
        {"section": ".data", "start": 0x100, "end": 0x108, "rename": None})
    backlog = select(fixtures, split_owned)
    check("a covered data symbol is not backlog", [e["name"] for e in backlog], ["free1", "free2"])
    check("functions are not data", all((e.get("type") or "") != "function" for e in backlog), True)
    check("fragment sections are not data", is_data_section("extab"), False)
    check(".text is not a data section", is_data_section(".text"), False)
    check(".sdata2 is a data section", is_data_section(".sdata2"), True)

    # --- the run grouping
    runs = group_runs(backlog)
    check("contiguous symbols merge into one run", len(runs), 1)
    check("a run keeps its extent", (runs[0]["start"], runs[0]["end"]), (0x400, 0x408))
    check("a run keeps every label", runs[0]["names"], ["free1", "free2"])
    gapped = group_runs([sym("a", ".data", 0x400, 4), sym("b", ".data", 0x410, 4)])
    check("a gap starts a new run", len(gapped), 2)
    zeros = group_runs([sym("z0", ".bss", 0x500, 0), sym("z1", ".bss", 0x501, 0)])
    check("zero-size labels do not break a run", len(zeros), 1)
    owners = {"fnA": "A/a", "fnB": "B/b"}
    refs2 = {"p": {"fnA"}, "q": {"fnB"}}
    cut = split_by_owner(group_runs([sym("p", ".data", 0x600, 4), sym("q", ".data", 0x604, 4)]),
                         refs2, owners)
    check("an owner change splits a contiguous run", len(cut), 2)
    check("the split keeps each half's owner", [r["sig"] for r in cut], [frozenset({"A/a"}), frozenset({"B/b"})])
    same = split_by_owner(group_runs([sym("p", ".data", 0x600, 4), sym("r", ".data", 0x604, 4)]),
                          {"p": {"fnA"}, "r": {"fnA"}}, owners)
    check("the same owner does not split", len(same), 1)

    # --- the verdicts
    check("linker-generated data is never claimed", verdict_for(["_rom_copy_info"], 0, 1.0, 0x80006624),
          "never")
    check("the TRK vector table is owner-held",
          verdict_for(["gTRKInterruptVectorTable"], 0, 1.0, 0x80004380), "owner-held")
    check("a leak is not claimed", verdict_for(["x"], 1, 1.0, 0x400), "not claimed")
    check("a thin run is not claimed", verdict_for(["x"], 0, 0.25, 0x400), "not claimed")
    check("a clean run is proposed", verdict_for(["x"], 0, 1.0, 0x400), "proposed")

    # --- attribution, density and leak, through the real builder
    symbols = [
        sym("fn_1000", ".text", 0x1000, 8, "function"),
        sym("fn_2000", ".text", 0x2000, 8, "function"),
        sym("gAct", ".sdata2", 0x800, 4),
        sym("gShared", ".sdata2", 0x820, 4),
        sym("gMain", ".sdata2", 0x824, 4),
        sym("gLonely", ".data", 0x900, 4),
    ]
    funcs = {"fn_1000": {"refs": ["gAct", "gShared"]}, "fn_2000": {"refs": ["gShared", "gMain"]}}
    entries = build_entries(symbols, splits, funcs, lambda s, a: None)
    by_name = {e["start"]: e for e in entries}
    check("a run referenced by one unit is attributed to it", by_name[0x800]["unit"], "Pl/pl_act")
    check("a single-unit run does not leak", by_name[0x800]["leak"], 0)
    check("a single-unit run is proposed", by_name[0x800]["verdict"], "proposed")
    check("labels counts the run's symbols", by_name[0x800]["labels"], 1)
    check("a run referenced by two units leaks", by_name[0x820]["leak"], 1)
    check("a leaking run is not claimed", by_name[0x820]["verdict"], "not claimed")
    check("a change of owner cuts the run in two", [e["start"] for e in entries if e["section"] == ".sdata2"],
          [0x800, 0x820, 0x824])
    check("the second half of a cut run is attributed", by_name[0x824]["unit"], "main")
    check("an unreferenced run falls back to a region name", by_name[0x900]["unit"], "auto/00000900_data")
    check("an unreferenced run is proposed", by_name[0x900]["verdict"], "proposed")
    check("the entry has exactly the plan's keys", sorted(by_name[0x800]), sorted(
        ["unit", "section", "start", "end", "labels", "leak", "density", "verdict"]))

    # --- density from filler: an owned symbol inside the run lowers it
    symbols_filler = symbols + [sym("owned_in_run", ".sdata2", 0x822, 1)]
    splits_filler = splits + [{"unit": "main.cpp", "ranges": [
        {"section": ".sdata2", "start": 0x822, "end": 0x823, "rename": None}]}]
    e2 = {e["start"]: e for e in build_entries(symbols_filler, splits_filler, funcs, lambda s, a: None)}
    check("an owned symbol inside the run is filler", e2[0x820]["density"], 0.5)

    # --- deterministic ordering and idempotency
    check("the ordering is stable across builds", render(build_entries(symbols, splits, funcs, None)),
          render(build_entries(symbols, splits, funcs, None)))
    check("entries are sorted by section order then address",
          [e["section"] for e in build_entries(symbols, splits, funcs, None)],
          sorted([e["section"] for e in build_entries(symbols, splits, funcs, None)],
                 key=lambda s: _section_key(s)))
    check("a run sorts before a later one", [e["start"] for e in build_entries(
        [sym("a", ".sdata2", 0x900, 4), sym("b", ".sdata2", 0x800, 4)], splits, {}, None)], [0x800, 0x900])
    check("a section orders before a later one", [e["section"] for e in build_entries(
        [sym("a", ".sdata2", 0x800, 4), sym("b", ".data", 0x900, 4)], splits, {}, None)],
        [".data", ".sdata2"])
    check("the text is valid JSON", json.loads(render(entries))[0]["section"], ".data")

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "tools", "units", "data-queue.json")
        text = render(entries)
        write_queue(path, text)
        first = open(path, "rb").read()
        write_queue(path, text)
        check("writing twice is byte-identical", open(path, "rb").read(), first)
        check("the shared-file layer leaves no temp file",
              sorted(str(p) for p in Path(tmp).rglob("*" + sf.TMP_SUFFIX)), [])

        # --- round-trip through brief.py's own reader
        from units import brief as brief_mod  # noqa: E402
        check("brief.py reads the queue back", brief_mod.data_queue_entries(tmp, "Pl/pl_act"),
              [e for e in entries if e["unit"] == "Pl/pl_act"])
        check("brief.py finds nothing for a unit with no runs",
              brief_mod.data_queue_entries(tmp, "Nope/none"), [])
        # brief.py also accepts the {"entries": [...]} form; the writer's bare list must survive both
        wrapped = os.path.join(tmp, "tools", "units", "data-queue.json")
        write_queue(wrapped, json.dumps({"entries": entries}, indent=1, sort_keys=True) + "\n")
        check("the dict form is the same queue",
              brief_mod.data_queue_entries(tmp, "Pl/pl_act"),
              [e for e in entries if e["unit"] == "Pl/pl_act"])

        # --- a missing queue is not a crash for the reader
        os.remove(wrapped)
        check("a missing queue reads as empty", brief_mod.data_queue_entries(tmp, "Pl/pl_act"), [])

    # --- the graph-cache guard and the region fallback
    check("a missing cache costs only the attribution", read_graph_cache(
        os.path.join(os.sep, "does", "not", "exist")), ({}, "no graph cache - runs will be named by "
                                                          "split region only"))
    check("a run with no covering object still names itself",
          region_name({"section": ".sbss", "start": 0x807953C8}), "auto/807953C8_sbss")

    if fails:
        print("FAIL (%d)" % len(fails))
        for failure in fails:
            print("  " + failure)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--dry-run", action="store_true", help="report what would be written, write nothing")
    ap.add_argument("--limit", type=int, default=0, help="write only the first N entries (a preview)")
    ap.add_argument("--json", action="store_true", help="print the queue on stdout, write nothing")
    ap.add_argument("--out", default=None, help="output path (default: tools/units/data-queue.json)")
    ap.add_argument("--root", default=None, help="repository root (default: this checkout)")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    root = os.path.abspath(args.root) if args.root else os.path.dirname(os.path.dirname(HERE))
    symbols, splits, funcs, cover, warning = load_inputs(root)
    entries = build_entries(symbols, splits, funcs, cover)
    if args.limit:
        entries = entries[:args.limit]
    text = render(entries)
    path = args.out or os.path.join(root, QUEUE_REL)
    if args.json:
        sys.stdout.write(text)
        return 0
    print(summary(entries, path, warning, {os.path.splitext(b["unit"])[0] for b in splits}))
    if args.dry_run:
        print("dry run: nothing written")
        return 0
    write_queue(path, text)
    print("wrote %s" % path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
