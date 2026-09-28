#!/usr/bin/env python3
"""One ranked register of everything the campaign filed but has not done - the *backlog*.

The owner's rule (2026-09-27, revised): **one resolved backlog item per new proposal claim.** While the
backlog has open items, `queue.py next` does not hand out an unbounded stream of new proposal claims: it
keeps a **credit ledger**. A `done` earns 1 credit; a claim spends 1 credit (`--ratio K` makes a claim cost
K, i.e. K backlog items per claim); the register starts with 1 credit so the campaign can begin. The rule
was unenforceable because the backlog was invisible: every lane ends with an outbox `.pi/outbox/*.json`
whose `config_requests` list records what it found but was not allowed to change, and nothing tracked
whether any of it was ever done (266 outboxes / 703 requests on 2026-09-27). This tool makes it one register
with a lifecycle and a ledger, and `queue.py next` reads it.

**The ledger can never drift.** Earnings are *derived from the item statuses* - the count of items a lane
resolved (`done` that is not the source's default) - not from a stored counter, so enforcement never depends
on the file surviving a clean checkout. `parked` earns **no** credit: parking removes a ghost, it does not
buy a claim; only `done` does. The spent side (the claims handed out) is the one thing that cannot be
derived from the statuses, so it is persisted alongside them in `.pi/backlog.json`; losing that file is
fail-open by design (the statuses are lost with it, so the register restarts at 1 credit).

**`triage` is evidence-based and never guesses.** Many "open" items were filed weeks ago against code that
has since moved, so `python tools/units/backlog.py triage` classifies every open item as `resolved`,
`stale` or `open` and prints the check that proved each. `--apply` writes the verdicts (`resolved` -> `done`,
`stale` -> `parked`) and is idempotent, respecting any status a human already set. Anything that cannot be
proved from the repository stays `open (no check)` - a triage that guesses is worse than the pile it is
triaging. A lint-derived item follows the same rule: a `naming` / `band-header` item is `resolved` when the
file no longer carries that rule's findings (re-linted, not remembered), `stale` when the file is gone, and
`open` with the live count otherwise.

Three sources, one register:

* **`config_requests`** in `.pi/outbox/*.json`. A `range` (a seam re-draw, or a data run to claim) is open
  until a proposal pass re-draws it; a `shared-file` (a defect in a header a worker may not touch) is open
  until fixed; a `flag` (a compiler flag for a lib) is open until measured and adopted or rejected. A
  **`rename` is not backlog** - the landing applies it, so it is done when its batch lands - and is not
  carried at all.
* **`tools/units/tooling.py`**'s own register (`docs/tooling-requests.md`): the ranked tooling/environment
  requests with their open/done/parked statuses. It is read, never duplicated.
* **`tools/units/stylelint.py`**'s findings, aggregated **per file**: one `naming` item per file carrying
  rule-7 findings (`fn_XXXXXXXX` / `lbl_XXXXXXXX` / `loc_XXXXXXXX` / bare `unk*`), one `band-header` item
  per file carrying rule-2 findings (an `extern` that belongs in the owner's header or `include/unsplit/`),
  and one `untyped` item per file carrying rule-11 findings (a `void *` parameter or return type with no
  `/* untyped: <reason> */` marker). The item's ask names the file, the rule and the count, and the count
  is the item's rank weight, so the high-traffic file surfaces first. A lint item's key is the (kind, file)
  pair, so a partial fix keeps its status; the item is carried forward from the published register even
  after the findings are gone, so `triage` can prove the file clean and close it (and so a resolved item
  stays in the register and earns its credit). This is the "do not revoke committed progress - work the
  debt slowly" half of the owner's naming ruling (2026-09-27): hundreds of such open items against the
  campaign's balance keep naming and typing work interleaved with new claims through the ratio, with no
  special-casing.

This is also the owner's "do not revoke committed progress - put the mounted naming debt in a backlog and
work on it slowly" half: the stylelint items are ordinary open items, so the credit ratio rations new
claims against them exactly as it rations against the rest.

The pile is not 703 problems. The same defect is filed by several lanes over weeks, so items are keyed on
`(kind, target, normalised-defect)`: for `shared-file` the header path plus a normalised summary of the
defect, for `range` the address span, for `flag` the lib plus the flag. Repeats collapse into one item that
records **how many lanes filed it and which** - a defect three lanes independently hit is a priority signal.

`shared-file` mixes *records* with *requests*. A lane that used the shared-file exception files an entry
describing what it **did** ("Added one union member to the +0x328 union ..."); those changes rode the branch
that landed, so they are done by definition. Only an entry that states a live defect ("line 67 declares X
while ... declares Y, so any TU that includes both fails with MWCC"; "should take one argument") is open.
A small classifier splits them; every item keeps the raw filing text, so a wrong classification is visible
rather than hidden.

Statuses and the credit ledger live in `MAIN/.pi/backlog.json` (gitignored, the way `claims.json` is), so
they survive a regeneration and can be set with `--set-status KEY STATUS`. The register is regenerated after
each change.

    python tools/units/backlog.py                       # regenerate MAIN/.pi/backlog.json + summary
    python tools/units/backlog.py --print [--top N]     # human summary, write nothing
    python tools/units/backlog.py --json                # the register as JSON on stdout (no write)
    python tools/units/backlog.py --check               # exit 1 when the register is missing or stale
    python tools/units/backlog.py --set-status KEY done # open / done / parked, then regenerate
    python tools/units/backlog.py --selftest

Ranking is by what predicts value: the number of independent filers, then an item's weight (a
`naming`/`band-header` item's live finding count, so the high-traffic file leads), then recency, then
`tooling.py`'s votes. An item also shows how long it has been open; an item filed in an early phase may be
stale because the code moved on, and that is exactly what the register is for - it is surfaced, never
silently dropped, and a human or lane parks it.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import glob
import hashlib
import json
import os
import re
import sys
import tempfile
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

from units import tooling as tg  # noqa: E402  (the second source: its register is read, not rebuilt)
from units import handoff as handoff_mod  # noqa: E402 (the outbox schema: FREE_TEXT_FIELDS is one definition)

STATUSES = ("open", "done", "parked")
NEW_KINDS = ("shared-file", "range", "seam", "flag", "tooling", "naming", "band-header", "untyped")
# default open; a `rename` is never carried
# The lint-derived kinds: `naming` is one item per file carrying rule-7 findings, `band-header` one per file
# carrying rule-2 findings, `untyped` one per file carrying rule-11 findings (`build_items` /
# `collect_lint_items`). All are ordinary open items, so the credit ratio rations new claims against them -
# no special-casing.
LINT_KINDS = ("naming", "band-header", "untyped")
LINT_RULES = {"naming": 7, "band-header": 2, "untyped": 11}

# -----------------------------------------------------------------------------------------------------------
# Text helpers
# -----------------------------------------------------------------------------------------------------------
def read(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def asstr(x) -> str:
    """A config_requests field as text (some are lists - e.g. a flag `change` of `["-opt","nopeephole"]`)."""
    if isinstance(x, str):
        return x
    if x is None:
        return ""
    return json.dumps(x, ensure_ascii=False)


def one_line(s: str, limit: int = 200) -> str:
    s = re.sub(r"\s+", " ", (s or "").strip())
    s = s.replace("|", "/")
    if len(s) > limit:
        cut = s[:limit].rstrip()
        if " " in cut:
            cut = cut[: cut.rfind(" ")]
        s = cut.rstrip(" .,;:") + " ..."
    return s


def free_text(r: dict, preferred: tuple = ()) -> str:
    """A request's free-text content: the first non-empty field, `preferred` spellings first then the shared
    `handoff.FREE_TEXT_FIELDS` list. `handoff.py` owns the schema and this intake reads the same list, so a
    filing that puts its content under its lane's own spelling (`why`, `request`, `what`, `subject`, `note`)
    is read here rather than registering content-free and being swallowed by a neighbour.
    """
    for field in tuple(preferred) + handoff_mod.FREE_TEXT_FIELDS:
        value = asstr(r.get(field))
        if value.strip():
            return value
    return ""


def norm_file(x) -> str:
    """A shared-file target: the header path, with the reporters' noise (``(NEW)``, ``(or a shared owner)``)."""
    s = asstr(x).strip()
    s = re.sub(r"\s*\(.*?\)\s*$", "", s)          # trailing parenthetical note
    s = re.sub(r"^include/\s*\(.*", "include/", s)  # "include/ (a new shared header, e.g. ...)"
    s = re.sub(r"^src/\s*\(.*", "src/", s)
    s = s.replace("\\", "/").strip().strip("`")
    return re.sub(r"\s+", " ", s).lower()


