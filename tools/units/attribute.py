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
    python tools/units/attribute.py apply 0x80040598 0x800408A8 --max-total-bytes 0x80000
    python tools/units/attribute.py --selftest

`plan` is read-only. `apply` writes the `splits.txt` blocks, the `configure.py` objects and a stub
source per unit; nothing is measured by it - run `ninja` (which re-splits) and read the ledger after.
The `configure.py` objects extend the existing `auto` lib when one is already declared, and only the
first batch appends the block - a second `"lib": "auto"` block would be a different lib with the same
name.

**The registration cap (docs/plan.md §3 clause 2, roadmap 7.14).** `--max-total-bytes` (default
0x80000 = 0.5 MB, the plan's registration-batch ceiling; `0` disables) keeps the longest address-ordered
prefix of the proposals whose `.text` totals no more than the cap, and names the first candidate it
refused with the exact overflow. It counts `.text` bytes because that is all `apply` claims - the data
runs are recorded as comments and cannot move the DOL. A batch that lands exactly on the cap passes.
With `--json`, `plan` prints the capped batch (the proposals that would land) and the refusal goes to
stderr, so a consumer parsing the JSON still sees a consistent list.

**The size defaults (roadmap 7.13).** `--min-bytes` and `--max-bytes` are derived from the 15 units
already registered, not guessed. Their `.text` spans run 16 B .. 27436 B, but the sub-100 B entries
(`OSAlarm`, `NetworkWiiMediator`, `lobby_scene`, `g3d_resanmamblight`, `global_destructor_chain`) are
fragments and `__init_cpp_exceptions` (112 B) is a stub, so the smallest *unit-sized* one is
`sys_mem.cpp` at **288 B** and the largest is `Pl/pl_act.cpp` at **27436 B (~27 KB)**. Hence
`MIN_BYTES_DEFAULT = 288` (below it, a piece is not a file of its own) and
`MAX_BYTES_DEFAULT = 27436` (above it, a piece is too big and is split, flagged as a guess).

**The language (docs/plan.md, "The language comes from the symbol").** The extension a stub is named
with is not cosmetic: `dtk` turns it into the front-end flag (`-lang=c` / `-lang=c++`), so a new unit's
extension decides how it is compiled from its first build. `region_language()` reads the verdict from
`tools/units/langcheck.py` using the evidence an *unclaimed* region already exposes - the map's mangled
names, the graph's mangled callees (the disassembly's relocation names), and the `__FILE__` names whose
referring functions sit inside the region - and, if a region still holds two distinct `__FILE__` names,
says so, because that is a boundary defect rather than a language one.

**Transactional apply (roadmap 7.20).** `apply` validates the whole batch before writing anything (no
proposal overlaps another or a range `splits.txt` already owns, no unit twice, every function
resolvable in the map at the address and size claimed, both `configure.py` anchors present), then
hands the whole batch to `tools/units/sharedfiles.py`, which preserves each file's line ending, asserts
every anchor, and writes through a `*.sharedfiles-tmp` temp file and `os.replace` - restoring the exact
previous bytes of every file it already replaced if any write fails. `--dry-run` touches nothing.
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
import sharedfiles as sf  # noqa: E402
import langcheck as lc  # noqa: E402

SPLITS = ROOT / "config" / "RMHE08" / "splits.txt"
CONFIGURE = ROOT / "configure.py"
AUTO_DIR = ROOT / "src" / "auto"

# A unit's name is a placeholder until someone has evidence for the real path. The header comment of
# each stub says so; this is the only place that decides what a placeholder looks like.
NAME_MAX = 48

# docs/plan.md §3 clause 2: one registration batch claims at most 0.5 MB of `.text` (a blast-radius
# ceiling, not a target). `0` disables the cap.
CAP_DEFAULT = 0x80000

# The two size defaults, derived from the registered units (roadmap 7.13): 288 B is `sys_mem.cpp`, the
# smallest unit-sized `.text`; 27436 B is `Pl/pl_act.cpp`, the largest (~27 KB). See the docstring.
MIN_BYTES_DEFAULT = 288
MAX_BYTES_DEFAULT = 27436

