"""lib.cache: the four states, the two signature modes, and the per-file keys."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import hashlib
import json
import os
import tempfile
from pathlib import Path

from tools.lib import cache, testing

TIER = "fixture"


def _bump(path: Path, data: bytes) -> None:
    """Rewrite `path` and move its mtime forward, so a stat signature sees the change on any filesystem."""
    path.write_bytes(data)
    st = os.stat(path)
    os.utime(path, ns=(st.st_atime_ns, st.st_mtime_ns + 2_000_000_000))


def test_states(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        a, b, other = root / "a.txt", root / "b.txt", root / "other.txt"
        a.write_bytes(b"one")
        b.write_bytes(b"two")
        other.write_bytes(b"x")
        st = cache.Stamped(root / "cache.json", [a, b], schema=1)
        c.check("no file is missing", (st.state(), st.load()), ("missing", None))
        (root / "cache.json").write_text("not json", encoding="utf-8")
        c.check("an unreadable file is unstamped", st.state(), "unstamped")
        (root / "cache.json").write_text(json.dumps({"payload": 1}), encoding="utf-8")
        c.check("a file without a signature is unstamped", st.state(), "unstamped")
        st.save({"k": [1, 2]})
        c.check("a saved cache is fresh and loads", (st.state(), st.load()), ("fresh", {"k": [1, 2]}))
        other.write_bytes(b"changed")
        c.check("an edit to a file not in inputs leaves it fresh", st.state(), "fresh")
        a.write_bytes(b"ONE")
        c.check("a changed input is stale and loads nothing", (st.state(), st.load()), ("stale", None))
        st.save("again")
        c.check("a schema change is stale", cache.Stamped(root / "cache.json", [a, b], schema=2).state(), "stale")
        b.unlink()
        c.check("a deleted input is stale", st.state(), "stale")
        c.raises("an unknown mode is refused", ValueError, cache.Stamped, root / "c.json", [a], mode="mtime")


def test_stat_mode_and_discovered_inputs(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "dump").mkdir()
        f1 = root / "dump" / "a.s"
        f1.write_bytes(b"x")

        def files():
            return sorted((root / "dump").glob("*.s"))

        st = cache.Stamped(root / "graph.json", files, mode="stat", base=root / "dump")
        st.save([1])
        c.check("a stat-signed cache is fresh", st.state(), "fresh")
        (root / "dump" / "b.s").write_bytes(b"y")
        c.check("a new file in a discovered input set is stale", st.state(), "stale")
        st.save([2])
        _bump(f1, b"x")
        c.check("a rewrite (same bytes, newer mtime) is stale in stat mode", st.state(), "stale")


def test_file_keys(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        p = root / "x.bin"
        p.write_bytes(b"abc")
        c.check("content_hash is the sha1 of the bytes", cache.content_hash(p), hashlib.sha1(b"abc").hexdigest())
        c.check("... or another algorithm", cache.content_hash(p, "sha256"), hashlib.sha256(b"abc").hexdigest())
        st = os.stat(p)
        c.check("stat_key is [mtime_ns, size]", cache.stat_key(p), [st.st_mtime_ns, 3])
        c.check("stat_key of a missing file is None", cache.stat_key(root / "nope"), None)
        h = hashlib.sha1(("x.bin\0%d\0%d\n" % (st.st_size, st.st_mtime_ns)).encode("utf-8")).hexdigest()
        c.check("stat_digest is the callers.py dump signature (name, size, mtime per file)",
                cache.stat_digest([p], root), h)
        before = cache.stat_digest([p], root)
        _bump(p, b"abd")
        c.check("... and moves when a file is rewritten", cache.stat_digest([p], root) != before, True)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