def clauses(why) -> list[str]:
    """Split one entry that carries several problems into its clauses.

    A single shared-file entry can state *two* defects in one header ("two problems in the header this band
    takes ... from. (1) line 23 opens a file-wide `#pragma peephole off` ... (2) line 336 declares ..."), and
    those are two backlog items, not one. Only explicit enumeration is split - a defect statement is not
    chopped at every full stop.
    """
    why = asstr(why).strip()
    marks = list(re.finditer(r"(?:^|[\s.;])(?:\((\d)\)|(\d)\))\s+", why))
    if len(marks) >= 2:
        out = []
        for i, m in enumerate(marks):
            start = m.start()
            end = marks[i + 1].start() if i + 1 < len(marks) else len(why)
            out.append(why[start:end].strip(" .;"))
        return [c for c in out if len(c) > 15]
    return [why] if why else []


# -----------------------------------------------------------------------------------------------------------
# The classifier: a *record* (done when its branch lands) versus a *defect* (open until fixed).
# A live `shared-file` defect names a state that is still wrong; a record names an action already taken.
# -----------------------------------------------------------------------------------------------------------
OPEN_RE = re.compile(
    r"(DEFECT\b|not a request|REPAIR NEEDED|wrong arity|\barity\b|should take|should move|must move|"
    r"needs to move|conflict|re-?declares?|declared in (two|both)|defined in (two|\d+)|"
    r"two problems|two definitions|opens? a file-wide|leaks? into|illegal function|fails? with MWCC|"
    r"still (declares?|defines?|contains?|owns?|carries?)|has no declaration|no declaration in|"
    r"worth re-?measuring|rule.?2 (debt|follow)|must (move|trim|be moved)|the declaration should|"
    r"should (move|take|include|declare)|cannot enter their owner|declare them once|"
    r"must not silently|has to move|call site consumes|returned pointer|consumes the return)", re.I)
RECORD_LEAD_RE = re.compile(
    r"^\s*(Added|Adds?|added|Removed|removed|Renamed|renamed|Created|created|Moved|moved|"
    r"Registered|registered|Declared|declared|Extended|extended|Updated|updated|Edited|edited|"
    r"Split|split|Fixed|fixed|Done|done|The \d+ renames|the \d+ renames|This batch|My batch|Our batch|"
    r"new (owner |shared |unit )?header|New (owner |shared |unit )?header|"
    r"Rule [12] home|rule [12] home|the §2 registration|section 2 registration|"
    r"Registration note|registration note|Registers?\b)", re.I)


def classify_shared(text: str) -> str:
    """`open` for a defect statement, `done` for a past-tense record (a change the branch already made)."""
    if OPEN_RE.search(text or ""):
        return "open"
    return "done"


DEFECT_CLASSES = (
    ("pragma", r"#?pragma|peephole|exceptions off"),
    ("re-split", r"re-?split|trim|still contains|private cop(y|ies)"),
    ("band-ownership", r"band-ownership|band header|still declared in the band"),
    ("duplicate", r"re-?declares?|declared in (two|both)|defined in (two|\d+)|two (views|units|definitions)|"
                  r"duplicat|conflict|illegal|overload|both sit|second consumer|rule ?1|fold"),
    ("arity", r"arity|wrong arity|takes? .{0,25}arguments?|should take|call site|signature|spelling|"
              r"returned pointer|return the|returns? a"),
    ("owner-move", r"owner'?s? header|move .{0,25}header|should move|follow-?up|declared in the source|"
                   r"move (the|each|it)|to its owner|declared once|declare them once"),
    ("missing-decl", r"no declaration|have no declaration|needs? a declaration|implicit|worth re-?measuring"),
)


def defect_class(text: str) -> str:
    for name, pat in DEFECT_CLASSES:
        if re.search(pat, text or "", re.I):
            return name
    return "other"


def symbols(text: str) -> list[str]:
    """The primary symbols a defect names, so two defects in one header stay two items."""
    return sorted({s.lower() for s in re.findall(r"\b(?:fn_[0-9A-Fa-f]{6,8}|lbl_[0-9A-Za-z_]+)\b", text or "")})[:2]


def norm_defect(text: str, target: str = "") -> str:
    """The normalised summary of a `shared-file` defect: its class, plus the symbols it names.

    Symbols named by the *target itself* (`include/enemy/fn_801251D0.h`) are dropped: the header's own name is
    not the defect's subject, and counting it stopped the same defect filed against it by two lanes from
    collapsing into one item.
    """
    cls = defect_class(text)
    sym = [s for s in symbols(text) if s not in (target or "").lower()]
    return cls + (":" + ",".join(sym) if sym else "")


def fingerprint(text: str, size: int = 8) -> str:
    """A short, stable content hash for a span-less filing.

    A request filed before its address is known carries no span; two such findings must not collapse into one
    content-free item (and one must not swallow the other's content), so the item keys on a fingerprint of
    the free-text evidence instead of a shared empty target.
    """
    return hashlib.sha1(one_line(text).lower().encode("utf-8")).hexdigest()[:size]


def range_flavour(evidence: str) -> tuple[str, str]:
    """A `range` is a re-draw request; a *seam unproven admission* is not a request at all.

    "SEAM UNPROVEN - registered whole as the brief proposed" is the lane recording that the seam was never
    proven and the whole region was registered as a guess. That is an admission, not a request, so it is done
    by default; a genuine re-draw or a data run to claim stays open.
    """
    if re.search(r"seam.{0,30}unproven|unproven.{0,30}seam|registered whole|as the brief proposed",
                 evidence or "", re.I):
        return "admission", "done"
    return "redraw", "open"


def norm_flag_target(lib) -> str:
    return re.sub(r"\s*\(.*", "", asstr(lib)).strip().lower()


def norm_flag_defect(change) -> str:
    ch = asstr(change)
    ch = re.sub(r"\s+", " ", ch).strip().lower()
    return ch


def flag_is_request(change) -> bool:
    """A flag entry whose `change` is "none ..." asks for no flag - the lib already carries what it needs."""
    return not re.match(r"\s*none\b", asstr(change), re.I)


# -----------------------------------------------------------------------------------------------------------
# Items
# -----------------------------------------------------------------------------------------------------------
@dataclass
class Filing:
    source: str   # the outbox file basename / tooling source label - one identity per lane
    lane: str     # the worker (or unit) that filed it
    when: str     # ISO timestamp, "" when unknown
    detail: str   # the raw sentence, kept so a misclassification is visible


@dataclass
class Item:
    kind: str
    target: str
    defect: str
    status: str
    default_status: str
    ask: str
    filings: list[Filing] = field(default_factory=list)
    votes: int = 0          # tooling.py's vote count (0 for a config_request item)
    weight: int = 0         # a lint item's live finding count (0 for every other kind)
    flavour: str = ""
    first: str = ""
    last: str = ""
    age_days: float | None = None

    @property
    def key(self) -> str:
        raw = "%s\x00%s\x00%s" % (self.kind, self.target, self.defect)
        digest = hashlib.sha1(raw.encode("utf-8")).hexdigest()[:6]
        return "%s-%s-%s" % (self.kind, tg.slug("%s %s" % (self.target, self.defect))[:48], digest)

    @property
    def filers(self) -> list[str]:
        seen, out = set(), []
        for f in self.filings:
            if f.source not in seen:
                seen.add(f.source)
                out.append(f.lane or f.source)
        return out

    @property
    def filer_count(self) -> int:
        return len({f.source for f in self.filings})


def _merge(items: dict, key: tuple, item: Item) -> None:
    if key in items:
        items[key].filings.extend(item.filings)
        return
    items[key] = item


