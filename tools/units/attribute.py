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
    python tools/units/attribute.py queue 0x80040598 0x800408A8
    python tools/units/attribute.py queue 0x80003100 0x80600000 --max-total-bytes 0
    python tools/units/attribute.py queue 0x80040598 0x800408A8 --replace-region
    python tools/units/attribute.py --selftest

`plan` is read-only. `queue` caps and validates the batch, then writes the **proposal queue**
(`tools/units/attribution-queue.json`): the discovered units as *work to hand out*, not as registrations.
That is option A (owner, 2026-09-24) - the `src/auto/` scaffolding bucket is retired, and the worker that
takes a proposal registers the unit at its final `src/<module>/<name>.<ext>` home, from the evidence it has
by then. So neither `plan` nor `queue` touches `splits.txt`, `configure.py` or `src/`.

**`queue` rewrites the whole file, so a sub-region run is opt-in (2026-09-25).** The queue *is* one
region's tiling, not an append log: a run over `[start, end)` writes the proposals that region supports and
nothing else. That default is destructive - `queue 0x8008F8E4 0x80097D40` used to take a 419-entry queue
down to 1, and the file had to be restored by hand - so `queue` now refuses unless the run would *lose no
coverage*: every proposal already in the file must stay covered by the new proposals plus the ranges
`splits.txt` already owns. A re-cut of the same region (a better tiler, fresh evidence) loses nothing and
still passes; a narrower region, a `--limit` or a live `--max-total-bytes` cap that drops the tail is
refused, naming what it would discard and the `--replace-region` flag that overrides the refusal. The
guard reads the region, not the byte count: the destructive act is a range disappearing, and the error
names it.

`apply` is the **retired** behaviour - it wrote the `splits.txt` blocks, the `configure.py` objects and a
stub source per unit under `src/auto/`. It now refuses unless `--legacy-register` is passed, which exists
only to reproduce a batch landed before the policy changed.

The queue records the data runs a proposal saw (a `.text` line is safe - a `NonMatching` unit's bytes are
not linked, so the DOL hash cannot move - while a data line can make dtk drop the target's pool relocations
and *lower* a neighbouring unit's score, playbook idea 23). Claiming data stays a separate, measured pass.
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

**A proposal is one source file (2026-09-24).** The size tiling above is arbitrary with respect to
translation-unit boundaries, and both ways it can be wrong have cost a worker: one proposal spanning
*several* TUs (`proposal/80063888`, three files) and one TU split across two adjacent proposals
(`proposal/8007270C` and `proposal/80073180`, both `g3d_calcvtx.cpp`). The evidence is already in
`tudiscover`'s `__FILE__` anchors, so `propose` now uses it: an accepted source name's first referrer
is a seam whatever the soft vote's width (`interior_seams`), and the pieces one name *owns* are joined
back (`owner_merge`), because a candidate pool seam inside one file's span is not a boundary. Every
proposal also carries `tu_probe()`'s verdict - `one-tu`, `partial` (a range edge cuts a file),
`multi-tu` (a union) or `merged` (one file with a swallowed seam inside) - which `brief.py --pool`
turns into a plain warning at the top of the brief. The probe reads `an`, which `load()` already
builds, so it needs no cache of its own; a whole-queue plan pays ~10 s.

**A proposal's range is TU-bounded, and two proposals never overlap (2026-09-25).** A byte budget is not a
translation-unit boundary, and neither is a claim boundary: the `--max-total-bytes`/`--max-bytes` caps and
the `--min-bytes` merge are heuristics that fill the gaps *between* the evidence, so they may no longer
cross it. A byte cap or a min-bytes merge that would land inside an accepted `__FILE__` name slides to that
file's own edge instead (`segments`), a piece that starts at a name is never merged into its predecessor,
and a piece whose range the cap could not cut without splitting a file is emitted whole and flagged (its
`tu` verdict is `capped` when it has no TU evidence at all, which is the honest label for "this edge is a
size decision"). The region's own edges are part of the same rule: a function that straddles `end` is not
part of the region (it belongs to the next one), so two adjacent runs cannot each claim its bytes.
`overlap_report` is the invariant in one function - no proposal overlaps another proposal, and none
overlaps a range `splits.txt` already owns - and `propose` emits through it, dropping (and reporting) a
range that would break it rather than writing it. A stale queue entry that overlaps a unit another worker
has registered since is the same defect seen from the consumer's side; `brief.py` refuses to brief it.

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
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "splits"))
sys.path.insert(0, str(ROOT / "tools" / "units"))

import tudiscover as td  # noqa: E402  (path set above)
import dataorder as do  # noqa: E402  (the `.data` emission-order seams; docs/data-order-seams.md)
import sharedfiles as sf  # noqa: E402
import langcheck as lc  # noqa: E402
import poolseams as ps  # noqa: E402  (literal pools as TU evidence; docs/pool-seams.md)

