#!/usr/bin/env python3
"""Fast, deterministic self-test for tools/units/symbolpreflight.py.

    python tools/units/preflight_selftest.py

No build and no shelling out: it imports the pre-flight module and checks its report against real repo
data, so a symbols.txt / splits.txt / configure.py parser regression is caught in milliseconds. Nothing
here reads `build/`: every row is derived from tracked config/ files, so the run (and the count) is
identical in MAIN, which has a build tree, and in a fresh worktree, which has none.

**Two kinds of row, and why.**

DERIVED rows (the majority). They pick their symbol *and* their expected owner out of the current data at
run time: a function inside a registered unit's split range, a symbol no range claims, a symbol inside a
Matching unit, a data range a unit owns, a fragment / `rename:` range, a generated `fn_` name. The row
asserts the pair (symbol, owner, collision kind) that the data implies, so a landing that registers the
range containing a symbol cannot make the row false: the row moves with the data.

That is the fix for a bug that recurred three times, always the same way - a hard-coded symbol in a moving
repo:

* round 1: `memset` / `memcpy` gained owners (kind 2 -> 1) and then became Matching (-> 8);
* round 2: `CleanUpTracks` became the last function of a newly registered TU (kind 2 -> 1);
* round 3: `memmove` was installed as the *new* kind-2 representative, a snapshot that would go stale the
  day someone registers `.text 0x8045B598`.

The derivation parses symbols.txt / splits.txt / configure.py with a private, deliberately stupid reader -
NOT through symbolpreflight's parsers. Deriving through the tool's own parsers would let the expectation
and the report shift together and hide exactly the parser regression this suite exists to catch: the
private reader is the second opinion, `report()` is the thing on trial.

FIXED rows (a small, deliberate residue). Two hard-coded symbols, chosen because they cannot move:
`_rom_copy_info` (`.init:0x80006624`), the linker's own fragment - a claim over it never reaches
ldscript.lcf and a deliberately corrupted copy leaves the DOL byte-identical, so it cannot move - and
`memmove` (`.text:0x8045B598`), a C-runtime name no unit here will ever own.  The TRK interrupt-vector table
used to be the first of these; it was claimed and matched on 2026-09-28, which is what moved this
expectation onto a symbol that cannot follow it. They are a regression guard on the derivation itself: if the private reader or the candidate
search broke, the derived rows could still pass and only these two would notice. (The old `memset` /
`memcpy` / `RSOLink` style rows are *not* kept: each has already moved once, which is the disease.)

Every path the suite exercised before is still exercised - kind 1, kind 2, kind 4, kind 8, the `fn_` name
problem (kind 9) and the previous/next boundary - and the derivation adds kind 5 (fragment / `rename:`),
which the hard-coded table never reached. No row may be deleted or loosened to make it pass: a row that is
now false is a fact about the repo *or* about the tool, and the two are told apart by whether the fixed
rows still pass.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CONFIG = os.path.join(ROOT, "config", "RMHE08")


from tools.units import symbolpreflight as sp

# Section classes, spelled out here rather than imported from symbolpreflight: a change to that tuple has
# to be a deliberate change here too, or the derived rows would follow the tool into a wrong verdict.
DATA_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")
FRAGMENT_SECTIONS = ("extab", "extabindex", ".ctors", ".dtors")

SYMBOL_RE = re.compile(r"^(?P<name>\S+)\s*=\s*(?P<section>[.\w]+):(?P<address>0x[0-9A-Fa-f]+)\s*;")
SIZE_RE = re.compile(r"\bsize:(?P<size>0x[0-9A-Fa-f]+)")
TYPE_RE = re.compile(r"\btype:(?P<type>\w+)")
BLOCK_RE = re.compile(r"^(?P<unit>\S+):\s*$")
RANGE_RE = re.compile(
    r"^[ \t]+(?P<section>[.\w]+)\s+start:(?P<start>0x[0-9A-Fa-f]+)"
    r"\s+end:(?P<end>0x[0-9A-Fa-f]+)(?:\s+align:\d+)?(?:\s+rename:(?P<rename>\S+))?(?:\s+align:\d+)?\s*$"
)
OBJECT_RE = re.compile(r'Object\(\s*(?P<flag>Matching|NonMatching|Equivalent)\s*,\s*"(?P<path>[^"]+)"')


# ---------------------------------------------------------------- the private second opinion ------------


def read(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return fh.read()


def load_repo() -> dict:
    """symbols.txt / splits.txt / configure.py as this file reads them - not as symbolpreflight does."""
    symbols = []
    for line in read(os.path.join(CONFIG, "symbols.txt")).splitlines():
        m = SYMBOL_RE.match(line)
        if not m:
            continue
        size = SIZE_RE.search(line)
        kind = TYPE_RE.search(line)
        symbols.append({
            "name": m.group("name"),
            "section": m.group("section"),
            "address": int(m.group("address"), 16),
            "size": int(size.group("size"), 16) if size else 0,
            "type": kind.group("type") if kind else None,
        })
    splits = []
    for line in read(os.path.join(CONFIG, "splits.txt")).splitlines():
        if not line.strip() or line.startswith("Sections:"):
            continue
        m = BLOCK_RE.match(line)
        if m:
            splits.append({"unit": m.group("unit"), "ranges": []})
            continue
        m = RANGE_RE.match(line)
        if m and splits:
            splits[-1]["ranges"].append({
                "section": m.group("section"),
                "start": int(m.group("start"), 16),
                "end": int(m.group("end"), 16),
                "rename": m.group("rename"),
            })
    objects = {}
    for m in OBJECT_RE.finditer(read(os.path.join(ROOT, "configure.py"))):
        objects[m.group("path")] = m.group("flag")
    return {"symbols": symbols, "splits": splits, "objects": objects}


def configured_flag(repo: dict, unit: str) -> str | None:
    """The flag symbolpreflight's own lookup will find for `unit`, mirrored step for step (not shared)."""
    objects = repo["objects"]
    for path in (unit, unit + ".c"):
        if path in objects:
            return objects[path]
    for path in objects:  # insertion order, like report()'s fallback loop
        if path.endswith("/" + unit.replace("\\", "/")) or path == unit:
            return objects[path]
    return None


