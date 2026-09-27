#!/usr/bin/env python3
"""One ranked register of everything the campaign filed but has not done - the *backlog*.

The owner's rule (2026-09-27): **backlog first.** While the backlog has open items, work those before
handing out a new proposal claim. That rule was unenforceable because the backlog was invisible: every lane
ends with an outbox `.pi/outbox/*.json` whose `config_requests` list records what it found but was not
allowed to change, and nothing tracked whether any of it was ever done (266 outboxes / 703 requests on
2026-09-27). This tool makes it one register with a lifecycle, and `queue.py next` reads it.

Two sources, one register:

* **`config_requests`** in `.pi/outbox/*.json`. A `range` (a seam re-draw, or a data run to claim) is open
  until a proposal pass re-draws it; a `shared-file` (a defect in a header a worker may not touch) is open
  until fixed; a `flag` (a compiler flag for a lib) is open until measured and adopted or rejected. A
  **`rename` is not backlog** - the landing applies it, so it is done when its batch lands - and is not
  carried at all.
* **`tools/units/tooling.py`**'s own register (`docs/tooling-requests.md`): the ranked tooling/environment
  requests with their open/done/parked statuses. It is read, never duplicated.

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

Statuses live in `MAIN/.pi/backlog.json` (gitignored, the way `claims.json` is), so they survive a
regeneration and can be set with `--set-status KEY STATUS`. The register is regenerated after each change.

    python tools/units/backlog.py                       # regenerate MAIN/.pi/backlog.json + summary
    python tools/units/backlog.py --print [--top N]     # human summary, write nothing
    python tools/units/backlog.py --json                # the register as JSON on stdout (no write)
    python tools/units/backlog.py --check               # exit 1 when the register is missing or stale
    python tools/units/backlog.py --set-status KEY done # open / done / parked, then regenerate
    python tools/units/backlog.py --selftest

Ranking is by what predicts value: the number of independent filers, then recency, then `tooling.py`'s
votes. An item also shows how long it has been open; an item filed in an early phase may be stale because
the code moved on, and that is exactly what the register is for - it is surfaced, never silently dropped,
and a human or lane parks it.
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

STATUSES = ("open", "done", "parked")
NEW_KINDS = ("shared-file", "range", "flag", "tooling")  # default open; a `rename` is never carried

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
                tooling_statuses: dict[str, str] | None = None) -> list[Item]:
    """Aggregate both sources into one de-duplicated register, then apply persisted statuses.

    `tooling_register` is the tracked `docs/tooling-requests.md`; its statuses are read from it when
    `tooling_statuses` is not supplied.
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
        for c in clauses(r.get("why", "")):
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
        target = "%s %s-%s" % (sec, start, end)
        flavour, st = range_flavour(asstr(r.get("evidence")))
        key = ("range", target.lower(), flavour)
        _merge(grouped, key, Item(kind="range", target=target, defect=flavour, status=st,
                                  default_status=st, ask=one_line(asstr(r.get("evidence"))),
                                  flavour=flavour,
                                  filings=[Filing(source, lane, when, one_line(asstr(r.get("evidence")), 400))]))
    elif kind == "flag":
        lib = norm_flag_target(r.get("lib"))
        change = norm_flag_defect(r.get("change"))
        request = flag_is_request(r.get("change"))
        st = "open" if request else "done"
        key = ("flag", lib, change or "no-request")
        ask = one_line(asstr(r.get("change")) or asstr(r.get("evidence"))) or (lib or "?")
        _merge(grouped, key, Item(kind="flag", target=lib or "?", defect=change or "no-request",
                                  status=st, default_status=st, ask=ask,
                                  filings=[Filing(source, lane, when,
                                                  one_line(asstr(r.get("evidence")) or asstr(r.get("change")), 400))]))
    else:
        # `tooling` and anything else the schema drifts into: include it, default open, but never a rename.
        target = norm_file(r.get("file") or r.get("target") or r.get("what") or kind)
        detail = asstr(r.get("why") or r.get("evidence") or r.get("change") or "")
        st = "done" if kind == "done-in-this-fold" else "open"
        key = (kind, target, "other")
        _merge(grouped, key, Item(kind=kind, target=target, defect="other", status=st, default_status=st,
                                  ask=one_line(detail) or kind,
                                  filings=[Filing(source, lane, when, one_line(detail, 400))]))


