#!/usr/bin/env python3
"""One ranked register of everything the campaign filed but has not done (the backlog). Spec: docs/tools/spec/backlog.md.
CLI: backlog.py [--print [--top N] | --json | --check | --set-status KEY open|done|parked | triage | --selftest]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

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
from tools.lib import text as libtext

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)

from tools.lib.lanes import launch as lanecmd  # the launch line
from tools.lib import project as _project  # the configure / splits readers
from tools.units import tooling as tg  # the second source: its register is read, not rebuilt
from tools.lib import outbox as _outbox  # the outbox schema: FREE_TEXT_FIELDS is one definition

STATUSES = ("open", "done", "parked")
NEW_KINDS = ("shared-file", "range", "seam", "flag", "tooling", "naming", "band-header", "untyped",
            "method", "undefrefs", "data-claim")
# default open; a `rename` is never carried
# The lint-derived kinds: `naming` is one item per file carrying rule-7 findings, `band-header` one per file
# carrying rule-2 findings, `untyped` one per file carrying rule-11 findings and `method` one per file carrying
# rule-13 findings (a `<Type>_<name>(<Type>* self)` free function that is a member) (`build_items` /
# `collect_lint_items`). All are ordinary open items, so the credit ratio rations new claims against them -
# no special-casing.
LINT_KINDS = ("naming", "band-header", "untyped", "method")
LINT_RULES = {"naming": 7, "band-header": 2, "untyped": 11, "method": 13}
# The name-based lint kinds: their weight is the number of **distinct at-fault names** - the token
# `stylelint.finding_identity` carries - not the occurrence count. One name repeated 500 times is one
# rename; 50 distinct names are 50 (`lint_index`). `untyped` is deliberately not one of them: a rule-11
# finding is a declaration's `void *`, not a name to clear, so it keeps its occurrence count.
NAME_KINDS = ("naming", "band-header")
RULE_KIND = {7: "naming", 2: "band-header", 11: "untyped", 13: "method"}
# The lint kinds the queue can hand out as a *claim* (`queue.py debt`): the item names a file and carries
# its distinct name list, so a lane can be given "clean the N names in this file" exactly like a unit
# proposal - the same `claims.py` claim, the same credit balance, one resolved item still earning one.
DEBT_KINDS = ("naming", "band-header")
# The undefined-reference source: one item per unit whose object relocates a name no link input can define.
# `undefrefs.py` owns the one rule for "a name nothing defines" (`unresolved_names`); this source reads it,
# never re-implements it, and - like every other kind - it is an ordinary open item the ratio rations
# against with no special-casing.
UNDEFREF_KIND = "undefrefs"
# The data-claim source (owner, 2026-09-29: "no data should be left behind", strict): one item per unit that still
# has refusable sole-owned orphan data - data only that unit's target object references, which no `splits.txt`
# claim covers and which is claimable on its own. `datagap.py` owns the one rule (`strict_report`); this source
# reads it, never re-implements it. The item's weight is the pair count; `triage` re-runs the rule and closes the
# item once the unit has none left. The unit's pairs a deferred class keeps out (`datagap.STRICT_CLASSES`) are
# not backlog: they are reported by the row, never owed.
DATACLAIM_KIND = "data-claim"
# A second defect of the same kind (owner, 2026-09-30, the `claim-exposed` deferral class): a pair only the unit's
# OWN claimed data references - claiming data exposes the pairs its relocations name, so the land gate defers
# them (`datagap.claim_exposed_pairs`) rather than chain claim after claim. One item per (unit, pair-run), its
# defect carrying the run's extent so `triage` can re-check exactly that range; the credit ratio rations it like
# any other item.
EXPOSED_PREFIX = "claim-exposed:"
_EXPOSED_RE = re.compile(r"^claim-exposed:(\.?[A-Za-z0-9]+):([0-9A-Fa-f]{8})-([0-9A-Fa-f]{8})$")

# -----------------------------------------------------------------------------------------------------------
# Text helpers
# -----------------------------------------------------------------------------------------------------------
def read(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def asstr(x) -> str:
    """A config_requests field as text (some are lists - e.g. a flag `change` of `["-opt","nopeephole"]`)."""
    return _outbox.asstr(x)


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
    `lib.outbox.FREE_TEXT_FIELDS` list. `lib/outbox.py` owns the schema and this intake reads the same list, so a
    filing that puts its content under its lane's own spelling (`why`, `request`, `what`, `subject`, `note`)
    is read here rather than registering content-free and being swallowed by a neighbour.
    """
    for field in tuple(preferred) + _outbox.FREE_TEXT_FIELDS:
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
    weight: int = 0         # the item's rank weight: a naming/band-header item's distinct-name count, an
                            # untyped item's finding count, an undefrefs unit's reference count (0 otherwise)
    names: list[str] = field(default_factory=list)  # a naming/band-header item's distinct at-fault names
    count: int = 0          # the live occurrence count behind `weight` (0 for every non-lint kind)
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
                lint_items: list[Item] | None = None,
                undefref_items: list[Item] | None = None,
                dataclaim_items: list[Item] | None = None) -> list[Item]:
    """Aggregate all five sources into one de-duplicated register, then apply persisted statuses.

    `tooling_register` is the tracked `docs/tooling-requests.md`; its statuses are read from it when
    `tooling_statuses` is not supplied. `lint_items` is the stylelint source (one item per file with
    rule-7/rule-2 findings), built by `collect_lint_items`; it is `None` when a caller has no tree to lint.
    `undefref_items` is the undefined-reference source (one item per unit with pre-existing unresolved
    references), built by `collect_undefref_items`; it is `None` when a caller has no compiled tree.
    `dataclaim_items` is the strict data-claim source (one item per unit with refusable sole-owned orphan
    data), built by `collect_dataclaim_items`; `None` when a caller has no split tree.
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

    # The undefined-reference source: an already-built item per unit, open until the rule stops firing.
    for it in (undefref_items or []):
        _merge(grouped, (it.kind, it.target, it.defect), it)

    # The data-claim source: an already-built item per unit, open until the unit's own data is claimed.
    for it in (dataclaim_items or []):
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
        from tools.units import stylelint as sl
        return sl.lint_all(main)
    except Exception:
        return []


def lint_counts(findings: list[dict]) -> dict:
    """`{(kind, file): n}` - the raw occurrence counts for the lint rules the backlog is filed against.

    This is the *count* the item's report keeps (`Item.count`), never its weight: the weight is the distinct
    name count (`lint_index`), so the two differ exactly on the file that repeats one name.
    """
    out: dict = {}
    for f in findings:
        kind = RULE_KIND.get(f["rule"])
        if kind:
            key = (kind, f["file"])
            out[key] = out.get(key, 0) + 1
    return out


def lint_names(findings: list[dict]) -> set:
    """The distinct at-fault names among `findings` - the lint's own token notion, never a second one.

    `stylelint.finding_identity` is what the landing gate's `--diff` compares (rule, file, at-fault token,
    detail); the **token** is the name a rename/move clears.  Reading the token through that function is
    what keeps this register from re-deriving "a name" from a finding's prose, so the register and the gate
    cannot disagree about which occurrences carry one name.
    """
    from tools.units import stylelint as sl
    return {sl.finding_identity(f)[2] for f in findings}


def lint_index(findings: list[dict]) -> dict:
    """`{(kind, file): {"names": [...], "count": n, "weight": w}}` for the backlog's lint kinds.

    `weight` is the **distinct at-fault name count** for `naming`/`band-header` (rules 7 and 2) - the unit
    of work this register measures, because one name repeated 500 times is one rename while 50 names are
    50 - and the occurrence count for `untyped` (rule 11), whose finding is a declaration's `void *`, not a
    name.  `names` is the sorted name list a claim's brief hands a lane; `count` is the raw occurrence
    count, kept so a reader can see the file whose 500 occurrences are one name.
    """
    grouped: dict = {}
    for f in findings:
        kind = RULE_KIND.get(f["rule"])
        if kind:
            grouped.setdefault((kind, f["file"]), []).append(f)
    out: dict = {}
    for key, finds in grouped.items():
        names = lint_names(finds)
        out[key] = {"names": sorted(n for n in names if n),
                    "count": len(finds),
                    "weight": len(names) if key[0] in NAME_KINDS else len(finds)}
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
    `defect` (`rule 7` / `rule 2`), so a partial fix keeps its status. `weight` is the **distinct at-fault
    name count** for `naming`/`band-header` (`lint_index`), so `rank` surfaces the file with the most
    *renames* outstanding - not the most occurrences - and the item carries the `names` list a claim hands a
    lane; `untyped` keeps its occurrence count. A zero-weight carry-forward keeps the last known count in its
    ask for the human reading it, but weighs nothing and is not claimable.
    """
    index = lint_index(stylelint_findings(main))
    prior = _prior_lint_items(load_register(main, register))
    items: list[Item] = []
    for key in sorted(set(index) | set(prior)):
        kind, target = key
        row = index.get(key) or {"names": [], "count": 0, "weight": 0}
        live, names, count = row["weight"], list(row["names"]), row["count"]
        if kind not in NAME_KINDS:
            names = []   # `untyped` weighs occurrences; its names are not the unit of work
        last = int((prior.get(key) or {}).get("weight") or 0)
        rule = LINT_RULES[kind]
        if kind == "naming":
            ask = ("`%s` carries %d distinct rule-7 name(s) (auto-generated `fn_`/`lbl_`/`loc_` or bare "
                   "`unk*` spellings the file already flagged) - name each from what it does or holds and "
                   "rename the map row in the same change"
                   % (target, live or last))
        elif kind == "method":
            ask = ("`%s` carries %d rule-13 finding(s) (a `<Type>_<name>(<Type>* self, ...)` free function - a "
                   "member spelled the C way) - declare it in the class, define `Type::name`, rename the map "
                   "row to the mangling and sweep the call sites (`python tools/units/methodize.py <Type>` "
                   "prints the plan), or mark a genuine C function `/* free: <reason> */`"
                   % (target, live or last))
        elif kind == "untyped":
            ask = ("`%s` carries %d rule-11 finding(s) (a `void *` parameter or return type) - name the "
                   "real type at every call site, or mark the declaration `/* untyped: <byte range|opaque "
                   "handle|caller-owned payload> */` with the case that makes it genuinely untyped"
                   % (target, live or last))
        else:
            ask = ("`%s` carries %d distinct rule-2 name(s) (an `extern` declared where it is not owned) - "
                   "move each declaration to its owner's header (or `include/unsplit/`) and #include it"
                   % (target, live or last))
        items.append(Item(kind=kind, target=target, defect="rule %d" % rule, status="open",
                          default_status="open", ask=ask, weight=live, names=names, count=count,
                          filings=[Filing(source="stylelint", lane="stylelint", when="", detail=ask)]))
    return items