SPLITS = ROOT / "config" / "RMHE08" / "splits.txt"
CONFIGURE = ROOT / "configure.py"
SYMBOLS = ROOT / "config" / "RMHE08" / "symbols.txt"
AUTO_DIR = ROOT / "src" / "auto"

# A proposal's *label* is provisional identity for the queue and the claim machinery, never a unit: under
# option A the worker that takes a proposal registers the unit at its final path. The label is deliberately
# not a `src/` path so it cannot be mistaken for one, and this is the only place that decides its shape.
# The extension it carries is a *hint* from the region's language verdict - the worker re-derives it from
# the unit's own evidence (its `__FILE__` string, a mangled definition) which outranks a region verdict.
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

# The proposal queue (option A). It lives beside the tool rather than under `.pi/` because it is campaign
# state every agent reads, not one session's scratch; it is gitignored because it is regenerated from the
# DOL and the map by `attribute.py queue`.
QUEUE_PATH = ROOT / "tools" / "units" / "attribution-queue.json"


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


def claimed_overlap(addr: int, size: int,
                    claimed: list[tuple[int, int, str]]) -> tuple[int, int, str] | None:
    """The claimed range a function's *own bytes* intersect, if any - a start test is not enough.

    A start test is not enough: a function whose bytes run into a claimed range (a boundary defect in
    `splits.txt`, or a region given with an `end` inside a function) would still be proposed, and the
    proposal would then overlap a translation unit that is already live. A proposal's range must never
    do that, so the partitioner takes the function's whole `[addr, addr + size)` span.
    """
    for s, e, u in claimed:
        if addr < e and s < addr + size:
            return (s, e, u)
    return None


def merge_intervals(ranges) -> list[tuple[int, int]]:
    """`ranges` as the minimal sorted set of disjoint half-open intervals covering the same addresses."""
    out: list[tuple[int, int]] = []
    for s, e in sorted(ranges):
        if s >= e:
            continue
        if out and s <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], e))
        else:
            out.append((s, e))
    return out


def interval_covered(merged: list[tuple[int, int]], start: int, end: int) -> bool:
    """Whether `[start, end)` sits inside one of `merged`'s disjoint intervals ("is already covered")."""
    return any(s <= start and end <= e for s, e in merged)


def overlap_report(proposals: list[dict], claimed=()) -> list[str]:
    """Every way a batch breaks the one invariant a proposal queue must hold - empty when it holds.

    Two proposals must never overlap: the tiler walks maximal *unclaimed* runs and cuts each one, so the
    ranges are disjoint by construction, and this is what keeps them that way when a region edge, a cap or
    a repair changes. No proposal may overlap a range `splits.txt` already owns either - a proposal there
    is stale (the range is live work under another name) and handing it out sets two workers on one range,
    the 8008F8E4 incident: that entry capped five TUs, four of them already claimed and live.
    """
    out: list[str] = []
    spans = sorted((tuple(p["text"]), p["unit"]) for p in proposals)
    for (s, e), unit in spans:
        for cs, ce, cu in claimed:
            if s < ce and cs < e:
                out.append("%s: 0x%08X..0x%08X overlaps %s's registered .text 0x%08X..0x%08X"
                           % (unit, s, e, cu, cs, ce))
    for i, ((s, e), unit) in enumerate(spans):
        for (s2, e2), unit2 in spans[:i]:
            if s < e2 and s2 < e:
                out.append("%s: 0x%08X..0x%08X overlaps %s's 0x%08X..0x%08X"
                           % (unit, s, e, unit2, s2, e2))
    return sorted(set(out))


def drop_overlaps(proposals: list[dict], claimed=()) -> tuple[list[dict], list[str]]:
    """`(kept, dropped)` - the lowest-address proposal of every overlap wins, the rest are not emitted.

    `propose` returns through this, so an overlapping range is *never* written to the queue: the earlier
    proposal wins (it is the one a worker is likelier to have taken already) and the loser is reported,
    because a silent drop is the destructive default this tool is being fixed for. Address order, so the
    result is deterministic.
    """
    kept: list[dict] = []
    dropped: list[str] = []
    for p in sorted(proposals, key=lambda p: tuple(p["text"])):
        why = overlap_report([p], claimed) + [
            "%s: 0x%08X..0x%08X overlaps %s earlier in the batch" % (p["unit"], p["text"][0],
                                                                    p["text"][1], q["unit"])
            for q in kept if p["text"][0] < q["text"][1] and q["text"][0] < p["text"][1]]
        if why:
            dropped.extend(why)
            continue
        kept.append(p)
    return kept, dropped


def placeholder(first: str, addr: int, cxx: bool) -> str:
    """`proposal/<addr>_<symbol>.<ext>` - unique, greppable, and obviously *not* a registered unit."""
    stem = re.sub(r"[^A-Za-z0-9_]", "_", first)[:NAME_MAX] or "unit"
    if stem[0].isdigit():
        stem = "u" + stem
    return "proposal/%08X_%s%s" % (addr, stem, ".cpp" if cxx else ".c")


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


