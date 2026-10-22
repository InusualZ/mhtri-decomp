"""The `own_rows` manifest key (docs/pipeline.md 16.1): a lane's batch may change `symbols.txt` only at addresses its
units claim, and `land --branch` lands such a branch (no registration diff) with its manifest's `units`."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import subprocess
import unittest.mock as mock

from tools.lib import testing
from tools.lib.project import Splits
from tools.units.landing import api as L
from tools.units.landing import state
from tools.units.landing.common import Batch, changed_paths
from tools.units.landing.rows import manifest as M

TIER = "fixture"

testing.isolate_live_state()

SPLITS = ("Sections:\n\t.text       type:code align:32\n\t.data       type:data align:8\n\n"
          "enemy/em_a.cpp:\n\t.text       start:0x80010000 end:0x80010100\n\t.data       start:0x80500000 end:0x80500010\n\n"
          "enemy/em_b.cpp:\n\t.text       start:0x80010100 end:0x80010200\n")
SYMBOLS = ("fn_80010000 = .text:0x80010000; // type:function size:0x80\n"
           "fn_80010080 = .text:0x80010080; // type:function size:0x80\n"
           "fn_80010100 = .text:0x80010100; // type:function size:0x100\n"
           "lbl_80500000 = .data:0x80500000; // type:object size:0x10\n")
UNIT_A = "enemy/em_a"


def test_out_of_claim_rows(c):
    claims = Splits.parse(SPLITS)
    own = SYMBOLS.replace("fn_80010000", "em_a_init").replace("lbl_80500000", "em_a_table")
    c.check("a rename of the unit's own function and data rows is inside its claims",
            M.out_of_claim_rows(SYMBOLS, own, claims, [UNIT_A]), (4, []))
    split = SYMBOLS.replace("fn_80010080 = .text:0x80010080; // type:function size:0x80",
                            "fn_80010080 = .text:0x80010080; // type:function size:0x40\n"
                            "em_a_tail = .text:0x800100C0; // type:function size:0x40")
    c.check("a split (a resized row plus an added one) inside the claim passes",
            M.out_of_claim_rows(SYMBOLS, split, claims, ["enemy/em_a.cpp"])[1], [])
    foreign = SYMBOLS.replace("fn_80010100", "em_b_step")
    c.check("a rename of another unit's row is refused, both sides named with their owner",
            M.out_of_claim_rows(SYMBOLS, foreign, claims, [UNIT_A])[1],
            ["-fn_80010100 .text:0x80010100 (claimed by enemy/em_b)",
             "+em_b_step .text:0x80010100 (claimed by enemy/em_b)"])
    unclaimed = SYMBOLS + "fn_80020000 = .text:0x80020000; // type:function size:0x10\n"
    c.check("an added row nobody claims is refused",
            M.out_of_claim_rows(SYMBOLS, unclaimed, claims, [UNIT_A])[1],
            ["+fn_80020000 .text:0x80020000 (claimed by no unit)"])
    deleted = SYMBOLS.replace("lbl_80500000 = .data:0x80500000; // type:object size:0x10\n", "")
    c.check("a deleted row of the unit's own data passes", M.out_of_claim_rows(SYMBOLS, deleted, claims, [UNIT_A]),
            (1, []))
    wrong_section = SYMBOLS + "em_a_bss = .bss:0x80010010; // type:object size:0x4\n"
    c.check("the claim is read in the row's own section (a .bss row at a .text address is not inside)",
            M.out_of_claim_rows(SYMBOLS, wrong_section, claims, [UNIT_A])[1],
            ["+em_a_bss .bss:0x80010010 (claimed by no unit)"])
    c.check("a changed line that is no map row is refused",
            M.out_of_claim_rows(SYMBOLS, "// a comment\n" + SYMBOLS, claims, [UNIT_A])[1],
            ["+// a comment (not a map row)"])
    c.check("line endings and order do not count as changes",
            M.out_of_claim_rows(SYMBOLS, "".join(reversed(SYMBOLS.splitlines(True))).replace("\n", "\r\n"), claims,
                                [UNIT_A]), (0, []))


def test_manifest_paths(c):
    man = {"lane": "l", "owns": ["src/enemy/"], "own_rows": ["config/RMHE08/symbols.txt"]}
    c.check("an own_rows map is not 'outside the manifest' (it is judged row by row)",
            M.outside_manifest(["src/enemy/em_a.cpp", "config/RMHE08/symbols.txt", "configure.py"], man),
            [("configure.py", "not in `owns`")])
    c.check("... and row_paths names it", M.row_paths(["src/enemy/em_a.cpp", "config/RMHE08/symbols.txt"], man),
            ["config/RMHE08/symbols.txt"])
    both = dict(man, owns=["src/enemy/", "config/RMHE08/symbols.txt"])
    c.check("listed in owns too, own_rows still narrows it", M.row_paths(["config/RMHE08/symbols.txt"], both),
            ["config/RMHE08/symbols.txt"])
    ro = dict(man, read_only=["config/RMHE08/symbols.txt"])
    c.check("read_only wins over own_rows", (M.row_paths(["config/RMHE08/symbols.txt"], ro),
                                             M.outside_manifest(["config/RMHE08/symbols.txt"], ro)),
            ([], [("config/RMHE08/symbols.txt", "read-only `config/RMHE08/symbols.txt`")]))
    c.check("own_rows must be a list of strings", M.shape_problems(dict(man, own_rows="x")),
            ["`own_rows` must be a list of non-empty strings"])


def test_registry_is_batch_material(c):
    from tools.units.landing.common import outside_batch
    from tools.units.landing.rows.subject import land_subject
    c.check("the name registry (tools/symbols/names.py) is batch material, landed under config/symbols",
            (outside_batch(["config/RMHE08/name_registry.jsonl"]), land_subject(["config/RMHE08/name_registry.jsonl"])),
            ([], "config/symbols: land config/RMHE08/name_registry.jsonl"))


def _git(root, *args):
    p = subprocess.run(["git", "-c", "user.email=t@example.invalid", "-c", "user.name=t", "-c", "commit.gpgsign=false",
                        *args], cwd=root, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
    return p.stdout.strip()


def _write(root, files):
    for rel, text in files.items():
        path = os.path.join(root, *rel.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    _git(root, "add", "-A")
    _git(root, "commit", "-q", "-m", "tree")


def _branch_fixture(root, symbols_on_branch):
    """`main` with two registered units; `worker/em-a` renames map rows and edits em_a's source, registering nothing;
    the lane's manifest `.pi/outbox/em-a.json` owns its source and lists the map under `own_rows`."""
    _git(root, "init", "-q")
    _git(root, "checkout", "-q", "-b", "main")
    _write(root, {"config/RMHE08/splits.txt": SPLITS, "config/RMHE08/symbols.txt": SYMBOLS,
                  "configure.py": 'config.libs = [\n    Object(NonMatching, "enemy/em_a.cpp"),\n'
                                  '    Object(NonMatching, "enemy/em_b.cpp"),\n]\n',
                  "src/enemy/em_a.cpp": "void fn_80010000(void) {}\n", "src/enemy/em_b.cpp": "int b;\n",
                  ".gitignore": ".pi/\n"})
    _git(root, "checkout", "-q", "-b", "worker/em-a")
    _write(root, {"config/RMHE08/symbols.txt": symbols_on_branch, "src/enemy/em_a.cpp": "void em_a_init(void) {}\n"})
    _git(root, "checkout", "-q", "main")
    os.makedirs(os.path.join(root, ".pi", "outbox"), exist_ok=True)
    with open(os.path.join(root, ".pi", "outbox", "em-a.json"), "w", encoding="utf-8") as fh:
        json.dump({"manifest": {"lane": "em-a", "owns": ["src/enemy/em_a.cpp"],
                                "own_rows": ["config/RMHE08/symbols.txt"], "units": ["enemy/em_a"]}}, fh)