def build_items(outbox_dir: str, notes_dir: str, tooling_register: str = "",
                statuses: dict[str, str] | None = None,
                tooling_statuses: dict[str, str] | None = None,
                lint_items: list[Item] | None = None) -> list[Item]:
    """Aggregate all three sources into one de-duplicated register, then apply persisted statuses.

    `tooling_register` is the tracked `docs/tooling-requests.md`; its statuses are read from it when
    `tooling_statuses` is not supplied. `lint_items` is the stylelint source (one item per file with
    rule-7/rule-2 findings), built by `collect_lint_items`; it is `None` when a caller has no tree to lint.
    """
    statuses = statuses or {}
    if tooling_statuses is None:
        tooling_statuses = (tg.parse_statuses(read(tooling_register))
                            if tooling_register and os.path.exists(tooling_register) else {})
    grouped: dict[tuple, Item] = {}

    for path in sorted(glob.glob(os.path.join(outbox_dir, "*.json"))):
        try:
            d = json.loads(read(path))
        except (OSError, json.JSONDecodeError):
            continue
        if not isinstance(d, dict):
            continue
        source = os.path.basename(path)
        lane = str(d.get("worker") or d.get("unit") or source)
        when = str(d.get("finished_at") or "")
        for r in (d.get("config_requests") or []):
            if not isinstance(r, dict):
                continue
            _add_request(grouped, r, source, lane, when)

    # The tooling register is read, never rebuilt: carry its entries and their statuses.
    tooling_statuses = tooling_statuses if tooling_statuses is not None else {}
    try:
        sources = tg.load_sources(outbox_dir, notes_dir)
        entries, _ = tg.build_entries(sources, tooling_statuses)
    except Exception:  # a malformed note must not take the whole register down
        entries = []
    for e in entries:
        filings = [Filing(source="tooling:" + ev.voter, lane=ev.label, when="", detail=ev.snippet)
                   for ev in e.evidence]
        item = Item(kind="tooling", target=e.key, defect="", ask=e.ask, status=e.status,
                    default_status=e.status, filings=filings, votes=e.votes)
        grouped[("tooling", "tooling:" + e.key, "")] = item

    # The stylelint source: an already-built item per (kind, file), open until its findings are gone.
    for it in (lint_items or []):
        _merge(grouped, (it.kind, it.target, it.defect), it)

    items = list(grouped.values())
    for it in items:
        it.status = statuses.get(it.key, it.default_status)
    for it in items:
        it.first = min((f.when for f in it.filings if f.when), default="")
        it.last = max((f.when for f in it.filings if f.when), default="")
    today = _as_of(items)
    for it in items:
        it.age_days = _age(it.first, today)
    return rank(items)


def _add_request(grouped: dict, r: dict, source: str, lane: str, when: str) -> None:
    kind = str(r.get("kind") or "other")
    if kind == "rename":
        return  # not backlog: the landing applies it, so it is done when the batch lands
    if kind == "shared-file":
        target = norm_file(r.get("file"))
        # The content may be under `why` (canonical) or a lane's own `request`/`evidence`/...: read the same
        # free-text list the validator does, so a real shared-file request does not register empty.
        for c in clauses(free_text(r, ("why",))):
            if not c:
                continue
            st = classify_shared(c)
            defect = "record" if st == "done" else norm_defect(c, target)
            key = ("shared-file", target, defect)
            _merge(grouped, key, Item(kind="shared-file", target=target, defect=defect, status=st,
                                      default_status=st, ask=one_line(c),
                                      filings=[Filing(source, lane, when, one_line(c, 400))]))
    elif kind == "range":
        sec = asstr(r.get("section"))
        start, end = asstr(r.get("start")), asstr(r.get("end"))
        evidence = free_text(r, ("evidence", "why"))
        # A range filed with its content in `why`/`request`/... still counts, and a range filed before the
        # span was known keys on a fingerprint of its evidence (the `seam` branch does the same) so two
        # span-less ranges cannot collapse into one empty-target item that swallows a lane's finding.
        span = "-".join(x for x in (start, end) if x)
        target = " ".join(x for x in (sec, span) if x).strip() or ("range@%s" % fingerprint(evidence))
        flavour, st = range_flavour(evidence)
        key = ("range", target.lower(), flavour)
        _merge(grouped, key, Item(kind="range", target=target, defect=flavour, status=st,
                                  default_status=st, ask=one_line(evidence),
                                  flavour=flavour,
                                  filings=[Filing(source, lane, when, one_line(evidence, 400))]))
    elif kind == "seam":
        # A seam finding: a code span whose boundary is wrong, so the split needs re-drawing. Its own kind
        # rather than folded into `range`, because the ask is different - `range` claims a data run, this
        # says the cut is in the wrong place - and two lanes filed it independently before it existed.
        sec = asstr(r.get("section"))
        start, end = asstr(r.get("start")), asstr(r.get("end"))
        evidence = free_text(r, ("evidence", "why"))
        # A seam filed before the kind existed carries no span; fall back to a fingerprint of its evidence so
        # two span-less findings do not collapse into one item with an empty target.
        span = "-".join(x for x in (start, end) if x)
        target = " ".join(x for x in (sec, span) if x).strip() or norm_defect(evidence, "seam")
        key = ("seam", target.lower(), norm_defect(evidence, target))
        _merge(grouped, key, Item(kind="seam", target=target, defect="seam", status="open",
                                  default_status="open", ask=one_line(evidence),
                                  filings=[Filing(source, lane, when, one_line(evidence, 400))]))
    elif kind == "flag":
        lib = norm_flag_target(r.get("lib"))
        change = norm_flag_defect(r.get("change"))
        evidence = free_text(r, ("evidence", "why"))
        request = flag_is_request(r.get("change"))
        st = "open" if request else "done"
        key = ("flag", lib, change or "no-request")
        ask = one_line(asstr(r.get("change")) or evidence) or (lib or "?")
        _merge(grouped, key, Item(kind="flag", target=lib or "?", defect=change or "no-request",
                                  status=st, default_status=st, ask=ask,
                                  filings=[Filing(source, lane, when,
                                                  one_line(evidence or asstr(r.get("change")), 400))]))
    else:
        # `tooling` and anything else the schema drifts into: include it, default open, but never a rename.
        target = norm_file(r.get("file") or r.get("target") or r.get("what") or kind)
        detail = free_text(r, ("why", "evidence", "request", "change", "note"))
        st = "done" if kind == "done-in-this-fold" else "open"
        key = (kind, target, "other")
        _merge(grouped, key, Item(kind=kind, target=target, defect="other", status=st, default_status=st,
                                  ask=one_line(detail) or kind,
                                  filings=[Filing(source, lane, when, one_line(detail, 400))]))


def stylelint_findings(main: str) -> list[dict]:
    """`stylelint.lint_all` for `main`, or `[]` when there is no `src/` tree to lint.

    A fixture tree in a self-test has no `src/`, and a lint failure must never take the other sources down
    with it: this third source is additive, so an import/parse error leaves an empty cell rather than
    crashing a `queue.py next` refusal.
    """
    if not os.path.isdir(os.path.join(main, "src")):
        return []
    try:
        from units import stylelint as sl
        return sl.lint_all(main)
    except Exception:
        return []


def lint_counts(findings: list[dict]) -> dict:
    """`{(kind, file): n}` for the lint findings the backlog is filed against (rules 7, 2 and 11)."""
    out: dict = {}
    for f in findings:
        kind = {7: "naming", 2: "band-header", 11: "untyped"}.get(f["rule"])
        if kind:
            key = (kind, f["file"])
            out[key] = out.get(key, 0) + 1
    return out


def _prior_lint_items(register: dict) -> dict:
    """The `(kind, file)` lint items the published register already carries, keyed for carry-forward.

    A lint item must survive the regeneration that follows its fix: the findings are gone, but the item is
    still the `triage` subject and the ledger's unit of credit. The published payload's `items` list is the
    only place that remembers it between runs; `statuses` alone would remember the status but not the ask.
    """
    out: dict = {}
    for it in (register or {}).get("items", []):
        if isinstance(it, dict) and it.get("kind") in LINT_KINDS:
            out[(it["kind"], it.get("target", ""))] = it
    return out