def accepted_sources(an) -> list[dict]:
    """The `__FILE__` names `tudiscover` trusts as a TU identity, in address order.

    `tudiscover.analyse` anchors a source name only while its whole referrer span holds no function
    referencing a *different* name and stays within `--source-span-max`; a rejected name is a shared
    literal, not a boundary. The name is one TU's identity: its first referrer is the earliest cut its
    TU can start at, and the next name's first referrer is the latest its TU can end at.
    """
    return sorted((r for r in an.get("source_names", ()) if not r.get("reject")),
                  key=lambda r: r["lo"])


def is_source_seam(why) -> bool:
    """Whether a cut's own evidence is an accepted `__FILE__` start - a TU boundary, not a pool guess.

    The distinction the size repairs turn on: a piece starting at a name *is* that file's first piece
    (however small the file is), while a piece starting at a pool jump is a candidate cut the merge may
    take back.
    """
    return bool(why) and any(k == "source" for k, _ in why)


def source_owner(an, index: int) -> str | None:
    """The accepted source file owning function index `index` - the latest name starting at or before.

    None before the run's first name: an unowned stretch has no file identity of its own and keeps the
    pool-jump tiling. This is the ownership the `segments` repair uses to join the pieces one file was
    cut into; it is deliberately *not* a claim that every function after a name belongs to it.
    """
    owner = None
    for r in accepted_sources(an):
        if r["lo"] <= index:
            owner = r["src"]
        else:
            break
    return owner


