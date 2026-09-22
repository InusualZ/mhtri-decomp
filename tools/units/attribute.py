"""Bulk attribution: partition an unclaimed `.text` region into candidate translation units.

The campaign's first objective is that **every symbol belongs to a unit**. Doing that one symbol at a
time (`decompile-symbol`) does not scale to 20k functions, and the expensive part is not the bookkeeping
- it is the *seam*: where one unit ends and the next begins. This tool answers only that question, in
bulk, from evidence the repo already has:

* `tools/splits/tudiscover.py` supplies the unit structure - the must-link anchors around a seed (a
  `__FILE__` assert string, a shared literal pool, a jump table, a kept `bl` to a tiny static) and the
  scored candidate cuts. This tool walks the region seed by seed and takes each seed's closure.
* `tools/units/symbolpreflight.py` supplies the collision verdicts, so a proposal never lands on a
  claimed range.

What it deliberately does **not** do: claim data. A `.text` line is safe (a `NonMatching` unit's bytes
are not linked, so the DOL hash cannot move), while a data line can make dtk drop the target's pool
relocations and *lower* a neighbouring unit's score (playbook idea 23). So `apply` writes `.text` only
and prints the data runs it saw for a second, measured pass.

Usage:

    python tools/units/attribute.py plan 0x80040598 0x800408A8
    python tools/units/attribute.py plan 0x80040598 0x800408A8 --json
    python tools/units/attribute.py apply 0x80040598 0x800408A8 --dry-run
    python tools/units/attribute.py apply 0x80040598 0x800408A8

`plan` is read-only. `apply` appends the `splits.txt` blocks, the `configure.py` objects and a stub
source per unit; nothing is measured by it - run `ninja` (which re-splits) and read the ledger after.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "splits"))
sys.path.insert(0, str(ROOT / "tools" / "units"))

import tudiscover as td  # noqa: E402  (path set above)
import symbolpreflight as pf  # noqa: E402

SPLITS = ROOT / "config" / "RMHE08" / "splits.txt"
CONFIGURE = ROOT / "configure.py"
AUTO_DIR = ROOT / "src" / "auto"

# A unit's name is a placeholder until someone has evidence for the real path. The header comment of
# each stub says so; this is the only place that decides what a placeholder looks like.
NAME_MAX = 48


def load(span_max: int = 0x4000, source_span_max: int = 0x8000):
    """The map, the graph and the analysis - the three expensive inputs, loaded once."""
    fns, labels = td.load_map()
    graph = td.build_graph(fns, labels, force=False)
    dol = td.Dol(td.DOL)
    an = td.analyse(fns, labels, graph, dol, span_max, source_span_max)
    return fns, labels, graph, an


def claimed_text() -> list[tuple[int, int, str]]:
    """Every `.text` range `splits.txt` already owns, as (start, end, unit) sorted by start."""
    out = []
    for unit, secs in td.claimed_units().items():
        if ".text" in secs:
            s, e = secs[".text"]
            out.append((s, e, unit))
    return sorted(out)


def in_claimed(addr: int, claimed: list[tuple[int, int, str]]) -> tuple[int, int, str] | None:
    for s, e, u in claimed:
        if s <= addr < e:
            return (s, e, u)
    return None


def placeholder(first: str, addr: int, cxx: bool) -> str:
    """`auto/<addr>_<symbol>.c` - unique, greppable, and obviously provisional."""
    stem = re.sub(r"[^A-Za-z0-9_]", "_", first)[:NAME_MAX] or "unit"
    if stem[0].isdigit():
        stem = "u" + stem
    return "auto/%08X_%s%s" % (addr, stem, ".cpp" if cxx else ".c")


def mangled(name: str) -> bool:
    """MWCC's C++ mangling puts the argument list after `__` (`fn__Fv`, `Pl_Skill_ck__FP4_PLWUs`)."""
    return "__" in name and re.search(r"__(F|Q)", name) is not None


