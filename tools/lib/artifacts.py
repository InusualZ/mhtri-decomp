"""The derived-artifact registry: what each build artefact is made from, how to rebuild it, and `ensure()`.
Spec: docs/tools/spec/lib-artifacts.md. CLI: none (library; `tools/units/fresh.py` is its command line)."""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass, field
from typing import Callable, Iterable, Sequence

from tools.lib import proc, testing

GAME = "RMHE08"
#: What `ensure()` does with a stale artifact: rebuild it, use it and say so, or refuse with the remedy.
POLICIES = ("auto", "warn", "refuse")
#: The environment override of a tool's default policy (`FRESH=auto python tools/splits/tudiscover.py ...`).
ENV = "FRESH"
#: Above this many seconds a rebuild is announced on one line before it starts.
EXPENSIVE_S = 60.0
#: Where the per-artifact rebuild locks live (gitignored build output, per tree).
LOCK_REL = os.path.join("build", "tmp", "fresh")
#: How often a waiting process re-reads a lock.
POLL_S = 0.25
#: The states; `n/a` is an artifact that does not apply to this tree (a slot check outside a slot).
STATES = ("fresh", "stale", "missing", "unknown", "n/a")

BUILD_REL = os.path.join("build", GAME)
#: The files `dtk dol split` reads (`ninja -t deps build/RMHE08/config.json`), relative to a tree.
SPLIT_INPUTS = (os.path.join("config", GAME, "config.yml"), os.path.join("config", GAME, "symbols.txt"),
                os.path.join("config", GAME, "splits.txt"), os.path.join("orig", GAME, "sys", "main.dol"),
                os.path.join("orig", GAME, "files", "mh3.sel"))
#: The files `build.ninja` is generated from.
MANIFEST_INPUTS = ("configure.py", os.path.join("tools", "project.py"), os.path.join("tools", "ninja_syntax.py"))
_NINJA_STEP = re.compile(r"^\[\d+/\d+\]\s*(.*)$", re.M)


# --- statuses and errors ----------------------------------------------------------------------------------------

@dataclass
class Status:
    """One artifact's verdict in one tree. `fresh` covers `n/a` (nothing to do)."""
    name: str
    state: str
    reason: str
    command: str = ""
    cost: str = ""
    path: str = ""
    refreshed: bool = False
    seconds: float | None = None
    note: str = ""

    @property
    def fresh(self) -> bool:
        return self.state in ("fresh", "n/a")

    def as_dict(self) -> dict:
        return {"name": self.name, "state": self.state, "reason": self.reason, "command": self.command,
                "cost": self.cost, "path": self.path, "refreshed": self.refreshed, "seconds": self.seconds,
                "note": self.note}

    def remedy(self) -> str:
        return "refresh: %s" % self.command if self.command else "no refresh command"


class StaleArtifact(SystemExit):
    """`refuse`: a stale artifact, with its reason and refresh command (exit 1 when uncaught)."""

    def __init__(self, status: Status) -> None:
        self.status = status
        super().__init__("REFUSED: %s is %s - %s\n  %s (or FRESH=auto to let the tool do it, FRESH=warn to use it "
                         "as it is)" % (status.name, status.state, status.reason, status.remedy()))


class RefreshFailed(SystemExit):
    """`auto`: the rebuild command failed, or ran and left the artifact stale."""

    def __init__(self, status: Status, detail: str) -> None:
        self.status = status
        super().__init__("REFRESH FAILED: %s (%s): %s" % (status.name, status.command, detail))


# --- the context a check runs in --------------------------------------------------------------------------------

