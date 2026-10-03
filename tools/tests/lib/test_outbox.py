"""lib.outbox: the schema, validation as Findings, the declared units, tolerant loading of outboxes and notes."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import io
import json
import os
import tempfile

from tools.lib import outbox as ob
from tools.lib import testing

TIER = "fixture"

GOOD = {"unit": "auto/x", "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 97.19,
        "symbols": [{"name": "fn_1", "percent": 100.0}], "residual": "none",
        "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}


def errors(entry, owned=frozenset({"fn_1"})):
    return ob.errors_and_warnings(ob.validate(entry, set(owned)))[0]


def test_validate(c):
    c.check("a good entry passes", errors(GOOD), [])
    c.check("unit_percent range", errors(dict(GOOD, unit_percent=101)), ["unit_percent out of range: 101"])
    c.check("an unowned symbol", errors(dict(GOOD, symbols=[{"name": "fn_2", "percent": 1.0}])),
            ["symbols[0] `fn_2` is not owned by this unit"])
    c.check("a summary row is not ownership-checked", errors(dict(GOOD, symbols=[{"name": "80 more", "percent": 1.0}])), [])
    c.check("missing fields", errors({k: v for k, v in GOOD.items() if k not in ("residual", "measured_with")}),
            ["missing field `residual`", "missing field `measured_with`",
             "residual must be a non-empty string ('none' is a valid answer)"])
    for row in ob.config_schema_rows():
        c.check("a %s without its fields is refused" % row["kind"],
                bool(errors(dict(GOOD, config_requests=[{"kind": row["kind"]}]))), True)
        c.check("... accepted with content under a free-text field",
                errors(dict(GOOD, config_requests=[{"kind": row["kind"], "request": "settle it"}])), [])
    c.check("an out-of-schema kind with content is a filing",
            errors(dict(GOOD, config_requests=[{"kind": "tooling", "why": "x"}])), [])
    c.check("... without content it is refused", len(errors(dict(GOOD, config_requests=[{"kind": "tooling"}]))), 1)
    c.check("a prose probe and a lane's own keys are content",
            errors(dict(GOOD, flags_probed=["-O2: worse", {"flag": "-O3", "result": "same"}])), [])
    c.check("a bad verdict is refused", len(errors(dict(GOOD, flags_probed=[{"flags": "a", "effect": "b",
                                                                            "verdict": "maybe"}]))), 1)
    fs = ob.validate(dict(GOOD, blockers=[], flags_probed=[{"flag": "-O3"}]), {"fn_1"}, file="o.json")
    c.check("warnings carry the WARNING rule, the field token and the file",
            [(f.rule, f.token, f.file) for f in fs], [("outbox-warning", "flags_probed[0]", "o.json"),
                                                       ("outbox-warning", "blockers", "o.json")])


def test_declared_and_text(c):
    c.check("units_declared", ob.units_declared({"unit": "A/a.c", "units": [{"name": "A/a"}, "B/b"],
                                                  "also_changed_units": ["C/c"], "per_unit": [{"unit": "D/d"}]}),
            ["A/a.c", "A/a", "B/b", "C/c", "D/d"])
    c.check("a prose unit is not split", ob.units_declared({"unit": "A + B (2 units)"}), ["A + B (2 units)"])
    c.check("request_content takes the first non-empty free text",
            ob.request_content({"evidence": " ", "why": "", "request": "r", "note": "n"}), "r")
    c.check("asstr", [ob.asstr(x) for x in ("s", None, ["-opt", "x"])], ["s", "", '["-opt", "x"]'])


def test_loading(c):
    with tempfile.TemporaryDirectory() as tmp:
        box, notes = os.path.join(tmp, "outbox"), os.path.join(tmp, "notes")
        os.makedirs(box)
        os.makedirs(notes)
        for name, body in (("b.json", json.dumps(dict(GOOD, units=["B/b"]))), ("a.json", "{not json"),
                           ("c.json", "[1, 2]")):
            with open(os.path.join(box, name), "w", encoding="utf-8") as fh:
                fh.write(body)
        with open(os.path.join(notes, "b.md"), "w", encoding="utf-8") as fh:
            fh.write("# note\n")
        warn = io.StringIO()
        got = ob.load_outboxes(box, warn=warn)
        c.check("a bad file is skipped, sorted by path", [e.stem for e in got], ["b"])
        c.check("... and named on the warning stream", [ln.split(" (")[0] for ln in warn.getvalue().splitlines()],
                ["warn: skipping a.json", "warn: skipping c.json"])
        c.check("keep_bad keeps it with its error", [bool(e.error) for e in ob.load_outboxes(box, keep_bad=True)],
                [True, False, True])
        e = got[0]
        c.check("Entry accessors", (e.text("unit"), e.units(), e.symbols(), e.requests()),
                ("auto/x", ["auto/x", "B/b"], ["fn_1"], []))
        c.check("load_notes", ob.load_notes(notes), {"b": "# note\n"})
        c.check("a missing directory is empty", (ob.load_outboxes(os.path.join(tmp, "none")), ob.load_notes(tmp)),
                ([], {}))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
