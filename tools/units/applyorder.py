#!/usr/bin/env python3
"""Write regsim's declaration orders into a function's source, keep each only on a strict gain, round after round.
Spec: docs/tools/spec/applyorder.md. CLI: applyorder.py row UNIT FUNCTION | batch ROWS_FILE [--plans N] [--rounds N]
[--workers N] [--dry-run] [--restore] (also `regsim.py run ... --apply-order [--rows-file F]`)."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import contextlib
import difflib
import json
import os
import re
import subprocess
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

from tools.lib import regsim, regsimorder, repo, report, trialrun, units

#: a row's last word: what happened to it
STATUSES = ("KEPT", "SAME", "REVERTED", "FEWFS", "BADLINE", "NODEF", "FULL", "DRYRUN", "ERROR")
#: the per-row logs and the report lock live under this tree's `.pi/tmp/`
_PCT_ABOVE = 99.99


@dataclass
class Row:
    """One line of a row list: `size,unit,function,pct`."""
    size: int
    unit: str
    function: str
    pct: float


@dataclass
class Result:
    """What one row came to: `status` (STATUSES), `detail` (one line), the function's score before and after."""
    status: str
    detail: str = ""
    unit: str = ""
    function: str = ""
    before: float | None = None
    after: float | None = None
    matched: float = 0.0
    rounds: int = 0
    kept: list = field(default_factory=list)

    def line(self) -> str:
        score = "" if self.before is None else " %.4f -> %s" % (self.before, "?" if self.after is None else "%.4f" % self.after)
        return "%-8s %s %s:%s %s" % (self.status, self.unit, self.function, score, self.detail)


# --- the row list -----------------------------------------------------------------------------------------------

_ADDRESS = re.compile(r"^0x[0-9A-Fa-f]{8}$")


def parse_row(line: str) -> Row | None:
    """`size,unit,function,pct` -> Row; the function may contain commas (cut from the left twice, the pct from the
    right) and a trailing `,0xADDRESS` column of `remaining_functions.csv` is dropped. None for a blank line, a
    comment or a header."""
    text = line.strip()
    if not text or text.startswith("#"):
        return None
    head, _, tail = text.rpartition(",")
    if _ADDRESS.match(tail.strip()):
        text = head
    size, sep, rest = text.partition(",")
    unit, sep2, rest = rest.partition(",")
    if not (sep and sep2) or not size.strip().isdigit():
        return None
    function, sep3, pct = rest.rpartition(",")
    if not sep3:
        return None
    try:
        value = float(pct)
    except ValueError:
        return None
    return Row(int(size), unit.strip(), function.replace("\\", "").strip(), value)


def read_rows(path: str) -> list[Row]:
    with open(path, encoding="utf-8", errors="replace") as fh:
        return [r for r in map(parse_row, fh) if r]


def partition(rows: list[Row], workers: int) -> list[list[Row]]:
    """Up to `workers` lists of rows, every unit whole in one list (never two writers on one unit); the biggest unit
    goes to the emptiest list first."""
    by_unit: dict[str, list[Row]] = {}
    for r in rows:
        by_unit.setdefault(r.unit, []).append(r)
    lanes: list[list[Row]] = [[] for _ in range(max(1, min(workers, len(by_unit))))]
    for group in sorted(by_unit.values(), key=len, reverse=True):
        min(lanes, key=len).extend(group)
    return [lane for lane in lanes if lane]


def current_scores(root: str) -> "report.Report":
    return report.read(os.path.join(root, "build", "RMHE08", "report.json"))


# --- one row ----------------------------------------------------------------------------------------------------

def unified(old: str, new: str, name: str) -> str:
    return "".join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), "a/" + name, "b/" + name, n=1))