def collect_lint_items(main: str, register: str | None = None) -> list[Item]:
    """The third source: the lint's rule-7/rule-2 findings as one item per (file, rule).

    Current findings create items; `_prior_lint_items` carries the already-published ones forward so a file
    whose findings were fixed is not silently dropped - its item stays for `triage` to prove done (and, once
    done, stays in the register and earns the credit). The item's key is the (kind, file) pair with a stable
    `defect` (`rule 7` / `rule 2`), so a partial fix keeps its status. `weight` is the live finding count, so
    `rank` surfaces the high-traffic file first; a zero-count carry-forward keeps the last known count in its
    ask for the human reading it, but weighs nothing.
    """
    counts = lint_counts(stylelint_findings(main))
    prior = _prior_lint_items(load_register(main, register))
    items: list[Item] = []
    for key in sorted(set(counts) | set(prior)):
        kind, target = key
        live = counts.get(key, 0)
        last = int((prior.get(key) or {}).get("weight") or 0)
        rule = LINT_RULES[kind]
        if kind == "naming":
            ask = ("`%s` carries %d rule-7 finding(s) (auto-generated `fn_`/`lbl_`/`loc_` or bare `unk*` "
                   "names) - name each from what it does or holds and rename the map row in the same "
                   "change" % (target, live or last))
        elif kind == "untyped":
            ask = ("`%s` carries %d rule-11 finding(s) (a `void *` parameter or return type) - name the "
                   "real type at every call site, or mark the declaration `/* untyped: <byte range|opaque "
                   "handle|caller-owned payload> */` with the case that makes it genuinely untyped"
                   % (target, live or last))
        else:
            ask = ("`%s` carries %d rule-2 finding(s) (an `extern` declared where it is not owned) - move "
                   "each declaration to its owner's header (or `include/unsplit/`) and #include it"
                   % (target, live or last))
        items.append(Item(kind=kind, target=target, defect="rule %d" % rule, status="open",
                          default_status="open", ask=ask, weight=live,
                          filings=[Filing(source="stylelint", lane="stylelint", when="", detail=ask)]))
    return items


def rank(items: list[Item]) -> list[Item]:
    """Filers first (the priority signal), then the item's weight, then recency; open before closed.

    The weight is a `naming`/`band-header` item's live finding count, so the high-traffic file surfaces
    first even though it carries no filing date; every other kind weighs 0, which leaves their existing
    filer/recency/vote order untouched. `votes` (`tooling.py`'s) stays the last tie-break.
    """
    order = {"open": 0, "parked": 1, "done": 2}
    return sorted(items, key=lambda it: (order.get(it.status, 3), -it.filer_count, -it.weight,
                                         _neg(it.last), -it.votes, it.kind, it.target, it.defect))


def _neg(iso: str) -> float:
    return float("inf") if not iso else -_ts(iso)


def _ts(iso: str) -> float:
    try:
        return _dt.datetime.fromisoformat(iso.replace("Z", "+00:00")).timestamp()
    except (ValueError, TypeError):
        return 0.0


def _as_of(items: list[Item]) -> str:
    """The register's reference time: the newest filing in the sources, so the output is deterministic."""
    return max((it.last for it in items if it.last), default="")


def _age(first: str, as_of: str) -> float | None:
    if not first or not as_of:
        return None
    return max(0.0, (_ts(as_of) - _ts(first)) / 86400.0)


# -----------------------------------------------------------------------------------------------------------
# Persistence (`.pi/backlog.json`, gitignored, the way `claims.json` is)
# -----------------------------------------------------------------------------------------------------------
def register_path(main: str) -> str:
    return os.path.join(main, ".pi", "backlog.json")


def outbox_dir(main: str) -> str:
    return os.path.join(main, ".pi", "outbox")


def notes_dir(main: str) -> str:
    return os.path.join(main, ".pi", "notes")


def tooling_register_path(main: str) -> str:
    return os.path.join(main, "docs", "tooling-requests.md")


def load_register(main: str, register: str | None = None) -> dict:
    """The persisted `.pi/backlog.json` as a dict, or `{}` when missing/unreadable."""
    path = register or register_path(main)
    if not os.path.exists(path):
        return {}
    try:
        d = json.loads(read(path))
    except json.JSONDecodeError:
        return {}
    return d if isinstance(d, dict) else {}


def load_statuses(main: str, register: str | None = None) -> dict[str, str]:
    st = load_register(main, register).get("statuses")
    return dict(st) if isinstance(st, dict) else {}


# -----------------------------------------------------------------------------------------------------------
# The credit ledger: one resolved backlog item per new proposal claim.
#
# Earnings are DERIVED from the item statuses - never a stored counter - so the rule cannot silently drift
# and never depends on the register file surviving a clean checkout. `parked` earns nothing: parking removes
# a ghost, it does not buy a claim. Only the claims handed out cannot be derived from the statuses, so they
# are the one thing persisted.
#
# A claim is FREE while the register has no open items (owner's ruling, 2026-09-27): the ledger rations
# against known backlog work, and with nothing to fix there is nothing to ration. Charging anyway would let a
# clean stretch accrue negative credit and then demand catch-up resolutions the day items reappeared. Free
# claims are still counted, so the ledger shows everything that was handed out - nothing is hidden.
# -----------------------------------------------------------------------------------------------------------
BASE_CREDITS = 1
RATIO_DEFAULT = 1


def load_ledger(main: str, register: str | None = None) -> dict:
    """The persisted ledger: `{"claims": [...], "ratio": K, "free": N}` (empty when absent)."""
    raw = load_register(main, register).get("ledger")
    if not isinstance(raw, dict):
        return {"claims": [], "ratio": RATIO_DEFAULT, "free": 0}
    claims = raw.get("claims")
    free = raw.get("free")
    return {"claims": list(claims) if isinstance(claims, list) else [],
            "ratio": raw.get("ratio", RATIO_DEFAULT),
            "free": int(free) if isinstance(free, int) and free > 0 else 0}


def ledger_earned(items: list[Item]) -> int:
    """Resolutions: items a lane closed. A source-default `done` (a record, an admission, a no-request flag)
    is done by definition, not a resolution, so it earns nothing and cannot inflate the campaign's credit."""
    return sum(1 for it in items if it.status == "done" and it.default_status != "done")


def ledger_summary(items: list[Item], claims, ratio: int = RATIO_DEFAULT, free: int = 0) -> dict:
    """The balance, with its derivation, so every refusal and every summary can show the rule rather than
    leave it to be inferred: `balance = base + earned - ratio * spent`. `free` counts the claims handed out
    while the register was clean - they cost nothing, but they are still reported."""
    earned = ledger_earned(items)
    spent = len(claims or [])
    return {"base": BASE_CREDITS, "earned": earned, "spent": spent, "ratio": ratio, "free": free,
            "cost": ratio, "balance": BASE_CREDITS + earned - ratio * spent,
            "claims": list(claims or [])}


def ledger_line(summary: dict, prefix: str = "credits") -> str:
    """One human line for the balance, used by `--print` and the `queue.py next` refusal."""
    line = ("%s: balance %d  (base %d + %d resolved - %d x %d claim(s) handed out)"
            % (prefix, summary["balance"], summary["base"], summary["earned"],
               summary["ratio"], summary["spent"]))
    if summary.get("free"):
        line += "; %d claim(s) free while the register was clean" % summary["free"]
    return line


def payload(items: list[Item], as_of: str, counts: dict, ledger=None) -> dict:
    ledger = ledger if isinstance(ledger, dict) else {}
    claims = ledger.get("claims") or []
    ratio = ledger.get("ratio", RATIO_DEFAULT)
    summary = ledger_summary(items, claims, ratio, free=int(ledger.get("free") or 0))
    return {
        "version": 1,
        "as_of": as_of,
        "counts": counts,
        "ledger": {"claims": list(claims), "ratio": ratio, "base": summary["base"],
                   "earned": summary["earned"], "spent": summary["spent"],
                   "free": summary["free"], "balance": summary["balance"]},
        "items": [
            {
                "key": it.key,
                "kind": it.kind,
                "target": it.target,
                "defect": it.defect,
                "flavour": it.flavour,
                "status": it.status,
                "default_status": it.default_status,
                "ask": it.ask,
                "filers": it.filers,
                "filer_count": it.filer_count,
                "votes": it.votes,
                "weight": it.weight,
                "first": it.first,
                "last": it.last,
                "age_days": None if it.age_days is None else round(it.age_days, 2),
                "filings": [{"source": f.source, "lane": f.lane, "when": f.when, "detail": f.detail}
                            for f in it.filings],
            }
            for it in items
        ],
        "statuses": {it.key: it.status for it in items if it.status != it.default_status},
    }


def canonical(p: dict) -> str:
    return json.dumps(p, sort_keys=True, ensure_ascii=False)


