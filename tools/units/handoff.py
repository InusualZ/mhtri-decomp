"""Print the report skeleton a worker fills in, and validate what comes back.

docs/plan.md 7.4 / §5.3. Twelve workers in twelve processes produce twelve reports; if those are prose, the
orchestrator reconciles numbers by hand - which is the failure mode the protocol exists to remove. So the
handoff is data: `MAIN/.pi/outbox/<slug>.json`, and this tool both emits the skeleton and refuses a bad one.

    python tools/units/handoff.py <unit>                  # the digest skeleton (a table to fill in the reply)
    python tools/units/handoff.py <unit> --template       # the outbox JSON to fill in
    python tools/units/handoff.py <unit> --check FILE     # validate an outbox entry
    python tools/units/handoff.py --selftest

What `--check` enforces (each is something the orchestrator would otherwise have to notice by hand):

* the required fields exist and have the right types;
* every symbol it reports is a symbol the unit actually owns (a typo or a stale name would silently score 0),
  for the unit a single-unit outbox names *and* for every unit a batch outbox declares (`unit`, `units`,
  `also_changed_units`, `per_unit`) - a batch that registers or touches several units writes one outbox, and
  the ownership check has to read all of them;
* `unit_percent` and each `percent` are in 0-100;
* `measured_with` names the command, so a number can be reproduced and a hand-written compile spotted;
* `config_requests` entries carry the evidence the plan's §8 requires. The accepted kinds and their required
  fields are `lib.outbox.CONFIG_REQUEST_SCHEMA` - the one definition, which `brief.py` renders into the worker's
  brief (part 6). A kind outside the table (`tooling`, `naming`, ...) or a known kind whose structured fields
  are absent is still accepted when it carries its content under a free-text field (`FREE_TEXT_FIELDS`), so a
  real filing is never refused for spelling its field the way its lane does. `flags_probed` is a list of
  `{flags, effect, verdict}` objects, but a prose string or a probe filed under a lane's own keys
  (`flag`/`result`) is accepted as content rather than refused.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from units import brief as brief_mod  # noqa: E402
from units import claims  # noqa: E402
from units import recompile as rc  # noqa: E402
from tools.lib import outbox as _outbox  # noqa: E402  (the schema and the validator: one definition)

# The schema, the free-text fields and the validator are `lib.outbox` (docs/tools/spec/lib-outbox.md): `brief.py`
# renders the same table, `backlog.py` reads the same free-text list, `land.py` refuses on the same errors.
REQUIRED = _outbox.REQUIRED
CONFIG_REQUEST_SCHEMA = _outbox.CONFIG_REQUEST_SCHEMA
CONFIG_KINDS = _outbox.CONFIG_KINDS
CONFIG_NEEDS = _outbox.CONFIG_NEEDS
FLAG_PROBE_FIELDS = _outbox.FLAG_PROBE_FIELDS
FLAG_PROBE_VERDICTS = _outbox.FLAG_PROBE_VERDICTS
FREE_TEXT_FIELDS = _outbox.FREE_TEXT_FIELDS
config_schema_rows = _outbox.config_schema_rows
request_content = _outbox.request_content
symbol_like = _outbox.symbol_like
units_declared = _outbox.units_declared


def owned_symbols(main: str, units: list[str]) -> set[str]:
    """The symbols the given units own, unioned - the ownership check's reference set for a whole batch."""
    owned: set[str] = set()
    for spelling in units:
        unit = claims.norm_unit(spelling.strip("/"))
        rng = brief_mod.splits_range(main, unit)
        if rng.get(".text"):
            owned |= {s["name"] for s in brief_mod.symbols_in_range(main, rng[".text"][0], rng[".text"][1])}
    return owned


def outbox_path(main: str, unit: str) -> str:
    """`claims.outbox_path` - the claim's branch minus `worker/`, the one name brief.py writes and land.py reads."""
    return claims.outbox_path(main, unit)


def template(unit: str, worker: str = "worker-a") -> dict:
    return {
        "unit": unit,
        "worker": worker,
        "finished_at": "<YYYY-MM-DDTHH:MM:SS>",
        "claim_state": "released",
        "object": "<worktree>/build/RMHE08/src/<unit>.o",
        "unit_percent": None,
        "matched_bytes": [None, None],
        "symbols": [{"name": "<symbol>", "percent": None, "note": ""}],
        "residual": "<what still differs and why, or 'none'>",
        "config_requests": [],
        "flags_probed": [{"flags": "<flags>", "effect": "<symbol: before -> after>", "verdict": "reject"}],
        "blockers": [],
        "measured_with": "python tools/units/recompile.py <unit> --measure <symbol>",
    }