# The two anchors `configure.py` must have or the registration cannot land. The selftest uses a fixture
# layout with these instead of the real files.
CONFIG_LIBS_ANCHOR = "config.libs = ["
CONFIG_CAT_ANCHOR = "config.progress_categories = ["

# The one lib `apply` registers into. `configure_insertion` extends this lib's object list when the
# block is already in `configure.py`, so a later batch does not add a second block with the same name
# (a duplicate lib silently changes the flags every earlier unit was built with).
AUTO_LIB = "auto"


# The write layer lives in `sharedfiles.py` (roadmap 7.12); re-exported so existing callers and the
# selftest keep their names.
TMP_SUFFIX = sf.TMP_SUFFIX
Layout = sf.Layout
Transaction = sf.Transaction
read_text = sf.read_text
parse_ranges = sf.parse_ranges
overlaps = sf.overlaps

LAYOUT = Layout(SPLITS, CONFIGURE, ROOT / "src")


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
    return lc.mangled(name)


def region_language(an, graph, names, t0: int, t1: int) -> dict:
    """The language verdict for an *unclaimed* region - before any target object exists.

    The extension `placeholder()` picks is not cosmetic: dtk derives `-lang=c`/`-lang=c++` from it, so a
    wrong one is a wrong compiler front-end from the unit's first build (owner's rule, `docs/plan.md`
    "The language comes from the symbol"; `tools/units/langcheck.py` is the one implementation). With
    no object to read, the same evidence comes from what the region already exposes: the map's mangled
    names, the graph's mangled *callees* (the disassembly's relocation names - exactly the names the
    target object would carry as undefined), and the `__FILE__` names whose referring functions sit
    inside `[t0, t1)`.
    """
    callees = set()
    for n in names:
        rec = (graph or {}).get("funcs", {}).get(n)
        if rec:
            callees.update(c for c in rec.get("calls", ()) if lc.mangled(c))
    sources = [r["src"] for r in (an or {}).get("source_names", ())
               if r["start"] >= t0 and r["end"] <= t1]
    return lc.classify([n for n in names if lc.mangled(n)], sorted(callees), sources)


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


def propose(an, fns, labels, graph, start: int, end: int, min_bytes: int = MIN_BYTES_DEFAULT,
            max_bytes: int = MAX_BYTES_DEFAULT) -> list[dict]:
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
            first_addr = an["addr"][lo_i]
            end_addr = an["addr"][hi_i - 1] + an["size"][hi_i - 1]
            language = region_language(an, graph, names, first_addr, end_addr)
            cxx = language["lang"] == "c++"
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
                "language": language,
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
        lang = p.get("language") or {}
        tag = "C++" if p["cxx"] else "C"
        if lang.get("lang"):
            tag += " (%s: %s)" % (lang.get("confidence"), lc.evidence_text(lang))
        print("%-46s 0x%08X..0x%08X  %3d fn  %6d B  %s" %
              (p["unit"], t0, t1, p["count"], p["bytes"], tag))
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
 * Language: %(language)s
 * (`tools/units/langcheck.py`; dtk turns the extension into `-lang`, so this decides the front-end).
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
    lang = p.get("language") or {}
    if lang.get("lang"):
        language = "%s (%s: %s)" % ("C++" if lang["lang"] == "c++" else "C", lang.get("confidence"),
                                     lc.evidence_text(lang))
        if lang.get("conflict"):
            language += " - two source files, so the boundary needs a re-check"
    elif p["cxx"]:
        language = "C++ (a mangled name is in the region)"
    else:
        language = "C (no evidence; the default)"
    return STUB % {"unit": p["unit"], "count": p["count"], "t0": p["text"][0],
                   "t1": p["text"][1], "evidence": "".join(ev), "language": language}


