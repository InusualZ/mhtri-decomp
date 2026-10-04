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

One more, and it is the reason this file was touched (2026-09-28): a **structured `tooling` list** is one
row per bullet, keyed by the target it names - `.init` filed two bullets and the register carried one, so
neither bullet may be agglomerated with the other, the identical bullet from a second lane is a second
*vote*, and a bullet that was skipped (too short, or already owned by a curated topic) is reported.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json
import os
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from tools.units import tooling as tg


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
    # `target_of` keys a structured bullet on its FIRST tool: bullets naming the same tool pool their votes
    # (keying on the first two produced `...-datagap-py-flipcheck-py` *and* `...-flipcheck-py` for bullets
    # naming flipcheck.py, so the row the register exists to build was split in two)
    check("target_of keys a bullet on its first tool",
          tg.target_of("`datagap.py` / `flipcheck.py` do not know about the .init rows"), "datagap.py")
    check("... and the same for a single-tool bullet",
          tg.target_of("`flipcheck.py` reports READY for a unit whose flip cannot link"), "flipcheck.py")
    check("... so two bullets naming that tool share their key",
          tg.target_of("`flipcheck.py` reports READY") == tg.target_of("flipcheck.py mislabels the claim"),
          True)
    check("... while a bullet naming no tool falls back to its leading words",
          tg.target_of("build/RMHE08/asm is stale relative to symbols.txt"),
          "build/RMHE08/asm is stale relative")
    check("an explicit `target` field wins over the bullet",
          tg.target_of("`a.py` and `b.py`", {"target": "objalign.py"}), "objalign.py")

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

    # --- structured `tooling` LISTS: one row per bullet, keyed by target ---------------------------
    # A lane that files two bullets used to get one row (or none): the bullets were flattened into the
    # novel pool and agglomerated by token overlap with everything else.  `.init` (2026-09-28) filed two
    # and the register carried one - so every element is now its own item, keyed by the target it names.
    obx2 = os.path.join(tmp, "outbox2")
    os.makedirs(obx2)
    bullet_1 = ("datagap.py / flipcheck.py do not know about the linker's own .init rows: a claim ending "
                "at the next symbol's start absorbs the linker's *fill*, so datagap reports `target-extra` "
                "and flipcheck reports NOT READY. Cost ~40 min.")
    bullet_2 = ("The same reader settles 'is this range a TU's or the linker's?' in one command: "
                "mwlink_debugger.py trace <unit> names the input file per symbol. Worth one line in the "
                "rule-12 write-up.")
    bullet_3 = ("callees.py cannot resolve a callee declared only in an unsplit band header: it reports "
                "the edge as external. Cost: 5 minutes.")
    bullet_short = "mwcc too slow"
    seed_bullet = ("A fresh worktree ships with no `orig/` payload and no `build/compilers`, so a split "
                   "needs them copied in from MAIN first. Cost: 10 minutes.")
    fixtures2 = {
        "init.json": outbox("init/section", "w-init", tooling=[bullet_1, bullet_2]),
        "again.json": outbox("init/again", "w-two", tooling=[bullet_2]),
        "flip.json": outbox("res/file", "w-flip", tools_wanted=[bullet_3, bullet_short, seed_bullet]),
    }
    for name, d in fixtures2.items():
        with open(os.path.join(obx2, name), "w", encoding="utf-8", newline="\n") as fh:
            json.dump(d, fh)
    _text2, report2, entries2 = tg.scan(obx2, notes, {})
    by_key2 = {e.key: e for e in entries2}
    check("two bullets in one `tooling` list become two DISTINCT rows",
          sorted(k for k in by_key2 if k.startswith("tooling-")),
          ["tooling-callees-py", "tooling-datagap-py", "tooling-mwlink-debugger-py"])
    check("... the key names the FIRST tool the bullet is about",
          any(k == "tooling-datagap-py" for k in by_key2), True)
    check("... and the slot a second mention would have taken is not a second row",
          all("flipcheck" not in k for k in by_key2)
          and "flipcheck.py" in by_key2["tooling-datagap-py"].ask, True)
    check("... the identical bullet from a second lane is a VOTE, not a row",
          by_key2["tooling-mwlink-debugger-py"].votes, 2)
    check("a `tools_wanted` list is ingested the same way", "tooling-callees-py" in by_key2, True)
    check("a bullet a curated topic already owns is FOLDED, and reported",
          sorted(s["why"] for s in report2["skipped_tooling"]),
          ["folded into a curated topic row", "too short to be a request"])
    check("... the too-short bullet is named",
          any("mwcc too slow" in s["text"] for s in report2["skipped_tooling"]), True)
    check_true("the JSON carries the skipped bullets", "skipped_tooling" in report2)

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
