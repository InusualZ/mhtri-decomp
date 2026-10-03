#!/usr/bin/env python3
"""Audit the `refs/rescue/*` safety net and prune only what is provably redundant.

`claims.py release` / `timeout` copy a branch's unlanded commits to `refs/rescue/<slug>` **before**
deleting the branch, because the branch is the lock and the work it held must not vanish with it.  The
refs are a safety net, not debris: some of them hold the only copy of work no landing ever took.  They
also accumulate - 193 of them by 2026-09-27 - and nothing in the campaign had ever looked at them.

    python tools/units/rescue.py audit [--prune] [--json] [--full-diff] [--ref REF]...
                                       [--repo PATH] [--main REF]

For every ref the audit reports

* the ref name and the tip's date and subject;
* **the unit(s) the ref registers**, derived from the ref's registration **diff against its merge-base
  with `main`** - the `Object(...)` rows and `splits.txt` unit headers that ref *added*.  A whole-file
  string match against `main` ('every name in the ref's configure.py that also appears in main') matches
  every unit in the file and is useless; the diff is what names the ref's own registration.  When the
  registration diff is empty (a ref that only *edits* a registration main already had) the units are read
  from the `src/**` paths the ref touched and the derivation is labelled `touched-path`;
* whether each unit is registered on `main` today (an `Object(...)` row **and** a `splits.txt` block - the
  two halves `verifyunit.registration_problems` asserts);
* the content diff of those touched paths against `main`.

and classifies it:

* ``redundant``        - every unit is on `main` and every touched path matches `main`; nothing is missing.
* ``landed-with-drift``- every unit is on `main` but the paths differ: `main` has moved on since.
* ``unlanded``         - at least one unit is **not** on `main`; this ref may hold the only copy.  Never pruned.
* ``unknown``          - no merge-base with `main`, or nothing parseable to derive a unit from.  Never pruned.

`--prune` deletes **only** `redundant` refs and prints each deletion; without it the audit is strictly
read-only.  `landed-with-drift`, `unlanded` and `unknown` are never touched, prune or not.

The classification is deliberately conservative: a unit *renamed* on `main` (the `auto/` placeholders that
migrated to their final homes) still fails the by-name registration check and lands in `unlanded`, where it
is surfaced and kept - the safe direction.  The point is to stop a future teardown from ever deleting the
last copy of work, not to reclaim disk.

    python tools/units/rescue.py --selftest
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import subprocess
import sys
from tools.lib.git import Git

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from units import unionresolve as ur  # noqa: E402
from units import verifyunit as vu  # noqa: E402

DEFAULT_PREFIX = "refs/rescue/"
CONFIGURE = "configure.py"
SPLITS = "config/RMHE08/splits.txt"

VERDICT_REDUNDANT = "redundant"
VERDICT_DRIFT = "landed-with-drift"
VERDICT_UNLANDED = "unlanded"
VERDICT_UNKNOWN = "unknown"
VERDICTS = (VERDICT_REDUNDANT, VERDICT_DRIFT, VERDICT_UNLANDED, VERDICT_UNKNOWN)

DERIVATION_REGISTERED = "registered"
DERIVATION_TOUCHED = "touched-path"
DERIVATION_NONE = "none"

# A `src/**` path names a unit once the `src/` prefix and the source extension are stripped: the same
# normalisation `verifyunit.unit_stem` applies to a `configure.py`/`splits.txt` name.
_SRC_EXTS = (".c", ".cpp", ".cp", ".cxx", ".cc", ".c++", ".C")


# --------------------------------------------------------------------------------------------------
# git plumbing (read-only except `delete_ref`)
# --------------------------------------------------------------------------------------------------

def _run(repo: str, args: list[str]) -> subprocess.CompletedProcess:
    return Git(repo).run(*args)


def git(repo: str, args: list[str], check: bool = True) -> str:
    p = _run(repo, args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), repo, p.stderr.strip()))
    return p.stdout


def delete_ref(repo: str, ref: str) -> None:
    """Delete one ref - the only write this tool ever performs (and only under `--prune`)."""
    git(repo, ["update-ref", "-d", ref])


def default_repo() -> str:
    p = _run(os.getcwd(), ["rev-parse", "--show-toplevel"])
    return p.stdout.strip() or os.getcwd()


# --------------------------------------------------------------------------------------------------
# the audit
# --------------------------------------------------------------------------------------------------

def list_rescue_refs(repo: str, prefix: str) -> list[dict]:
    """Every ref under `prefix`, with its tip's short date and subject, sorted by name."""
    out = git(repo, ["for-each-ref", "--sort=refname",
                     "--format=%(refname)%1f%(committerdate:short)%1f%(objectname)%1f%(subject)",
                     prefix], check=False)
    rows: list[dict] = []
    for line in out.splitlines():
        if not line.strip():
            continue
        parts = line.split("\x1f")
        if len(parts) < 4:
            continue
        ref, date, tip, subject = parts[0], parts[1], parts[2], parts[3]
        rows.append({"ref": ref, "date": date, "tip": tip, "subject": subject})
    return rows