def interior_seams(an, lo: int, hi: int) -> dict[int, list[tuple[str, str]]]:
    """Cut indices strictly inside `(lo, hi)` that pin a boundary, plus every accepted source start.

    `tudiscover.score_cuts` only scores the cuts *around* an existing closure, which is what extending a
    known unit needs. Partitioning a whole unclaimed region needs the opposite: the seams inside it. An
    observation is an interval of cuts it admits, so a seam is only worth taking when the interval is
    narrow (<= 4 cuts) and its kind is one of `tudiscover.STRONG` - a wide or weak observation says
    nothing about *where* the boundary is, only that somewhere nearby is possible. A cut inside a
    must-link anchor is illegal by definition.

    A source name's first referrer is added as a seam whatever the soft vote's width: the boundary
    between two files is real, and a wide vote (a gap between the two names' referrer runs) is exactly
    the case that left `proposal/80063888` a union of three files. Cutting at the *later* name's first
    referrer keeps both names' spans whole - a cut after the earlier name's last referrer.
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
    for r in accepted_sources(an):
        c = r["lo"]
        if lo < c < hi and not any(a < c <= b for a, b, _ in anchors):
            out.setdefault(c, []).append(("source", '"%s" starts here' % r["src"]))
    return out


def owner_merge(an, pieces: list[tuple], max_bytes: int) -> list[tuple]:
    """Join the consecutive pieces one accepted source file owns - a source file is one TU.

    `interior_seams` takes a pool-run jump inside a source name's ownership span as a seam, and that is
    how one file became two proposals (`proposal/8007270C` and `proposal/80073180`, both
    `g3d_calcvtx.cpp`, set two workers on one TU). A candidate seam inside one file's span is not a
    boundary, so the pieces join, and `tu_probe` re-reads the seams left inside to warn the worker the
    range may still be two TUs. A join that would exceed `--max-bytes` is left alone - the cap is a
    registration ceiling and the pieces stay flagged as a guess.

    `pieces` is `[(lo_i, hi_i, why, note)]`, the shape `segments` builds.
    """
    out: list[tuple] = []
    for lo_i, hi_i, why, note in pieces:
        if out:
            plo, _phi, _pwhy, _pnote = out[-1]
            owner = source_owner(an, lo_i)
            size = an["addr"][hi_i - 1] + an["size"][hi_i - 1] - an["addr"][plo]
            if owner is not None and owner == source_owner(an, plo) and size <= max_bytes:
                out[-1] = (plo, hi_i, None,
                           "one source file (%s): a candidate seam inside it was not taken" % owner)
                continue
        out.append((lo_i, hi_i, why, note))
    return out


def segments(an, lo: int, hi: int, min_bytes: int, max_bytes: int):
    """Split `[lo, hi)` at the pinned seams, then repair the pieces that are not TU-shaped.

    Three rules, all about what a translation unit is: a piece below `min_bytes` is too small to be a
    file of its own (merge it into its neighbour), a piece above `max_bytes` is too large (split it at
    its cheapest seam and mark the result as a guess), and the pieces one accepted `__FILE__` name owns
    are one file (join them, because a candidate seam inside a file is not a boundary). Everything a
    region offers as evidence is used first; the repairs only apply where there is none.

    The two size repairs are bounded by the evidence, because **a byte budget is not a TU boundary**
    (2026-09-25): a piece that starts at an accepted name is never merged leftward (the name is the TU,
    however small the file turns out to be), and neither the cap nor the merge may cut inside one. When
    the cap can end a piece at the edge of a file at or below its byte position, that *is* the cut; when
    the file itself is over the cap the cut slides out of it and the piece is emitted whole, flagged.
    `tu_probe` re-reads the seams inside each piece and gives the size-only edges their own verdict, so
    what the evidence could not decide never reads as if it had.

    Returns `[(lo_i, hi_i, why, note)]`.
    """
    seams = interior_seams(an, lo, hi)
    cuts = sorted(seams) + [hi]
    parts, prev = [], lo
    for c in cuts:
        if c > prev:
            parts.append((prev, c, seams.get(c) if c != hi else None))
            prev = c
    # merge from the left: a too-small part joins the part before it (or the one after, if it is first) -
    # unless the cut it would lose is an accepted source start, which is a TU boundary and not a pool
    # guess. `why` is the seam at a part's *end*, so the cut a merge removes is the previous part's.
    merged: list[list] = []
    for k, (lo_i, hi_i, why) in enumerate(parts):
        size = an["addr"][hi_i - 1] + an["size"][hi_i - 1] - an["addr"][lo_i]
        if merged and size < min_bytes and not is_source_seam(parts[k - 1][2] if k else None):
            merged[-1][1] = hi_i
            merged[-1][2] = None          # the seam it was cut at is gone, so the union has no pin
        else:
            merged.append([lo_i, hi_i, why])
    # split from the left: a part over the cap is cut at the cap, flagged as a guess - but never inside a
    # must-link anchor or an accepted `__FILE__` name, which are the two cuts the evidence forbids outright
    srcs = [(r["lo"], r["hi"], r["src"]) for r in accepted_sources(an)]

    def split_source(c: int) -> str | None:
        """The accepted `__FILE__` name a cut at `c` would split in two, if any."""
        for r_lo, r_hi, src in srcs:
            if r_lo < c <= r_hi:
                return src
        return None

    def legal(c: int) -> bool:
        return not any(a < c <= b for a, b, _ in an["must_link"]) and split_source(c) is None

    def end_of_source(lo_i: int, at: int) -> tuple[int, str] | None:
        """The last cut at or before `at` that ends an accepted `__FILE__` name, with its name.

        The edge the cap should end a piece on: a byte budget is not a TU boundary, so when a file ends
        at or below the cap the piece ends where the file does (and is *smaller* than the cap).
        """
        best = [r_hi + 1 for r_lo, r_hi, _src in srcs if lo_i < r_hi + 1 <= at and legal(r_hi + 1)]
        if not best:
            return None
        cut = max(best)
        return cut, next(src for _lo, r_hi, src in srcs if r_hi + 1 == cut)

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
            edge = end_of_source(lo_i, at)
            if edge is not None:
                # a file ends at or below the cap: end the piece where the file does, not at a byte
                at, src = edge
                out.append((lo_i, at, why, "ends at the edge of source file %s, not at the byte cap" % src))
                lo_i, why = at, None
                continue
            while at < hi_i and not legal(at):
                at += 1                      # slide out of an anchor or a file, even past the cap
            if at >= hi_i:
                src = split_source(hi_i - 1)
                note = "over --max-bytes with no legal cut - kept whole" if src is None else \
                    "over --max-bytes inside one source file (%s) - the file is the TU, kept whole" % src
                out.append((lo_i, hi_i, why, note))
                break
            src = split_source(best or lo_i + 1)
            note = "capped at --max-bytes, seam is a guess" if at == best else \
                "over --max-bytes: the cut is the edge of %s, not a byte position" % \
                ("source file %s" % src if src else "a must-link anchor")
            out.append((lo_i, at, why, note))
            lo_i, why = at, None
        else:
            out.append((lo_i, hi_i, why, None))
    return owner_merge(an, out, max_bytes)


def data_seam_records(syms) -> list[dict]:
    """`dataorder.seams` with each seam's two vtable owners attached (`before_owner`/`after_owner`, or None).

    `dataorder` classifies and finds the seams; this only adds what a `.text` cut needs, the address of each
    vtable's first code slot. The classification is never redone here. A `V->S` row is a gap `[addr, latest)`
    (the boundary lies after its inline-tail strings), so its `after_owner` is the vtable that ends the gap, at
    `latest`; a `V->tail` row (a vtable, strings, no later vtable) is no evidence and is dropped.
    """
    by_addr = {s.addr: i for i, s in enumerate(syms)}
    out = []
    for sm in do.seams(syms):
        if sm["kind"] == "V->tail":
            continue
        j = by_addr.get(sm["addr"])
        if j is None:
            continue
        before = next((syms[k] for k in range(j - 1, -1, -1)
                       if syms[k].name == sm["before"] and syms[k].kind == do.VTABLE), None)
        after = syms[by_addr[sm["latest"]]] if sm.get("latest") in by_addr else syms[j]
        out.append(dict(sm, before_owner=before.owner if before else None,
                        after_owner=after.owner if after.kind == do.VTABLE else None))
    return out


#: A `.data` run is `min..max` over the labels the range's functions reference, so a range that touches two
#: far-apart shared globals gets a run spanning dozens of other TUs. Only a *dense* run (most labels inside it
#: are the range's own - `tudiscover.data_runs`' `density`) is contiguous enough for a seam inside it to mean
#: the range's data is several TUs.
DENSE_RUN_MIN = 0.5


def data_seams_for(text, runs, records) -> dict | None:
    """The `.data` seams that bear on one proposal, or None when there are none.

    A seam is **interior** when it splits the proposal's own `.data` run (`start < addr < end`) or, for a
    zigzag, when both vtable owners are inside `text` - then the range holds data of two TUs. A `V->S` seam is a
    gap `[addr, latest)`: it says a boundary lies somewhere in it (after the inline-tail strings), so it is at
    least one more TU and `min_tus` stays a valid lower bound, but the *first string* is not the cut. A run counts only
    when it is dense (`DENSE_RUN_MIN`); a sparse run is not evidence of anything. A seam with one
    owner inside `text` and nothing else is an **edge** (it names a neighbour's boundary; informative, not
    counted). `min_tus` is `1 +` the interior strong seams (`V->S`, zigzag); an interior `V->D` is weak
    (a jump table is `.data` too) and is only counted in `weak`. `cut` is a candidate `.text` address read
    off the vtable owners - never applied: a zigzag and a `V->S` gap put it between the two owners (when both are
    known and ordered), a `V->D` after the owner of the vtable before the seam. `V->tail` rows are no evidence
    and are never counted.
    """
    t0, t1 = text
    d = (runs or {}).get(".data") or {}
    lo, hi = d.get("start"), d.get("end")

    def inside(a):
        return a is not None and t0 <= a < t1

    seen, out = set(), []
    for r in records or ():
        if r["addr"] in seen:
            continue
        bo, ao = r.get("before_owner"), r.get("after_owner")
        in_run = lo is not None and d.get("density", 1.0) >= DENSE_RUN_MIN and lo < r["addr"] < hi
        both = r["kind"] == "zigzag" and inside(bo) and inside(ao)
        if not (in_run or inside(bo) or inside(ao)):
            continue
        if r["kind"] == "V->tail":
            continue
        seen.add(r["addr"])
        item = {"addr": r["addr"], "kind": r["kind"], "before": r["before"], "after": r["after"],
                "interior": bool(in_run or both)}
        if r.get("latest") is not None:
            item.update(latest=r["latest"], width=r.get("width"), tail=r.get("tail", 0))
        if r["kind"] in ("zigzag", "V->S"):
            if inside(bo) and inside(ao) and bo < ao:
                item["cut"] = {"between": [bo, ao]}
            elif r["kind"] == "V->S" and inside(bo):
                item["cut"] = {"after": bo}
        elif inside(bo):
            item["cut"] = {"after": bo}
        out.append(item)
    if not out:
        return None
    strong = sum(1 for i in out if i["interior"] and i["kind"] != "V->D")
    return {"seams": out, "min_tus": 1 + strong,
            "weak": sum(1 for i in out if i["interior"] and i["kind"] == "V->D")}


def load_data_seam_records() -> list[dict]:
    """The DOL's `.data` seams with owners (reads `orig/` and the map; writes nothing)."""
    rows = do.load_symbols()
    return data_seam_records(do.classify_all(rows, td.Dol(do.DOL)))


