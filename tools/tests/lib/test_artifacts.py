"""lib.artifacts: the registry's order, the split/manifest rules, and ensure() under each policy - with a stub rebuild
command, the lock (two concurrent callers rebuild once, a dead holder is taken over) and the fixture-tier guard."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import dataclasses
import io
import json
import os
import subprocess
import tempfile
import threading
import time
from pathlib import Path

from tools.lib import artifacts, testing
from tools.lib.lanes import seed

TIER = "fixture"
os.environ.pop(artifacts.ENV, None)       # the defaults are what is tested, never the caller's FRESH


def _age(path: Path, seconds: float) -> None:
    t = time.time() - seconds
    os.utime(path, (t, t))


def _tree(root: Path) -> None:
    """A tree with a split and a manifest, both newer than their inputs."""
    for rel in artifacts.SPLIT_INPUTS + artifacts.MANIFEST_INPUTS:
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("x", encoding="utf-8")
        _age(p, 100)
    (root / "build" / "RMHE08").mkdir(parents=True, exist_ok=True)
    (root / "build" / "RMHE08" / "config.json").write_text("{}", encoding="utf-8")
    (root / "build.ninja").write_text("", encoding="utf-8")


class Stub:
    """A rebuild command that makes the marker file (the 'artifact') and counts its runs; `fail` exits 1, `noop`
    succeeds and changes nothing, `delay` holds the lock long enough for a second caller to wait on it."""

    def __init__(self, marker: Path, fail: bool = False, noop: bool = False, delay: float = 0.0) -> None:
        self.marker, self.fail, self.noop, self.delay = marker, fail, noop, delay
        self.calls: list[list[str]] = []
        self.guard = threading.Lock()

    def __call__(self, argv, cwd):
        with self.guard:
            self.calls.append(list(argv))
        time.sleep(self.delay)
        if self.fail:
            return subprocess.CompletedProcess(argv, 1, "", "boom: the rebuild failed\n")
        if not self.noop:
            self.marker.write_text("built", encoding="utf-8")
        return subprocess.CompletedProcess(argv, 0, "", "")


def _marker_check(marker: Path):
    def check(_ctx):
        return ("fresh", "built") if marker.exists() else ("stale", "the marker is missing")
    return check


def test_order(c):
    c.check("dependencies come first, each once", artifacts.order(["report", "tudiscover-graph"]),
            ["manifest", "split", "objects", "report", "asm-dump", "tudiscover-graph"])
    c.check("an artifact with no deps is itself", artifacts.order(["asm-dump"]), ["asm-dump"])
    c.raises("an unknown name is refused", SystemExit, artifacts.order, ["nope"])
    saved = dict(artifacts.REGISTRY)
    try:
        artifacts.REGISTRY["a"] = dataclasses.replace(artifacts.REGISTRY["asm-dump"], name="a", deps=("b",))
        artifacts.REGISTRY["b"] = dataclasses.replace(artifacts.REGISTRY["asm-dump"], name="b", deps=("a",))
        c.raises("a dependency cycle is refused", ValueError, artifacts.order, ["a"])
    finally:
        artifacts.REGISTRY.clear()
        artifacts.REGISTRY.update(saved)
    for name, art in artifacts.REGISTRY.items():
        c.expect("%s: every dependency is registered" % name, all(d in artifacts.REGISTRY for d in art.deps))
        c.expect("%s: a cost and a description" % name, art.cost[0] <= art.cost[1] and art.what)


def test_split_and_manifest_rules(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        c.check("no split is missing", artifacts.check_split(artifacts.Context(tmp))[0], "missing")
        c.check("... and the seeder agrees", seed.build_is_current(tmp), False)
        _tree(root)
        c.check("a split newer than its inputs is fresh", artifacts.check_split(artifacts.Context(tmp))[0], "fresh")
        c.check("a manifest newer than configure.py is fresh",
                artifacts.check_manifest(artifacts.Context(tmp))[0], "fresh")
        c.check("the seeder agrees with both", seed.build_is_current(tmp), True)
        sym = root / "config" / "RMHE08" / "symbols.txt"
        sym.write_text("renamed", encoding="utf-8")
        os.utime(sym, (time.time() + 5, time.time() + 5))
        state, why = artifacts.check_split(artifacts.Context(tmp))
        c.check("an edited map makes the split stale", state, "stale")
        c.contains("... naming the input", why, "symbols.txt")
        c.check("... and the seeder says so too (one rule)", seed.build_is_current(tmp), False)
        _age(sym, 100)
        cfg = root / "configure.py"
        os.utime(cfg, (time.time() + 5, time.time() + 5))
        c.check("an edited configure.py makes the manifest stale",
                artifacts.check_manifest(artifacts.Context(tmp))[0], "stale")
        c.check("... and the seeder", seed.build_is_current(tmp), False)


def test_refuse_warn_and_fresh(c):
    with tempfile.TemporaryDirectory() as tmp:
        marker = Path(tmp) / "artifact"
        stub = Stub(marker)
        ctx = artifacts.Context(tmp, runner=stub)
        checks = {"asm-dump": _marker_check(marker)}
        try:
            artifacts.ensure(["asm-dump"], ctx, "refuse", checks=checks, out=io.StringIO())
            c.fail("refuse raises", "no exception")
        except artifacts.StaleArtifact as exc:
            c.contains("refuse names the artifact and its reason", str(exc.code), "asm-dump is stale - the marker")
            c.contains("... and the refresh command", str(exc.code), "tools/splits/dump_asm.py")
        out = io.StringIO()
        res = artifacts.ensure(["asm-dump"], ctx, "warn", checks=checks, out=out)
        c.check("warn uses it stale", (res[0].state, res[0].refreshed, stub.calls), ("stale", False, []))
        c.contains("warn says what it is using", out.getvalue(), "WARNING: asm-dump is stale")
        marker.write_text("built", encoding="utf-8")
        out = io.StringIO()
        for policy in artifacts.POLICIES:
            artifacts.ensure(["asm-dump"], ctx, policy, checks=checks, out=out)
        c.check("a fresh artifact prints nothing under any policy (byte-identical output)", out.getvalue(), "")
        c.check("... and runs nothing", stub.calls, [])


def test_auto_rebuilds_once(c):
    with tempfile.TemporaryDirectory() as tmp:
        marker = Path(tmp) / "artifact"
        stub = Stub(marker)
        ctx = artifacts.Context(tmp, runner=stub)
        out = io.StringIO()
        res = artifacts.ensure(["asm-dump"], ctx, "auto", checks={"asm-dump": _marker_check(marker)}, out=out)
        c.check("auto rebuilds a stale artifact and reports it fresh", (res[0].state, res[0].refreshed), ("fresh", True))
        c.check("... with the registry's command, once", [a[-1].replace("\\", "/")[-19:] for a in stub.calls],
                ["/splits/dump_asm.py"])
        c.contains("... announcing it on one line", out.getvalue(), "refreshing asm-dump: the marker is missing")
        c.check("... and releases its lock", os.path.exists(artifacts.lock_path(tmp, "asm-dump")), False)
        artifacts.ensure(["asm-dump"], ctx, "auto", checks={"asm-dump": _marker_check(marker)}, out=io.StringIO())
        c.check("a second ensure finds it fresh and runs nothing", len(stub.calls), 1)


def test_auto_failures(c):
    with tempfile.TemporaryDirectory() as tmp:
        marker = Path(tmp) / "artifact"
        checks = {"asm-dump": _marker_check(marker)}
        ctx = artifacts.Context(tmp, runner=Stub(marker, fail=True))
        try:
            artifacts.ensure(["asm-dump"], ctx, "auto", checks=checks, out=io.StringIO())
            c.fail("a failing rebuild raises", "no exception")
        except artifacts.RefreshFailed as exc:
            c.contains("a failing rebuild names its output", str(exc.code), "boom: the rebuild failed")
        c.check("... and releases the lock", os.path.exists(artifacts.lock_path(tmp, "asm-dump")), False)
        ctx = artifacts.Context(tmp, runner=Stub(marker, noop=True))
        try:
            artifacts.ensure(["asm-dump"], ctx, "auto", checks=checks, out=io.StringIO())
            c.fail("a rebuild that leaves it stale raises", "no exception")
        except artifacts.RefreshFailed as exc:
            c.contains("a rebuild that changed nothing is a failure, never a silent pass", str(exc.code),
                       "left it stale")


def test_concurrent_callers_rebuild_once(c):
    with tempfile.TemporaryDirectory() as tmp:
        marker = Path(tmp) / "artifact"
        stub = Stub(marker, delay=0.4)
        checks = {"asm-dump": _marker_check(marker)}
        results: list = []

        def worker():
            ctx = artifacts.Context(tmp, runner=stub)
            results.append(artifacts.ensure(["asm-dump"], ctx, "auto", checks=checks, out=io.StringIO())[0])

        threads = [threading.Thread(target=worker) for _ in range(3)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(10)
        c.check("three concurrent callers: one rebuild", len(stub.calls), 1)
        c.check("... and all three see it fresh", sorted(r.state for r in results), ["fresh"] * 3)
        c.check("... the waiters say another process rebuilt it",
                sum(1 for r in results if "rebuilt by another process" in r.note), 2)


def test_dead_lock_holder_is_taken_over(c):
    with tempfile.TemporaryDirectory() as tmp:
        marker = Path(tmp) / "artifact"
        lock = Path(artifacts.lock_path(tmp, "asm-dump"))
        lock.parent.mkdir(parents=True)
        lock.write_text(json.dumps({"pid": 2 ** 30}), encoding="utf-8")
        stub = Stub(marker)
        res = artifacts.ensure(["asm-dump"], artifacts.Context(tmp, runner=stub), "auto",
                               checks={"asm-dump": _marker_check(marker)}, out=io.StringIO(), lock_timeout=5)
        c.check("a lock left by a dead process does not wedge the rebuild", (res[0].state, len(stub.calls)),
                ("fresh", 1))
        held = artifacts.Lock(str(lock), timeout=0.3, poll=0.05, alive=lambda pid: True)
        lock.write_text(json.dumps({"pid": os.getpid()}), encoding="utf-8")
        c.raises("a live holder past the timeout is refused, naming the lock", SystemExit, held.__enter__)
        lock.unlink()


def test_policy_resolution_and_fixture_guard(c):
    saved = os.environ.get(artifacts.ENV)
    try:
        os.environ[artifacts.ENV] = "warn"
        c.check("FRESH overrides the tool's default", artifacts.resolve_policy(None, "refuse"), "warn")
        c.check("an explicit policy (a flag) beats FRESH", artifacts.resolve_policy("auto", "refuse"), "auto")
        os.environ[artifacts.ENV] = "sometimes"
        c.raises("an unknown FRESH value is refused", SystemExit, artifacts.resolve_policy, None, "refuse")
        os.environ.pop(artifacts.ENV)
        c.check("no FRESH: the tool's default", artifacts.resolve_policy(None, "auto"), "auto")
        out = io.StringIO()
        c.check("fixture tier: auto without a stub degrades to the tool's default",
                artifacts.effective_policy("auto", "refuse", None, out), "refuse")
        c.check("... to warn when that default is auto", artifacts.effective_policy("auto", "auto", None, out), "warn")
        c.contains("... and says so", out.getvalue(), "suppressed under the fixture test tier")
        c.check("... while a stub runner keeps auto", artifacts.effective_policy("auto", "refuse", print, out), "auto")
        with tempfile.TemporaryDirectory() as tmp:
            marker = Path(tmp) / "artifact"
            res = artifacts.ensure(["asm-dump"], artifacts.Context(tmp), "auto", default="auto",
                                   checks={"asm-dump": _marker_check(marker)}, out=io.StringIO())
            c.check("fixture tier: ensure(auto) with no stub runs nothing", (res[0].state, res[0].refreshed),
                    ("stale", False))
    finally:
        if saved is None:
            os.environ.pop(artifacts.ENV, None)
        else:
            os.environ[artifacts.ENV] = saved


def test_expensive_rebuild_is_announced_with_its_cost(c):
    saved = artifacts.REGISTRY["asm-dump"]
    try:
        artifacts.REGISTRY["asm-dump"] = dataclasses.replace(saved, cost=(200.0, 400.0))
        with tempfile.TemporaryDirectory() as tmp:
            marker = Path(tmp) / "artifact"
            out = io.StringIO()
            artifacts.ensure(["asm-dump"], artifacts.Context(tmp, runner=Stub(marker)), "auto",
                             checks={"asm-dump": _marker_check(marker)}, out=out)
            c.contains("an expensive rebuild prints its cost first", out.getvalue(),
                       "refreshing asm-dump (~200-400 s): the marker is missing")
    finally:
        artifacts.REGISTRY["asm-dump"] = saved


def test_unit_scoped_objects_and_report(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        src = root / "src" / "demo" / "unit.c"
        hdr = root / "include" / "demo.h"
        obj = root / "build" / "RMHE08" / "src" / "demo" / "unit.o"
        rep = root / "build" / "RMHE08" / "report.json"
        for p in (src, hdr, obj, rep):
            p.parent.mkdir(parents=True, exist_ok=True)
        hdr.write_text("int x;\n", encoding="utf-8")
        src.write_text('#include "demo.h"\n', encoding="utf-8")
        obj.write_bytes(b"o")
        rep.write_text("{}", encoding="utf-8")
        for p, age in ((hdr, 30), (src, 30), (obj, 20), (rep, 10)):
            _age(p, age)
        ctx = artifacts.Context(tmp, src=str(src), obj=str(obj))
        c.check("an object newer than its closure is fresh", artifacts.status("objects", ctx).state, "fresh")
        c.check("a report newer than the object is fresh", artifacts.status("report", ctx).state, "fresh")
        c.check("the unit's refresh command is its object", artifacts.status("objects", ctx).command,
                "ninja build/RMHE08/src/demo/unit.o")
        _age(hdr, 0)
        st = artifacts.status("objects", ctx)
        c.check("an edited header makes the object stale", st.state, "stale")
        c.contains("... naming the header (lib.report's rule)", st.reason, "demo.h")
        c.check("... and the report too", artifacts.status("report", ctx).state, "stale")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
