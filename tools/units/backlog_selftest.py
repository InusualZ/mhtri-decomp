#!/usr/bin/env python3
"""Deterministic self-test for tools/units/backlog.py.

    python tools/units/backlog_selftest.py
    python tools/units/backlog.py --selftest

No repository state and no build: every outbox is a fixture written into a temp directory, so the contract
is pinned - the `rename` kind is never carried; a repeated `shared-file` defect from two lanes becomes **one**
item with a filer count of 2; a record ("Added one union member ...") defaults to `done` while a defect
("... illegal function overloading") defaults to `open`; one entry carrying two defects in one header becomes
two items; a `range` admission is done while a re-draw is open; a `flag` whose change is "none" is done; a
status survives regeneration; and `refusal()` names the top item while an empty register does not refuse.
"""
from __future__ import annotations

import json
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if os.path.join(ROOT, "tools") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools"))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import backlog as bl  # noqa: E402


def outbox(unit, worker, when, requests):
    return {"unit": unit, "worker": worker, "finished_at": when, "unit_percent": 100.0,
            "symbols": [], "residual": "none", "config_requests": requests}


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
    check("norm_file strips (NEW) and lowercases",
          bl.norm_file("include/enemy/fn_801251D0.h (NEW)"), "include/enemy/fn_801251d0.h")
    check("norm_file folds an unnamed shared header",
          bl.norm_file("include/ (a new shared header, e.g. include/nw4r/math.h + include/game/_PLW.h)"),
          "include/")
    check("clauses splits an enumerated two-problem entry",
          len(bl.clauses("two problems in the header. (1) line 23 opens a file-wide `#pragma peephole off` "
                         "that leaks. (2) line 336 declares `extern \"C\" void fn_80041E8C(f32*)`.")), 2)
    check("clauses leaves a single defect alone",
          len(bl.clauses("line 67 declares X while Y declares Z; both fail with MWCC.")), 1)
    check("a past-tense record is done",
          bl.classify_shared("Added one union member to the +0x328 union for this unit's parts."), "done")
    check("a removal record is done",
          bl.classify_shared("Removed the `s32 em_act_ck(...)` declaration from enemy.h; a note stands in "
                             "its place."), "done")
    check("the 12-renames record is done",
          bl.classify_shared("the 12 renames below (each map row moved to its owner via symedit.py)"), "done")
    check("a conflicting declaration is open",
          bl.classify_shared("line 67 declares `void fn_80128A8C(...)` while fn_801251D0.h:41 declares "
                             "`fn_80128A8C(...)`; both sit inside extern \"C\", so any TU that includes both "
                             "fails with MWCC (10197) illegal function overloading."), "open")
    check("a leaking pragma is open",
          bl.classify_shared("line 23 opens a file-wide `#pragma peephole off` that leaks into every TU "
                             "including it"), "open")
    check("a wrong-arity defect is open",
          bl.classify_shared("it should take one argument: all three target call sites pass one"), "open")
    check("a seam-unproven range is an admission (done)",
          bl.range_flavour("SEAM UNPROVEN - registered whole as the brief proposed."), ("admission", "done"))
    check("a genuine range re-draw is open",
          bl.range_flavour("Requested, not claimed: the 0x150-byte record table fn_8004991C walks."),
          ("redraw", "open"))
    check("a flag whose change is none is not a request", bl.flag_is_request("none - the pragma is per file"),
          False)
    check("a real flag change is a request", bl.flag_is_request("-opt nopeephole"), True)

    # --- fixtures: two lanes, one repeated defect, one record, a multi-defect header -----------------
    tmp = tempfile.mkdtemp(prefix="backlog-selftest-")
    obx = os.path.join(tmp, "outbox")
    notes = os.path.join(tmp, "notes")
    os.makedirs(obx)
    conflict_a = ("line 67 declares `void fn_80128A8C(struct _ENEMY_WORK* self, u8 a, u8 b)` while "
                  "include/enemy/fn_801251D0.h:41 declares `fn_80128A8C(struct _ENEMY_WORK*, u32, u32)`; "
                  "both sit inside extern \"C\", so any TU that includes both fails with MWCC (10197) "
                  "illegal function overloading.")
    conflict_b = ("`fn_80128A8C` is declared twice with incompatible argument lists (u8/u8 vs u32/u32); "
                  "MWCC rejects a TU that sees both - illegal function overloading.")
    fixtures = {
        "a.json": outbox("enemy/em019_prog", "w1", "2026-09-20T10:00:00", [
            {"kind": "shared-file", "file": "include/enemy/fn_801251D0.h (NEW)", "why": conflict_a},
            {"kind": "shared-file", "file": "include/enemy/enemy_work.h",
             "why": "added `_ENEMY_PART` (6-byte stride) plus `_ENEMY_WORK::amount_0x7A0` for this unit."},
            {"kind": "range", "section": ".text", "start": "0x80059550", "end": "0x8005AA28",
             "evidence": "SEAM UNPROVEN - registered whole as the brief proposed. Nothing else pins it."},
            {"kind": "range", "section": ".data", "start": "0x80582D38", "end": "0x80582EE0",
             "evidence": "The unit's own face/skin load table; read only by this unit. Requested, not claimed."},
            {"kind": "flag", "lib": "enemy", "change": "-opt nopeephole", "evidence": "measured 97.6 %"},
            {"kind": "rename", "old": "fn_80128A8C", "new": "em_parts_damage_level_get",
             "evidence": "the jumptable dispatch"},
            {"kind": "done-in-this-fold", "file": "include/unsplit/ef.h", "why": "folded into the owner"},
        ]),
        "b.json": outbox("enemy/fn_8012E968", "w2", "2026-09-25T10:00:00", [
            {"kind": "shared-file", "file": "include/enemy/fn_801251D0.h", "why": conflict_b},
            {"kind": "shared-file", "file": "include/stage/fn_802B2AA0.h",
             "why": "two problems in the header this band takes shell_set_func_ptr from. (1) line 23 opens a "
                    "file-wide `#pragma peephole off` that leaks into every TU including it. (2) line 336 "
                    "declares `extern \"C\" void fn_80041E8C(f32* out, f32 x, f32 y, f32 z)` where the "
                    "target's call site consumes the returned pointer."},
            {"kind": "flag", "lib": "enemy", "change": "none - the pragma is per file",
             "evidence": "no lib change needed"},
        ]),
    }
    for name, d in fixtures.items():
        with open(os.path.join(obx, name), "w", encoding="utf-8", newline="\n") as fh:
            json.dump(d, fh)

    items = bl.build_items(obx, notes, os.path.join(tmp, "no-tooling-register.md"), {})
    by = {}
    for it in items:
        by.setdefault(it.kind, []).append(it)

    check("the rename kind is never carried", any(i.kind == "rename" for i in items), False)
    check("nine distinct items from ten requests (attribute the rest by kind)",
          len(items), 9)
    open_ = [i for i in items if i.status == "open"]
    done = [i for i in items if i.status == "done"]
    check("five live items", len(open_), 5)
    check("four done items", len(done), 4)

    # the repeated shared-file defect collapses to one item with two filers
    conflict = [i for i in items if i.kind == "shared-file" and "fn_80128a8c" in i.defect]
    check("the repeated defect is one item", len(conflict), 1)
    check("... filed by two lanes", conflict[0].filer_count, 2)
    check("... and names both lanes", sorted(conflict[0].filers), ["w1", "w2"])
    check("... and is open", conflict[0].status, "open")

    # one entry carrying two defects in one header is two items
    two = [i for i in items if i.target == "include/stage/fn_802b2aa0.h"]
    check("a two-problem header yields two items", len(two), 2)
    check("... one is the pragma leak",
          sorted(i.defect for i in two), ["arity:fn_80041e8c", "pragma"])

    # the record is done, the admission is done, the open data run is open
    record = [i for i in items if i.target == "include/enemy/enemy_work.h"]
    check("a past-tense record defaults to done", (len(record), record[0].status), (1, "done"))
    adm = [i for i in items if i.flavour == "admission"]
    check("the seam-unproven admission is done", (len(adm), adm[0].status), (1, "done"))
    redraw = [i for i in items if i.flavour == "redraw"]
    check("the genuine range re-draw is open", (len(redraw), redraw[0].status), (1, "open"))
    check("a flag whose change is none is done",
          [(i.kind, i.status) for i in items if i.kind == "flag" and i.defect.startswith("none")],
          [("flag", "done")])
    check("a done-in-this-fold entry is done",
          [i.status for i in items if i.kind == "done-in-this-fold"], ["done"])

    # ranking: the two-filer open item leads
    check("the most-filed open item ranks first", items[0].filer_count, 2)
    check("... and it is the shared-file defect", items[0].target, "include/enemy/fn_801251d0.h")
    check("... open items precede done", [i.status for i in items[:len(open_)]], ["open"] * len(open_))
    check("the non-empty set of filers is a list", isinstance(items[0].filers, list), True)

    # --- status persistence ------------------------------------------------------------------------
    key = conflict[0].key
    check("an item's key is stable", bl.build_items(obx, notes, os.path.join(tmp, "x"), {})[0].key == key, True)
    statuses = {key: "parked"}
    parked = bl.build_items(obx, notes, os.path.join(tmp, "x"), statuses)
    check("a persisted status is applied", [i for i in parked if i.key == key][0].status, "parked")
    check("... and it leaves the open set",
          sum(1 for i in parked if i.status == "open"), len(open_) - 1)
    reg = os.path.join(tmp, ".pi", "backlog.json")
    bl.write_atomic(reg, json.dumps(bl.payload(parked, "2026-09-25T10:00:00",
                                                {"open": 4, "done": 4, "parked": 1, "total": 9})))
    check("the register round-trips on disk", bl.load_statuses(tmp, reg), {key: "parked"})

    # --- payload / JSON ----------------------------------------------------------------------------
    p = bl.payload(items, "2026-09-25T10:00:00", {"open": 5, "done": 4, "parked": 0, "total": 9})
    check("json has every item", len(p["items"]), len(items))
    check_true("json carries key/kind/status/filings",
               all({"key", "kind", "status", "filings", "filer_count"} <= set(e) for e in p["items"]))
    check("json records the age of an item", [e for e in p["items"] if e["filer_count"] == 2][0]["age_days"],
          5.0)
    check("json statuses only lists overrides",
          set(bl.payload(parked, "", {}).get("statuses", {})), {key})

    # --- the credit ledger: a `done` earns 1, a claim spends `ratio`, the base is 1 ---------------------
    check("a default-done record earns no credit", bl.ledger_earned(items), 0)
    check("the ledger starts at 1 credit", bl.ledger_summary(items, [])["balance"], 1)
    check("a claim spends 1 credit", bl.ledger_summary(items, [{"unit": "u1"}])["balance"], 0)
    check("... two claims overdraw it", bl.ledger_summary(items, [{"unit": "u1"}, {"unit": "u2"}])["balance"], -1)
    resolved = bl.build_items(obx, notes, os.path.join(tmp, "x"), {key: "done"})
    check("a resolved `done` earns 1 credit", bl.ledger_earned(resolved), 1)
    check("... so a claim is affordable again", bl.ledger_summary(resolved, [{"unit": "u1"}])["balance"], 1)
    check("... but `parked` earns nothing", bl.ledger_earned(
        bl.build_items(obx, notes, os.path.join(tmp, "x"), {key: "parked"})), 0)
    check("ratio 2 makes one claim cost two credits",
          bl.ledger_summary(resolved, [{"unit": "u1"}], 2)["balance"], 0)
    check("a default-done item does not count even as an override",
          bl.ledger_earned(bl.build_items(obx, notes, os.path.join(tmp, "x"),
                                          {done[0].key: "done"})), 0)
    # record_claims persists the spent side and leaves the earned side derivable from the statuses
    ledger_reg = os.path.join(tmp, ".pi", "ledger.json")
    summ = bl.record_claims(tmp, [{"unit": "u1"}], ratio=1, register=ledger_reg,
                            outbox=obx, notes=notes, tooling_register=os.path.join(tmp, "x"))
    check("record_claims persists the claim", bl.load_ledger(tmp, ledger_reg)["claims"], [{"unit": "u1"}])
    check("... and reports the spent balance", summ["balance"], 0)
    check("... and the claim can be read back", bl.load_ledger(tmp, ledger_reg)["ratio"], 1)
    p = bl.payload(items, "", {}, {"claims": [{"unit": "u1"}], "ratio": 1})
    check("the payload carries the ledger balance", p["ledger"]["balance"], 0)
    check("... and its derivation", (p["ledger"]["base"], p["ledger"]["earned"], p["ledger"]["spent"]),
          (1, 0, 1))
    # enforcement reads only the claims from the file; `earned` is always derived, so a hand-edited balance
    # cannot buy a claim, and recording a claim preserves the statuses it shares the file with.
    bogus = os.path.join(tmp, "bogus.json")
    bl.write_atomic(bogus, json.dumps({"statuses": {}, "ledger": {"claims": [], "ratio": 1,
                                                                    "balance": 9999}}))
    check("a hand-edited ledger balance is ignored (earned is derived)",
          bl.refusal(tmp, outbox=obx, notes=notes, register=bogus, wants=2) is not None, True)
    seeded = os.path.join(tmp, "seeded.json")
    bl.write_atomic(seeded, json.dumps({"statuses": {key: "done"}, "ledger": {"claims": [], "ratio": 1}}))
    bl.record_claims(tmp, [{"unit": "u9"}], ratio=1, register=seeded, outbox=obx, notes=notes,
                     tooling_register=os.path.join(tmp, "x"))
    check("recording a claim preserves the statuses", bl.load_statuses(tmp, seeded), {key: "done"})
    check("... and appends to the ledger", len(bl.load_ledger(tmp, seeded)["claims"]), 1)
    # A claim is free while the register is CLEAN (owner, 2026-09-27): the ledger rations against known
    # backlog work, so with nothing to fix there is nothing to ration - charging would let a clean stretch
    # accrue negative credit and then demand catch-up resolutions the day items reappeared.
    clean = tempfile.mkdtemp(prefix="backlog-clean-")
    os.makedirs(os.path.join(clean, "outbox"))
    clean_reg = os.path.join(clean, ".pi", "clean.json")
    check("a clean register has no open items", bl.open_items(clean), [])
    fsum = bl.record_claims(clean, [{"unit": "uF"}], ratio=1, register=clean_reg)
    check("... a claim while it is clean is free", fsum["balance"], 1)
    check("... and spends nothing", bl.load_ledger(clean, clean_reg)["claims"], [])
    check("... but is still counted, so nothing is hidden", bl.load_ledger(clean, clean_reg)["free"], 1)
    check("... and the balance line says so",
          "free while the register was clean" in bl.ledger_line(fsum), True)
    check("... and the payload carries it",
          bl.payload([], "", {}, bl.load_ledger(clean, clean_reg))["ledger"]["free"], 1)
    check("... a second free claim accumulates",
          bl.record_claims(clean, [{"unit": "uG"}], ratio=1, register=clean_reg)["free"], 2)
    check("... and the balance is untouched by either",
          bl.record_claims(clean, [], ratio=1, register=clean_reg)["balance"], 1)
    check("a claim against an open backlog still spends",
          bl.record_claims(tmp, [{"unit": "u10"}], ratio=1,
                           register=os.path.join(tmp, ".pi", "spend.json"),
                           outbox=obx, notes=notes,
                           tooling_register=os.path.join(tmp, "x"))["balance"], 0)

    # --- refusal: the balance, not a hard gate -------------------------------------------------------
    no_reg = os.path.join(tmp, "no-register.json")
    check("the starting balance covers a single claim",
          bl.refusal(tmp, outbox=obx, notes=notes, register=no_reg), None)
    msg = bl.refusal(tmp, outbox=obx, notes=notes, register=no_reg, wants=2)
    check_true("a claim the balance cannot cover is refused", msg and msg.startswith("backlog:"))
    check_true("... and shows the balance", "balance is 1" in (msg or ""))
    check_true("... names the top item's target", "include/enemy/fn_801251d0.h" in (msg or ""))
    check_true("... and carries a paste-ready lane", 'subagent(agent="fixer"' in (msg or ""))
    check_true("... and says parked earns no credit", "`parked` earns no credit" in (msg or ""))
    empty = tempfile.mkdtemp(prefix="backlog-empty-")
    os.makedirs(os.path.join(empty, "outbox"))
    check("an empty register does not refuse", bl.refusal(empty), None)
    check("... even when the balance is spent (nothing to work)",
          bl.refusal(empty, wants=99), None)
    check("an empty register has no open items", bl.open_items(empty), [])

    # --- triage: evidence-based classification, and never a guess ------------------------------------
    tdir = tempfile.mkdtemp(prefix="backlog-triage-")
    tobx = os.path.join(tdir, "outbox")
    tnotes = os.path.join(tdir, "notes")
    os.makedirs(tobx)
    os.makedirs(os.path.join(tdir, "config", "RMHE08"))
    os.makedirs(os.path.join(tdir, "include"))
    open(os.path.join(tdir, "configure.py"), "w", encoding="utf-8").write(
        'cflags_base = ["-O4,p", "-inline auto"]\n'
        'cflags_test = [*cflags_base, "-opt nopeephole"]\n'
        'config.libs = [\n'
        '    {"lib": "test", "cflags": cflags_test, "objects": []},\n'
        '    {"lib": "plain", "cflags": cflags_base, "objects": []},\n'
        ']\n')
    open(os.path.join(tdir, "config", "RMHE08", "splits.txt"), "w", encoding="utf-8").write(
        "g3d/covered.cpp:\n\t.text       start:0x80100000 end:0x80100100\n")
    open(os.path.join(tdir, "include", "pragma_off.h"), "w").write(
        "#pragma peephole off\nvoid fn_80001111(void);\n")
    open(os.path.join(tdir, "include", "pragma_gone.h"), "w").write("void fn_80002222(void);\n")
    open(os.path.join(tdir, "include", "decl_present.h"), "w").write("void fn_80003333(void);\n")
    open(os.path.join(tdir, "include", "decl_gone.h"), "w").write("void other(void);\n")
    treqs = [
        {"kind": "range", "section": ".text", "start": "0x80100000", "end": "0x80100100",
         "evidence": "Requested, not claimed: the run."},
        {"kind": "range", "section": ".data", "start": "0x80500000", "end": "0x80500100",
         "evidence": "Requested, not claimed: the table."},
        {"kind": "range", "section": ".data", "start": "0x805C02xx", "end": "0x805C53xx",
         "evidence": "Requested, not claimed: the pool."},
        {"kind": "flag", "lib": "test", "change": "-opt nopeephole", "evidence": "measured"},
        {"kind": "flag", "lib": "plain", "change": "-opt nopeephole", "evidence": "measured"},
        {"kind": "flag", "lib": "plain", "change": ["-inline", "noauto"], "evidence": "measured"},
        {"kind": "flag", "lib": "test", "change": "a per-region cflags group with `-opt level=4` (x)",
         "evidence": "x"},
        {"kind": "shared-file", "file": "include/pragma_off.h",
         "why": "line 23 opens a file-wide `#pragma peephole off` that leaks into every TU including it"},
        {"kind": "shared-file", "file": "include/pragma_gone.h",
         "why": "line 23 opens a file-wide `#pragma peephole off` that leaks into every TU including it"},
        {"kind": "shared-file", "file": "include/decl_present.h",
         "why": "the header still declares `void fn_80003333(void)` while the owner's body returns void."},
        {"kind": "shared-file", "file": "include/decl_gone.h",
         "why": "the header still declares `void fn_80004444(void)` while the owner's body returns void."},
        {"kind": "shared-file", "file": "include/gone.h",
         "why": "the header should take one argument: all call sites pass one."},
        {"kind": "shared-file", "file": "include/ (a new shared header, e.g. include/nw4r.h)",
         "why": "the whole-file `#pragma peephole off` must move to the owner band header."},
        {"kind": "tooling", "what": "teach objdiff to pair symbols by size", "why": "a request in prose"},
    ]
    with open(os.path.join(tobx, "t.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(outbox("auto/t", "wt", "2026-09-27T00:00:00", treqs), fh)
    treg = os.path.join(tdir, ".pi", "backlog.json")
    decisions, _ = bl.triage(tdir, outbox=tobx, notes=tnotes,
                             tooling_register=os.path.join(tdir, "none.md"), register=treg)

    def dec(kind, target, needle=""):
        for it, d, ev in decisions:
            if it.kind == kind and it.target == target and needle in (it.defect or it.ask or ""):
                return d, ev
        return None, None

    check("triage: a covered span is resolved", dec("range", ".text 0x80100000-0x80100100")[0], "resolved")
    check_true("... and says which unit claims it", "g3d/covered.cpp" in (dec("range", ".text 0x80100000-0x80100100")[1] or ""))
    check("triage: an uncovered span stays open", dec("range", ".data 0x80500000-0x80500100")[0], "open")
    check("triage: an unparseable span stays open (no check)",
          dec("range", ".data 0x805C02xx-0x805C53xx")[0], "open")
    check_true("... and says why", "parseable" in (dec("range", ".data 0x805C02xx-0x805C53xx")[1] or ""))
    check("triage: a flag the lib already carries is resolved",
          dec("flag", "test", "nopeephole")[0], "resolved")
    check("triage: a flag the lib lacks stays open", dec("flag", "plain", "nopeephole")[0], "open")
    check("triage: a JSON flag spec is checked, not guessed",
          dec("flag", "plain", "noauto")[0], "open")
    check("triage: a prose flag request is unprovable and stays open",
          dec("flag", "test", "per-region")[0], "open")
    check("triage: a pragma still present stays open", dec("shared-file", "include/pragma_off.h")[0], "open")
    check("triage: a pragma now gone is resolved", dec("shared-file", "include/pragma_gone.h")[0], "resolved")
    check("triage: a still-declared owned symbol stays open",
          dec("shared-file", "include/decl_present.h")[0], "open")
    check("triage: a no-longer-named symbol is resolved",
          dec("shared-file", "include/decl_gone.h")[0], "resolved")
    check("triage: a missing file is stale", dec("shared-file", "include/gone.h")[0], "stale")
    check("triage: a prose target is unprovable and stays open",
          dec("shared-file", "include/")[0], "open")
    check("triage: a tooling request has no artifact and stays open",
          dec("tooling", "teach objdiff to pair symbols by size")[0], "open")
    rep = bl.triage_report(decisions)
    check("triage reports resolved/stale/open by kind",
          (rep["counts"]["resolved"], rep["counts"]["stale"], rep["counts"]["open"]), (4, 1, 9))
    check("... with a per-kind breakdown", rep["by_kind"]["range"]["resolved"], 1)

    # --apply: resolved -> done, stale -> parked; idempotent; a human's status is never flipped
    out1 = bl.apply_triage(tdir, decisions, register=treg, outbox=tobx, notes=tnotes,
                           tooling_register=os.path.join(tdir, "none.md"))
    check("triage --apply marks resolved done and stale parked",
          (out1["changed"]["done"], out1["changed"]["parked"]), (4, 1))
    check("... and leaves the rest open", out1["changed"]["open"], 9)
    check("... the register now reads 9 open / 4 done / 1 parked",
          (out1["counts"]["open"], out1["counts"]["done"], out1["counts"]["parked"]), (9, 4, 1))
    check("... and the four resolutions earn 4 credits", out1["summary"]["earned"], 4)
    decisions2, _ = bl.triage(tdir, outbox=tobx, notes=tnotes,
                              tooling_register=os.path.join(tdir, "none.md"), register=treg)
    out2 = bl.apply_triage(tdir, decisions2, register=treg, outbox=tobx, notes=tnotes,
                           tooling_register=os.path.join(tdir, "none.md"))
    check("triage --apply is idempotent (a second run changes nothing)",
          (out2["changed"]["done"], out2["changed"]["parked"]), (0, 0))
    check("... and reclassifies only the survivors", len(decisions2), 9)
    # a human's non-default status is respected: seed one resolved item as `parked` by hand
    hkey = [it.key for it, d, _ in decisions if d == "resolved"][0]
    bl.write_atomic(treg, json.dumps({"version": 1, "statuses": {hkey: "parked"},
                                      "ledger": {"claims": [], "ratio": 1}}))
    items_h, _ = bl.build(tdir, outbox=tobx, notes=tnotes,
                          tooling_register=os.path.join(tdir, "none.md"), register=treg)
    check("a hand-set status wins over the default", [i for i in items_h if i.key == hkey][0].status, "parked")

    # --- the third source: stylelint's findings, aggregated per file -------------------------------
    check("lint_counts: rule 7 aggregates under `naming`",
          bl.lint_counts([{"rule": 7, "file": "src/a.c", "line": 1}]), {("naming", "src/a.c"): 1})
    check("lint_counts: rule 7 stays one item per file",
          bl.lint_counts([{"rule": 7, "file": "src/a.c", "line": 1},
                          {"rule": 7, "file": "src/b.c", "line": 1},
                          {"rule": 7, "file": "src/a.c", "line": 2}]),
          {("naming", "src/a.c"): 2, ("naming", "src/b.c"): 1})
    check("lint_counts: rule 2 aggregates under `band-header`",
          bl.lint_counts([{"rule": 2, "file": "src/c.c", "line": 3}]), {("band-header", "src/c.c"): 1})
    check("lint_counts: the other rules are not backlog",
          bl.lint_counts([{"rule": r, "file": "src/a.c", "line": 1} for r in (1, 3, 4, 5, 6, 8, 9, 10)]), {})
    check("the lint item kinds are declared once", sorted(bl.LINT_KINDS), ["band-header", "naming"])
    check("the lint item rules are fixed", bl.LINT_RULES, {"naming": 7, "band-header": 2})
    check("a lint item keeps the new kind and a stable defect (the key survives a partial fix)",
          [(i.kind, i.target, i.defect) for i in bl.build_items(
              obx, notes, os.path.join(tmp, "x"), {},
              lint_items=[bl.Item(kind="naming", target="src/a.c", defect="rule 7", status="open",
                                  default_status="open", ask="a", votes=3)]) if i.kind == "naming"],
          [("naming", "src/a.c", "rule 7")])

    # a real tree: one rule-7 file and one rule-2 file
    ldir = tempfile.mkdtemp(prefix="backlog-lint-")
    for d in ("src/mod", "config/RMHE08", ".pi/outbox", ".pi/notes"):
        os.makedirs(os.path.join(ldir, d))
    with open(os.path.join(ldir, "configure.py"), "w", encoding="utf-8") as fh:
        fh.write("config.libs = [\n]\n")
    with open(os.path.join(ldir, "src", "mod", "a.c"), "w", encoding="utf-8") as fh:
        fh.write("void fn_80040598(void) {}\nvoid fn_80040599(void) {}\n")
    with open(os.path.join(ldir, "src", "mod", "b.c"), "w", encoding="utf-8") as fh:
        fh.write("extern void foo(void);\nvoid b(void) { foo(); }\n")
    with open(os.path.join(ldir, "config", "RMHE08", "symbols.txt"), "w", encoding="utf-8") as fh:
        fh.write("fn_80040598 = .text:0x1000; // type:function\n"
                 "fn_80040599 = .text:0x1010; // type:function\n"
                 "foo = .text:0x1800; // type:function\n")
    with open(os.path.join(ldir, "config", "RMHE08", "splits.txt"), "w", encoding="utf-8") as fh:
        fh.write("mod/a.c:\n\t.text       start:0x1000 end:0x2000\n")
    lit = {i.kind: i for i in bl.collect_lint_items(ldir)}
    check("the lint source files one item per (file, rule)",
          sorted((i.kind, i.target) for i in bl.collect_lint_items(ldir)),
          [("band-header", "src/mod/b.c"), ("naming", "src/mod/a.c")])
    check("the naming item carries its rule-7 count as its weight", lit["naming"].weight, 2)
    check("the naming ask names the file, the rule and the count",
          ("src/mod/a.c" in lit["naming"].ask and "rule-7" in lit["naming"].ask
           and "2" in lit["naming"].ask), True)
    check("the band-header item carries its rule-2 count", lit["band-header"].weight, 1)
    check("the band-header ask names the owner-header fix",
          ("rule-2" in lit["band-header"].ask and "include" in lit["band-header"].ask), True)
    check("the high-traffic file leads the register (most findings first)",
          [i.kind for i in bl.build_items(bl.outbox_dir(ldir), bl.notes_dir(ldir),
                                          os.path.join(ldir, "none.md"), {},
                                          lint_items=bl.collect_lint_items(ldir))][0], "naming")
    dated = bl.Item(kind="shared-file", target="src/mod/c.c", defect="d", status="open",
                    default_status="open", ask="a",
                    filings=[bl.Filing("o", "w", "2026-09-27T00:00:00", "d")])
    check("a lint item surfaces above a dated item of the same filer count",
          bl.rank([dated, lit["naming"]])[0].kind, "naming")
    nkey = lit["naming"].key
    check("a naming item's key is stable across runs",
          [i.key for i in bl.collect_lint_items(ldir) if i.kind == "naming"], [nkey])
    check("the key is the (kind, file, rule) identity", nkey.startswith("naming-"), True)
    check("a set-status override applies to a lint item",
          [i.status for i in bl.build_items(bl.outbox_dir(ldir), bl.notes_dir(ldir),
                                            os.path.join(ldir, "none.md"), {nkey: "parked"},
                                            lint_items=bl.collect_lint_items(ldir)) if i.key == nkey],
          ["parked"])
    # carry-forward + triage: fix the file, the item survives and triage proves it done
    lreg = os.path.join(ldir, ".pi", "backlog.json")
    bl.write_register(ldir, bl.collect_lint_items(ldir), "",
                      {"open": 2, "done": 0, "parked": 0, "total": 2}, {"claims": [], "ratio": 1}, lreg)
    with open(os.path.join(ldir, "src", "mod", "a.c"), "w", encoding="utf-8") as fh:
        fh.write("void named(void) {}\n")
    check("a fixed file's item is carried forward, not silently dropped",
          nkey in {i.key for i in bl.collect_lint_items(ldir, lreg)}, True)
    check("... and its live count is now zero",
          [i.weight for i in bl.collect_lint_items(ldir, lreg) if i.key == nkey], [0])
    decd, dece = bl._check_lint(ldir, lit["naming"], {"splits": bl._splits_ranges(ldir)})
    check("triage: the naming item is resolved once rule 7 no longer fires", decd, "resolved")
    check_true("... and says it re-linted the file", "re-linted" in (dece or ""))
    band, bande = bl._check_lint(ldir, lit["band-header"], {"splits": bl._splits_ranges(ldir)})
    check("triage: an unfixed band-header item stays open", band, "open")
    check_true("... naming the live count", "rule-2" in (bande or ""))
    decisions3, _ = bl.triage(ldir, outbox=bl.outbox_dir(ldir), notes=bl.notes_dir(ldir),
                              tooling_register=os.path.join(ldir, "none.md"), register=lreg)
    check("triage classifies the carried naming item as resolved",
          [d for it, d, _ in decisions3 if it.key == nkey], ["resolved"])
    out3 = bl.apply_triage(ldir, decisions3, register=lreg, outbox=bl.outbox_dir(ldir),
                           notes=bl.notes_dir(ldir), tooling_register=os.path.join(ldir, "none.md"))
    check("triage --apply marks the naming item done", out3["changed"]["done"], 1)
    check("... and the naming resolution earns a credit", out3["summary"]["earned"], 1)
    check("a missing file makes the lint item stale",
          bl._check_lint(ldir, bl.Item(kind="naming", target="src/mod/gone.c", defect="rule 7",
                                       status="open", default_status="open", ask="x"),
                         {"splits": {}})[0], "stale")
    check("a tree with no src/ contributes no lint items", bl.collect_lint_items(tmp), [])

    # --- --check semantics -------------------------------------------------------------------------
    import contextlib
    import io
    reg2 = os.path.join(tmp, ".pi", "check.json")

    def run(*args):
        argv = sys.argv
        sys.argv = ["backlog.py", "--main", tmp, "--outbox", obx, "--notes", notes,
                    "--register", reg2, "--tooling-register", os.path.join(tmp, "none.md"), *args]
        try:
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                return bl.main()
        finally:
            sys.argv = argv

    check("--check exits 1 when the register is missing", run("--check"), 1)
    check("a plain run writes the register", run(), 0)
    check("--check exits 0 when the register is current", run("--check"), 0)
    with open(os.path.join(obx, "c.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(outbox("auto/z", "w3", "2026-09-26T10:00:00", [
            {"kind": "shared-file", "file": "include/other.h",
             "why": "the header should take one argument: all call sites pass one."}]), fh)
    check("--check exits 1 when a source changed", run("--check"), 1)
    os.remove(os.path.join(obx, "c.json"))

    # --- lane task ---------------------------------------------------------------------------------
    lane = bl.lane_task(tmp, items[0])
    check("a shared-file item gets a fixer lane", lane["agent"], "fixer")
    check_true("the lane's call is one line and a subagent call",
               "\n" not in lane["call"] and lane["call"].startswith("subagent(agent=\"fixer\""))
    check_true("the lane's task names the key and how to close it",
               key in lane["task"] and "--set-status" in lane["task"])
    check("a tooling item gets a worker lane",
          bl.lane_task(tmp, bl.Item(kind="tooling", target="x", defect="", status="open",
                                    default_status="open", ask="x", votes=1))["agent"], "worker")
    check("a range item gets a decompiler lane",
          bl.lane_task(tmp, bl.Item(kind="range", target="x", defect="redraw", status="open",
                                    default_status="open", ask="x"))["agent"], "decompiler")
    check("a naming item gets a fixer lane",
          bl.lane_task(tmp, bl.Item(kind="naming", target="src/a.c", defect="rule 7", status="open",
                                    default_status="open", ask="x"))["agent"], "fixer")
    check("a band-header item gets a fixer lane",
          bl.lane_task(tmp, bl.Item(kind="band-header", target="src/a.c", defect="rule 2", status="open",
                                    default_status="open", ask="x"))["agent"], "fixer")

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(selftest())
