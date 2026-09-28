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
  fields are `CONFIG_REQUEST_SCHEMA` below - the one definition, which `brief.py` renders into the worker's
  brief (part 6). A kind outside the table (`tooling`, `naming`, ...) or a known kind whose structured fields
  are absent is still accepted when it carries its content under a free-text field (`FREE_TEXT_FIELDS`), so a
  real filing is never refused for spelling its field the way its lane does. `flags_probed` is a list of
  `{flags, effect, verdict}` objects, but a prose string or a probe filed under a lane's own keys
  (`flag`/`result`) is accepted as content rather than refused.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from units import brief as brief_mod  # noqa: E402
from units import claims  # noqa: E402
from units import recompile as rc  # noqa: E402

REQUIRED = ("unit", "worker", "finished_at", "unit_percent", "symbols", "residual", "measured_with")

# The `config_requests` schema: the kind, the fields this validator *requires* and the fields the
# orchestrator reads when they are present. `brief.py` renders this table verbatim (part 6), so the brief
# asks for exactly what the validator accepts - the two cannot drift, because there is one definition.
#
# Why this direction (the brief copies the schema, not the reverse): the kind name carries the required
# fields. `range` means section/start/end, `rename` means old/new/evidence, and a synonym such as `data` or
# `tool` would have to be mapped back to a canonical kind before those checks run - two vocabularies and two
# chances for a typo to skip a required-field check. The brief is machine-generated, so it can carry the
# exact vocabulary at no cost. The 2026-09-23 `RSO/runtime` round produced an 18-error outbox precisely
# because the brief named no schema and the worker invented `data`/`flags`/`tool` and a non-dict
# `flags_probed`; a brief that states the schema is the fix, not a validator that guesses.
CONFIG_REQUEST_SCHEMA = (
    {"kind": "range", "needs": ("section", "start", "end"), "also": ("evidence",),
     "means": "a data range this unit owns (a splits.txt range plus its configure.py entry)"},
    {"kind": "seam", "needs": ("section", "start", "end", "evidence"), "also": ("why",),
     "means": "a seam finding: this code span's boundary is in the wrong place and the unit split should be "
              "re-drawn - distinct from `range`, which claims a data run this unit already owns"},
    {"kind": "rename", "needs": ("old", "new", "evidence"), "also": (),
     "means": "a map-symbol rename, with the evidence for the new name"},
    {"kind": "flag", "needs": ("evidence",), "also": ("lib", "change"),
     "means": "a compiler flag for a lib, with the probe numbers that justify it"},
    {"kind": "shared-file", "needs": ("why",), "also": ("file",),
     "means": "an edit to a file a worker may not touch (a tool, configure.py, splits.txt)"},
)
CONFIG_KINDS = tuple(row["kind"] for row in CONFIG_REQUEST_SCHEMA)
CONFIG_NEEDS = {row["kind"]: row["needs"] for row in CONFIG_REQUEST_SCHEMA}
FLAG_PROBE_FIELDS = ("flags", "effect", "verdict")
FLAG_PROBE_VERDICTS = ("reject", "adopt", "inconclusive", "kept")

# The free-text fields a lane files its content under when it does not use - or the schema does not name -
# the kind's structured fields. The schema and `backlog.py`'s intake have to agree about what a filing *is*:
# a request with its content in `why`/`request`/`what`/`subject`/`note`/`evidence` is a real filing, and
# neither the validator nor `_add_request` may drop it for spelling its field differently. This is the one
# list both sides read (backlog imports it), so the two cannot drift.
FREE_TEXT_FIELDS = ("evidence", "why", "request", "what", "subject", "note")


def config_schema_rows() -> list[dict]:
    """The schema table `brief.py` renders - the one definition both tools read."""
    return [dict(row) for row in CONFIG_REQUEST_SCHEMA]


def request_content(req: dict) -> str:
    """A request's free-text content: the first non-empty `FREE_TEXT_FIELDS` value (`""` when none)."""
    for field in FREE_TEXT_FIELDS:
        value = req.get(field)
        if isinstance(value, str) and value.strip():
            return value
    return ""


def symbol_like(name: str) -> bool:
    """Whether `name` is one symbol and not a summary row (`"80 more symbols"`, `"a / b / c"`).

    The ownership check can only speak about a name the map could hold; a prose line in `symbols` is a human
    summary, and flagging it "not owned" is a false positive that would refuse a real batch.
    """
    s = (name or "").strip()
    return bool(s) and " " not in s and "/" not in s


