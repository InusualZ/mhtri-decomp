"""The measure a source edit is judged by (the writable `Workspace`, the scored `Measurer`, `judge`, `TrialRunner`),
shared by `tools/units/unittry.py` and `tools/units/applyorder.py`.
Spec: docs/tools/spec/lib-trialrun.md. CLI: none (library)."""
from __future__ import annotations

import os
import time
from dataclasses import dataclass, field

from tools.lib import pendingmap, repo, report, text, units

#: The scratch directory under `build/tmp/` (the object and the report each trial is scored from).
SCRATCH = "unittry"
#: A score change smaller than this is no change.
EPS = 1e-6


# --------------------------------------------------------------------------------------------------
# the files: the accepted state, the candidate, the byte-for-byte restore
# --------------------------------------------------------------------------------------------------

class Workspace:
    """The files a run may write: the unit's source and the headers it may edit.

    `orig` holds each file's bytes and mtime as the run found it; `cur` the LF text of the accepted state. A write
    keeps the file's line endings; `restore` puts the accepted state back and a file whose accepted text is its
    original text gets its original bytes and mtime, so ninja sees nothing changed."""

    def __init__(self, allowed: dict[str, str]):
        self.allowed = allowed            # abs path -> why it may be written ("source", "own header", ...)
        self.orig: dict[str, tuple[bytes, int]] = {}
        self.cur: dict[str, str] = {}

    def text(self, path: str) -> str:
        if path not in self.cur:
            raw = open(path, "rb").read()
            self.orig[path] = (raw, os.stat(path).st_mtime_ns)
            self.cur[path] = raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n")
        return self.cur[path]

    def _write(self, path: str, lf: str) -> None:
        raw, mtime = self.orig[path]
        if lf == raw.decode("utf-8", "surrogateescape").replace("\r\n", "\n"):
            if open(path, "rb").read() != raw:
                text.atomic_write(path, raw)
            os.utime(path, ns=(mtime, mtime))
            return
        out = lf.replace("\n", "\r\n") if b"\r\n" in raw else lf
        text.atomic_write(path, out.encode("utf-8", "surrogateescape"))

    def write(self, files: dict[str, str]) -> None:
        """Write a candidate: `files` over the accepted state."""
        for path, lf in files.items():
            self.text(path)
            self._write(path, lf)

    def accept(self, files: dict[str, str]) -> None:
        self.cur.update(files)

    def restore(self, original: bool = False) -> None:
        """Put the accepted state (or, `original`, the state the run found) back on disk."""
        for path in self.cur:
            if original:
                self.cur[path] = self.orig[path][0].decode("utf-8", "surrogateescape").replace("\r\n", "\n")
            self._write(path, self.cur[path])

    def changed(self) -> list[str]:
        return [p for p in self.cur
                if self.cur[p] != self.orig[p][0].decode("utf-8", "surrogateescape").replace("\r\n", "\n")]


# --------------------------------------------------------------------------------------------------
# the measurement: compile into the tool's scratch, score with the official report, the .sdata2 order
# --------------------------------------------------------------------------------------------------

@dataclass
class Measurement:
    ok: bool
    functions: dict = field(default_factory=dict)          # name -> (percent, target size)
    fuzzy: float | None = None
    error: str = ""
    sdata2: str | None = None                              # same / differs / absent / None (not checked)
    seconds: float = 0.0
    report: str | None = None

    @property
    def matched(self) -> float:
        return sum(p / 100.0 * s for p, s in self.functions.values())


def sdata2_state(obj: str, target: str) -> str:
    """`same` when the object's `.sdata2` bytes are the target's, else `differs@0xN` / `absent` / `extra`."""
    from tools.lib.binary.elf import Elf
    a, b = Elf.read(target).section_bytes(".sdata2"), Elf.read(obj).section_bytes(".sdata2")
    if a is None and b is None:
        return "same"
    if a is None:
        return "extra"
    if b is None:
        return "absent"
    if a == b:
        return "same"
    first = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
    return "differs@0x%x" % first + ("" if len(a) == len(b) else "(0x%x/0x%x)" % (len(b), len(a)))


def report_score(target: str, unit_name: str, tmpdir: str, objdiff: str, cwd: str):
    """The official metric (`lib.report.score`, the one `measure.py` and `unitscore --measure` use), its report
    written under the tool's own `tmpdir` and deleted before each run -> `score(obj) -> (functions, fuzzy, path)`."""
    def score(obj: str):
        rep = report.score(target, obj, unit_name, tmpdir, objdiff=objdiff, cwd=cwd)
        unit = report.first_unit(rep)
        funcs = {}
        for f in unit.get("functions") or []:
            size = f.get("size")
            funcs[f.get("name")] = (float(report.score_of(f)), int(size) if str(size).isdigit() else 0)
        return funcs, (unit.get("measures") or {}).get("fuzzy_match_percent"), rep.path
    return score