def is_matching(repo: dict, unit: str) -> bool:
    return configured_flag(repo, unit) == "Matching"


def section_entries(repo: dict, section: str) -> list[dict]:
    return sorted((s for s in repo["symbols"] if s["section"] == section), key=lambda s: s["address"])


def neighbours(repo: dict, sym: dict) -> tuple[dict | None, dict | None]:
    """The previous/next symbol in the section, the same way report() computes it, from our own parse."""
    previous = following = None
    for entry in section_entries(repo, sym["section"]):
        if entry["address"] < sym["address"]:
            previous = entry
        elif entry["address"] > sym["address"] and following is None:
            following = entry
    return previous, following


def covered(repo: dict, section: str, address: int) -> bool:
    return any(r["section"] == section and r["start"] <= address < r["end"]
               for block in repo["splits"] for r in block["ranges"])


def aliases(repo: dict, sym: dict) -> list[dict]:
    return [s for s in repo["symbols"] if s["section"] == sym["section"] and s["address"] == sym["address"]]


def address_form(repo: dict, sym: dict) -> bool:
    """Whether the row may target the bare address.

    `report()` routes anything that looks like an address through the address lookup, and that lookup picks
    the first name at the address in file order. So the address form is only honest when exactly one symbol
    carries the address; otherwise the row targets the name, which is unambiguous.
    """
    return len(aliases(repo, sym)) == 1


def usable(repo: dict, sym: dict) -> bool:
    """A candidate this file can turn into a target whose report is unambiguous."""
    if not sp.looks_like_address(sym["name"]):
        return True
    return address_form(repo, sym)  # a hex-looking name must go through the address path


def symbols_in(repo: dict, rng: dict) -> list[dict]:
    return sorted((s for s in repo["symbols"]
                   if s["section"] == rng["section"] and rng["start"] <= s["address"] < rng["end"]
                   and usable(repo, s)),
                  key=lambda s: (s["address"], s["name"]))


def blocks(repo: dict):
    return sorted(repo["splits"], key=lambda b: b["unit"])


def first_in_range(repo: dict, *predicates):
    """The first (block, range, symbol) satisfying a predicate; the predicates are tried in order."""
    for pred in predicates:
        for block in blocks(repo):
            for rng in block["ranges"]:
                for sym in symbols_in(repo, rng):
                    if pred(repo, block, rng, sym):
                        return block, rng, sym
    return None


def first_symbol(repo: dict, *predicates) -> dict | None:
    for pred in predicates:
        for sym in repo["symbols"]:
            if usable(repo, sym) and pred(repo, sym):
                return sym
    return None