def write_atomic(path: str, text: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=os.path.dirname(path), prefix=".backlog-", suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)


# -----------------------------------------------------------------------------------------------------------
# Build / load
# -----------------------------------------------------------------------------------------------------------
def build(main: str, outbox: str | None = None, notes: str | None = None,
          tooling_register: str | None = None, register: str | None = None) -> tuple[list[Item], dict]:
    outbox = outbox or outbox_dir(main)
    notes = notes or notes_dir(main)
    tooling_register = tooling_register or tooling_register_path(main)
    statuses = load_statuses(main, register)
    ledger = load_ledger(main, register)
    items = build_items(outbox, notes, tooling_register, statuses,
                        lint_items=collect_lint_items(main, register))
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    return items, {"as_of": as_of, "counts": counts, "ledger": ledger,
                   "summary": ledger_summary(items, ledger["claims"], ledger["ratio"])}


def open_items(main: str, **kw) -> list[Item]:
    items, _ = build(main, **kw)
    return [i for i in items if i.status == "open"]


def write_register(main: str, items: list[Item], as_of: str, counts: dict, ledger: dict,
                   register: str | None = None) -> None:
    path = register or register_path(main)
    write_atomic(path, json.dumps(payload(items, as_of, counts, ledger), indent=1,
                                  ensure_ascii=False, sort_keys=True) + "\n")


def record_claims(main: str, claims: list[dict], ratio: int = RATIO_DEFAULT,
                  register: str | None = None, **kw) -> dict:
    """Append handed-out claims to the persisted ledger and rewrite the register. Returns the new summary.

    Only a *real* claim is recorded (`queue.py next` calls this for a non-dry run); a `--dry-run` and the
    deliberate `--ignore-backlog` override hand out work without spending, so they never call it.

    A claim is **free while the register has no open items** (owner, 2026-09-27): there is nothing to ration
    against, so charging would let a clean stretch accrue negative credit and then demand catch-up
    resolutions the day items reappeared. A free claim is counted in `ledger["free"]` instead, so the ledger
    still accounts for every claim handed out.
    """
    items, meta = build(main, register=register, **kw)
    ledger = meta["ledger"]
    ledger["ratio"] = ratio
    if any(it.status == "open" for it in items):
        ledger["claims"].extend(claims)
    else:
        ledger["free"] = int(ledger.get("free") or 0) + len(claims)
    write_register(main, items, meta["as_of"], meta["counts"], ledger, register)
    return ledger_summary(items, ledger["claims"], ratio, free=ledger.get("free", 0))


def lane_task(main: str, item: Item) -> dict:
    """A ready-to-paste lane for a backlog item - mirroring how `queue.py next` prints its spawn line."""
    profile = {"shared-file": "fixer", "range": "decompiler", "flag": "fixer",
               "naming": "fixer", "band-header": "fixer", "untyped": "fixer",
               "tooling": "worker"}.get(item.kind, "worker")
    task = ("Work the campaign backlog item `%s` (%s %s): %s. "
            "This is on the backlog, so `queue.py next` spends a credit on a new proposal claim until it is "
            "resolved or parked. Do the work, then mark it: `python tools/units/backlog.py --set-status %s done` "
            "(or `parked` with a reason - but `parked` earns no credit). The filing lanes' raw evidence is in %s. "
            "Commit on your own branch; end your turn with your report - your final message is the result the "
            "orchestrator receives."
            % (item.key, item.kind, item.target, item.ask, item.key,
               register_path(main).replace("\\", "/")))
    return {"agent": profile, "name": "%s-backlog-%s" % (profile, item.key[:32]),
            "cwd": main.replace("\\", "/"), "task": task,
            "call": "subagent(agent=\"%s\", cwd=\"%s\", task=%s)"
                    % (profile, main.replace("\\", "/"), json.dumps(task))}


def refusal(main: str, top: int = 3, ratio: int = RATIO_DEFAULT, wants: int = 1, **kw) -> str | None:
    """The `queue.py next` refusal when the credit balance does not cover `wants` claim(s).

    Returns `None` - hand out work normally - when there is **no open backlog** (there is nothing to ration
    against, so the campaign must not halt) or when the balance covers the claim(s). When it does not, the
    message prints the balance with its derivation and the top item(s) with a ready-to-paste lane.
    """
    items, meta = build(main, **kw)
    summary = ledger_summary(items, meta["ledger"]["claims"], ratio,
                             free=int(meta["ledger"].get("free") or 0))
    open_ = [i for i in items if i.status == "open"]
    if not open_:
        return None
    cost = ratio * max(1, wants)
    if summary["balance"] >= cost:
        return None
    need = "%d credit(s)" % cost if cost != 1 else "1 credit"
    lines = ["backlog: a claim needs %s, balance is %d - resolve a backlog item (a `done` earns 1 credit) "
             "before handing out another" % (need, summary["balance"]),
             "  %s" % ledger_line(summary),
             "  `parked` earns no credit: parking removes a ghost, it does not buy a claim; only `done` does.",
             "  %d open item(s) - top %d shown; `--ignore-backlog` hands out work without spending on purpose"
             % (len(open_), min(top, len(open_)))]
    for n, it in enumerate(open_[:top], 1):
        age = "" if it.age_days is None else "  %.0fd open" % it.age_days
        lines.append("  %d. %d filer(s)%s  %s" % (n, it.filer_count, age, it.kind))
        lines.append("     %s: %s" % (it.target, one_line(it.ask, 220)))
        who = ", ".join(it.filers[:6]) + ("" if len(it.filers) <= 6 else ", ...")
        lines.append("     filed by: %s" % who)
    lane = lane_task(main, open_[0])
    lines.append("")
    lines.append("top item's lane (paste whole):")
    lines.append("  agent: %s" % lane["agent"])
    lines.append("  cwd:   %s" % lane["cwd"])
    lines.append("  task:  %s" % lane["task"])
    lines.append("")
    lines.append("  %s" % lane["call"])
    lines.append("")
    lines.append("remedy: resolve or park each item with `python tools/units/backlog.py --set-status <KEY> "
                 "done|parked`, then rerun; see `python tools/units/backlog.py --print`")
    return "\n".join(lines)


# -----------------------------------------------------------------------------------------------------------
# Triage: classify every OPEN item as `resolved` / `stale` / `open`, with the check that proved it.
#
# The rule is evidence, never a guess: an item is `resolved` only when the repository shows the defect is gone
# (its span is registered, its flag is present, its file no longer names the symbol, its `#pragma` is gone),
# `stale` only when the artifact it was filed against no longer exists, and otherwise `open`. An item whose
# evidence is prose - a tooling request, a defect with no extractable predicate - stays `open (no check)`.
# -----------------------------------------------------------------------------------------------------------
TRIAGE_DECISIONS = ("resolved", "stale", "open")


def _norm_section(s: str) -> str:
    return re.sub(r"^[.\s]+", "", (s or "").strip()).lower()


def _bracket_rhs(text: str, name: str) -> str | None:
    """The text inside `name = [ ... ]`, honouring nesting, or None when the assignment is absent."""
    m = re.search(r"^%s\s*=\s*\[" % re.escape(name), text, re.M)
    if not m:
        return None
    i = text.index("[", m.start())
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "[":
            depth += 1
        elif text[j] == "]":
            depth -= 1
            if depth == 0:
                return text[i + 1:j]
    return None


def _cflags_groups(main: str) -> tuple[str, dict]:
    """`configure.py`'s text and every `cflags_* = [...]` RHS, for the flag checks."""
    text = read(os.path.join(main, "configure.py"))
    raw = {}
    for m in re.finditer(r"^(cflags_\w+)\s*=\s*\[", text, re.M):
        rhs = _bracket_rhs(text, m.group(1))
        if rhs is not None:
            raw[m.group(1)] = rhs
    return text, raw


