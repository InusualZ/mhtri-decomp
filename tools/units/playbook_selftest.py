#!/usr/bin/env python3
"""Deterministic self-test for tools/units/playbook.py.

    python tools/units/playbook_selftest.py
    python tools/units/playbook.py --selftest

No build, no `ninja` and no repository state: every outbox, note, CLAUDE.md and matching.md is a fixture
written into a temp directory, so the contract is pinned - a finding is classified against the registry, five
identical adopt probes become one group, an already-landed idea is skipped by number, a finding with no
numbers is refused, and `run()` writes the drafts without touching `docs/**` or `CLAUDE.md`.
"""
from __future__ import annotations

import hashlib
import json
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import playbook as pb  # noqa: E402

AGENTS = """\
# CLAUDE.md

# Matching playbook - index

| # | idea | status | tags | problem |
| --- | --- | --- | --- | --- |
| 35 | [A dead copy chain steers the allocator's web priority](035-dead-copy-chain.md) | works | allocator | Two webs sharing one register pair look unreachable from the source. |
| 38 | [An `s16` parameter with a compound assignment makes a narrow field store raw](038-s16.md) | works | | A masked `stb`/`sth` means MWCC is narrowing the value to the field. |
"""

MATCHING_IDEAS = {
    "035-dead-copy-chain.md": "---\nid: 35\ntitle: A dead copy chain steers the allocator's web priority\n---\n\n# 35.\n",
    "038-s16.md": "---\nid: 38\ntitle: An `s16` parameter with a compound assignment is what makes a field store raw\n"
                  "---\n\n# 38.\n",
}