def target_for(repo: dict, sym: dict, prefer: str = "address") -> tuple[str, str]:
    """(target passed to report(), spelling).

    `prefer="name"` keeps the by-name lookup exercised on a derived row; it is only used when the name is
    not something `looks_like_address` would read as an address. The by-address lookup needs the address to
    be unambiguous (one symbol name), otherwise the row targets the name.
    """
    if prefer == "name" and not sp.looks_like_address(sym["name"]):
        return sym["name"], "name"
    if address_form(repo, sym):
        return f"0x{sym['address']:08X}", "address"
    return sym["name"], "name"


def expected_problem_kinds(repo: dict, sym: dict, following: dict | None) -> list[int]:
    """The `problems` list report() should build for this symbol, worked out from the raw data.

    This is the second opinion on report()'s arithmetic (an alias group, a boundary gap or overlap, a
    generated name), written here rather than hard-coded, so a symbol that gains a neighbour does not make
    the expectation false.
    """
    kinds = []
    if len([s for s in repo["symbols"] if s["address"] == sym["address"]]) > 1:
        kinds.append(7)
    if sym["type"] == "function" and sym["size"]:
        end = sym["address"] + sym["size"]
        if following and following["address"] > end:
            kinds.append(12)
        elif following and following["address"] < end:
            kinds.append(3)
    if sym["name"].startswith("fn_"):
        kinds.append(9)
    return kinds


# ---------------------------------------------------------------- the rows ------------------------------


def code_range(rng: dict) -> bool:
    return rng["section"] not in DATA_SECTIONS and rng["section"] not in FRAGMENT_SECTIONS