def _prior_undefref_items(register: dict) -> dict:
    """The `unit` undefrefs items the published register already carries, keyed for carry-forward.

    Like a lint item, an `undefrefs` item must survive the regeneration that follows its fix: the rule no
    longer fires, but the item is still the `triage` subject and the ledger's unit of credit. The published
    payload's `items` list is the only place that remembers it between runs.
    """
    out: dict = {}
    for it in (register or {}).get("items", []):
        if isinstance(it, dict) and it.get("kind") == UNDEFREF_KIND:
            out[it.get("target", "")] = it
    return out


def undefref_counts(main: str) -> dict:
    """`{unit: unresolved-reference count}` from `undefrefs.census` - the one rule, never a second.

    `undefrefs.census` is exactly `unresolved_names` run over every compiled unit; reading it here means
    the backlog and the gate can never disagree about what "undefined" means. A tree with no compiled
    objects contributes nothing, and a read failure is an empty cell rather than a crashed run.
    """
    if not os.path.isdir(os.path.join(main, "build", "RMHE08", "src")):
        return {}
    try:
        from tools.units import undefrefs as ur
        rows = ur.census(main)
    except Exception:                              # additive source: never take the register down
        return {}
    counts: dict = {}
    for unit, _name, _spelling, _how in rows:
        counts[unit] = counts.get(unit, 0) + 1
    return counts