def merge_base(repo: str, main_ref: str, ref: str) -> str | None:
    """The merge-base of `main_ref` and `ref`, or None when the histories are unrelated."""
    return Git(repo).merge_base(main_ref, ref)


def added_registrations(repo: str, base: str, ref: str) -> tuple[list[str], list[str]]:
    """The units a ref *added* vs `base`: (`configure.py` Object names, `splits.txt` unit headers)."""
    conf_out = git(repo, ["diff", base, ref, "--", CONFIGURE], check=False)
    objects: list[str] = []
    for line in conf_out.splitlines():
        if line.startswith("+") and not line.startswith("+++"):
            objects.extend(ur.object_names(line[1:]))

    splits_out = git(repo, ["diff", base, ref, "--", SPLITS], check=False)
    units: list[str] = []
    for line in splits_out.splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        body = line[1:]
        if body[:1] not in (" ", "\t") and body.rstrip().endswith(":"):
            name = body.strip()[:-1].strip()
            if name:
                units.append(name)
    return objects, units


def touched_paths(repo: str, base: str, ref: str) -> list[str]:
    out = git(repo, ["diff", "--name-only", base, ref], check=False)
    return [p for p in out.splitlines() if p.strip()]


def units_from_touched_paths(paths: list[str]) -> list[str]:
    """The unit stems named by the `src/**` paths a ref touched (the fallback derivation)."""
    out: list[str] = []
    for path in paths:
        path = path.replace("\\", "/")
        if not path.startswith("src/"):
            continue
        stem = vu.unit_stem(path)
        if stem:
            out.append(stem)
    return out


def dedupe(items: list[str]) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for item in items:
        if item and item not in seen:
            seen.add(item)
            out.append(item)
    return out


def derive_units(repo: str, main_ref: str, ref: str, base: str | None) -> tuple[list[str], str]:
    """The ref's units and how they were derived: `registered`, `touched-path` or `none`."""
    if not base:
        return [], DERIVATION_NONE
    objects, headers = added_registrations(repo, base, ref)
    units = dedupe([vu.unit_stem(name) for name in objects] +
                   [vu.unit_stem(name) for name in headers])
    if units:
        return units, DERIVATION_REGISTERED
    touched = units_from_touched_paths(touched_paths(repo, base, ref))
    if touched:
        return dedupe(touched), DERIVATION_TOUCHED
    return [], DERIVATION_NONE


def main_registration(repo: str, main_ref: str) -> tuple[set[str], set[str]]:
    """`main`'s registered unit stems: (configure.py Object names, splits.txt unit keys)."""
    conf = git(repo, ["show", "%s:%s" % (main_ref, CONFIGURE)], check=False)
    splits = git(repo, ["show", "%s:%s" % (main_ref, SPLITS)], check=False)
    return ({vu.unit_stem(n) for n in vu.configure_object_names(conf)},
            vu.splits_unit_names(splits))


def drift_paths(repo: str, main_ref: str, ref: str, paths: list[str]) -> list[str]:
    """The touched paths whose content differs between `ref` and `main` (empty = identical)."""
    if not paths:
        return []
    out = git(repo, ["diff", "--name-only", main_ref, ref, "--", *paths], check=False)
    return [p for p in out.splitlines() if p.strip()]


