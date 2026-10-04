#!/usr/bin/env python3
"""One function's disassembly from the target object, ours, or both side by side. Spec: docs/tools/spec/fnasm.md.
CLI: python tools/objdiff/fnasm.py [-u <unit>] <symbol|0xADDR> [--side target|ours|both] [--raw] [--json] | --object O [--object O] <symbol>."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import sys

from tools.lib import repo as _repo
from tools.lib import units as _units
from tools.lib.binary import objdump as _objdump
from tools.lib.binary.elf import Elf, ElfError
from tools.lib.project.splits import Splits
from tools.lib.project.symbols import SymbolMap

#: The symbol map and the splits, relative to a tree (address -> name -> owning unit).
SYMBOLS_REL = os.path.join("config", "RMHE08", "symbols.txt")
SPLITS_REL = os.path.join("config", "RMHE08", "splits.txt")
#: The code sections a function can live in.
CODE_SECTIONS = (".text", ".init")
#: A relocation type's operand spelling: the suffix written after the symbol (`lis r3,sym@ha`).
RELOC_SUFFIX = {"R_PPC_ADDR16_HA": "@ha", "R_PPC_ADDR16_LO": "@l", "R_PPC_ADDR16_HI": "@h",
                "R_PPC_EMB_SDA21": "@sda21", "R_PPC_ADDR16": "", "R_PPC_ADDR32": ""}
#: A branch's relocated operand is replaced whole by the symbol.
BRANCH_RELOCS = ("R_PPC_REL24", "R_PPC_REL14", "R_PPC_ADDR24", "R_PPC_ADDR14")
_TARGET_RE = re.compile(r"(?P<hex>[0-9a-f]+) <(?P<label>[^>]+)>$")
_DISP_RE = re.compile(r"^(?P<disp>-?(?:0x)?[0-9a-f]+)\((?P<reg>r\d+)\)$")


# --------------------------------------------------------------------------------------------------
# the pure core
# --------------------------------------------------------------------------------------------------
def function_extent(elf: Elf, name: str) -> tuple[str, int, int] | None:
    """`(section, offset, size)` of the defined symbol `name` in a code section of the object, or None."""
    for s in elf.symbols:
        if s.name == name and s.defined and s.section in CODE_SECTIONS:
            return s.section, s.value, s.size
    return None


def render_operands(mnemonic: str, operands: str, relocs: list[dict], lo: int) -> str:
    """The operands as a reader wants them: a relocated operand spelled `sym@ha`/`sym@l`/`sym@sda21(r13)`,
    a relocated branch target as the symbol, a local branch target as `+0xNN` from the function start."""
    parts = [p.strip() for p in operands.split(",")] if operands else []
    if relocs:
        r = relocs[0]
        if r["type"] in BRANCH_RELOCS and parts:
            parts[-1] = r["symbol"]
            return ",".join(parts)
        suffix = RELOC_SUFFIX.get(r["type"])
        if suffix is not None and parts:
            m = _DISP_RE.match(parts[-1])
            if m and int(m.group("disp"), 0) == 0:
                parts[-1] = "%s%s(%s)" % (r["symbol"], suffix, m.group("reg"))
                return ",".join(parts)
            if parts[-1] in ("0", "0x0"):
                parts[-1] = r["symbol"] + suffix
                return ",".join(parts)
        return ",".join(parts) + "  # %s %s" % (r["type"], r["symbol"])
    if mnemonic.startswith("b") and parts:
        m = _TARGET_RE.search(parts[-1])
        if m:
            parts[-1] = "+0x%X" % (int(m.group("hex"), 16) - lo)
    return ",".join(parts)


def parse_listing(text: str, section: str, lo: int, hi: int) -> list[dict]:
    """The instructions of `[lo, hi)` in `section` of an `objdump -dr` listing, each with its relocations.

    A relocation belongs to the instruction whose four bytes contain its offset (an `@ha`/`@l` reloc sits at
    the instruction's address + 2). `offset` is relative to the function start."""
    out: list[dict] = []
    current = None
    for raw in text.splitlines():
        line = _objdump.tokenize(raw)
        if line is None:
            continue
        if line.kind == "section":
            current = line.name
            continue
        if current != section or line.address is None or not lo <= line.address < hi:
            continue
        if line.kind == "insn":
            out.append({"address": line.address, "mnemonic": line.mnemonic, "operands": line.operands,
                        "raw": line.raw, "relocs": []})
        elif line.kind == "reloc" and out and out[-1]["address"] <= line.address < out[-1]["address"] + 4:
            out[-1]["relocs"].append({"type": line.reloc, "symbol": line.name})
    for insn in out:
        insn["offset"] = insn.pop("address") - lo
        insn["text"] = ("%-8s %s" % (insn["mnemonic"],
                                     render_operands(insn["mnemonic"], insn["operands"], insn["relocs"], lo))).rstrip()
    return out


def pair(left: list[dict], right: list[dict]) -> dict:
    """Index-aligned comparison of two listings: `{rows: [(marker, l, r)], same, first_divergence}`.

    The marker is ` ` for the same rendered text, `|` for a different one, `<`/`>` for a row only one side has.
    Relocated operands compare by symbol, local branches by their offset from the function start, so two objects
    that place the function at different offsets still compare equal."""
    rows = []
    same = 0
    first = None
    for i in range(max(len(left), len(right))):
        a = left[i] if i < len(left) else None
        b = right[i] if i < len(right) else None
        if a is not None and b is not None:
            marker = " " if a["text"] == b["text"] else "|"
        else:
            marker = "<" if b is None else ">"
        if marker == " ":
            same += 1
        elif first is None:
            first = i
        rows.append((marker, a, b))
    return {"rows": rows, "same": same, "first_divergence": first}


# --------------------------------------------------------------------------------------------------
# the impure edges
# --------------------------------------------------------------------------------------------------
def resolve_address(tree: str, address: int) -> tuple[str, int, str] | None:
    """`(name, offset into it, section)` of the code symbol in the tree's map covering `address`, or None."""
    best = None
    for row in SymbolMap(os.path.join(tree, SYMBOLS_REL)).rows():
        if row.section not in CODE_SECTIONS or not row.address <= address:
            continue
        if address < row.address + max(row.size or 0, 1):
            if best is None or row.address > best.address:
                best = row
    return (best.name, address - best.address, best.section) if best else None


def address_of(tree: str, name: str) -> tuple[str, int] | None:
    """`(section, address)` of a code symbol by name in the tree's map, or None."""
    for row in SymbolMap(os.path.join(tree, SYMBOLS_REL)).rows():
        if row.name == name and row.section in CODE_SECTIONS:
            return row.section, row.address
    return None


def owner_unit(tree: str, section: str, address: int) -> str | None:
    """The `splits.txt` unit whose range covers the address, or None."""
    try:
        hit = Splits.read(os.path.join(tree, SPLITS_REL)).covering(section, address)
    except OSError:
        return None
    return hit.unit if hit else None


def disassemble(obj: str, name: str, objdump: str | None, runner=None) -> dict:
    """One side: `{object, section, offset, size, instructions}` or `{object, error}`."""
    side: dict = {"object": obj}
    try:
        elf = Elf.read(obj)
    except (OSError, ElfError) as exc:
        side["error"] = "cannot read the object: %s" % exc
        return side
    extent = function_extent(elf, name)
    if extent is None:
        side["error"] = "the object defines no code symbol %s" % name
        return side
    section, lo, size = extent
    side.update(section=section, offset=lo, size=size)
    if runner is None:
        if not objdump:
            side["error"] = "no objdump (build/binutils is empty: `ninja tools`)"
            return side
        try:
            text = _objdump.disassemble(objdump, obj, sections=[section], relocs=True)
        except RuntimeError as exc:
            side["error"] = str(exc)
            return side
    else:
        text = runner(obj, section)
    side["instructions"] = parse_listing(text, section, lo, lo + size)
    return side


def run(objects: dict[str, str], name: str, objdump: str | None, runner=None) -> dict:
    """`{symbol, sides: {target|ours: side}, pair?}` for the named objects (`pair` when both listed)."""
    out: dict = {"symbol": name, "sides": {k: disassemble(v, name, objdump, runner) for k, v in objects.items()}}
    sides = out["sides"]
    if all(k in sides and "instructions" in sides[k] for k in ("target", "ours")):
        p = pair(sides["target"]["instructions"], sides["ours"]["instructions"])
        out["pair"] = {"same": p["same"], "rows": len(p["rows"]), "first_divergence": p["first_divergence"],
                       "markers": "".join(m for m, _a, _b in p["rows"])}
    return out


def render(result: dict, raw: bool = False, out=None) -> None:
    """The text form: one block per side, or one side-by-side block with a marker column."""
    out = out or sys.stdout
    sides = result["sides"]
    for key, side in sides.items():
        if "error" in side:
            print("%s %s: %s" % (key, side["object"], side["error"]), file=out)
    ok = {k: s for k, s in sides.items() if "instructions" in s}

    def cell(insn):
        if insn is None:
            return ""
        return ("%04X  %s%s" % (insn["offset"], (insn["raw"] + "  ") if raw else "", insn["text"]))

    if len(ok) == 2:
        t, o = ok["target"], ok["ours"]
        p = pair(t["instructions"], o["instructions"])
        print("%s  target %s (0x%X B)  vs  ours %s (0x%X B): %d of %d rows the same, first divergence %s"
              % (result["symbol"], t["object"], t["size"], o["object"], o["size"], p["same"], len(p["rows"]),
                 "none" if p["first_divergence"] is None else "row %d (+0x%X)" % (
                     p["first_divergence"], 4 * p["first_divergence"])), file=out)
        width = max([len(cell(a)) for _m, a, _b in p["rows"]] + [20])
        for marker, a, b in p["rows"]:
            print("  %-*s %s %s" % (width, cell(a), marker, cell(b)), file=out)
        return
    for key, side in ok.items():
        print("%s  %s %s %s+0x%X (0x%X B, %d instructions)"
              % (result["symbol"], key, side["object"], side["section"], side["offset"], side["size"],
                 len(side["instructions"])), file=out)
        for insn in side["instructions"]:
            print("  " + cell(insn), file=out)


def _rel(path: str, tree: str) -> str:
    """A path relative to the tree, forward slashes (left absolute when it is outside the tree)."""
    try:
        rel = os.path.relpath(path, tree)
    except ValueError:
        return path.replace("\\", "/")
    return (path if rel.startswith("..") else rel).replace("\\", "/")


def main(argv=None, runner=None, root=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("symbol", help="the function's name, or its DOL address (0x...) resolved through the map")
    ap.add_argument("-u", "--unit", help="the unit whose target/ours objects to read")
    ap.add_argument("--object", action="append", default=[], metavar="O",
                    help="an arbitrary object instead of a unit (twice: the first is the target, the second ours)")
    ap.add_argument("--side", choices=("target", "ours", "both"), default="both")
    ap.add_argument("--raw", action="store_true", help="print the raw instruction bytes too")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    tree = root or _repo.repo_root()
    name = args.symbol
    resolved = None
    where = None
    if re.fullmatch(r"0[xX][0-9A-Fa-f]+", name):
        hit = resolve_address(tree, int(name, 16))
        if hit is None:
            print("fnasm: no code symbol in %s covers %s" % (SYMBOLS_REL, name), file=sys.stderr)
            return 2
        resolved = {"address": int(name, 16), "symbol": hit[0], "offset": hit[1]}
        name, where = hit[0], (hit[2], int(args.symbol, 16))
    if args.object:
        if len(args.object) > 2:
            ap.error("--object takes at most two objects")
        objects = dict(zip(("target", "ours"), args.object))
    else:
        spec = args.unit
        if spec is None:
            where = where or address_of(tree, name)
            spec = owner_unit(tree, *where) if where else None
            if spec is None:
                print("fnasm: %s is in no registered unit's range (or not in the map) - name the unit with -u"
                      % args.symbol, file=sys.stderr)
                return 2
        try:
            unit = _units.Unit.resolve(spec, tree)
        except SystemExit as exc:
            print("fnasm: %s" % exc, file=sys.stderr)
            return 2
        both = {"target": _rel(unit.obj_target, tree), "ours": _rel(unit.obj_ours, tree)}
        objects = both if args.side == "both" else {args.side: both[args.side]}
        objects = {k: os.path.join(tree, v) for k, v in objects.items()}
    result = run(objects, name, _objdump.locate(tree), runner)
    for side in result["sides"].values():
        side["object"] = _rel(side["object"], tree)
    if resolved:
        result["resolved"] = resolved
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        if resolved:
            print("%s is %s+0x%X" % (args.symbol, resolved["symbol"], resolved["offset"]))
        render(result, raw=args.raw)
    return 0 if any("instructions" in s for s in result["sides"].values()) else 2


if __name__ == "__main__":
    raise SystemExit(main())
