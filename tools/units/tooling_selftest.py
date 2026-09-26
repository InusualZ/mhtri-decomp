#!/usr/bin/env python3
"""Deterministic self-test for tools/units/tooling.py.

    python tools/units/tooling_selftest.py
    python tools/units/tooling.py --selftest

No build, no `ninja` and no repository state: every outbox and note is a fixture written into a temp
directory, so the contract is pinned - a structured outbox key and a prose `## Tooling and environment`
section both yield suggestions; two workers phrasing the same wall differently become **one** entry with a
count of 2; two novel phrasings of one request are clustered by similarity; a duplicate outbox from one
worker does not double-vote; the ranking is votes-descending then cost-descending; and a `**Status.**` line
survives regeneration (the register's only hand-edited field).
"""
from __future__ import annotations

import json
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import tooling as tg  # noqa: E402


def outbox(unit, worker, **kw):
    d = {"unit": unit, "worker": worker, "finished_at": "2026-01-01T00:00:00", "unit_percent": 100.0,
         "symbols": [], "residual": "none"}
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

    # --- pure helpers ------------------------------------------------------------------------------
    check("tokens drop stopwords", "worktree" in tg.tokens("a worktree with the orig/ payload"), True)
    check("tokens drop addresses", tg.tokens("fn_80073398 0x80073398"), set())
    check("cost minutes", tg.cost_of("the setup cost 10 minutes")[0], 10.0)
    check("cost seconds", round(tg.cost_of("about 90 seconds")[0], 2), 1.5)
    check("cost measurements", tg.cost_of("58 measurements, then 13762 objects")[0], 29.0)
    check("cost absent", tg.cost_of("it was awkward")[0], 0.0)
    check("slug", tg.slug("Ship the `build/tmp/*.py` measurer!"), "ship-the-build-tmp-py-measurer")
    check("one_line strips a bullet", tg.one_line("* **bold** idea\nsecond line"), "**bold** idea second line")
    check("novel register is tracked, not .pi", ".pi" not in tg.REGISTER and tg.REGISTER.endswith("tooling-requests.md"),
          True)
    check("jaccard disjoint", tg.jaccard(tg.tokens("alpha beta"), tg.tokens("gamma delta")), 0.0)

    # --- fixtures ----------------------------------------------------------------------------------
    tmp = tempfile.mkdtemp(prefix="tooling-selftest-")
    obx = os.path.join(tmp, "outbox")
    notes = os.path.join(tmp, "notes")
    os.makedirs(obx)
    os.makedirs(notes)

    seed_a = ("## Tooling and environment\n\n* The worktree needed `orig/RMHE08/**` and `build/compilers` "
              "copied from MAIN; without the DOL nothing splits. Cost: 10 minutes.\n")
    seed_b = ("## Tooling and environment\n\n* A fresh worktree has no `orig/` and no `build/compilers`; "
              "they must be copied from MAIN first (2-3 min).\n")
    fixtures = {
        "a.json": outbox("auto/A", "w1", notes=seed_a),
        "b.json": outbox("auto/B", "w2", notes=seed_b),
        "c.json": outbox("auto/A", "w1", notes=seed_a),  # same worker: one vote, not two
        "d.json": outbox("auto/D", "w3", tooling={"request": "`recompile.py --measure` cannot run for a "
                        "proposal unit: MAIN has no ninja rule and no target object, so measurement needs "
                        "`--main <worktree>`."}),
    }
    for name, d in fixtures.items():
        with open(os.path.join(obx, name), "w", encoding="utf-8", newline="\n") as fh:
            json.dump(d, fh)

    notes_files = {
        "n1.md": "# n1\n\n## Environment note\n\n* The worktree has no `orig/RMHE08` binary, so "
                 "`orig/RMHE08/{sys,files,disc}` and `build/compilers` are junctions to MAIN.\n",
        "n2.md": "# n2\n\n## Tooling note\n\n* `build/tmp/m.py` is the scratch measurer (gitignored): it "
                 "compiles the unit and scores every symbol with one `objdiff report generate`. "
                 "Cost: 58 measurements.\n",
        "n3.md": "# n3\n\n## Tooling and environment\n\n* Teach `m2c` the paired-single `psq_l`/`psq_st` "
                 "saves; today it renders them as `xxsel`/`vmrghb` garbage.\n",
        "n4.md": "# n4\n\n## Tooling and environment\n\n* The claims tool should not reclaim a worktree "
                 "while a long ninja build is still running; it cost a lost build.\n",
        "n5.md": "# n5\n\n## Tooling note\n\n* `claims.py` reclaims the worktree during a long ninja build; "
                 "the in-flight build was lost, twice.\n",
    }
    for name, text in notes_files.items():
        with open(os.path.join(notes, name), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)

    # --- extraction + topic clustering -------------------------------------------------------------
    sources = tg.load_sources(obx, notes)
    check("every outbox and note is a source", len(sources), len(fixtures) + len(notes_files))
    voters = {s.path: s.voter for s in sources}
    check("a duplicate outbox keeps the worker identity", voters["a.json"], voters["c.json"])

    statuses = {}
    text, report, entries = tg.scan(obx, notes, statuses)
    by_key = {e.key: e for e in entries}

    check("the seed wall is one entry from three voters", by_key["seed-worktree"].votes, 3)
    check("a differently-phrased wall is the same entry",
          sorted(by_key["seed-worktree"].voters), ["n1", "w1", "w2"])
    check("the structured outbox key is read", by_key["recompile-worktree-target"].votes, 1)
    check("the scratch measurer is one entry", by_key["scratch-measurer"].votes, 1)
    check("the m2c ask is one entry", by_key["m2c-paired-single"].votes, 1)

    # two novel phrasings of one request cluster into one entry with two votes
    novel = [e for e in entries if e.votes == 2 and e.key.startswith("novel-")]
    check("two novel phrasings cluster into one 2-vote entry", len(novel), 1)
    check_true("the novel ask is about the claims/worktree wall",
               "reclaim" in novel[0].ask.lower() or "worktree" in novel[0].ask.lower())

    # --- ranking -----------------------------------------------------------------------------------
    order = [e.key for e in entries]
    check("most-asked first", order[0], "seed-worktree")
    check("votes are descending", [e.votes for e in entries], sorted((e.votes for e in entries), reverse=True))
    one_vote = [e for e in entries if e.votes == 1]
    check_true("cost breaks the 1-vote tie (measured before unmeasured)",
               one_vote[0].cost >= one_vote[-1].cost and one_vote[0].cost > 0)
    check("only real requests are listed", len(entries), 6)

    # --- rendering + status persistence ------------------------------------------------------------
    check_true("the table has a status column", "| # | request | votes | cost | status |" in text)
    check_true("evidence names the workers", "`w1`" in text and "`n1`" in text)
    check_true("cost shows on the seed entry", "10 min" in text)
    check("status round-trips", tg.parse_statuses(text), {e.key: "open" for e in entries})
    text_done, _, _ = tg.scan(obx, notes, {"seed-worktree": "done"})
    check("a status is applied", tg.parse_statuses(text_done)["seed-worktree"], "done")
    text_again, _, _ = tg.scan(obx, notes, tg.parse_statuses(text_done))
    check("a status survives regeneration", tg.parse_statuses(text_again)["seed-worktree"], "done")
    check("rendering is deterministic", tg.scan(obx, notes, statuses)[0], text)

    # --- JSON --------------------------------------------------------------------------------------
    payload = tg.entries_to_json(entries, 4, 5, len(sources))
    check("json has every entry", len(payload["entries"]), len(entries))
    check("json is ranked", [e["rank"] for e in payload["entries"]], list(range(1, len(entries) + 1)))
    check_true("json carries evidence and status",
               all(e["status"] in tg.STATUSES and e["evidence"] for e in payload["entries"]))
    check("json round-trips", json.loads(json.dumps(payload))["requests"], len(entries))

    # --- check semantics: a new report makes the register stale ------------------------------------
    stale_note = os.path.join(notes, "z-stale.md")
    with open(stale_note, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("# z\n\n## Tooling and environment\n\n* The splitter should seed `orig/RMHE08/**` and "
                 "`build/compilers` automatically; it cost 20 minutes.\n")
    text_stale, _, _ = tg.scan(obx, notes, statuses)
    check_true("a new report changes the register", text_stale != text)
    os.remove(stale_note)
    check("removing it restores the register", tg.scan(obx, notes, statuses)[0], text)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
