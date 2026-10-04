"""Rescue refs: park a branch's tip at `refs/rescue/<slug>`, classify a ref against `main` (redundant /
landed-with-drift / unlanded / unknown), and prune only what is provably redundant.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library; `tools/units/rescue.py` is the audit CLI)."""
from __future__ import annotations

import os

from tools.lib import project as _project
from tools.lib import units as _units
from tools.lib.git import Git

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

unit_stem = _units.stem


def git(repo: str, args: list[str], check: bool = True) -> str:
    p = Git(repo).run(*args)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), repo, p.stderr.strip()))
    return p.stdout


def make(repo: str, ref: str, branch: str) -> None:
    """Park `branch`'s tip at `ref` (the step before any branch delete)."""
    git(repo, ["update-ref", ref, branch])


def delete(repo: str, ref: str) -> None:
    git(repo, ["update-ref", "-d", ref])


def list_refs(repo: str, prefix: str = DEFAULT_PREFIX) -> list[dict]:
    """Every ref under `prefix` with its tip's short date and subject, sorted by name."""
    out = git(repo, ["for-each-ref", "--sort=refname",
                     "--format=%(refname)%1f%(committerdate:short)%1f%(objectname)%1f%(subject)", prefix],
              check=False)
    rows: list[dict] = []
    for line in out.splitlines():
        if not line.strip():
            continue
        parts = line.split("\x1f")
        if len(parts) < 4:
            continue
        rows.append({"ref": parts[0], "date": parts[1], "tip": parts[2], "subject": parts[3]})
    return rows


def merge_base(repo: str, main_ref: str, ref: str) -> str | None:
    return Git(repo).merge_base(main_ref, ref)


def object_names(text: str) -> list[str]:
    """Every unit a `configure.py` text registers through a closed one-line `Object(kind, "unit")`."""
    return [c.path for c in _project.object_calls(text) if c.closed]


def added_registrations(repo: str, base: str, ref: str) -> tuple[list[str], list[str]]:
    """The units a ref added vs `base`: (`configure.py` Object names, `splits.txt` unit headers)."""
    objects: list[str] = []
    for line in git(repo, ["diff", base, ref, "--", CONFIGURE], check=False).splitlines():
        if line.startswith("+") and not line.startswith("+++"):
            objects.extend(object_names(line[1:]))
    units: list[str] = []
    for line in git(repo, ["diff", base, ref, "--", SPLITS], check=False).splitlines():
        if not line.startswith("+") or line.startswith("+++"):
            continue
        body = line[1:]
        if body[:1] not in (" ", "\t") and body.rstrip().endswith(":"):
            name = body.strip()[:-1].strip()
            if name:
                units.append(name)
    return objects, units


def touched_paths(repo: str, base: str, ref: str) -> list[str]:
    return [p for p in git(repo, ["diff", "--name-only", base, ref], check=False).splitlines() if p.strip()]


def units_from_touched_paths(paths: list[str]) -> list[str]:
    """The unit stems named by the `src/**` paths a ref touched (the fallback derivation)."""
    out: list[str] = []
    for path in paths:
        path = path.replace("\\", "/")
        if path.startswith("src/"):
            stem = unit_stem(path)
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
    units = dedupe([unit_stem(n) for n in objects] + [unit_stem(n) for n in headers])
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
    return ({unit_stem(c.path) for c in _project.object_calls(conf or "")},
            {unit_stem(u) for u in _project.Splits.parse(splits or "").units if u.strip()})


def drift_paths(repo: str, main_ref: str, ref: str, paths: list[str]) -> list[str]:
    if not paths:
        return []
    out = git(repo, ["diff", "--name-only", main_ref, ref, "--", *paths], check=False)
    return [p for p in out.splitlines() if p.strip()]