def rank(items: list[Item]) -> list[Item]:
    """Filers first (the priority signal), then recency, then tooling.py's votes; open before closed."""
    order = {"open": 0, "parked": 1, "done": 2}
    return sorted(items, key=lambda it: (order.get(it.status, 3), -it.filer_count,
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


def load_statuses(main: str, register: str | None = None) -> dict[str, str]:
    path = register or register_path(main)
    if not os.path.exists(path):
        return {}
    try:
        d = json.loads(read(path))
    except json.JSONDecodeError:
        return {}
    st = d.get("statuses")
    return dict(st) if isinstance(st, dict) else {}


def payload(items: list[Item], as_of: str, counts: dict) -> dict:
    return {
        "version": 1,
        "as_of": as_of,
        "counts": counts,
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
    items = build_items(outbox, notes, tooling_register, statuses)
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    return items, {"as_of": as_of, "counts": counts}


def open_items(main: str, **kw) -> list[Item]:
    items, _ = build(main, **kw)
    return [i for i in items if i.status == "open"]


def lane_task(main: str, item: Item) -> dict:
    """A ready-to-paste lane for a backlog item - mirroring how `queue.py next` prints its spawn line."""
    profile = {"shared-file": "fixer", "range": "decompiler", "flag": "fixer",
               "tooling": "worker"}.get(item.kind, "worker")
    task = ("Work the campaign backlog item `%s` (%s %s): %s. "
            "This is on the backlog, so `queue.py next` refuses a new proposal claim until it is resolved or "
            "parked. Do the work, then mark it: `python tools/units/backlog.py --set-status %s done` "
            "(or `parked` with a reason). The filing lanes' raw evidence is in %s. Commit on your own branch; "
            "end your turn with your report - your final message is the result the orchestrator receives."
            % (item.key, item.kind, item.target, item.ask, item.key,
               register_path(main).replace("\\", "/")))
    return {"agent": profile, "name": "%s-backlog-%s" % (profile, item.key[:32]),
            "cwd": main.replace("\\", "/"), "task": task,
            "call": "subagent(agent=\"%s\", cwd=\"%s\", task=%s)"
                    % (profile, main.replace("\\", "/"), json.dumps(task))}


def refusal(main: str, top: int = 3, **kw) -> str | None:
    """The `queue.py next` refusal while the register is open, with the top item(s) and a lane for #1.

    `None` when nothing is open, so the caller hands out work normally.
    """
    items, meta = build(main, **kw)
    open_ = [i for i in items if i.status == "open"]
    if not open_:
        return None
    lines = ["backlog: %d open item(s) - work the backlog before a new proposal claim (top %d shown; "
             "`--ignore-backlog` parks the whole register on purpose)" % (len(open_), min(top, len(open_)))]
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
# Rendering
# -----------------------------------------------------------------------------------------------------------
def print_report(items: list[Item], meta: dict, top: int) -> None:
    c = meta["counts"]
    print("backlog: %d open / %d done / %d parked  (as of %s)"
          % (c["open"], c["done"], c["parked"], meta["as_of"] or "?"))
    print("  rank  filers  age  kind         item")
    for i, it in enumerate(items[:top], 1):
        age = "-" if it.age_days is None else "%.0fd" % it.age_days
        print("  %4d  %5d  %4s  %-11s  %s" % (i, it.filer_count, age, it.kind[:11],
                                               one_line("%s: %s" % (it.target, it.ask), 96)))
    if len(items) > top:
        print("  ... %d more (use --json for all)" % (len(items) - top))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--main", default=None, help="MAIN worktree (default: the first `git worktree list`)")
    ap.add_argument("--outbox", default=None)
    ap.add_argument("--notes", default=None)
    ap.add_argument("--tooling-register", default=None)
    ap.add_argument("--register", default=None)
    ap.add_argument("--json", action="store_true", help="emit the register as JSON on stdout (no write)")
    ap.add_argument("--check", action="store_true", help="exit 1 when the register is missing or stale")
    ap.add_argument("--print", dest="print_only", action="store_true", help="summary only, write nothing")
    ap.add_argument("--top", type=int, default=20)
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
    if args.set_status:
        key, status = args.set_status
        status = status.lower()
        if status not in STATUSES:
            print("status must be one of: %s" % ", ".join(STATUSES), file=sys.stderr)
            return 2
        known = {it.key for it in build_items(outbox, notes, tooling_register, statuses)}
        if key not in known:
            print("unknown backlog key %r - see `python tools/units/backlog.py --print`" % key, file=sys.stderr)
            return 2
        statuses[key] = status

    items = build_items(outbox, notes, tooling_register, statuses)
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    meta = {"as_of": as_of, "counts": counts}

    if args.check:
        if not os.path.exists(register):
            print("check: %s does not exist; run the tool" % register, file=sys.stderr)
            return 1
        try:
            stored = json.loads(read(register))
        except json.JSONDecodeError:
            stored = None
        fresh = payload(items, as_of, counts)
        if stored is None or canonical(stored) != canonical(fresh):
            print("check: %s is stale; run `python tools/units/backlog.py`" % register, file=sys.stderr)
            return 1
        print("check: %s is up to date (%d open / %d items)" % (register, counts["open"], counts["total"]))
        return 0

    if args.json:
        print(json.dumps(payload(items, as_of, counts), indent=2, ensure_ascii=False))
        return 0

    print_report(items, meta, args.top)
    if not args.print_only:
        write_atomic(register, json.dumps(payload(items, as_of, counts), indent=1,
                                          ensure_ascii=False, sort_keys=True) + "\n")
        print("wrote %s (%d open / %d items)" % (register, counts["open"], counts["total"]),
              file=sys.stderr)
    return 0


def default_main() -> str:
    """MAIN - the first worktree `git worktree list` prints - so a run inside a lane worktree reads MAIN."""
    import subprocess
    try:
        p = subprocess.run(["git", "worktree", "list", "--porcelain"], cwd=os.getcwd(),
                           capture_output=True, text=True, errors="replace")
        for line in p.stdout.splitlines():
            if line.startswith("worktree "):
                return line.split(" ", 1)[1]
    except OSError:
        pass
    return os.path.dirname(TOOLS)


if __name__ == "__main__":
    sys.exit(main())