def cmd_dataseams(args) -> int:
    """Read-only: attach `data_seams` to the queue file's entries in memory and report; the file is not written."""
    doc = json.loads(Path(args.queue).read_text(encoding="utf-8"))
    records = load_data_seam_records()
    hits = []
    for u in doc.get("units", []):
        ds = data_seams_for(u["text"], u.get("runs"), records)
        if ds:
            hits.append((u["label"], ds))
    if args.json:
        print(json.dumps({lbl: ds for lbl, ds in hits}, indent=1))
        return 0
    print("%d of %d queue entries have a data seam bearing on them" % (len(hits), len(doc.get("units", []))))
    for lbl, ds in hits:
        shown = ["0x%08X %s%s%s" % (i["addr"], i["kind"],
                                    " gap->0x%08X" % i["latest"] if i.get("latest") else "",
                                    "" if i["interior"] else " (edge)")
                 for i in ds["seams"]]
        print("  %-46s min %d TUs, %d weak: %s%s" % (lbl, ds["min_tus"], ds["weak"], ", ".join(shown[:4]),
                                                  " (+%d)" % (len(shown) - 4) if len(shown) > 4 else ""))
    return 0


def tu_probe(an, lo_i: int, hi_i: int, note: str | None = None) -> dict:
    """What `tudiscover`'s own evidence says about this range - one TU, part of one, or several.

    The queue's cut is a heuristic, so a proposal is checked against the evidence that does name a TU:
    an accepted `__FILE__` source name. A name wholly inside the range is one TU; a name the range cuts
    is a *partial* TU (the range's edge is inside a file); two names are a *union* of TUs; a candidate
    seam left inside the range is `merged` - the range may still be two TUs and no source name says so
    (this is the g3d_calcvtx case: a pool run jump inside one file's span). No name at all is
    `unproven`, which is the honest label for a region with no TU evidence - and when the range's edge is
    the `--max-bytes` cap rather than any evidence (`note`, from `segments`) it is `capped`, because a
    byte budget is not a boundary either.
    """
    t0 = an["addr"][lo_i]
    t1 = an["addr"][hi_i - 1] + an["size"][hi_i - 1]
    inside, partial = [], None
    for r in accepted_sources(an):
        if r["start"] >= t0 and r["end"] <= t1:
            inside.append(r["src"])
        elif (r["start"] < t0 < r["end"]) or (r["start"] < t1 < r["end"]):
            partial = r["src"]
    # A source *change* seam is not "open": it is the boundary the queue already cut at. Any other
    # candidate seam left strictly inside the piece is one the queue swallowed (a min-bytes merge, a
    # pool run jump inside a source file's span), and it is what makes the range possibly two TUs.
    open_seams = [{"cut": c, "why": why[0][1]}
                  for c, why in sorted(interior_seams(an, lo_i, hi_i).items())
                  if not any(k == "source" for k, _ in why)]
    if partial:
        verdict = "partial"
    elif len(inside) > 1:
        verdict = "multi-tu"
    elif open_seams and inside:
        # only a *named* file is worth warning about: an unowned range is already "unproven", and a
        # candidate seam there is the normal tiling, not a file that may have been split in two
        verdict = "merged"
    elif inside:
        verdict = "one-tu"
    elif note and "--max-bytes" in note:
        # the edge is a byte budget, not evidence: the region has no `__FILE__` name to bound it, so the
        # honest label is the cap itself rather than a size masquerading as a translated file
        verdict = "capped"
    else:
        verdict = "unproven"
    return {"sources": inside, "partial_source": partial, "open_seams": open_seams,
            "verdict": verdict}