def interior_seams(an, lo: int, hi: int) -> dict[int, list[tuple[str, str]]]:
    """Cut indices strictly inside `(lo, hi)` that a narrow, strong observation pins.

    `tudiscover.score_cuts` only scores the cuts *around* an existing closure, which is what extending a
    known unit needs. Partitioning a whole unclaimed region needs the opposite: the seams inside it. An
    observation is an interval of cuts it admits, so a seam is only worth taking when the interval is
    narrow (<= 4 cuts) and its kind is one of `tudiscover.STRONG` - a wide or weak observation says
    nothing about *where* the boundary is, only that somewhere nearby is possible. A cut inside a
    must-link anchor is illegal by definition.
    """
    out: dict[int, list[tuple[str, str]]] = {}
    anchors = an["must_link"]
    for olo, ohi, _w, kind, why in an["soft"]:
        if kind not in td.STRONG or ohi - olo + 1 > 4:
            continue
        for c in range(max(olo, lo + 1), min(ohi, hi - 1) + 1):
            if any(a < c <= b for a, b, _ in anchors):
                continue
            out.setdefault(c, []).append((kind, why))
    return out


def segments(an, lo: int, hi: int, min_bytes: int, max_bytes: int):
    """Split `[lo, hi)` at the pinned seams, then repair the pieces that are not TU-shaped.

    Two rules, both about what a translation unit is: a piece below `min_bytes` is too small to be a
    file of its own (merge it into its neighbour), and a piece above `max_bytes` is too large (split it
    at its cheapest seam and mark the result as a guess). Everything a region offers as evidence is used
    first; the repairs only apply where there is none.
    """
    seams = interior_seams(an, lo, hi)
    cuts = sorted(seams) + [hi]
    parts, prev = [], lo
    for c in cuts:
        if c > prev:
            parts.append((prev, c, seams.get(c) if c != hi else None))
            prev = c
    # merge from the left: a too-small part joins the part before it (or the one after, if it is first)
    merged: list[list] = []
    for lo_i, hi_i, why in parts:
        size = an["addr"][hi_i - 1] + an["size"][hi_i - 1] - an["addr"][lo_i]
        if merged and size < min_bytes:
            merged[-1][1] = hi_i
            merged[-1][2] = None          # the seam it was cut at is gone, so the union has no pin
        else:
            merged.append([lo_i, hi_i, why])
    # split from the left: a part over the cap is cut at the cap, flagged as a guess - but never inside a
    # must-link anchor, which is the one cut the evidence forbids outright (the anchor is the TU)
    def legal(c: int) -> bool:
        return not any(a < c <= b for a, b, _ in an["must_link"])

    out = []
    for lo_i, hi_i, why in merged:
        while an["addr"][hi_i - 1] + an["size"][hi_i - 1] - an["addr"][lo_i] > max_bytes:
            best = None
            for at in range(lo_i + 1, hi_i + 1):
                if an["addr"][at - 1] + an["size"][at - 1] - an["addr"][lo_i] <= max_bytes:
                    best = at
                else:
                    break
            at = best or lo_i + 1           # a single function over the cap is still one function
            while at < hi_i and not legal(at):
                at += 1                      # slide out of an anchor, even past the cap
            if at >= hi_i:
                out.append((lo_i, hi_i, why, "over --max-bytes with no legal cut - kept whole"))
                break
            out.append((lo_i, at, why, "capped at --max-bytes, seam is a guess"))
            lo_i, why = at, None
        else:
            out.append((lo_i, hi_i, why, None))
    return out