def regsim_command(args, unit: str, function: str, out_json: Path, root: str | None = None) -> list[str]:
    """`regsim.py run` for one dump and search, its plans written to `out_json` (`--plans-json`)."""
    cmd = [sys.executable, str(Path(root or repo.repo_root()) / "tools" / "mwcc-debugger" / "regsim.py"), "run", unit, function,
           "--plans-json", str(out_json), "--top", str(max(args.top, args.plans)), "--max-evals", str(args.max_evals),
           "--pass", str(args.pass_no), "--seed", str(args.seed), "--timeout", str(args.timeout)]
    for flag, value in (("--gdb", args.gdb), ("--size", args.size), ("--params", args.params), ("--want", args.want)):
        if value is not None:
            cmd += [flag, str(value)]
    cmd += [f for f, on in (("--saved-only", args.saved_only), ("--no-edits", args.no_edits)) if on]
    return cmd


class Plan:
    """A plan read back from `regsim.py run --plans-json`: its declaration order as vregs, the rewrite cost, its edits."""

    def __init__(self, entry: dict):
        self.distance = entry["distance"]
        self.layout = type("Layout", (), {"decl": [d["vreg"] for d in entry["decl"]], "edits": tuple(entry["edits"])})()
        self.entries = entry["decl"]


def read_plans(data: dict, text: str, function: str) -> tuple[list[Plan], dict]:
    """(the plans, {vreg: SourceLocal}): each declared entry (name, line, scope) is found again in the source's locals."""
    locs = regsim.source_locals(text, function)
    mapping = {}
    for entry in data["plans"]:
        for d in entry["decl"]:
            if d["line"] is None:
                continue
            hit = [loc for loc in locs if (loc.name, loc.line, loc.scope) == (d["name"], d["line"], d["scope"])]
            if hit:
                mapping[d["vreg"]] = hit[0]
    return [Plan(e) for e in data["plans"]], mapping


def dump_and_search(args, out, rnd: int, function: str, text: str):
    """One round's regsim: the dump, the verify and the search of the source as it stands (a `regsim.py run` child whose
    output goes to the log) -> (symbol, plans, mapping); `SystemExit(reason)` when it produced none."""
    root = repo.repo_root()
    tmp = repo.scratch("applyorder", root) / ("plans-%d-%s.json" % (os.getpid(), re.sub(r"\W", "_", function)[:60]))
    if tmp.exists():
        tmp.unlink()
    p = subprocess.run(regsim_command(args, args.unit, function, tmp, root), capture_output=True, text=True, encoding="utf-8",
                       errors="replace", cwd=root, timeout=args.timeout + 600)
    log = (p.stdout or "") + (p.stderr or "")
    if p.returncode != 0 or not tmp.exists():
        raise SystemExit((log.strip().splitlines() or ["regsim produced nothing"])[-1][:300])
    data = json.loads(tmp.read_text(encoding="utf-8"))
    tmp.unlink()
    plans, mapping = read_plans(data, text, data["function"])
    return data["function"], plans, mapping


def try_plans(runner, path: str, text: str, plans: list, mapping: dict, symbol: str, tried: set, refusals: list,
              dry_run: bool, out, rnd: int = 1) -> list[str]:
    """Writes each plan's spellings in turn (`regsimorder.spellings`), `unittry`'s judge keeping the first strict gain:
    -> [what was kept] (at most one: the source changed, so the plans are stale). A plan with rewrite cost 0 is the
    source's own order. The spellings and refusals are recorded in `tried` / `refusals`."""
    for number, plan in enumerate(plans, 1):
        if plan.distance == 0:
            refusals.append("plan %d: the source's own order" % number)
            continue
        spells, why = regsimorder.spellings(plan.layout.decl, mapping, text, symbol, plan.layout.edits)
        for label, exc in why:
            refusals.append("plan %d %s: %s" % (number, label, exc.reason))
            if dry_run:
                out("plan %d %s: refused - %s" % (number, label, exc.reason))
        for sp in spells:
            if sp.text in tried:
                continue
            tried.add(sp.text)
            if dry_run:
                out("plan %d %s: %s\n%s" % (number, sp.label, sp.summary, unified(text, sp.text, Path(path).name)))
                continue
            row, _m = runner.evaluate("plan%d-%s" % (number, sp.label), "%s plan %d" % (sp.label, number), {path: sp.text})
            if row.verdict == "KEPT":
                return ["round %d plan %d %s: %s" % (rnd, number, sp.label, sp.summary)]
    return []


