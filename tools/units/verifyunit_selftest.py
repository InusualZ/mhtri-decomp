#!/usr/bin/env python3
"""Self-test for tools/units/verifyunit.py - the independent per-symbol verifier and registration gate.

    python tools/units/verifyunit_selftest.py

Each check is pinned against a fixture that must **refuse** (or, for the happy path, must pass), so the
failure the check exists for is reproducible rather than described:

* a unit registered in name only (a source with no `Object(...)` line, no `splits.txt` block, and no
  object target in the build graph) must refuse - and one registered on all three axes must pass;
* a per-symbol score that is not reproducible from a fresh `report generate`, and a 100 % claim whose
  bytes are not identical, must refuse;
* a function with no `fuzzy_match_percent` key must be read as 0 %, and the unit arithmetic that proves
  it must refuse when the two readings disagree;
* a split target object that moved for a unit the batch does not name must refuse (a neighbour the
  `splits.txt` change re-ranged), while the batch's own unit may move;
* a row whose name is dtk's own (`pad_*`, for a range with no function prologue) resolves to our
  symbol at the same section and offset, passes when the bytes match, and still refuses when they do
  not - the two directions the `worker/trk-init-vectors-2226` refusal turned on.

Three layers need `objdiff-cli` (and a pair of objects) and are skipped, not failed, without it: the
live cross-check of a registered unit, the doctored-report fixture that must refuse, and the
`pad_*`-named row's end-to-end re-measure. The `pad_*` fixtures' *objects* are synthesized in this
file - the shape is dtk's own, so no branch's build tree can supply it.
"""
from __future__ import annotations

import json
import os
import shutil
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
if os.path.dirname(HERE) not in sys.path:
    sys.path.insert(0, os.path.dirname(HERE))

import verifyunit as vu  # noqa: E402


def _ok(label, got, want, failures):
    if got == want:
        print(f"ok    {label}")
        return failures
    print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures + 1


def name_rows() -> int:
    """The three spellings of one unit collapse to one key, and objects/report names derive from it."""
    failures = 0
    failures = _ok("a source path becomes a stem", vu.unit_stem("src/hud/fn_80334568.cpp"),
                   "hud/fn_80334568", failures)
    failures = _ok("the report spelling becomes the same stem", vu.unit_stem("main/hud/fn_80334568"),
                   "hud/fn_80334568", failures)
    failures = _ok("a build object becomes the same stem",
                   vu.unit_stem("build/RMHE08/src/hud/fn_80334568.o"), "hud/fn_80334568", failures)
    failures = _ok("a .c unit keeps its stem", vu.unit_stem("Camellia/camellia.c"), "Camellia/camellia",
                   failures)
    failures = _ok("the candidate object path", vu.src_object_rel("hud/fn_80334568"),
                   "build/RMHE08/src/hud/fn_80334568.o".replace("/", os.sep), failures)
    failures = _ok("the target object path", vu.target_object_rel("hud/fn_80334568"),
                   "build/RMHE08/obj/hud/fn_80334568.o".replace("/", os.sep), failures)
    failures = _ok("the report names it main/<stem>", vu.report_unit_name("hud/fn_80334568"),
                   "main/hud/fn_80334568", failures)
    return failures


