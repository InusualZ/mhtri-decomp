"""Write the one file a worker is handed, `tools/units/briefs/<slug>.md` (`tools/units/briefing/`).
Spec: docs/tools/spec/brief.md. CLI: python tools/units/brief.py <unit> [--task T] [--out F] [--stdout] [--json] |
--pool [--force] [--no-prune] [--prune-promoted] | --check-promoted | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os

# the brief carries the plan's text verbatim (<=, >=, em dashes), and a Windows console is cp1252
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass

from tools.lib.lanes import naming, registry
import tools.units.briefing as briefing

_sources, _render, _pool = briefing.sources, briefing.render, briefing.pool

# the names every caller of `brief` uses (land, handoff, dossier, dataqueue, queue, sync_profiles' selftest)
SRC_EXT, BAR, RULE_DOCS = _sources.SRC_EXT, _sources.BAR, _sources.RULE_DOCS
source_name = _sources.source_name
strip_comments = _sources.strip_comments
text_has_bodies = _sources.text_has_bodies
has_bodies = _sources.has_bodies
registered_units = _sources.registered_units
registered_objects = _sources.registered_objects
pool_units = _sources.pool_units
claim_for = _sources.claim_for
claim_slug = _sources.claim_slug
handoff_paths = _sources.handoff_paths
splits_range = _sources.splits_range
registered_text_ranges = _sources.registered_text_ranges
map_rows = _sources.map_rows
symbols_in_range = _sources.symbols_in_range
report_scores = _sources.report_scores
header_comment = _sources.header_comment
flags_for = _sources.flags_for
lib_for = _sources.lib_for
shared_headers = _sources.shared_headers
plan_section = _sources.plan_section
_section_span = _sources._section_span
data_queue_entries = _sources.data_queue_entries
build = _sources.build
STAMP_MARK = _render.STAMP_MARK
_stamp_line = _render._stamp_line
unit_stamp = _render.unit_stamp
entry_stamp = _render.entry_stamp
brief_stamp = _render.brief_stamp
config_schema_lines = _render.config_schema_lines
integrator_lines = _render.integrator_lines
unclaimed_notice = _render.unclaimed_notice
render = _render.render
brief_for = _render.brief_for
cluster_index = _render.cluster_index
brief_unit = _pool.brief_unit
pool_dir = _pool.pool_dir
pool = _pool.pool
promoted_dir = _pool.promoted_dir
brief_text_range = _pool.brief_text_range
promoted_litter = _pool.promoted_litter
prune_promoted = _pool.prune_promoted


def selftest() -> int:
    import shutil
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("source_name adds the extension", source_name("Pl/pl_act"), "Pl/pl_act.cpp")
    check("source_name keeps one", source_name("main.cpp"), "main.cpp")
    check("source_name's .cpp default is unchanged without a root", source_name("RSO/runtime"), "RSO/runtime.cpp")
    check("source_name resolves the real extension from the tree", source_name("RSO/runtime", "."), "RSO/runtime.c")
    check("source_name strips an extension first (norm_unit)", source_name("Camellia/camellia.c", "."),
          "Camellia/camellia.c")
    check("splits parses a block", splits_range(".", "Pl/pl_act").get(".text") is not None
          and len(splits_range(".", "Pl/pl_act")[".text"]) == 3, True)
    check("splits finds a .c unit from either spelling",
          splits_range(".", "RSO/runtime.c") == splits_range(".", "RSO/runtime") != {}, True)
    check("splits ignores an unknown unit", splits_range(".", "Nope/nothing"), {})

    # the pool's definition: a registered unit that is not Matching and whose source exists (2026-10-04: the
    # pool used to be the body-less units only, which hid every NonMatching unit that had a body)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src", "auto"))
        open(os.path.join(tmp, "src", "auto", "stub.c"), "w").write("/* header only */\n")
        open(os.path.join(tmp, "src", "auto", "done.c"), "w").write("/* header */\nint f(void) { return 1; }\n")
        open(os.path.join(tmp, "src", "auto", "ok.c"), "w").write("int g(void) { return 2; }\n")

        def configure(ok_flag):
            open(os.path.join(tmp, "configure.py"), "w").write(
                'config.libs = [\n    {\n        "lib": "auto",\n        "objects": [\n'
                '            Object(NonMatching, "auto/stub.c"),\n'
                '            Object(NonMatching, "auto/done.c"),\n'
                '            Object(%s, "auto/ok.c"),\n'
                '            Object(NonMatching, "auto/missing.c"),\n'
                "        ],\n    },\n]\n" % ok_flag)
        configure("Matching")
        check("registered_units parses the object list", registered_units(tmp),
              ["auto/stub.c", "auto/done.c", "auto/ok.c", "auto/missing.c"])
        check("has_bodies: a header-only placeholder / a body / a missing file",
              [has_bodies(os.path.join(tmp, "src", "auto", n)) for n in ("stub.c", "done.c", "missing.c")],
              [False, True, False])
        check("the pool holds every registered NonMatching unit with a source, body or not",
              pool_units(tmp), ["auto/done", "auto/stub"])
        h = handoff_paths(tmp, "auto/stub", assume_claim=True)
        check("assume_claim marks the brief claimed with the default branch's slug",
              (h["claimed"], h["slug"]), (True, naming.slug("auto/stub")))
        check("assume_claim writes no registry", registry.load(tmp), {})
        registry.save(tmp, {"auto/stub": {"branch": "worker/" + naming.slug("auto/stub") + "-zz"}})
        check("a real claim wins over assume_claim",
              handoff_paths(tmp, "auto/stub", assume_claim=True)["slug"], naming.slug("auto/stub") + "-zz")
        registry.save(tmp, {"cluster/auto": {"branch": "worker/cluster-auto-1234", "units": ["auto/done"]}})
        check("a unit a cluster claim holds takes the cluster's handoff",
              handoff_paths(tmp, "auto/done")["slug"], "cluster-auto-1234")
        registry.save(tmp, {})
        out = pool(tmp)
        check("pool writes a brief per pool unit", out["wrote"], ["auto/done", "auto/stub"])
        brief_path = os.path.join(pool_dir(tmp), naming.slug("auto/stub") + ".md")
        check("the pooled brief is named by the unit slug and parses back",
              (os.path.exists(brief_path), brief_unit(brief_path)), (True, "auto/stub"))
        brief_text = open(brief_path, encoding="utf-8").read()
        check("the pooled brief carries the claim's outbox", "outbox" in brief_text
              and "no active claim" not in brief_text, True)
        check("the handoff is the final message", "final message" in brief_text and "subagent_done" not in brief_text,
              True)
        check("the ack line takes no herdr pane", ("--agent <your-name>" in brief_text, "--pane" in brief_text),
              (True, False))
        check("the brief states rule 7 has no exemption or deferral",
              "no exemption and no deferral" in brief_text and "`rule 7 deferred` comment exempts nothing" in brief_text,
              True)
        check("the brief tells the worker to run the gate's naming rule locally",
              "land.rule7_defer_growth" in brief_text and "land.band_ownership_warnings" in brief_text, True)
        check("the brief bans claims.py release with its consequence",
              "NEVER run `claims.py release`" in brief_text and "WIPED the directory" in brief_text, True)
        check("the brief ships the per-lane measurer and the whole-tree diff",
              "tools/units/measure.py" in brief_text and "--against-main" in brief_text
              and "ninja changes" in brief_text and "never wave it through" in brief_text, True)
        check("the brief carries the order-only report.json trap and the one-report scorer",
              "order-only target of `all_source`" in brief_text and "rm -f build/RMHE08/report.json" in brief_text
              and "tools/objdiff/unitscore.py" in brief_text and "--force-stale" in brief_text, True)
        check("the brief names the asm dump and the build's objdump",
              "python tools/splits/dump_asm.py" in brief_text and "build/binutils/powerpc-eabi-objdump.exe" in brief_text,
              True)
        check("the brief carries the git add hygiene and the selftest runner",
              "`git add -A` with no path arguments" in brief_text and "tools/selftest.py --changed" in brief_text, True)
        check("the brief carries the your-tree block, naming the worktree and the self-check",
              "## 0 · Your tree, then acknowledge" in brief_text
              and naming.worktree_for("auto/stub", tmp).replace("\\", "/") in brief_text
              and "git rev-parse --show-toplevel" in brief_text and "STOP and report it" in brief_text, True)
        again = pool(tmp)
        check("pool is idempotent (an unchanged stamp is not rewritten)", (again["skipped"], again["refreshed"]),
              (["auto/done", "auto/stub"], []))
        check("... and its stamp equals the entry's", brief_stamp(brief_path), entry_stamp(tmp, "auto/stub"))
        configure("NonMatching")
        configure_out = pool(tmp)
        check("a unit that leaves Matching joins the pool", configure_out["wrote"], ["auto/ok"])
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [{"lib": "auto", "objects": [Object(NonMatching, "auto/stub.c"), '
            'Object(Matching, "auto/done.c"), Object(NonMatching, "auto/ok.c")]}]\n')
        check("pool prunes a unit that became Matching", [p["unit"] for p in pool(tmp)["pruned"]], ["auto/done"])

    # a promoted brief outlives its claim: one whose unit's split range moved is litter, and is deleted only
    # when no live claim owns it
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src"))
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [{"lib": "g", "objects": [Object(NonMatching, "g/moved.cpp")]}]\n')
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "g/moved.cpp:\n\t.text       start:0x80100000 end:0x80100400\n")
        os.makedirs(promoted_dir(tmp))
        litter_path = os.path.join(promoted_dir(tmp), "moved-1234.md")
        open(litter_path, "w").write("# Brief: g/moved\n\n| sections | .text 0x80100000-0x80100200 |\n")
        ok_path = os.path.join(promoted_dir(tmp), "ok.md")
        open(ok_path, "w").write("# Brief: g/moved\n\n| sections | .text 0x80100000-0x80100400 |\n")
        gone_path = os.path.join(promoted_dir(tmp), "gone.md")
        open(gone_path, "w").write("# Brief: g/gone\n\n| sections | .text 0x80900000-0x80900100 |\n")
        litter = promoted_litter(tmp)
        check("a promoted brief whose split range moved, or that nothing carries, is litter",
              sorted(r["slug"] for r in litter), ["gone", "moved-1234"])
        check("... with the range it states and the current one",
              [(r["stated"], r["expected"]) for r in litter if r["slug"] == "moved-1234"],
              [([0x80100000, 0x80100200], [0x80100000, 0x80100400])])
        check("a matching promoted brief is not litter", all(r["slug"] != "ok" for r in litter), True)
        registry.save(tmp, {"g/gone": {"branch": naming.branch_for("g/gone")}})
        held = prune_promoted(tmp)
        check("only the claim-free litter is pruned", [r["slug"] for r in held], ["moved-1234"])
        check("... the claimed one survives, the matching one too", (os.path.exists(gone_path), os.path.exists(ok_path)),
              (True, True))

    # the brief's own schema table is what a worker follows, so an outbox shaped by it must validate clean
    handoff_mod = _sources._handoff()
    table = "\n".join(config_schema_lines())
    check("the brief names every config kind", all(k in table for k in handoff_mod.CONFIG_KINDS), True)
    check("the brief names the required fields",
          all(f in table for row in handoff_mod.config_schema_rows() for f in row["needs"]), True)
    check("the brief names every flags_probed field", all(f in table for f in handoff_mod.FLAG_PROBE_FIELDS), True)
    from tools.lib import requests as _requests
    pilot = "\n".join(integrator_lines())
    check("the brief names every integrator request kind", [k for k in _requests.KINDS if "`%s`" % k not in pilot], [])
    check("the brief shows the STOPGAP marker integrate.py deletes",
          "STOPGAP-BEGIN(<id>)" in pilot and "STOPGAP-END(<id>)" in pilot, True)
    brief_shaped = {"unit": "Pl/pl_act", "worker": "a", "finished_at": "2026-01-01T00:00:00",
                    "unit_percent": 50.0, "symbols": [{"name": "fn_1", "percent": 50.0}],
                    "residual": "none", "measured_with": "recompile.py", "blockers": [],
                    "config_requests": [], "flags_probed": []}
    for row in handoff_mod.config_schema_rows():
        brief_shaped["config_requests"].append(dict({"kind": row["kind"]}, **{f: "<%s>" % f for f in row["needs"]}))
    brief_shaped["flags_probed"].append({"flags": "<flags>", "effect": "<symbol: before -> after>", "verdict": "reject"})
    check("a brief-shaped outbox validates clean", handoff_mod.validate(brief_shaped, {"fn_1"})[0], [])
    check("plan_section finds §6.5 and its goto rule",
          ("Type and naming discipline" in plan_section(".", "### 6.5 Type and naming discipline"),
           "`goto` is forbidden" in plan_section(".", "### 6.5 Type and naming discipline")), (True, True))
    check("plan_section finds §8 and is empty for nonsense",
          ("Invariants" in plan_section(".", "## 8. Invariants"), plan_section(".", "### 99 nope")), (True, ""))
    moved = tempfile.mkdtemp(prefix="brief_sec_")
    os.makedirs(os.path.join(moved, "docs"), exist_ok=True)
    open(os.path.join(moved, "docs", "plan.md"), "w", encoding="utf-8").write(
        "# plan\n\n## 5. The protocol\n\nMoved to pipeline.md 10.\n\n## 6. Next\n\nafter\n")
    open(os.path.join(moved, "docs", "pipeline.md"), "w", encoding="utf-8").write(
        "# pipeline\n\n## 10. The protocol\n\n### 10.6 A worker does not fan out subagents\n\nbody-10-6\n\n"
        "### 10.7 Acknowledgement, heartbeats and timeouts\n\nbody-10-7\n")
    sec = plan_section(moved, "### 5.5 A worker does not fan out subagents")
    check("plan_section follows a section that moved, stopping at its own next heading",
          ("body-10-6" in sec, "body-10-7" in sec), (True, False))
    shutil.rmtree(moved, ignore_errors=True)

    # the slug is the claim's branch minus worker/, because that is what land.py's gate keys the outbox by
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        registry.save(main, {
            "Pl/pl_act": {"branch": naming.branch_for("Pl/pl_act"), "worktree": os.path.join(tmp, "ws"),
                          "base": "0" * 40},
            "Pl/pl_skill": {"branch": naming.branch_for("Pl/pl_skill") + "-dd6e"}})
        check("an unclaimed unit has no claim, and no branch means no slug",
              (claim_for(main, "RSO/runtime"), claim_slug({})), ({}, None))
        check("the slug is the branch minus worker/", claim_slug(claim_for(main, "Pl/pl_act")), naming.slug("Pl/pl_act"))
        check("claim_slug is the registry's one rule", claim_slug(claim_for(main, "Pl/pl_skill")),
              registry.claim_slug(main, "Pl/pl_skill"))
        h = handoff_paths(main, "Pl/pl_act")
        check("the outbox and notes are the registry's own paths",
              (h["claimed"], h["outbox"], h["notes"]),
              (True, registry.outbox_path(main, "Pl/pl_act"), registry.notes_path(main, "Pl/pl_act")))
        check("the outbox is the one handoff.py names", os.path.basename(h["outbox"]),
              os.path.basename(handoff_mod.outbox_path(main, "Pl/pl_act")))
        check("a branch suffix survives into the slug", handoff_paths(main, "Pl/pl_skill")["slug"],
              naming.slug("Pl/pl_skill") + "-dd6e")
        check("the ack and rescue ref stay keyed by the unit", (h["ack"], h["rescue"]),
              (registry.ack_path(main, "Pl/pl_act"), "refs/rescue/%s" % naming.slug("Pl/pl_act")))
        u = handoff_paths(main, "RSO/runtime")
        check("an unclaimed unit has no slug and no outbox, and says so",
              (u["slug"], u["outbox"], u["claimed"], "no active claim" in unclaimed_notice(main, "RSO/runtime")),
              (None, None, False, True))

    # the brief names the shared headers the unit should reuse instead of re-creating (typeregistry)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "include", "nw4r"))
        os.makedirs(os.path.join(tmp, "src", "auto"))
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [{"lib": "auto", "objects": [Object(NonMatching, "auto/copies.c")]}]')
        open(os.path.join(tmp, "include", "ef.h"), "w").write(
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\n#define EF_ASSERT_PTR(p) do { } while (0)\n")
        open(os.path.join(tmp, "include", "nw4r", "math.h"), "w").write(
            "namespace nw4r { namespace math { struct VEC3 { f32 x; f32 y; f32 z; }; } }\n")
        open(os.path.join(tmp, "src", "auto", "copies.c"), "w").write(
            "typedef struct Vec { f32 x; f32 y; f32 z; } Vec;\nvoid f(Vec* v) { EF_ASSERT_PTR(v); }\n")
        open(os.path.join(tmp, "src", "auto", "plain.c"), "w").write("void g(void) { }\n")
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/copies.c:\n\t.text start:0x80000000 end:0x80000004\n")
        open(os.path.join(tmp, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_1 = .text:0x80000000; // type:func size:0x4\n"
            "make__FPQ34nw4r4math4VEC3 = .text:0x80000002; // type:func size:0x2\n")
        _sources.clear_caches()
        b = build(tmp, tmp, "auto/copies", None)
        check("build carries the shared-header advice", [h["header"] for h in b["shared_headers"]],
              ["include/ef.h", "include/nw4r/math.h"])
        check("the copying header is marked duplicated, the mangled symbol names the second",
              (b["shared_headers"][0]["duplicated"], b["shared_headers"][1]["used"]), (["Vec"], ["VEC3"]))
        text = render(tmp, b, None)
        check("the brief prints the block, both headers and the duplication",
              ("Shared headers this unit should reuse" in text, "`include/ef.h`" in text,
               "`include/nw4r/math.h`" in text, "you define these too" in text), (True, True, True, True))
        check("an unreadable target leaves the language unrecorded, and the brief says so",
              (b["language"]["lang"], "| language | not on record" in text, "language is not on record" in text),
              (None, True, True))
        plain = build(tmp, tmp, "auto/plain", None)
        check("a unit with no match still carries the rule",
              (plain["shared_headers"], "No shared header declares anything" in render(tmp, plain, None)), ([], True))

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").split("\n")[0])
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--task", default=None, help="override part 5 with your own task text")
    ap.add_argument("--out", default=None)
    ap.add_argument("--stdout", action="store_true")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--pool", action="store_true",
                    help="write a brief for every registered unit that is not Matching into tools/units/briefs/pool/")
    ap.add_argument("--force", action="store_true", help="with --pool, rewrite briefs that already exist")
    ap.add_argument("--no-prune", action="store_true", help="with --pool, keep briefs whose unit left the pool")
    ap.add_argument("--prune-promoted", action="store_true",
                    help="with --pool, delete promoted briefs in tools/units/briefs/ that no live claim owns"
                         " and whose range no longer matches")
    ap.add_argument("--check-promoted", action="store_true",
                    help="report promoted briefs whose range no longer matches, then exit (read-only)")
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    main_wt = registry.main_of()
    if args.check_promoted:
        litter = promoted_litter(main_wt)
        for row in litter:
            print("%s  %s" % ("HELD  " if row["claimed"] else "LITTER", row["slug"]))
            print("    unit   %s" % row["unit"])
            print("    stated %s" % ("0x%08X..0x%08X" % tuple(row["stated"])))
            print("    now    %s" % ("0x%08X..0x%08X" % tuple(row["expected"]) if row["expected"] else "(not registered)"))
            print("    reason %s" % row["reason"])
        if not litter:
            print("ok - no promoted brief disagrees with the registered ranges")
            return 0
        print("\n%d promoted brief(s) disagree with the registered ranges (HELD = a live claim owns them,"
              " do not delete)" % len(litter))
        return 1
    if args.pool:
        out = pool(main_wt, force=args.force, prune=not args.no_prune, prune_promoted_litter=args.prune_promoted)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        print("pool %s" % out["dir"])
        print("  %d registered unit(s) to hand out" % len(out["units"]))
        print("  wrote    %d" % len(out["wrote"]))
        for unit in out["wrote"]:
            print("      + %s" % unit)
        print("  skipped  %d  (up to date)" % len(out["skipped"]))
        print("  refreshed %d  (entry changed since the brief was written)" % len(out["refreshed"]))
        for unit in out["refreshed"]:
            print("      ~ %s" % unit)
        print("  pruned   %d  (unit became Matching, or is no longer registered)" % len(out["pruned"]))
        for row in out["pruned"]:
            print("      - %s  (%s)" % (row["unit"] or "?", row["slug"]))
        litter = out.get("litter") or []
        print("  promoted %d  brief(s) disagree with the registered ranges" % len(litter))
        for row in litter:
            print("      %s %s  (%s)" % ("HOLD" if row["claimed"] else "LITTER", row["slug"], row["reason"]))
        for row in out.get("pruned_promoted") or []:
            print("      - pruned %s" % row["slug"])
        return 0
    if not args.unit:
        ap.print_help()
        return 0

    wt = registry.toplevel_of()
    b, text = brief_for(main_wt, wt, args.unit, args.task)
    if args.json:
        print(json.dumps({k: v for k, v in b.items() if k != "header"}, indent=2))
        return 0
    if args.stdout:
        print(text)
        return 0
    if not b["slug"]:
        print("WARNING: %s has no active claim in %s - the brief says so and offers no outbox path; the file "
              "name falls back to the registry slug" % (args.unit, registry.registry_path(main_wt)), file=sys.stderr)
    out = args.out or os.path.join(main_wt, "tools", "units", "briefs", (b["slug"] or naming.slug(args.unit)) + ".md")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    print("wrote %s (%d lines, %d symbols, %d below the bar)" % (out, text.count("\n"), len(b["symbols"]), b["below_bar"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