def pool_seams_of(names, graph, labels, pool_touch) -> dict | None:
    """The registered units that already touch a pooled literal the functions `names` read, or None.

    MWCC emits one literal pool per TU, so a literal a registered unit reads or claims puts this range in the same
    original TU as that unit (`poolseams.pool_seams_for`); `pool_touch` is `poolseams.literal_units`' shape
    (None/empty = no evidence, nothing is attached).
    """
    if not pool_touch:
        return None
    lits = [labels[r]["addr"] for n in names for r in graph["funcs"].get(n, {}).get("refs", ())
            if r in labels and td.is_pool_literal(labels[r])]
    return ps.pool_seams_for(lits, pool_touch)


def propose(an, fns, labels, graph, start: int, end: int, min_bytes: int = MIN_BYTES_DEFAULT,
            max_bytes: int = MAX_BYTES_DEFAULT, claimed=None, seam_records=None, pool_touch=None) -> list[dict]:
    """One proposal per unit the region's evidence supports, in address order.

    The walk is over maximal *unclaimed* runs of functions inside `[start, end)`, not over seeds: a run
    is partitioned at its pinned seams, and a run with no evidence stays one unit (bounded by
    `--max-bytes`), because one function per file is certainly wrong while one file per region is only
    unproven. Every proposal says which of the two it is.

    The region is half-open **on function boundaries**: a function whose last byte is past `end` is not
    part of this region (it belongs to the next one), so two adjacent runs can never both claim its bytes -
    the same rule the run walk applies to the ranges `splits.txt` already owns, taking a function's whole
    span rather than its start. Two proposals that overlap, or that overlap a registered unit, are not
    emitted at all (`drop_overlaps` reports them); with today's evidence the tiling is disjoint by
    construction, and this is the invariant that keeps it so. `claimed` is `claimed_text()`'s shape,
    injectable so the selftest can partition without the repo's `splits.txt`. `seam_records` is
    `data_seam_records`' shape (None = no data-order evidence): a proposal that a `.data` seam bears on gets a
    `data_seams` entry (`data_seams_for`).
    """
    ordered = an["ordered"]
    claimed = claimed_text() if claimed is None else claimed
    runs, i = [], 0
    while i < len(ordered):
        addr, size = an["addr"][i], an["size"][i]
        if addr < start or addr + size > end or claimed_overlap(addr, size, claimed):
            i += 1
            continue
        j = i + 1
        while (j < len(ordered) and an["addr"][j] + an["size"][j] <= end
               and not claimed_overlap(an["addr"][j], an["size"][j], claimed)):
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
            entry = {
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
                "tu": tu_probe(an, lo_i, hi_i, note),
                "runs": data,
            }
            ds = data_seams_for(entry["text"], data, seam_records)
            if ds:
                entry["data_seams"] = ds
            pool = pool_seams_of(names, graph, labels, pool_touch)
            if pool:
                entry["pool_seams"] = pool
            out.append(entry)
    kept, dropped = drop_overlaps(out, claimed)
    for why in dropped:
        print("dropped: %s" % why, file=sys.stderr)
    return kept


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


