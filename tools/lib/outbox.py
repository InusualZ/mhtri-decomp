"""The outbox and notes a lane hands back: the schema, tolerant loading and validation.
Spec: docs/tools/spec/lib-outbox.md. CLI: none (library)."""
from __future__ import annotations

import glob
import json
import os
from dataclasses import dataclass, field
from typing import Any, TextIO

from tools.lib.findings import Finding

#: The fields every outbox entry must carry.
REQUIRED = ("unit", "worker", "finished_at", "unit_percent", "symbols", "residual", "measured_with")

#: The `config_requests` kinds: the fields the validator requires and the ones the orchestrator reads when
#: present. `brief.py` renders this table verbatim, so the brief asks for exactly what the validator accepts.
CONFIG_REQUEST_SCHEMA = (
    {"kind": "range", "needs": ("section", "start", "end"), "also": ("evidence",),
     "means": "a data range this unit owns (a splits.txt range plus its configure.py entry)"},
    {"kind": "seam", "needs": ("section", "start", "end", "evidence"), "also": ("why",),
     "means": "a seam finding: this code span's boundary is in the wrong place and the unit split should be "
              "re-drawn - distinct from `range`, which claims a data run this unit already owns"},
    {"kind": "rename", "needs": ("old", "new", "evidence"), "also": (),
     "means": "a map-symbol rename, with the evidence for the new name"},
    {"kind": "flag", "needs": ("evidence",), "also": ("lib", "change"),
     "means": "a compiler flag for a lib, with the probe numbers that justify it"},
    {"kind": "shared-file", "needs": ("why",), "also": ("file",),
     "means": "an edit to a file a worker may not touch (a tool, configure.py, splits.txt)"},
)
CONFIG_KINDS = tuple(row["kind"] for row in CONFIG_REQUEST_SCHEMA)
CONFIG_NEEDS = {row["kind"]: row["needs"] for row in CONFIG_REQUEST_SCHEMA}
FLAG_PROBE_FIELDS = ("flags", "effect", "verdict")
FLAG_PROBE_VERDICTS = ("reject", "adopt", "inconclusive", "kept")

#: Where a lane files a request's content when it does not use (or the schema does not name) the kind's
#: structured fields - one list for the validator and every intake.
FREE_TEXT_FIELDS = ("evidence", "why", "request", "what", "subject", "note")

#: The `Finding.rule` of a validation error (the batch may not land) and of a warning.
ERROR, WARNING = "outbox-error", "outbox-warning"


def asstr(x: Any) -> str:
    """A field as text: a string as is, None as `""`, anything else as compact JSON."""
    if isinstance(x, str):
        return x
    if x is None:
        return ""
    return json.dumps(x, ensure_ascii=False)


def config_schema_rows() -> list[dict]:
    """The schema table, as fresh dicts."""
    return [dict(row) for row in CONFIG_REQUEST_SCHEMA]


def request_content(req: dict) -> str:
    """A request's free-text content: the first non-empty string under `FREE_TEXT_FIELDS` (`""` when none)."""
    for name in FREE_TEXT_FIELDS:
        value = req.get(name)
        if isinstance(value, str) and value.strip():
            return value
    return ""


def symbol_like(name: str) -> bool:
    """Whether `name` could be one map symbol rather than a summary row (`"80 more symbols"`, `"a / b"`)."""
    s = (name or "").strip()
    return bool(s) and " " not in s and "/" not in s


def units_declared(entry: dict) -> list[str]:
    """The unit spellings an entry declares: `unit`, then a batch's `units`/`also_changed_units`/`changed_units`
    and `per_unit` (items may be strings or `{unit|name}` objects), de-duplicated in order. A prose `unit` is
    never split."""
    out: list[str] = []

    def add(value: Any) -> None:
        if isinstance(value, str) and value.strip():
            out.append(value.strip())

    add(entry.get("unit"))
    for key in ("units", "also_changed_units", "changed_units"):
        value = entry.get(key)
        if isinstance(value, list):
            for item in value:
                add((item.get("unit") or item.get("name")) if isinstance(item, dict) else item)
    for item in (entry.get("per_unit") or []):
        if isinstance(item, dict):
            add(item.get("unit") or item.get("name"))
    return list(dict.fromkeys(out))