def _land(root):
    """`land --branch worker/em-a --manifest em-a` with the gate reduced to its manifest row (run for real on the applied
    tree) -> (exit, answer line, the units the gate saw, the row's (status, detail))."""
    seen = {}

    def verify(main, units, base, dry_run, no_build, problems=None, **_kw):
        b = Batch(main=main, units=units, unit_units=units, base=None, recorded={})
        b.paths = changed_paths(main)
        M.manifest_row(b)
        row = b.checks[-1]
        seen.update(units=list(units), row=(row.status, row.detail))
        if row.failed:
            if problems is not None:
                problems.append("%s [GATE]: %s" % (row.name, row.detail))
            return 1
        L.write_land_message(main, "land: %s\n\nledger: (fixture)\n" % ",".join(units))
        return 0
    out = io.StringIO()
    state.set_manifest("em-a")
    try:
        with mock.patch.object(L.gate, "verify", verify), contextlib.redirect_stdout(out), \
                contextlib.redirect_stderr(io.StringIO()):
            code = L.land_branch(root, "worker/em-a", no_build=True, check_outbox=False, release_claims=False,
                                 subject="game/enemy: name the em_a rows")
    finally:
        state.set_manifest(None)
    return code, L.answer_line(out.getvalue()), seen.get("units"), seen.get("row")


def test_land_branch_own_rows(c):
    own = SYMBOLS.replace("fn_80010000", "em_a_init")
    with testing.temp_dir() as root:
        _branch_fixture(root, own)
        head = _git(root, "rev-parse", "HEAD")
        code, answer, units, row = _land(root)
        c.check("a branch that registers nothing lands with its manifest's units (no integrator)",
                (code, answer.split(" ")[0], units), (0, "LANDED", ["enemy/em_a"]))
        c.check("... the manifest row passed on the applied tree, counting its own rows", row[0], "PASS")
        c.check("... and the one commit carries the map row with the source",
                sorted(_git(root, "diff", "--name-only", head, "HEAD").splitlines()),
                ["config/RMHE08/symbols.txt", "src/enemy/em_a.cpp"])
    with testing.temp_dir() as root:
        _branch_fixture(root, own.replace("fn_80010100", "em_b_step"))
        head = _git(root, "rev-parse", "HEAD")
        code, answer, units, row = _land(root)
        c.check("a branch that also renames another unit's row is refused", (code, answer.split(" ")[0], row[0]),
                (1, "REFUSED", "FAIL"))
        c.check("... naming the out-of-range rows and their owner",
                "-fn_80010100 .text:0x80010100 (claimed by enemy/em_b)" in row[1]
                and "+em_b_step .text:0x80010100 (claimed by enemy/em_b)" in row[1], True)
        c.check("... with main unchanged and the apply undone",
                (_git(root, "rev-parse", "HEAD"), _git(root, "status", "--porcelain", "--untracked-files=no")),
                (head, ""))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