def collect_undefref_items(main: str, register: str | None = None) -> list[Item]:
    """The fourth source: `undefrefs.py`'s pre-existing unresolved references, one item per unit.

    The count is the item's `weight` (the mechanism `naming`/`band-header` already use), so the unit with
    the most undefined references leads. The item is an ordinary open item - the credit ratio rations new
    claims against it exactly as against the rest, with no special-casing. A unit whose debt is gone is
    carried forward from the published register (weight 0) so `triage` can re-run the one rule, prove it
    and earn the credit; the rule itself is `undefrefs.unresolved_names`, read here and never re-implemented.
    """
    counts = undefref_counts(main)
    prior = _prior_undefref_items(load_register(main, register))
    items: list[Item] = []
    for unit in sorted(set(counts) | set(prior)):
        live = counts.get(unit, 0)
        last = int((prior.get(unit) or {}).get("weight") or 0)
        ask = ("`%s` still makes %d reference(s) no link input can define - fix each to the spelling the "
               "target records (or add the name the map should carry) until the rule stops firing"
               % (unit, live or last))
        items.append(Item(kind=UNDEFREF_KIND, target=unit, defect="unresolved", status="open",
                          default_status="open", ask=ask, weight=live,
                          filings=[Filing(source="undefrefs", lane="undefrefs", when="", detail=ask)]))
    return items


_DATACLAIM_CACHE: dict = {}


def _dataclaim_state(main: str):
    """`(counts, exposed_runs)` from ONE strict report + census over the tree, or `None` when it cannot run.

    `counts` is `{unit: refusable sole-owned pair count}`; `exposed_runs` is `datagap.tree_claim_exposed` - the
    claim-exposed pairs (owner, 2026-09-30), which the `sole-owned` count leaves out so no pair is owed twice.
    Cached per process on the map/splits mtimes, because `build` runs several times per command.
    """
    base = os.path.join(main, "config", "RMHE08")
    if not (os.path.isdir(os.path.join(main, "build", "RMHE08", "obj"))
            and os.path.exists(os.path.join(base, "splits.txt")) and os.path.exists(os.path.join(base, "symbols.txt"))):
        return None
    stamp = tuple(os.path.getmtime(os.path.join(base, f)) for f in ("splits.txt", "symbols.txt"))
    hit = _DATACLAIM_CACHE.get(main)
    if hit and hit[0] == stamp:
        return hit[1]
    try:
        from tools.units import datagap as dg
        ranges = dg.load_claims(main)
        (records, _stats), _n, have = dg.census(main, None, ranges=ranges)
        if not have:
            return None
        report = dg.strict_report(main, records, ranges)
        runs = dg.tree_claim_exposed(main, records)
    except Exception:                              # additive source: never take the register down
        return None
    exposed = {(p["unit"], p["address"]) for run in runs for p in run["pairs"]}
    counts: dict = {}
    for block in report["blocks"]:
        if block["verdict"] == "refuse":
            n = sum(1 for p in block["pairs"] if (p["unit"], p["address"]) not in exposed)
            if n:
                counts[block["unit"]] = counts.get(block["unit"], 0) + n
    _DATACLAIM_CACHE[main] = (stamp, (counts, runs))
    return counts, runs