class Measurer:
    """One compile of the unit's real source into the scratch directory, then one score. The object is deleted
    before the compile and must be newer than the trial's start, so a stale object is never scored."""

    def __init__(self, cmd: list[str], cwd: str, objdir: str, target: str, score, sdata2: bool = False,
                 real_obj: str | None = None, report_out: str | None = None, objdiff: str | None = None,
                 root: str | None = None):
        self.cmd, self.cwd, self.objdir, self.target, self.score, self.sdata2 = cmd, cwd, objdir, target, score, sdata2
        self.real_obj, self.report_out, self.objdiff, self.root = real_obj, report_out, objdiff, root

    def refresh(self, regenerate=report.regenerate_project) -> list[str]:
        """After a kept edit: compile the unit into its real object (`real_obj`, the one `mt.py diff` reads) and, when
        the project report exists, rewrite it from the objects on disk, so neither describes the source as it was
        before the run. -> lines saying what was done (a failure is a line, never an exception)."""
        if not self.real_obj:
            return []
        os.makedirs(os.path.dirname(self.real_obj), exist_ok=True)
        rc, log, _obj = units.run_tokens(self.cmd, self.cwd, expect=self.real_obj)
        if rc != 0:
            return ["refresh: the kept source does not compile into %s (rc %d): %s" % (
                self.real_obj, rc, units.quiet(log).strip()[-200:])]
        lines = ["refresh: compiled %s" % self.real_obj]
        if self.report_out and os.path.exists(self.report_out) and self.objdiff:
            why = regenerate(self.root or self.cwd, self.objdiff, self.report_out)
            lines.append("refresh: " + (why or "rewrote %s" % self.report_out))
        return lines

    def __call__(self) -> Measurement:
        t0 = time.time()
        rc, log, obj = units.run_tokens(self.cmd, self.cwd, scratch_dir=self.objdir)
        if rc != 0:
            return Measurement(False, error=units.quiet(log).strip()[-800:] or "compile failed (rc %d)" % rc,
                               seconds=time.time() - t0)
        if os.path.getmtime(obj) < t0 - 1:
            return Measurement(False, error="the object %s did not move: refusing a stale score" % obj,
                               seconds=time.time() - t0)
        try:
            funcs, fuzzy, path = self.score(obj)
        except report.ReportError as exc:
            return Measurement(False, error=str(exc)[:800], seconds=time.time() - t0)
        m = Measurement(True, funcs, fuzzy, seconds=time.time() - t0, report=path)
        if self.sdata2:
            m.sdata2 = sdata2_state(obj, self.target)
        return m


# --------------------------------------------------------------------------------------------------
# the verdict and the run
# --------------------------------------------------------------------------------------------------

def compare(before: Measurement, after: Measurement) -> tuple[list, list]:
    """`(up, down)`: `[(name, before %, after %)]` rows that rose / fell; a row the candidate lost is a fall."""
    up, down = [], []
    for name, (p0, _s) in before.functions.items():
        p1 = after.functions.get(name, (None, 0))[0]
        if p1 is None or p1 < p0 - EPS:
            down.append((name, p0, p1))
        elif p1 > p0 + EPS:
            up.append((name, p0, p1))
    for name, (p1, _s) in after.functions.items():
        if name not in before.functions and p1 > EPS:
            up.append((name, None, p1))
    return up, down


def judge(before: Measurement, after: Measurement, sdata2: bool = False) -> tuple[bool, str, list, list]:
    """`(keep, why, up, down)`: keep only a candidate that compiled, dropped no row and raised the unit's matched
    bytes; with `sdata2`, one that turns a target-identical `.sdata2` into a different one is refused too."""
    if not after.ok:
        return False, "failed: " + (after.error.strip().splitlines() or ["no result"])[-1][:160], [], []
    up, down = compare(before, after)
    if down:
        return False, "%d row(s) drop: %s" % (len(down), ", ".join(
            "%s %.2f->%s" % (n, a, "gone" if b is None else "%.2f" % b) for n, a, b in down[:3])), up, down
    if after.matched <= before.matched + EPS:
        return False, "no gain", up, down
    if sdata2 and before.sdata2 == "same" and after.sdata2 != "same":
        return False, ".sdata2 order changed (%s)" % after.sdata2, up, down
    return True, "kept", up, down


@dataclass
class Row:
    """One candidate's outcome: the verdict, the unit's matched-bytes change, the rows that rose and fell."""
    trial: str
    name: str
    verdict: str                         # KEPT, best-of, reverted, skip, guarded, failed
    why: str = ""
    delta: float | None = None
    fuzzy: float | None = None
    up: int = 0
    down: int = 0
    sdata2: str | None = None
    seconds: float = 0.0

    def to_json(self) -> dict:
        return dict(self.__dict__)