def run_row(args, out=print) -> Result:
    """Dump, search and write the plans of `args.unit`/`args.function` round by round: a spelling is measured as the
    unit's matched bytes with no row down (`unittry`) and kept only on a strict gain; the rest is restored."""
    root = repo.repo_root()
    unit = units.Unit.resolve(args.unit, root)
    res = Result("ERROR", unit=unit.key, function=args.function)
    if not os.path.exists(unit.source):
        res.detail = "no source %s" % unit.source
        return res
    probe = Path(unit.source).read_text(encoding="utf-8", errors="surrogateescape").replace("\r\n", "\n")
    if not args.function.startswith("0x") and regsim.find_definition(probe, args.function) is None:
        res.status, res.detail = "NODEF", "the source defines no function `%s`" % args.function
        return res
    path = os.path.normpath(unit.source)
    ws = trialrun.Workspace({path: "source"})
    measure, _target = trialrun.build_measurer(unit, root, out=out)
    runner = trialrun.TrialRunner(ws, measure, unit, root, out)
    base = runner.baseline()
    if not base.ok:
        res.detail = "the unit as it stands does not measure: %s" % base.error.strip().splitlines()[-1:]
        return res
    tried: set[str] = set()
    refusals: list[str] = []
    symbol = args.function
    seen_plan = False
    failure: tuple[str, str] | None = None
    try:
        for rnd in range(1, args.rounds + 1):
            res.rounds = rnd
            text = ws.text(path)
            try:
                symbol, plans, mapping = dump_and_search(args, out, rnd, args.function, text)
            except (SystemExit, subprocess.TimeoutExpired) as exc:
                failure = ("ERROR", str(getattr(exc, "code", exc))[:300])
                break
            if not mapping and plans:
                failure = ("BADLINE", "the dump's named locals do not line up with the source's declarations")
                break
            if regsim.find_definition(text, symbol) is None:
                failure = ("NODEF", "the source defines no function `%s`" % symbol)
                break
            seen_plan = seen_plan or bool(plans)
            kept = try_plans(runner, path, text, plans[:args.plans], mapping, symbol, tried, refusals,
                             args.dry_run, out, rnd)
            res.kept += kept
            progress = bool(kept)
            if args.dry_run or not progress:
                break
            tried.clear()
    finally:
        ws.restore(original=bool(args.restore or args.dry_run))
    final = runner.cur
    res.matched = final.matched - base.matched
    res.before = base.functions.get(symbol, (None, 0))[0]
    res.after = final.functions.get(symbol, (None, 0))[0]
    if res.kept:
        res.status = "KEPT"
        res.detail = "%+.2f B matched; %s" % (res.matched, "; ".join(res.kept))
        if not args.restore and not args.no_refresh:
            for line in locked_refresh(measure, root):
                out(line)
    elif failure:
        res.status, res.detail = failure
    elif args.dry_run:
        res.status, res.detail = "DRYRUN", "%d spelling(s) shown, nothing written or measured" % len(tried)
        if not tried:
            res.status, res.detail = classify(refusals, seen_plan)
    elif tried:
        res.status, res.detail = "REVERTED", "%d spelling(s) measured, none gained%s" % (
            len(tried), "".join("; " + r for r in refusals if not _OWN.search(r))[:240])
    else:
        res.status, res.detail = classify(refusals, seen_plan)
    return res


_OWN = re.compile(r"source's own order|plan's order is the source's own|no scope lists two locals")