def registration_rows() -> int:
    """A unit is in the build only when configure.py, splits.txt and the graph all carry it."""
    failures = 0
    unit = "hud/fn_80334568"
    conf = 'config.libs = [\n    Object(NonMatching, "hud/fn_80334568.cpp"),\n]\n'
    spl = "Sections:\nhud/fn_80334568.cpp:\n\t\t.text start:0x80334568 end:0x80338808\n"
    ninja = "build build\\RMHE08\\src\\hud\\fn_80334568.o: mwcc_sjis\n  cflags = ...\n"
    failures = _ok("a fully registered unit has no problem",
                   vu.registration_problems([unit], conf, spl, ninja), [], failures)
    # the incident: the source exists, the registration does not
    source_only = "build build\\RMHE08\\main.elf: link build\\RMHE08\\src\\main.o\n"
    problems = vu.registration_problems([unit], "config.libs = []\n", "Sections:\n", source_only)
    failures = _ok("a unit registered in name only refuses", problems != [], True, failures)
    failures = _ok("... and all three axes are named", len(problems), 3, failures)
    failures = _ok("... the configure.py axis", any("Object(" in p for p in problems), True, failures)
    failures = _ok("... the splits.txt axis", any("splits.txt" in p for p in problems), True, failures)
    failures = _ok("... the build-graph axis", any("build graph" in p for p in problems), True, failures)
    # a half-registration: the Object line is there but the split is not
    failures = _ok("an Object line without a splits block still refuses",
                   vu.registration_problems([unit], conf, "Sections:\n", ninja) != [], True, failures)
    # the Object line and the block are there but configure.py was never re-run
    failures = _ok("a unit missing from a stale build.ninja still refuses",
                   vu.registration_problems([unit], conf, spl, source_only) != [], True, failures)
    failures = _ok("no units means no work", vu.registration_problems([], conf, spl, ninja), [],
                   failures)
    # target extraction normalises separators and ignores phony/order-only tokens
    targets = vu.build_ninja_targets("build a\\b.o: rule\nbuild c.o d.o: rule | e.o\n")
    failures = _ok("backslash targets are normalised", "a/b.o" in targets, True, failures)
    failures = _ok("several outputs on one line are all read", {"c.o", "d.o"} <= targets, True,
                   failures)
    failures = _ok("order-only inputs are not targets", "e.o" in targets, False, failures)
    return failures


def registration_check_rows() -> int:
    """`registration_check` reads a tree (not just texts) and refuses the source-only fixture."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = []\n")
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write("Sections:\n")
        open(os.path.join(tmp, "build.ninja"), "w").write("")
        ok, _detail = vu.registration_check(tmp, ["hud/fn_80334568"])
        failures = _ok("registration_check refuses a source-only unit", ok, False, failures)
        # now register it on all three axes
        open(os.path.join(tmp, "configure.py"), "w").write(
            'Object(NonMatching, "hud/fn_80334568.cpp")\n')
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "Sections:\nhud/fn_80334568.cpp:\n\t\t.text start:0x1 end:0x2\n")
        open(os.path.join(tmp, "build.ninja"), "w").write(
            "build build\\RMHE08\\src\\hud\\fn_80334568.o: mwcc_sjis\n")
        ok, detail = vu.registration_check(tmp, ["hud/fn_80334568"])
        failures = _ok("registration_check passes the fully registered unit", ok, True, failures)
        failures = _ok("... and says so", "registered" in detail, True, failures)
    return failures


def drift_rows() -> int:
    """A target object that moved for a non-batch unit refuses; the batch's own unit may move."""
    failures = 0
    before = {"ours": "aaaa", "neighbour": "bbbb", "gone": "cccc"}
    after = {"ours": "XXXX", "neighbour": "YYYY", "new": "dddd"}
    problems = vu.target_drift_problems(before, after, ["ours"])
    failures = _ok("the batch's own unit may change", any(p.startswith("ours") for p in problems),
                   False, failures)
    failures = _ok("a re-ranged neighbour refuses", any(p.startswith("neighbour") for p in problems),
                   True, failures)
    failures = _ok("... and the message says it was re-ranged",
                   any("re-ranged a neighbour" in p for p in problems), True, failures)
    failures = _ok("a target object that disappeared refuses",
                   any("disappeared" in p for p in problems), True, failures)
    failures = _ok("a target object that appeared for a non-batch unit refuses",
                   any(p.startswith("new") for p in problems), True, failures)
    failures = _ok("an unchanged tree has no drift",
                   vu.target_drift_problems(before, before, []), [], failures)
    failures = _ok("a newly registered batch unit may appear",
                   vu.target_drift_problems({"a": None}, {"a": "hash"}, ["a"]), [], failures)
    return failures