def dataclaim_counts(main: str) -> dict | None:
    """`{unit: refusable pair count}` from `datagap.strict_report` - the one rule, never a second.

    Claim-exposed pairs are not counted here (they are `dataclaim_exposed`'s). `None` when the rule cannot run
    here (no split objects, or a read failure): the source then contributes nothing and `triage` leaves the
    items `open (no check)`, rather than reading an empty tree as "all claimed".
    """
    state = _dataclaim_state(main)
    return None if state is None else state[0]


def dataclaim_exposed(main: str) -> list | None:
    """The tree's claim-exposed runs (`datagap.tree_claim_exposed`), or `None` when the rule cannot run here."""
    state = _dataclaim_state(main)
    return None if state is None else state[1]


def exposed_defect(section: str, start: int, end: int) -> str:
    """The claim-exposed item's defect: `claim-exposed:<section>:<start>-<end>` (hex, the run's extent)."""
    return "%s%s:%08X-%08X" % (EXPOSED_PREFIX, section, start, end)


def parse_exposed_defect(defect: str):
    """`(section, start, end)` of a claim-exposed defect, or `None` for any other item."""
    m = _EXPOSED_RE.match(defect or "")
    return (m.group(1), int(m.group(2), 16), int(m.group(3), 16)) if m else None


def _prior_dataclaim_items(register: dict) -> dict:
    """The `unit` data-claim items the published register already carries, keyed for carry-forward (see
    `_prior_undefref_items`: the item must survive the regeneration that follows its fix, so `triage` can close it).
    Keyed `(target, defect)`: a unit has one `sole-owned` item and one `claim-exposed` item per pair-run."""
    out: dict = {}
    for it in (register or {}).get("items", []):
        if isinstance(it, dict) and it.get("kind") == DATACLAIM_KIND:
            out[(it.get("target", ""), it.get("defect", "sole-owned") or "sole-owned")] = it
    return out


def collect_dataclaim_items(main: str, register: str | None = None) -> list[Item]:
    """The fifth source: `datagap.py`'s strict data-claim rule.

    One `sole-owned` item per unit with refusable pairs, weight = the pair count, so the unit with the most
    unclaimed data of its own leads; plus one `claim-exposed` item per (unit, pair-run) of claim-exposed pairs
    (owner, 2026-09-30: the gate defers them, the backlog owes them), weight = the run's pair count. An item
    whose debt is gone is carried forward from the published register (weight 0) so `triage` can re-run the rule
    and earn the credit. A tree the rule cannot run on contributes nothing new but still carries the published
    items forward.
    """
    counts = dataclaim_counts(main)
    runs = dataclaim_exposed(main) or []
    prior = _prior_dataclaim_items(load_register(main, register))
    items: list[Item] = []
    live_sole = counts or {}
    for unit in sorted({u for u, d in prior if d == "sole-owned"} | set(live_sole)):
        live = live_sole.get(unit, 0)
        last = int((prior.get((unit, "sole-owned")) or {}).get("weight") or 0)
        ask = ("`%s` still has %d data pair(s) only its own object references and no `splits.txt` claim covers - "
               "claim them (`python tools/units/dataclaim.py --unit %s` prints the exact lines) and reconstruct "
               "the bytes, until the strict data-claim rule stops refusing the unit"
               % (unit, live or last, unit))
        items.append(Item(kind=DATACLAIM_KIND, target=unit, defect="sole-owned", status="open",
                          default_status="open", ask=ask, weight=live,
                          filings=[Filing(source="datagap", lane="datagap", when="", detail=ask)]))
    live_exposed = {(r["unit"], exposed_defect(r["section"], r["start"], r["end"])): r for r in runs}
    for unit, defect in sorted({k for k in prior if k[1].startswith(EXPOSED_PREFIX)} | set(live_exposed)):
        run = live_exposed.get((unit, defect))
        parsed = parse_exposed_defect(defect)
        if parsed is None:
            continue
        section, start, end = parsed
        last = int((prior.get((unit, defect)) or {}).get("weight") or 0)
        n = len(run["pairs"]) if run else 0
        ask = ("`%s` has %d %s data pair(s) at 0x%08X-0x%08X that only its own claimed data references "
               "(claim-exposed: claiming that data exposed them, so the gate that landed it deferred them) - claim "
               "them (`python tools/units/dataclaim.py --unit %s` prints the exact lines), until the unit's "
               "object no longer leaves them unclaimed" % (unit, n or last, section, start, end, unit))
        items.append(Item(kind=DATACLAIM_KIND, target=unit, defect=defect, status="open", default_status="open",
                          ask=ask, weight=n, filings=[Filing(source="datagap", lane="datagap", when="", detail=ask)]))
    return items


