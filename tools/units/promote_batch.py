#!/usr/bin/env python3
"""Plan, apply and check a *batch* of promotions with **one** re-split and **one** gate.

`tools/units/promote.py` does one unit and, for one unit, the re-split is free. A batch cannot pay it
per unit (`docs/plan.md` 5.6: a rename and a move both cost the split, so promotions ride a batch), so
this tool is the batch: it plans every entry against the *current* tree, applies them one after another
(each re-planned, because the first one moved `splits.txt`/`configure.py` under the next), and prints
the single command block that finishes the job.

    python tools/units/promote_batch.py plan  <spec> [--json]
    python tools/units/promote_batch.py apply <spec> [--dry-run] [--allow-claimed] [--manifest <path>]
    python tools/units/promote_batch.py check <spec> [--manifest <path>] [--json]
    python tools/units/promote_batch.py --selftest

**The spec** is a text file, one entry per line; `#` starts a comment, blank lines are skipped, and the
options are `promote.py`'s plus `--lang`:

    # docs/plan.md, "The language comes from the symbol" - one re-split for the whole file
    auto/800CCFB0_fn_800CCFB0   --lang c++        # in place: the stem stays, the extension moves
    auto/802B2978_fn_802B2978   --name colour_blend --module Pl

* `--lang c++|c` changes the front-end. With no `--name`/`--module` it is the **language promotion**:
  the unit keeps its stem, its module and its lib and only the extension moves - so it is legal even
  inside the `auto` bucket, which `promote.py` refuses to move a unit *to*. `promote.py`'s
  `--allow-ext-change` is what a single-unit caller passes; here it is derived.
* `--name`/`--module`/`--lib`/`--symbol` are exactly `promote.py`'s options and describe a real
  promotion (rename and/or move).

**Two kinds, two byte expectations.** A *promotion* (a move/rename) must be **byte-identical**: a name
and a path change no instructions, so any difference is a bug in the move. A *language* change is the
opposite: the front-end changes, so the bytes are **expected to move**, and `check` reports what moved
and whether the score improved instead of failing on the difference. `check` still refuses a unit that
is not in the build graph, which is the bug that let a promoted unit vanish from `report.json`.

**One re-split, one gate.** `apply` never builds and never re-splits; it ends by printing the single
block the orchestrator runs:

    ninja build/RMHE08/report.json      # re-splits, re-runs configure.py, compiles, reports
    ninja build/RMHE08/ok               # the gate
    python tools/units/promote_batch.py check <spec>

`apply` snapshots each pre-change object into `.pi/promote/` and writes a manifest next to it, so
`check` can compare the object built at the new path against the baseline after the re-split - and so
the batch survives a `build/` wipe. `check` reads the unit's score before (from the manifest) and after
(from `build/RMHE08/report.json`) and prints the delta, which is the evidence a language verdict is
right or wrong.
"""

from __future__ import annotations

import argparse
import contextlib
import io
import json
import os
import shlex
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "units"))
sys.path.insert(0, str(ROOT / "tools" / "symbols"))

import promote as pr  # noqa: E402  the single-unit plan/apply/check this batches

KIND_LANGUAGE = "language"
KIND_PROMOTION = "promotion"
LANG_EXT = {"c": ".c", "c++": ".cpp"}
SPEC_MANIFEST_DIR = ".pi/promote"


# --------------------------------------------------------------------------------------------------
# the spec
# --------------------------------------------------------------------------------------------------
@dataclass
class Entry:
    unit: str
    line: int = 0
    lang: str | None = None
    name: str | None = None
    module: str | None = None
    lib: str | None = None
    symbols: list[str] = field(default_factory=list)
    note: str = ""

    def describe(self) -> str:
        bits = []
        if self.lang:
            bits.append("--lang %s" % self.lang)
        if self.name:
            bits.append("--name %s" % self.name)
        if self.module:
            bits.append("--module %s" % self.module)
        if self.lib:
            bits.append("--lib %s" % self.lib)
        for s in self.symbols:
            bits.append("--symbol %s" % s)
        return " ".join(bits)