def classify_ref(repo: str, main_ref: str, row: dict,
                 main_conf: set[str], main_splits: set[str], full_diff: bool = False) -> dict:
    """Audit one rescue ref -> the report row (`verdict`, `units`, `drift_paths`, ...)."""
    ref = row["ref"]
    out = dict(row)
    out["merge_base"] = None
    out["units"] = []
    out["units_on_main"] = {}
    out["derivation"] = DERIVATION_NONE
    out["touched_paths"] = []
    out["drift_paths"] = []
    out["diff_stat"] = ""
    out["diff"] = ""

    base = merge_base(repo, main_ref, ref)
    if not base:
        out["verdict"] = VERDICT_UNKNOWN
        out["reason"] = "no merge-base with %s (unrelated histories)" % main_ref
        return out
    out["merge_base"] = base

    units, derivation = derive_units(repo, main_ref, ref, base)
    out["derivation"] = derivation
    out["units"] = units
    if not units:
        out["verdict"] = VERDICT_UNKNOWN
        out["reason"] = ("no parseable unit: the registration diff is empty and no `src/**` path was "
                         "touched")
        return out

    out["units_on_main"] = {u: (u in main_conf and u in main_splits) for u in units}
    touched = touched_paths(repo, base, ref)
    out["touched_paths"] = touched
    out["drift_paths"] = drift_paths(repo, main_ref, ref, touched)
    if touched:
        stat = git(repo, ["diff", "--stat", main_ref, ref, "--", *touched], check=False)
        out["diff_stat"] = stat.strip()
        if full_diff and out["drift_paths"]:
            out["diff"] = git(repo, ["diff", main_ref, ref, "--", *touched], check=False)

    missing = [u for u, ok in out["units_on_main"].items() if not ok]
    if missing:
        out["verdict"] = VERDICT_UNLANDED
        out["reason"] = ("unit(s) not registered on %s: %s - this ref may hold the only copy"
                         % (main_ref, ", ".join(missing)))
    elif out["drift_paths"]:
        out["verdict"] = VERDICT_DRIFT
        out["reason"] = ("registered on %s, but %d touched path(s) differ (main has moved on)"
                         % (main_ref, len(out["drift_paths"])))
    else:
        out["verdict"] = VERDICT_REDUNDANT
        out["reason"] = "every unit is registered on %s and every touched path matches" % main_ref
    return out


def audit(repo: str, main_ref: str = "main", prefix: str = DEFAULT_PREFIX,
          refs: list[str] | None = None, prune: bool = False, full_diff: bool = False) -> dict:
    """Audit every rescue ref.  `prune` deletes only the `redundant` ones (printing each)."""
    rows = list_rescue_refs(repo, prefix)
    if refs:
        wanted = set(refs)
        rows = [r for r in rows if r["ref"] in wanted]
    main_conf, main_splits = main_registration(repo, main_ref)
    report = {"repo": repo, "main": main_ref, "prefix": prefix, "prune": prune,
              "summary": {v: 0 for v in VERDICTS}, "refs": [], "deleted": []}
    for row in rows:
        classified = classify_ref(repo, main_ref, row, main_conf, main_splits, full_diff=full_diff)
        report["summary"][classified["verdict"]] += 1
        report["refs"].append(classified)
        if prune and classified["verdict"] == VERDICT_REDUNDANT:
            delete_ref(repo, classified["ref"])
            report["deleted"].append(classified["ref"])
    report["count"] = len(report["refs"])
    return report


# --------------------------------------------------------------------------------------------------
# text rendering
# --------------------------------------------------------------------------------------------------