def validate(entry: dict, owned: set[str], file: str = "") -> list[Finding]:
    """Every problem with `entry`, as Findings (rule `ERROR` refuses a landing, `WARNING` does not); `owned` is
    the symbol set the declared units own (empty: no ownership check). `token` names the field at fault."""
    out: list[Finding] = []

    def err(token: str, detail: str) -> None:
        out.append(Finding(ERROR, file, 0, token, detail))

    def warn(token: str, detail: str) -> None:
        out.append(Finding(WARNING, file, 0, token, detail))

    for key in REQUIRED:
        if key not in entry:
            err(key, "missing field `%s`" % key)
    pct = entry.get("unit_percent")
    if not isinstance(pct, (int, float)):
        err("unit_percent", "unit_percent must be a number")
    elif not 0 <= pct <= 100:
        err("unit_percent", "unit_percent out of range: %r" % pct)
    syms = entry.get("symbols")
    if not isinstance(syms, list) or not syms:
        err("symbols", "symbols must be a non-empty list")
    else:
        for i, s in enumerate(syms):
            tok = "symbols[%d]" % i
            if not isinstance(s, dict) or not s.get("name"):
                err(tok, "symbols[%d] has no name" % i)
                continue
            value = s.get("percent")
            if not isinstance(value, (int, float)):
                err(tok, "symbols[%d] (%s) has no numeric percent" % (i, s["name"]))
            elif not 0 <= value <= 100:
                err(tok, "symbols[%d] (%s) percent out of range: %r" % (i, s["name"], value))
            if owned and symbol_like(s["name"]) and s["name"] not in owned:
                err(tok, "symbols[%d] `%s` is not owned by this unit" % (i, s["name"]))
    if not isinstance(entry.get("residual"), str) or not entry.get("residual", "").strip():
        err("residual", "residual must be a non-empty string ('none' is a valid answer)")
    if "measured_with" in entry and (not isinstance(entry.get("measured_with"), str)
                                     or not entry.get("measured_with", "").strip()):
        err("measured_with", "measured_with must name the command the numbers came from")
    for i, req in enumerate(entry.get("config_requests") or []):
        tok = "config_requests[%d]" % i
        if not isinstance(req, dict):
            err(tok, "config_requests[%d] is not an object" % i)
            continue
        kind = req.get("kind")
        content = request_content(req)
        if kind not in CONFIG_NEEDS:
            # an out-of-schema kind is a real filing when it carries free-text content
            if not content:
                err(tok, "config_requests[%d] has kind %r (not one of %s) and no free-text content (%s)"
                    % (i, kind, list(CONFIG_KINDS), ", ".join(FREE_TEXT_FIELDS)))
            continue
        missing = [f for f in CONFIG_NEEDS[kind] if f not in req or req.get(f) in (None, "")]
        if missing and not content:
            err(tok, "config_requests[%d] (%s) needs %s (or content under one of %s)"
                % (i, kind, ", ".join(missing), ", ".join(FREE_TEXT_FIELDS)))
    for i, probe in enumerate(entry.get("flags_probed") or []):
        tok = "flags_probed[%d]" % i
        if isinstance(probe, str):
            if not probe.strip():
                err(tok, "flags_probed[%d] is an empty string" % i)
            continue
        if not isinstance(probe, dict):
            err(tok, "flags_probed[%d] is not an object or a string (it needs flags, effect, verdict)" % i)
            continue
        flags = str(probe.get("flags") or probe.get("flag") or "").strip()
        effect = str(probe.get("effect") or probe.get("result") or probe.get("evidence") or "").strip()
        verdict = str(probe.get("verdict") or "").strip()
        if not (flags or effect or verdict):
            err(tok, "flags_probed[%d] carries no flags/effect/verdict content" % i)
            continue
        missing = [name for name, value in (("flags", flags), ("effect", effect), ("verdict", verdict))
                   if not value]
        if missing:
            warn(tok, "flags_probed[%d] does not name %s" % (i, ", ".join(missing)))
        elif not any(verdict.lower().startswith(v) for v in FLAG_PROBE_VERDICTS):
            err(tok, "flags_probed[%d] verdict %r is not one of %s"
                % (i, probe.get("verdict"), "/".join(FLAG_PROBE_VERDICTS)))
    if entry.get("claim_state") not in (None, "released", "open"):
        warn("claim_state", "claim_state %r is not open or released" % entry["claim_state"])
    if not (entry.get("blockers") or []):
        warn("blockers", "no blockers listed ('[]' is a valid answer)")
    return out