def input_fingerprints(layout: Layout | None = None) -> dict:
    """The inputs the partition is a function of, hashed - provenance that is *deterministic*.

    A wall-clock timestamp made every regeneration a diff and hid the case that matters: **the same inputs
    must produce the same queue.** These hashes are stable for the same inputs, so `attribute.py queue` is
    reproducible - regenerating over an unchanged tree writes a byte-identical file and `git status` stays
    clean - while a rename (which changes `symbols.txt`) or a re-split (which changes `splits.txt`) shows up
    as a real change, with the input that moved named in the file itself.

    The DOL is the ground truth the seams are read from; `symbols.txt` decides the labels' stems and the
    function names; `splits.txt` decides which ranges are already claimed, i.e. excluded.
    """
    layout = layout or LAYOUT
    out = {}
    for key, path in (("dol", Path(td.DOL)), ("symbols", SYMBOLS), ("splits", layout.splits),
                      ("configure", layout.configure)):
        try:
            out[key + "_sha1"] = hashlib.sha1(path.read_bytes()).hexdigest()
        except OSError:
            out[key + "_sha1"] = None
    return out


def queue_doc(proposals: list[dict], cap: int, fingerprints: dict | None = None) -> dict:
    """The proposal queue as a plain dict - the shape `brief.py` and `queue.py` read.

    `label` is a *provisional identity* for the claim/queue machinery only (`claims.py` keys on a string,
    and a proposal has no unit yet). It is never a registered unit and never a `configure.py` path.

    Deliberately carries no timestamp: see `input_fingerprints`.
    """
    return {
        "version": 1,
        "cap": cap,
        **(fingerprints if fingerprints is not None else input_fingerprints()),
        "total_bytes": sum(p["bytes"] for p in proposals),
        "units": [{
            "label": p["unit"],
            "text": p["text"],
            "count": p["count"],
            "bytes": p["bytes"],
            "cxx": p["cxx"],
            "language": p.get("language"),
            "seam": p.get("seam"),
            "seam_note": p.get("seam_note"),
            "tu": p.get("tu"),
            "functions": p["functions"],
            "runs": p["runs"],
            **({"data_seams": p["data_seams"]} if p.get("data_seams") else {}),
            **({"pool_seams": p["pool_seams"]} if p.get("pool_seams") else {}),
        } for p in proposals],
    }


def queue_guard(path: Path | None, fresh: list[dict], claimed, replace_region: bool = False,
                ) -> str | None:
    """Why this `queue` run would lose proposals - `None` when it may write (the destructive default).

    `queue` writes the *whole* file for the region it was given, so a run over a sub-region silently
    discards every other proposal: `queue 0x8008F8E4 0x80097D40` took a 419-entry queue down to 1, and the
    file had to be restored by hand. The refusal is a *coverage* test rather than an equality test, because
    a legitimate re-run does re-cut: the region's proposals change whenever the tiler or the evidence does
    (`429 -> 230` proposals was one such re-cut), and what must never happen is a range the file proposed
    being left with no proposal and no registration to hold it. Every existing entry must stay covered by
    the fresh proposals plus the ranges `splits.txt` already owns; when one does not, the refusal names it.
    `--replace-region` overrides - the flag is the opt-in the destructive default was missing.
    """
    path = path or QUEUE_PATH
    if replace_region or not path.exists():
        return None
    try:
        doc = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None                       # an unreadable queue is not something to refuse *over*
    old = [p for p in (doc.get("units") or []) if isinstance(p.get("text"), list)
           and len(p["text"]) == 2]
    if not old:
        return None
    keep = merge_intervals([tuple(p["text"]) for p in fresh]
                           + [(s, e) for s, e, _u in claimed])

    def lost(p: dict) -> bool:
        """Whether this entry held a *function* the rewrite leaves unowned.

        Function starts, not the entry's whole range: the bytes between two functions belong to nobody
        (padding inside `.text`), so a re-cut that lands there drops no work - and the range of an entry
        written before `functions` was recorded falls back to the whole range.
        """
        fns = [f for f in (p.get("functions") or []) if isinstance(f, dict) and "address" in f]
        if fns:
            return not all(any(s <= f["address"] < e for s, e in keep) for f in fns)
        return not interval_covered(keep, p["text"][0], p["text"][1])

    dropped = [p for p in old if lost(p)]
    if not dropped:
        return None
    fns = [f for p in dropped for f in (p.get("functions") or [])
           if isinstance(f, dict) and "address" in f]
    t0 = min([f["address"] for f in fns] + [p["text"][0] for p in dropped])
    t1 = max([f["address"] + f.get("size", 0) for f in fns] + [p["text"][1] for p in dropped])
    first = dropped[0]
    return ("refusing to rewrite %s: the run would drop %d of its %d proposal(s) - `queue` writes the *whole* queue\n"
            "  for the region it is given, and these are covered by neither the new proposals nor a\n"
            "  registered unit (0x%08X..0x%08X). First dropped: %s (0x%08X..0x%08X).\n"
            "  Regenerate a region that covers the queue (a re-cut of the same region is allowed), or pass\n"
            "  --replace-region to replace the whole file anyway."
            % (path, len(dropped), len(old), t0, t1, first.get("label", "?"),
               first["text"][0], first["text"][1]))