def rank(items: list[Item]) -> list[Item]:
    """Filers first (the priority signal), then the item's weight, then recency; open before closed.

    The weight is a `naming`/`band-header` item's **distinct-name count** (one name repeated 500 times is
    nearly no work; 50 names are 50 renames), an `untyped` item's finding count, or an `undefrefs` unit's
    reference count, so the item with the most work outstanding surfaces first even though it carries no
    filing date; every other kind weighs 0, which leaves their existing filer/recency/vote order untouched.
    `votes` (`tooling.py`'s) stays the last tie-break.
    """
    order = {"open": 0, "parked": 1, "done": 2}
    return sorted(items, key=lambda it: (order.get(it.status, 3), -it.filer_count, -it.weight,
                                         _neg(it.last), -it.votes, it.kind, it.target, it.defect))


def weight_sums(items: list[Item]) -> dict:
    """The open debt by kind - the summed rank weight - plus a `total`, the report's burn-down number.

    `naming`/`band-header` contribute their distinct-name count (the renames outstanding), `untyped` its
    `void *` count, `undefrefs` its undefined-reference count; every other kind weighs 0 and is omitted.
    Summing only `open` items makes "is the debt shrinking?" a number rather than a memory. `payload`
    publishes this under `weights`, and `--print` shows it.
    """
    out: dict = {}
    for it in items:
        if it.status == "open" and it.weight:
            out[it.kind] = out.get(it.kind, 0) + it.weight
    out["total"] = sum(v for k, v in out.items() if k != "total")
    return out


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
        "version": 2,
        "as_of": as_of,
        "counts": counts,
        "weights": weight_sums(items),
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
                "count": it.count,
                "names": list(it.names),
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
    libtext.atomic_write(path, text)


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
                        lint_items=collect_lint_items(main, register),
                        undefref_items=collect_undefref_items(main, register),
                        dataclaim_items=collect_dataclaim_items(main, register))
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    return items, {"as_of": as_of, "counts": counts, "ledger": ledger,
                   "summary": ledger_summary(items, ledger["claims"], ledger["ratio"]),
                   "weights": weight_sums(items)}


def open_items(main: str, **kw) -> list[Item]:
    items, _ = build(main, **kw)
    return [i for i in items if i.status == "open"]


def debt_items(items: list[Item]) -> list[Item]:
    """The claimable debt among `items`: open `naming`/`band-header` items with names outstanding.

    Pure, so the queue can filter a single `build` instead of re-linting the tree.  A carried-forward item
    whose findings are gone weighs 0 and has no names, so it is not claimable (it is for `triage` to close).
    """
    return [it for it in items
            if it.status == "open" and it.kind in DEBT_KINDS and it.weight > 0 and it.names]


def open_debt_items(main: str, **kw) -> list[Item]:
    """The claimable debt in rank order (the file with the most names outstanding leads).

    These are the items the queue can hand out as a claim (`queue.py debt`): the register already rations new
    proposal claims against them, and this is the other half - the debt itself can be scheduled.
    """
    return debt_items(open_items(main, **kw))


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


def debt_unit(item: Item) -> str:
    """The claim unit of a debt item: the file it names.

    A debt claim is held on the file (`claims.claim`), so the same branch/worktree lock a unit proposal uses
    covers the file: a lane can be handed the debt while it is unclaimed, and a debt item whose file is
    already claimed is skipped, never handed to a second lane.
    """
    return item.target


def debt_names_line(item: Item, limit: int = 0) -> str:
    """`item`'s distinct names as one clause for a brief - all of them, or the first `limit` with a count."""
    names = list(item.names or [])
    if not names:
        return "no live names"
    shown = names if limit <= 0 else names[:limit]
    text = ", ".join("`%s`" % n for n in shown)
    if limit > 0 and len(names) > limit:
        text += ", ... (%d more)" % (len(names) - limit)
    return text


def debt_brief(main: str, item: Item) -> str:
    """The brief for a claimable debt item - its file, its rule and the exact names to clear.

    A naming item's names are renamed (and the map row renamed in the same change); a band-header item's
    extern declarations are moved to their owner.  The list is the item's `names`, which came from the
    lint's own token notion, so it is exactly the work left and nothing else.
    """
    rule = LINT_RULES.get(item.kind, "?")
    fix = ("Rename each name below from what the function/data means, and rename its `symbols.txt` row in "
           "the same change, until rule 7 no longer fires in the file." if item.kind == "naming" else
           "For each symbol below, move its declaration to its owner's header (or `include/unsplit/`) and "
           "`#include` it, until rule 2 no longer fires in the file.")
    lines = ["# Debt brief: %s" % item.key, "",
             "File: `%s`" % item.target,
             "Rule: `%s` (rule %s)" % (item.kind, rule),
             "Names outstanding: %d  (occurrences: %d)" % (len(item.names or []), item.count),
             "", fix, ""]
    if item.names:
        lines += ["Names (%d):" % len(item.names), ""]
        lines += ["- `%s`" % n for n in item.names]
    else:
        lines += ["No names are live now; the item is carried forward for `triage` to close."]
    lines += ["", "When every name is gone the item resolves:", "",
              "    python tools/units/backlog.py --set-status %s done" % item.key, ""]
    return "\n".join(lines)