def propose(an, fns, labels, graph, start: int, end: int, min_bytes: int = 0x200,
            max_bytes: int = 0x4000) -> list[dict]:
    """One proposal per unit the region's evidence supports, in address order.

    The walk is over maximal *unclaimed* runs of functions inside `[start, end)`, not over seeds: a run
    is partitioned at its pinned seams, and a run with no evidence stays one unit (bounded by
    `--max-bytes`), because one function per file is certainly wrong while one file per region is only
    unproven. Every proposal says which of the two it is.
    """
    ordered, claimed = an["ordered"], claimed_text()
    runs, i = [], 0
    while i < len(ordered):
        addr = an["addr"][i]
        if addr < start or addr >= end or in_claimed(addr, claimed):
            i += 1
            continue
        j = i + 1
        while (j < len(ordered) and an["addr"][j] < end
               and not in_claimed(an["addr"][j], claimed)):
            j += 1
        runs.append((i, j))
        i = j

    out: list[dict] = []
    for lo, hi in runs:
        for lo_i, hi_i, why, note in segments(an, lo, hi, min_bytes, max_bytes):
            names = ordered[lo_i:hi_i]
            cxx = any(mangled(n) for n in names)
            first_addr = an["addr"][lo_i]
            data = td.data_runs(an, labels, lo_i, hi_i)
            data.update(td.extab_runs(an, labels, graph, lo_i, hi_i))
            out.append({
                "unit": placeholder(names[0], first_addr, cxx),
                "text": [first_addr, an["addr"][hi_i - 1] + an["size"][hi_i - 1]],
                "functions": [{"name": n, "address": an["addr"][lo_i + k],
                               "size": an["size"][lo_i + k]} for k, n in enumerate(names)],
                "count": len(names),
                "bytes": sum(an["size"][lo_i:hi_i]),
                "cxx": cxx,
                "seam": None if why is None else [{"kind": k, "why": w} for k, w in why],
                "seam_note": note,
                "runs": data,
            })
    return out


def human(proposals: list[dict]) -> None:
    for p in proposals:
        t0, t1 = p["text"]
        seam = "pinned: " + "; ".join(w for _, w in [(s["kind"], s["why"]) for s in p["seam"]]) \
            if p["seam"] else "no evidence - one run, seam is unproven"
        print("%-46s 0x%08X..0x%08X  %3d fn  %6d B  %s" %
              (p["unit"], t0, t1, p["count"], p["bytes"], "C++" if p["cxx"] else "C"))
        print("      %s" % seam[:150])
        if p["seam_note"]:
            print("      WARNING: %s" % p["seam_note"])
        for section, r in sorted(p["runs"].items()):
            tag = "NOT CLAIMED" if r["leak"] or r.get("density", 1.0) < 0.5 else "data proposal"
            print("      %-14s 0x%08X..0x%08X  %2d labels  %s" %
                  (section, r["start"], r["end"], r["labels"], tag))
    pinned = sum(1 for p in proposals if p["seam"])
    print("\n%d unit(s), %d functions, %d bytes; %d seam(s) pinned by evidence, %d unproven" %
          (len(proposals), sum(p["count"] for p in proposals),
           sum(p["bytes"] for p in proposals), pinned, len(proposals) - pinned))


STUB = '''/* %(unit)s - placeholder attribution, %(count)d function(s), 0x%(t0)08X..0x%(t1)08X.
 *
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout, and the bodies are still the original bytes (`Object(NonMatching, …)`, so the link keeps
 * them and `ninja build/RMHE08/ok` cannot move).
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
%(evidence)s *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit %(unit)s`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
'''


def stub_text(p: dict) -> str:
    ev = []
    if p["seam"]:
        for s in p["seam"]:
            ev.append(" *   pinned seam (%s): %s\n" % (s["kind"], s["why"]))
    else:
        ev.append(" *   the seam is **unproven**: the region offered no narrow, strong boundary evidence, so\n"
                  " *   this is one maximal unclaimed run (or a `--max-bytes` slice of one) and not a measured\n"
                  " *   translation unit. Re-check it with `tudiscover at <addr>` before writing source.\n")
    if p["seam_note"]:
        ev.append(" *   WARNING: %s\n" % p["seam_note"])
    return STUB % {"unit": p["unit"], "count": p["count"], "t0": p["text"][0],
                   "t1": p["text"][1], "evidence": "".join(ev)}


