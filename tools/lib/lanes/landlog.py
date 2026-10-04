"""The landing log: one JSON line per `land.py land` attempt in `MAIN/.pi/land-log.jsonl` (outcome, refused row,
conflicted paths, seconds) - appended by the gate, read and summarised by `tools/units/landlog.py`.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import json
import os
import time
from collections import Counter
from dataclasses import asdict, dataclass, field

LOG_NAME = "land-log.jsonl"
OUTCOMES = ("landed", "refused", "conflict", "error")
SCHEMA = 1


@dataclass(frozen=True)
class Attempt:
    """One landing attempt. `refused_row` is the gate row that refused (None when it landed); `conflicts` the
    paths `apply` could not resolve; `units` the batch's units; `seconds` the wall time."""
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

    def __post_init__(self) -> None:
        if self.outcome not in OUTCOMES:
            raise ValueError("outcome %r is not one of %s" % (self.outcome, ", ".join(OUTCOMES)))

    def to_json(self) -> str:
        data = asdict(self)
        data["conflicts"] = list(self.conflicts)
        data["units"] = list(self.units)
        if not data["extra"]:
            del data["extra"]
        return json.dumps(data, sort_keys=True, separators=(",", ":"))


def log_path(main: str) -> str:
    return os.path.join(main, ".pi", LOG_NAME)


def append(main: str, attempt: Attempt) -> str:
    """Append one line (stamped now when `at` is empty) -> the log's path. One `write` of one line, so two
    concurrent appenders interleave whole lines."""
    if not attempt.at:
        attempt = Attempt(**{**asdict(attempt), "at": time.strftime("%Y-%m-%dT%H:%M:%S"),
                             "conflicts": tuple(attempt.conflicts), "units": tuple(attempt.units)})
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


def summary(rows: list[dict]) -> dict:
    """Counts by outcome, the refusing rows and conflicted paths by frequency, and wall-time totals."""
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
            "seconds_median_landed": round(sorted(landed)[len(landed) // 2], 1) if landed else None}
