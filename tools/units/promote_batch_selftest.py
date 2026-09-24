"""Self-test for `tools/units/promote_batch.py` - the spec, the batch plan, the sequential apply and
the batch check.

The three things a batch adds over `promote.py` are what this checks:

* the **spec** - comments/blanks skipped, `promote.py`'s options plus `--lang`, duplicates and unknown
  options refused;
* the **sequential apply** - every entry is re-planned against the tree the previous one left (so the
  `splits.txt`/`configure.py` line indices cannot go stale), one manifest is written, and a language
  promotion leaves the unit's pooled brief alone (the stem did not change);
* the **batch check** - the build-graph gate still refuses a unit that is not in the graph, a
  promotion whose bytes moved fails, and a language change whose bytes moved is reported, not failed.

The fixture is `promote_selftest.build_fixture`'s, so the two tools are tested against the same tree;
`apply` is exercised with the same `git` shim.

    python tools/units/promote_batch_selftest.py
    python tools/units/promote_batch.py --selftest
"""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import promote as pr  # noqa: E402
import promote_batch as pb  # noqa: E402
import promote_selftest as ps  # noqa: E402

SPEC = """\
# promote batch selftest
auto/80001000_fn_80001000  --lang c++      # in place: the front-end changes, the stem stays

auto/80002000_fn_80002000  --name second --module Pl   # a real promotion (rename + move)
"""


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def message(fn):
        try:
            fn()
            return ""
        except SystemExit as exc:
            return str(exc)
        except Exception as exc:                      # noqa: BLE001 - the selftest wants the text
            return "%s: %s" % (type(exc).__name__, exc)

    tmp = Path(tempfile.mkdtemp(prefix="promote-batch-selftest-"))
    try:
        # -- the spec --------------------------------------------------------------------------
        check("spec: comments and blanks are skipped",
              len(pb.load_spec(str(_write(tmp / "s.txt", SPEC)))["entries"]), 2)
        e = pb.parse_entry("auto/X --lang c++ --symbol a=b --note hi", 1)
        check("spec: --lang parsed", e.lang, "c++")
        check("spec: --symbol repeats", e.symbols, ["a=b"])
        check("spec: --note parsed", e.note, "hi")
        check("spec: a bad --lang is refused",
              "--lang 'x' is not c or c++" in message(lambda: pb.parse_entry("auto/X --lang x", 1)),
              True)
        check("spec: an unknown option is refused",
              "unknown option" in message(lambda: pb.parse_entry("auto/X --nope", 1)), True)
        check("spec: a missing value is refused",
              "needs a value" in message(lambda: pb.parse_entry("auto/X --name", 1)), True)
        dup = _write(tmp / "dup.txt", "auto/X --lang c++\nauto/X.c --lang c\n")
        check("spec: a duplicate unit is refused",
              "twice" in message(lambda: pb.load_spec(str(dup))), True)
        empty = _write(tmp / "empty.txt", "# nothing\n")
        check("spec: an empty spec is refused",
              "no promotions" in message(lambda: pb.load_spec(str(empty))), True)

        # -- the fixture -----------------------------------------------------------------------
        ctx = ps.build_fixture(tmp)
        spec = pb.load_spec(str(_write(tmp / "batch.txt", SPEC)))
        plans = pb.plan_batch(ctx, spec, conflict_check=False)
        check("plan: two units", len(plans), 2)
        lang, promo = plans
        check("plan: the language entry is a language change", lang["_kind"], pb.KIND_LANGUAGE)
        check("plan: the language entry is in place", lang["_in_place"], True)
        check("plan: the language entry keeps the stem", lang["new_unit"],
              "auto/80001000_fn_80001000.cpp")
        check("plan: the language entry keeps the lib", lang["lib"], "auto")
        check("plan: the language entry's front-end delta",
              (lang["flags"]["before"]["lang"], lang["flags"]["after"]["lang"]), ("-lang=c", "-lang=c++"))
        check("plan: the language entry adds no rule-7 finding", lang["lint"]["count"], 0)
        check("plan: the language entry keeps its pooled brief", lang["pool"], [])
        check("plan: the promotion entry is a promotion", promo["_kind"], pb.KIND_PROMOTION)
        check("plan: the promotion moves module", promo["new_unit"], "Pl/second.c")
        check("plan: the promotion preserves the Matching flag", promo["flag"], "Matching")
        check("plan: the promotion drops the stale pooled brief? (it has none)", promo["pool"], [])
        s = pb.batch_summary(plans, ctx)
        check("summary: counts", (s["units"], s["languages"], s["promotions"]), (2, 1, 1))
        check("summary: the unit-key moves (the language extension is one too)", s["moves"],
              [("auto/80001000_fn_80001000.c", "auto/80001000_fn_80001000.cpp"),
               ("auto/80002000_fn_80002000.c", "Pl/second.c")])
        check("summary: the shared files", s["shared_files"], ["config/RMHE08/splits.txt",
                                                              "configure.py"])
        check("summary: the language delta list", s["lang_changes"],
              [("auto/80001000_fn_80001000.c", "-lang=c", "-lang=c++")])
        check("plan: renders without error", ps.silent(lambda: pb.human_plan(plans, spec, ctx)) is None,
              True)
        check("plan: --json is serialisable", json.dumps(pb.plan_json(plans, spec, ctx))[:1], "{")

        # the refusals a batch inherits
        bad = pb.load_spec(str(_write(tmp / "bad.txt", "auto/nope --lang c++\n")))
        check("plan: an unregistered unit is refused",
              "not a registered source" in message(lambda: pb.plan_batch(ctx, bad, False)), True)

        # a boundary defect (two source files in one object) is surfaced and refused by apply
        real_conflict = pb.detect_conflict
        pb.detect_conflict = lambda c, reg: ["a.cpp", "b.cpp"]
        try:
            p_conf = pb.entry_plan(ctx, spec["entries"][0])
            check("conflict: the plan records the two source names", p_conf["_conflict"],
                  ["a.cpp", "b.cpp"])
            check("conflict: the summary counts it",
                  pb.batch_summary([p_conf], ctx)["conflicts"],
                  [("auto/80001000_fn_80001000.c", ["a.cpp", "b.cpp"])])
            rc, out = ps.captured(lambda: pb.apply_batch(ctx, spec, allow_conflict=False))
            check("conflict: apply refuses it", rc, 1)
            check("conflict: the refusal names the defect", "two source files" in out, True)
            check("conflict: nothing was written",
                  (ctx.src / "auto" / "80001000_fn_80001000.c").is_file(), True)
        finally:
            pb.detect_conflict = real_conflict

        # -- apply (sequential, with the git shim) ---------------------------------------------
        ctx.object_dir.joinpath("auto").mkdir(parents=True, exist_ok=True)
        ps.write_elf(ctx.object_dir / "auto" / "80001000_fn_80001000.o")
        ps.write_elf(ctx.object_dir / "auto" / "80002000_fn_80002000.o")
        watched = [ctx.configure, ctx.splits, ctx.mapfile,
                   ctx.src / "auto" / "80001000_fn_80001000.c",
                   ctx.src / "auto" / "80002000_fn_80002000.c"]
        before = [p.read_bytes() for p in watched]
        rc = ps.silent(lambda: pb.apply_batch(ctx, spec, dry_run=True, conflict_check=False))
        check("apply --dry-run: returns 0", rc, 0)
        check("apply --dry-run: writes nothing", [p.read_bytes() for p in watched], before)
        check("apply --dry-run: no manifest",
              not (ctx.root / ".pi" / "promote" / "batch-batch.json").exists(), True)

        real_git = pr.git

        def fake_git(root, *args, check=True):
            if args[:1] == ("mv",):
                src, dst = Path(root) / args[1], Path(root) / args[2]
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(src), str(dst))
                return ""
            if args[:1] == ("status",):
                return ""
            raise AssertionError("unexpected git call: %r" % (args,))

        pr.git = fake_git
        try:
            rc = ps.silent(lambda: pb.apply_batch(ctx, spec, conflict_check=False))
        finally:
            pr.git = real_git
        check("apply: returns 0", rc, 0)
        check("apply: the language unit moved extension",
              (ctx.src / "auto" / "80001000_fn_80001000.cpp").is_file(), True)
        check("apply: the language unit's old file is gone",
              (ctx.src / "auto" / "80001000_fn_80001000.c").exists(), False)
        check("apply: the promotion moved",
              (ctx.src / "Pl" / "second.c").is_file(), True)
        check("apply: the language entry kept its pooled brief",
              [p.name for p in ctx.pool.glob("*.md")], ["80001000-fn-80001000-abcd.md"])
        conf = pr.read_text(ctx.configure)
        check("apply: configure has the language unit at the new extension",
              '"auto/80001000_fn_80001000.cpp"' in conf, True)
        check("apply: configure has the promotion in Pl",
              'Object(Matching, "Pl/second.c")' in conf, True)
        check("apply: the auto lib kept its other unit? (it is the one that moved)",
              '"auto/80002000_fn_80002000.c"' in conf, False)
        splits = pr.read_text(ctx.splits)
        check("apply: the language splits key moved",
              "auto/80001000_fn_80001000.cpp:" in splits, True)
        check("apply: the promotion splits key moved", "Pl/second.c:" in splits, True)
        check("apply: no old splits key is left",
              ("auto/80001000_fn_80001000.c:" in splits
               or "auto/80002000_fn_80002000.c:" in splits), False)
        mpath = ctx.root / ".pi" / "promote" / "batch-batch.json"
        check("apply: the manifest was written", mpath.is_file(), True)
        manifest = json.loads(mpath.read_text(encoding="utf-8"))
        check("apply: the manifest has both records", len(manifest["records"]), 2)
        check("apply: the manifest records the language kind", manifest["records"][0]["kind"],
              pb.KIND_LANGUAGE)
        check("apply: the manifest records the lang delta",
              (manifest["records"][0]["lang_before"], manifest["records"][0]["lang_after"]),
              ("-lang=c", "-lang=c++"))
        check("apply: the before-object was snapshotted",
              (ctx.scratch / "auto_80001000_fn_80001000.c.before.o").is_file(), True)
        check("apply: the manifest names the new object",
              manifest["records"][0]["new_obj"],
              "build/RMHE08/src/auto/80001000_fn_80001000.o")

        # -- check -----------------------------------------------------------------------------
        # no build graph yet: the language unit's source exists but config.json/build.ninja do not
        ctx.build_config.parent.mkdir(parents=True, exist_ok=True)
        ctx.build_config.write_text(json.dumps({"units": [], "modules": []}), encoding="utf-8")
        check("check: a unit outside the build graph is refused",
              ps.silent(lambda: pb.check_batch(ctx, manifest)), 1)

        # put both units in the graph, and build the two "after" objects
        ctx.build_config.write_text(json.dumps(
            {"units": [{"name": "auto/80001000_fn_80001000.cpp"}, {"name": "Pl/second.c"}],
             "modules": []}), encoding="utf-8")
        with open(ctx.build_ninja, "a", encoding="utf-8", newline="") as f:
            f.write("build build/RMHE08/src/auto/80001000_fn_80001000.o: mwcc_sjis "
                    "src/auto/80001000_fn_80001000.cpp\n"
                    "  mw_version = Wii/1.3\n  cflags = -nodefaults -O3 -lang=c++\n"
                    "build build/RMHE08/src/Pl/second.o: mwcc_sjis src/Pl/second.c\n"
                    "  mw_version = Wii/1.0\n  cflags = -nodefaults -O3 -lang=c\n")
        ctx.object_dir.joinpath("Pl").mkdir(parents=True, exist_ok=True)
        # the language unit: the bytes moved (the front-end changed) - reported, not failed
        ps.write_elf(ctx.object_dir / "auto" / "80001000_fn_80001000.o",
                     text=b"\x10\x00\x00\x00" * 3 + b"\x11")
        # the promotion: byte-identical modulo the file/symbol names - the expected verdict
        ps.write_elf(ctx.object_dir / "Pl" / "second.o", syms=[
            ("", 0, 0, 0, 0), ("moved.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)])
        rc, out = ps.captured(lambda: pb.check_batch(ctx, manifest))
        check("check: the batch passes", rc, 0)
        check("check: the language change is reported as differs",
              "auto/80001000_fn_80001000.cpp" in out and "language" in out and "differs" in out, True)
        check("check: the promotion is names-only",
              "Pl/second.c" in out and "names-only" in out, True)
        rc, out = ps.captured(lambda: pb.check_batch(ctx, manifest, json_out=True))
        data = json.loads(out)
        check("check: --json is serialisable", data["batch"], "batch")
        by_unit = {r["new_unit"]: r for r in data["results"]}
        check("check: the language verdict", by_unit["auto/80001000_fn_80001000.cpp"]["verdict"],
              "differs")
        check("check: the promotion verdict", by_unit["Pl/second.c"]["verdict"], "names-only")

        # a promotion whose bytes moved is a bug in the move - the batch must fail
        ps.write_elf(ctx.object_dir / "Pl" / "second.o", text=b"\x22" * 16, syms=[
            ("", 0, 0, 0, 0), ("moved.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)])
        check("check: a promotion whose bytes moved fails",
              ps.silent(lambda: pb.check_batch(ctx, manifest)), 1)

        # a language change whose bytes did NOT move is a NOTE, not a failure
        ps.write_elf(ctx.object_dir / "Pl" / "second.o", syms=[
            ("", 0, 0, 0, 0), ("moved.c", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)])
        ps.write_elf(ctx.object_dir / "auto" / "80001000_fn_80001000.o",
                     syms=[("", 0, 0, 0, 0), ("same.cpp", 0, 0, 4, 0xFFF1), ("main_fn", 0, 16, 0x12, 1)])
        rc, out = ps.captured(lambda: pb.check_batch(ctx, manifest))
        check("check: an unchanged language object still passes", rc, 0)
        check("check: and it is noted as a no-op", "no-op" in out, True)

        # drop one unit from config.json only - the object bytes are untouched
        ctx.build_config.write_text(json.dumps(
            {"units": [{"name": "auto/80001000_fn_80001000.cpp"}], "modules": []}),
            encoding="utf-8")
        check("check: a promotion missing from config.json fails",
              ps.silent(lambda: pb.check_batch(ctx, manifest)), 1)

        # a missing manifest is a clear refusal
        check("check: a missing manifest is refused",
              "no manifest" in message(lambda: pb.check_batch(
                  ctx, pb.read_manifest(ctx, spec, str(tmp / "nope.json")))), True)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("%d checks, %d failed" % (checks, len(fails)))
    for f in fails:
        print("  FAIL %s" % f)
    return 1 if fails else 0


def _write(path: Path, text: str) -> Path:
    path.write_text(text, encoding="utf-8", newline="")
    return path


if __name__ == "__main__":
    sys.exit(selftest())