def write_queue(proposals: list[dict], cap: int, path: Path | None = None,
                dry_run: bool = False) -> int:
    """Write the proposal queue - discovered units as work, never as registrations (option A).

    Touches none of the four shared files. The write is temp + replace, the same discipline as
    `sharedfiles.Transaction`, so a reader never sees a half-written queue. **Deterministic**: the same
    inputs produce the same bytes, so a re-run over an unchanged tree is not a diff.
    """
    path = path or QUEUE_PATH
    doc = queue_doc(proposals, cap)
    text = json.dumps(doc, indent=1) + "\n"
    units = len(proposals)
    funcs = sum(p["count"] for p in proposals)
    byts = sum(p["bytes"] for p in proposals)
    if dry_run:
        print("--- would write %s - %d unit(s), %d function(s), %d .text bytes"
              % (path, units, funcs, byts))
        return 0
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + sf.TMP_SUFFIX)
    tmp.write_text(text, encoding="utf-8")
    tmp.replace(path)
    print("wrote %s" % path)
    print("  %d proposed unit(s), %d function(s), %d .text bytes (cap 0x%X)" % (units, funcs, byts, cap))
    print("  inputs: dol %s  symbols %s" % ((doc.get("dol_sha1") or "?")[:12],
                                              (doc.get("symbols_sha1") or "?")[:12]))
    print("  a proposal is not a unit: the worker that takes one registers it at its final")
    print("  src/<module>/<name>.<ext> home, from the evidence it has by then.")
    return 0


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
    parsers = {}
    for cmd in ("plan", "apply", "queue"):
        a = sub.add_parser(cmd)
        parsers[cmd] = a
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
    ds = sub.add_parser("dataseams", help="read-only: which queue entries a `.data` seam bears on")
    ds.add_argument("--queue", default=str(QUEUE_PATH))
    ds.add_argument("--json", action="store_true")
    parsers["apply"].add_argument(
        "--legacy-register", action="store_true",
        help="the retired behaviour: write src/auto stubs plus the configure.py and splits.txt entries. "
             "Option A forbids it; it exists only to reproduce a batch landed before 2026-09-24.")
    parsers["queue"].add_argument(
        "--replace-region", action="store_true",
        help="rewrite the whole queue even when the region does not cover the proposals already in it. "
             "`queue` writes the whole file, so a run that would drop a range is refused without this "
             "flag (the destructive default is opt-in).")
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

    if args.cmd == "dataseams":
        return cmd_dataseams(args)
    start, end = int(args.start, 0), int(args.end, 0)
    fns, labels, graph, an = load()
    straddling = [(an["addr"][i], an["ordered"][i]) for i in range(len(an["ordered"]))
                  if start <= an["addr"][i] < end and an["addr"][i] + an["size"][i] > end]
    if straddling:
        # the region is half-open on function boundaries: say so instead of dropping a function silently
        print("note: %d function(s) start inside the region but end past 0x%08X (%s at 0x%08X) - the\n"
              "      region is half-open on function boundaries, so they belong to the next one; pass an\n"
              "      `end` on a function edge to include them"
              % (len(straddling), end, straddling[0][1], straddling[0][0]), file=sys.stderr)
    props = propose(an, fns, labels, graph, start, end, args.min_bytes, args.max_bytes,
                    seam_records=load_data_seam_records(), pool_touch=ps.literal_units(str(ROOT)))
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
    if args.cmd == "queue":
        reason = queue_guard(QUEUE_PATH, kept, claimed_text(), args.replace_region)
        if reason:
            print(reason, file=sys.stderr)
            print("nothing written", file=sys.stderr)
            return 1
        cap_report(kept, detail, args.max_total_bytes)
        return write_queue(kept, args.max_total_bytes, dry_run=args.dry_run)
    if not args.legacy_register:
        print("refusing: option A retired registration - `apply` no longer writes src/auto stubs,")
        print("`configure.py` entries or `splits.txt` blocks (owner, 2026-09-24).")
        print("Use `attribute.py queue <start> <end>` to record the proposals; the worker that takes")
        print("one registers the unit at its final home. `--legacy-register` reproduces an old batch.")
        return 2
    return apply(props, args.dry_run, args.max_total_bytes, fns=fns)


if __name__ == "__main__":
    sys.exit(main())