@dataclass
class Context:
    """The tree an artifact lives in, plus the unit for the unit-scoped checks (`objects`, `report`)."""
    root: str
    src: str | None = None      # the unit's source file (absolute)
    obj: str | None = None      # the unit's compiled object (absolute)
    report: str | None = None   # the report the unit's rows are read from (default: the tree's)
    runner: Callable | None = None   # `runner(argv, cwd) -> CompletedProcess` for ninja probes and rebuilds
    main: str | None = None     # MAIN, for the slot check (default: `lib.repo.main_tree(root)`)
    extra: dict = field(default_factory=dict)

    def path(self, *parts: str) -> str:
        return os.path.join(self.root, *parts)

    def run(self, argv: Sequence[str], cwd: str | None = None, env: dict | None = None):
        if self.runner is not None:
            return self.runner(list(argv), cwd or self.root)
        kwargs = {"env": env} if env is not None else {}
        return proc.run(list(argv), cwd=cwd or self.root, **kwargs)

    def rel(self, path: str) -> str:
        try:
            return os.path.relpath(path, self.root).replace("\\", "/")
        except ValueError:
            return path


# --- the shared rules (one implementation: `lib.lanes.seed.build_is_current` calls these) -----------------------

def _stamp(t: float) -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(t))


def _mtime(path: str) -> float | None:
    try:
        return os.path.getmtime(path)
    except OSError:
        return None


def newer_inputs(output: str, inputs: Iterable[str]) -> list[str]:
    """The inputs whose mtime is strictly newer than `output`'s, as `name (input time > output time)`; an input that
    cannot be stat'ed is skipped (a worktree without `orig/`), a missing output is the caller's case."""
    t = _mtime(output)
    if t is None:
        return []
    out = []
    for p in inputs:
        m = _mtime(p)
        if m is not None and m > t:
            out.append("%s (%s > %s)" % (os.path.basename(p), _stamp(m), _stamp(t)))
    return out


def split_reasons(build_root: str, input_root: str | None = None) -> list[str]:
    """Why `build_root`'s split (`build/RMHE08/config.json`) is older than the map/DOL under `input_root` - `[]` when
    current, `["no build/RMHE08/config.json"]` when there is none."""
    input_root = input_root or build_root
    cfg = os.path.join(build_root, BUILD_REL, "config.json")
    if not os.path.isfile(cfg):
        return ["no %s" % os.path.join(BUILD_REL, "config.json").replace("\\", "/")]
    return newer_inputs(cfg, [os.path.join(input_root, r) for r in SPLIT_INPUTS])


def manifest_reasons(build_root: str, input_root: str | None = None) -> list[str]:
    """Why `build_root`'s `build.ninja` is older than its generator under `input_root` - `[]` when current."""
    input_root = input_root or build_root
    ninja = os.path.join(build_root, "build.ninja")
    if not os.path.isfile(ninja):
        return ["no build.ninja"]
    return newer_inputs(ninja, [os.path.join(input_root, r) for r in MANIFEST_INPUTS])


def ninja_plan(ctx: Context, target: str | None) -> tuple[list[str] | None, str]:
    """`(descriptions, note)` of the edges `ninja -n [target]` would run in the tree; None when ninja cannot say.
    Dry-run only: nothing is built and `.ninja_log` is not written (measured 0.04 s on MAIN)."""
    if not os.path.isfile(ctx.path("build.ninja")):
        return None, "no build.ninja"
    try:
        p = ctx.run(["ninja", "-n"] + ([target] if target else []))
    except OSError as exc:
        return None, "ninja unavailable (%s)" % exc
    if p.returncode != 0:
        tail = [ln for ln in ((p.stderr or "") + "\n" + (p.stdout or "")).strip().splitlines() if ln.strip()]
        return None, "ninja -n failed: %s" % (tail[-1] if tail else "exit %d" % p.returncode)
    return [m.group(1).strip() for m in _NINJA_STEP.finditer(p.stdout or "")], ""


def _plan_reason(steps: list[str], what: str) -> str:
    shown = ", ".join(steps[:3]) + ("" if len(steps) <= 3 else " (+%d more)" % (len(steps) - 3))
    return "ninja would run %d step(s) for %s: %s" % (len(steps), what, shown)


# --- the checks ----------------------------------------------------------------------------------------------------

def check_manifest(ctx: Context) -> tuple[str, str]:
    reasons = manifest_reasons(ctx.root)
    if reasons == ["no build.ninja"]:
        return "missing", "no build.ninja in this tree"
    if reasons:
        return "stale", "newer than build.ninja: " + ", ".join(reasons)
    return "fresh", "build.ninja is newer than configure.py, tools/project.py and tools/ninja_syntax.py"


