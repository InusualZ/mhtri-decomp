#!/usr/bin/env python3
"""Turn the `## Tooling and environment` sections of worker reports into a ranked register.

Every `decompiler` report now ends with one to three *tooling/environment* entries: the wall that cost the
most wall-clock time, phrased as a capability ("the worktree needs `orig/RMHE08/**` + `build/compilers`,
without the DOL nothing splits") rather than a complaint. Those entries used to die in the report; this
tool collects them from the same two sources `tools/units/playbook.py` reads - `.pi/outbox/*.json` and
`.pi/notes/*.md` - **clusters the ones that ask for the same thing**, and writes a single tracked register
(`docs/tooling-requests.md`) sorted by demand.

    python tools/units/tooling.py                 # scan, write docs/tooling-requests.md
    python tools/units/tooling.py --json          # the same data as JSON on stdout
    python tools/units/tooling.py --check         # exit 1 when the register is stale (for a commit hook)
    python tools/units/tooling.py --print         # human summary, write nothing
    python tools/units/tooling.py --set-status seed-worktree done
    python tools/units/tooling.py --outbox DIR --notes DIR --register PATH
    python tools/units/tooling.py --selftest

How a suggestion is found (all optional - the report schemas drift between rounds):

* an outbox key whose name contains `tool` or `environment` (string, list or object) - the structured
  channel the harness populates (`tooling`, `tools_wanted`, ...).  A **list** value is the structured channel proper: every
  element is one request the lane filed and becomes **its own row**, keyed by the tool/path it names
  (`target_of`), and the rows it did not become are reported (`skipped_tooling`, printed by `--print`/
  `--json`).  A lane that files two bullets gets two rows; they are never agglomerated with each other.
* a `.pi/notes/*.md` section whose heading names tooling/environment/worktree/setup/reproduction (the
  section variants workers actually wrote: `## Tooling notes for the next worker`, `## Environment note`,
  `## Worktree setup`, `## How to reproduce a measurement`, `## Tooling worth keeping`, ...);
* a signal line anywhere in a note or an outbox: a tool/env noun (`worktree`, `orig/`, `compilers`, `ninja`,
  `configure.py`, `recompile`, `objdiff`, `m2c`, `stylelint`, `junction`, ...) within a line that also carries
  a friction verb (`no`, `without`, `lacks`, `needs`, `had to`, `copied`, `by hand`, `cost`, `minutes`,
  `fails`, `unsafe`, `declines`, ...).

Clustering is the point. Two workers who hit the same wall phrase it differently, so the register does not
key on wording:

* a small curated `TOPICS` registry (the walls already known by name, e.g. the empty-worktree split) matches
  a whole *source*, and every distinct worker that mentions the wall is one vote;
* everything else is agglomerated by token overlap (Jaccard, with an overlap-coefficient escape hatch for a
  short phrasing of a longer one), so two novel phrasings of one request become **one entry with two votes**.
  A **structured** `tooling` list element is exempt from this: it is one filed request, so it never merges
  with another bullet (only the identical bullet from another lane is a second vote).

A `**Status.**` line per entry (`open` / `done` / `parked`) is carried across regenerations - the only
hand-editable part of the file - so the list stays honest as things get fixed.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import re
import sys
from dataclasses import dataclass, field

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUTBOX = os.path.join(MAIN, ".pi", "outbox")
NOTES = os.path.join(MAIN, ".pi", "notes")
REGISTER = os.path.join(MAIN, "docs", "tooling-requests.md")

STATUSES = ("open", "done", "parked")

# -----------------------------------------------------------------------------------------------------------
# The registry: the tooling walls already known by name. `match` is any-of, `match_all` is all-of; a source
# must satisfy `match_all` (when present) and at least one `match` to vote. Extending this tuple is how a
# recurring request gets a curated one-line ask instead of being clustered by similarity.
# -----------------------------------------------------------------------------------------------------------
TOPICS = (
    {
        "key": "seed-worktree",
        "ask": ("Seed a fresh worktree with `orig/RMHE08/**` and `build/compilers`, so a split runs without "
                "hand-copying them from MAIN."),
        "match": (
            r"worktree[^\n]{0,80}no `?orig/",
            r"no `?orig/[^\n]{0,40}at all",
            r"copied[^\n]{0,50}orig/RMHE08",
            r"orig/RMHE08[^\n]{0,40}(copied in|copy in|copied from|seed)",
            r"needs? `?orig/RMHE08",
            r"worktree[^\n]{0,60}(ships with no|has no|had no|without)[^\n]{0,40}(orig|compiler|build/)",
            r"build/compilers[^\n]{0,60}(copied|copy in|copied in|junction|seed|189 ?MB|167 ?MB|missing|absent)",
            r"fresh worktree[^\n]{0,80}no `?build/",
            r"no `?build\.ninja[^\n]{0,40}no `?orig/",
        ),
        "match_all": (r"orig/|build/compilers|build\.ninja",),
    },
    {
        "key": "recompile-worktree-target",
        "ask": ("Make `recompile.py --measure` work for a proposal unit: MAIN has no ninja rule or target "
                "object for it, so every measurement needs `--main <worktree>`."),
        "match": (
            r"MAIN (has|does not have)[^\n]{0,60}(no |target|this unit|ninja rule)",
            r"cannot run for a proposal unit",
            r"no ninja rule and no target",
            r"main[^\n]{0,20}no target object",
            r"--main <worktree>[^\n]{0,40}(required|only exist)",
            r"target object and the compile",
            r"does not exist[^\n]{0,40}measure against",
        ),
        "match_all": (r"recompile\.py",),
    },
    {
        "key": "scratch-measurer",
        "ask": ("Ship the worktree's scratch compile-and-score measurer (`build/tmp/*.py`, `build/scratch/`) as "
                "a supported tool, instead of an uncommitted script every worker rewrites."),
        "match": (
            r"scratch (harness|measurer|wrapper|script|file)",
            r"build/tmp/[a-z0-9_]+\.py",
            r"\.pi/scratch",
        ),
        "match_all": (r"measure|score|compile|recompile|report generate|objdiff",),
    },
    {
        "key": "score-with-one-report",
        "ask": ("Score a unit's symbols in one `objdiff report generate` call instead of N per-symbol "
                "measurements (one report gives all of them, ~40 s)."),
        "match": (
            r"one `?objdiff report generate",
            r"all \d+ symbols from one",
            r"instead of \d+ (measurements|calls|probes)",
            r"~?40 ?s per iteration",
            r"one `?report generate`? call",
        ),
    },
    {
        "key": "junction-unsafe",
        "ask": ("Do not junction `build/compilers` (or another build input) into MAIN: ninja can write or "
                "re-download through the junction."),
        "match": (
            r"do not junction",
            r"junction[^\n]{0,120}(unsafe|re-?download|write|rmdir|read-only)",
            r"re-?download[^\n]{0,40}through the junction",
        ),
    },
    {
        "key": "objdiff-size-gap",
        "ask": ("Teach objdiff (or the report) to pair symbols with a >50 % size gap: it declines them, so "
                "they read as 0 % and hide real unpaired code."),
        "match": (
            r"size gap",
            r">50 ?%",
            r"declines?[^\n]{0,40}(unpa|symbol|function)",
        ),
        "match_all": (r"objdiff|unpaired|pair",),
    },
    {
        "key": "include-order-shadow",
        "ask": ("`recompile.py` should put the worktree's `-i` includes before MAIN's, so a worktree header "
                "edit is not shadowed by MAIN's copy."),
        "match": (
            r"shadow",
            r"include order[^\n]{0,40}(worktree|MAIN)",
            r"-i[^\n]{0,12}paths[^\n]{0,40}(order|shadow)",
            r"prepend[^\n]{0,40}(worktree|include)",
        ),
        "match_all": (r"recompile|-i |include",),
    },
    {
        "key": "m2c-paired-single",
        "ask": ("Teach `m2c` (or filter) the Wii paired-single `psq_l`/`psq_st` saves: it renders them as "
                "`xxsel`/`vmrghb` garbage, so the first shape is unusable."),
        "match": (r"m2c",),
        "match_all": (r"psq_|xxsel|paired.single|vmrghb",),
    },
    {
        "key": "configure-stub-ninja",
        "ask": ("`configure.py` on a worktree without `orig/` emits a stub `build.ninja`; warn or fail loudly "
                "instead of building a tree that cannot split."),
        "match": (
            r"stub `?build\.ninja",
            r"only the top-?level rules",
            r"gated on `?build/RMHE08/config\.json",
        ),
    },
)

# -----------------------------------------------------------------------------------------------------------
# Text model
# -----------------------------------------------------------------------------------------------------------
HEAD = re.compile(r"^(#{1,4})\s+(.+?)\s*$")
# A section heading that names the tooling/environment channel (or the report variants workers used).
TOOL_HEAD = re.compile(
    r"tooling|environment|worktree (note|setup|build|/)|setup note|measuring in a fresh|"
    r"reproduce a measurement|how to reproduce|notes for the next|what the next|for the next "
    r"(round|lane|session)|tool note|build note|measurement note|worktree build|environment note|"
    r"tooling note|reproduc",
    re.I,
)
FRICTION = re.compile(
    r"no |without|lacks|lack |needs? |missing|had to|by hand|copy|copied|seed|cost|slow|took|"
    r"minutes|seconds|turns|calls|fails?|blocked|gap|does not exist|unavailable|deferred|manual|"
    r"instead|shadow|unsafe|re-?download|declines|twice|per iteration|wall.?clock",
    re.I,
)
ENV = re.compile(
    r"worktree|orig/|compilers|binutils|ninja|configure\.py|recompile|objdiff|m2c|stylelint|"
    r"tudiscover|build/|junction|harness|tooling|toolchain|split|target object|report generate",
    re.I,
)
FENCE = re.compile(r"^\s*(```|~~~)")
ASK = re.compile(
    r"needs? |cannot|can't|had to|by hand|would (have|save)|cost |slow|missing|lacks?|without|"
    r"fails?|blocked|unavailable|stub|instead of|deferred|manual|only writes|writes only|no target|"
    r"does not exist|per iteration|re-?download|unsafe|declines|spend|rewrite|re-?split",
    re.I,
)

STOP = set(
    "a an the is are was were be been being of to and or in on for it its by as that this these those at "
    "from into but not no when where which who whom what with without will would can could should may might "
    "must do does did done has have had here there then than so if only also very more most some any all "
    "each both few many much own same such too s t just now new still already being under over again once "
    "because while during before after above below off out up down".split()
)


def read(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def tokens(s: str) -> set[str]:
    """Subject tokens for clustering: words and code identifiers, no addresses, no stopwords."""
    out: set[str] = set()
    for w in re.findall(r"[a-z0-9_]+", (s or "").lower()):
        if w in STOP or len(w) < 3:
            continue
        if re.fullmatch(r"[0-9a-f]{6,}", w) and any(c.isdigit() for c in w):
            continue  # a bare hex address
        if re.search(r"\d{4,}", w):
            continue  # a symbol like fn_80073398 or auto_03_8005AA20_text
        out.add(w)
    return out


def jaccard(a: set[str], b: set[str]) -> float:
    if not a or not b:
        return 0.0
    return len(a & b) / len(a | b)


def slug(s: str) -> str:
    s = re.sub(r"[^a-z0-9]+", "-", (s or "").lower()).strip("-")
    return re.sub(r"-+", "-", s)[:60] or "request"


# A `*.py`/`*.md`/`*.exe` tool, or a `tools/...` path - what a structured tooling entry's key names.
TOOL_TOKEN_RE = re.compile(r"[A-Za-z0-9_][A-Za-z0-9_./\\-]*\.(?:py|md|exe|sh|cpp|h)\b|tools/[A-Za-z0-9_./-]+")


def target_of(text: str, entry=None) -> str:
    """What a structured tooling entry is *about* - the register key's stem ("key derived from the target").

    A `tooling` list element is a request the lane filed; the key names the tool the request is about, so
    two lanes asking about the **same first tool** land on one row (their votes pool) while two asks about
    different tools do not.  An explicit `target` (or `tool`/`path`/`subject`) field wins; else the **first**
    tool-ish token of the bullet (`datagap.py / flipcheck.py ...` -> `datagap.py`) - the further mentions
    stay in the ask text, where a pairing is part of the request rather than a second row; else the leading
    words before the first separator, for a bullet that names no tool at all.

    Keying on the first token is deliberate: with the first *two* the live scan produced both
    `tooling-datagap-py-flipcheck-py` and `tooling-flipcheck-py` for bullets naming the same tool, which is
    exactly the pooling this key exists to do (review, tooling.py 2026-09-28).
    """
    if isinstance(entry, dict):
        for k in ("target", "tool", "path", "subject"):
            v = entry.get(k)
            if isinstance(v, str) and v.strip():
                return v.strip()
    found = TOOL_TOKEN_RE.findall(text or "")
    if found:
        seen, out = set(), []
        for t in found:
            base = t.replace("\\", "/").rstrip(".")
            name = base.split("/")[-1]
            if name not in seen:
                seen.add(name)
                out.append(name)
        return " ".join(out[:1])
    head = re.split(r"[:;\u2014]\s|\s-\s", text or "", 1)[0]
    return " ".join((head or text or "").split()[:4])


def one_line(s: str, limit: int = 160) -> str:
    """Collapse a snippet to a single readable line, dropping markdown bullet/heading noise."""
    s = re.sub(r"^\s*(?:#{1,6}|[*+\-]|\d+\.)\s*", "", (s or "").strip())
    s = re.sub(r"`+", "`", s)
    s = re.sub(r"\s+", " ", s).strip()
    s = s.replace("|", "/")
    if len(s) > limit:
        cut = s[:limit].rstrip()
        if " " in cut:
            cut = cut[: cut.rfind(" ")]
        s = cut.rstrip(" .,;:") + " ..."
    return s


# -----------------------------------------------------------------------------------------------------------
# Cost: what the request cost the worker who filed it (minutes, and a human note)
# -----------------------------------------------------------------------------------------------------------
HOURS = re.compile(r"(\d+(?:\.\d+)?)\s*(?:hours?|hrs?|h)\b", re.I)
MINS = re.compile(r"(\d+(?:\.\d+)?)\s*(?:minutes?|mins?|min)\b", re.I)
SECS = re.compile(r"(\d+(?:\.\d+)?)\s*(?:seconds?|secs?|sec|s)\b", re.I)
COUNTS = re.compile(r"(\d+)\s*(measurements?|calls?|probes?|iterations?|rounds?|turns?|variants?|steps?)\b", re.I)


def cost_of(text: str) -> tuple[float, str]:
    """-> (minutes, human note). The largest single time unit wins; call/measurement counts weigh 0.5 min."""
    text = text or ""
    best, note = 0.0, ""
    m = HOURS.search(text)
    if m:
        best = float(m.group(1)) * 60
        note = m.group(0).strip()
    m = MINS.search(text)
    if m and float(m.group(1)) > best:
        best = float(m.group(1))
        note = m.group(0).strip()
    m = SECS.search(text)
    if m and float(m.group(1)) / 60 > best:
        best = float(m.group(1)) / 60
        note = m.group(0).strip()
    m = COUNTS.search(text)
    if m and int(m.group(1)) * 0.5 > best:
        best = int(m.group(1)) * 0.5
        note = m.group(0).strip()
    return best, note


def format_cost(minutes: float) -> str:
    if minutes >= 60:
        return "~%g h" % (minutes / 60)
    if minutes >= 1:
        return "~%g min" % minutes
    if minutes > 0:
        return "~%g s" % round(minutes * 60)
    return "-"


# -----------------------------------------------------------------------------------------------------------
# Sources and candidates
# -----------------------------------------------------------------------------------------------------------
@dataclass
class Source:
    kind: str  # outbox / note
    path: str
    voter: str  # one identity per worker/source, so a duplicate outbox does not double-vote
    worker: str
    unit: str
    text: str
    candidates: list["Cand"] = field(default_factory=list)


@dataclass
class Cand:
    source: "Source"
    text: str
    heading: str = ""
    scope: str = "section"  # section / line / tooling (a structured `tooling` list element)
    entry: object = None     # the original element, when it was structured (a dict may name its `target`)


def _flatten(value, out: list[str]) -> None:
    if isinstance(value, str):
        if value.strip():
            out.append(value.strip())
    elif isinstance(value, dict):
        for v in value.values():
            _flatten(v, out)
    elif isinstance(value, (list, tuple)):
        for v in value:
            _flatten(v, out)


def _sections(text: str) -> list[tuple[str, str]]:
    """[(heading, body)] for every heading, body up to the next heading of the same or higher level."""
    lines = text.splitlines()
    heads = [(i, len(m.group(1)), m.group(2)) for i, line in enumerate(lines)
             for m in [HEAD.match(line)] if m]
    out = []
    for j, (i, lvl, h) in enumerate(heads):
        end = len(lines)
        for i2, lvl2, _ in heads[j + 1:]:
            if lvl2 <= lvl:
                end = i2
                break
        out.append((h.strip(), "\n".join(lines[i + 1:end])))
    return out


def _items(body: str) -> list[str]:
    """Split a section body into bullet items and paragraphs, ignoring fenced code blocks."""
    items: list[str] = []
    cur: list[str] = []
    fence = False

    def flush():
        if cur:
            blob = " ".join(l.strip() for l in cur).strip()
            if len(blob) > 30:
                items.append(blob)
            cur.clear()

    for line in body.splitlines():
        if FENCE.match(line):
            fence = not fence
            continue
        if fence:
            continue
        is_bullet = bool(re.match(r"^\s*(?:[*+\-]|\d+\.)\s+", line))
        if is_bullet:
            flush()
        if line.strip():
            cur.append(line)
        else:
            flush()
    flush()
    return items


def candidates_for(source: Source) -> list[Cand]:
    out: list[Cand] = []
    seen: set[str] = set()
    if source.kind == "outbox":
        try:
            d = json.loads(source.text)
        except json.JSONDecodeError:
            d = {}
        blobs: list[str] = []
        for key, value in (d.items() if isinstance(d, dict) else []):
            if not re.search(r"tool|environment", str(key), re.I):
                continue
            if isinstance(value, list):
                # A STRUCTURED list of tooling entries: each element is a request the lane filed, and each
                # becomes its OWN register item (`build_entries`).  They used to be flattened into one blob
                # and agglomerated by token overlap with everything else, so a lane that filed two bullets
                # got one item (the `.init` lane, 2026-09-28) or none, and its second ask was read by
                # nobody.  No friction/env filter here: the lane said "tooling", which is the signal.
                for el in value:
                    if isinstance(el, str):
                        text = " ".join(el.split())
                    else:
                        parts: list[str] = []
                        _flatten(el, parts)
                        text = " ".join(" ".join(parts).split())
                    if text:
                        out.append(Cand(source, text, str(key), "tooling", el))
                continue
            _flatten(value, blobs)
        for blob in blobs:
            for h, body in _sections(blob):
                if TOOL_HEAD.search(h):
                    for item in _items(body):
                        out.append(Cand(source, item, h, "section"))
            if not _sections(blob):
                out.append(Cand(source, blob, "tooling", "section"))
        # a `## Tooling and environment` section stored under any string field (e.g. `notes`)
        for value in (d.values() if isinstance(d, dict) else []):
            vals: list[str] = []
            _flatten(value, vals)
            for v in vals:
                for h, body in _sections(v):
                    if TOOL_HEAD.search(h):
                        for item in _items(body):
                            out.append(Cand(source, item, h, "section"))
        # structured prose fields are evidence for a topic, but never novel candidates
        for key in ("residual", "blockers", "measured_with", "notes"):
            vals = []
            _flatten((d or {}).get(key), vals)
            for v in vals:
                out.append(Cand(source, v, str(key), "line"))
    else:
        for h, body in _sections(source.text):
            if TOOL_HEAD.search(h):
                for item in _items(body):
                    out.append(Cand(source, item, h, "section"))
    # signal lines anywhere in the source (shared by both kinds)
    fence = False
    for line in source.text.splitlines():
        if FENCE.match(line):
            fence = not fence
            continue
        if fence or HEAD.match(line) or not line.strip():
            continue
        if FRICTION.search(line) and ENV.search(line) and len(line.strip()) > 30:
            out.append(Cand(source, line.strip(), "", "line"))
    deduped: list[Cand] = []
    for c in out:
        k = re.sub(r"\s+", " ", c.text).lower()
        if k in seen:
            continue
        seen.add(k)
        deduped.append(c)
    return deduped


def load_sources(outbox_dir: str, notes_dir: str) -> list[Source]:
    sources: list[Source] = []
    for path in sorted(glob.glob(os.path.join(outbox_dir, "*.json"))):
        try:
            d = json.loads(read(path))
        except (OSError, json.JSONDecodeError) as exc:
            print("warn: skipping %s (%s)" % (os.path.basename(path), exc), file=sys.stderr)
            continue
        stem = os.path.splitext(os.path.basename(path))[0]
        worker = str(d.get("worker") or "")
        voter = re.sub(r"^worker-", "", worker or stem)
        s = Source(kind="outbox", path=os.path.basename(path), voter=voter, worker=worker,
                   unit=str(d.get("unit") or ""),
                   text=json.dumps(d, sort_keys=True, ensure_ascii=False, indent=1))
        s.candidates = candidates_for(s)
        sources.append(s)
    for path in sorted(glob.glob(os.path.join(notes_dir, "*.md"))):
        try:
            text = read(path)
        except OSError:
            continue
        stem = os.path.splitext(os.path.basename(path))[0]
        s = Source(kind="note", path=os.path.basename(path), voter=stem, worker=stem, unit="", text=text)
        s.candidates = candidates_for(s)
        sources.append(s)
    return sources


# -----------------------------------------------------------------------------------------------------------
# Topic matching + similarity clustering
# -----------------------------------------------------------------------------------------------------------
def topic_hit(topic: dict, text: str) -> bool:
    if topic.get("match_all") and not all(re.search(p, text, re.I) for p in topic["match_all"]):
        return False
    return (not topic.get("match")) or any(re.search(p, text, re.I) for p in topic["match"])


def _primary_patterns(topic: dict) -> tuple[str, ...]:
    return tuple(topic.get("match_all") or ()) + tuple(topic.get("match") or ())


def _hit_count(topic: dict, text: str) -> int:
    n = 0
    for pat in tuple(topic.get("match_all") or ()) + tuple(topic.get("match") or ()):
        if re.search(pat, text, re.I):
            n += 1
    return n


def best_evidence(source: Source, topic: dict) -> str:
    """The worker's own words for this wall: the best-matching tooling item, else the matching line."""
    scored: list[tuple[int, int, str]] = []
    for c in source.candidates:
        if not topic_hit(topic, c.text):
            continue
        score = _hit_count(topic, c.text) * 10
        if c.scope == "section":
            score += 2
        if ASK.search(c.text):
            score += 3
        if FRICTION.search(c.text):
            score += 2
        if cost_of(c.text)[0] > 0:
            score += 4
        score -= min(len(c.text) // 150, 5)
        scored.append((score, len(c.text), c.text))
    if scored:
        scored.sort(key=lambda t: (-t[0], t[1]))
        return one_line(scored[0][2], 320)
    # no pre-extracted item: the best single line that names the wall
    best, best_score = "", 0
    lines = source.text.splitlines()
    for i, line in enumerate(lines):
        ls = line.strip()
        if len(ls) < 30 or ls.startswith('"') or ls.startswith("'") or HEAD.match(ls):
            continue
        if not topic_hit(topic, ls):
            continue
        score = _hit_count(topic, ls) + (2 if ASK.search(ls) else 0)
        if score > best_score:
            nxt = lines[i + 1].strip() if i + 1 < len(lines) and len(ls) < 140 else ""
            best, best_score = one_line(ls + (" " + nxt if nxt else ""), 320), score
    if best:
        return best
    for pat in _primary_patterns(topic):
        m = re.search(pat, source.text, re.I)
        if m:
            a, b = max(0, m.start() - 120), min(len(source.text), m.end() + 160)
            return one_line(source.text[a:b], 320)
    return ""


@dataclass
class Cluster:
    cands: list[Cand]

    @property
    def sources(self) -> list[Source]:
        seen, out = set(), []
        for c in self.cands:
            if c.source.path not in seen:
                seen.add(c.source.path)
                out.append(c.source)
        return out

    def ask(self) -> str:
        uniq: list[Cand] = []
        seen: set[str] = set()
        for c in self.cands:
            k = re.sub(r"\s+", " ", c.text).lower()
            if k not in seen:
                seen.add(k)
                uniq.append(c)
        if len(uniq) == 1:
            return one_line(uniq[0].text)
        toks = [tokens(c.text) for c in uniq]
        best, best_score = uniq[0], -1.0
        for i, c in enumerate(uniq):
            score = sum(jaccard(toks[i], toks[j]) for j in range(len(uniq)) if j != i)
            if score > best_score:
                best, best_score = c, score
        return one_line(best.text)


def similarity_clusters(cands: list[Cand], threshold: float = 0.34) -> list[Cluster]:
    """Greedy agglomeration: two phrasings of one request land in the same cluster."""
    clusters: list[Cluster] = []
    members: list[set[str]] = []
    for c in sorted(cands, key=lambda c: (c.source.voter, c.text)):
        tk = tokens(c.text)
        best, best_score = None, 0.0
        for i, cl in enumerate(clusters):
            inter = len(tk & members[i])
            if not inter:
                continue
            j = inter / len(tk | members[i])
            if j >= threshold:
                if j > best_score:
                    best, best_score = i, j
        if best is not None:
            clusters[best].cands.append(c)
            members[best] |= tk
        else:
            clusters.append(Cluster([c]))
            members.append(set(tk))
    return clusters


# -----------------------------------------------------------------------------------------------------------
# Entries
# -----------------------------------------------------------------------------------------------------------
@dataclass
class Evidence:
    voter: str
    label: str
    snippet: str
    cost: float
    cost_note: str


@dataclass
class Entry:
    key: str
    ask: str
    votes: int
    cost: float
    cost_note: str
    status: str
    evidence: list[Evidence]

    @property
    def voters(self) -> list[str]:
        seen, out = set(), []
        for e in self.evidence:
            if e.voter not in seen:
                seen.add(e.voter)
                out.append(e.voter)
        return out


def build_entries(sources: list[Source], statuses: dict[str, str]) -> tuple[list[Entry], list[dict]]:
    entries: list[Entry] = []
    claimed: set[str] = set()  # candidate texts already owned by a topic

    for topic in TOPICS:
        ev: list[Evidence] = []
        seen_voters: set[str] = set()
        # notes before outboxes: the tooling section lives in the note, and one voter must not be
        # represented by a terse outbox when their report is available
        ordered = sorted(sources, key=lambda s: (0 if s.kind == "note" else 1, s.voter, s.path))
        for s in ordered:
            if not topic_hit(topic, s.text):
                continue
            if s.voter in seen_voters:
                continue  # one vote per worker, however many reports they filed
            seen_voters.add(s.voter)
            snippet = best_evidence(s, topic)
            minutes, note = cost_of(snippet)
            if minutes == 0:  # the quote may not carry the cost; the section often does
                blob = " ".join(c.text for c in s.candidates if topic_hit(topic, c.text))
                minutes, note = cost_of(blob)
            label = s.worker or s.unit or s.path
            ev.append(Evidence(voter=s.voter, label=label, snippet=snippet, cost=minutes, cost_note=note))
            for c in s.candidates:
                if topic_hit(topic, c.text):
                    claimed.add(re.sub(r"\s+", " ", c.text).lower())
        if not ev:
            continue
        ev.sort(key=lambda e: (e.label.lower(), e.snippet))
        cost = max(e.cost for e in ev)
        note = next((e.cost_note for e in ev if e.cost == cost and e.cost_note), "")
        entries.append(Entry(key=topic["key"], ask=topic["ask"], votes=len(ev), cost=cost,
                             cost_note=note, status=statuses.get(topic["key"], "open"), evidence=ev))

    # Novel requests: tooling-section items no topic owns, clustered by what they ask for.
    novel = []
    for s in sources:
        for c in s.candidates:
            if c.scope != "section":
                continue
            if re.sub(r"\s+", " ", c.text).lower() in claimed:
                continue
            if any(topic_hit(t, c.text) for t in TOPICS):
                continue
            if not (FRICTION.search(c.text) and ENV.search(c.text)):
                continue  # an informational item, not a request
            novel.append(c)
    novel_entries: list[Entry] = []
    for cl in similarity_clusters(novel):
        srcs = cl.sources
        # a novel row needs demand: a single unmeasured mention is a note, not a ranked request
        if len(srcs) < 2:
            continue
        ask = cl.ask()
        if not ask:
            continue
        key = "novel-" + slug(ask)[:48]
        ev = []
        for c in cl.cands:
            minutes, note = cost_of(c.text)
            ev.append(Evidence(voter=c.source.voter, label=c.source.worker or c.source.unit or c.source.path,
                               snippet=one_line(c.text, 320), cost=minutes, cost_note=note))
        ev.sort(key=lambda e: (e.label.lower(), e.snippet))
        uniq: list[Evidence] = []
        seen: set[str] = set()
        for e in ev:
            if e.voter in seen:
                continue
            seen.add(e.voter)
            uniq.append(e)
        cost = max((e.cost for e in uniq), default=0.0)
        note = next((e.cost_note for e in uniq if e.cost == cost and e.cost_note), "")
        novel_entries.append(Entry(key=key, ask=ask, votes=len(uniq), cost=cost, cost_note=note,
                                   status=statuses.get(key, "open"), evidence=uniq))
    entries.extend(novel_entries)

    # --- Structured tooling entries: every element of an outbox's `tooling`/`environment` LIST -----------
    # One request per bullet, each its own register item keyed by the target it names.  These used to be
    # flattened into the generic novel pool and agglomerated by token overlap with every other candidate,
    # so a lane that filed two bullets got one row (the `.init` lane, 2026-09-28: two `tooling` bullets,
    # one row) or none, and the `residual` prose that carried the rest was read by nobody.  A lane's two
    # asks are two asks: nothing is clustered ACROSS bullets here.  Real demand only - a bullet one lane
    # filed is a request, because the lane said "tooling" - and a bullet a curated topic already owns is
    # folded, not duplicated; both the folds and the skips are reported.
    groups: dict[str, list[Cand]] = {}
    order: list[str] = []
    skipped: list[dict] = []
    for s in sources:
        for c in s.candidates:
            if c.scope != "tooling":
                continue
            label = s.worker or s.unit or s.path
            norm = re.sub(r"\s+", " ", c.text).strip().lower()
            if not norm or len(norm) <= 30:
                skipped.append({"voter": s.voter, "label": label, "text": one_line(c.text, 200),
                                "why": "too short to be a request"})
                continue
            if norm in claimed:
                skipped.append({"voter": s.voter, "label": label, "text": one_line(c.text, 200),
                                "why": "folded into a curated topic row"})
                continue
            if norm not in groups:
                groups[norm] = []
                order.append(norm)
            groups[norm].append(c)
    used: set[str] = set()
    for norm in order:
        cands = groups[norm]
        target = target_of(cands[0].text, cands[0].entry)
        base_key = "tooling-" + slug(target)[:48]
        key, n = base_key, 2
        while key in used:
            key, n = "%s-%d" % (base_key, n), n + 1
        used.add(key)
        ev = []
        for c in cands:
            minutes, note = cost_of(c.text)
            ev.append(Evidence(voter=c.source.voter, label=c.source.worker or c.source.unit or c.source.path,
                               snippet=one_line(c.text, 320), cost=minutes, cost_note=note))
        ev.sort(key=lambda e: (e.label.lower(), e.snippet))
        uniq: list[Evidence] = []
        seen: set[str] = set()
        for e in ev:
            if e.voter in seen:
                continue
            seen.add(e.voter)
            uniq.append(e)
        cost = max((e.cost for e in uniq), default=0.0)
        note = next((e.cost_note for e in uniq if e.cost == cost and e.cost_note), "")
        entries.append(Entry(key=key, ask=one_line(cands[0].text), votes=len(uniq), cost=cost,
                             cost_note=note, status=statuses.get(key, "open"), evidence=uniq))

    entries.sort(key=lambda e: (-e.votes, -e.cost, e.ask.lower()))
    return entries, skipped


# -----------------------------------------------------------------------------------------------------------
# Status parsing and rendering
# -----------------------------------------------------------------------------------------------------------
KEY_RE = re.compile(r"<!--\s*tooling-key:\s*([a-z0-9\-]+)\s*-->")
STATUS_RE = re.compile(r"^\*\*Status\.\*\*\s*`?(open|done|parked)`?\s*$", re.I | re.M)


def parse_statuses(text: str) -> dict[str, str]:
    """The `**Status.**` line under each `<!-- tooling-key: ... -->`, the one hand-editable field."""
    out: dict[str, str] = {}
    key = None
    for line in text.splitlines():
        m = KEY_RE.search(line)
        if m:
            key = m.group(1)
            continue
        m = STATUS_RE.match(line.strip())
        if m and key:
            out[key] = m.group(1).lower()
            key = None
    return out


def render(entries: list[Entry], outboxes: int, notes: int, sources: int) -> str:
    votes = sum(e.votes for e in entries)
    lines = [
        "# Tooling requests",
        "",
        "The ranked list of tooling and environment improvements the workers have asked for. Each entry is",
        "one *request*, not one report: workers who phrase the same wall differently vote on the same row, and",
        "the list is sorted by how many workers asked for it, then by what it cost. Generated by",
        "`tools/units/tooling.py` from `.pi/outbox/*.json` and `.pi/notes/*.md` - rerun it rather than editing",
        "anything but the **Status.** line (that line is carried across regenerations).",
        "",
        "Status is `open` (not built), `done` (built), or `parked` (decided against). Set it with "
        "`python tools/units/tooling.py --set-status <key> <status>` or edit the **Status.** line directly; "
        "the key is the `tooling-key` comment above it.",
        "",
        "_%d sources (%d outboxes, %d notes), %d requests, %d votes._"
        % (sources, outboxes, notes, len(entries), votes),
        "",
        "| # | request | votes | cost | status |",
        "| --- | --- | --- | --- | --- |",
    ]
    for i, e in enumerate(entries, 1):
        rows = []
        for ev in e.evidence:
            who = ev.label
            rows.append("`%s`" % who)
        lines.append("| %d | %s | %d | %s | %s |" % (i, e.ask, e.votes, format_cost(e.cost), e.status))
    lines += ["", "---", ""]
    for i, e in enumerate(entries, 1):
        lines += ["## %d. %s" % (i, e.ask), "", "<!-- tooling-key: %s -->" % e.key,
                  "**Votes.** %d  |  **Cost.** %s%s" %
                  (e.votes, format_cost(e.cost), (" (%s)" % e.cost_note) if e.cost_note else ""),
                  "", "**Status.** %s" % e.status, "", "**Evidence.**", ""]
        for ev in e.evidence:
            cost = ("; cost: %s" % (ev.cost_note or format_cost(ev.cost))) if (ev.cost or ev.cost_note) else ""
            lines.append("* `%s` - %s%s" % (ev.label, ev.snippet, cost))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def entries_to_json(entries: list[Entry], outboxes: int, notes: int, sources: int,
                    skipped: list[dict] | None = None) -> dict:
    return {
        "sources": sources,
        "outboxes": outboxes,
        "notes": notes,
        "requests": len(entries),
        "votes": sum(e.votes for e in entries),
        "skipped_tooling": skipped or [],
        "entries": [
            {
                "rank": i,
                "key": e.key,
                "ask": e.ask,
                "votes": e.votes,
                "cost_minutes": round(e.cost, 3),
                "cost_note": e.cost_note,
                "status": e.status,
                "evidence": [
                    {"worker": ev.label, "voter": ev.voter, "snippet": ev.snippet,
                     "cost_minutes": round(ev.cost, 3), "cost_note": ev.cost_note}
                    for ev in e.evidence
                ],
            }
            for i, e in enumerate(entries, 1)
        ],
    }


# -----------------------------------------------------------------------------------------------------------
# Run
# -----------------------------------------------------------------------------------------------------------
def scan(outbox_dir: str, notes_dir: str, statuses: dict[str, str]) -> tuple[str, dict, list[Entry]]:
    sources = load_sources(outbox_dir, notes_dir)
    outboxes = sum(1 for s in sources if s.kind == "outbox")
    notes = sum(1 for s in sources if s.kind == "note")
    entries, skipped = build_entries(sources, statuses)
    text = render(entries, outboxes, notes, len(sources))
    report = entries_to_json(entries, outboxes, notes, len(sources), skipped)
    return text, report, entries


def print_report(report: dict) -> None:
    print("sources: %d (%d outboxes, %d notes) | requests: %d | votes: %d"
          % (report["sources"], report["outboxes"], report["notes"], report["requests"], report["votes"]))
    for e in report["entries"]:
        print("  %2d. %-5d votes  %-8s %s" % (e["rank"], e["votes"], e["status"], one_line(e["ask"], 90)))
    skipped = report.get("skipped_tooling") or []
    if skipped:
        print("tooling bullets not made into a row: %d" % len(skipped))
        for s in skipped:
            print("  - %s [%s]: %s" % (s["label"], s["why"], one_line(s["text"], 90)))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--outbox", default=OUTBOX)
    ap.add_argument("--notes", default=NOTES)
    ap.add_argument("--register", default=REGISTER)
    ap.add_argument("--json", action="store_true", help="emit the register as JSON on stdout (no write)")
    ap.add_argument("--check", action="store_true", help="exit 1 when the register is missing or stale")
    ap.add_argument("--print", dest="print_only", action="store_true", help="summary only, write nothing")
    ap.add_argument("--set-status", nargs=2, metavar=("KEY", "STATUS"),
                    help="set an entry's status (open/done/parked) and regenerate")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        import tooling_selftest
        return tooling_selftest.selftest()

    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, OSError):
        pass

    existing = read(args.register) if os.path.exists(args.register) else ""
    statuses = parse_statuses(existing)
    if args.set_status:
        key, status = args.set_status
        status = status.lower()
        if status not in STATUSES:
            print("status must be one of: %s" % ", ".join(STATUSES), file=sys.stderr)
            return 2
        statuses[key] = status

    text, report, _ = scan(args.outbox, args.notes, statuses)

    if args.check:
        if not os.path.exists(args.register):
            print("check: %s does not exist; run the tool" % args.register, file=sys.stderr)
            return 1
        if existing != text:
            print("check: %s is stale; run `python tools/units/tooling.py`" % args.register, file=sys.stderr)
            return 1
        print("check: %s is up to date (%d requests)" % (args.register, report["requests"]))
        return 0

    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print_report(report)

    if not args.print_only and not args.json:
        os.makedirs(os.path.dirname(args.register), exist_ok=True)
        if existing != text:
            with open(args.register, "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
            print("wrote %s (%d requests)" % (args.register, report["requests"]), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