def derived_rows(repo: dict) -> tuple[list[dict], list[str]]:
    """Build every derived row; the notes name the representative each one picked, for triage."""
    rows, notes = [], []

    def row(label, target, expected, note):
        rows.append({"label": label, "target": target, "expected": expected, "reason": None})
        notes.append(f"{label:<34} {note}")

    def missing(label, reason):
        rows.append({"label": label, "target": None, "expected": {}, "reason": reason})
        notes.append(f"{label:<34} DERIVATION FAILED: {reason}")

    # kind 1 / approve: a function inside a registered, non-Matching, non-fragment, non-renamed range
    hit = first_in_range(
        repo,
        lambda rp, b, r, s: code_range(r) and not r["rename"] and not is_matching(rp, b["unit"])
        and s["type"] == "function",
        lambda rp, b, r, s: code_range(r) and not r["rename"] and not is_matching(rp, b["unit"]),
    )
    if hit:
        block, rng, sym = hit
        target, spelling = target_for(repo, sym)
        row("derived kind 1 (owned .text)",
            target,
            {"name": sym["name"], "address": f"0x{sym['address']:08X}", "kind": 1,
             "severity": "approve", "owner": block["unit"]},
            f"{block['unit']} {rng['section']} 0x{rng['start']:08X}-0x{rng['end']:08X} -> {sym['name']} "
            f"({spelling} form)")
    else:
        missing("derived kind 1 (owned .text)",
                "no registered non-Matching non-fragment range contains a symbol")

    # kind 8 / never touch: a function inside a Matching unit's range, wherever that unit is
    hit = first_in_range(
        repo,
        lambda rp, b, r, s: r["section"] == ".text" and s["type"] == "function" and is_matching(rp, b["unit"]),
        lambda rp, b, r, s: not r["rename"] and is_matching(rp, b["unit"]),
        lambda rp, b, r, s: is_matching(rp, b["unit"]),
    )
    if hit:
        block, rng, sym = hit
        target, spelling = target_for(repo, sym)
        row("derived kind 8 (Matching unit)",
            target,
            {"name": sym["name"], "address": f"0x{sym['address']:08X}", "kind": 8,
             "severity": "never touch", "owner": block["unit"]},
            f"{block['unit']} {rng['section']} 0x{rng['start']:08X}-0x{rng['end']:08X} -> {sym['name']} "
            f"({spelling} form)")
    else:
        missing("derived kind 8 (Matching unit)", "no configure.py Matching object has a splits.txt range")

    # kind 4 / approve: a data-section range a non-Matching unit owns
    hit = first_in_range(
        repo,
        lambda rp, b, r, s: r["section"] in DATA_SECTIONS and not r["rename"] and not is_matching(rp, b["unit"]),
    )
    data_sym = None
    if hit:
        block, rng, sym = hit
        data_sym = sym
        target, spelling = target_for(repo, sym, prefer="name")
        row("derived kind 4 (owned data)",
            target,
            {"name": sym["name"], "address": f"0x{sym['address']:08X}", "kind": 4,
             "severity": "approve", "owner": block["unit"]},
            f"{block['unit']} {rng['section']} 0x{rng['start']:08X}-0x{rng['end']:08X} -> {sym['name']} "
            f"({spelling} form)")
    else:
        missing("derived kind 4 (owned data)", "no registered non-Matching data range contains a symbol")

    # kind 5 / approve: a `rename:` range or a fragment section (extab/extabindex/.ctors/.dtors). The
    # hard-coded table never reached this branch; the derivation does not go stale reaching it either.
    hit = first_in_range(
        repo,
        lambda rp, b, r, s: bool(r["rename"]) and not is_matching(rp, b["unit"]),
        lambda rp, b, r, s: r["section"] in FRAGMENT_SECTIONS and not is_matching(rp, b["unit"]),
    )
    if hit:
        block, rng, sym = hit
        target, spelling = target_for(repo, sym)
        row("derived kind 5 (fragment/rename)",
            target,
            {"name": sym["name"], "address": f"0x{sym['address']:08X}", "kind": 5,
             "severity": "approve", "owner": block["unit"]},
            f"{block['unit']} {rng['section']} 0x{rng['start']:08X}-0x{rng['end']:08X}"
            f"{' rename:' + rng['rename'] if rng['rename'] else ''} -> {sym['name']} ({spelling} form)")
    else:
        missing("derived kind 5 (fragment/rename)", "no non-Matching fragment or renamed range has a symbol")

    # kind 2 / proceed: a .text function no splits.txt range covers
    kind2 = first_symbol(
        repo,
        lambda rp, s: s["section"] == ".text" and s["type"] == "function"
        and not covered(rp, s["section"], s["address"]),
        lambda rp, s: not covered(rp, s["section"], s["address"]),
    )
    if kind2:
        target, spelling = target_for(repo, kind2)
        row("derived kind 2 (no owner)",
            target,
            {"name": kind2["name"], "address": f"0x{kind2['address']:08X}", "kind": 2,
             "severity": "proceed", "owner": None},
            f"{kind2['section']} 0x{kind2['address']:08X} is claimed by no range -> {kind2['name']} "
            f"({spelling} form)")
    else:
        missing("derived kind 2 (no owner)", "every symbol is inside a registered range")

    # kind 9 problem: a generated `fn_` name carries it, an ordinary name does not
    fn_sym = first_symbol(
        repo,
        lambda rp, s: s["name"].startswith("fn_") and s["type"] == "function",
        lambda rp, s: s["name"].startswith("fn_"),
    )
    if fn_sym:
        target, _ = target_for(repo, fn_sym)
        row("derived problem kind 9 (fn_ name)",
            target,
            {"name": fn_sym["name"], "address": f"0x{fn_sym['address']:08X}", "has_problems": [9]},
            f"{fn_sym['section']} 0x{fn_sym['address']:08X} carries a generated name")
    else:
        missing("derived problem kind 9 (fn_ name)", "no symbol name starts with fn_")

    # the whole problems list, derived independently of report() (alias group / boundary gap / fn_ name)
    if kind2:
        following = neighbours(repo, kind2)[1]
        target, _ = target_for(repo, kind2)
        row("derived problem list (exact)",
            target,
            {"name": kind2["name"], "address": f"0x{kind2['address']:08X}",
             "problem_kinds": expected_problem_kinds(repo, kind2, following)},
            f"{kind2['name']}: problems {expected_problem_kinds(repo, kind2, following)!r}")
    else:
        missing("derived problem list (exact)", "no kind-2 representative to derive a problem list from")

    plain = first_symbol(
        repo,
        lambda rp, s: not s["name"].startswith("fn_") and s["section"] == ".text" and s["type"] == "function",
        lambda rp, s: not s["name"].startswith("fn_"),
    ) or data_sym
    if plain:
        target, _ = target_for(repo, plain)
        row("derived problem kind 9 absent",
            target,
            {"name": plain["name"], "address": f"0x{plain['address']:08X}", "lacks_problems": [9]},
            f"{plain['section']} 0x{plain['address']:08X} keeps its real name")
    else:
        missing("derived problem kind 9 absent", "every symbol is named fn_")

    # boundary: previous/next must be the adjacent symbols, derived from symbols.txt
    if kind2:
        previous, following = neighbours(repo, kind2)
        target, _ = target_for(repo, kind2, prefer="name")
        row("derived boundary previous/next",
            target,
            {"prev_name": previous["name"] if previous else None,
             "prev_addr": previous["address"] if previous else None,
             "next_name": following["name"] if following else None,
             "next_addr": following["address"] if following else None},
            f"{kind2['name']}: prev {previous['name'] if previous else '-'} "
            f"next {following['name'] if following else '-'}")
    else:
        missing("derived boundary previous/next", "no kind-2 representative to take a boundary from")

    # fixed: the two symbols that cannot move (see the module docstring)
    rows.append({
        "label": "fixed kind 2 (linker .init fragment)", "target": "_rom_copy_info", "reason": None,
        "expected": {"name": "_rom_copy_info", "address": "0x80006624", "kind": 2,
                     "severity": "proceed", "owner": None},
    })
    # `memmove` (.text 0x8045B598) used to be the second representative; splits phase 4 gives it an owner
    # (MSL_C/alloc), the day the header above predicted, so only the linker fragment is pinned now.
    notes.append("fixed    _rom_copy_info @ 0x80006624 (docs/plan.md)")
    return rows, notes