def clock(seconds: float) -> str:
    seconds = int(seconds)
    return "%02d:%02d" % (seconds // 60, seconds % 60) if seconds < 3600 else "%dh%02d" % (seconds // 3600,
                                                                                         seconds // 60 % 60)


class TrialRunner:
    """Applies candidates one at a time to the accepted state, measuring each and keeping the winners."""

    def __init__(self, ws: Workspace, measure, unit, root: str, out=print, sdata2: bool = False,
                 shared_ok: bool = False):
        self.ws, self.measure, self.unit, self.root, self.out = ws, measure, unit, root, out
        self.sdata2, self.shared_ok = sdata2, shared_ok
        self.rows: list[Row] = []
        self.graph: dict = {}
        self.base: Measurement | None = None
        self.cur: Measurement | None = None
        self.done = 0
        self.planned = 0
        self.t0 = time.time()

    def _progress(self, row: Row) -> None:
        self.done += 1
        el = time.time() - self.t0
        per = el / max(1, self.done)
        left = max(0, self.planned - self.done) * per
        self.out("[%3d/%-3d %s eta %s] %-8s %-28s %s%s" % (
            self.done, max(self.planned, self.done), clock(el), clock(left), row.verdict, row.name[:28],
            "%+9.2f B  up %d down %d" % (row.delta, row.up, row.down) if row.delta is not None else "",
            "  " + row.why if row.why and row.verdict not in ("KEPT",) else ""))

    def _skip(self, trial: str, name: str, verdict: str, why: str, counted: bool = False) -> Row:
        row = Row(trial, name, verdict, why)
        self.rows.append(row)
        self.done += 1 if counted else 0
        self.out("            %-8s %-28s %s" % (verdict, name[:28], why))
        return row

    def evaluate(self, trial: str, name: str, files: dict[str, str], keep: bool = True) -> tuple[Row, Measurement]:
        """Write the candidate, measure it, then keep it (when `keep` and it wins) or restore the accepted state."""
        self.ws.write(files)
        m = self.measure()
        ok, why, up, down = judge(self.cur, m, self.sdata2)
        row = Row(trial, name, "KEPT" if ok and keep else ("better" if ok else
                                                            ("failed" if not m.ok else "reverted")),
                  "" if ok else (m.error.splitlines()[-1][:160] if not m.ok and m.error else why),
                  m.matched - self.cur.matched if m.ok else None, m.fuzzy, len(up), len(down), m.sdata2, m.seconds)
        if ok and keep:
            self.ws.accept(files)
            self.cur = m
        else:
            self.ws.restore()
        self.rows.append(row)
        self._progress(row)
        return row, m

    def baseline(self) -> Measurement:
        self.planned += 1
        m = self.measure()
        self.done += 1
        self.base = self.cur = m
        return m


def build_measurer(unit, root: str, rename_map: str | None = None, sdata2: bool = False, out=print):
    """The compile command (the scorer's own resolution: `lib.units.unit_tokens` + `reconcile_flags`) pointed at
    the tool's scratch, and the target (`lib.units.resolve_target`, optionally renamed by `rename_map`)."""
    main = repo.main_checkout(root)
    scratch = str(repo.scratch(SCRATCH, root) / unit.key.replace("/", "_"))
    tokens, _src = units.unit_tokens(main, root, unit.spelling)
    tokens, notes = units.reconcile_flags(tokens, root, unit.spelling)
    for n in notes:
        out("note: " + n)
    dry = units.compile(unit.spelling, main, root, dry_run=True, tokens=tokens)
    target, kind, note = units.resolve_target(root, main, unit.spelling, unit.file)
    if kind == "missing":
        raise SystemExit("tryvar: " + note)
    if note:
        out("note: " + note)
    if rename_map:
        ops = pendingmap.load(rename_map)
        pending = os.path.join(scratch, "target.pending.o")
        os.makedirs(scratch, exist_ok=True)
        outcome = pendingmap.apply_file(target, pending, ops)
        out("pending map %s: %s" % (rename_map, outcome.summary()))
        if outcome.refused:
            raise SystemExit("tryvar: --rename-map refused %d line(s)" % len(outcome.refused))
        target = pending
    score = report_score(target, unit.report_name, os.path.join(scratch, "report"),
                         report.objdiff_cli(root, main), root)
    report_out = os.path.join(root, "build", "RMHE08", "report.json")
    return Measurer(dry["command"], main, os.path.join(scratch, "obj"), target, score, sdata2,
                    real_obj=dry["object"], report_out=report_out, objdiff=report.objdiff_cli(root, main),
                    root=root), target
