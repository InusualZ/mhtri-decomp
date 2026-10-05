#!/usr/bin/env python3
"""Find immediates dtk relocated against a code or extab label (`lis`/`addi` of `fn_X+N`, `@eti_X+N`) and print the
`block_relocations` entries that keep them constants. Spec: docs/tools/spec/immreloc.md.
CLI: immreloc.py [-u UNIT ...] [--obj-dir DIR] [--ours-dir DIR] [--unblocked] [--all-code] [--json] [--root DIR]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import subprocess
from dataclasses import dataclass, field

from tools.lib import names as libnames
from tools.lib import objcompare, ppc
from tools.lib import repo as _repo
from tools.lib import report as libreport
from tools.lib.binary.elf import Elf, ElfError
from tools.lib.project.symbols import SymbolMap

GAME = _repo.VERSION
CODE_SECTIONS = (".text", ".init")
#: Sections code never takes the address of: unwind tables are reached by the runtime, not by a `lis`/`addi`.
UNWIND_SECTIONS = ("extab", "extabindex")
#: `R_PPC_ADDR16_LO`, `_HI`, `_HA`: the halves of a 32-bit constant built in two instructions.
LO, HI, HA = 4, 5, 6
HALF_TYPES = (LO, HI, HA)
#: Where `--unblocked` writes its scratch split (gitignored build output of the tree).
SCRATCH_REL = os.path.join("build", "tmp", "immreloc")
#: The error-code shape: a high half `0x8001..0x801F` (a module id; the network codes seen are 0x8001..0x800A) and a
#: low half under 0x100. A real address can sit there (low `.text` starts at 0x80004000), so the shape is a reading,
#: the target-vs-ours comparison is the evidence; a value outside it (`OSDisableInterrupts+0xC`) is listed for review.
ERROR_CODE_HIGH = range(0x8001, 0x8020)
ERROR_CODE_LOW_MAX = 0x100


@dataclass
class Candidate:
    """One relocated pair (or lone half) that reads as an immediate."""
    unit: str
    function: str
    section: str
    start: int                 # DOL address of the first instruction
    end: int                   # DOL address after the last instruction
    target: str                # the label dtk relocated against
    target_section: str
    addend: int
    value: int
    halves: list = field(default_factory=list)       # relocation type names, in address order
    ours: str = ""                                    # what our compiled object does there
    ours_immediate: bool = False
    covered_by: str | None = None
    score: float | None = None

    @property
    def shape(self) -> str:
        if self.value >> 16 in ERROR_CODE_HIGH and (self.value & 0xFFFF) < ERROR_CODE_LOW_MAX:
            return "error-code"
        return ""

    @property
    def proposed(self) -> bool:
        """Worth an entry: the error-code shape, or our object materialises the same value as an immediate."""
        return bool(self.shape) or self.ours_immediate

    def entry(self) -> str:
        """The `block_relocations` item (config.yml's `source`/`end` form)."""
        return "- source: %s:0x%08X\n  end: %s:0x%08X" % (self.section, self.start, self.section, self.end)

    def to_dict(self) -> dict:
        return {"unit": self.unit, "function": self.function, "section": self.section,
                "start": "0x%08X" % self.start, "end": "0x%08X" % self.end, "target": self.target,
                "target_section": self.target_section, "addend": "0x%X" % self.addend, "value": "0x%08X" % self.value,
                "halves": self.halves, "shape": self.shape, "ours": self.ours, "ours_immediate": self.ours_immediate,
                "covered_by": self.covered_by, "score": self.score, "proposed": self.proposed, "entry": self.entry()}


# --- the map -----------------------------------------------------------------------------------------------------

class Map:
    """Name -> `(section, address, size)` from a `symbols.txt` (a duplicated name is ambiguous and dropped)."""

    def __init__(self, rows) -> None:
        self.by_name: dict[str, tuple[str, int, int]] = {}
        dup = set()
        for r in rows:
            if r.name in self.by_name:
                dup.add(r.name)
            self.by_name[r.name] = (r.section, r.address, r.size or 0)
        for n in dup:
            self.by_name.pop(n, None)

    @classmethod
    def read(cls, path: str) -> "Map":
        return cls(SymbolMap(path).rows())

    def get(self, name: str) -> tuple[str, int, int] | None:
        return self.by_name.get(name)


def guess_section(name: str) -> str | None:
    """The section a dtk label's name spells when the map does not carry it (`@etb_` extab, `@eti_` extabindex,
    `fn_` code)."""
    if name.startswith("@etb_"):
        return "extab"
    if name.startswith("@eti_"):
        return "extabindex"
    if name.startswith("fn_"):
        return ".text"
    return None


# --- the scan ------------------------------------------------------------------------------------------------------

def _functions(elf: Elf) -> list:
    return sorted((s for s in elf.symbols if s.defined and s.section in CODE_SECTIONS and s.size and s.type == 2),
                  key=lambda s: (s.section, s.value))


def _containing(funcs, section: str, offset: int):
    for f in funcs:
        if f.section == section and f.value <= offset < f.value + f.size:
            return f
    return None


def resolve_label(elf: Elf, sym, cmap: Map) -> tuple[str, int] | None:
    """`(section, address)` of a relocation's symbol: the map's row, else (defined here) the object's own section via
    a neighbour the map places, else the section and address its dtk name spells."""
    hit = cmap.get(sym.name)
    if hit:
        return hit[0], hit[1]
    if sym.defined and sym.section:
        for other in elf.symbols:
            if other.defined and other.section == sym.section and other.name != sym.name:
                o = cmap.get(other.name)
                if o and o[0] == sym.section:
                    return sym.section, o[1] - other.value + sym.value
    sec, addr = guess_section(sym.name), libnames.address_of(sym.name)
    return (sec, addr) if sec and addr is not None else None


def scan_object(target: Elf, cmap: Map, unit: str = "", all_code: bool = False) -> list[Candidate]:
    """Every relocated `@ha`/`@h`/`@l` half in a target object's code whose label is code with an addend (or any code
    label with `all_code`), or an unwind table: grouped per function, label and addend into one candidate."""
    funcs = _functions(target)
    groups: dict[tuple, list] = {}
    for r in target.relocs():
        if r.type not in HALF_TYPES or r.section not in CODE_SECTIONS:
            continue
        sym = target.symbols[r.symbol]
        where = resolve_label(target, sym, cmap)
        if where is None:
            continue
        sec, addr = where
        if sec in CODE_SECTIONS:
            if r.addend == 0 and not all_code:
                continue                                  # `fn` itself: a function pointer
        elif sec not in UNWIND_SECTIONS:
            continue
        f = _containing(funcs, r.section, r.offset)
        if f is None:
            continue
        groups.setdefault((f.name, r.section, sym.name, r.addend, sec, addr), []).append((r.offset & ~3, r.type))
    out = []
    for (fname, section, label, addend, sec, addr), halves in sorted(groups.items()):
        frow = cmap.get(fname)
        fn = next(f for f in funcs if f.name == fname and f.section == section)
        if frow is None:
            continue
        base = frow[1] - fn.value
        for run in _runs(sorted(halves)):
            out.append(Candidate(unit, fname, section, base + run[0][0], base + run[-1][0] + 4, label, sec, addend,
                                 (addr + addend) & 0xFFFFFFFF, [_TYPE_NAME[t] for _o, t in run]))
    return out


#: Halves further apart than this are separate sites: one `source` range must never span the code between two of them.
RUN_GAP = 0x10


def _runs(halves: list) -> list[list]:
    """`[(offset, type)]` (sorted) cut wherever two neighbours are more than `RUN_GAP` bytes apart."""
    out: list[list] = []
    for h in halves:
        if out and h[0] - out[-1][-1][0] <= RUN_GAP:
            out[-1].append(h)
        else:
            out.append([h])
    return out


_TYPE_NAME = {LO: "@l", HI: "@h", HA: "@ha"}


def ours_evidence(c: Candidate, ours: Elf | None, ours_relocs: dict | None) -> tuple[str, bool]:
    """What our compiled object does in the same function: materialises the value as an immediate (`lis`+`addi|ori`,
    no relocation on it), relocates it too, or does not say."""
    if ours is None:
        return "no compiled object", False
    fn = next((s for s in ours.symbols if s.name == c.function and s.defined and s.section == c.section), None)
    if fn is None:
        return "our object defines no %s" % c.function, False
    sec = ours.section(c.section)
    code = sec.data[fn.value:fn.value + fn.size] if sec is not None else b""
    relocated = {o & ~3 for o, _s, t, _a in (ours_relocs or {}).get(c.section, ()) if t in HALF_TYPES}
    for site, value in ppc.materialisations(code, fn.value):
        if value & 0xFFFFFFFF == c.value:
            if any(site - 16 <= o <= site for o in relocated):
                return "ours relocates it too at +0x%X" % (site - fn.value), False
            return "ours: immediate 0x%08X at +0x%X" % (c.value, site - 4 - fn.value), True
    return "ours: no matching immediate", False


def block_entries(config_text: str) -> list[dict]:
    """config.yml's `block_relocations` items as `{form, section, start, end}` (`form` is `source` or `target`)."""
    blocks, _problems = _repo.config_blocks(config_text)
    out, cur = [], None
    for line in blocks.get("block_relocations", [])[1:]:
        s = line.strip()
        key, _, val = s.lstrip("- ").partition(":")
        val = val.strip()
        sec, _, addr = val.rpartition(":")
        if s.startswith("-"):
            cur = {"form": key.strip(), "section": sec, "start": int(addr, 16) if addr else None, "end": None}
            out.append(cur)
        elif cur is not None and key.strip() == "end":
            cur["end"] = int(addr, 16)
    return [e for e in out if e["start"] is not None and e["end"] is not None]


def covered(c: Candidate, entries: list[dict]) -> str | None:
    """The config entry that already keeps this candidate a constant, or None."""
    for e in entries:
        if e["form"] == "source" and e["section"] == c.section and e["start"] <= c.start and c.end <= e["end"]:
            return "source %s:0x%08X..0x%08X" % (e["section"], e["start"], e["end"])
        if e["form"] == "target" and e["section"] == c.target_section and e["start"] <= c.value < e["end"]:
            return "target %s:0x%08X..0x%08X" % (e["section"], e["start"], e["end"])
    return None


# --- the tree ----------------------------------------------------------------------------------------------------

def object_files(obj_dir: str, units=None) -> list[tuple[str, str]]:
    """`[(unit, path)]` of the split objects under `obj_dir` (narrowed to `units`, extensions dropped)."""
    want = {os.path.splitext(u.replace("\\", "/"))[0] for u in units} if units else None
    out = []
    for dp, _dirs, names in os.walk(obj_dir):
        for n in names:
            if n.endswith(".o"):
                p = os.path.join(dp, n)
                unit = os.path.relpath(p, obj_dir)[:-2].replace("\\", "/")
                if want is None or unit in want:
                    out.append((unit, p))
    return sorted(out)


def unblocked_split(root: str, runner=subprocess.run) -> str:
    """Split the tree with `block_relocations` removed into `build/tmp/immreloc/unblocked` (the replay state) ->
    its `obj` directory. `SystemExit` when dtk is missing or fails."""
    with open(os.path.join(root, "config", GAME, "config.yml"), encoding="utf-8") as fh:
        blocks, problems = _repo.config_blocks(fh.read())
    if problems:
        raise SystemExit("config.yml cannot be read safely: %s" % problems[0])
    blocks.pop("block_relocations", None)
    blocks["write_asm"] = ["write_asm: false"]
    scratch = os.path.join(root, SCRATCH_REL)
    out = os.path.join(scratch, "unblocked")
    os.makedirs(out, exist_ok=True)
    cfg = os.path.join(scratch, "unblocked.yml")
    with open(cfg, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(line for lines in blocks.values() for line in lines) + "\n")
    dtk = os.path.join(root, "build", "tools", "dtk.exe" if os.name == "nt" else "dtk")
    if not os.path.isfile(dtk):
        raise SystemExit("no %s - run `ninja tools`" % os.path.relpath(dtk, root))
    p = runner([dtk, "dol", "split", "--no-update", cfg, out], cwd=root, capture_output=True, text=True,
               encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("`dtk dol split` of the unblocked config failed (exit %d): %s"
                         % (p.returncode, (p.stderr or "")[-400:]))
    return os.path.join(out, "obj")


def run(root: str, obj_dir: str, ours_dir: str, units=None, all_code: bool = False) -> dict:
    cmap = Map.read(os.path.join(root, "config", GAME, "symbols.txt"))
    cfg = os.path.join(root, "config", GAME, "config.yml")
    entries = block_entries(open(cfg, encoding="utf-8").read()) if os.path.exists(cfg) else []
    rep = libreport.read(os.path.join(root, "build", GAME, "report.json"))
    scores = {r["address"]: r["score"] for r in libreport.address_rows(rep) if r["address"] is not None}
    rows, unreadable = [], []
    objs = object_files(obj_dir, units)
    for unit, path in objs:
        try:
            target = Elf.read(path)
        except (OSError, ElfError) as exc:
            unreadable.append("%s: %s" % (unit, exc))
            continue
        found = scan_object(target, cmap, unit, all_code)
        if not found:
            continue
        ours_path = os.path.join(ours_dir, unit + ".o")
        ours = objcompare.be32(ours_path) if os.path.exists(ours_path) else None
        ours_relocs = objcompare.reloc_rows(ours_path)[0] if ours is not None else None
        for c in found:
            c.ours, c.ours_immediate = ours_evidence(c, ours, ours_relocs)
            c.covered_by = covered(c, entries)
            frow = cmap.get(c.function)
            c.score = scores.get(frow[1]) if frow else None
            rows.append(c)
    return {"objects": len(objs), "candidates": rows, "unreadable": unreadable, "entries": entries}


def entry_text(e: dict) -> str:
    return "%s %s:0x%08X..0x%08X" % (e["form"], e["section"], e["start"], e["end"])


def target_runs(cands: list[Candidate]) -> list[tuple[str, int, int]]:
    """`[(section, start, end)]` covering the values of unwind-label candidates, each value widened to its 16-byte
    line and touching lines merged - the `target:` form config.yml's extabindex entries use."""
    out: list[list] = []
    for c in sorted((c for c in cands if c.target_section in UNWIND_SECTIONS), key=lambda c: (c.target_section, c.value)):
        lo, hi = c.value & ~0xF, (c.value | 0xF) + 1
        if out and out[-1][0] == c.target_section and lo <= out[-1][2]:
            out[-1][2] = max(out[-1][2], hi)
        else:
            out.append([c.target_section, lo, hi])
    return [tuple(r) for r in out]


def summary(result: dict) -> dict:
    rows = result["candidates"]
    new = [c for c in rows if not c.covered_by]
    return {"candidates": len(rows), "covered": len(rows) - len(new), "new": len(new),
            "new_proposed": sum(1 for c in new if c.proposed), "new_review": sum(1 for c in new if not c.proposed),
            "entries": {entry_text(e): sum(1 for c in rows if c.covered_by == entry_text(e))
                        for e in result["entries"]}}


def render(result: dict, out=None) -> None:
    out = out or sys.stdout
    rows = result["candidates"]
    s = summary(result)
    print("immreloc: %d candidate(s) in %d object(s): %d covered by config.yml's block_relocations, %d new "
          "(%d proposed, %d to review)" % (s["candidates"], result["objects"], s["covered"], s["new"],
                                          s["new_proposed"], s["new_review"]), file=out)
    for text, n in s["entries"].items():
        print("  entry  %-44s %3d candidate(s)%s" % (text, n, "" if n else "  (none found: stale, or not this scan)"),
              file=out)
    for c in rows:
        state = "covered" if c.covered_by else ("NEW" if c.proposed else "review")
        print("  %-7s %s %s (%s) %s:0x%08X..0x%08X  %s -> %s+0x%X (%s) = 0x%08X%s; %s" % (
            state, c.unit, c.function, "%.1f%%" % c.score if c.score is not None else "no score", c.section,
            c.start, c.end, "/".join(c.halves), c.target, c.addend, c.target_section, c.value,
            " [%s]" % c.shape if c.shape else "", c.ours), file=out)
    for line in result["unreadable"]:
        print("  unreadable %s" % line, file=out)
    new = [c for c in rows if not c.covered_by and c.proposed]
    if new:
        print("\n# new block_relocations entries (paste under config.yml's `block_relocations:`; not applied)", file=out)
        for c in new:
            print("# %s: 0x%08X is a constant%s, not %s+0x%X" % (
                c.function, c.value, " (ours materialises it)" if c.ours_immediate else "", c.target, c.addend),
                file=out)
            print(c.entry(), file=out)
        runs = target_runs(new)
        if runs:
            print("\n# or, for the unwind-label ones, one `target:` entry per run of values (the extabindex entries' "
                  "form)", file=out)
            for sec, lo, hi in runs:
                print("- target: %s:0x%08X\n  end: %s:0x%08X" % (sec, lo, sec, hi), file=out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("-u", "--unit", action="append", help="a unit (`Network/NetworkSessionManagerPat`); repeatable")
    ap.add_argument("--root", help="the tree (default: the invocation's)")
    ap.add_argument("--obj-dir", help="the target objects to scan (default: <root>/build/RMHE08/obj)")
    ap.add_argument("--ours-dir", help="our compiled objects (default: <root>/build/RMHE08/src)")
    ap.add_argument("--unblocked", action="store_true",
                    help="scan a scratch split made without block_relocations (the replay: what the entries hide)")
    ap.add_argument("--all-code", action="store_true", help="also report a code label with no addend")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    root = _repo.repo_root(args.root) if args.root else _repo.repo_root()
    obj_dir = args.obj_dir or (unblocked_split(root) if args.unblocked else os.path.join(root, "build", GAME, "obj"))
    if not os.path.isdir(obj_dir):
        print("no target objects at %s - run `ninja`" % obj_dir, file=sys.stderr)
        return 2
    result = run(root, obj_dir, args.ours_dir or os.path.join(root, "build", GAME, "src"), args.unit, args.all_code)
    s = summary(result)
    if args.json:
        print(json.dumps({"tool": "immreloc", "obj_dir": obj_dir, "objects": result["objects"], "summary": s,
                          "rows": [c.to_dict() for c in result["candidates"]], "unreadable": result["unreadable"],
                          "ok": s["new_proposed"] == 0}, indent=1))
    else:
        render(result)
    return 1 if s["new_proposed"] else 0


if __name__ == "__main__":
    sys.exit(main())