# --------------------------------------------------------------------------------------------------
# the cap (docs/plan.md 7.14)
# --------------------------------------------------------------------------------------------------
def cap_batch(proposals: list[dict], max_total_bytes: int) -> tuple[list[dict], list[dict], dict | None]:
    """The longest address-ordered prefix of `proposals` that fits `max_total_bytes`.

    Returns `(kept, refused, detail)`. `refused` is the tail that did not fit, and `detail` names the
    first candidate the cap refused, its size and the exact overflow - the number an operator needs to
    choose between a narrower range and a smaller `--limit`. A candidate that lands exactly on the cap
    is kept (`>` rather than `>=`), and `0` means no cap at all.
    """
    if not max_total_bytes:
        return list(proposals), [], None
    kept: list[dict] = []
    total = 0
    for i, p in enumerate(proposals):
        if total + p["bytes"] > max_total_bytes:
            return kept, proposals[i:], {
                "unit": p["unit"], "text": p["text"], "bytes": p["bytes"], "claimed": total,
                "cap": max_total_bytes, "overflow": total + p["bytes"] - max_total_bytes,
                "remaining": len(proposals) - i,
            }
        kept.append(p)
        total += p["bytes"]
    return kept, [], None


def cap_report(kept: list[dict], detail: dict | None, cap: int, file=None) -> None:
    """How many bytes the batch is about to claim, and exactly what the cap refused."""
    if cap:
        total = sum(p["bytes"] for p in kept)
        print("cap: --max-total-bytes 0x%X (%d B); this batch claims %d B of .text in %d unit(s), %d B left"
              % (cap, cap, total, len(kept), cap - total), file=file)
    if detail:
        t0, t1 = detail["text"]
        print("refused: %s (0x%08X..0x%08X, %d B) does not fit: %d + %d = %d B, %d B over the cap"
              % (detail["unit"], t0, t1, detail["bytes"], detail["claimed"], detail["bytes"],
                 detail["claimed"] + detail["bytes"], detail["overflow"]), file=file)
        print("         %d candidate(s) left unregistered; re-run from 0x%08X or raise --max-total-bytes"
              % (detail["remaining"], t0), file=file)