def units_declared(entry: dict) -> list[str]:
    """The unit spelling(s) an outbox declares: `unit`, plus a batch's `units`/`also_changed_units`/`per_unit`.

    A batch that registers or touches several units writes one outbox (the branch's) and names them here. The
    ownership check has to read all of them, or every symbol of the second unit is "not owned by this unit"
    and the batch can never satisfy the gate - which is how an outbox check gets skipped with `--no-outbox`
    and a filed request goes missing. Only the explicit list structure is read: the `unit` field is sometimes
    a prose summary (`"A + B (2 units; ...)"`) whose `+`-tokens are not reliably the batch's units, and
    splitting it would turn a previously-skipped ownership check on for records whose "symbols" are prose.
    """
    out: list[str] = []

    def add(value) -> None:
        if isinstance(value, str) and value.strip():
            out.append(value.strip())

    add(entry.get("unit"))
    for key in ("units", "also_changed_units", "changed_units"):
        value = entry.get(key)
        if isinstance(value, list):
            for item in value:
                add((item.get("unit") or item.get("name")) if isinstance(item, dict) else item)
    for item in (entry.get("per_unit") or []):
        if isinstance(item, dict):
            add(item.get("unit") or item.get("name"))
    return list(dict.fromkeys(out))


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
    """-> (errors, warnings). An error means the batch may not land."""
    errors, warnings = [], []
    for key in REQUIRED:
        if key not in entry:
            errors.append("missing field `%s`" % key)
    pct = entry.get("unit_percent")
    if not isinstance(pct, (int, float)):
        errors.append("unit_percent must be a number")
    elif not 0 <= pct <= 100:
        errors.append("unit_percent out of range: %r" % pct)
    syms = entry.get("symbols")
    if not isinstance(syms, list) or not syms:
        errors.append("symbols must be a non-empty list")
    else:
        for i, s in enumerate(syms):
            if not isinstance(s, dict) or not s.get("name"):
                errors.append("symbols[%d] has no name" % i)
                continue
            value = s.get("percent")
            if not isinstance(value, (int, float)):
                errors.append("symbols[%d] (%s) has no numeric percent" % (i, s["name"]))
            elif not 0 <= value <= 100:
                errors.append("symbols[%d] (%s) percent out of range: %r" % (i, s["name"], value))
            if owned and symbol_like(s["name"]) and s["name"] not in owned:
                errors.append("symbols[%d] `%s` is not owned by this unit" % (i, s["name"]))
    if not isinstance(entry.get("residual"), str) or not entry.get("residual", "").strip():
        errors.append("residual must be a non-empty string ('none' is a valid answer)")
    if "measured_with" in entry and (not isinstance(entry.get("measured_with"), str)
                                     or not entry.get("measured_with", "").strip()):
        errors.append("measured_with must name the command the numbers came from")
    for i, req in enumerate(entry.get("config_requests") or []):
        if not isinstance(req, dict):
            errors.append("config_requests[%d] is not an object" % i)
            continue
        kind = req.get("kind")
        content = request_content(req)
        if kind not in CONFIG_NEEDS:
            # An out-of-schema kind (`tooling`, `naming`, `done-in-this-fold`) is a real filing the schema has
            # not caught up with: accept it when it carries free-text content and refuse it only when empty,
            # so a lane's own kind name cannot cost it a landing (or its request).
            if not content:
                errors.append("config_requests[%d] has kind %r (not one of %s) and no free-text content (%s)"
                              % (i, kind, list(CONFIG_KINDS), ", ".join(FREE_TEXT_FIELDS)))
            continue
        missing = [f for f in CONFIG_NEEDS[kind] if f not in req or req.get(f) in (None, "")]
        if missing and not content:
            errors.append("config_requests[%d] (%s) needs %s (or content under one of %s)"
                          % (i, kind, ", ".join(missing), ", ".join(FREE_TEXT_FIELDS)))
    for i, probe in enumerate(entry.get("flags_probed") or []):
        if isinstance(probe, str):
            if not probe.strip():
                errors.append("flags_probed[%d] is an empty string" % i)
            continue
        if not isinstance(probe, dict):
            errors.append("flags_probed[%d] is not an object or a string (it needs flags, effect, verdict)" % i)
            continue
        flags = str(probe.get("flags") or probe.get("flag") or "").strip()
        effect = str(probe.get("effect") or probe.get("result") or probe.get("evidence") or "").strip()
        verdict = str(probe.get("verdict") or "").strip()
        if not (flags or effect or verdict):
            errors.append("flags_probed[%d] carries no flags/effect/verdict content" % i)
            continue
        missing = [name for name, value in (("flags", flags), ("effect", effect), ("verdict", verdict))
                   if not value]
        if missing:
            # A probe filed under a lane's own keys (`flag`/`result`) or without a verdict is still content;
            # the gate refuses a *bad verdict*, never a probe that spells its fields differently.
            warnings.append("flags_probed[%d] does not name %s" % (i, ", ".join(missing)))
        elif not any(verdict.lower().startswith(v) for v in FLAG_PROBE_VERDICTS):
            errors.append("flags_probed[%d] verdict %r is not one of %s"
                          % (i, probe.get("verdict"), "/".join(FLAG_PROBE_VERDICTS)))
    if entry.get("claim_state") not in (None, "released", "open"):
        warnings.append("claim_state %r is not open or released" % entry["claim_state"])
    if not (entry.get("blockers") or []):
        warnings.append("no blockers listed ('[]' is a valid answer)")
    return errors, warnings


def digest(unit: str, rows: list[dict]) -> str:
    lines = ["| symbol | measured % | note |", "| --- | --- | --- |"]
    for r in rows:
        lines.append("| `%s` | %s | |" % (r["name"], "%.2f" % r["percent"] if r.get("percent") is not None else ""))
    lines.append("")
    lines.append("unit: %s | matched bytes: %s | residual: %s" % (unit, "<ours>/<target>", "<one line>"))
    return "\n".join(lines)


def selftest() -> int:
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
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

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
