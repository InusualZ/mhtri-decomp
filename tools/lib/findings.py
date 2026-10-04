"""The one shape for a check's result (Finding, Row, Verdict), its add-only comparison, rendering and exit code.
Spec: docs/tools/spec/lib-findings.md. CLI: none (library)."""
from __future__ import annotations

import json
from dataclasses import dataclass
from typing import Any, Callable, Iterable, Mapping

PASS, FAIL, UNKNOWN, SKIP = "PASS", "FAIL", "UNKNOWN", "SKIP"
STATUSES = (PASS, FAIL, UNKNOWN, SKIP)

#: A row's KIND: GATE refuses because the batch is bad, BOOKKEEPING because the landing's own state is stale.
KIND_GATE = "gate"
KIND_BOOKKEEPING = "bookkeeping"
KIND_TAG = {KIND_GATE: "GATE", KIND_BOOKKEEPING: "BOOKKEEPING"}

#: The exit convention: 0 ok, 1 findings or a refusal, 2 could not run.
EXIT_OK, EXIT_FINDINGS, EXIT_ERROR = 0, 1, 2


# --- Finding: a lint-style item ------------------------------------------------------------------------------

@dataclass(frozen=True)
class Finding:
    """One finding at `file:line`. `token` is the identifier at fault (None when the rule names none), `text` the
    source line it sits on, `detail` the message, `remedy` what closes it."""
    rule: Any
    file: str
    line: int
    token: str | None = None
    detail: str = ""
    remedy: str | None = None
    text: str = ""

    def identity(self) -> tuple:
        """Line-independent: `(rule, file, token, detail)` - what the add-only diff compares."""
        return (self.rule, self.file, self.token, self.detail)

    def to_dict(self) -> dict:
        """The JSON schema: `rule, file, line, token, text, detail`, plus `remedy` when set."""
        d = {"rule": self.rule, "file": self.file, "line": self.line, "token": self.token, "text": self.text,
             "detail": self.detail}
        if self.remedy is not None:
            d["remedy"] = self.remedy
        return d

    @classmethod
    def from_dict(cls, d: Mapping) -> "Finding":
        return cls(d["rule"], d["file"], d.get("line", 0), d.get("token"), d.get("detail", ""), d.get("remedy"),
                   d.get("text", ""))


def _get(f: Any, key: str, default: Any = None) -> Any:
    return f.get(key, default) if isinstance(f, Mapping) else getattr(f, key, default)


def identity(f: Finding | Mapping) -> tuple:
    """`Finding.identity` for a `Finding` or its dict form."""
    if isinstance(f, Finding):
        return f.identity()
    return (f["rule"], f["file"], f.get("token"), f["detail"])


Credit = Callable[[Any], Iterable[Any]]


def added(before: Iterable, after: Iterable, credit: Credit | None = None) -> dict:
    """`{(rule, file): [finding, ...]}` - one entry per after-side identity new to that `(rule, file)`.

    A set difference per `(rule, file)`, not a count: spelling a known identity again adds nothing, a new one is
    reported once (its first occurrence by line). `credit(f)` yields the other spellings a base finding is also
    known under (a rename, a moved file), so a rename is never a removal plus an addition.
    """
    known: dict = {}
    for f in before:
        s = known.setdefault((_get(f, "rule"), _get(f, "file")), set())
        s.add(identity(f))
        for g in (credit(f) if credit else ()):
            s.add(identity(g))
    groups: dict = {}
    for f in after:
        groups.setdefault((_get(f, "rule"), _get(f, "file")), []).append(f)
    out: dict = {}
    for key, finds in groups.items():
        k = known.get(key, ())
        seen: dict = {}
        for f in sorted(finds, key=lambda f: _get(f, "line")):
            ident = identity(f)
            if ident in k or ident in seen:
                continue
            seen[ident] = f
        if seen:
            out[key] = list(seen.values())
    return out


def removed(before: Iterable, after: Iterable, credit: Credit | None = None) -> dict:
    """`{(rule, token, detail): [file, ...]}` - one entry per base identity a file stopped carrying under every
    spelling `credit` gives it; the mirror of `added`.

    The key is spelled the way the **after** side spells the identity - the first spelling `credit` gives (a map
    rename, a re-homed path), else the base spelling - so a removal can be matched to the addition another file of
    the same batch made under the new name (a move that also renamed). Without a credit, or when `credit` gives the
    finding back unchanged, the key is the base identity's own `(rule, token, detail)`.
    """
    here_by: dict = {}
    for f in after:
        here_by.setdefault((_get(f, "rule"), _get(f, "file")), set()).add(identity(f))
    out: dict = {}
    seen: set = set()
    for f in before:
        ident = identity(f)
        if ident in seen:
            continue
        seen.add(ident)
        here = here_by.get((_get(f, "rule"), _get(f, "file")), set())
        spellings = list(credit(f)) if credit else []
        if ident in here or any(identity(g) in here for g in spellings):
            continue
        now = spellings[0] if spellings else f
        out.setdefault((_get(now, "rule"), _get(now, "token"), _get(now, "detail")), []).append(_get(f, "file"))
    return out