def _resolve_group(text: str, raw: dict, name: str, seen=None) -> list[str]:
    """A cflags group as an ordered token list, resolving `*cflags_*` spreads and the `[f for f in ...]`
    filters the file uses. The tokens are the quoted list elements, so a flag written as `"-opt nopeephole"`
    is one token while `"-func_align", "4"` is two - membership is tested on the joined string."""
    seen = set(seen or ())
    if name in seen:
        return []
    seen.add(name)
    rhs = raw.get(name)
    if rhs is None:
        return []
    rhs = re.sub(r"#.*", "", rhs)
    out: list[str] = []
    pat = re.compile(r"\*?\[f for f in (cflags_\w+) if f (?:not in \(([^)]*)\)|!=\s*(\"[^\"]*\"))\]"
                     r"|\*(cflags_\w+)|\"([^\"]*)\"")
    for m in pat.finditer(rhs):
        if m.group(1):
            base = _resolve_group(text, raw, m.group(1), seen)
            if m.group(2) is not None:
                excl = set(re.findall(r'"([^"]*)"', m.group(2)))
                out.extend(t for t in base if t not in excl)
            else:
                out.extend(t for t in base if t != m.group(3))
        elif m.group(4):
            out.extend(_resolve_group(text, raw, m.group(4), seen))
        elif m.group(5) is not None:
            out.append(m.group(5))
    return out


def _lib_groups(text: str) -> dict:
    """lib name -> its cflags group name, from both the dict entries and the `DolphinLib`/`Rel` helpers."""
    libs: dict = {}
    for m in re.finditer(r'"lib"\s*:\s*"([^"]+)"', text):
        seg = text[m.end():m.end() + 4000]
        cg = re.search(r'"cflags"\s*:\s*(cflags_\w+)', seg)
        libs[m.group(1)] = cg.group(1) if cg else None
    for helper, grp in (("DolphinLib", "cflags_base"), ("Rel", "cflags_rel")):
        for m in re.finditer(re.escape(helper) + r'\("([^"]+)"', text):
            libs[m.group(1)] = grp
    return libs


_FLAG_PART_RE = re.compile(r"-[A-Za-z_]\w*(?:\s+[A-Za-z0-9,_=\.]+)?$")
_ARROW_RE = re.compile(r"->\s*(\S.*)$")
_REPLACE_RE = re.compile(r"replace\s+(\S.*?)\s+with\s+(\S.*)$", re.I)


def flag_specs(defect: str) -> list[str] | None:
    """The flag(s) a defect requests, or `None` when it is prose and cannot be checked. A trailing
    parenthetical explanation is dropped; `X -> Y` and `replace X with Y` keep `Y`; `A and B` is two specs.
    Anything that is not purely flag tokens returns `None` - the point is to never guess."""
    s = (defect or "").strip()
    if not s:
        return None
    if s.startswith("["):
        try:
            v = json.loads(s)
        except (ValueError, TypeError):
            return None
        if isinstance(v, list) and v and all(isinstance(x, str) for x in v):
            return [" ".join(v)]
        return None
    s = re.sub(r"\([^()]*\)", " ", s).replace("`", "")
    s = re.sub(r"^\s*--\s*", "", s)
    m = _ARROW_RE.search(s)
    if m:
        s = m.group(1)
    m = _REPLACE_RE.search(s)
    if m:
        s = m.group(2)
    specs = []
    for part in re.split(r"\s+and\s+", s):
        part = part.strip().strip(",").strip()
        if not _FLAG_PART_RE.match(part):
            return None
        specs.append(part)
    return specs or None


def _flag_present(tokens: list[str], spec: str) -> bool:
    hay = " ".join(re.sub(r"\s+", " ", t).strip() for t in tokens)
    return re.search(r"(?<![\w-])" + re.escape(spec) + r"(?![\w-])", hay) is not None


def _splits_ranges(main: str) -> dict:
    """`section -> [(start, end, unit)]` from `config/RMHE08/splits.txt` - the registration test."""
    path = os.path.join(main, "config", "RMHE08", "splits.txt")
    out: dict = {}
    if not os.path.exists(path):
        return out
    unit = ""
    for line in read(path).replace("\r\n", "\n").split("\n"):
        if not line.strip():
            continue
        if not line[0].isspace():
            unit = line.rstrip().rstrip(":") if line.rstrip().endswith(":") else unit
            continue
        m = re.match(r"\s+(\S+)\s+start:0x([0-9A-Fa-f]+)\s+end:0x([0-9A-Fa-f]+)", line)
        if m and unit:
            out.setdefault(_norm_section(m.group(1)), []).append(
                (int(m.group(2), 16), int(m.group(3), 16), unit))
    return out


def _parse_span(target: str):
    """`(section, start, end)` or None. `0x` hex and bare decimal parse; a `xx` placeholder does not - an
    unparseable span is unprovable, not resolved."""
    m = re.match(r"^\s*(\S+)\s+(\S+)\s*-\s*(\S+)\s*$", target or "")
    if not m:
        return None

    def _addr(v):
        v = v.strip()
        if re.fullmatch(r"0[xX][0-9A-Fa-f]+", v):
            return int(v, 16)
        if re.fullmatch(r"\d+", v):
            return int(v)
        return None

    s, e = _addr(m.group(2)), _addr(m.group(3))
    if s is None or e is None:
        return None
    return _norm_section(m.group(1)), s, e


def _coverage(ivals: list, start: int, end: int) -> tuple[bool, list[str]]:
    """Whether `[start,end)` is fully covered by the `(s,e,unit)` intervals, and which units touch it."""
    hits = sorted({u for s, e, u in ivals if s < end and e > start})
    spans = sorted((max(s, start), min(e, end)) for s, e, u in ivals if s < end and e > start)
    cur, total = start, 0
    for s, e in spans:
        if s > cur:
            break
        if e > cur:
            total += e - cur
            cur = e
        if cur >= end:
            break
    return (cur >= end and total >= end - start), hits


def _span_decision(splits: dict, target: str):
    span = _parse_span(target)
    if not span:
        return ("open", "no check: %r is not a parseable <section> <start>-<end> span" % one_line(target, 60))
    sec, s, e = span
    if s >= e:
        return ("open", "no check: empty or inverted span 0x%X-0x%X" % (s, e))
    covered, hits = _coverage(splits.get(sec, []), s, e)
    if covered:
        shown = ", ".join(hits[:3]) + ("" if len(hits) <= 3 else ", ...")
        return ("resolved", "span 0x%X-0x%X is now claimed by %s in splits.txt" % (s, e, shown))
    return ("open", "span 0x%X-0x%X is still unclaimed in %s (splits.txt)" % (s, e, sec))


def _resolve_repo_path(main: str, rel: str) -> str | None:
    """The on-disk path for a (possibly lowercased) repo-relative path, or None when it does not exist.

    The register lowercases targets (`norm_file`), so `config/rmhe08/splits.txt` has to resolve against the
    real `config/RMHE08/splits.txt` - matched case-insensitively, component by component."""
    rel = (rel or "").strip().strip("`").strip("/")
    if not rel:
        return None
    direct = os.path.join(main, *rel.split("/"))
    if os.path.exists(direct):
        return direct
    cur = main
    for part in rel.split("/"):
        if not os.path.isdir(cur):
            return None
        match = [e for e in os.listdir(cur) if e.lower() == part.lower()]
        if not match:
            return None
        cur = os.path.join(cur, match[0])
    return cur if os.path.exists(cur) else None


_PATH_LIKE = re.compile(r"^(?:[^/\s]+/)*[^/\s]+\.[A-Za-z0-9]+$")


def _shared_targets(item: Item) -> list[str]:
    """The repo paths a shared-file item names, split on the filers' connectors (`,`, ` + `, ` and `)."""
    parts = re.split(r"\s*(?:,|\+| and )\s*", item.target or "")
    return [t.strip().strip("`") for t in parts if t.strip()] if item.target else []


def _path_like(t: str) -> bool:
    return bool(_PATH_LIKE.match((t or "").strip()))


def _filing_text(item: Item) -> str:
    return (item.ask or "") + " " + " ".join(f.detail or "" for f in item.filings)


def _pragma_specs(item: Item) -> list[str]:
    """`peephole off` etc. named by a `#pragma` defect, normalised and de-duplicated."""
    out, seen = [], set()
    for m in re.finditer(r"#?\s*pragma\s+([A-Za-z_]+)(?:\s+(off|on))?", _filing_text(item), re.I):
        spec = " ".join(x.lower() for x in m.groups() if x)
        if spec not in seen:
            seen.add(spec)
            out.append(spec)
    return out


def _has_pragma(path: str, spec: str) -> bool:
    words = [re.escape(w) for w in spec.split()]
    return re.search(r"#\s*pragma\s+" + r"\s+".join(words), read(path), re.I) is not None