def render(report: dict) -> str:
    lines: list[str] = []
    for row in report["refs"]:
        units = ", ".join(row["units"]) or "(none)"
        on_main = ", ".join("%s=%s" % (u, "yes" if ok else "NO")
                            for u, ok in row["units_on_main"].items()) or "(n/a)"
        lines.append("%s  %s  [%s]" % (row["ref"], row["date"] or "?", row["verdict"]))
        lines.append("    units: %s  (derivation: %s)" % (units, row["derivation"]))
        if row["units_on_main"]:
            lines.append("    on main: %s" % on_main)
        lines.append("    reason: %s" % row["reason"])
        if row["drift_paths"]:
            lines.append("    drifted paths (%d): %s"
                         % (len(row["drift_paths"]), ", ".join(row["drift_paths"][:8])
                            + (" ..." if len(row["drift_paths"]) > 8 else "")))
    lines.append("")
    s = report["summary"]
    lines.append("rescue audit: %d ref(s) - redundant %d, landed-with-drift %d, unlanded %d, unknown %d"
                 % (report["count"], s[VERDICT_REDUNDANT], s[VERDICT_DRIFT], s[VERDICT_UNLANDED],
                    s[VERDICT_UNKNOWN]))
    if report["prune"]:
        lines.append("pruned %d redundant ref(s): %s"
                     % (len(report["deleted"]), ", ".join(report["deleted"]) or "(none)"))
    for row in report["refs"]:
        if row["verdict"] == VERDICT_UNLANDED:
            lines.append("  UNLANDED  %s  %s  unit(s): %s"
                         % (row["ref"], row["date"] or "?", ", ".join(row["units"])))
    return "\n".join(lines)


# --------------------------------------------------------------------------------------------------
# selftest: synthetic rescue refs in a temp repo, all four verdicts and the prune rules
# --------------------------------------------------------------------------------------------------

def _selftest_commit(repo: str, message: str) -> str:
    subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                    "add", "-A"], cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace")
    p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                        "commit", "-q", "-m", message], cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("selftest commit failed: %s" % (p.stderr or p.stdout))
    return subprocess.run(["git", "rev-parse", "HEAD"], cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace").stdout.strip()


def _write(repo: str, rel: str, text: str) -> None:
    path = os.path.join(repo, *rel.split("/"))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)


def _registration(units: list[str]) -> str:
    rows = "".join('    Object(NonMatching, "%s.cpp"),\n' % u for u in units)
    return "config.libs = [\n%s]\n" % rows


def _splits(units: list[str]) -> str:
    out = ["Sections:\n\t.text       type:code align:32\n"]
    addr = 0x80000100
    for u in units:
        out.append("%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n" % (u, addr, addr + 0x100))
        addr += 0x100
    return "\n".join(out) + "\n"