def outbox(unit, worker, **kw):
    d = {"unit": unit, "worker": worker, "finished_at": "2026-01-01T00:00:00", "unit_percent": 100.0,
         "symbols": [], "residual": "none", "measured_with": "recompile.py"}
    d.update(kw)
    return d


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def check_true(name, got):
        check(name, bool(got), True)

    rows = pb.existing_rows(AGENTS, os.path.join(tempfile.mkdtemp(prefix="pb-none-"), "absent"))

    # --- pure helpers ------------------------------------------------------------------------------
    check("tokens strip backticks/stopwords", "s16" in pb.tokens("An `s16` parameter with a compound"), True)
    check("next_number is max+1", pb.next_number(rows), 39)
    check("landed_in finds the s16 row",
          pb.landed_in("An `s16` parameter with a compound assignment makes a narrow field store raw", rows), 38)
    check("landed_in finds the dead-copy row",
          pb.landed_in("A dead copy chain steers the allocator's web priority", rows), 35)
    check("landed_in does not flag a new lever",
          pb.landed_in("A unit whose retail code keeps unfused peephole folds needs the peephole pass off", rows),
          None)
    check("jaccard of disjoint is 0", pb.jaccard(pb.tokens("alpha beta"), pb.tokens("gamma delta")), 0.0)

    check("peephole pragma -> peephole-off",
          pb.registry_match("#pragma peephole off (file-scoped) fn_A 71.25 -> 100.0", ("lever",))["key"],
          "peephole-off")
    check("nopeephole flag -> peephole-off",
          pb.registry_match("-opt nopeephole", ("lever",))["key"], "peephole-off")
    check("fp_contract -> fp-contract-off",
          pb.registry_match("#pragma fp_contract off", ("lever",))["key"], "fp-contract-off")
    check("optimization_level 1 -> the trap",
          pb.registry_match("#pragma optimization_level 1 (file-scoped) no change at all; peephole still folds",
                            ("trap",))["key"], "opt-level-not-peephole")
    check("extern C -> extern-c-mangling",
          pb.registry_match('extern "C" on the fn_* definitions', ("lever",))["key"], "extern-c-mangling")
    check("optimization_level 4 is not registered",
          pb.registry_match("#pragma optimization_level 4", ("lever", "trap")), None)
    check("has_numbers before/after", pb.has_numbers("98.90 -> 100"), True)
    check("has_numbers percent", pb.has_numbers("unit 99.9613 %"), True)
    check("has_numbers hex size", pb.has_numbers(".text 0x26C"), True)
    check("has_numbers qualitative", pb.has_numbers("no change at all"), False)

    # --- fixtures ----------------------------------------------------------------------------------
    tmp = tempfile.mkdtemp(prefix="playbook-selftest-")
    obx = os.path.join(tmp, "outbox")
    notes = os.path.join(tmp, "notes")
    drafts = os.path.join(tmp, "drafts")
    os.makedirs(obx)
    os.makedirs(notes)
    agents_p = os.path.join(tmp, "CLAUDE.md")
    matching_p = os.path.join(tmp, "matching")
    os.makedirs(matching_p)
    for _n, _t in MATCHING_IDEAS.items():
        open(os.path.join(matching_p, _n), "w", encoding="utf-8").write(_t)
    open(agents_p, "w", encoding="utf-8").write(AGENTS)

    fixtures = {
        "a.json": outbox("auto/A", "w1",
                         flags_probed=[
                             {"flags": "#pragma peephole off (file-scoped)",
                              "effect": "fn_A 71.25 -> 100.0", "verdict": "adopt"},
                             {"flags": "#pragma optimization_level 1",
                              "effect": "no change at all: the command line's peephole still folds",
                              "verdict": "reject"},
                             {"flags": "a brand new lever", "effect": "fn_X 50.0 -> 60.0", "verdict": "adopt"},
                             {"flags": "cflags_main as committed (-O3, peephole on)",
                              "effect": "fn_A 100.000 % (the baseline)", "verdict": "adopt"},
                         ],
                         playbook_candidate={
                             "title": "An `s16` parameter with a compound assignment makes a narrow field store raw",
                             "problem": "masked store", "result": "A044 82.2 -> 100",
                             "measured_by": "w1"}),
        "b.json": outbox("auto/B", "w2",
                         flags_probed=[
                             {"flags": "#pragma peephole off", "effect": "fn_B 95.8 -> 100.0", "verdict": "adopt"},
                             {"flags": "#pragma fp_contract off", "effect": "fn_C 79.12 -> 100.0",
                              "verdict": "adopt"},
                             {"flags": 'extern "C" on the fn_* definitions',
                              "effect": "every fn_* symbol: 0 -> 100", "verdict": "adopt"},
                         ]),
        "c.json": outbox("auto/C", "w3",
                         flags_probed=[{"flags": "#pragma peephole off",
                                        "effect": "fn_D 94.63 -> 97.79", "verdict": "adopt"}]),
        "d.json": outbox("auto/D", "w4",
                         config_requests=[
                             {"kind": "shared-file", "file": "docs/matching.md",
                              "why": "needs #pragma peephole off: fn_E 82.56 -> 100"},
                             {"kind": "shared-file", "file": "CLAUDE.md",
                              "why": "the table row for the same idea (a unit needing the peephole pass off)"},
                             {"kind": "flag", "lib": "rso", "change": "-str reuse,pool",
                              "evidence": "fn_E 84.0 -> 90.0; readonly .rodata 0xDA / .data 0x38 -> .data 0x112"},
                         ]),
        "e.json": outbox("auto/E", "w5",
                         flags_probed=[{"flags": "#pragma optimization_level 1",
                                        "effect": "not tried - it does not turn the peephole off",
                                        "verdict": "inconclusive"}],
                         residual="a hand-written definition is possible but MWCC on this host turns the "
                                  "`\\n` in the literals into CRLF, so the bytes do not match the DOL's LF"),
        "f.json": outbox("auto/F", "w6",
                         playbook_candidate={"title": "A completely new idea with no numbers",
                                             "problem": "it improved", "result": "it got better"}),
        "g.json": outbox("Pl/pl_master", "w7",
                         suggested_playbook_row="A dead copy chain steers the allocator's web priority - when a "
                                                "residual is two webs sharing one register pair; 98.85 -> 100 %"),
    }
    for name, d in fixtures.items():
        with open(os.path.join(obx, name), "w", encoding="utf-8", newline="\n") as fh:
            json.dump(d, fh)

    open(os.path.join(notes, "ctors-rule.md"), "w", encoding="utf-8").write(
        "# The `.ctors`/`.dtors` flip blocker\n\nA flipped unit's `.ctors$10` word is reordered by the linker,\n"
        "and `flipcheck.py` cannot see it.\n")

    # --- extraction --------------------------------------------------------------------------------
    entries = pb.load_outboxes(obx)
    check("all outboxes load", len(entries), len(fixtures))
    findings, undraft = pb.extract_findings(entries)
    notes_findings = pb.extract_note_findings(notes)
    findings += notes_findings
    by_key = {}
    for f in findings:
        by_key.setdefault(f.key, []).append(f)
    check("peephole group has 5 findings (3 probes + the two shared-file requests)",
          len(by_key["peephole-off"]), 5)
    check("fp_contract group", len(by_key["fp-contract-off"]), 1)
    check("the opt-level trap has 2 findings", len(by_key["opt-level-not-peephole"]), 2)
    check("extern C", len(by_key["extern-c-mangling"]), 1)
    check("str reuse pool", len(by_key["str-reuse-pool"]), 1)
    check("crlf trap from the residual", len(by_key["crlf-literals"]), 1)
    check("ctors trap from the note", len(by_key["ctors-fragment-flip"]), 1)
    check("an unregistered adopt probe is reported, not drafted",
          any(u["text"] == "a brand new lever" for u in undraft), True)
    check("a committed-baseline adopt probe is not a finding",
          any(u["text"].startswith("cflags_main as committed") for u in undraft), False)

    # --- grouping ----------------------------------------------------------------------------------
    ready, already, notdraft = pb.group_findings(findings, rows)
    ready_keys = {g.key for g in ready}
    check("peephole is ready", "peephole-off" in ready_keys, True)
    check("fp_contract is ready", "fp-contract-off" in ready_keys, True)
    check("the opt-level trap is ready", "opt-level-not-peephole" in ready_keys, True)
    check("crlf trap is ready", "crlf-literals" in ready_keys, True)
    check("ctors trap is ready", "ctors-fragment-flip" in ready_keys, True)
    check("the s16 candidate is already landed", [a["row"] for a in already], [38, 35])
    check("the no-numbers candidate is refused",
          any(n["reason"] == "no-numbers" for n in notdraft), True)
    peep = next(g for g in ready if g.key == "peephole-off")
    check("five findings merged into one row", len(peep.findings), 5)
    check("four duplicates merged", len(peep.findings) - 1, 4)

    # --- rendering ---------------------------------------------------------------------------------
    sec = pb.render_section(peep, 39)
    for needle in ("id: 39", "# 39.", "tags: [", "**Problem.**", "**Why try it.**", "**Result.**", "**Example.**",
                   "auto/A", "auto/B", "auto/C", "auto/D"):
        check_true("section contains %s" % needle, needle in sec)
    row = pb.render_agents_row(peep, 39)
    check("the Result has one bullet per unit, not per finding", sec.count("* `auto/"), 4)
    check_true("agents row has the number", row.startswith("| 39 |"))
    check_true("agents row has a status", row.endswith("| done |"))
    check("section is deterministic", pb.render_section(peep, 39), sec)

    # --- end-to-end run ----------------------------------------------------------------------------
    h_before = hashlib.sha1(open(agents_p, "rb").read()).hexdigest()
    m_before = sorted((f, hashlib.sha1(open(os.path.join(matching_p, f), "rb").read()).hexdigest())
                      for f in os.listdir(matching_p))
    report = pb.run(obx, notes, drafts, matching_p, agents_p, write=True)
    check("next row is 39", report["next_row"], 39)
    check("rows drafted", report["rows_drafted"], len(ready))
    check("duplicates merged", report["duplicates_merged"], sum(len(g.findings) - 1 for g in ready))
    check("docs/matching.md untouched",
          sorted((f, hashlib.sha1(open(os.path.join(matching_p, f), "rb").read()).hexdigest())
                 for f in os.listdir(matching_p)), m_before)
    check("CLAUDE.md untouched",
          hashlib.sha1(open(agents_p, "rb").read()).hexdigest(), h_before)
    files = set(os.listdir(drafts))
    check_true("README written", "README.md" in files)
    check_true("agents index written", "agents-index-rows.md" in files)
    check_true("findings json written", "findings.json" in files)
    check_true("peephole draft written", any(f.startswith("matching-row-39-peephole-off") for f in files))
    check_true("fp_contract draft written", any(f.startswith("matching-row-40-fp-contract-off") for f in files))
    agents_rows = open(os.path.join(drafts, "agents-index-rows.md"), encoding="utf-8").read()
    check_true("agents index has row 39", "| 39 |" in agents_rows)
    readme = open(os.path.join(drafts, "README.md"), encoding="utf-8").read()
    check_true("README names the already-landed rows", "row 38" in readme and "row 35" in readme)
    check_true("README reports the no-registry probe", "a brand new lever" in readme)

    # a re-run clears a draft whose row no longer groups
    stale = os.path.join(drafts, "matching-row-99-stale.md")
    open(stale, "w", encoding="utf-8").write("stale\n")
    pb.run(obx, notes, drafts, matching_p, agents_p, write=True)
    check("a stale draft is removed on re-run", os.path.exists(stale), False)

    # determinism across two runs
    files = set(os.listdir(drafts))
    snap = {f: hashlib.sha1(open(os.path.join(drafts, f), "rb").read()).hexdigest() for f in sorted(files)}
    pb.run(obx, notes, drafts, matching_p, agents_p, write=True)
    snap2 = {f: hashlib.sha1(open(os.path.join(drafts, f), "rb").read()).hexdigest()
             for f in sorted(os.listdir(drafts))}
    check("a second run is byte-identical", snap2, snap)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