# --------------------------------------------------------------------------------------------------
# validation and apply (docs/plan.md 7.20; the writes themselves are in sharedfiles.py)
# --------------------------------------------------------------------------------------------------
def validate(proposals: list[dict], layout: Layout, fns: dict | None = None) -> list[str]:
    """Everything that must hold before a byte is written. An empty list means the batch may land.

    The write phase is the only thing that can fail afterwards, and every way it can fail has been
    turned into an error here: an anchor that is not in `configure.py`, a section name `SECTION_ORDER`
    does not know, a proposal that would double-claim a range. `fns` is `tudiscover.load_map()`'s
    function table - with it, every function a proposal names must resolve at the address and size the
    proposal claims, which is the "every referenced symbol resolvable" half of 7.20.
    """
    errs: list[str] = []
    gone = [str(p) for p in (layout.splits, layout.configure) if not p.exists()]
    if gone:
        return ["%s: does not exist" % g for g in gone]
    splits = sf.read_text(layout.splits)
    conf = sf.read_text(layout.configure)
    nl_conf = sf.line_ending(conf)
    have = sf.parse_ranges(splits)

    seen: dict[str, tuple[int, int]] = {}
    for p in proposals:
        u = p["unit"]
        if u in seen:
            errs.append("%s: the batch names this unit twice" % u)
        parts = Path(u).parts
        if Path(u).is_absolute() or ".." in parts:
            errs.append("%s: the unit path is absolute or escapes src/ - refused" % u)
        t0, t1 = p["text"]
        if t0 >= t1:
            errs.append("%s: empty or inverted .text range 0x%08X..0x%08X" % (u, t0, t1))
            continue
        if not p["functions"]:
            errs.append("%s: no functions in the proposal" % u)
        else:
            lo = min(f["address"] for f in p["functions"])
            hi = max(f["address"] + f["size"] for f in p["functions"])
            if (t0, t1) != (lo, hi):
                errs.append("%s: .text 0x%08X..0x%08X is not the span of its functions (0x%08X..0x%08X)"
                            % (u, t0, t1, lo, hi))
            total = sum(f["size"] for f in p["functions"])
            if total != p["bytes"]:
                errs.append("%s: byte total %d is not the sum of its functions (%d)" % (u, p["bytes"], total))
            if fns is not None:
                for f in p["functions"]:
                    rec = fns.get(f["name"])
                    if rec is None:
                        errs.append("%s: %s is not a .text function in the symbol map" % (u, f["name"]))
                    elif (rec["addr"], rec["size"]) != (f["address"], f["size"]):
                        errs.append("%s: %s is 0x%08X+%d in the map, not 0x%08X+%d"
                                    % (u, f["name"], rec["addr"], rec["size"], f["address"], f["size"]))
        for section in p["runs"]:
            if section not in td.SECTION_ORDER:
                errs.append("%s: unknown section %r - not in tudiscover.SECTION_ORDER" % (u, section))
        hit = sf.find_overlap(have, t0, t1, unit=u)
        if hit is not None:
            other, section, s, e = hit
            errs.append("%s: .text 0x%08X..0x%08X overlaps %s's %s range 0x%08X..0x%08X"
                        % (u, t0, t1, other, section, s, e))
        for other, (s, e) in seen.items():
            if sf.overlaps((t0, t1), (s, e)):
                errs.append("%s: .text 0x%08X..0x%08X overlaps %s earlier in the batch" % (u, t0, t1, other))
        own = [(s, e) for o, sec, s, e in have if o == u and sec == ".text"]
        if own and (t0, t1) not in own:
            errs.append("%s: already in splits.txt at 0x%08X..0x%08X, the batch says 0x%08X..0x%08X"
                        % (u, own[0][0], own[0][1], t0, t1))
        seen[u] = (t0, t1)

    missing = [p for p in proposals if '"%s"' % p["unit"] not in conf]
    if missing:
        if lib_objects_anchor(conf, AUTO_LIB) is None:
            if lib_present(conf, AUTO_LIB):
                errs.append("configure.py: the `\"lib\": \"%s\"` block has no multi-line "
                            "`\"objects\": [` list to extend - refused rather than append a second "
                            "lib block" % AUTO_LIB)
            elif CONFIG_LIBS_ANCHOR + nl_conf not in conf:
                errs.append("configure.py: the `%s` anchor is missing (line endings?) - the object line "
                            "would not land" % CONFIG_LIBS_ANCHOR)
        if 'ProgressCategory("auto"' not in conf and CONFIG_CAT_ANCHOR + nl_conf not in conf:
            errs.append("configure.py: the `%s` anchor is missing - the auto progress category "
                        "cannot be declared" % CONFIG_CAT_ANCHOR)
    return errs


def split_block(p: dict, nl: str) -> str:
    """The `splits.txt` block for one proposal: `.text` claimed, the data runs recorded as comments."""
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
    return nl.join(b) + nl


def lib_present(conf: str, lib_name: str) -> bool:
    """Whether `configure.py` already declares a lib with this name."""
    return '"lib": "%s",' % lib_name in conf


def lib_objects_anchor(conf: str, lib_name: str) -> str | None:
    """The lib block's own header up to and including the last line before its `"objects": [` closes.

    The anchor has to be the lib's own header, not the bare `"objects": [` line: several libs have an
    objects list and only this one may be extended. It runs to the last line *before* the list's
    closing `],` so an insertion lands after the objects already there (append) rather than before them
    (prepend), which is both the natural order and keeps the anchor stable across batches. Returns None
    when the lib is absent, or when its objects list is not the multi-line form the tool writes (an
    inline `"objects": [],` or a `"objects": objects,` variable) - extending those would be a rewrite
    of the line, not an insertion, so `apply` refuses instead of guessing.
    """
    lines = conf.splitlines()
    want = '"lib": "%s",' % lib_name
    for i, line in enumerate(lines):
        if line.strip() != want:
            continue
        for j in range(i + 1, len(lines)):
            s = lines[j].strip()
            if s == '"objects": [':
                k = j + 1
                while k < len(lines) and lines[k].strip() != "],":
                    k += 1
                last = k - 1 if k > j else j
                return sf.line_ending(conf).join(lines[i:last + 1])
            if s.startswith('"lib":'):
                break
        return None
    return None


