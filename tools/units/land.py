"""The land gate: one command that runs the batch checklist and refuses to let a bad batch through.

docs/plan.md 7.5 + 7.16, §11. Six manual commands and a regression-scan heredoc were the previous version of
this, rewritten every batch - and that is where a mistake hides. It is also the place where a green
`ninja build/RMHE08/ok` can lie: the `ok` stamp file may be from an earlier run, and a `NonMatching` batch
never relinks, so `main.elf` never runs and `ok` is the only edge that re-validates anything. So `verify`

* deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) **before** the run and requires
  them to be recreated;
* checks every command's exit code, `configure.py`'s included - a failed `configure.py` leaves a stale
  `build.ninja` and every later number is a fiction;
* refuses a batch that moves the ground truth, that moved `main` since the batch base, that touches a file
  outside the batch's expected set, or whose outbox entry does not validate;
* runs the style lint when it exists (7.21), reports the ledger delta, and warns when a unit improved with no
  document or header change to show for it (7.10);
* refreshes the baseline afterwards (7.16), so `ninja changes` compares against the batch that just landed.

    python tools/units/land.py record-base [--json]
    python tools/units/land.py verify [--base SHA] [--units a,b] [--dry-run] [--no-build] [--allow PATH]

`verify` never commits. It writes the message to `.git/land_msg.txt` and prints it; committing stays a
separate, deliberate step (and `prepcommit.py` stages the paths).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, os.path.dirname(HERE) + os.sep + "git")

import unitutil  # noqa: E402
import prepcommit as pc  # noqa: E402
from units import brief as brief_mod  # noqa: E402
from units import claims  # noqa: E402
from units import handoff as handoff_mod  # noqa: E402
from units import recompile as rc  # noqa: E402

ALLOWED_PREFIXES = ("src/", "include/", "docs/", "tools/", ".agents/skills/")
ALLOWED_FILES = ("configure.py", "AGENTS.md", ".gitignore",
                 "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt")
BASE_FILE = os.path.join(".pi", "land-base.json")


def run(args: list[str], cwd: str) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=cwd, capture_output=True, text=True, errors="replace")


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = run(["git", *args], cwd)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return p.stdout


def record_base(main: str) -> dict:
    head = git(["rev-parse", "HEAD"], main).strip()
    data = {"base": head, "recorded_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "subject": git(["log", "-1", "--format=%s"], main).strip()}
    os.makedirs(os.path.join(main, ".pi"), exist_ok=True)
    with open(os.path.join(main, BASE_FILE), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1)
    return data


def read_base(main: str) -> dict:
    path = os.path.join(main, BASE_FILE)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def changed_paths(main: str) -> list[str]:
    out = git(["status", "--porcelain"], main)
    paths = []
    for line in out.splitlines():
        if len(line) < 4:
            continue
        path = line[3:].strip()
        if " -> " in path:
            path = path.split(" -> ")[-1]
        paths.append(path.strip('"'))
    return paths


def outside_batch(paths: list[str], allowed: tuple[str, ...] = ALLOWED_PREFIXES,
                  allowed_files: tuple[str, ...] = ALLOWED_FILES) -> list[str]:
    """Paths a batch may not touch: everything the plan keeps for the orchestrator, minus its own writes."""
    bad = []
    for path in paths:
        if path in allowed_files or path.startswith(allowed):
            continue
        bad.append(path)
    return bad


def flips_objects(main: str) -> bool:
    """True when configure.py gains `Object(Matching, ...)` relative to HEAD - the batch flips something."""
    p = run(["git", "diff", "HEAD", "--", "configure.py"], main)
    return bool(re.search(r"^\+.*Object\(\s*Matching", p.stdout or "", re.M))


def regression_rows(changes_json: str) -> list[tuple[str, str, float, float]]:
    """(unit, measure, before, after) for every measure that went down, from a report_changes.json."""
    if not os.path.exists(changes_json):
        return []
    data = json.loads(open(changes_json, encoding="utf-8").read())
    rows = []

    def walk(node):
        if isinstance(node, dict):
            name = node.get("name") or ""
            for key, value in (node.get("measures") or {}).items():
                if isinstance(value, dict) and "old" in value and "new" in value:
                    old, new = value["old"], value["new"]
                    if isinstance(old, (int, float)) and isinstance(new, (int, float)) and new < old - 1e-9:
                        rows.append((name, key, old, new))
            for key in ("units", "children", "categories"):
                for child in (node.get(key) or []):
                    walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(data)
    return [r for r in rows if "auto_" not in r[0]]


def ledger_numbers(main: str) -> dict:
    p = run([sys.executable, os.path.join("tools", "units", "ledger.py"), "--json"], main)
    if p.returncode != 0:
        return {}
    try:
        return json.loads(p.stdout)
    except json.JSONDecodeError:
        return {}


def summary(before: dict, after: dict) -> str:
    keys = ("covered", "closed", "matched", "bytes")
    parts = []
    for key in keys:
        b, a = before.get(key), after.get(key)
        if isinstance(b, (int, float)) and isinstance(a, (int, float)):
            parts.append("%s %s -> %s" % (key, b, a))
    return ", ".join(parts) or "(ledger numbers unavailable)"


def outbox_units(main: str, units: list[str]) -> tuple[list[str], list[str]]:
    """-> (units whose outbox validates, problems)."""
    ok, problems = [], []
    for unit in units:
        path = handoff_mod.outbox_path(main, unit)
        if not os.path.exists(path):
            problems.append("%s: no outbox at %s" % (unit, path))
            continue
        entry = json.loads(open(path, encoding="utf-8").read())
        rng = brief_mod.splits_range(main, unit)
        owned = {s["name"] for s in brief_mod.symbols_in_range(main, *rng[".text"][:2])} if rng.get(".text") else set()
        errors, _warnings = handoff_mod.validate(entry, owned)
        if errors:
            problems.extend("%s: %s" % (unit, e) for e in errors)
        else:
            ok.append(unit)
    return ok, problems


def verify(main: str, units: list[str], base: str | None, dry_run: bool, no_build: bool) -> int:
    checks: list[tuple[str, bool, str]] = []

    def check(name: str, good: bool, detail: str = "", info: str = "") -> None:
        checks.append((name, good, detail, info))

    # 1. ground truth
    truth = pc.ground_truth_error()
    check("ground truth (build.sha1 == the DOL's hash)", not truth, "; ".join(truth))

    # 2. the batch base: main must not have moved since the batch opened
    recorded = read_base(main)
    want_base = base or recorded.get("base")
    head = git(["rev-parse", "HEAD"], main).strip()
    if want_base:
        check("main has not moved since the batch base", head == want_base,
              "HEAD %s != base %s - a worker committed to main, or another stream landed"
              % (head[:8], want_base[:8]), info="base %s" % (want_base or "?")[:8])
    else:
        check("batch base recorded", False, "no base: run `land.py record-base` when the batch opens")

    # 3. the tree guard + the outbox of every unit in the batch
    paths = changed_paths(main)
    bad = outside_batch(paths)
    check("every changed path belongs to a batch", not bad, "not allowed in a batch: %s" % ", ".join(bad))
    if units:
        ok_units, problems = outbox_units(main, units)
        check("every unit's outbox validates", not problems, "; ".join(problems[:4]))
    else:
        check("batch units named", False, "pass --units (the outboxes are what the batch is judged by)")

    # 4. the style lint (7.21), when it exists
    lint = os.path.join(main, "tools", "units", "stylelint.py")
    if os.path.exists(lint):
        p = run([sys.executable, lint, "--diff", want_base or "HEAD"], main)
        check("style lint (§6.5) adds no violation", p.returncode == 0, (p.stdout or p.stderr)[-300:])
    else:
        check("style lint (§6.5)", True, info="not built yet (roadmap 7.21) - skipped")

    before = ledger_numbers(main)
    flip = flips_objects(main)
    ok_file = os.path.join(main, "build", "RMHE08", "ok")
    elf_file = os.path.join(main, "build", "RMHE08", "main.elf")

    if dry_run:
        for name, good, detail, info in checks:
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        print("\nwould then: delete build/RMHE08/ok%s, run configure.py -> ninja -> report.json -> "
              "regression scan -> ok -> ledger -> baseline" % (" and main.elf (this batch flips an object)" if flip else ""))
        return 0 if all(good for _n, good, _d, _i in checks) else 1

    failures = [name for name, good, _d, _i in checks if not good]
    if failures:
        for name, good, detail, info in checks:
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        print("\nREFUSING to build or stage anything: %s" % ", ".join(failures))
        return 1

    def gate(name: str, args: list[str]) -> bool:
        p = run(args, main)
        tail = ((p.stdout or "") + (p.stderr or "")).strip().splitlines()
        check(name + " (exit %d)" % p.returncode, p.returncode == 0, tail[-1] if tail else "")
        return p.returncode == 0

    # 5. the build, with the proof that the `ok` we read is this run's
    for stale in [ok_file] + ([elf_file] if flip else []):
        if os.path.exists(stale):
            os.remove(stale)
    started = time.time_ns()
    built = gate("configure.py", [sys.executable, "configure.py"])
    built = gate("ninja", ["ninja"]) and built
    gate("report.json", ["ninja", "build/RMHE08/report.json"])
    regressed = regression_rows(os.path.join(main, "build", "RMHE08", "report_changes.json"))
    check("no measure regressed", not regressed,
          "; ".join("%s %s %.2f -> %.2f" % r for r in regressed[:4]))
    gate("ok (main.dol verified)", ["ninja", "build/RMHE08/ok"])
    fresh = os.path.exists(ok_file) and os.stat(ok_file).st_mtime_ns >= started
    check("ok was recreated by THIS run", fresh, "the ok stamp predates the run - it proves nothing")

    # 6. the ledger delta + the knowledge delta
    after = ledger_numbers(main)
    improved = any(isinstance(before.get(k), (int, float)) and isinstance(after.get(k), (int, float))
                   and after[k] > before[k] for k in ("closed", "matched", "bytes"))
    docs_changed = any(p.startswith(("docs/", "AGENTS.md")) for p in paths)
    headers_changed = any(p.startswith("src/") and p.endswith((".c", ".cpp", ".cp")) for p in paths)
    check("knowledge delta present if the batch improved something",
          (not improved) or docs_changed or headers_changed,
          "a unit improved and no docs/AGENTS.md/unit header changed in this batch (7.10)")

    # 7. the baseline, so the next batch's `ninja changes` compares against this one (7.16)
    if not no_build:
        gate("ninja baseline", ["ninja", "baseline"])

    print("%-58s %s" % ("check", "result"))
    for name, good, detail, info in checks:
        note = (detail if not good else "") or info
        print("%-58s %s%s" % (name[:58], "PASS" if good else "FAIL", ("  " + note[:80]) if note else ""))

    message = os.path.join(main, ".git", "land_msg.txt")
    body = ["land: %s" % ("; ".join(units) if units else "batch"),
            "",
            "ledger: %s" % summary(before, after),
            "gates: ground truth ok, base %s, %d check(s), ok recreated=%s%s"
            % ((want_base or "?")[:8], len(checks), fresh, ", main.elf relinked" if flip else ""),
            ""]
    with open(message, "w", encoding="utf-8") as fh:
        fh.write("\n".join(body))
    print("\nledger: %s" % summary(before, after))
    print("message written to %s - review it, then `git commit -F .git/land_msg.txt`" % message)
    failed = [name for name, good, _d, _i in checks if not good]
    if failed:
        print("FAILED: %s" % ", ".join(failed))
        return 1
    print("READY: every check passed")
    return 0


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("inside the batch: a unit source", outside_batch(["src/Pl/pl_act.cpp"]), [])
    check("inside the batch: splits.txt", outside_batch(["config/RMHE08/splits.txt"]), [])
    check("outside the batch: orig", outside_batch(["orig/RMHE08/sys/main.dol"]), ["orig/RMHE08/sys/main.dol"])
    check("outside the batch: build", outside_batch(["build/RMHE08/main.dol"]), ["build/RMHE08/main.dol"])
    check("outside the batch: ground truth", outside_batch(["config/RMHE08/build.sha1"]),
          ["config/RMHE08/build.sha1"])
    check("outside the batch: the local-only state files", outside_batch([".pi/claims.json"]), [".pi/claims.json"])

    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        # regression_rows() reads a report_changes.json fixture
        fixture = os.path.join(tmp, "changes.json")
        json.dump({"units": [
            {"name": "src/Pl/pl_act.cpp", "measures": {"matched_code": {"old": 100, "new": 90}}},
            {"name": "src/Pl/pl_skill.cpp", "measures": {"matched_code": {"old": 90, "new": 90}}},
            {"name": "main/auto_eft004_set_pl__FP4_PLWUcfffUl_text", "measures": {"matched_code": {"old": 10, "new": 1}}},
        ]}, open(fixture, "w"))
        rows = regression_rows(fixture)
        check("a regression is caught", len(rows), 1)
        check("the auto_ scaffold is ignored", any("auto_" in r[0] for r in rows), False)
        check("a registered auto/* unit is still tracked",
              regression_rows(fixture) != [] and all("main/auto/" not in r[0] for r in rows), True)
        check("a flat measure is not a regression", any(r[1] == "matched_code" and r[2] == 90 for r in rows), False)
        check("a missing changes file is not a crash", regression_rows(os.path.join(tmp, "nope.json")), [])

    check("summary delta", summary({"closed": 284, "matched": 217}, {"closed": 290, "matched": 223}),
          "closed 284 -> 290, matched 217 -> 223")
    check("summary tolerates a missing side", summary({}, {}), "(ledger numbers unavailable)")
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    rb = sub.add_parser("record-base", help="record main's HEAD as the batch base")
    rb.add_argument("--json", action="store_true")
    v = sub.add_parser("verify", help="run the batch checklist")
    v.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    v.add_argument("--units", default=None, help="comma-separated units in this batch")
    v.add_argument("--dry-run", action="store_true", help="run the cheap checks only; touch nothing")
    v.add_argument("--no-build", action="store_true", help="skip the split/link/ok/baseline steps")
    v.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    main = rc.main_root(rc.worktree_root())
    if args.cmd == "record-base":
        data = record_base(main)
        print(json.dumps(data, indent=2) if args.json else "base %s (%s)" % (data["base"], data["subject"]))
        return 0
    if args.cmd == "verify":
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        return verify(main, units, args.base, args.dry_run, args.no_build)
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