def parse_entry(line: str, lineno: int) -> Entry:
    """One spec line: the unit, then `promote.py`'s options plus `--lang`."""
    try:
        toks = shlex.split(line, comments=True)
    except ValueError as exc:
        raise SystemExit("refusing: %s:%d is not parseable (%s)" % ("spec", lineno, exc))
    if not toks:
        raise SystemExit("refusing: spec line %d is empty" % lineno)
    entry = Entry(unit=toks[0], line=lineno)
    i = 1
    while i < len(toks):
        tok = toks[i]
        if tok in ("--name", "--module", "--lib", "--lang", "--symbol", "--note"):
            if i + 1 >= len(toks):
                raise SystemExit("refusing: spec line %d: %s needs a value" % (lineno, tok))
            value = toks[i + 1]
            if tok == "--lang":
                if value not in LANG_EXT:
                    raise SystemExit("refusing: spec line %d: --lang %r is not c or c++"
                                     % (lineno, value))
                entry.lang = value
            elif tok == "--name":
                entry.name = value
            elif tok == "--module":
                entry.module = value
            elif tok == "--lib":
                entry.lib = value
            elif tok == "--symbol":
                entry.symbols.append(value)
            else:
                entry.note = value
            i += 2
            continue
        raise SystemExit("refusing: spec line %d: unknown option %r" % (lineno, tok))
    return entry


def load_spec(path: str) -> dict:
    text = pr.read_text(Path(path))
    entries = []
    for i, line in enumerate(text.split("\n"), 1):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        entries.append(parse_entry(line, i))
    if not entries:
        raise SystemExit("refusing: %s holds no promotions" % path)
    seen = {}
    for e in entries:
        key = pr.norm_unit(e.unit)
        if key in seen:
            raise SystemExit("refusing: %s names %s twice (lines %d and %d)"
                             % (path, e.unit, seen[key], e.line))
        seen[key] = e.line
    return {"name": Path(path).stem, "path": path, "entries": entries}


# --------------------------------------------------------------------------------------------------
# the plan
# --------------------------------------------------------------------------------------------------
def detect_conflict(ctx: pr.Ctx, registered: str) -> list[str]:
    """The `__FILE__` names the unit's **target** object references, when there are two.

    A unit whose object references two distinct source names spans two original translation units
    (`langcheck`'s `conflict`): the split range is wrong, so its extension cannot be trusted until the
    range is re-cut (`docs/plan.md`, the language rule). Best-effort - the map/DOL oracle is not always
    loadable, and a batch must not depend on it to plan.
    """
    target = ctx.object_dir.parent / "obj" / registered
    target = target.with_suffix(".o")
    if not target.is_file():
        return []
    try:
        import langcheck
        labels, dol = langcheck.oracle()
        v = langcheck.object_verdict(str(target), labels, dol)
        return list(v.get("sources") or []) if v.get("conflict") else []
    except Exception:                                 # no map, no DOL, no oracle - no claim either way
        return []


def entry_plan(ctx: pr.Ctx, entry: Entry, conflict_check: bool = True) -> dict:
    """`promote.plan` for one entry, with the in-place language promotion derived from the spec."""
    registered = pr.source_name(ctx.root, entry.unit)
    if registered is None:
        raise SystemExit("refusing: %s (spec line %d) is not a registered source"
                         % (entry.unit, entry.line))
    old_ext = Path(registered).suffix
    stem = Path(registered).stem
    src_module = Path(registered).parent.as_posix()
    ext = LANG_EXT[entry.lang] if entry.lang else old_ext
    module = entry.module if entry.module is not None else src_module
    module_norm = module.strip().strip("/").replace("\\", "/") or "."
    in_place = module_norm == src_module
    name = entry.name if entry.name is not None else (stem + ext)
    p = pr.plan(ctx, entry.unit, name, module, entry.lib, entry.symbols,
                allow_ext_change=(ext != old_ext), in_place=in_place)
    p["_entry"] = entry
    p["_registered"] = registered
    p["_kind"] = KIND_LANGUAGE if ext != old_ext else KIND_PROMOTION
    p["_in_place"] = in_place
    p["_conflict"] = detect_conflict(ctx, registered) if conflict_check else []
    return p


def plan_batch(ctx: pr.Ctx, spec: dict, conflict_check: bool = True) -> list[dict]:
    return [entry_plan(ctx, e, conflict_check) for e in spec["entries"]]


def batch_summary(plans: list[dict], ctx: pr.Ctx) -> dict:
    languages = [p for p in plans if p["_kind"] == KIND_LANGUAGE]
    promotions = [p for p in plans if p["_kind"] == KIND_PROMOTION]
    lint = sum(p["lint"]["count"] for p in plans if p["lint"]["enforced"])
    claims = [(p["unit"], p["claim"]) for p in plans if p["claim"]]
    conflicts = [(p["unit"], p["_conflict"]) for p in plans if p["_conflict"]]
    moves = [(p["unit"], p["new_unit"]) for p in plans if p["unit"] != p["new_unit"]]
    return {
        "units": len(plans),
        "languages": len(languages),
        "promotions": len(promotions),
        "moves": moves,
        "shared_files": sorted({pr.SPLITS, pr.CONFIGURE}),
        "lint_added": lint,
        "claims": claims,
        "conflicts": conflicts,
        "splits_blocks": sum(1 for p in plans if p["splits_block"]),
        "lang_changes": [(p["unit"], p["flags"]["before"]["lang"], p["flags"]["after"]["lang"])
                         for p in languages],
    }