def check_split(ctx: Context) -> tuple[str, str]:
    reasons = split_reasons(ctx.root)
    if reasons and reasons[0].startswith("no "):
        return "missing", "%s - this tree has no split of its own" % reasons[0]
    if reasons:
        return "stale", "newer than build/RMHE08/config.json: " + ", ".join(reasons)
    return "fresh", "config.json is newer than config.yml, symbols.txt, splits.txt and the DOL"


def dump_stamp(ctx: Context):
    """The asm dump's stamp for the tree (`lib.refs.DumpStamp.for_tree`: its own dump, else MAIN's by path)."""
    from tools.lib import refs, repo  # noqa: PLC0415 - refs is heavy; only the dump checks need it
    return refs.DumpStamp.for_tree(ctx.root, honour_env=repo.is_served(ctx.root), game=GAME)


def check_asm_dump(ctx: Context) -> tuple[str, str]:
    try:
        state, msg = dump_stamp(ctx).status()
    except OSError as exc:
        return "unknown", "the dump's stamp could not be checked (%s)" % exc
    return ("fresh" if state == "fresh" else "missing" if state == "missing" else "stale"), msg


def check_callers_index(ctx: Context) -> tuple[str, str]:
    from tools.lib import refs, repo  # noqa: PLC0415
    asm_dir = refs.dump_dir(ctx.root, honour_env=repo.is_served(ctx.root), game=GAME)
    files = refs.dump_files(asm_dir)
    cache = refs.dump_cache(ctx.root)
    if not files:
        return "n/a", "no asm dump - callers answers from the split objects (build/tmp/callers/elf-graph.json)"
    if not os.path.exists(cache):
        return "missing", "no %s" % ctx.rel(cache)
    try:
        with open(cache, encoding="utf-8") as fh:
            index = json.load(fh)
    except (OSError, ValueError) as exc:
        return "stale", "unreadable index (%s)" % exc
    if index.get("schema") != refs.SCHEMA:
        return "stale", "index schema %s, the tool writes %s" % (index.get("schema"), refs.SCHEMA)
    if index.get("signature") != refs.dump_signature(asm_dir, files, ctx.root) or index.get("files") != len(files):
        return "stale", "%s since the index was built (%d file(s) now)" % (refs.DUMP_CHANGED, len(files))
    return "fresh", "matches the dump's %d file(s)" % len(files)


def check_tudiscover_graph(ctx: Context) -> tuple[str, str]:
    """The graph's stamp hashes `tudiscover`'s own parse code, so the tool answers for it (`cache --check`)."""
    tool = ctx.path("tools", "splits", "tudiscover.py")
    if not os.path.isfile(tool):
        return "n/a", "this tree has no tools/splits/tudiscover.py"
    p = ctx.run([sys.executable, tool, "cache", "--check"])
    if "unrecognized arguments" in (p.stderr or ""):
        return "unknown", "this tree's tudiscover has no `cache --check` (it predates lib.artifacts)"
    line = ((p.stdout or "").strip().splitlines() or [""])[-1]
    msg = line.split(None, 2)[-1] if line.startswith("graph cache") else (line or "exit %d" % p.returncode)
    if p.returncode == 0:
        return "fresh", msg
    if p.returncode == 2 or "no graph cache" in msg:
        return "missing", msg
    return ("stale" if p.returncode == 1 else "unknown"), msg


def _objects(ctx: Context) -> tuple[str, str]:
    if ctx.src and ctx.obj:
        from tools.lib import report  # noqa: PLC0415
        reasons, _newest = report.unit_reasons(ctx.src, ctx.obj, ctx.root, rel=ctx.rel)
        if not reasons:
            return "fresh", "%s is newer than its include closure" % ctx.rel(ctx.obj)
        return ("missing" if not os.path.exists(ctx.obj) else "stale"), "; ".join(reasons)
    steps, note = ninja_plan(ctx, "all_source")
    if steps is None:
        return ("missing" if note == "no build.ninja" else "unknown"), note
    return ("stale", _plan_reason(steps, "all_source")) if steps else ("fresh", "ninja -n all_source: no work")