def _mentions(path: str, sym: str) -> bool:
    return re.search(r"(?<![\w])" + re.escape(sym) + r"(?![\w])", read(path)) is not None


def _decl_symbols(item: Item) -> list[str]:
    """The `fn_*`/`lbl_*` symbols a defect's own backticked declaration text names, excluding paths."""
    syms = set()
    for span in re.findall(r"`([^`]+)`", _filing_text(item)):
        if "/" in span or re.search(r"\.(?:cpp|h|c|txt|md)\b", span):
            continue
        syms.update(re.findall(r"\b(fn_[0-9A-Fa-f]{6,8}|lbl_[0-9A-Za-z_]+)\b", span))
    return sorted(syms)


def _check_shared(main: str, item: Item, ctx: dict):
    paths = [t for t in _shared_targets(item) if _path_like(t)]
    if not paths:
        return ("open", "no check: target %r is not a clean repo path" % one_line(item.target, 50))
    found = [(t, p) for t in paths if (p := _resolve_repo_path(main, t))]
    if not found:
        return ("stale", "none of the %d path(s) still exist: %s"
                % (len(paths), ", ".join(paths[:4]) + (" ..." if len(paths) > 4 else "")))
    cls = (item.defect or "").split(":")[0]
    if cls == "pragma":
        specs = _pragma_specs(item)
        if not specs:
            return ("open", "no check: a pragma defect with no extractable `#pragma` in its filing text")
        for spec in specs:
            for t, p in found:
                if _has_pragma(p, spec):
                    return ("open", "%s still carries `#pragma %s`" % (t, spec))
        return ("resolved", "the `#pragma %s` is gone from %s" % (", ".join(specs), found[0][0]))
    code = [(t, p) for t, p in found if re.search(r"\.(?:h|c|cpp|hpp|cc|hxx|cxx)$", t, re.I)]
    syms = _decl_symbols(item) if code else []
    if syms:
        present = [s for s in syms if any(_mentions(p, s) for _, p in code)]
        if cls == "missing-decl":
            if present:
                return ("resolved", "%s now declares %s" % (code[0][0], ", ".join(present)))
            return ("open", "%s still has no declaration of %s" % (code[0][0], ", ".join(syms)))
        if present:
            return ("open", "%s still names %s" % (code[0][0], ", ".join(present)))
        return ("resolved", "%s no longer names the owned symbol(s) %s" % (code[0][0], ", ".join(syms)))
    return ("open", "no check: shared-file defect %r has no repo-provable predicate"
            % one_line(item.defect or "?", 40))


def _check_flag(main: str, item: Item, ctx: dict):
    specs = flag_specs(item.defect)
    if specs is None:
        return ("open", "no check: flag request %r is not a single measurable flag change"
                % one_line(item.defect, 60))
    lib = (item.target or "").strip()
    group = ctx["libs"].get(lib) or (lib if lib in ctx["groups"] else None)
    if not group:
        return ("open", "no check: no cflags group resolves for lib %r in configure.py" % lib)
    if group not in ctx["resolved"]:
        ctx["resolved"][group] = _resolve_group(ctx["text"], ctx["raw"], group)
    toks = ctx["resolved"][group]
    missing = [s for s in specs if not _flag_present(toks, s)]
    if missing:
        return ("open", "%s still lacks %s in configure.py (%s)"
                % (lib, " and ".join("`%s`" % m for m in missing), group))
    return ("resolved", "%s already carries %s in configure.py (%s)"
            % (lib, " and ".join("`%s`" % s for s in specs), group))


def _check_tooling(main: str, item: Item, ctx: dict):
    """A tooling request is not given a write artifact by this tool, so there is nothing to prove from the
    repository: it stays open and says why rather than guessing from the request's prose."""
    return ("open", "no check: a tooling request has no repo-provable completion artifact "
                    "(tracked in docs/tooling-requests.md)")


def _check_lint(main: str, item: Item, ctx: dict):
    """Whether a lint-derived item's findings are gone - re-linted now, never remembered.

    The count is read from the file itself with `stylelint.lint_source` (rule 11 with `rule11_findings`,
    which has no ownership dependency and sees the unsplit band), the same functions that produced the
    item, so the check cannot drift from the lint. A missing file is `stale`; a rule-2 check needs the
    symbols/splits map, so with the map absent it stays open rather than call itself resolved.
    """
    rule = LINT_RULES.get(item.kind)
    # A lint kind filed as an outbox `config_requests` entry (`kind: naming`) with no file names the *kind* as
    # its target (`norm_file(... or kind)`). It has no file to re-lint: it must stay open, not be called
    # `stale` (which the next `triage --apply` would `park`, closing a real request).
    if not _path_like(item.target):
        return ("open", "no check: %r names no file to re-lint (a bare kind, not a lint finding)" % item.target)
    path = _resolve_repo_path(main, item.target)
    if not path:
        return ("stale", "the file no longer exists: %s" % item.target)
    try:
        from units import stylelint as sl
    except ImportError:  # pragma: no cover - the tool is always beside this module
        return ("open", "no check: stylelint.py is not importable")
    if rule == 2 and "ownership" not in ctx:
        ctx["ownership"] = sl.load_ownership(main)
    ownership = ctx.get("ownership")
    if rule == 2 and ownership is None:
        return ("open", "no check: config/RMHE08/symbols.txt or splits.txt is absent")
    if rule == 11:
        # Rule 11 is source-local and has no ownership dependency, so it is read straight from the file:
        # `lint_source` returns early for the unsplit band (rule 2 only), which would call a band item
        # resolved while its `void *` parameters are still there.
        findings = sl.rule11_findings(sl.Source(path, item.target, read(path)))
    else:
        findings = [f for f in sl.lint_source(sl.Source(path, item.target, read(path)), ownership)
                    if f["rule"] == rule]
    if findings:
        return ("open", "%s still carries %d rule-%d finding(s)"
                % (item.target, len(findings), rule))
    return ("resolved", "rule %d no longer fires in %s (re-linted with the same rule that filed it)"
            % (rule, item.target))


def triage_item(main: str, item: Item, ctx: dict):
    if item.kind == "range":
        return _span_decision(ctx["splits"], item.target)
    if item.kind == "seam":
        return _span_decision(ctx["splits"], item.target)
    if item.kind == "flag":
        return _check_flag(main, item, ctx)
    if item.kind == "shared-file":
        return _check_shared(main, item, ctx)
    if item.kind in LINT_KINDS:
        return _check_lint(main, item, ctx)
    return _check_tooling(main, item, ctx)


def triage(main: str, **kw) -> tuple[list, dict]:
    """Classify every OPEN item; returns `([(item, decision, evidence)], meta)`."""
    items, meta = build(main, **kw)
    text, raw = _cflags_groups(main)
    ctx = {"text": text, "raw": raw, "groups": set(raw), "resolved": {},
           "libs": _lib_groups(text), "splits": _splits_ranges(main)}
    out = []
    for it in items:
        if it.status != "open":
            continue
        try:
            decision, evidence = triage_item(main, it, ctx)
        except Exception as exc:  # a malformed fixture must stay open, never crash the whole run
            decision, evidence = "open", "no check: %s" % exc
        out.append((it, decision, evidence))
    return out, meta


def apply_triage(main: str, decisions: list, register: str | None = None, **kw) -> dict:
    """Write the verdicts: `resolved` -> `done`, `stale` -> `parked`. Idempotent, and an item a human (or a
    previous triage) already set is never flipped. Returns the change counts and the new credit summary."""
    register = register or register_path(main)
    statuses = load_statuses(main, register)
    ledger = load_ledger(main, register)
    changed = {"done": 0, "parked": 0, "open": 0, "skipped": 0}
    for it, decision, _ in decisions:
        if it.key in statuses:                     # an existing non-default status is a human's decision
            changed["skipped"] += 1
            continue
        if decision == "resolved":
            statuses[it.key] = "done"
            changed["done"] += 1
        elif decision == "stale":
            statuses[it.key] = "parked"
            changed["parked"] += 1
        else:
            changed["open"] += 1
    outbox = kw.get("outbox") or outbox_dir(main)
    notes = kw.get("notes") or notes_dir(main)
    tooling_register = kw.get("tooling_register") or tooling_register_path(main)
    items = build_items(outbox, notes, tooling_register, statuses,
                        lint_items=collect_lint_items(main, register))
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    write_register(main, items, _as_of(items), counts, ledger, register)
    return {"changed": changed, "counts": counts,
            "summary": ledger_summary(items, ledger["claims"], ledger["ratio"])}


