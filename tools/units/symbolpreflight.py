#!/usr/bin/env python3
"""Pre-flight one symbol before any source is written: who owns the address, and what would registration touch?

    python tools/units/symbolpreflight.py <address|name> [--json]

Offline and bounded: it reads `config/RMHE08/symbols.txt` (through `tools/symbols/symedit.py`'s parser),
`config/RMHE08/splits.txt`, `configure.py` and, when it exists, `build/RMHE08/config.json` (dtk's split
output, where the per-function `auto_*` objects live).

What it answers, in the order the decompile skill needs it:

    symbol      name, section, address, type, size
    boundary    the previous and next symbol in that section, and whether this symbol's end meets the next
                symbol's start (a delta is a boundary that will have to be explained)
    owner       the splits.txt unit whose range covers the address, if any, and the range itself
    configured  that unit's lib, mw_version, cflags and Matching/NonMatching flag from configure.py
    collision   the kind from the skill's stop list, its severity, and the analysis still owed
    drafts      a splits.txt block and a configure.py entry, as text - nothing is written

Severities are the skill's four: `escalate` (an anomaly that should not exist), `approve` (analyse, propose,
then wait for approval), `proceed` (the owner's claim is weak or absent) and `never touch`.

Nothing here decides anything: it reports the mechanical facts and names the analysis that remains.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = "RMHE08"

from tools.lib import project  # noqa: E402  (the one map / splits / configure parser)

DATA_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")
FRAGMENT_SECTIONS = ("extab", "extabindex", ".ctors", ".dtors")


def load_symbols() -> tuple[dict, dict, dict]:
    """Return (by_name, by_section_sorted, by_address) of entry dicts (`lib.project.Symbol.to_dict`)."""
    path = os.path.join(ROOT, "config", GAME, "symbols.txt")
    by_name, by_section, by_address = {}, {}, {}
    for entry in (e.to_dict() for e in project.SymbolMap(path).rows()):
        by_name[entry["name"]] = entry
        by_section.setdefault(entry["section"], []).append(entry)
        by_address.setdefault(entry["address"], []).append(entry["name"])
    for section in by_section:
        by_section[section].sort(key=lambda e: e["address"])
    return by_name, by_section, by_address


def load_splits() -> list[dict]:
    """`[{unit, ranges: [{section, start, end, rename}]}]` in file order (`lib.project.Splits`)."""
    path = os.path.join(ROOT, "config", GAME, "splits.txt")
    return [{"unit": b.unit,
             "ranges": [{"section": r.section, "start": r.start, "end": r.end, "rename": r.rename}
                        for r in b.ranges]}
            for b in project.Splits.read(path).blocks]


def load_configure() -> tuple[dict, list[dict]]:
    """Return (object_path -> {flag, path, lib, mw_version, cflags (the lib's group name)}, libs)."""
    objects, libs = {}, []
    for lib in project.Configure.load(os.path.join(ROOT, "configure.py")).libs():
        current = {"lib": lib.name, "objects": []}
        libs.append(current)
        for o in lib.objects:
            if o.flag not in project.configure.FLAGS:
                continue
            info = {"flag": o.flag, "path": o.path, "lib": lib.name, "mw_version": o.mw_version,
                    "cflags": o.lib_cflags}
            objects[o.path] = info
            current["objects"].append(info)
    return objects, libs


def covering(blocks: list[dict], section: str, address: int) -> dict | None:
    for block in blocks:
        for rng in block["ranges"]:
            if rng["section"] == section and rng["start"] <= address < rng["end"]:
                return {"unit": block["unit"], **rng}
    return None


def neighbours(section_entries: list[dict], address: int) -> tuple[dict | None, dict | None]:
    previous = following = None
    for entry in section_entries:
        if entry["address"] < address:
            previous = entry
        elif entry["address"] > address and following is None:
            following = entry
            break
    return previous, following


def severity_for(section: str, covered: dict | None, configured: dict | None) -> dict:
    """The mechanical part of the stop list; the analysis that remains is named, never skipped."""
    if covered is None:
        return {
            "kind": 2,
            "severity": "proceed",
            "why": "no splits.txt range covers the address: the region belongs to a per-function auto_* object or is unsplit",
            "analysis": "auto_* is build scaffolding, not TU evidence - but a wrong boundary upstream is usually what it means, so record the clash in the handover",
        }
    if configured and configured.get("flag") == "Matching":
        return {
            "kind": 8,
            "severity": "never touch",
            "why": f"{covered['unit']} is registered as Matching, so its bytes are linked in place of the original",
            "analysis": "none - re-attributing this region would change main.dol's hash for everyone",
        }
    if section in FRAGMENT_SECTIONS or covered.get("rename"):
        return {
            "kind": 5,
            "severity": "approve",
            "why": f"{covered['unit']} owns this {section} fragment" + (f" (renamed {covered['rename']})" if covered.get("rename") else ""),
            "analysis": "if that unit's source does not directly use this symbol, it probably does not belong to it: propose the re-attribution with that reason and ask",
        }
    if section in DATA_SECTIONS:
        return {
            "kind": 4,
            "severity": "approve",
            "why": f"{covered['unit']} already owns this {section} range",
            "analysis": "check whether that unit's source uses this data, then propose the move with its before/after measurement plan - a claimed pool has already cost 1.35 points here",
        }
    return {
        "kind": 1,
        "severity": "approve",
        "why": f"{covered['unit']} owns this {section} range",
        "analysis": "check whether that unit's source or object references the symbol and whether the boundaries agree; propose adding it to that unit, or re-deriving the boundary with the reason, then wait for approval",
    }


def looks_like_address(text: str) -> bool:
    """`0x…` always, or a bare hex run of at least 8 digits. A symbol really called `fade` is not an address."""
    if text.lower().startswith("0x"):
        return bool(re.fullmatch(r"0x[0-9a-fA-F]+", text))
    return bool(re.fullmatch(r"[0-9a-fA-F]{8,}", text))


def report(target: str) -> dict:
    by_name, by_section, by_address = load_symbols()
    splits = load_splits()
    objects, libs = load_configure()

    entry = None
    if looks_like_address(target):
        address = int(target, 16)
        for candidate in by_section.get(".text", []):
            if candidate["address"] == address:
                entry = candidate
                break
        if entry is None:
            for section in by_section:
                for candidate in by_section[section]:
                    if candidate["address"] == address:
                        entry = candidate
                        break
                if entry:
                    break
    else:
        entry = by_name.get(target)

    if entry is None:
        return {
            "target": target,
            "error": "not found in config/RMHE08/symbols.txt",
            "hint": "use tools/symbols/symedit.py at <address> to see what is around it",
        }

    section, address = entry["section"], entry["address"]
    size = entry.get("size") or 0
    previous, following = neighbours(by_section.get(section, []), address)
    covered = covering(splits, section, address)
    configured = None
    if covered:
        configured = objects.get(covered["unit"]) or objects.get(f"{covered['unit']}.c")
        if configured is None:
            for path, info in objects.items():
                if path.endswith("/" + covered["unit"].replace("\\", "/")) or path == covered["unit"]:
                    configured = info
                    break

    verdict = severity_for(section, covered, configured)
    problems = []

    if len(by_address.get(address, [])) > 1:
        names = by_address[address]
        functions = [n for n in names if by_name[n].get("type") == "function" and not n.startswith("lbl_")]
        if len(functions) > 1:
            problems.append(
                {
                    "kind": 7,
                    "severity": "escalate",
                    "why": f"two function names share {section}:0x{address:X}: {', '.join(functions)}",
                    "analysis": "should never happen - report both entries and the section, flag it as an anomaly, write nothing and do not pick a winner",
                }
            )
        else:
            problems.append(
                {
                    "kind": 7,
                    "severity": "proceed",
                    "why": f"alias group at {section}:0x{address:X}: {', '.join(names)}",
                    "analysis": "documented as normal in this repo (dtk synthesizes aliases); use the entry that carries size and type and note the alias",
                }
            )

    if entry.get("type") == "function" and size:
        end = address + size
        if following and following["address"] > end:
            problems.append(
                {
                    "kind": 12,
                    "severity": "approve",
                    "why": f"a {following['address'] - end} byte gap to {following['name']} at {section}:0x{following['address']:X}",
                    "analysis": "analyse the exact addresses against the neighbour's start, propose the corrected range and measure before and after",
                }
            )
        elif following and following["address"] < end:
            problems.append(
                {
                    "kind": 3,
                    "severity": "approve",
                    "why": f"this symbol's end 0x{end:X} overlaps {following['name']} at 0x{following['address']:X}",
                    "analysis": "one of the two boundaries is wrong: compare the target object's function list and the sizes in symbols.txt, name the boundary that is wrong and why, propose, then wait",
                }
            )

    if entry["name"].startswith("fn_"):
        problems.append(
            {
                "kind": 9,
                "severity": "proceed",
                "why": "the map still carries a generated name",
                "analysis": "check the shared runtime dump (docs/memory-dump.md) for the real name and signature - a rename is symedit.py plus the source, in one edit",
            }
        )

    drafts = {
        "splits.txt": draft_split(entry, splits),
        "configure.py": draft_configure(entry, configured, libs),
    }
    return {
        "target": target,
        "symbol": {
            "name": entry["name"],
            "section": section,
            "address": f"0x{address:08X}",
            "type": entry.get("type"),
            "size": size,
            "size_hex": f"0x{size:X}",
        },
        "boundary": {
            "previous": f"{previous['name']} @ 0x{previous['address']:08X}" if previous else None,
            "next": f"{following['name']} @ 0x{following['address']:08X}" if following else None,
        },
        "owner": {
            "unit": covered["unit"] if covered else None,
            "range": f"{covered['section']} 0x{covered['start']:08X}-0x{covered['end']:08X}" if covered else None,
            "configured": configured,
        },
        "collision": verdict,
        "problems": problems,
        "drafts": drafts,
    }


def draft_split(entry: dict, splits: list[dict]) -> str:
    """A template for the new unit's block. The unit name is a placeholder: propose the path from the module
    evidence (assert strings, pooled literals, escaped mangled names), and list *every* section the unit owns,
    not just this symbol's."""
    unit = "Dir/file.c"
    for block in splits:
        for rng in block["ranges"]:
            if rng["section"] == entry["section"] and rng["start"] <= entry["address"] < rng["end"]:
                unit = block["unit"]
    size = entry.get("size") or 0
    if size:
        end = f"0x{entry['address'] + size:08X}"
    else:
        end = "0x...  # size unknown - take the end from the next symbol's start"
    return "\n".join([f"{unit}:", f"\t{entry['section']}       start:0x{entry['address']:08X} end:{end}"])


def draft_configure(entry: dict, configured: dict | None, libs: list[dict]) -> str:
    """A starting point for the lib entry, carrying over what the owning/sibling unit already proves."""
    lib = None
    if configured and configured.get("lib"):
        lib = next((c for c in libs if c.get("lib") == configured["lib"]), None)
    if lib is None:
        lib = libs[0] if libs else {"lib": "<Lib>"}
    mw = (configured or {}).get("mw_version") or "<Wii/1.3 | Wii/1.0 | GC/3.0a3 - decide from the neighbours and the dump>"
    cflags = (configured or {}).get("cflags")
    cflags_line = (
        f'    "cflags": {cflags},'
        if cflags
        else '    "cflags": cflags_<lib>,   # start from cflags_runtime/cflags_base, override only with evidence'
    )
    unit_dir = os.path.dirname((configured or {}).get("path", "")).replace("\\", "/")
    source = f"{unit_dir}/<file>.c" if unit_dir else "Dir/<file>.c"
    return "\n".join(
        [
            "{",
            f'    "lib": "{lib.get("lib", "<Lib>")}",',
            f'    "mw_version": "{mw}",',
            cflags_line,
            '    "objects": [',
            f'        Object(NonMatching, "{source}"),',
            "    ],",
            "},",
        ]
    )


def render(data: dict) -> str:
    if "error" in data:
        return f"not found: {data['target']}\n  hint: {data['hint']}"
    sym, owner, col = data["symbol"], data["owner"], data["collision"]
    out = []
    out.append(
        f"symbol      {sym['name']}  {sym['section']}:{sym['address']}  {sym['type']} size:{sym['size_hex']}"
    )
    out.append(f"boundary    prev {data['boundary']['previous'] or '-'}   next {data['boundary']['next'] or '-'}")
    if owner["unit"]:
        out.append(f"owner       {owner['unit']}  ({owner['range']})")
    else:
        out.append("owner       none - no splits.txt range covers this address")
    if owner["configured"]:
        cfg = owner["configured"]
        out.append(
            f"configured  {cfg['flag']}  lib:{cfg['lib']}  mw_version:{cfg['mw_version']}  cflags:{cfg['cflags']}"
        )
    elif owner["unit"]:
        out.append("configured  that unit is not in configure.py (auto scaffolding or not yet registered)")
    out.append(f"collision   kind {col['kind']} [{col['severity']}] {col['why']}")
    out.append(f"            analysis still owed: {col['analysis']}")
    for problem in data["problems"]:
        out.append(f"also        kind {problem['kind']} [{problem['severity']}] {problem['why']}")
        out.append(f"            analysis still owed: {problem['analysis']}")
    out.append("")
    out.append("drafts (text only - nothing was written):")
    for line in data["drafts"]["splits.txt"].splitlines():
        out.append(f"  splits  | {line}")
    for line in data["drafts"]["configure.py"].splitlines():
        out.append(f"  config  | {line}")
    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", help="symbol name or address (0x...)")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    args = parser.parse_args()
    data = report(args.target)
    print(json.dumps(data, indent=2) if args.json else render(data))
    return 1 if "error" in data else 0


if __name__ == "__main__":
    raise SystemExit(main())