def _report(ctx: Context) -> tuple[str, str]:
    path = ctx.report or ctx.path(BUILD_REL, "report.json")
    if ctx.src and ctx.obj:
        from tools.lib import report  # noqa: PLC0415
        reasons, _newest = report.report_reasons(path, ctx.obj, ctx.src, ctx.root, rel=ctx.rel)
        if not reasons:
            return "fresh", "the report is newer than %s and its include closure" % ctx.rel(ctx.obj)
        return ("missing" if not os.path.exists(path) else "stale"), "; ".join(reasons)
    if not os.path.exists(path):
        return "missing", "no %s" % ctx.rel(path)
    steps, note = ninja_plan(ctx, os.path.join(BUILD_REL, "report.json").replace("\\", "/"))
    if steps is None:
        return "unknown", note
    if steps:
        return "stale", _plan_reason(steps, "report.json")
    t, newest = _mtime(path), _newest_object(ctx.path(BUILD_REL, "obj"))
    if t is not None and newest is not None and newest[1] > t:
        return "stale", "a split target object is newer than the report (%s, %s > %s)" % (
            ctx.rel(newest[0]), _stamp(newest[1]), _stamp(t))
    return "fresh", "ninja -n report.json: no work, and no split target object is newer"


def _newest_object(top: str) -> tuple[str, float] | None:
    best = None
    for dirpath, _dirs, names in os.walk(top):
        for n in names:
            if n.endswith(".o"):
                p = os.path.join(dirpath, n)
                t = _mtime(p)
                if t is not None and (best is None or t > best[1]):
                    best = (p, t)
    return best


def check_slot_build(ctx: Context) -> tuple[str, str]:
    from tools.lib import repo  # noqa: PLC0415
    from tools.lib.lanes import pool  # noqa: PLC0415 - lanes imports seed, which imports this module
    main = ctx.main or repo.main_tree(ctx.root)
    n = pool.slot_of_path(main, ctx.root) if main else None
    if n is None:
        return "n/a", "not a slot (`slots.py status` lists the pool)"
    ctx.extra["slot"], ctx.extra["main"] = n, main
    v = pool.verify(main, n)
    if v["ok"]:
        return "fresh", "slot %d: split, report bytes and compile outputs agree with MAIN" % n
    return "stale", "slot %d: %s" % (n, "; ".join(v["reasons"]))


# --- the registry --------------------------------------------------------------------------------------------------

def _py(*argv: str) -> Callable[[Context], list[str]]:
    return lambda ctx: [sys.executable] + [ctx.path(*argv[0].split("/"))] + list(argv[1:])


def _ninja(target: str) -> Callable[[Context], list[str]]:
    return lambda ctx: ["ninja", target]


def _unit_ninja(default: str) -> Callable[[Context], list[str]]:
    def cmd(ctx: Context) -> list[str]:
        return ["ninja", ctx.rel(ctx.obj) if ctx.obj else default]
    return cmd


def _slot_cmd(ctx: Context) -> list[str] | None:
    n, main = ctx.extra.get("slot"), ctx.extra.get("main")
    if n is None or not main:
        return None
    return [sys.executable, os.path.join(main, "tools", "units", "slots.py"), "refresh", str(n)]


@dataclass(frozen=True)
class Artifact:
    """One derived artifact: what it is, what makes it stale, what it needs first and how it is rebuilt."""
    name: str
    what: str
    path: str
    inputs: tuple[str, ...]
    deps: tuple[str, ...]
    cost: tuple[float, float]           # measured seconds (low, high) for one rebuild
    cost_note: str
    check: Callable[[Context], tuple[str, str]]
    command: Callable[[Context], list[str] | None]
    tree_local: bool = True             # rebuilt in the tree it describes (else from MAIN)
    touches_tracked: bool = False       # the rebuild may write a tracked file: guarded by `git status`, reported

    def cost_text(self) -> str:
        lo, hi = self.cost
        return "~%g s" % hi if lo == hi else "~%g-%g s" % (lo, hi)

    def display(self, ctx: Context) -> str:
        argv = self.command(ctx)
        if not argv:
            return ""
        shown = []
        for a in argv:
            if a == sys.executable:
                shown.append("python")
            elif os.path.isabs(a):
                shown.append(ctx.rel(a))
            else:
                shown.append(a)
        return " ".join(shown)


