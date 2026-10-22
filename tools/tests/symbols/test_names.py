"""names.py: the name registry on fixture repositories - propose (first proposal wins, the refusals), lookup through
main's ref from a lane worktree, the union merge of two lanes' appends, list, export."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import subprocess

from tools.lib import testing
from tools.symbols import names as N

TIER = "fixture"

SPLITS = ("Sections:\n\t.text       type:code align:32\n\n"
          "enemy/em_hub.cpp:\n\t.text       start:0x80010000 end:0x80010100\n\n"
          "enemy/em_a.cpp:\n\t.text       start:0x80010100 end:0x80010200\n")
SYMBOLS = ("fn_80010000 = .text:0x80010000; // type:function size:0x40\n"
           "em_hub_named = .text:0x80010040; // type:function size:0x40\n"
           "fn_80010080 = .text:0x80010080; // type:function size:0x80\n"
           "fn_80010100 = .text:0x80010100; // type:function size:0x100\n")


def _repo(fx):
    fx.init()
    fx.commit({".gitattributes": N.ATTRIBUTE_RULE + "\n", "config/RMHE08/splits.txt": SPLITS,
               "config/RMHE08/symbols.txt": SYMBOLS,
               N.REGISTRY_REL: ""}, "base")
    return str(fx.root)


def _cli(root, *argv):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        code = N.main(list(argv), root=root)
    return code, out.getvalue(), err.getvalue()


def test_propose_rules(c):
    with testing.GitFixture() as fx:
        root = _repo(fx)
        code, out, _ = _cli(root, "propose", "0x80010000", "em_hub_update", "--lane", "em-a", "--evidence", "calls x")
        c.check("a first proposal is recorded", (code, out.split(" ")[0]), (0, "PROPOSED"))
        line = json.loads(open(os.path.join(root, *N.REGISTRY_REL.split("/")), encoding="utf-8").read())
        c.check("... as one JSON line naming the row it renames", (line["address"], line["name"], line["old"],
                line["section"], line["lane"]), ("0x80010000", "em_hub_update", "fn_80010000", ".text", "em-a"))
        code, out, _ = _cli(root, "propose", "0x80010000", "em_hub_update", "--lane", "em-b", "--evidence", "y")
        c.check("the same name again is `already`, nothing written", (code, out.split(" ")[0],
                len(open(os.path.join(root, *N.REGISTRY_REL.split("/"))).read().splitlines())), (0, "ALREADY", 1))
        code, out, _ = _cli(root, "propose", "0x80010000", "em_hub_tick", "--lane", "em-b", "--evidence", "y")
        c.check("another name for a proposed address is refused, naming the winner (first proposal wins)",
                (code, "first proposal wins: use em_hub_update" in out), (1, True))
        code, out, _ = _cli(root, "propose", "0x80010080", "em_hub_update", "--lane", "em-b", "--evidence", "y")
        c.check("a name already proposed for another address is refused", (code, "0x80010000" in out), (1, True))
        code, out, _ = _cli(root, "propose", "0x80010080", "em_hub_named", "--lane", "em-b", "--evidence", "y")
        c.check("a name the map uses at another address is refused", (code, "0x80010040" in out), (1, True))
        code, out, _ = _cli(root, "propose", "0x80010040", "em_hub_other", "--lane", "em-b", "--evidence", "y")
        c.check("a row main already named keeps its name: refused, printing it",
                (code, "main already named 0x80010040 em_hub_named - use em_hub_named" in out), (1, True))
        c.check("a generated, address-named or invalid name is refused (rule 7)",
                [_cli(root, "propose", "0x80010080", n, "--lane", "b", "--evidence", "y")[0]
                 for n in ("lbl_80010080", "em_fn_80010080_x", "Panel80010080", "a b")], [1, 1, 1, 1])
        c.check("an address with no map row is refused", _cli(root, "propose", "0x80010004", "em_x", "--lane", "b",
                                                               "--evidence", "y")[0], 1)
        code, out, _ = _cli(root, "propose", "0x80010080", "em_hub_draw", "--lane", "em-b", "--evidence", "y",
                            "--dry-run")
        c.check("--dry-run judges and writes nothing", (code, "dry run" in out,
                len(open(os.path.join(root, *N.REGISTRY_REL.split("/"))).read().splitlines())), (0, True, 1))


def test_lanes_share_main(c):
    """Two lanes in their own worktrees: each sees main's landed registry through the `main` ref, its own appends in
    its tree; their appends to the one file merge as a union, and the reader resolves the clash first-proposal-wins."""
    with testing.GitFixture() as fx:
        root = _repo(fx)
        _cli(root, "propose", "0x80010000", "em_hub_update", "--lane", "hub", "--evidence", "x")
        fx.git("commit", "-q", "-am", "registry: em_hub_update")
        a = fx.worktree(os.path.join(str(fx.root) + "-a"), "lane-a")
        b = fx.worktree(os.path.join(str(fx.root) + "-b"), "lane-b")
        code, out, _ = _cli(str(a), "lookup", "0x80010000")
        c.check("a lane worktree reads main's registry through the ref", "em_hub_update winner" in out, True)
        c.check("... and reuses the name rather than proposing its own",
                _cli(str(a), "propose", "0x80010000", "em_hub_step", "--lane", "a", "--evidence", "x")[0], 1)
        _cli(str(a), "propose", "0x80010080", "em_a_draw", "--lane", "a", "--evidence", "x",)
        _cli(str(b), "propose", "0x80010080", "em_b_draw", "--lane", "b", "--evidence", "x")
        fx.git("commit", "-q", "-am", "a", cwd=a)
        fx.git("commit", "-q", "-am", "b", cwd=b)
        fx.git("merge", "-q", "--no-edit", "lane-a")
        base = fx.git("merge-base", "main", "lane-b").strip()
        patch = subprocess.run(["git", "diff", "--binary", base, "lane-b"], cwd=root, capture_output=True).stdout
        ap = subprocess.run(["git", "apply", "-3", "-"], cwd=root, input=patch, capture_output=True)
        text = open(os.path.join(root, *N.REGISTRY_REL.split("/")), encoding="utf-8").read()
        c.check("two lanes' appends merge cleanly under the registry's attribute rule (land.py's `git apply -3`)",
                (ap.returncode, "<<<<<<<" in text, "em_a_draw" in text and "em_b_draw" in text), (0, False, True))
        reg = N.Registry(root)
        c.check("... and the earlier proposal wins the address, the later one is superseded",
                sorted((e.name, e.status) for e in reg.entries if e.address == 0x80010080),
                [("em_a_draw", "winner"), ("em_b_draw", "superseded")])


def test_resolve_and_export(c):
    e = [N.Entry(0x80010080, "late", at="2026-10-22T10:00:00", order=0),
         N.Entry(0x80010080, "early", at="2026-10-22T09:00:00", order=1),
         N.Entry(0x80010100, "early", at="2026-10-22T11:00:00", order=2),
         N.Entry(0x80010080, "early", at="2026-10-22T12:00:00", order=3)]
    c.check("resolve: first by time wins, a later other name is superseded, a winning name elsewhere is taken, a repeat "
            "is a duplicate", [(x.name, x.status, x.winner) for x in N.resolve(e)],
            [("late", "superseded", "early"), ("early", "winner", ""), ("early", "name-taken", "0x80010080"),
             ("early", "duplicate", "early")])
    with testing.GitFixture() as fx:
        root = _repo(fx)
        _cli(root, "propose", "0x80010000", "em_hub_update", "--lane", "hub", "--evidence", "x")
        _cli(root, "propose", "0x80010100", "em_a_main", "--lane", "a", "--evidence", "x")
        fx.git("commit", "-q", "-am", "registry")
        # main lands em_a's own-row rename under another name before the export
        fx.commit({"config/RMHE08/symbols.txt": SYMBOLS.replace("fn_80010100", "em_a_entry")}, "game/enemy: em_a rows")
        code, out, err = _cli(root, "export")
        c.check("export: an `old new` line per pending winner (symedit rename-batch input)", out, "fn_80010000 em_hub_update\n")
        c.check("... and a winner whose row main named meanwhile is a note, not a rename",
                "main named it em_a_entry - the proposal is stale" in err, True)
        code, out, _ = _cli(root, "list", "--pending")
        c.check("list --pending: the winners main's map does not carry", (out.count("winner"), "em_hub_update" in out),
                (2, True))
        code, out, _ = _cli(root, "lookup", "em_hub_update", "0x80010100")
        c.check("lookup by name and by address", ("reg  0x80010000 em_hub_update" in out,
                "map  0x80010100 .text em_a_entry size:0x100 owner:enemy/em_a" in out), (True, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