def snapshot_rows() -> int:
    """`target_object_snapshot` scopes to splits.txt units and sees a mutated object."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj"))
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "Sections:\na/a.cpp:\n\t\t.text start:0x1 end:0x2\nb/b.cpp:\n\t\t.text start:0x2 end:0x3\n")
        for rel in ("a/a.o", "b/b.o"):
            path = os.path.join(tmp, "build", "RMHE08", "obj", *rel.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as fh:
                fh.write(b"data-" + rel.encode())
        before = vu.target_object_snapshot(tmp)
        failures = _ok("every splits.txt unit is snapshotted", sorted(before), ["a/a", "b/b"], failures)
        failures = _ok("... including a unit with no object yet",
                       vu.target_object_snapshot(tmp).get("a/a") is not None, True, failures)
        with open(os.path.join(tmp, "build", "RMHE08", "obj", "a", "a.o"), "wb") as fh:
            fh.write(b"mutated")
        after = vu.target_object_snapshot(tmp)
        failures = _ok("a mutated object is seen as drift by an outsider",
                       vu.target_drift_problems(before, after, []) != [], True, failures)
        failures = _ok("... and tolerated for the unit the batch names",
                       vu.target_drift_problems(before, after, ["a/a"]), [], failures)
    return failures


def arithmetic_rows() -> int:
    """The `fuzzy_match_percent`-absent trap: absent is 0 %, and the identity must reproduce."""
    failures = 0
    functions = {"a": {"name": "a", "size": "100", "fuzzy_match_percent": 100.0},
                 "b": {"name": "b", "size": "100"}}          # b has no key
    ok, detail = vu.arithmetic_crosscheck({"total_code": 200, "fuzzy_match_percent": 50.0}, functions)
    failures = _ok("an absent key is 0%, so the unit arithmetic reproduces", ok, True, failures)
    failures = _ok("... and the check reports the computed number", "50.00000" in detail, True,
                   failures)
    # the wrong reading (absent = 100) would give 100; a report that says 100 is the liar
    ok, detail = vu.arithmetic_crosscheck({"total_code": 200, "fuzzy_match_percent": 100.0}, functions)
    failures = _ok("a unit fuzzy that only reproduces if absent=100 refuses", ok, False, failures)
    failures = _ok("... and the detail names the trap", "0%" in detail and "100%" in detail, True,
                   failures)
    failures = _ok("a subset of listed partials still reproduces",
                   vu.arithmetic_crosscheck({"total_code": 100, "fuzzy_match_percent": 100.0},
                                            {"a": {"size": "100", "fuzzy_match_percent": 100.0}})[0],
                   True, failures)
    return failures


def symbol_rows() -> int:
    """The report, the fresh report and the raw bytes are cross-checked against each other."""
    failures = 0
    raw_ok = {"a": {"target_size": 16, "candidate_size": 16, "in_target": True, "in_candidate": True,
                    "identical": True}}
    failures = _ok("a 100% symbol that is byte-identical passes",
                   vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                      {"a": {"fuzzy_match_percent": 100.0}}, raw_ok), ([], []),
                   failures)
    # report says 100, the bytes say otherwise
    raw_bad = {"a": dict(raw_ok["a"], identical=False)}
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 100.0}}, raw_bad)
    failures = _ok("a 100% claim with differing bytes refuses",
                   any("not identical" in p for p in hard), True, failures)
    raw_sized = {"a": dict(raw_ok["a"], candidate_size=32, identical=False)}
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 100.0}}, raw_sized)
    failures = _ok("a 100% claim with differing sizes refuses",
                   any("sizes differ" in p for p in hard), True, failures)
    # the fresh report disagrees with the committed one
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}}, {}, raw_ok)
    failures = _ok("a symbol the fresh report does not pair refuses",
                   any("does not pair" in p for p in hard), True, failures)
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"fuzzy_match_percent": 50.0}}, raw_ok)
    failures = _ok("a score that is not reproducible refuses",
                   any("not reproducible" in p for p in hard), True, failures)
    hard, _soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 100.0}},
                                     {"a": {"name": "a", "size": "16"}}, raw_ok)
    failures = _ok("one report scoring a symbol the other reads as 0% refuses",
                   any("reads it as 0%" in p for p in hard), True, failures)
    hard, soft = vu.symbol_problems({"a": {"fuzzy_match_percent": 50.0}},
                                    {"a": {"fuzzy_match_percent": 50.0}}, raw_ok)
    failures = _ok("identical bytes scored below 100 is an advisory, not a refusal", (hard, soft != []),
                   ([], True), failures)
    return failures


def size_gap_rows() -> int:
    """A symbol present on both sides that objdiff declines to pair is named."""
    failures = 0
    raw = {"x": {"target_size": 16, "candidate_size": 100, "in_target": True, "in_candidate": True,
                 "identical": False}}
    problems = vu.size_gap_problems({}, raw)
    failures = _ok("a >50% size gap is detected", len(problems), 1, failures)
    failures = _ok("... and says objdiff declines the pair", "declines the pair" in problems[0], True,
                   failures)
    failures = _ok("... and reads as untouched", "untouched" in problems[0], True, failures)
    failures = _ok("a similar size is not flagged",
                   vu.size_gap_problems({}, {"x": dict(raw["x"], candidate_size=20)}), [], failures)
    failures = _ok("an unwritten symbol (absent from ours) is honest 0%, not the trap",
                   vu.size_gap_problems({}, {"x": dict(raw["x"], in_candidate=False,
                                                       candidate_size=None)}), [], failures)
    failures = _ok("a paired symbol with a score is not flagged",
                   vu.size_gap_problems({"x": {"fuzzy_match_percent": 40.0}}, raw), [], failures)
    return failures


# --------------------------------------------------------------------------------------------------
# the pad-named row: resolved by ADDRESS, judged by its bytes (both directions)
# --------------------------------------------------------------------------------------------------

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB = 1, 2, 3
SHF_ALLOC, SHF_EXECINSTR = 2, 4
STB_GLOBAL, STT_OBJECT, STT_FUNC = 0, 1, 2


def _align(n: int, a: int = 4) -> int:
    return (n + a - 1) // a * a


def build_object(sections, symbols) -> bytes:
    """A minimal ELF32 big-endian object - the fixture shape `symbol_locations` reads.

    sections : [(name, data, flags)]                in shndx order
    symbols  : [(name, section, size, value, info)] the null symbol is implicit at index 0

    `raw_symbol_rows`/`symbol_locations` go through `unitutil.read_elf`, so the fixture needs exactly
    what that reads: the ELF header's `shoff`/`shentsize`/`shnum`/`shstrndx`, `>IIIIIIIIII` section
    headers, the first `SHT_SYMTAB` as the symbol table, its `link` as the string table, and
    `>IIIBBH` symbol entries.
    """
    sec_names = [""] + [n for n, _d, _f in sections] + [".symtab", ".strtab", ".shstrtab"]
    index = {n: i for i, n in enumerate(sec_names)}
    strtab = bytearray(b"\0")
    name_off = {}
    for name, _sec, _size, _val, _info in symbols:
        name_off[name] = len(strtab)
        strtab += name.encode() + b"\0"
    symtab = bytearray(struct.pack(">IIIBBH", 0, 0, 0, 0, 0, 0))
    for name, sec, size, val, info in symbols:
        symtab += struct.pack(">IIIBBH", name_off[name], val, size, info, 0, index[sec])
    shstr = bytearray(b"\0")
    sh_name = {}
    for n in sec_names:
        sh_name[n] = len(shstr)
        shstr += n.encode() + b"\0"
    data = {n: d for n, d, _f in sections}
    flags = {n: f for n, _d, f in sections}
    data[".symtab"], data[".strtab"], data[".shstrtab"] = bytes(symtab), bytes(strtab), bytes(shstr)
    placed, off = [], 52
    for n in sec_names:
        if not n:
            placed.append((n, 0, 0))
            continue
        off = _align(off)
        placed.append((n, off, len(data[n])))
        off += len(data[n])
    shoff = _align(off)
    buf = bytearray(shoff + 40 * len(sec_names))
    struct.pack_into(">4sBBBBB7s", buf, 0, b"\x7fELF", 1, 2, 1, 0, 0, b"\0" * 7)
    struct.pack_into(">HHIIIIIHHHHHH", buf, 0x10, 1, 20, 1, 0, 0, shoff, 0, 52, 0, 0, 40,
                     len(sec_names), len(sec_names) - 1)
    for n, o, size in placed:
        if n:
            buf[o:o + size] = data[n]
    for i, (n, o, size) in enumerate(placed):
        if not n:
            continue
        typ = SHT_SYMTAB if n == ".symtab" else SHT_STRTAB if n in (".strtab", ".shstrtab") \
            else SHT_PROGBITS
        struct.pack_into(">IIIIIIIIII", buf, shoff + i * 40, sh_name[n], typ, flags.get(n, 0), 0, o,
                         size, index[".strtab"] if n == ".symtab" else 0, 0, 4,
                         16 if typ == SHT_SYMTAB else 0)
    return bytes(buf)


PAD_UNIT = "Runtime.PPCEABI.H/TRK_interrupt_vectors"
PAD_BYTES = bytes(range(16))
FUNC = (STB_GLOBAL << 4) | STT_FUNC
OBJ = (STB_GLOBAL << 4) | STT_OBJECT


def _pad_objects(corrupt=False, candidate_size=16, candidate_offset=0):
    """The `worker/trk-init-vectors-2226` shape: dtk names the range, the map names the same bytes.

    The TRK interrupt vectors carry no function prologue, so `dtk dol split` names the range
    `pad_00_80004380_init` (a FUNC of the claimed size) and leaves the map's label sizeless at the same
    offset - giving the label an extent makes the split fail on the overlap, so the name cannot win.
    `corrupt` flips one byte in the CANDIDATE's `.init` only (the target stays the truth), and
    `candidate_size`/`candidate_offset` are what the lie variants move.
    """
    ours = bytearray(PAD_BYTES)
    if corrupt:
        ours[8] ^= 0xFF
    target = build_object([(".init", PAD_BYTES, SHF_ALLOC | SHF_EXECINSTR)],
                          [("pad_00_80004380_init", ".init", 16, 0, FUNC),
                           ("gTRKInterruptVectorTable", ".init", 0, 0, OBJ)])
    candidate = build_object([(".init", bytes(ours), SHF_ALLOC | SHF_EXECINSTR)],
                             [("gTRKInterruptVectorTable", ".init", candidate_size,
                               candidate_offset, OBJ),
                              # a zero-size section symbol at the same offset: never the one chosen,
                              # because it carries no bytes to compare
                              (".init", ".init", 0, 0, (STB_GLOBAL << 4) | 3)])
    return target, candidate


def _write(tmp, name, obj):
    path = os.path.join(tmp, name)
    open(path, "wb").write(obj)
    return path


def pad_name_rows() -> int:
    """A row dtk named itself is resolved by ADDRESS: byte-identical passes, a lie still refuses.

    Both directions of the 2026-09-28 refusal: the TRK `.init` claim must land, and a corrupted byte
    in that range must not.
    """
    failures = 0
    report = {"fuzzy_match_percent": 100.0, "size": "16"}      # report.json's row for the pad symbol
    fresh = {"size": "16"}                                     # what a fresh generate can pair: nothing

    with tempfile.TemporaryDirectory() as tmp:
        target, candidate = _pad_objects()
        t = _write(tmp, "target.o", target)
        c = _write(tmp, "candidate.o", candidate)
        raw = vu.raw_symbol_rows(t, c)
        pad = raw["pad_00_80004380_init"]
        failures = _ok("a pad-named row is resolved by address", pad["resolved_by"], "address", failures)
        failures = _ok("... to the symbol our object carries there", pad["candidate_name"],
                       "gTRKInterruptVectorTable", failures)
        failures = _ok("... and is present on both sides with equal sizes",
                       (pad["in_candidate"], pad["target_size"], pad["candidate_size"]),
                       (True, 16, 16), failures)
        failures = _ok("... with identical bytes", pad["identical"], True, failures)
        failures = _ok("a size-0 symbol at the address is not chosen", pad["candidate_name"] != ".init",
                       True, failures)
        # ... and with ONLY a size-0 symbol there, there is nothing to resolve to at all
        t0 = _write(tmp, "t_zero.o", build_object([(".init", PAD_BYTES, SHF_ALLOC | SHF_EXECINSTR)],
                                                  [("pad_00_80004380_init", ".init", 16, 0, FUNC)]))
        c0 = _write(tmp, "c_zero.o", build_object([(".init", PAD_BYTES, SHF_ALLOC | SHF_EXECINSTR)],
                                                  [("gTRKInterruptVectorTable", ".init", 0, 0, OBJ)]))
        zeros = vu.raw_symbol_rows(t0, c0)["pad_00_80004380_init"]
        failures = _ok("... and a range with no real symbol there does not resolve",
                       (zeros["in_candidate"], zeros["resolved_by"]), (False, None), failures)
        hard, soft = vu.symbol_problems({"pad_00_80004380_init": report},
                                        {"pad_00_80004380_init": fresh}, raw)
        failures = _ok("the pad-named row PASSES (dtk's name is not a refusal)", (hard, soft),
                       ([], []), failures)

        # direction (b): the claim is a lie - one corrupted byte in the same range
        target_c, candidate_c = _pad_objects(corrupt=True)
        raw_bad = vu.raw_symbol_rows(_write(tmp, "t_bad.o", target_c),
                                     _write(tmp, "c_bad.o", candidate_c))
        failures = _ok("a corrupted byte is not identical", raw_bad["pad_00_80004380_init"]["identical"],
                       False, failures)
        hard, _soft = vu.symbol_problems({"pad_00_80004380_init": report},
                                         {"pad_00_80004380_init": fresh}, raw_bad)
        failures = _ok("the corrupted range REFUSES", any("not identical" in p for p in hard), True,
                       failures)
        failures = _ok("... and the refusal names the symbol it compared",
                       any("gTRKInterruptVectorTable" in p for p in hard), True, failures)

        # a truncated / moved symbol at that address is not a resolution, it is a mismatch
        target_s, candidate_s = _pad_objects(candidate_size=8, candidate_offset=8)
        raw_short = vu.raw_symbol_rows(_write(tmp, "t_short.o", target_s),
                                       _write(tmp, "c_short.o", candidate_s))
        failures = _ok("a symbol that starts elsewhere does not resolve by address",
                       raw_short["pad_00_80004380_init"]["in_candidate"], False, failures)
        hard, _soft = vu.symbol_problems({"pad_00_80004380_init": report},
                                         {"pad_00_80004380_init": fresh}, raw_short)
        failures = _ok("an unwritten pad range still REFUSES", hard != [], True, failures)
        target_t, candidate_t = _pad_objects(candidate_size=8)
        raw_trunc = vu.raw_symbol_rows(_write(tmp, "t_tr.o", target_t),
                                       _write(tmp, "c_tr.o", candidate_t))
        hard, _soft = vu.symbol_problems({"pad_00_80004380_init": report},
                                         {"pad_00_80004380_init": fresh}, raw_trunc)
        failures = _ok("a truncated symbol at the address still REFUSES",
                       any("sizes differ" in p for p in hard), True, failures)

    failures += _pad_row_end_to_end()
    return failures


def _pad_row_end_to_end() -> int:
    """The gate ROW itself (`verify_units`) on the pad shape. Skipped without `objdiff-cli`."""
    objdiff = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
    if not os.path.exists(objdiff):
        print("skip  pad-row re-measure (no objdiff-cli)")
        return 0
    failures = 0
    for label, corrupt, want in (("intact", False, True), ("one corrupted byte", True, False)):
        with tempfile.TemporaryDirectory() as tmp:
            for rel in (vu.target_object_rel(PAD_UNIT), vu.src_object_rel(PAD_UNIT)):
                os.makedirs(os.path.dirname(os.path.join(tmp, rel)), exist_ok=True)
            target, candidate = _pad_objects(corrupt=corrupt)
            _write(tmp, os.path.join("build", "RMHE08", "obj", "Runtime.PPCEABI.H",
                                     "TRK_interrupt_vectors.o"), target)
            _write(tmp, os.path.join("build", "RMHE08", "src", "Runtime.PPCEABI.H",
                                     "TRK_interrupt_vectors.o"), candidate)
            json.dump({"units": [{"name": vu.report_unit_name(PAD_UNIT),
                                   "measures": {"total_code": "16", "fuzzy_match_percent": 100.0},
                                   "functions": [{"name": "pad_00_80004380_init", "size": "16",
                                                  "fuzzy_match_percent": 100.0}]}]},
                      open(os.path.join(tmp, "build", "RMHE08", "report.json"), "w"))
            ok, detail, _adv = vu.verify_units(tmp, [PAD_UNIT], objdiff=objdiff)
            failures = _ok("verify_units %s pad-named row -> %s" % (label, "passes" if want
                                                                   else "refuses"), ok, want,
                           failures)
            if not want:
                failures = _ok("... and the refusal says which bytes",
                               "pad_00_80004380_init" in detail and "not identical" in detail, True,
                               failures)
    return failures


# --------------------------------------------------------------------------------------------------
# layers that need the real build tree (skipped, not failed, without it)
# --------------------------------------------------------------------------------------------------

def _real_objects():
    tgt = os.path.join(ROOT, "build", "RMHE08", "obj", "hud", "fn_80334568.o")
    cand = os.path.join(ROOT, "build", "RMHE08", "src", "hud", "fn_80334568.o")
    objdiff = os.path.join(ROOT, "build", "tools", "objdiff-cli.exe")
    return tgt, cand, objdiff


def live_rows() -> int:
    """A registered unit, re-measured from its objects, reproduces the report. Skipped without a tree."""
    tgt, cand, objdiff = _real_objects()
    report = os.path.join(ROOT, "build", "RMHE08", "report.json")
    if not (os.path.exists(tgt) and os.path.exists(cand) and os.path.exists(objdiff)
            and os.path.exists(report)):
        print("skip  live cross-check (no compiled unit / report / objdiff)")
        return 0
    failures = 0
    ok, detail, advisories = vu.verify_units(ROOT, ["hud/fn_80334568"], objdiff=objdiff)
    failures = _ok("verify_units accepts a real registered unit", ok, True, failures)
    failures = _ok("... and reports the objects it re-measured", "re-measured" in detail, True, failures)
    for line in advisories:
        print("note  advisory: " + line)
    ok, detail = vu.registration_check(ROOT, ["hud/fn_80334568"])
    failures = _ok("the real tree registers hud/fn_80334568", ok, True, failures)
    failures = _ok("... and the registration check refuses a unit the tree lacks",
                   vu.registration_check(ROOT, ["hud/fn_00000000"])[0], False, failures)
    return failures


def doctored_report_rows() -> int:
    """A report doctored to disagree with a real `report generate` must refuse. Skipped without a tree."""
    tgt, cand, objdiff = _real_objects()
    report = os.path.join(ROOT, "build", "RMHE08", "report.json")
    if not (os.path.exists(tgt) and os.path.exists(cand) and os.path.exists(objdiff)
            and os.path.exists(report)):
        print("skip  doctored-report fixture (no compiled unit / report / objdiff)")
        return 0
    failures = 0
    real = None
    for entry in json.load(open(report, encoding="utf-8")).get("units") or []:
        if entry.get("name") == "main/hud/fn_80334568":
            real = entry
            break
    if real is None:
        print("skip  doctored-report fixture (hud/fn_80334568 not in the report)")
        return 0
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "obj", "hud"))
        os.makedirs(os.path.join(tmp, "build", "RMHE08", "src", "hud"))
        shutil.copyfile(tgt, os.path.join(tmp, "build", "RMHE08", "obj", "hud", "fn_80334568.o"))
        shutil.copyfile(cand, os.path.join(tmp, "build", "RMHE08", "src", "hud", "fn_80334568.o"))
        doctored = json.loads(json.dumps(real))
        # flip the first 100% symbol's score so the committed report can no longer be reproduced
        victim = next((f for f in doctored["functions"]
                       if isinstance(f.get("fuzzy_match_percent"), (int, float))
                       and f["fuzzy_match_percent"] >= 100.0), None)
        if victim is None:
            print("skip  doctored-report fixture (no 100% symbol to doctor)")
            return 0
        victim["fuzzy_match_percent"] = 13.0
        json.dump({"units": [doctored]},
                  open(os.path.join(tmp, "build", "RMHE08", "report.json"), "w"))
        ok, detail, _adv = vu.verify_units(tmp, ["hud/fn_80334568"], objdiff=objdiff)
        failures = _ok("a report that disagrees with a fresh measurement refuses", ok, False, failures)
        failures = _ok("... and names the symbol", victim["name"] in detail, True, failures)
    return failures


def main() -> int:
    failures = name_rows()
    failures += registration_rows()
    failures += registration_check_rows()
    failures += drift_rows()
    failures += snapshot_rows()
    failures += arithmetic_rows()
    failures += symbol_rows()
    failures += size_gap_rows()
    failures += pad_name_rows()
    failures += live_rows()
    failures += doctored_report_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