REGISTRY: dict[str, Artifact] = {a.name: a for a in (
    Artifact("manifest", "build.ninja, objdiff.json and compile_commands.json, generated by configure.py",
             "build.ninja", MANIFEST_INPUTS, (), (0.5, 2.0), "measured 2026-10-04", check_manifest,
             _py("configure.py")),
    Artifact("split", "the split target objects (build/RMHE08/obj) and config.json, `dtk dol split`",
             "build/RMHE08/config.json", SPLIT_INPUTS, ("manifest",), (2.0, 3.0),
             "1.8-1.9 s in .ninja_log at 415 objects (docs/build-performance.md's ~18 s was 13.5 k objects)",
             check_split, _ninja("build/RMHE08/config.json"), touches_tracked=True),
    Artifact("asm-dump", "the on-demand disassembly build/RMHE08/asm (write_asm: false), stamped",
             "build/RMHE08/asm/.stamp.json", SPLIT_INPUTS[1:4], (), (4.0, 6.0),
             "4.2-6.0 s wall at 415 files, replacing the old dump (2026-10-05; 200-400 s was 13.5 k files)",
             check_asm_dump, _py("tools/splits/dump_asm.py")),
    Artifact("tudiscover-graph", "tudiscover's per-function graph cache over the dump",
             "build/tmp/tudiscover/graph.json",
             ("every dump file's name and content (sha1)", "symbols.txt (sha1)", "tudiscover's SCHEMA and parse code"),
             ("asm-dump",), (15.0, 20.0), "17 s cold, 1.9 s cached (2026-10-04)", check_tudiscover_graph,
             _py("tools/splits/tudiscover.py", "cache")),
    Artifact("callers-index", "callers' address-keyed reference index over the dump",
             "build/tmp/callers/graph.json", ("every dump file's name and content (sha1)", "lib.refs SCHEMA"),
             ("asm-dump",), (10.0, 15.0), "11.8 s cold, 1.6 s cached (2026-10-04)", check_callers_index,
             _py("tools/units/callers.py", "--stats")),
    Artifact("objects", "compiled objects build/RMHE08/src/**.o (one unit's, when a unit is named)",
             "build/RMHE08/src", ("each unit's source and in-tree include closure", "build.ninja (flags)"),
             ("manifest",), (0.2, 30.0), "0.2-24 s per unit (mwcceppc)", _objects, _unit_ninja("all_source")),
    Artifact("report", "the objdiff report build/RMHE08/report.json",
             "build/RMHE08/report.json", ("compiled objects (all_source)", "split target objects", "objdiff.json"),
             ("split", "objects"), (1.0, 4.0), "1.1-1.2 s in .ninja_log, plus any stale compile", _report,
             _ninja("build/RMHE08/report.json")),
    Artifact("slot-build", "a slot's whole build tree, seeded from MAIN and verified against it",
             "build/", ("MAIN's split, report.json and compile outputs",), (), (1.0, 10.0),
             "an incremental re-seed (see slots.py refresh)", check_slot_build, _slot_cmd, tree_local=False),
)}


def get(name: str) -> Artifact:
    try:
        return REGISTRY[name]
    except KeyError:
        raise SystemExit("unknown artifact %r (known: %s)" % (name, ", ".join(REGISTRY))) from None


def order(names: Iterable[str]) -> list[str]:
    """`names` and everything they depend on, dependencies first, each once (registry order breaks ties)."""
    seen: list[str] = []

    def visit(n: str, stack: tuple[str, ...]) -> None:
        if n in stack:
            raise ValueError("artifact dependency cycle: %s" % " -> ".join(stack + (n,)))
        if n in seen:
            return
        for d in get(n).deps:
            visit(d, stack + (n,))
        seen.append(n)

    for n in names:
        visit(n, ())
    return seen


# --- status and ensure ---------------------------------------------------------------------------------------------