def human_plan(plans: list[dict], spec: dict, ctx: pr.Ctx) -> None:
    s = batch_summary(plans, ctx)
    print("promote batch %r: %d unit(s) - %d language change(s), %d promotion(s)"
          % (spec["name"], s["units"], s["languages"], s["promotions"]))
    print("  spec       %s" % spec["path"])
    print("  shared     %s (one re-split for the whole batch)" % ", ".join(s["shared_files"]))
    print("  re-split   ONE (a rename and a move each cost the split; this pays it once)")
    for p in plans:
        e = p["_entry"]
        kind = "language" if p["_kind"] == KIND_LANGUAGE else "promotion"
        print("  %-9s %s -> %s  [%s lib, %s]" % (kind, p["unit"], p["new_unit"],
                                                 p["lib"], p["flag"]))
        if p["_kind"] == KIND_LANGUAGE:
            print("             front-end %s -> %s   expected: the object bytes move"
                  % (p["flags"]["before"]["lang"], p["flags"]["after"]["lang"]))
        else:
            print("             expected: byte-identical (a name/path change is no codegen)")
        if e.describe():
            print("             options  %s" % e.describe())
        if p["pairs"]:
            print("             symbols  %s" % ", ".join("%s -> %s" % (o, n) for o, n in p["pairs"]))
        if p["lint"]["count"]:
            print("             lint     rule 7 would gain %d finding(s) at %s - land.py refuses "
                  "that" % (p["lint"]["count"], p["new_unit"]))
        if p["claim"]:
            print("             claim    live - apply refuses unless --allow-claimed")
        if p["_conflict"]:
            print("             CONFLICT the target object names two source files (%s) - the range "
                  "spans two translation units, so the extension is not trustworthy until it is "
                  "re-cut; apply refuses unless --allow-conflict" % ", ".join(p["_conflict"]))
        if p["pool"]:
            print("             pool     %d stale brief(s)" % len(p["pool"]))
    if s["lint_added"]:
        print("  BLOCKER    rule 7: this batch adds %d finding(s); rename them with --symbol"
              % s["lint_added"])
    if s["claims"]:
        print("  BLOCKER    %d live claim(s): %s"
              % (len(s["claims"]), ", ".join(u for u, _ in s["claims"])))
    if s["conflicts"]:
        print("  BLOCKER    %d boundary defect(s) (one object, two source files): %s"
              % (len(s["conflicts"]), ", ".join(u for u, _ in s["conflicts"])))
    print("next, after `apply` (do NOT run configure.py before the split - it reads the *old* "
          "build/RMHE08/config.json and the new unit vanishes from the report):")
    print("  ninja build/RMHE08/report.json      # re-splits, re-runs configure.py, compiles, reports")
    print("  ninja build/RMHE08/ok               # the gate")
    print("  python tools/units/promote_batch.py check %s" % spec["path"])


# --------------------------------------------------------------------------------------------------
# apply - sequential, each re-planned against the tree the previous one left
# --------------------------------------------------------------------------------------------------
def manifest_path(ctx: pr.Ctx, spec: dict, override: str | None) -> Path:
    if override:
        return Path(override)
    return ctx.root / SPEC_MANIFEST_DIR / ("batch-%s.json" % spec["name"])


def report_score(root: Path, unit_stem: str) -> float | None:
    """The unit's `fuzzy_match_percent` in the current report, or None when there is no report."""
    path = root / "build" / "RMHE08" / "report.json"
    if not path.is_file():
        return None
    try:
        data = json.loads(pr.read_text(path))
    except ValueError:
        return None
    for u in data.get("units") or []:
        if u.get("name") == "main/" + unit_stem:
            return (u.get("measures") or {}).get("fuzzy_match_percent")
    return None


def unit_stem(registered: str) -> str:
    return registered[: -len(Path(registered).suffix)]