def classify(repo: str, main_ref: str, row: dict, main_conf: set[str], main_splits: set[str],
             full_diff: bool = False) -> dict:
    """Audit one rescue ref -> the report row (`verdict`, `reason`, `units`, `units_on_main`, `drift_paths`...)."""
    ref = row["ref"]
    out = dict(row)
    out.update({"merge_base": None, "units": [], "units_on_main": {}, "derivation": DERIVATION_NONE,
                "touched_paths": [], "drift_paths": [], "diff_stat": "", "diff": ""})
    base = merge_base(repo, main_ref, ref)
    if not base:
        out["verdict"] = VERDICT_UNKNOWN
        out["reason"] = "no merge-base with %s (unrelated histories)" % main_ref
        return out
    out["merge_base"] = base
    units, derivation = derive_units(repo, main_ref, ref, base)
    out["derivation"], out["units"] = derivation, units
    if not units:
        out["verdict"] = VERDICT_UNKNOWN
        out["reason"] = "no parseable unit: the registration diff is empty and no `src/**` path was touched"
        return out
    out["units_on_main"] = {u: (u in main_conf and u in main_splits) for u in units}
    touched = touched_paths(repo, base, ref)
    out["touched_paths"] = touched
    out["drift_paths"] = drift_paths(repo, main_ref, ref, touched)
    if touched:
        out["diff_stat"] = git(repo, ["diff", "--stat", main_ref, ref, "--", *touched], check=False).strip()
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


def audit(repo: str, main_ref: str = "main", prefix: str = DEFAULT_PREFIX, refs: list[str] | None = None,
          prune: bool = False, full_diff: bool = False) -> dict:
    """Audit every rescue ref under `prefix` (or only `refs`); `prune` deletes the `redundant` ones only."""
    rows = list_refs(repo, prefix)
    if refs:
        wanted = set(refs)
        rows = [r for r in rows if r["ref"] in wanted]
    main_conf, main_splits = main_registration(repo, main_ref)
    report = {"repo": repo, "main": main_ref, "prefix": prefix, "prune": prune,
              "summary": {v: 0 for v in VERDICTS}, "refs": [], "deleted": []}
    for row in rows:
        classified = classify(repo, main_ref, row, main_conf, main_splits, full_diff=full_diff)
        report["summary"][classified["verdict"]] += 1
        report["refs"].append(classified)
        if prune and classified["verdict"] == VERDICT_REDUNDANT:
            delete(repo, classified["ref"])
            report["deleted"].append(classified["ref"])
    report["count"] = len(report["refs"])
    return report


def verdict(main: str, ref: str, prune: bool = True) -> dict:
    """Classify one ref at the moment a teardown creates it -> `{ref, verdict, units, date, reason, drift,
    pruned, line}`. Prunes only `redundant`; never raises (an audit that cannot run is `unknown`, ref kept)."""
    rows: list[dict] = []
    deleted: list[str] = []
    error: str | None = None
    try:
        report = audit(main, main_ref="main", refs=[ref], prune=prune)
        rows, deleted = report["refs"], report.get("deleted") or []
    except (SystemExit, Exception) as exc:  # noqa: BLE001 - a verdict must never block a teardown
        error = (str(exc).strip().splitlines() or [exc.__class__.__name__])[0]
    if not rows:
        why = error or "the ref was not there to audit"
        return {"ref": ref, "verdict": VERDICT_UNKNOWN, "units": [], "date": "?", "reason": why, "drift": 0,
                "pruned": False, "line": "UNKNOWN %s: %s - the teardown kept it" % (ref, why)}
    row = rows[0]
    out = {"ref": ref, "verdict": row["verdict"], "units": row["units"], "date": row["date"] or "?",
           "reason": row["reason"], "drift": len(row["drift_paths"]), "pruned": ref in deleted}
    units = ", ".join(out["units"]) or "(none)"
    if out["verdict"] == VERDICT_REDUNDANT:
        out["line"] = "pruned %s - redundant: %s" % (ref, out["reason"])
    elif out["verdict"] == VERDICT_DRIFT:
        out["line"] = ("kept %s - landed-with-drift: %s; drift can hide an unlanded hunk, so it stays"
                       % (ref, out["reason"]))
    elif out["verdict"] == VERDICT_UNLANDED:
        out["line"] = ("UNLANDED WORK at %s (%s): unit(s) %s hold no landing on main - this ref may be the "
                       "only copy and the teardown kept it (inspect: python tools/units/rescue.py audit "
                       "--ref %s)" % (ref, out["date"], units, ref))
    else:
        out["line"] = ("UNKNOWN %s (%s): nothing parseable to prove containment (%s) - the teardown kept it"
                       % (ref, out["date"], out["reason"]))
    return out


def default_repo(cwd: str | None = None) -> str:
    """The checkout the audit inspects: `git rev-parse --show-toplevel` of `cwd`, else `cwd`."""
    cwd = cwd or os.getcwd()
    p = Git(cwd).run("rev-parse", "--show-toplevel")
    return p.stdout.strip() or cwd