def errors_and_warnings(findings: list[Finding]) -> tuple[list[str], list[str]]:
    """`(errors, warnings)` as message lists, in order."""
    return ([f.detail for f in findings if f.rule == ERROR], [f.detail for f in findings if f.rule == WARNING])


# --- loading -------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Entry:
    """One outbox file: `data` is its JSON object (`{}` when it could not be read, with `error` saying why)."""
    path: str
    data: dict = field(default_factory=dict)
    error: str = ""

    @property
    def stem(self) -> str:
        return os.path.splitext(os.path.basename(self.path))[0]

    def text(self, name: str) -> str:
        """Field `name` as text (`asstr`)."""
        return asstr(self.data.get(name))

    def requests(self) -> list[dict]:
        """The `config_requests` that are objects."""
        reqs = self.data.get("config_requests")
        return [r for r in reqs if isinstance(r, dict)] if isinstance(reqs, list) else []

    def units(self) -> list[str]:
        return units_declared(self.data)

    def symbols(self) -> list[str]:
        """The names under `symbols` (objects with a `name`, or plain strings)."""
        syms = self.data.get("symbols")
        if not isinstance(syms, list):
            return []
        names = [(s.get("name") if isinstance(s, dict) else s) for s in syms]
        return [n for n in names if isinstance(n, str) and n]


def _read(path: str) -> str:
    with open(path, encoding="utf-8", errors="replace") as fh:
        return fh.read()


def load_outbox(path: str) -> Entry:
    """One outbox file; an unreadable file or a non-object is an Entry with `error` set."""
    try:
        data = json.loads(_read(path))
    except (OSError, json.JSONDecodeError) as exc:
        return Entry(path, {}, str(exc))
    if not isinstance(data, dict):
        return Entry(path, {}, "not a JSON object")
    return Entry(path, data)


def load_outboxes(directory: str, warn: TextIO | None = None, keep_bad: bool = False) -> list[Entry]:
    """Every `*.json` in `directory`, sorted by path. A bad file is skipped (with a `warn: skipping <name>
    (<why>)` line on `warn` when given) unless `keep_bad`."""
    out = []
    for path in sorted(glob.glob(os.path.join(directory, "*.json"))):
        entry = load_outbox(path)
        if entry.error and not keep_bad:
            if warn is not None:
                print("warn: skipping %s (%s)" % (os.path.basename(path), entry.error), file=warn)
            continue
        out.append(entry)
    return out


def requests_path(directory: str, slug: str) -> str:
    """`<directory>/<slug>-requests.json` - the lane's integrator requests, one JSON object per line, beside its
    outbox (the schema is `lib.requests.SCHEMA`; the integrator's verdicts go to the `.status.json` sidecar)."""
    from tools.lib import requests as _requests
    return os.path.join(directory, slug + _requests.REQUESTS_SUFFIX)


def check_requests(path: str) -> tuple[list[str], list[str]]:
    """`(errors, notes)` for a lane's request file: a new-schema line is validated (`lib.requests.validate`); a
    free-text line is accepted and noted with what the loader read from it (kind, class, targets)."""
    from tools.lib import requests as _requests
    errors, notes = [], []
    for n, line in enumerate(_read(path).splitlines(), 1):
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError as exc:
            errors.append("line %d: not JSON (%s)" % (n, exc))
            continue
        if not isinstance(row, dict):
            errors.append("line %d: not an object" % n)
            continue
        if "id" in row:
            errors += ["line %d: %s" % (n, e) for e in _requests.validate(row)]
        else:
            req = _requests.normalise_legacy(row, os.path.basename(path), n)
            cls, why = _requests.classify(req)
            notes.append("line %d: free text, read as %s / %s (%s): %s" % (
                n, req.kind, cls, why, ", ".join(t.symbol for t in req.targets) or "no symbol"))
    return errors, notes


def load_notes(directory: str) -> dict[str, str]:
    """`{stem: text}` for every `*.md` in `directory` that can be read, sorted by path."""
    out = {}
    for path in sorted(glob.glob(os.path.join(directory, "*.md"))):
        try:
            out[os.path.splitext(os.path.basename(path))[0]] = _read(path)
        except OSError:
            continue
    return out