def _apply_unit(repo: str, unit: str, src: str | None = None) -> None:
    """Add `unit` to configure.py + splits.txt (+ a source file) in the working tree."""
    conf_path = os.path.join(repo, CONFIGURE)
    spl_path = os.path.join(repo, SPLITS)
    conf = open(conf_path, encoding="utf-8").read().replace("\n]", '\n    Object(NonMatching, "%s.cpp"),\n]' % unit)
    with open(conf_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(conf)
    unit_line = "%s.cpp:\n\t.text       start:0x%08X end:0x%08X\n" % (unit, 0x80001000, 0x80001100)
    with open(spl_path, "a", encoding="utf-8", newline="\n") as fh:
        fh.write("\n" + unit_line)
    _write(repo, "src/%s.cpp" % unit, src if src is not None else "int %s(void) { return 0; }\n" % unit.replace("/", "_"))


def selftest() -> int:
    import tempfile

    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    with tempfile.TemporaryDirectory() as repo:
        def g(*args):
            return subprocess.run(["git", *args], cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace").stdout

        subprocess.run(["git", "init", "-q", "-b", "main"], cwd=repo, capture_output=True)
        _write(repo, CONFIGURE, _registration(["mainunit"]))
        _write(repo, SPLITS, _splits(["mainunit"]))
        _write(repo, "src/mainunit.cpp", "int mainunit(void) { return 0; }\n")
        base = _selftest_commit(repo, "base")

        # --- redundant: the ref edits an already-registered unit; main has the identical content.
        # A *registered* redundant ref cannot coexist with a sibling that moved main's configure.py -
        # the registration files then differ - so this one is source-only and the dedicated repo below
        # covers the registered derivation.
        subprocess.run(["git", "checkout", "-q", "-b", "b-red"], cwd=repo, capture_output=True)
        _write(repo, "src/mainunit.cpp", "int mainunit(void) { return 7; }\n")
        red_tip = _selftest_commit(repo, "red")
        # main takes the same content independently, so the merge-base stays `base`
        subprocess.run(["git", "checkout", "-q", "main"], cwd=repo, capture_output=True)
        _write(repo, "src/mainunit.cpp", "int mainunit(void) { return 7; }\n")
        _selftest_commit(repo, "land red")
        g("update-ref", "refs/rescue/red", red_tip)

        # --- landed-with-drift: main registers the unit but with different source content ---
        subprocess.run(["git", "checkout", "-q", "-b", "b-drift", base], cwd=repo, capture_output=True)
        _apply_unit(repo, "drift", "int drift(void) { return 1; }\n")
        drift_tip = _selftest_commit(repo, "drift v1")
        subprocess.run(["git", "checkout", "-q", "main"], cwd=repo, capture_output=True)
        _apply_unit(repo, "drift", "int drift(void) { return 2; }\n")
        _selftest_commit(repo, "land drift v2")
        g("update-ref", "refs/rescue/drift", drift_tip)

        # --- unlanded: the unit never reaches main ---
        subprocess.run(["git", "checkout", "-q", "-b", "b-unl", base], cwd=repo, capture_output=True)
        _apply_unit(repo, "unl")
        unl_tip = _selftest_commit(repo, "unl")
        g("update-ref", "refs/rescue/unl", unl_tip)

        # --- unknown: a ref of unrelated history (no merge-base) ---
        subprocess.run(["git", "checkout", "-q", "--orphan", "orphan"], cwd=repo, capture_output=True)
        _write(repo, "tools/scratch.txt", "nothing here\n")
        orphan_tip = _selftest_commit(repo, "orphan root")
        g("update-ref", "refs/rescue/orphan", orphan_tip)
        subprocess.run(["git", "checkout", "-q", "main"], cwd=repo, capture_output=True)

        # --- unknown: a ref that touches nothing parseable (a tools edit, no src/ and no registration) ---
        subprocess.run(["git", "checkout", "-q", "-b", "b-tools", base], cwd=repo, capture_output=True)
        _write(repo, "tools/scratch.txt", "tool change\n")
        tools_tip = _selftest_commit(repo, "tools only")
        g("update-ref", "refs/rescue/tools-only", tools_tip)

        # --- read-only audit: no refs deleted ---
        before = set(g("for-each-ref", "--format=%(refname)", "refs/rescue/").split())
        report = audit(repo, "main", refs=None, prune=False)
        after = set(g("for-each-ref", "--format=%(refname)", "refs/rescue/").split())
        check("read-only audit leaves every ref in place", after, before)

        by_ref = {r["ref"]: r for r in report["refs"]}
        check("redundant: verdict", by_ref["refs/rescue/red"]["verdict"], VERDICT_REDUNDANT)
        check("redundant: units come from the touched path (no registration change)",
              by_ref["refs/rescue/red"]["units"], ["mainunit"])
        check("redundant: derivation is touched-path",
              by_ref["refs/rescue/red"]["derivation"], DERIVATION_TOUCHED)
        check("redundant: unit is on main",
              by_ref["refs/rescue/red"]["units_on_main"], {"mainunit": True})
        check("redundant: no drifted path", by_ref["refs/rescue/red"]["drift_paths"], [])
        check("redundant: the touched source path is reported",
              by_ref["refs/rescue/red"]["touched_paths"], ["src/mainunit.cpp"])

        check("drift: verdict", by_ref["refs/rescue/drift"]["verdict"], VERDICT_DRIFT)
        check("drift: units come from the registration diff",
              by_ref["refs/rescue/drift"]["units"], ["drift"])
        check("drift: derivation is registered",
              by_ref["refs/rescue/drift"]["derivation"], DERIVATION_REGISTERED)
        check("drift: unit is on main", by_ref["refs/rescue/drift"]["units_on_main"], {"drift": True})
        check("drift: the differing source path is named",
              by_ref["refs/rescue/drift"]["drift_paths"], ["src/drift.cpp"])

        check("unlanded: verdict", by_ref["refs/rescue/unl"]["verdict"], VERDICT_UNLANDED)
        check("unlanded: unit is not on main",
              by_ref["refs/rescue/unl"]["units_on_main"], {"unl": False})
        check("unknown: no merge-base is unknown",
              by_ref["refs/rescue/orphan"]["verdict"], VERDICT_UNKNOWN)
        check("unknown: nothing parseable is unknown",
              by_ref["refs/rescue/tools-only"]["verdict"], VERDICT_UNKNOWN)
        check("summary counts all four verdicts",
              {k: report["summary"][k] for k in VERDICTS},
              {VERDICT_REDUNDANT: 1, VERDICT_DRIFT: 1, VERDICT_UNLANDED: 1, VERDICT_UNKNOWN: 2})

        # --- prune: deletes only the redundant ref ---
        pruned = audit(repo, "main", prune=True)
        check("prune: exactly the redundant ref was deleted",
              pruned["deleted"], ["refs/rescue/red"])
        left = set(g("for-each-ref", "--format=%(refname)", "refs/rescue/").split())
        check("prune: the redundant ref is gone", "refs/rescue/red" in left, False)
        check("prune: landed-with-drift survives", "refs/rescue/drift" in left, True)
        check("prune: unlanded survives", "refs/rescue/unl" in left, True)
        check("prune: unknown survives", "refs/rescue/orphan" in left, True)
        check("prune: the second unknown survives", "refs/rescue/tools-only" in left, True)

        # --- --ref restricts the audit ---
        one = audit(repo, "main", refs=["refs/rescue/unl"])
        check("--ref audits one ref", [r["ref"] for r in one["refs"]], ["refs/rescue/unl"])

    # --- a *registered* redundant ref, pruned: the branch's registration is identical on main --------
    with tempfile.TemporaryDirectory() as repo:
        subprocess.run(["git", "init", "-q", "-b", "main"], cwd=repo, capture_output=True)
        _write(repo, CONFIGURE, _registration(["mainunit"]))
        _write(repo, SPLITS, _splits(["mainunit"]))
        _write(repo, "src/mainunit.cpp", "int mainunit(void) { return 0; }\n")
        _selftest_commit(repo, "base")
        subprocess.run(["git", "checkout", "-q", "-b", "b-reg"], cwd=repo, capture_output=True)
        _apply_unit(repo, "reg")
        reg_tip = _selftest_commit(repo, "reg")
        subprocess.run(["git", "checkout", "-q", "main"], cwd=repo, capture_output=True)
        _apply_unit(repo, "reg")
        _selftest_commit(repo, "land reg")
        subprocess.run(["git", "update-ref", "refs/rescue/reg", reg_tip], cwd=repo, capture_output=True)
        report = audit(repo, "main")
        row = {r["ref"]: r for r in report["refs"]}["refs/rescue/reg"]
        check("registered redundant: verdict", row["verdict"], VERDICT_REDUNDANT)
        check("registered redundant: unit from the registration diff", row["units"], ["reg"])
        check("registered redundant: no drifted path", row["drift_paths"], [])
        check("registered redundant: prune deletes it", audit(repo, "main", prune=True)["deleted"],
              ["refs/rescue/reg"])

    if fails:
        print("rescue: %d check(s), %d failure(s)" % (checks, len(fails)))
        for f in fails:
            print("  FAIL " + f)
        return 1
    print("rescue: %d check(s), 0 failure(s)" % checks)
    return 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("cmd", nargs="?", choices=["audit"], default="audit",
                    help="audit the rescue refs (the only command)")
    ap.add_argument("--prune", action="store_true",
                    help="delete the redundant refs (and only those), printing each deletion")
    ap.add_argument("--json", action="store_true", help="emit the report as JSON")
    ap.add_argument("--full-diff", action="store_true",
                    help="include the complete touched-path patch in each report row (large)")
    ap.add_argument("--ref", action="append", default=[], help="audit only this ref; repeatable")
    ap.add_argument("--repo", default=None, help="the repository to inspect (default: this checkout)")
    ap.add_argument("--main", default="main", help="the branch that represents landed state (default: main)")
    ap.add_argument("--prefix", default=DEFAULT_PREFIX, help="the rescue ref prefix")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    repo = args.repo or default_repo()
    report = audit(repo, args.main, args.prefix, refs=args.ref or None, prune=args.prune,
                   full_diff=args.full_diff)
    print(json.dumps(report, indent=2) if args.json else render(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