def status(name: str, ctx: Context, check: Callable[[Context], tuple[str, str]] | None = None) -> Status:
    art = get(name)
    try:
        state, reason = (check or art.check)(ctx)
    except Exception as exc:                          # noqa: BLE001 - a check that cannot answer is `unknown`
        state, reason = "unknown", "the check failed (%s: %s)" % (type(exc).__name__, exc)
    return Status(name, state, reason, art.display(ctx), art.cost_text(), art.path)


def statuses(ctx: Context, names: Iterable[str] | None = None) -> list[Status]:
    return [status(n, ctx) for n in (order(names) if names else list(REGISTRY))]


def resolve_policy(policy: str | None, default: str = "refuse") -> str:
    """The explicit policy, else `$FRESH`, else the tool's default."""
    chosen = policy or os.environ.get(ENV) or default
    if chosen not in POLICIES:
        raise SystemExit("%s=%r is not a policy (one of %s)" % (ENV if not policy else "policy", chosen,
                                                                 ", ".join(POLICIES)))
    return chosen


def effective_policy(policy: str | None, default: str = "refuse", runner: Callable | None = None,
                     out=sys.stderr) -> str:
    """`resolve_policy`, except that under the fixture test tier `auto` never runs a real rebuild: with no stub
    `runner` it degrades to the tool's own default (`warn` when that default is `auto`) and says so (one line)."""
    chosen = resolve_policy(policy, default)
    if chosen == "auto" and testing.current_tier() == "fixture" and runner is None:
        fallback = "warn" if default == "auto" else default
        _announce(out, "fresh: auto refresh suppressed under the fixture test tier (%s instead)" % fallback)
        return fallback
    return chosen


def lock_path(root: str, name: str) -> str:
    return os.path.join(root, LOCK_REL, name + ".lock")


def _pid_alive(pid: int) -> bool:
    try:
        from tools.lib.lanes import sessions  # noqa: PLC0415
        return bool(sessions.pid_alive(pid))
    except Exception:                                 # noqa: BLE001
        return True


class Lock:
    """An exclusive-create lock file: one rebuild of an artifact per tree at a time. A second process waits, and a
    lock whose holder is dead is taken over (it never wedges)."""

    def __init__(self, path: str, timeout: float, poll: float = POLL_S, sleep=time.sleep, alive=_pid_alive) -> None:
        self.path, self.timeout, self.poll, self.sleep, self.alive = path, timeout, poll, sleep, alive
        self.waited = 0.0

    def __enter__(self) -> "Lock":
        os.makedirs(os.path.dirname(self.path), exist_ok=True)
        start = time.time()
        while True:
            try:
                fd = os.open(self.path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o644)
            except FileExistsError:
                holder = self._holder()
                if holder is not None and not self.alive(holder):
                    try:
                        os.unlink(self.path)
                    except OSError:
                        pass
                    continue
                self.waited = time.time() - start
                if self.waited > self.timeout:
                    raise SystemExit("REFUSED: %s is held by pid %s for over %.0f s - remove it if that process is "
                                     "gone" % (self.path, holder, self.timeout))
                self.sleep(self.poll)
                continue
            with os.fdopen(fd, "w", encoding="utf-8") as fh:
                json.dump({"pid": os.getpid(), "since": time.strftime("%Y-%m-%dT%H:%M:%S")}, fh)
            self.waited = time.time() - start
            return self

    def _holder(self) -> int | None:
        try:
            with open(self.path, encoding="utf-8") as fh:
                return int(json.load(fh).get("pid"))
        except (OSError, ValueError, TypeError, AttributeError):
            return None

    def __exit__(self, *exc) -> None:
        try:
            os.unlink(self.path)
        except OSError:
            pass


def _tracked_dirty(root: str) -> set[str]:
    """The tracked paths `git status` reports modified (the guard around a rebuild that may touch the tree)."""
    try:
        p = proc.run(["git", "-C", root, "status", "--porcelain", "-uno"], timeout=60)
    except (OSError, subprocess.SubprocessError):
        return set()
    return {ln[3:].strip() for ln in (p.stdout or "").splitlines() if len(ln) > 3}