def classify(refusals: list[str], seen_plan: bool) -> tuple[str, str]:
    """The status of a row that measured nothing, from the refusals of its plans."""
    real = [r for r in refusals if not _OWN.search(r)]
    if real:
        return "BADLINE", real[0]
    if any("no scope lists two locals" in r for r in refusals):
        return "FEWFS", "no scope declares two locals the plan could reorder"
    return "SAME", "the best plan is the source's own order" if seen_plan else "the search found no plan"


def locked_refresh(measure, root: str) -> list[str]:
    """`measure.refresh()` (the unit's object and `build/RMHE08/report.json`) one process at a time."""
    lock = repo.scratch("applyorder", root) / "refresh.lock"
    deadline = time.time() + 600
    while True:
        try:
            fd = os.open(str(lock), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            break
        except FileExistsError:
            if time.time() > deadline:
                with contextlib.suppress(OSError):
                    os.remove(lock)
            time.sleep(1.0)
    try:
        return measure.refresh()
    finally:
        os.close(fd)
        with contextlib.suppress(OSError):
            os.remove(lock)


# --- the command lines -------------------------------------------------------------------------------------------

def row_cli(args) -> int:
    """`regsim.py run UNIT FUNCTION --apply-order`: one row, the human line and, with `--json`, a `RESULT {...}` line."""
    res = run_row(args)
    print(res.line())
    if getattr(args, "json", False):
        print("RESULT " + json.dumps(res.__dict__))
    return {"KEPT": 0, "SAME": 0, "REVERTED": 0, "FEWFS": 0, "FULL": 0, "DRYRUN": 0}.get(res.status, 3 if res.status == "BADLINE" else 2)


def forwarded(args) -> list[str]:
    """The options a row process takes from its batch (every one `add_arguments` defines but the batch's own)."""
    cmd = ["--plans", str(args.plans), "--rounds", str(args.rounds), "--top", str(args.top), "--max-evals",
           str(args.max_evals), "--pass", str(args.pass_no), "--seed", str(args.seed), "--timeout", str(args.timeout)]
    for flag, value in (("--gdb", args.gdb), ("--size", args.size), ("--params", args.params), ("--want", args.want)):
        if value is not None:
            cmd += [flag, str(value)]
    cmd += [f for f, on in (("--saved-only", args.saved_only), ("--no-edits", args.no_edits), ("--dry-run", args.dry_run),
                            ("--restore", args.restore), ("--no-refresh", args.no_refresh)) if on]
    return cmd


def child_command(args, row: Row) -> list[str]:
    """The `applyorder.py row` command line one batch row runs as."""
    return [sys.executable, str(Path(__file__).resolve()), "row", row.unit, row.function, "--json"] + forwarded(args)


def run_child(args, row: Row, log_dir: Path) -> Result:
    log = log_dir / (re.sub(r"\W", "_", row.unit + "__" + row.function)[:120] + ".log")
    try:
        p = subprocess.run(child_command(args, row), capture_output=True, text=True, encoding="utf-8", errors="replace",
                           cwd=repo.repo_root(), timeout=args.row_timeout)
        text = (p.stdout or "") + (p.stderr or "")
    except subprocess.TimeoutExpired:
        text = "TIMEOUT after %ss" % args.row_timeout
    log.write_text(text, encoding="utf-8")
    m = re.findall(r"^RESULT (\{.*\})$", text, re.M)
    if m:
        data = json.loads(m[-1])
        return Result(**data)
    return Result("ERROR", (text.strip().splitlines() or ["no output"])[-1][:200], row.unit, row.function)


def batch_cli(args) -> int:
    """`--rows-file`: every row of the list once, up to `--workers` units at a time (never two writers on a unit), one
    line a row as it completes and a summary. A row already at 100 % in `build/RMHE08/report.json` is FULL."""
    root = repo.repo_root()
    rows = read_rows(args.rows_file)
    if not rows:
        print("applyorder: no rows in %s" % args.rows_file)
        return 2
    rep = current_scores(root)
    log_dir = Path(root) / ".pi" / "tmp" / "applyorder"
    log_dir.mkdir(parents=True, exist_ok=True)
    lock = threading.Lock()
    results: list[Result] = []

    def emit(res: Result) -> None:
        with lock:
            results.append(res)
            print(res.line(), flush=True)

    todo = []
    for r in rows:
        try:
            u = units.Unit.resolve(r.unit, root)
        except SystemExit:
            emit(Result("NODEF", "no such unit", r.unit, r.function))
            continue
        now = rep.functions(u.report_name).get(r.function)
        if now is not None and now >= _PCT_ABOVE:
            emit(Result("FULL", "already 100%% in report.json", r.unit, r.function, now, now))
        else:
            todo.append(r)

    def work(lane: list[Row]) -> None:
        for r in lane:
            emit(run_child(args, r, log_dir))

    lanes = partition(todo, args.workers)
    print("applyorder: %d row(s), %d to run in %d worker(s) over %d unit(s); logs %s" % (
        len(rows), len(todo), len(lanes), len({r.unit for r in todo}), log_dir), flush=True)
    # plain threads: `tools/units/queue.py` shadows the standard `queue` when this file runs as a script
    threads = [threading.Thread(target=work, args=(lane,)) for lane in lanes]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    counts = {s: sum(1 for x in results if x.status == s) for s in STATUSES}
    print("summary: " + ", ".join("%s %d" % (s, n) for s, n in counts.items() if n) + "; %d row(s), %+.2f B matched in all"
          % (len(results), sum(x.matched for x in results)))
    return 0 if not counts["ERROR"] else 2


def add_arguments(p: argparse.ArgumentParser) -> None:
    """The options of a row and of a batch (those from `--gdb` on go to `regsim.py run` unchanged)."""
    p.add_argument("--plans", type=int, default=3, help="the best N plans of each round are written (default 3)")
    p.add_argument("--rounds", type=int, default=4, help="rounds (dump, search, write) until one keeps nothing (default 4)")
    p.add_argument("--dry-run", action="store_true", help="print every spelling as a diff and every refusal, write nothing")
    p.add_argument("--restore", action="store_true", help="measure and put every source back as it was found")
    p.add_argument("--no-refresh", action="store_true", help="do not rebuild the object and report.json after a keep")
    p.add_argument("--workers", type=int, default=3, help="batch: unit-disjoint workers (default 3)")
    p.add_argument("--row-timeout", type=float, default=3600.0, help="batch: seconds a row may take")
    p.add_argument("--json", action="store_true", help="a final `RESULT {...}` line")
    p.add_argument("--gdb", help="the gdb to use (default: this tree's .pi/tmp/mwcc-dbg, PATH, else fetched)")
    p.add_argument("--size", help="the symbols.txt size that picks one of several functions of a name")
    p.add_argument("--timeout", type=float, default=900.0, help="seconds the debugger session may take")
    p.add_argument("--pass", dest="pass_no", type=int, default=1, help="the colouring pass (default 1)")
    p.add_argument("--params", type=int, help="how many parameters (default: read from the source)")
    p.add_argument("--want", help="targets by hand, rVREG=rPHYS,... (instead of retail's object)")
    p.add_argument("--saved-only", action="store_true", help="aim at the callee-saved registers only")
    p.add_argument("--no-edits", action="store_true", help="declaration orders only, no source edits")
    p.add_argument("--max-evals", type=int, default=20000, help="simulations per order search")
    p.add_argument("--top", type=int, default=3, help="plans the search prints (at least --plans are used)")
    p.add_argument("--seed", type=int, default=0)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    row = sub.add_parser("row", help="one function")
    row.add_argument("unit")
    row.add_argument("function")
    batch = sub.add_parser("batch", help="a row list")
    batch.add_argument("rows_file", metavar="ROWS_FILE")
    for p in (row, batch):
        add_arguments(p)
    args = ap.parse_args(argv)
    if args.cmd == "batch":
        return batch_cli(args)
    return row_cli(args)


if __name__ == "__main__":
    raise SystemExit(main())
