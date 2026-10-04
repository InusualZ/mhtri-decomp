"""The neighbour row: a unit the batch does not name whose split target object changed by NAMES only (same
rename-insensitive fingerprint, different bytes) is not drift and is named separately; a moved one still refuses."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os
import tempfile
import unittest.mock as mock

import tools.units.verifyunit as vu
from tools.lib import testing
from tools.units.landing import api as L

TIER = "fixture"


def test_names_only_changes(c):
    fp_b, fp_a = {"a": "1", "n": "2", "m": "3", "new": None}, {"a": "9", "n": "2", "m": "4", "new": "5"}
    raw_b, raw_a = {"a": "x", "n": "y", "m": "z"}, {"a": "x2", "n": "y2", "m": "z2", "new": "w"}
    c.check("same fingerprint, other bytes, not in the batch: names only",
            vu.names_only_changes(fp_b, fp_a, raw_b, raw_a, ["a"]), ["n"])
    c.check("a batch unit is never listed", vu.names_only_changes(fp_b, fp_a, raw_b, raw_a, ["n"]), [])
    c.check("identical bytes are no change", vu.names_only_changes(fp_b, fp_b, raw_b, raw_b, []), [])
    with tempfile.TemporaryDirectory() as d:
        os.makedirs(os.path.join(d, "config", "RMHE08"))
        os.makedirs(os.path.join(d, "build", "RMHE08", "obj", "U"))
        with open(os.path.join(d, "config", "RMHE08", "splits.txt"), "w") as fh:
            fh.write("Sections:\n\t.text type:code\n\nU/u.cpp:\n\t.text start:0x80000000 end:0x80000010\n")
        with open(os.path.join(d, "build", "RMHE08", "obj", "U", "u.o"), "wb") as fh:
            fh.write(b"not an elf")
        c.check("target_object_hashes reads the registered units' split objects",
                list(vu.target_object_hashes(d)), ["U/u"])


def test_drift_row_names_them(c):
    b = L.Batch(main=".", units=["Net/a"], unit_units=["Net/a"], base=None, recorded={})
    b.extra.update(targets_before={"Net/a": "1", "Net/n": "2"}, targets_raw_before={"Net/a": "x", "Net/n": "y"})
    with mock.patch.object(vu, "target_object_snapshot", lambda main: {"Net/a": "9", "Net/n": "2"}), \
            mock.patch.object(vu, "target_object_hashes", lambda main: {"Net/a": "x9", "Net/n": "y2"}), \
            contextlib.redirect_stderr(io.StringIO()) as err:
        L.objects.drift_row(b)
    row = b.checks[-1]
    c.check("a names-only neighbour passes the row and is named in its evidence",
            (row.status, row.evidence), ("PASS", "names only, not drift (name them in --units): 1 - Net/n"))
    c.contains("... and in the gate log", err.getvalue(), "Net/n")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