def insert_lines_after(conf: str, anchor: str, insertion: str) -> str:
    """Insert `insertion` directly under the line `anchor` (no blank separator), asserting the anchor.

    `sharedfiles.insert_after_anchor` puts a blank line between the anchor and the insertion, which is
    right for a block-level edit (`config.libs = [` ...) but wrong inside an object list: the new
    `Object(...)` lines belong directly under the objects already there, and a blank line would
    accumulate once per registration batch. The anchor assertion is the same - a missing anchor raises
    instead of silently skipping the edit - and the insertion takes configure.py's own line ending.
    """
    nl = sf.line_ending(conf)
    marker = anchor + nl
    if marker not in conf:
        raise sf.AnchorError("%r not found (line ending %r?)" % (anchor, nl))
    return conf.replace(marker, marker + sf.with_ending(insertion, nl), 1)


def configure_insertion(conf: str, units: list[str]) -> str:
    """`conf` with `units` registered in the `auto` lib, the progress category declared if it is new.

    The lib is **extended** when `configure.py` already declares it and appended only when it does not.
    The units already present are dropped first, so the same batch applied twice is a no-op - an
    `Object(...)` line is never duplicated and a second `"lib": "auto"` block is never created. Every
    anchor is asserted (never silently skipped) and the insertion takes configure.py's own CRLF ending.
    """
    units = [u for u in units if '"%s"' % u not in conf]
    if not units:
        return conf
    objects = "".join('            Object(NonMatching, "%s"),\n' % u for u in units)
    anchor = lib_objects_anchor(conf, AUTO_LIB)
    if anchor is not None:
        conf = insert_lines_after(conf, anchor, objects)
    else:
        if lib_present(conf, AUTO_LIB):
            raise sf.AnchorError('the `"lib": "%s"` block has no multi-line `"objects": [` list to '
                                 "extend" % AUTO_LIB)
        lib = ('    {\n        "lib": "%s",\n        "mw_version": "Wii/1.3",\n'
               '        "cflags": cflags_main,\n        "progress_category": "auto",\n'
               '        "objects": [\n%s        ],\n    },\n' % (AUTO_LIB, objects))
        conf, _ = sf.insert_after_anchor(conf, CONFIG_LIBS_ANCHOR, lib)
    # dtk refuses a progress_category it does not know (`Progress category 'auto' missing from
    # config.progress_categories`), so registering the first auto unit declares the category too.
    conf, _ = sf.insert_after_anchor(conf, CONFIG_CAT_ANCHOR,
                                     '    ProgressCategory("auto", "Auto (bulk attribution)"),\n',
                                     present='ProgressCategory("auto"')
    return conf


def plan_writes(proposals: list[dict], layout: Layout) -> dict:
    """The complete new content of every file `apply` would touch, computed before anything is written.

    Nothing here touches the disk. Building the full text up front is what makes the write phase pure
    I/O - so a failure can only be a failed rename, and the rollback is a list of renames rather than
    a guess about what the file looked like.
    """
    splits = sf.read_text(layout.splits)
    conf = sf.read_text(layout.configure)
    nl = sf.line_ending(splits)
    writes: list[tuple[Path, str]] = []

    blocks = [(p["unit"], split_block(p, nl)) for p in proposals]
    new_splits, added = sf.append_blocks(splits, blocks)
    if added:
        writes.append((layout.splits, new_splits))
    missing = [p["unit"] for p in proposals if '"%s"' % p["unit"] not in conf]
    if missing:
        writes.append((layout.configure, configure_insertion(conf, missing)))
    stubs = 0
    for p in proposals:
        path = layout.src / p["unit"]
        if not path.exists():
            writes.append((path, stub_text(p)))
            stubs += 1
    return {"writes": writes, "splits": added, "objects": len(missing), "stubs": stubs}