def apply_batch(ctx: pr.Ctx, spec: dict, dry_run: bool = False, allow_claimed: bool = False,
                manifest: str | None = None, allow_conflict: bool = False,
                conflict_check: bool = True) -> int:
    records, failures = [], []
    for i, entry in enumerate(spec["entries"], 1):
        try:
            p = entry_plan(ctx, entry, conflict_check)
        except SystemExit as exc:
            failures.append((entry.unit, str(exc)))
            print("  FAILED to plan %s: %s" % (entry.unit, exc))
            break
        if p["_conflict"] and not allow_conflict:
            failures.append((entry.unit, "boundary defect: %s" % ", ".join(p["_conflict"])))
            print("  REFUSED %s: the target object names two source files (%s) - the range spans two "
                  "translation units; re-cut it first, or pass --allow-conflict"
                  % (entry.unit, ", ".join(p["_conflict"])))
            break
        kind = p["_kind"]
        before = report_score(ctx.root, unit_stem(p["_registered"]))
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = pr.apply_plan(ctx, p, dry_run=dry_run, allow_claimed=allow_claimed)
        if rc != 0:
            failures.append((entry.unit, "apply returned %d" % rc))
            print("  FAILED to apply %s (exit %d); output:" % (entry.unit, rc))
            for line in buf.getvalue().splitlines():
                print("    %s" % line)
            break
        records.append({
            "unit": p["unit"], "registered": p["_registered"], "new_unit": p["new_unit"],
            "ext": p["ext"], "kind": kind, "module": p["module"], "name": p["name"],
            "lib": p["lib"], "flag": p["flag"], "in_place": p["_in_place"],
            "old_obj": ctx.rel(p["old_obj"]), "new_obj": ctx.rel(p["new_obj"]),
            "lang_before": p["flags"]["before"]["lang"], "lang_after": p["flags"]["after"]["lang"],
            "score_before": before,
        })
        print("  [%d/%d] %-9s %s -> %s" % (i, len(spec["entries"]), kind, p["unit"],
                                           p["new_unit"]))
    if dry_run:
        print("dry run: %d of %d planned, nothing written" % (len(records), len(spec["entries"])))
        return 1 if failures else 0
    if failures and not records:
        print("nothing applied")
        return 1
    out = manifest_path(ctx, spec, manifest)
    out.parent.mkdir(parents=True, exist_ok=True)
    payload = {"batch": spec["name"], "spec": spec["path"], "records": records,
               "failed": [{"unit": u, "why": w} for u, w in failures]}
    tx = pr.sf.Transaction()
    try:
        tx.write(out, json.dumps(payload, indent=2) + "\n")
    except BaseException:
        tx.rollback()
        raise
    finally:
        tx.cleanup()
    print("applied %d of %d unit(s); manifest %s" % (len(records), len(spec["entries"]),
                                                     ctx.rel(out)))
    if failures:
        print("STOPPED at %s: %s - the batch is half-applied; `git status --short` shows it"
              % (failures[0][0], failures[0][1]))
    print("next (ONE re-split, ONE gate):")
    print("  ninja build/RMHE08/report.json")
    print("  ninja build/RMHE08/ok")
    print("  python tools/units/promote_batch.py check %s --manifest %s" % (spec["path"], ctx.rel(out)))
    return 1 if failures else 0


# --------------------------------------------------------------------------------------------------
# check - the build graph plus the byte verdict, per unit, with the score delta
# --------------------------------------------------------------------------------------------------
def check_record(ctx: pr.Ctx, rec: dict) -> dict:
    graph = pr.build_graph(ctx, rec["new_unit"], rec["ext"])
    before = ctx.scratch / (rec["unit"].replace("/", "_") + ".before.o")
    if not before.is_file():
        before = ctx.root / rec["old_obj"]
    after = ctx.root / rec["new_obj"]
    out = {"unit": rec["unit"], "new_unit": rec["new_unit"], "kind": rec["kind"],
           "build_graph": graph, "before": ctx.rel(before), "after": ctx.rel(after),
           "score_before": rec.get("score_before"),
           "score_after": report_score(ctx.root, unit_stem(rec["new_unit"])),
           "verdict": "?", "diffs": [], "sections": []}
    if not graph["in_graph"]:
        out["verdict"] = "not-in-build"
        return out
    if not before.is_file() or not after.is_file():
        out["verdict"] = "no-object"
        out["missing_object"] = ctx.rel(before if not before.is_file() else after)
        return out
    res = pr.compare_objects(before, after)
    out["verdict"] = res["verdict"]
    out["diffs"] = res["diffs"]
    out["sections"] = [row for row in res["sections"]
                       if row[3] in ("equal", "DIFFERS", "absent")]
    return out


