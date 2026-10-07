"""The landing log: one JSON line per `land.py land` attempt in `MAIN/.pi/land-log.jsonl` (outcome, refused row,
conflicted paths, seconds, allowances granted) - appended by the gate, read and summarised by `tools/units/landlog.py`.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import json
import os
import time
from collections import Counter
from dataclasses import asdict, dataclass, field

from tools.lib import repo as _repo

LOG_NAME = "land-log.jsonl"
OUTCOMES = ("landed", "refused", "conflict", "error")
#: Schema 2 adds `allow` and `warnings`; a reader accepts every schema in `SCHEMAS` (a schema-1 line has neither).
SCHEMA = 2
SCHEMAS = (1, 2)
#: The allowance classes `allow` may carry: the `--allow-regression` units, the `--allow-rule10` keys, the
#: `--allow-rule12` tokens, the `--allow-orphan` addresses, `--no-outbox`, `--no-selftests`, the `--unit-rename`
#: pairs. A list class holds what the command line named; a flag class is `true`. An unused class is absent.
ALLOW_CLASSES = ("regression", "rule10", "rule12", "orphan", "no_outbox", "no_selftests", "unit_renames")


@dataclass(frozen=True)
class Attempt:
    """One landing attempt. `refused_row` is the gate row that refused (None when it landed); `conflicts` the
    paths `apply` could not resolve; `units` the batch's units; `seconds` the wall time; `allow` the allowances
    the command line granted (`ALLOW_CLASSES`; `{}` when none); `warnings` the WARNING rows' findings
    (`<row>: <finding>`; a warning never refuses); `manifest` the lane manifest id `land --manifest` named (absent
    from the line when none - an optional key, so the schema stays 2); `seam_moves` the pure seam moves the
    regression row credited (`"A -> B (N functions)"`; absent from the line when none, likewise optional)."""
    branch: str
    outcome: str
    seconds: float
    refused_row: str | None = None
    conflicts: tuple[str, ...] = ()
    units: tuple[str, ...] = ()
    commit: str | None = None
    at: str = ""
    schema: int = SCHEMA
    extra: dict = field(default_factory=dict)
    allow: dict = field(default_factory=dict)
    warnings: tuple[str, ...] = ()
    manifest: str | None = None
    seam_moves: tuple[str, ...] = ()

    def __post_init__(self) -> None:
        if self.outcome not in OUTCOMES:
            raise ValueError("outcome %r is not one of %s" % (self.outcome, ", ".join(OUTCOMES)))
        unknown = sorted(set(self.allow) - set(ALLOW_CLASSES))
        if unknown:
            raise ValueError("allowance class(es) %s not in %s" % (", ".join(unknown), ", ".join(ALLOW_CLASSES)))

    def to_json(self) -> str:
        data = asdict(self)
        data["conflicts"] = list(self.conflicts)
        data["units"] = list(self.units)
        data["warnings"] = list(self.warnings)
        if not data["extra"]:
            del data["extra"]
        if data["manifest"] is None:
            del data["manifest"]
        if self.seam_moves:
            data["seam_moves"] = list(self.seam_moves)
        else:
            del data["seam_moves"]
        return json.dumps(data, sort_keys=True, separators=(",", ":"))


def log_path(main: str) -> str:
    """`<main>/.pi/land-log.jsonl` (`lib.repo.state`: the one list of campaign state names)."""
    return str(_repo.state(LOG_NAME, main))


def append(main: str, attempt: Attempt) -> str:
    """Append one line (stamped now when `at` is empty) -> the log's path. One `write` of one line, so two
    concurrent appenders interleave whole lines."""
    if not attempt.at:
        attempt = Attempt(**{**asdict(attempt), "at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                             "conflicts": tuple(attempt.conflicts), "units": tuple(attempt.units),
                             "warnings": tuple(attempt.warnings),
                             "seam_moves": tuple(attempt.seam_moves)})
    path = log_path(main)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "a", encoding="utf-8", newline="\n") as fh:
        fh.write(attempt.to_json() + "\n")
    return path


def read(main: str) -> tuple[list[dict], list[int]]:
    """`(records, bad line numbers)` - a line that is not a JSON object is reported, never fatal."""
    rows, bad = [], []
    try:
        with open(log_path(main), encoding="utf-8", errors="replace") as fh:
            lines = fh.read().splitlines()
    except OSError:
        return [], []
    for i, line in enumerate(lines, 1):
        if not line.strip():
            continue
        try:
            obj = json.loads(line)
        except ValueError:
            bad.append(i)
            continue
        if isinstance(obj, dict):
            rows.append(obj)
        else:
            bad.append(i)
    return rows, bad


def allowance_counts(rows: list[dict]) -> dict:
    """`{class: {"attempts": n, "entries": m}}` over the rows' `allow` (schema 2; a schema-1 row carries none): how
    many attempts used the class and how many keys/units/addresses they named in total (a flag counts 1)."""
    out: dict = {}
    for r in rows:
        allow = r.get("allow") if isinstance(r.get("allow"), dict) else {}
        for cls, value in allow.items():
            if not value:
                continue
            slot = out.setdefault(cls, {"attempts": 0, "entries": 0})
            slot["attempts"] += 1
            slot["entries"] += len(value) if isinstance(value, (list, tuple, dict)) else 1
    return {cls: out[cls] for cls in sorted(out, key=lambda c: (ALLOW_CLASSES + (c,)).index(c))}


def summary(rows: list[dict]) -> dict:
    """Counts by outcome, the refusing rows and conflicted paths by frequency, wall-time totals, the allowances
    (`allowance_counts`), how many attempts named a lane manifest, and the schema versions read."""
    outcomes = Counter(str(r.get("outcome")) for r in rows)
    refused = Counter(str(r["refused_row"]) for r in rows if r.get("refused_row"))
    conflicts = Counter(p for r in rows for p in (r.get("conflicts") or []))
    secs = [float(r.get("seconds") or 0.0) for r in rows]
    landed = [float(r.get("seconds") or 0.0) for r in rows if r.get("outcome") == "landed"]
    return {"attempts": len(rows),
            "outcomes": {k: outcomes.get(k, 0) for k in OUTCOMES},
            "landed_ratio": round(outcomes.get("landed", 0) / len(rows), 3) if rows else None,
            "refused_rows": refused.most_common(),
            "conflicted_paths": conflicts.most_common(),
            "seconds_total": round(sum(secs), 1),
            "seconds_median_landed": round(sorted(landed)[len(landed) // 2], 1) if landed else None,
            "allowances": allowance_counts(rows),
            "manifests": sum(1 for r in rows if r.get("manifest")),
            "warning_rows": Counter(str(w).split(":", 1)[0] for r in rows
                                    for w in (r.get("warnings") or [])).most_common(),
            "schemas": dict(sorted(Counter(int(r.get("schema") or 1) for r in rows).items()))}