def apply(proposals: list[dict], dry_run: bool = False, cap: int = CAP_DEFAULT,
          layout: Layout | None = None, fns: dict | None = None, rename=None) -> int:
    """Register a batch: cap it, validate it, then write every file through temp + rename.

    Nothing is written until the whole batch has been validated and its new content built, so the only
    way to fail once the write phase starts is a failed rename - and that restores every file it had
    already replaced. `cap` defaults to the plan's 0.5 MB registration ceiling.
    """
    layout = layout or LAYOUT
    kept, refused, detail = cap_batch(proposals, cap)
    if not kept:
        if detail:
            cap_report(kept, detail, cap)
            print("nothing written")
            return 1
        print("nothing to apply")
        return 0
    try:
        errs = validate(kept, layout, fns)
        if errs:
            print("refusing to write (%d problem(s)) - nothing was touched:" % len(errs))
            for e in errs:
                print("  " + e)
            return 1
        plan = plan_writes(kept, layout)
    except OSError as exc:
        print("cannot read %s (%s) - nothing was touched" % (exc.filename, exc))
        return 1
    except sf.AnchorError as exc:
        print("refusing to write: %s - nothing was touched" % exc)
        return 1
    cap_report(kept, detail, cap)
    if len(kept) > plan["splits"]:
        print("skipped %d split block(s) already in splits.txt" % (len(kept) - plan["splits"]))
    if dry_run:
        print("--- would write %d file(s):" % len(plan["writes"]))
        for path, text in plan["writes"]:
            print("    %-52s %d B" % (path.relative_to(ROOT) if path.is_relative_to(ROOT) else path,
                                       len(text.encode("utf-8"))))
        print("--- dry run: nothing written")
        return 0
    tx = sf.Transaction(rename=rename)
    try:
        for path, text in plan["writes"]:
            tx.write(path, text)
    except OSError as exc:
        tx.rollback()
        print("write failed (%s) - rolled back %d file(s), nothing was registered" % (exc, len(tx.order)))
        return 1
    finally:
        tx.cleanup()
    print("wrote %d split block(s), %d configure object(s), %d stub source(s)"
          % (plan["splits"], plan["objects"], plan["stubs"]))
    print("next: python configure.py && ninja   (re-split), then read the ledger")
    return 0


def build_parser() -> argparse.ArgumentParser:
    """The CLI, in one place so the selftest can check the flags without loading the DOL."""
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    sub = ap.add_subparsers(dest="cmd")
    for cmd in ("plan", "apply"):
        a = sub.add_parser(cmd)
        a.add_argument("start")
        a.add_argument("end")
        a.add_argument("--min-bytes", type=lambda v: int(v, 0), default=MIN_BYTES_DEFAULT,
                       help="a piece smaller than this joins its neighbour (default %d = sys_mem.cpp)"
                            % MIN_BYTES_DEFAULT)
        a.add_argument("--max-bytes", type=lambda v: int(v, 0), default=MAX_BYTES_DEFAULT,
                       help="a piece larger than this is split, flagged as a guess (default %d = "
                            "Pl/pl_act.cpp)" % MAX_BYTES_DEFAULT)
        a.add_argument("--max-total-bytes", type=lambda v: int(v, 0), default=CAP_DEFAULT,
                       help="register at most this many .text bytes in one batch (default 0x80000, the "
                            "plan's 0.5 MB registration cap; 0 disables)")
        a.add_argument("--json", action="store_true")
        a.add_argument("--dry-run", action="store_true")
        a.add_argument("--limit", type=int, default=0, help="apply at most N units")
    return ap


def main() -> int:
    ap = build_parser()
    args = ap.parse_args()

    if args.selftest:
        import attribute_selftest                 # imported late: it imports this module
        return attribute_selftest.selftest()
    if not args.cmd:
        ap.print_help()
        return 2

    start, end = int(args.start, 0), int(args.end, 0)
    fns, labels, graph, an = load()
    props = propose(an, fns, labels, graph, start, end, args.min_bytes, args.max_bytes)
    if args.limit:
        props = props[:args.limit]
    kept, refused, detail = cap_batch(props, args.max_total_bytes)
    if args.json:
        # the batch that would land, i.e. the capped prefix - a refusal is not part of the batch
        print(json.dumps(kept, indent=2))
    if args.cmd == "plan":
        if not args.json:
            human(kept)
        cap_report(kept, detail, args.max_total_bytes, file=sys.stderr if args.json else sys.stdout)
        return 0
    return apply(props, args.dry_run, args.max_total_bytes, fns=fns)


if __name__ == "__main__":
    sys.exit(main())