def debt_task(main: str, item: Item, cwd: str | None = None, brief: str | None = None) -> dict:
    """A ready-to-paste debt lane: "clean the N names in this file", with the names in the task.

    Mirrors `lane_task` but leads with the file and its name list, because the payment is the names.  `cwd`
    is the claim's worktree when the queue claimed one; `brief` names the written brief file.
    """
    root = (cwd or main).replace("\\", "/")
    names = debt_names_line(item, limit=60)
    task = ("Work the campaign debt item `%s` (%s): clean the %d distinct name(s) in `%s` - %s. "
            "This is claimable debt, so `queue.py debt` spends one credit on it exactly like a proposal "
            "claim, and resolving it earns one back: do the work, then mark it "
            "`python tools/units/backlog.py --set-status %s done` (`parked` earns no credit). "
            % (item.key, item.kind, len(item.names or []), item.target, names, item.key))
    if brief:
        task += "Your brief is %s. " % brief.replace("\\", "/")
    task += ("Commit on your own branch; end your turn with your report - your final message is the result "
             "the orchestrator receives.")
    return {"agent": "fixer", "name": "fixer-debt-%s" % item.key[:32], "cwd": root, "task": task,
            "call": lanecmd.lane_call("fixer", root, task, name="fixer-debt-%s" % item.key[:32],
                                      main=main, key="debt-%s" % item.key[:32])["call"]}


def lane_task(main: str, item: Item, cwd: str | None = None, brief: str | None = None) -> dict:
    """A ready-to-paste lane for a backlog item - mirroring how `queue.py next` prints its spawn line.

    A `naming`/`band-header` item delegates to `debt_task`, so the lane is told the file and its names
    (`debt_task`), not merely the item's key.
    """
    if item.kind in DEBT_KINDS and item.names:
        return debt_task(main, item, cwd=cwd, brief=brief)
    profile = {"shared-file": "fixer", "range": "decompiler", "flag": "fixer",
               "naming": "fixer", "band-header": "fixer", "untyped": "fixer", "method": "fixer",
               "undefrefs": "fixer", "data-claim": "decompiler",
               "tooling": "worker"}.get(item.kind, "worker")
    task = ("Work the campaign backlog item `%s` (%s %s): %s. "
            "This is on the backlog, so `queue.py next` spends a credit on a new proposal claim until it is "
            "resolved or parked. Do the work, then mark it: `python tools/units/backlog.py --set-status %s done` "
            "(or `parked` with a reason - but `parked` earns no credit). The filing lanes' raw evidence is in %s. "
            "Commit on your own branch; end your turn with your report - your final message is the result the "
            "orchestrator receives."
            % (item.key, item.kind, item.target, item.ask, item.key,
               register_path(main).replace("\\", "/")))
    root = (cwd or main).replace("\\", "/")
    if brief:
        task += " Your brief is %s." % brief.replace("\\", "/")
    return {"agent": profile, "name": "%s-backlog-%s" % (profile, item.key[:32]),
            "cwd": root, "task": task,
            "call": lanecmd.lane_call(profile, root, task, name="%s-backlog-%s" % (profile, item.key[:32]),
                                      main=main, key="backlog-%s" % item.key[:32])["call"]}


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