def _announce(out, text: str) -> None:
    if out is not None:
        print(text, file=out)
        try:
            out.flush()
        except (AttributeError, OSError):
            pass


def refresh(name: str, ctx: Context, out=sys.stderr, lock_timeout: float | None = None, check=None) -> Status:
    """Rebuild one stale artifact under its lock: wait for another process's rebuild, re-check after it, run the
    command, and re-check again - the rebuild must leave it fresh, or it is a failure."""
    art = get(name)
    before = status(name, ctx, check)
    if before.fresh:
        return before
    argv = art.command(ctx)
    if not argv:
        raise RefreshFailed(before, "no refresh command applies here (%s)" % before.reason)
    timeout = lock_timeout if lock_timeout is not None else max(600.0, 4 * art.cost[1])
    with Lock(lock_path(ctx.root, name), timeout) as lock:
        again = status(name, ctx, check)
        if again.fresh:                               # another process rebuilt it while this one waited
            again.note = "rebuilt by another process (waited %.1f s)" % lock.waited
            return again
        if art.cost[1] > EXPENSIVE_S:
            _announce(out, "refreshing %s (%s): %s" % (name, art.cost_text(), again.reason))
        else:
            _announce(out, "refreshing %s: %s" % (name, again.reason))
        dirty = _tracked_dirty(ctx.root) if art.touches_tracked else set()
        env = dict(os.environ, **{ENV: "warn"})       # a rebuild command never cascades: its deps came first
        t0 = time.time()
        try:
            p = ctx.run(argv, cwd=ctx.root if art.tree_local else (ctx.extra.get("main") or ctx.root), env=env)
        except OSError as exc:
            raise RefreshFailed(again, str(exc)) from None
        took = round(time.time() - t0, 1)
        if p.returncode != 0:
            tail = [ln for ln in ((p.stdout or "") + "\n" + (p.stderr or "")).strip().splitlines() if ln.strip()]
            raise RefreshFailed(again, "exit %d after %.1f s: %s" % (p.returncode, took, " | ".join(tail[-4:])))
        after = status(name, ctx, check)
        after.refreshed, after.seconds = True, took
        if art.touches_tracked:
            touched = sorted(_tracked_dirty(ctx.root) - dirty)
            if touched:
                after.note = "the rebuild modified tracked file(s): %s" % ", ".join(touched)
                _announce(out, "WARNING: refreshing %s modified tracked file(s): %s - review them before a commit"
                          % (name, ", ".join(touched)))
        if not after.fresh:
            raise RefreshFailed(after, "ran in %.1f s and left it %s: %s" % (took, after.state, after.reason))
        return after


def ensure(names: Iterable[str], ctx: Context | str, policy: str | None = None, default: str = "refuse",
           out=sys.stderr, lock_timeout: float | None = None, checks: dict | None = None) -> list[Status]:
    """Make `names` (and their dependencies, first) usable under the policy, and return each one's status.

    `auto` rebuilds what is stale, in dependency order, each under its lock; `warn` prints one line per stale
    artifact and goes on; `refuse` raises `StaleArtifact` at the first. Fresh artifacts print nothing, so a tool's
    output is byte-identical when everything is current. Under the fixture test tier `auto` never runs a real
    rebuild (it degrades to `warn`) unless the context carries a stub runner. `checks` overrides a check by name
    (a tool that can answer in-process)."""
    if isinstance(ctx, str):
        ctx = Context(ctx)
    chosen = effective_policy(policy, default, ctx.runner, out)
    checks = checks or {}
    results: list[Status] = []
    for name in order(names):
        st = status(name, ctx, checks.get(name))
        if not st.fresh:
            if chosen == "refuse":
                raise StaleArtifact(st)
            if chosen == "warn":
                _announce(out, "WARNING: %s is %s - %s; using it as it is (%s)" % (name, st.state, st.reason,
                                                                                  st.remedy()))
            else:
                st = refresh(name, ctx, out, lock_timeout, checks.get(name))
        results.append(st)
    return results


def by_name(results: Sequence[Status], name: str) -> Status | None:
    return next((s for s in results if s.name == name), None)