def triage_report(decisions: list) -> dict:
    """`{decision: n}` overall and `{kind: {decision: n}}` - the before/after arithmetic."""
    counts = {d: 0 for d in TRIAGE_DECISIONS}
    by_kind: dict = {}
    for it, decision, _ in decisions:
        counts[decision] = counts.get(decision, 0) + 1
        by_kind.setdefault(it.kind, {d: 0 for d in TRIAGE_DECISIONS})[decision] += 1
    return {"total": len(decisions), "counts": counts, "by_kind": by_kind}


# -----------------------------------------------------------------------------------------------------------
# Rendering
# -----------------------------------------------------------------------------------------------------------
def print_report(items: list[Item], meta: dict, top: int) -> None:
    c = meta["counts"]
    print("backlog: %d open / %d done / %d parked  (as of %s)"
          % (c["open"], c["done"], c["parked"], meta["as_of"] or "?"))
    if meta.get("summary"):
        print("  %s" % ledger_line(meta["summary"]))
        print("    (`parked` earns no credit - parking removes a ghost; only a resolved `done` buys a claim)")
    print("  rank  filers  age  kind         item")
    for i, it in enumerate(items[:top], 1):
        age = "-" if it.age_days is None else "%.0fd" % it.age_days
        print("  %4d  %5d  %4s  %-11s  %s" % (i, it.filer_count, age, it.kind[:11],
                                               one_line("%s: %s" % (it.target, it.ask), 96)))
    if len(items) > top:
        print("  ... %d more (use --json for all)" % (len(items) - top))


def print_triage(decisions: list, top: int, balance: dict) -> None:
    rep = triage_report(decisions)
    c = rep["counts"]
    print("triage: %d open item(s) -> %d resolved / %d stale / %d open (no check)"
          % (rep["total"], c["resolved"], c["stale"], c["open"]))
    print("  %s" % ledger_line(balance))
    print("  by kind:")
    for kind in sorted(rep["by_kind"]):
        kc = rep["by_kind"][kind]
        print("    %-12s resolved %3d  stale %3d  open %3d"
              % (kind, kc["resolved"], kc["stale"], kc["open"]))
    print("  each decision and the check that proved it:")
    for it, decision, evidence in decisions:
        print("    %-8s %-11s %s" % (decision, it.kind,
                                        one_line("%s: %s" % (it.target, it.ask), 84)))
        print("             %s" % one_line(evidence, 150))
    survivors = [(it, ev) for it, d, ev in decisions if d == "open"]
    survivors.sort(key=lambda t: (-t[0].filer_count, t[0].kind, t[0].target))
    print("  top %d still open:" % min(top, len(survivors)))
    for i, (it, ev) in enumerate(survivors[:top], 1):
        print("    %2d. %-11s %s" % (i, it.kind, one_line("%s: %s" % (it.target, it.ask), 92)))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cmd", nargs="?", choices=["triage"],
                    help="subcommand: `triage` classifies every open item (resolved/stale/open)")
    ap.add_argument("--main", default=None, help="MAIN worktree (default: the first `git worktree list`)")
    ap.add_argument("--outbox", default=None)
    ap.add_argument("--notes", default=None)
    ap.add_argument("--tooling-register", default=None)
    ap.add_argument("--register", default=None)
    ap.add_argument("--ratio", type=int, default=RATIO_DEFAULT,
                    help="credits a proposal claim costs - K backlog items per claim (default 1)")
    ap.add_argument("--json", action="store_true", help="emit the register as JSON on stdout (no write)")
    ap.add_argument("--check", action="store_true", help="exit 1 when the register is missing or stale")
    ap.add_argument("--print", dest="print_only", action="store_true", help="summary only, write nothing")
    ap.add_argument("--top", type=int, default=20)
    ap.add_argument("--apply", action="store_true",
                    help="with `triage`: write the verdicts (resolved -> done, stale -> parked)")
    ap.add_argument("--set-status", nargs=2, metavar=("KEY", "STATUS"),
                    help="set an item's status (open/done/parked) and regenerate")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        import backlog_selftest
        return backlog_selftest.selftest()

    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, OSError):
        pass

    main_wt = args.main or default_main()
    outbox = args.outbox or outbox_dir(main_wt)
    notes = args.notes or notes_dir(main_wt)
    register = args.register or register_path(main_wt)
    tooling_register = args.tooling_register or tooling_register_path(main_wt)

    statuses = load_statuses(main_wt, register)
    ledger = load_ledger(main_wt, register)
    lint = collect_lint_items(main_wt, register)   # the third source: one item per (file, rule)
    if args.set_status:
        key, status = args.set_status
        status = status.lower()
        if status not in STATUSES:
            print("status must be one of: %s" % ", ".join(STATUSES), file=sys.stderr)
            return 2
        known = {it.key for it in build_items(outbox, notes, tooling_register, statuses, lint_items=lint)}
        if key not in known:
            print("unknown backlog key %r - see `python tools/units/backlog.py --print`" % key, file=sys.stderr)
            return 2
        statuses[key] = status

    if args.cmd == "triage":
        decisions, _ = triage(main_wt, outbox=outbox, notes=notes,
                              tooling_register=tooling_register, register=register)
        rep = triage_report(decisions)
        all_items = build_items(outbox, notes, tooling_register, statuses, lint_items=lint)
        balance = ledger_summary(all_items, ledger["claims"], args.ratio)
        if args.json:
            print(json.dumps({"triage": rep, "credits": balance,
                              "decisions": [{"key": it.key, "kind": it.kind, "target": it.target,
                                             "decision": d, "evidence": e} for it, d, e in decisions]},
                             indent=2, ensure_ascii=False))
            return 0
        print_triage(decisions, args.top, balance)
        if args.apply:
            out = apply_triage(main_wt, decisions, register=register, outbox=outbox, notes=notes,
                               tooling_register=tooling_register)
            ch, co = out["changed"], out["counts"]
            print("applied: %d -> done, %d -> parked, %d left open, %d skipped (status already set)"
                  % (ch["done"], ch["parked"], ch["open"], ch["skipped"]))
            print("  register now: %d open / %d done / %d parked"
                  % (co["open"], co["done"], co["parked"]))
            print("  %s" % ledger_line(out["summary"]))
            print("  `parked` earns no credit - only `done` buys a claim; register written to %s" % register)
        else:
            print("dry run: nothing written (pass --apply to mark resolved/stale)")
        return 0

    items = build_items(outbox, notes, tooling_register, statuses, lint_items=lint)
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    meta = {"as_of": as_of, "counts": counts, "ledger": ledger,
            "summary": ledger_summary(items, ledger["claims"], args.ratio)}

    if args.check:
        if not os.path.exists(register):
            print("check: %s does not exist; run the tool" % register, file=sys.stderr)
            return 1
        try:
            stored = json.loads(read(register))
        except json.JSONDecodeError:
            stored = None
        fresh = payload(items, as_of, counts, ledger)
        if stored is None or canonical(stored) != canonical(fresh):
            print("check: %s is stale; run `python tools/units/backlog.py`" % register, file=sys.stderr)
            return 1
        print("check: %s is up to date (%d open / %d items)" % (register, counts["open"], counts["total"]))
        return 0

    if args.json:
        print(json.dumps(payload(items, as_of, counts, ledger), indent=2, ensure_ascii=False))
        return 0

    print_report(items, meta, args.top)
    if not args.print_only:
        write_register(main_wt, items, as_of, counts, ledger, register)
        print("wrote %s (%d open / %d items)" % (register, counts["open"], counts["total"]),
              file=sys.stderr)
    return 0


def default_main() -> str:
    """MAIN - the first worktree `git worktree list` prints - so a run inside a lane worktree reads MAIN."""
    import subprocess
    try:
        p = subprocess.run(["git", "worktree", "list", "--porcelain"], cwd=os.getcwd(),
                           capture_output=True, text=True, encoding="utf-8", errors="replace")
        for line in p.stdout.splitlines():
            if line.startswith("worktree "):
                return line.split(" ", 1)[1]
    except OSError:
        pass
    return os.path.dirname(TOOLS)


if __name__ == "__main__":
    sys.exit(main())