# --- Row: a gate-style item ----------------------------------------------------------------------------------

@dataclass(frozen=True)
class Row:
    """One check's verdict: `status` in `STATUSES`, `detail` what a failure printed, `evidence` the note a pass
    carries, `remedy` what to do, `kind` GATE or BOOKKEEPING."""
    name: str
    status: str
    detail: str = ""
    evidence: str = ""
    remedy: str = ""
    kind: str = KIND_GATE

    @classmethod
    def check(cls, name: str, good: bool, detail: str = "", evidence: str = "", kind: str = KIND_GATE,
              remedy: str = "") -> "Row":
        """A PASS/FAIL row from a boolean; an unknown kind reads as GATE (never soft-pedalled)."""
        return cls(name, PASS if good else FAIL, detail, evidence, remedy, kind if kind in KIND_TAG else KIND_GATE)

    @classmethod
    def from_tuple(cls, row: tuple | "Row") -> "Row":
        """`(name, good, detail, info[, kind[, remedy]])` - the gate's tuple form - as a Row."""
        if isinstance(row, Row):
            return row
        return cls.check(row[0], bool(row[1]), row[2] if len(row) > 2 else "", row[3] if len(row) > 3 else "",
                         row[4] if len(row) > 4 else KIND_GATE, row[5] if len(row) > 5 else "")

    @property
    def failed(self) -> bool:
        return self.status == FAIL

    def note(self) -> Any:
        """What a rendered row shows: a failure's detail, else the evidence."""
        return (self.detail if self.failed else "") or self.evidence

    def line(self) -> str:
        """`PASS name - note` (the compact listing)."""
        note = self.note()
        return "%s %s%s" % (self.status, self.name, (" - " + note) if note else "")

    def to_dict(self) -> dict:
        return {"name": self.name, "status": self.status, "detail": self.detail, "evidence": self.evidence,
                "remedy": self.remedy, "kind": self.kind}


def rows_of(items: Iterable) -> list[Row]:
    """Rows from Rows or gate tuples."""
    return [Row.from_tuple(r) for r in items]


def render_table(rows: Iterable, name_width: int = 58, note_width: int = 80, header: bool = True) -> str:
    """The gate's table: a `check / result` header, then `<name padded> <STATUS>  <note>` per row."""
    out = ["%-*s %s" % (name_width, "check", "result")] if header else []
    for r in rows_of(rows):
        note = r.note()
        out.append("%-*s %s%s" % (name_width, r.name[:name_width], r.status,
                                  ("  " + note[:note_width]) if note else ""))
    return "\n".join(out)


def render_lines(rows: Iterable) -> str:
    """One `Row.line()` per row."""
    return "\n".join(r.line() for r in rows_of(rows))


# --- Verdict -------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Verdict:
    """The fold of a check's rows (Rows or Findings; a Finding is a failure)."""
    rows: tuple = ()

    @classmethod
    def of(cls, items: Iterable) -> "Verdict":
        return cls(tuple(i if isinstance(i, (Row, Finding)) else Row.from_tuple(i) for i in items))

    @property
    def failures(self) -> list:
        return [r for r in self.rows if isinstance(r, Finding) or r.failed]

    @property
    def ok(self) -> bool:
        return not self.failures

    @property
    def failed_kinds(self) -> set[str]:
        """The KINDs among the failed rows (a Finding counts as GATE)."""
        return {r.kind if isinstance(r, Row) else KIND_GATE for r in self.failures}

    @property
    def summary(self) -> str:
        """`N row(s): a PASS, b FAIL, ...` over the statuses present (findings counted as `findings`)."""
        counts: dict = {}
        for r in self.rows:
            key = "finding" if isinstance(r, Finding) else r.status
            counts[key] = counts.get(key, 0) + 1
        parts = ["%d %s" % (counts[s], s) for s in STATUSES + ("finding",) if counts.get(s)]
        return "%d row(s)%s" % (len(self.rows), (": " + ", ".join(parts)) if parts else "")


def render_json(tool: str, verdict: Verdict, **extra: Any) -> str:
    """The one schema: `{"tool", "rows", "ok", "summary"}` (plus `extra` keys)."""
    payload = {"tool": tool, "rows": [r.to_dict() for r in verdict.rows], "ok": verdict.ok,
               "summary": verdict.summary}
    payload.update(extra)
    return json.dumps(payload, indent=2)


def exit_code(result: Verdict | BaseException | bool | int | None) -> int:
    """0 ok, 1 findings or a refusal, 2 could not run (an exception other than a `SystemExit`); an int passes
    through, a bool is ok/findings."""
    if isinstance(result, int) and not isinstance(result, bool):
        return result
    if isinstance(result, SystemExit):
        code = result.code
        return code if isinstance(code, int) else (EXIT_OK if code is None else EXIT_ERROR)
    if isinstance(result, BaseException):
        return EXIT_ERROR
    if isinstance(result, Verdict):
        return EXIT_OK if result.ok else EXIT_FINDINGS
    if result is None:
        return EXIT_OK
    return EXIT_OK if result else EXIT_FINDINGS
