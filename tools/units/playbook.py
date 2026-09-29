#!/usr/bin/env python3
"""Turn worker outboxes into ready-to-land playbook drafts (docs/matching/NNN-slug.md idea stubs).

The round's problem: many workers independently found the same levers - `#pragma peephole off` (five or
more units), `#pragma fp_contract off` (four or more), and the trap that `#pragma optimization_level 1`
does *not* turn the peephole off (two) - and two filed a `config_requests` entry asking for a row. Nothing
wrote those rows automatically: they sat in `.pi/outbox/*.json` until the orchestrator got to them, and a
lever rediscovered by five workers is a lever that was not written down in time.

This tool closes that loop. It reads every outbox, classifies each finding against a small registry of
*levers* and *traps*, groups duplicates (five identical findings become one row), and writes a draft
`docs/matching.md` section in the house style (Problem / Why try it / Result / Example) plus the matching
index row (informational: `tools/agents/sync_playbook_index.py` generates the real one), numbered from the current highest. The drafts go to `.pi/playbook-drafts/` - this tool
**never edits `docs/**` or `CLAUDE.md`**; the orchestrator lands the drafts in one commit.

    python tools/units/playbook.py                 # scan .pi/outbox, write .pi/playbook-drafts/
    python tools/units/playbook.py --print         # the same report, without writing
    python tools/units/playbook.py --outbox DIR --notes DIR --drafts DIR
    python tools/units/playbook.py --selftest

Where a finding can come from (all optional, the schemas drift between rounds):

* `playbook_candidate` (object: title/problem/result) and `suggested_playbook_row` (string) - a worker's
  explicit request;
* `config_requests` naming `docs/matching.md`/`CLAUDE.md` (kind `shared-file`) or a `flag` change whose
  evidence matches a registered lever;
* `flags_probed` / `flag_probes` - an `adopt` verdict is a lever, a `reject`/`inconclusive` verdict can be a
  registered trap;
* `.pi/notes/*.md` for the traps a worker records in prose rather than in the outbox.

A finding is only drafted when it carries evidence: a before/after number for a lever, or a measured probe
for a trap. Everything else is reported under "not drafted" with the reason - a finding with no numbers is
not a row.

The registry (`LEVERS`) is the one place the curated framing lives; extending it is how a new lever becomes
a row. It is deliberately data, so the next round's workers can be pointed at it.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import re
import sys
from dataclasses import dataclass, field

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "agents"))
import split_playbook  # noqa: E402  (derive_tags: the idea-tag keyword heuristic)

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUTBOX = os.path.join(MAIN, ".pi", "outbox")
NOTES = os.path.join(MAIN, ".pi", "notes")
DRAFTS = os.path.join(MAIN, ".pi", "playbook-drafts")
MATCHING = os.path.join(MAIN, "docs", "matching")          # the directory of NNN-slug.md idea files
AGENTS = os.path.join(MAIN, "docs", "matching", "index.md")  # the generated index

# -----------------------------------------------------------------------------------------------------------
# The registry: one entry per known lever/trap. `match` patterns are searched case-insensitively in the
# finding's text; a finding is assigned to the *earliest* match (so a combined probe lands on its dominant
# lever). `scan_notes` lets a trap be picked up from a `.pi/notes/*.md` line when no outbox records it.
# -----------------------------------------------------------------------------------------------------------
LEVERS = (
    {
        "key": "peephole-off",
        "kind": "lever",
        "title": "A unit whose retail code keeps unfused peephole folds needs the peephole pass off",
        "index": ("Retail keeps the unfused form of a fold our `-O3` peephole makes - a masked "
                  "`clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, an `li r0` + `psq_lx` "
                  "epilogue; `#pragma peephole off` (or `-opt nopeephole`) restores it."),
        "problem": ("The unit's retail object keeps instructions our peephole pass folds away: a masked "
                    "`clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, a record-form `clrlwi.` "
                    "the target does not have, or an `li r0,<slot>` + `psq_lx`/`psq_stx` epilogue. Our build, "
                    "with the command line's peephole on, emits the fused form and lands one or two "
                    "instructions short - a 4-25 % gap that reads as a source problem."),
        "why": ("`#pragma peephole off` is the source spelling of `-opt nopeephole`; it turns the pass off for "
                "the file or a scoped region and restores the target's unfused form. Several units in the "
                "`auto` bucket were built with it off, so it is the first lever to try when the diff is a "
                "*fold*, not a shape. Note the level is not the lever: `#pragma optimization_level 1` leaves "
                "the command line's peephole on (see the trap below)."),
        "example": "```c\n#pragma peephole off   /* the whole unit, or a scoped pair around one function */\n```",
        "match": (r"pragma\s+peephole\s+off", r"-opt\s+nopeephole", r"peephole\s+pass\s+off"),
    },
    {
        "key": "fp-contract-off",
        "kind": "lever",
        "title": "A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off`",
        "index": ("Our default `-fp_contract on` fuses `a*b + c` into one `fmadds`/`fmsubs` where retail "
                  "keeps `fmuls` + `fadds`; `#pragma fp_contract off` restores the two instructions."),
        "problem": ("Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default "
                    "`-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few "
                    "instructions short and every later register shifts. It reads as a source-shape problem "
                    "and sends you rewriting expressions that were already right."),
        "why": ("`#pragma fp_contract off` (or `-fp_contract off`) turns the contraction off, the two "
                "instructions come back, and the expression can stay natural. The recurring shapes are "
                "`2.0f*x - 1.0f` and `1.0f + rate*t`."),
        "example": "```c\n#pragma fp_contract off\n```",
        "match": (r"pragma\s+fp_contract\s+off", r"-fp_contract\s+off"),
    },
    {
        "key": "opt-level-not-peephole",
        "kind": "trap",
        "title": "`#pragma optimization_level 1` does not turn the peephole off",
        "index": ("A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to "
                  "get it; the pragma only sets the level, so the command line's peephole still folds. The "
                  "lever is `#pragma peephole off`."),
        "problem": ("A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way "
                    "to get it: `-O1` resolves to `-opt level=1`, and the level's switch set reads as if it "
                    "includes the peephole. It compiles, the level moves, and the narrowing still folds."),
        "why": ("The peephole is a separate switch on the command line and the pragma only sets the level, so "
                "the command line's `peephole on` survives. Several workers measured the same non-result "
                "independently. On the command line `-O1` produces the same object as `-O3` + the pragma here, "
                "so the level is not a proxy for the pass either."),
        "example": "```c\n#pragma peephole off        /* not `#pragma optimization_level 1` */\n```",
        "match": (r"optimization_level\s+1\b", r"-opt\s+level=1\b"),
    },
    {
        "key": "extern-c-mangling",
        "kind": "lever",
        "title": "A C++ free function needs `extern \"C\"` so objdiff can pair it by name",
        "index": ("A `fn_*` defined in a `.cpp` measures 0 % with byte-perfect code: MWCC mangles the name and "
                  "objdiff pairs by symbol name; `extern \"C\"` makes the emitted name the map's name."),
        "problem": ("A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC "
                    "mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by "
                    "name, so neither side pairs and the function contributes nothing."),
        "why": ("Wrap the `fn_*` definitions in `extern \"C\"`: the emitted symbol becomes the plain map name "
                "and every symbol pairs. This is the source half of playbook 31 (the map rename is the other "
                "half)."),
        "example": "```cpp\nextern \"C\" void fn_80073398(ResHandle* self) { ... }\n```",
        "match": (r"extern\s*\"c\"",),
    },
    {
        "key": "pool-off",
        "kind": "lever",
        "title": "Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off`",
        "index": ("Our string literals are addressed through one `@stringBase0` base register where retail "
                  "materialises each string with its own `lis`/`addi`; `-pool off` stops the pooling."),
        "problem": ("Our string literals are addressed through one `@stringBase0` base register (one `lis`, "
                    "then `addi` displacements) where retail materialises each string with its own "
                    "`lis`/`addi` - 0x20 bytes of `.text` short, and the `.rela.text` records name a base "
                    "symbol retail never had."),
        "why": ("`-pool off` stops the string pooling; the relocations become per-string and `.data` "
                "reproduces retail's pool. It has no source pragma, so it is a lib/per-unit flag request."),
        "example": "```\n-pool off\n```",
        "match": (r"-pool\s+off\b",),
    },
    {
        "key": "str-reuse-pool",
        "kind": "lever",
        "title": "A string pool in `.data` means the build was not `-str readonly`",
        "index": ("Retail's string pool in `.data` (not `.rodata`) says the build did not use "
                  "`-str readonly`; it is a placement diagnostic even when the score does not move on its own."),
        "problem": ("The unit's retail string pool sits in `.data`, but our `cflags` carry "
                    "`-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not "
                    "pair. It reads as a missing data range."),
        "why": ("The section is chosen by the string flag: `-str reuse,pool` without `readonly` emits the pool "
                "into `.data`. It is a diagnostic first - the RSO unit's literal reconstruction scored lower "
                "than the `extern` form until the pool was written the way the flag needs - but it tells you "
                "which `-str` the original build used."),
        "example": "```\n-str reuse,pool\n```",
        "match": (r"-str\s+reuse[^\n]*pool",),
    },
    {
        "key": "crlf-literals",
        "kind": "trap",
        "title": "A hand-written string literal's `\\n` becomes CRLF on this host",
        "index": ("MWCC on this host translates the `\\n` in a literal to CRLF, so a string pool written "
                  "in-source does not match the DOL's LF bytes; leave the range to the data pass."),
        "problem": ("A data range's string pool cannot be written in-source because MWCC on this host "
                    "translates the `\\n` in the literals to CRLF, so the emitted bytes do not match the "
                    "DOL's LF. It reads as a data-claim problem and invites a hand-written definition that "
                    "will not match."),
        "why": ("Record it before spending a pass on the pool: the bytes come from the compiler's literal "
                "path, which is host-newline sensitive. Leave the range to the data pass, or reference the "
                "strings as `extern` declarations so the object does not emit them."),
        "example": None,
        "match": (r"crlf",),
        "match_all": (r"mwcc", r"crlf", r"literal|\\n"),
        "scan_notes": True,
        "scan_text": True,
    },
    {
        "key": "ctors-fragment-flip",
        "kind": "trap",
        "title": "A flipped unit's `.ctors$10` fragment is reordered by the linker",
        "index": ("The object is byte-identical and `flipcheck.py` says READY, and the flip still loses the "
                  "unit's `.ctors$10`/`.dtors$15` words; re-split first, then suspect the linker's fixed "
                  "ctor/dtor name order."),
        "problem": ("A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks "
                    "the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` "
                    "tables. It reads as a linker/ordering mystery, and `flipcheck.py` cannot see it."),
        "why": ("Check the target object's freshness first: the blocker that motivated the 7.19 audit was a "
                "*stale target object, cured by a re-split*. If a fresh re-split does not cure it, the "
                "linker's built-in `.ctors`/`.dtors` path is the remaining suspect: it collects `$NN` "
                "fragments in a fixed name order (`.ctors$00, .ctors$10, .ctors, .ctors$99`), not link "
                "order, so compare the merged `.ctors`/`.dtors` words, not only the object's section sizes."),
        "example": None,
        "match": (r"\.ctors", r"\.dtors"),
        "match_all": (r"\.ctors|\.dtors", r"flip",
                      r"flip blocker|do not land|stale target|\.ctors.{0,4}blocker|\.dtors.{0,4}blocker"),
        "scan_notes": True,
    },
)


# -----------------------------------------------------------------------------------------------------------
# Finding / group model
# -----------------------------------------------------------------------------------------------------------
@dataclass
class Finding:
    key: str                    # registry key, or a slug for an explicit/unclassified finding
    kind: str                   # lever / trap / explicit
    source: str                 # the outbox or note the finding came from
    unit: str
    worker: str
    text: str                   # the evidence text the registry matched
    title: str = ""              # an explicit candidate's own title, when it has one
    evidence: list[str] = field(default_factory=list)
    numbers: bool = False


@dataclass
class Group:
    key: str
    title: str
    kind: str
    problem: str
    why: str
    example: str | None
    index: str
    findings: list[Finding] = field(default_factory=list)

    def units(self) -> list[str]:
        seen, out = set(), []
        for f in self.findings:
            u = f.unit or f.source
            if u not in seen:
                seen.add(u)
                out.append(u)
        return out


# -----------------------------------------------------------------------------------------------------------
# Small helpers
# -----------------------------------------------------------------------------------------------------------
def _read(path: str) -> str:
    with open(path, encoding="utf-8") as fh:
        return fh.read()


STOP = set("a an the is what with of to and in on for it its by as that this at from into but not no when "
           "where which be are was were has have had do does did can could would should".split())


def tokens(s: str) -> set[str]:
    words = re.findall(r"[a-z0-9]+", (s or "").lower().replace("`", ""))
    return {w for w in words if w not in STOP and len(w) > 1}


def jaccard(a: set[str], b: set[str]) -> float:
    if not a or not b:
        return 0.0
    return len(a & b) / len(a | b)


def slug(s: str) -> str:
    s = re.sub(r"[^a-z0-9]+", "-", (s or "").lower()).strip("-")
    return re.sub(r"-+", "-", s)[:60] or "finding"


NUM_RE = re.compile(r"(\d+(?:\.\d+)?)\s*%?\s*(?:->|\u2192|to)\s*(\d+(?:\.\d+)?)\s*%?")
PCT_RE = re.compile(r"\d+\.\d+\s*%")
SIZE_RE = re.compile(r"0x[0-9A-Fa-f]{3,}")


def has_numbers(s: str) -> bool:
    return bool(NUM_RE.search(s or "") or PCT_RE.search(s or "") or SIZE_RE.search(s or ""))


def registry_match(text: str, allowed: tuple[str, ...] = ("lever", "trap")) -> dict | None:
    """The registry entry whose earliest `match` pattern hits `text`, or None."""
    best, best_pos = None, None
    for entry in LEVERS:
        if entry["kind"] not in allowed:
            continue
        if entry.get("match_all") and not all(re.search(p, text, re.I) for p in entry["match_all"]):
            continue
        pos = None
        for pat in entry["match"]:
            m = re.search(pat, text, re.I)
            if m and (pos is None or m.start() < pos):
                pos = m.start()
        if pos is not None and (best_pos is None or pos < best_pos):
            best, best_pos = entry, pos
    return best


def existing_rows(index_text: str, matching_dir: str) -> list[tuple[int, str, str]]:
    """[(number, title, source)] from the generated docs/matching/index.md table and the front matter of the
    docs/matching/NNN-slug.md idea files (`matching_dir` is that directory; a missing one adds nothing)."""
    rows: list[tuple[int, str, str]] = []
    for m in re.finditer(r"^\|\s*(\d+)\s*\|\s*(?:\[([^\]]+)\]\([^)]*\)|([^|]+?))\s*\|", index_text, re.M):
        rows.append((int(m.group(1)), (m.group(2) or m.group(3)).strip(), "docs/matching/index.md"))
    if os.path.isdir(matching_dir):
        for name in sorted(os.listdir(matching_dir)):
            if not re.match(r"^\d{3}-.*\.md$", name):
                continue
            head = _read(os.path.join(matching_dir, name)).split("\n---", 1)[0]
            n = re.search(r"^id:\s*(\d+)\s*$", head, re.M)
            t = re.search(r"^title:\s*(.+?)\s*$", head, re.M)
            if n and t:
                rows.append((int(n.group(1)), t.group(1), "docs/matching/" + name))
    return rows


def next_number(rows: list[tuple[int, str, str]]) -> int:
    return (max((n for n, _, _ in rows), default=0)) + 1


def landed_in(title: str, rows: list[tuple[int, str, str]]) -> int | None:
    """The existing row number this title already is, or None."""
    t = tokens(title)
    for num, heading, _ in rows:
        h = tokens(heading)
        if not t or not h:
            continue
        if jaccard(t, h) >= 0.5:
            return num
        # a long shared phrase is enough even when the sentence frames it differently
        if len(t & h) >= 5 and len(t & h) / min(len(t), len(h)) >= 0.6:
            return num
    return None


# -----------------------------------------------------------------------------------------------------------
# Extraction
# -----------------------------------------------------------------------------------------------------------
def _effect_of(probe: dict) -> str:
    return str(probe.get("effect") or probe.get("result") or probe.get("what") or "")


def _flags_of(probe: dict) -> str:
    return str(probe.get("flags") or probe.get("probe") or probe.get("change") or "")


def extract_findings(entries: list[tuple[str, dict]]) -> tuple[list[Finding], list[dict]]:
    """-> (findings, undraftable) from every outbox entry."""
    findings: list[Finding] = []
    undraftable: list[dict] = []

    for path, d in entries:
        unit = str(d.get("unit") or "")
        worker = str(d.get("worker") or "")
        src = os.path.basename(path)

        # 0. traps recorded in prose (a residual/blocker), for registry entries that opt in
        prose = " ".join(str(d.get(k) or "") for k in ("residual", "blockers"))
        if prose.strip():
            for entry in LEVERS:
                if entry["kind"] != "trap" or not entry.get("scan_text"):
                    continue
                if entry.get("match_all") and not all(re.search(p, prose, re.I) for p in entry["match_all"]):
                    continue
                m = next((re.search(p, prose, re.I) for p in entry["match"] if re.search(p, prose, re.I)), None)
                if m:
                    snippet = prose[max(0, m.start() - 80):m.start() + 240].strip()
                    findings.append(Finding(key=entry["key"], kind="trap", source=src, unit=unit, worker=worker,
                                            text=snippet, evidence=[snippet], numbers=has_numbers(snippet)))
                    break

        # 1. explicit top-level requests ---------------------------------------------------------------
        cand = d.get("playbook_candidate")
        if isinstance(cand, dict):
            title = str(cand.get("title") or "")
            body = " ".join(str(cand.get(k) or "") for k in ("problem", "result", "measured_by"))
            findings.append(Finding(key=slug(title), kind="explicit", source=src, unit=unit, worker=worker,
                                    text=title + " " + body, title=title,
                                    evidence=[str(cand.get("result") or "")], numbers=has_numbers(body)))
        row = d.get("suggested_playbook_row")
        if isinstance(row, str) and row.strip():
            title = row.split(" - ")[0].strip().rstrip(".")
            findings.append(Finding(key=slug(title), kind="explicit", source=src, unit=unit, worker=worker,
                                    text=row, title=title, evidence=[row], numbers=has_numbers(row)))

        # 2. config_requests ---------------------------------------------------------------------------
        for req in d.get("config_requests") or []:
            if not isinstance(req, dict):
                continue
            kind = req.get("kind")
            if kind == "shared-file":
                blob = " ".join(str(req.get(k) or "") for k in ("file", "why"))
                if re.search(r"matching\.md|(?:CLAUDE|AGENTS)\.md", blob, re.I):
                    why = str(req.get("why") or "")
                    entry = registry_match(why, ("lever", "trap"))
                    if entry:
                        findings.append(Finding(key=entry["key"], kind=entry["kind"], source=src, unit=unit,
                                                worker=worker, text=why, evidence=[why],
                                                numbers=has_numbers(why)))
                    else:
                        findings.append(Finding(key="explicit-" + slug(why), kind="explicit", source=src,
                                                unit=unit, worker=worker, text=why, evidence=[why],
                                                numbers=has_numbers(why)))
                    continue
            if kind == "flag":
                change = str(req.get("change") or req.get("what") or "")
                evidence = str(req.get("evidence") or req.get("why") or "")
                text = change + " " + evidence
                entry = registry_match(text, ("lever",))
                if entry:
                    findings.append(Finding(key=entry["key"], kind="lever", source=src, unit=unit,
                                            worker=worker, text=text,
                                            evidence=[change + " - " + evidence],
                                            numbers=has_numbers(evidence)))

        # 3. probes ------------------------------------------------------------------------------------
        probes = d.get("flags_probed") or d.get("flag_probes") or []
        for probe in probes:
            if not isinstance(probe, dict):
                continue
            verdict = str(probe.get("verdict") or "")
            text = (_flags_of(probe) + " " + _effect_of(probe)).strip()
            if verdict == "adopt":
                if re.search(r"\bas committed\b|\bas configured\b", _flags_of(probe), re.I):
                    continue  # the committed baseline is not a finding
                entry = registry_match(text, ("lever",))
                if entry:
                    findings.append(Finding(key=entry["key"], kind="lever", source=src, unit=unit,
                                            worker=worker, text=text,
                                            evidence=[_flags_of(probe) + " - " + _effect_of(probe)],
                                            numbers=has_numbers(text)))
                else:
                    undraftable.append({"reason": "no-registry-entry", "source": src, "unit": unit,
                                        "text": _flags_of(probe), "effect": _effect_of(probe)})
            elif verdict in ("reject", "inconclusive"):
                entry = registry_match(text, ("trap",))
                if entry:
                    findings.append(Finding(key=entry["key"], kind="trap", source=src, unit=unit,
                                            worker=worker, text=text,
                                            evidence=[_flags_of(probe) + " - " + _effect_of(probe)],
                                            numbers=has_numbers(text)))

    return findings, undraftable


def extract_note_findings(notes_dir: str) -> list[Finding]:
    """Traps a worker recorded in prose, for registry entries that opt in (`scan_notes`)."""
    out: list[Finding] = []
    for path in sorted(glob.glob(os.path.join(notes_dir, "*.md"))):
        try:
            text = _read(path)
        except OSError:
            continue
        for entry in LEVERS:
            if not entry.get("scan_notes") or entry["kind"] != "trap":
                continue
            if entry.get("match_all") and not all(re.search(p, text, re.I) for p in entry["match_all"]):
                continue
            for pat in entry["match"]:
                m = re.search(pat, text, re.I)
                if not m:
                    continue
                line = text[:m.start()].count("\n")
                lines = text.splitlines()
                snippet = " ".join(l.strip() for l in lines[line:line + 3] if l.strip())[:400]
                out.append(Finding(key=entry["key"], kind="trap", source=os.path.basename(path),
                                   unit="", worker="", text=snippet, evidence=[snippet],
                                   numbers=has_numbers(snippet)))
                break
    return out


# -----------------------------------------------------------------------------------------------------------
# Grouping and rendering
# -----------------------------------------------------------------------------------------------------------
def group_findings(findings: list[Finding], rows: list[tuple[int, str, str]]) -> tuple[list[Group], list[dict], list[dict]]:
    """-> (groups to draft, already-landed, not-drafted)."""
    groups: dict[str, Group] = {}
    for f in findings:
        entry = next((e for e in LEVERS if e["key"] == f.key), None)
        if entry:
            g = groups.get(f.key)
            if not g:
                g = Group(key=f.key, title=entry["title"], kind=entry["kind"], problem=entry["problem"],
                          why=entry["why"], example=entry["example"], index=entry["index"])
                groups[f.key] = g
            g.findings.append(f)
        else:
            # an explicit request with no registry entry: one group per (worker, slug)
            key = f.key
            g = groups.get(key)
            if not g:
                title = f.title or f.text.split(" - ")[0].strip().rstrip(".") or f.text[:80]
                g = Group(key=key, title=title, kind="explicit", problem=f.text,
                          why="Reported by the worker; see the evidence below.",
                          example=None, index=(f.title or f.text).split(". ")[0][:220])
                groups[key] = g
            g.findings.append(f)

    already, notdraft, ready = [], [], []
    for g in groups.values():
        num = landed_in(g.title, rows)
        if num is not None:
            already.append({"key": g.key, "title": g.title, "row": num,
                            "units": g.units()})
            continue
        # evidence gate: a lever needs numbers; a trap needs a measured probe; explicit needs numbers
        numeric = any(f.numbers for f in g.findings)
        if g.kind == "trap":
            if not g.findings:
                notdraft.append({"key": g.key, "title": g.title, "reason": "no-measured-probe"})
            else:
                ready.append(g)
        elif numeric:
            ready.append(g)
        else:
            notdraft.append({"key": g.key, "title": g.title, "reason": "no-numbers",
                             "sources": [f.source for f in g.findings]})
    return ready, already, notdraft


def render_result(g: Group) -> str:
    """The Result paragraph: one bullet per distinct unit, carrying that worker's own numbers.

    A lever found by five workers is one row, so the evidence is grouped by unit and the most informative
    measured line is kept - not one bullet per outbox entry, which would repeat the same finding.
    """
    per_unit: dict[str, list[str]] = {}
    order: list[str] = []
    for f in g.findings:
        label = f.unit or f.source
        if label not in per_unit:
            per_unit[label] = []
            order.append(label)
        per_unit[label].extend(f.evidence)
    bullets = []
    for label in order:
        evs = [e.strip() for e in per_unit[label] if e.strip()]
        if not evs:
            continue
        numeric = [e for e in evs if has_numbers(e)]
        pick = min(numeric or evs, key=len)
        if len(pick) > 500:
            pick = pick[:497].rstrip() + "..."
        bullets.append("* `%s` - %s" % (label, pick))
    units = g.units()
    intro = ("%d unit(s) measured the same lever independently, so the evidence is grouped here rather than "
             "written once per outbox:" % len(units)) if g.kind != "trap" else \
            "Measured independently by %d worker(s):" % len(units)
    if not bullets:
        return intro
    return intro + "\n\n" + "\n".join(bullets)


def idea_stub_name(g: Group, number: int) -> str:
    """The suggested docs/matching file name for a drafted row: NNN-<up to six title words>.md."""
    words = re.findall(r"[a-z0-9]+", g.title.lower().replace("'", "").replace("`", ""))
    while len(words) > 1 and words[0] in ("a", "an", "the"):
        words = words[1:]
    return "%03d-%s.md" % (number, "-".join(words[:6]) or "idea")


def render_section(g: Group, number: int) -> str:
    """An idea file stub: the front matter prefilled (tags left for the author unless the heuristic is sure)."""
    tags = split_playbook.derive_tags(g.title, g.problem)
    parts = ["---", "id: %d" % number, "title: %s" % g.title, "status: works",
             "problem: %s" % " ".join(g.problem.split()), "tags: [%s]" % ", ".join(tags), "applies: []", "demo:",
             "---", "",
             "# %d. %s" % (number, g.title), "",
             "**Problem.** %s" % g.problem, "",
             "**Why try it.** %s" % g.why, "",
             "**Result.** %s" % render_result(g), ""]
    if g.example:
        parts += ["**Example.**", "", g.example, ""]
    return "\n".join(parts).rstrip() + "\n"


def render_agents_row(g: Group, number: int) -> str:
    return "| %d | %s | %s | done |" % (number, g.title, g.index)


def render_readme(ready, already, notdraft, undraftable, base) -> str:
    lines = ["# Playbook drafts (generated by `tools/units/playbook.py`)", "",
             "Drafts only - this tool never edits `docs/**` or `CLAUDE.md`. The orchestrator lands them in one",
             "commit: copy each `matching-row-*.md` (an idea stub, front matter prefilled) to `docs/matching/` under",
             "the file name in the table, review the tags, then run `python tools/agents/sync_playbook_index.py` and",
             "the skill's `sync_reference.py`. `agents-index-rows.md` is informational: the generated index picks",
             "rows from %d up up by itself." % (base - 1), "",
             "## Rows drafted", ""]
    if ready:
        lines += ["| new row | lever/trap | sources | findings | dups merged | draft | land as |",
                  "| --- | --- | --- | --- | --- | --- | --- |"]
        for i, g in enumerate(ready):
            num = base + i
            lines.append("| %d | %s | %d | %d | %d | `matching-row-%d-%s.md` | `docs/matching/%s` |"
                         % (num, g.title, len(g.units()), len(g.findings), len(g.findings) - 1,
                            num, g.key, idea_stub_name(g, num)))
    else:
        lines.append("(none)")
    lines += ["", "## Already landed (skipped)", ""]
    if already:
        for a in already:
            lines.append("* row %d - %s" % (a["row"], a["title"]))
    else:
        lines.append("(none)")
    lines += ["", "## Not drafted", ""]
    if notdraft:
        for n in notdraft:
            extra = (" sources: " + ", ".join(n.get("sources", []))) if n.get("sources") else ""
            lines.append("* %s - %s%s" % (n["title"], n["reason"], extra))
    else:
        lines.append("(none)")
    lines += ["", "## Other findings not drafted", "",
              "Adopted probes with no registry entry: they have numbers but no curated framing. Promote one to",
              "`LEVERS` in `tools/units/playbook.py` if it generalises, otherwise leave it unit-specific.", ""]
    if undraftable:
        for u in undraftable:
            lines.append("* [%s] `%s` (%s) - %s"
                         % (u["reason"], u["text"][:80], u["unit"], u["effect"][:160]))
    else:
        lines.append("(none)")
    return "\n".join(lines).rstrip() + "\n"


# -----------------------------------------------------------------------------------------------------------
# Run
# -----------------------------------------------------------------------------------------------------------
def load_outboxes(outbox_dir: str) -> list[tuple[str, dict]]:
    out = []
    for path in sorted(glob.glob(os.path.join(outbox_dir, "*.json"))):
        try:
            out.append((path, json.loads(_read(path))))
        except (OSError, json.JSONDecodeError) as exc:
            print("warn: skipping %s (%s)" % (os.path.basename(path), exc), file=sys.stderr)
    return out


def run(outbox_dir: str, notes_dir: str, drafts_dir: str, matching: str, agents: str,
        write: bool = True) -> dict:
    entries = load_outboxes(outbox_dir)
    rows = existing_rows(_read(agents) if os.path.isfile(agents) else "", matching)
    base = next_number(rows)

    findings, undraftable = extract_findings(entries)
    findings += extract_note_findings(notes_dir)
    ready, already, notdraft = group_findings(findings, rows)

    # stable order: levers first (registry order), then explicit, then traps - each by key
    order = {e["key"]: i for i, e in enumerate(LEVERS)}
    ready.sort(key=lambda g: (order.get(g.key, 1000), g.key))

    report = {
        "next_row": base,
        "outboxes": len(entries),
        "findings": len(findings),
        "rows_drafted": len(ready),
        "duplicates_merged": sum(len(g.findings) - 1 for g in ready),
        "already_landed": already,
        "not_drafted": notdraft,
        "unclassified": undraftable,
        "drafts": [],
    }

    if write:
        os.makedirs(drafts_dir, exist_ok=True)
        # a re-run must not leave a draft for a row that no longer groups (a stale matching-row-*.md)
        for stale in glob.glob(os.path.join(drafts_dir, "matching-row-*.md")):
            os.remove(stale)
        for i, g in enumerate(ready):
            num = base + i
            name = "matching-row-%d-%s.md" % (num, g.key)
            with open(os.path.join(drafts_dir, name), "w", encoding="utf-8", newline="\n") as fh:
                fh.write(render_section(g, num))
            report["drafts"].append(name)
        with open(os.path.join(drafts_dir, "agents-index-rows.md"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write("<!-- informational: docs/matching/index.md is generated; rows from %d up appear after sync_playbook_index.py -->\n" % (base - 1))
            for i, g in enumerate(ready):
                fh.write(render_agents_row(g, base + i) + "\n")
        with open(os.path.join(drafts_dir, "README.md"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(render_readme(ready, already, notdraft, undraftable, base))
        with open(os.path.join(drafts_dir, "findings.json"), "w", encoding="utf-8", newline="\n") as fh:
            json.dump(report, fh, indent=2, ensure_ascii=False)
            fh.write("\n")
    return report


def print_report(report: dict) -> None:
    print("next row: %d" % report["next_row"])
    print("outboxes: %d | findings: %d | rows drafted: %d | duplicates merged: %d"
          % (report["outboxes"], report["findings"], report["rows_drafted"], report["duplicates_merged"]))
    if report["drafts"]:
        print("drafts:")
        for d in report["drafts"]:
            print("  " + d)
    if report["already_landed"]:
        print("already landed:")
        for a in report["already_landed"]:
            print("  row %d  %s" % (a["row"], a["title"]))
    if report["not_drafted"]:
        print("not drafted:")
        for n in report["not_drafted"]:
            print("  %s (%s)" % (n["title"], n["reason"]))
    if report["unclassified"]:
        print("adopted probes with no registry entry: %d" % len(report["unclassified"]))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--outbox", default=OUTBOX)
    ap.add_argument("--notes", default=NOTES)
    ap.add_argument("--drafts", default=DRAFTS)
    ap.add_argument("--matching", default=MATCHING, help="the docs/matching directory of idea files")
    ap.add_argument("--agents", default=AGENTS, help="the generated docs/matching/index.md")
    ap.add_argument("--print", dest="print_only", action="store_true", help="report only, write nothing")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        import playbook_selftest
        return playbook_selftest.selftest()

    report = run(args.outbox, args.notes, args.drafts, args.matching, args.agents, write=not args.print_only)
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print_report(report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