def check_batch(ctx: pr.Ctx, manifest: dict, json_out: bool = False) -> int:
    results = [check_record(ctx, rec) for rec in manifest.get("records") or []]
    if json_out:
        print(json.dumps({"batch": manifest.get("batch"), "results": results}, indent=2))
    else:
        print("check batch %r: %d unit(s)" % (manifest.get("batch"), len(results)))
        print("  %-42s %-10s %-12s %s" % ("unit -> new", "kind", "verdict", "score before -> after"))
        for r in results:
            sb, sa = r.get("score_before"), r.get("score_after")
            delta = ("%s -> %s" % (round(sb, 3) if isinstance(sb, (int, float)) else "-",
                                   round(sa, 3) if isinstance(sa, (int, float)) else "-"))
            if isinstance(sb, (int, float)) and isinstance(sa, (int, float)):
                delta += "  (%+.3f)" % (sa - sb)
            print("  %-42s %-10s %-12s %s" % (r["new_unit"], r["kind"], r["verdict"], delta))
            if r["verdict"] == "not-in-build":
                print("      missing: %s" % ", ".join(r["build_graph"]["missing"]))
                print("      run the re-split: ninja build/RMHE08/report.json")
            elif r["verdict"] == "no-object":
                print("      not built: %s" % r.get("missing_object"))
            elif r["verdict"] == "differs":
                for line in r["diffs"][:6]:
                    print("      DIFF %s" % line)
        bad = [r for r in results if r["verdict"] in ("not-in-build", "no-object")
               or (r["kind"] == KIND_PROMOTION and r["verdict"] == "differs")]
        lang = [r for r in results if r["kind"] == KIND_LANGUAGE]
        print("")
        print("  %d promotion(s) must be byte-identical; %d language change(s) are expected to move"
              % (sum(1 for r in results if r["kind"] == KIND_PROMOTION), len(lang)))
        for r in lang:
            if r["verdict"] in ("identical", "names-only"):
                print("  NOTE %s: the machine code did not move (verdict %s) - the front-end change is "
                      "a no-op here, so the verdict is neither proved nor needed"
                      % (r["new_unit"], r["verdict"]))
        print("  verdict: %s" % ("OK" if not bad else "FAIL (%d unit(s))" % len(bad)))
    bad = [r for r in results if r["verdict"] in ("not-in-build", "no-object")
           or (r["kind"] == KIND_PROMOTION and r["verdict"] == "differs")]
    return 0 if not bad else 1


def read_manifest(ctx: pr.Ctx, spec: dict, override: str | None) -> dict:
    path = manifest_path(ctx, spec, override)
    if not path.is_file():
        raise SystemExit("no manifest at %s - `apply` writes it; pass --manifest" % ctx.rel(path))
    return json.loads(pr.read_text(path))


# --------------------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    p = sub.add_parser("plan", help="print the whole batch, read-only")
    p.add_argument("spec")
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("apply", help="apply every entry, one re-split/gate at the end")
    p.add_argument("spec")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--allow-claimed", action="store_true")
    p.add_argument("--allow-conflict", action="store_true",
                   help="apply a unit whose target object names two source files (a boundary defect)")
    p.add_argument("--manifest", default=None)
    p = sub.add_parser("check", help="build-graph + byte verdict per unit, with the score delta")
    p.add_argument("spec")
    p.add_argument("--manifest", default=None)
    p.add_argument("--json", action="store_true")
    return ap


def plan_json(plans: list[dict], spec: dict, ctx: pr.Ctx) -> dict:
    return {
        "batch": spec["name"],
        "units": [{
            "unit": p["unit"], "new_unit": p["new_unit"], "kind": p["_kind"], "in_place": p["_in_place"],
            "lib": p["lib"], "flag": p["flag"], "module": p["module"], "name": p["name"],
            "pairs": p["pairs"], "lang_before": p["flags"]["before"]["lang"],
            "lang_after": p["flags"]["after"]["lang"], "lint": p["lint"], "claim": p["claim"],
        } for p in plans],
        "summary": batch_summary(plans, ctx),
    }


def main(argv: list[str] | None = None) -> int:
    ap = build_parser()
    args = ap.parse_args(argv)
    if args.selftest:
        import promote_batch_selftest
        return promote_batch_selftest.selftest()
    if not args.cmd:
        ap.print_help()
        return 2
    ctx = pr.Ctx()
    spec = load_spec(args.spec)
    if args.cmd == "plan":
        plans = plan_batch(ctx, spec)
        if args.json:
            print(json.dumps(plan_json(plans, spec, ctx), indent=2))
        else:
            human_plan(plans, spec, ctx)
        return 0
    if args.cmd == "apply":
        return apply_batch(ctx, spec, args.dry_run, args.allow_claimed, args.manifest,
                           args.allow_conflict)
    return check_batch(ctx, read_manifest(ctx, spec, args.manifest), args.json)


if __name__ == "__main__":
    sys.exit(main())