def _flag_context(main: str) -> dict:
    """The flag checks' view of `configure.py`: every resolved `cflags_*` group (an ordered token list - a flag
    written as `"-opt nopeephole"` is one token, `"-func_align", "4"` two) and each lib's group name."""
    try:
        conf = _project.Configure.load(os.path.join(main, "configure.py"))
    except (OSError, SyntaxError):
        return {"groups": set(), "resolved": {}, "libs": {}}
    groups = {name: list(flags) for name, flags in conf.groups().items()}
    return {"groups": set(groups), "resolved": groups, "libs": {lib.name: lib.cflags_name for lib in conf.libs()}}


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
    for r in _project.Splits.read(path).ranges:
        out.setdefault(_norm_section(r.section), []).append((r.start, r.end, r.unit))
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
    toks = ctx["resolved"].get(group, [])
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

    The findings are read from the file itself with `stylelint.lint_source` (rule 11 with `rule11_findings`,
    which has no ownership dependency and sees the unsplit band), the same functions that produced the item,
    so the check cannot drift from the lint.  For `naming`/`band-header` the item is judged on the file's
    **distinct at-fault names** (`lint_names`, the lint's own token notion): it resolves only when every
    name is gone, never because the occurrence count merely fell - one rename of a repeated name is one
    name gone.  A missing file is `stale`; a rule-2 check needs the symbols/splits map, so with the map
    absent it stays open rather than call itself resolved.
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
        from tools.units import stylelint as sl
    except ImportError:  # pragma: no cover - the tool is always beside this module
        return ("open", "no check: stylelint.py is not importable")
    if rule == 2 and "ownership" not in ctx:
        ctx["ownership"] = sl.load_ownership(main)
    ownership = ctx.get("ownership")
    if rule == 2 and ownership is None:
        return ("open", "no check: config/RMHE08/symbols.txt or splits.txt is absent")
    if rule == 13:
        # Rule 13 reads the tree-wide type registry (`set_rule13_context`), built once per triage run and
        # cached by `stylelint` itself; the file is re-linted with the same function that filed it.
        if not ctx.get("rule13_ready"):
            sl.set_rule13_context(main)
            ctx["rule13_ready"] = True
        findings = sl.rule13_findings(sl.Source(path, item.target, read(path)))
        if findings:
            return ("open", "%s still carries %d rule-13 finding(s)" % (item.target, len(findings)))
        return ("resolved", "rule %d no longer fires in %s (re-linted with the same rule that filed it)"
                % (rule, item.target))
    if rule == 11:
        # Rule 11 is source-local and has no ownership dependency, so it is read straight from the file:
        # `lint_source` returns early for the unsplit band (rule 2 only), which would call a band item
        # resolved while its `void *` parameters are still there.  Its weight is the occurrence count, so
        # it resolves when no finding remains.
        findings = sl.rule11_findings(sl.Source(path, item.target, read(path)))
        if findings:
            return ("open", "%s still carries %d rule-11 finding(s)" % (item.target, len(findings)))
        return ("resolved", "rule %d no longer fires in %s (re-linted with the same rule that filed it)"
                % (rule, item.target))
    findings = [f for f in sl.lint_source(sl.Source(path, item.target, read(path)), ownership)
                if f["rule"] == rule]
    names = lint_names(findings)
    if names:
        shown = ", ".join("`%s`" % n for n in sorted(n for n in names if n)[:3])
        return ("open", "%s still carries %d distinct rule-%d name(s)%s"
                % (item.target, len(names), rule, (" (%s ...)" % shown) if shown else ""))
    return ("resolved", "rule %d no longer fires in %s (re-linted with the same rule that filed it; every "
            "distinct name is gone)" % (rule, item.target))


def _undefref_census(main: str, ctx: dict):
    """The one rule, re-run once per triage: `undefrefs.census` (which is `unresolved_names` over the tree).

    Cached in `ctx` so a run with many `undefrefs` items pays for the census once, not once per item - the
    same economy `_check_lint`'s per-file re-lint gets from being source-local. `None` when the rule cannot
    run at all (the module or the tree is unreadable), which is `no check`, never a guess.
    """
    if "undefref_census" not in ctx:
        try:
            from tools.units import undefrefs as ur
            ctx["undefref_census"] = ur.census(main)
            ctx["undefref_src"] = ur.SRC_REL
        except Exception as exc:               # a malformed fixture must stay open, never crash the run
            ctx["undefref_census"] = None
            ctx["undefref_error"] = str(exc)
    return ctx["undefref_census"]


def _check_undefrefs(main: str, item: Item, ctx: dict):
    """Whether a unit's undefined references are gone - the ONE rule re-run now, never remembered.

    `resolved` only when the unit still has a compiled object to judge and the rule names no reference
    for it; `open` otherwise, naming the live count. A unit with no compiled object has no
    evidence to re-run, so it stays `open` rather than guess. The rule is `undefrefs.unresolved_names`,
    reached through `undefrefs.census` - there is no second definition of "undefined" here.
    """
    rows = _undefref_census(main, ctx)
    if rows is None:
        return ("open", "no check: the undefined-reference rule is not runnable here (%s)"
                % one_line(ctx.get("undefref_error", "?"), 60))
    src = ctx.get("undefref_src") or os.path.join("build", "RMHE08", "src")
    if not os.path.exists(os.path.join(main, src, item.target + ".o")):
        return ("open", "no check: no compiled object for %s to re-run the rule" % item.target)
    hits = [name for unit, name, _spelling, _how in rows if unit == item.target]
    if not hits:
        return ("resolved", "the rule no longer fires for %s (re-ran undefrefs.unresolved_names over its "
                            "object; no reference is undefined)" % item.target)
    return ("open", "%s still references %d name(s) no link input can define: %s"
            % (item.target, len(hits), ", ".join(hits[:3]) + (" ..." if len(hits) > 3 else "")))


def _dataclaim_census(main: str, ctx: dict):
    """The one rule, re-run once per triage: `dataclaim_counts` (which is `datagap.strict_report` over the tree).

    Cached in `ctx` so many `data-claim` items pay for the census once. `None` when the rule cannot run at all,
    which is `no check`, never a guess.
    """
    if "dataclaim_counts" not in ctx:
        ctx["dataclaim_counts"] = dataclaim_counts(main)
    return ctx["dataclaim_counts"]


def _check_dataclaim(main: str, item: Item, ctx: dict):
    """Whether a unit still has refusable sole-owned data - the ONE rule re-run now, never remembered.

    `resolved` only when the rule ran, the unit still has a built target object to judge and the rule refuses
    none of its data; `open` otherwise, naming the live pair count. No object, no evidence, so it stays open.
    A `claim-exposed` item is judged on its own run: resolved when no claim-exposed pair of the unit is left in
    the item's range (the pairs were claimed, or the claim that carried the reference went away).
    """
    counts = _dataclaim_census(main, ctx)
    if counts is None:
        return ("open", "no check: the strict data-claim rule is not runnable here (no split objects)")
    if not os.path.exists(os.path.join(main, "build", "RMHE08", "obj", item.target + ".o")):
        return ("open", "no check: no target object for %s to re-run the rule" % item.target)
    span = parse_exposed_defect(item.defect)
    if span is not None:
        section, start, end = span
        if "dataclaim_exposed" not in ctx:
            ctx["dataclaim_exposed"] = dataclaim_exposed(main)
        live = [p for r in (ctx["dataclaim_exposed"] or []) if r["unit"] == item.target and r["section"] == section
                for p in r["pairs"] if start <= p["address"] < end]
        if live:
            return ("open", "%s still has %d claim-exposed data pair(s) in %s 0x%08X-0x%08X"
                    % (item.target, len(live), section, start, end))
        return ("resolved", "no claim-exposed pair of %s remains in %s 0x%08X-0x%08X (re-ran "
                            "datagap.tree_claim_exposed over the tree: claimed, or the referencing claim is gone)"
                % (item.target, section, start, end))
    live = counts.get(item.target, 0)
    if live:
        return ("open", "%s still has %d refusable sole-owned data pair(s)" % (item.target, live))
    return ("resolved", "the strict data-claim rule no longer refuses %s (re-ran datagap.strict_report over "
                        "the tree; no refusable sole-owned pair remains)" % item.target)


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
    if item.kind == UNDEFREF_KIND:
        return _check_undefrefs(main, item, ctx)
    if item.kind == DATACLAIM_KIND:
        return _check_dataclaim(main, item, ctx)
    return _check_tooling(main, item, ctx)


def triage(main: str, **kw) -> tuple[list, dict]:
    """Classify every OPEN item; returns `([(item, decision, evidence)], meta)`."""
    items, meta = build(main, **kw)
    ctx = dict(_flag_context(main), splits=_splits_ranges(main))
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
                        lint_items=collect_lint_items(main, register),
                        undefref_items=collect_undefref_items(main, register),
                        dataclaim_items=collect_dataclaim_items(main, register))
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
    w = meta.get("weights") or {}
    if w:
        names = [k for k in ("naming", "band-header") if w.get(k)]
        other = [k for k in sorted(w) if k not in ("total", "naming", "band-header") and w.get(k)]
        if names:
            print("  names outstanding: %s  (%d distinct names)"
                  % (" / ".join("%s %d" % (k, w[k]) for k in names),
                     sum(w[k] for k in names)))
        if other:
            print("  other open debt weight: %s"
                  % " / ".join("%s %d" % (k, w[k]) for k in other))
        print("  total open debt weight: %d" % w.get("total", 0))
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
        from tools.units import backlog_selftest
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
    undefref = collect_undefref_items(main_wt, register)  # the fourth source: one item per debt-carrying unit
    dataclaim = collect_dataclaim_items(main_wt, register)  # the fifth source: one item per unit with unclaimed data
    if args.set_status:
        key, status = args.set_status
        status = status.lower()
        if status not in STATUSES:
            print("status must be one of: %s" % ", ".join(STATUSES), file=sys.stderr)
            return 2
        known = {it.key for it in build_items(outbox, notes, tooling_register, statuses, lint_items=lint,
                                              undefref_items=undefref, dataclaim_items=dataclaim)}
        if key not in known:
            print("unknown backlog key %r - see `python tools/units/backlog.py --print`" % key, file=sys.stderr)
            return 2
        statuses[key] = status

    if args.cmd == "triage":
        decisions, _ = triage(main_wt, outbox=outbox, notes=notes,
                              tooling_register=tooling_register, register=register)
        rep = triage_report(decisions)
        all_items = build_items(outbox, notes, tooling_register, statuses, lint_items=lint,
                                undefref_items=undefref, dataclaim_items=dataclaim)
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

    items = build_items(outbox, notes, tooling_register, statuses, lint_items=lint,
                        undefref_items=undefref, dataclaim_items=dataclaim)
    as_of = _as_of(items)
    counts = {"open": sum(1 for i in items if i.status == "open"),
              "done": sum(1 for i in items if i.status == "done"),
              "parked": sum(1 for i in items if i.status == "parked"),
              "total": len(items)}
    meta = {"as_of": as_of, "counts": counts, "ledger": ledger,
            "summary": ledger_summary(items, ledger["claims"], args.ratio),
            "weights": weight_sums(items)}

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