def apply(proposals: list[dict], dry_run: bool) -> None:
    if not proposals:
        print("nothing to apply")
        return
    splits = open(SPLITS, encoding="utf-8", newline="").read()
    conf = open(CONFIGURE, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in splits else "\n"
    nl_conf = "\r\n" if "\r\n" in conf else "\n"   # the two files do not share a line ending
    blocks = []
    for p in proposals:
        t0, t1 = p["text"]
        b = ["%s:" % p["unit"], "\t.text       start:0x%08X end:0x%08X" % (t0, t1)]
        for section in sorted(p["runs"], key=td.SECTION_ORDER.index):
            r = p["runs"][section]
            if r["leak"] or r.get("density", 1.0) < 0.5:
                b.append("\t# %-10s 0x%08X..0x%08X not claimed - leak %d, density %.2f"
                         % (section, r["start"], r["end"], r["leak"], r.get("density", 1.0)))
                continue
            b.append("\t# %-10s 0x%08X..0x%08X - claim in the measured data pass"
                     % (section, r["start"], r["end"]))
        blocks.append(nl.join(b) + nl)
    if dry_run:
        print("--- splits.txt would gain:\n" + "".join(blocks))
        print("--- configure.py would gain %d object line(s) in a new `auto` lib" % len(proposals))
        print("--- src/ would gain:\n" + "\n".join("    %s" % p["unit"] for p in proposals))
        return
    fresh = [b for b in blocks if b.split(":", 1)[0] not in splits]
    if len(fresh) != len(blocks):
        print("skipped %d split block(s) already in splits.txt" % (len(blocks) - len(fresh)))
    if fresh:
        if not splits.endswith(nl):
            splits += nl
        open(SPLITS, "w", encoding="utf-8", newline="").write(splits + nl + "".join(fresh))
    missing = [p["unit"] for p in proposals if '"%s"' % p["unit"] not in conf]
    if missing:
        objects = "".join('            Object(NonMatching, "%s"),\n' % u for u in missing)
        lib = ('    {\n        "lib": "auto",\n        "mw_version": "Wii/1.3",\n'
               '        "cflags": cflags_main,\n        "progress_category": "auto",\n'
               '        "objects": [\n%s        ],\n    },\n' % objects)
        marker = "config.libs = [" + nl_conf
        # configure.py is CRLF: match its line ending, and fail loudly when the anchor is not there -
        # a silently skipped registration is a unit that exists in splits.txt and nowhere else.
        assert marker in conf, "configure.py: `config.libs = [` not found (line endings?)"
        conf = conf.replace(marker, marker + nl_conf + lib.replace("\n", nl_conf), 1)
        # dtk refuses a progress_category it does not know (`Progress category 'auto' missing from
        # config.progress_categories`), so registering the first auto unit declares the category too.
        cat = "config.progress_categories = [" + nl_conf
        assert cat in conf, "configure.py: `config.progress_categories = [` not found"
        if 'ProgressCategory("auto"' not in conf:
            conf = conf.replace(cat, cat + nl_conf +
                                '    ProgressCategory("auto", "Auto (bulk attribution)"),', 1)
        open(CONFIGURE, "w", encoding="utf-8", newline="").write(conf)
    for p in proposals:
        path = ROOT / "src" / p["unit"]
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists():
            open(path, "w", encoding="utf-8", newline="").write(stub_text(p))
    print("wrote %d split block(s), %d configure object(s), %d stub source(s)"
          % (len(proposals), len(proposals), len(proposals)))
    print("next: python configure.py && ninja   (re-split), then read the ledger")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for cmd in ("plan", "apply"):
        a = sub.add_parser(cmd)
        a.add_argument("start")
        a.add_argument("end")
        a.add_argument("--min-bytes", type=lambda v: int(v, 0), default=0x200,
                       help="a piece smaller than this joins its neighbour (default 0x200)")
        a.add_argument("--max-bytes", type=lambda v: int(v, 0), default=0x4000,
                       help="a piece larger than this is split, flagged as a guess (default 0x4000)")
        a.add_argument("--json", action="store_true")
        a.add_argument("--dry-run", action="store_true")
        a.add_argument("--limit", type=int, default=0, help="apply at most N units")
    args = ap.parse_args()

    start, end = int(args.start, 0), int(args.end, 0)
    fns, labels, graph, an = load()
    props = propose(an, fns, labels, graph, start, end, args.min_bytes, args.max_bytes)
    if args.limit:
        props = props[:args.limit]
    if args.json:
        print(json.dumps(props, indent=2))
    elif args.cmd == "plan":
        human(props)
    if args.cmd == "apply":
        apply(props, args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