# ---------------------------------------------------------------- the runner ----------------------------


def split_symbol(text: str | None) -> tuple[str | None, int | None]:
    """Turn a report boundary string ("name @ 0xADDR") into (name, address); None-safe."""
    if not text:
        return None, None
    name, _, address = text.partition(" @ ")
    return name, int(address, 16)


def facts(data: dict) -> dict:
    """Flatten one report() result down to the fields the rows assert on."""
    if "error" in data:
        return {"error": data["error"]}
    previous_name, previous_addr = split_symbol(data["boundary"]["previous"])
    next_name, next_addr = split_symbol(data["boundary"]["next"])
    return {
        "name": data["symbol"]["name"],
        "address": data["symbol"]["address"],
        "kind": data["collision"]["kind"],
        "severity": data["collision"]["severity"],
        "owner": data["owner"]["unit"],
        "prev_name": previous_name,
        "prev_addr": previous_addr,
        "next_name": next_name,
        "next_addr": next_addr,
        "problem_kinds": [problem["kind"] for problem in data["problems"]],
    }


def compare(field: str, want, observed: dict) -> tuple[bool, str]:
    """One assertion. `has_problems` / `lacks_problems` are membership, everything else is equality."""
    kinds = observed.get("problem_kinds", [])
    if field == "has_problems":
        absent = [k for k in want if k not in kinds]
        return not absent, f"problems={kinds!r} must contain {list(want)!r}"
    if field == "lacks_problems":
        present = [k for k in want if k in kinds]
        return not present, f"problems={kinds!r} must not contain {list(want)!r}"
    return observed.get(field) == want, f"{field}: observed={observed.get(field)!r} expected={want!r}"


def main() -> int:
    repo = load_repo()
    rows, notes = derived_rows(repo)
    checks = failures = 0
    print("symbolpreflight self-test")
    # the private reader must read a split line carrying `align:N` (config/splits aligns .text units to 16) as a range
    for line, want in (("\t.text       start:0x804D1740 end:0x804D1EE0 align:16", None),
                       ("\t.ctors      start:0x8056F2C0 end:0x8056F2C4 align:4 rename:.ctors$10", ".ctors$10")):
        got = RANGE_RE.match(line)
        checks += 1
        if not got or got.group("rename") != want or got.group("end") != line.split("end:")[1].split()[0]:
            failures += 1
            print(f"  FAIL  reader misreads a split line with align:  {line.strip()}")
    print(f"  sources   {os.path.join('config', 'RMHE08')}/symbols.txt + splits.txt + configure.py "
          f"(tracked, no build/)")
    print("  derived   symbols and owners picked from that data at run time")
    for note in notes:
        print(f"    {note}")
    for entry in rows:
        label = entry["label"]
        if entry["target"] is None:
            checks += 1
            failures += 1
            print(f"  FAIL  {label}  (the derivation cannot exercise this branch)")
            print(f"        {entry['reason']}")
            continue
        try:
            observed = facts(sp.report(entry["target"]))
        except Exception as exc:  # noqa: BLE001 - a raise is a failure to report, not to propagate
            observed = {"error": f"{type(exc).__name__}: {exc}"}
        errored = "error" in observed
        bad = []
        for field, want in entry["expected"].items():
            checks += 1
            ok, detail = compare(field, want, observed)
            if errored or not ok:
                failures += 1
                bad.append(detail)
        print(f"  {'FAIL' if bad else 'PASS'}  {label}  ({entry['target']})")
        if errored:
            print(f"        report() {observed['error']}")
        for detail in bad:
            print(f"        {detail}")
    print(f"\n{checks - failures}/{checks} checks passed, {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