def validate(entry: dict, owned: set[str]) -> tuple[list[str], list[str]]:
    """-> (errors, warnings). An error means the batch may not land (`lib.outbox.validate`)."""
    return _outbox.errors_and_warnings(_outbox.validate(entry, owned))


def digest(unit: str, rows: list[dict]) -> str:
    lines = ["| symbol | measured % | note |", "| --- | --- | --- |"]
    for r in rows:
        lines.append("| `%s` | %s | |" % (r["name"], "%.2f" % r["percent"] if r.get("percent") is not None else ""))
    lines.append("")
    lines.append("unit: %s | matched bytes: %s | residual: %s" % (unit, "<ours>/<target>", "<one line>"))
    return "\n".join(lines)


def selftest() -> int:
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    good = {"unit": "auto/x", "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 97.19,
            "symbols": [{"name": "fn_1", "percent": 100.0}], "residual": "none",
            "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}
    check("a good entry passes", validate(good, {"fn_1"})[0], [])

    bad = dict(good, unit_percent=101)
    check("unit_percent range", bool(validate(bad, {"fn_1"})[0]), True)
    bad = dict(good, symbols=[{"name": "fn_other", "percent": 10.0}])
    check("unknown symbol rejected", any("not owned" in e for e in validate(bad, {"fn_1"})[0]), True)
    bad = dict(good, symbols=[{"name": "fn_1"}])
    check("missing percent rejected", bool(validate(bad, {"fn_1"})[0]), True)
    check("missing residual rejected", bool(validate({k: v for k, v in good.items() if k != "residual"}, {"fn_1"})[0]), True)
    check("missing measured_with rejected",
          bool(validate({k: v for k, v in good.items() if k != "measured_with"}, {"fn_1"})[0]), True)
    check("a rename without evidence rejected",
          bool(validate(dict(good, config_requests=[{"kind": "rename", "old": "a", "new": "b"}]), {"fn_1"})[0]), True)
    check("a good rename accepted",
          validate(dict(good, config_requests=[{"kind": "rename", "old": "a", "new": "b", "evidence": "map"}]), {"fn_1"})[0], [])
    check("a bad flag verdict rejected",
          bool(validate(dict(good, flags_probed=[{"flags": "-O4,p", "effect": "x", "verdict": "maybe"}]), {"fn_1"})[0]), True)
    check("an unknown config kind rejected",
          bool(validate(dict(good, config_requests=[{"kind": "vibe"}]), {"fn_1"})[0]), True)
    check("the brief's old `data` kind is not silently accepted",
          bool(validate(dict(good, config_requests=[{"kind": "data", "section": ".data"}]), {"fn_1"})[0]), True)
    for row in config_schema_rows():
        check("a %s without its required fields is rejected" % row["kind"],
              bool(validate(dict(good, config_requests=[{"kind": row["kind"]}]), {"fn_1"})[0]), True)
    # A range/seam filed with its content in a free-text field rather than section/start/end is a real filing.
    check("a span-less range with its content in `why` is accepted",
          validate(dict(good, config_requests=[{"kind": "range", "why": "the right seam is unproven"}]),
                   {"fn_1"})[0], [])
    check("a range with content under `request` is accepted",
          validate(dict(good, config_requests=[{"kind": "range", "subject": "a", "request": "settle it"}]),
                   {"fn_1"})[0], [])
    check("a shared-file with its content in `request` (not `why`) is accepted",
          validate(dict(good, config_requests=[{"kind": "shared-file", "subject": "x.h",
                                               "request": "move the declaration"}]), {"fn_1"})[0], [])
    check("an out-of-schema kind with free-text content is accepted (a lane's own spelling)",
          validate(dict(good, config_requests=[{"kind": "tooling", "file": "tools/x.py",
                                                "why": "--target would help"}]), {"fn_1"})[0], [])
    check("an out-of-schema kind with no content is still rejected",
          bool(validate(dict(good, config_requests=[{"kind": "tooling"}]), {"fn_1"})[0]), True)
    check("residual 'none' is allowed", validate(dict(good, residual="none"), {"fn_1"})[0], [])
    with tempfile.TemporaryDirectory() as tmp:
        rq = os.path.join(tmp, "lane-a-requests.json")
        with open(rq, "w", encoding="utf-8") as fh:
            fh.write('{"id": "lane-a#1", "kind": "rename", "symbol": "fn_1", "proposed_name": "doIt", "evidence": "x"}\n'
                     '{"id": "lane-a#2", "kind": "decl", "symbol": "fn_2", "evidence": "x"}\n'
                     '{"kind": "rename", "symbol": "fn_3", "proposed": "doThat", "evidence": "x"}\n')
        errs, notes = _outbox.check_requests(rq)
        check("--check-requests: a decl of a generated name without a name is an error", errs,
              ["line 2: decl of the generated name fn_2 needs `proposed_name` (rule 7: the integrator names it)"])
        check("--check-requests: a free-text line is a note, not an error", len(notes), 1)

    # the outbox a worker writes by following the brief's schema table must validate clean: this is the
    # round-trip the two tools share (brief.py renders `config_schema_rows()`, the worker fills it in).
    brief_shaped = dict(good, config_requests=[], flags_probed=[])
    for row in config_schema_rows():
        req = {"kind": row["kind"]}
        for field in row["needs"]:
            req[field] = "<%s>" % field
        brief_shaped["config_requests"].append(req)
    brief_shaped["flags_probed"].append({"flags": "<flags>", "effect": "<symbol: before -> after>",
                                          "verdict": "reject"})
    check("a brief-shaped outbox validates clean", validate(brief_shaped, {"fn_1"})[0], [])
    # Real lanes file probes as prose strings, under `flag`/`result`, or with a parenthetical verdict; each is
    # content, not a defect (the 2026-09-28 `Network` outboxes failed on exactly these).
    check("a flags_probed prose string is accepted",
          validate(dict(good, flags_probed=["-O2: unit 89.58 %, rejected"]), {"fn_1"})[0], [])
    check("an empty flags_probed string is rejected",
          bool(validate(dict(good, flags_probed=["  "]), {"fn_1"})[0]), True)
    check("a probe under a lane's own `flag`/`result` keys is accepted",
          validate(dict(good, flags_probed=[{"flag": "-use_lmw_stmw off", "result": "no change"}]),
                   {"fn_1"})[0], [])
    check("a probe with a parenthetical verdict is accepted",
          validate(dict(good, flags_probed=[{"flags": "-O3", "effect": "+9%",
                                             "verdict": "adopt (none needed for codegen)"}]), {"fn_1"})[0], [])
    check("the `kept` verdict is accepted",
          validate(dict(good, flags_probed=[{"flags": "cflags_menu", "effect": "no change",
                                             "verdict": "kept"}]), {"fn_1"})[0], [])
    check("a probe that names no field at all is rejected",
          bool(validate(dict(good, flags_probed=[{}]), {"fn_1"})[0]), True)
    check("a template passes structurally",
          validate(template("auto/x") | {"symbols": [{"name": "fn_1", "percent": 0.0}], "unit_percent": 0.0,
                                         "residual": "none"}, {"fn_1"})[0], [])
    d = digest("u", [{"name": "fn_1", "percent": 50.0}])
    check("digest has a header and the row", ("| symbol | measured % | note |" in d and "| `fn_1` | 50.00 |" in d), True)

    # the outbox this tool names is the branch-derived one land.py's gate reads, not a name re-derived here
    import tempfile
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi"), exist_ok=True)
        claims.save_registry(tmp, {"Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act") + "-abcd"}})
        check("the outbox is the claim's branch minus worker/",
              os.path.basename(outbox_path(tmp, "Pl/pl_act")), claims.slug("Pl/pl_act") + "-abcd.json")
        check("handoff's outbox is claims.py's outbox",
              outbox_path(tmp, "Pl/pl_act"), claims.outbox_path(tmp, "Pl/pl_act"))
        check("an unclaimed unit still names a path to look at",
              os.path.basename(outbox_path(tmp, "RSO/runtime")), claims.slug("RSO/runtime") + ".json")

    # A batch outbox names several units; the ownership check must read every declared spelling, so a symbol
    # owned by the second unit is not "not owned by this unit" (which forced multi-unit batches to
    # `--no-outbox`, losing the outbox check entirely).
    check("units_declared reads `unit` + a batch's `units`/`also_changed_units`/`per_unit`",
          units_declared({"unit": "A/a.c", "units": [{"name": "A/a"}, "B/b"],
                          "also_changed_units": ["C/c"], "per_unit": [{"unit": "D/d"}]}),
          ["A/a.c", "A/a", "B/b", "C/c", "D/d"])
    check("units_declared does not split a prose `unit` (its `+`-tokens are not reliably units)",
          units_declared({"unit": "A/a.c + B/b.c (2 units; ...)"}), ["A/a.c + B/b.c (2 units; ...)"])
    check("units_declared is empty for an outbox that names no unit", units_declared({}), [])
    check("a summary row (`80 more symbols`) is not ownership-checked",
          symbol_like("80 more symbols"), False)
    check("a real symbol is ownership-checked", symbol_like("fn_80128A8C"), True)
    with tempfile.TemporaryDirectory() as tmp:
        cfg = os.path.join(tmp, "config", "RMHE08")
        os.makedirs(cfg)
        with open(os.path.join(cfg, "splits.txt"), "w", encoding="utf-8") as fh:
            fh.write("A/a.c:\n    .text start:0x100 end:0x200\nB/b.c:\n    .text start:0x200 end:0x300\n")
        with open(os.path.join(cfg, "symbols.txt"), "w", encoding="utf-8") as fh:
            fh.write("fn_a = .text:0x100; // type:function size:0x10\n"
                     "fn_b = .text:0x200; // type:function size:0x10\n")
        check("owned_symbols unions a batch's units",
              owned_symbols(tmp, ["A/a", "B/b"]), {"fn_a", "fn_b"})
        check("owned_symbols of one unit is just that unit", owned_symbols(tmp, ["A/a"]), {"fn_a"})
        entry = dict(good, unit="A/a.c + B/b.c", units=["A/a", "B/b"],
                     symbols=[{"name": "fn_a", "percent": 50.0}, {"name": "fn_b", "percent": 50.0}])
        errors, _ = validate(entry, owned_symbols(tmp, units_declared(entry)))
        check("a multi-unit outbox validates against the batch's whole owned set", errors, [])
        single, _ = validate(entry, owned_symbols(tmp, ["A/a"]))
        check("... validating against one unit alone would flag the other's symbol",
              bool(single), True)
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", nargs="?")
    ap.add_argument("--template", action="store_true", help="print the outbox JSON to fill in")
    ap.add_argument("--check", default=None, metavar="FILE", help="validate an outbox entry")
    ap.add_argument("--check-requests", default=None, metavar="FILE",
                    help="validate a lane's <slug>-requests.json (lib.requests schema; free-text lines are noted)")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    if args.check_requests:
        errors, notes = _outbox.check_requests(args.check_requests)
        for n in notes:
            print("note: %s" % n)
        for e in errors:
            print("ERROR: %s" % e)
        print("%d error(s), %d free-text line(s)" % (len(errors), len(notes)))
        return 1 if errors else 0

    wt = rc.worktree_root()
    main = rc.main_root(wt)

    if args.check:
        entry = json.loads(open(args.check, encoding="utf-8").read())
        # The ownership check reads every unit the outbox declares (a batch names several in one file), not
        # just `unit` - otherwise the second unit's symbols are all "not owned" and the batch cannot validate.
        declared = units_declared(entry)
        if not declared:
            fallback = claims.norm_unit((args.unit or "").strip("/"))
            declared = [fallback] if fallback else []
        owned = owned_symbols(main, declared)
        errors, warnings = validate(entry, owned)
        if args.json:
            print(json.dumps({"errors": errors, "warnings": warnings}, indent=2))
        else:
            for w in warnings:
                print("warn: %s" % w)
            for e in errors:
                print("ERROR: %s" % e)
            print("%d error(s), %d warning(s) - %s"
                  % (len(errors), len(warnings), "OK to land" if not errors else "do not land this"))
        return 1 if errors else 0

    if not args.unit:
        ap.print_help()
        return 0
    unit = claims.norm_unit(args.unit.strip("/"))
    rng = brief_mod.splits_range(main, unit)
    rows = brief_mod.symbols_in_range(main, *rng[".text"][:2]) if rng.get(".text") else []
    scores = brief_mod.report_scores(main, unit)
    for row in rows:
        row["percent"] = scores.get(row["name"])
    if args.template:
        print(json.dumps(template(unit), indent=2))
        return 0
    print(digest(unit, rows))
    print("\noutbox: %s" % outbox_path(main, unit))
    print("notes:  %s" % claims.notes_path(main, unit))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
