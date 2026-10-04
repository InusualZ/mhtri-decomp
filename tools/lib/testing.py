"""The test harness: Checker, FixtureTree, GitFixture, the fixture/smoke tiers and the live-tree guard.
Spec: docs/tools/spec/lib-testing.md. CLI: none (a test module ends with `raise SystemExit(testing.run(globals()))`)."""
from __future__ import annotations

import io
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import traceback
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterable

#: The two tiers. A test module declares `TIER = "fixture"` (the default) or `TIER = "smoke"`.
TIERS = ("fixture", "smoke")
#: The environment variable that carries the tier into every child process a test starts, so a tool run
#: against a fixture inherits the refusal (`assert_live_allowed`) as well.
TIER_ENV = "TOOLS_TEST_TIER"
#: The repository this harness lives in: the "live tree" a fixture-tier test must never read.
LIVE_ROOT = Path(__file__).resolve().parents[2]


class LiveTreeError(RuntimeError):
    """A fixture-tier test read (or ran a process in) the live repository."""


_STATE: dict[str, Any] = {"tier": None, "guard": False, "violations": [], "in_hook": False}


def current_tier() -> str | None:
    """The tier of the running test: the one `run()` set, else the inherited `TOOLS_TEST_TIER`, else None."""
    return _STATE["tier"] or os.environ.get(TIER_ENV) or None


def set_tier(tier: str) -> None:
    """Make `tier` the tier of this process and of every child it starts."""
    if tier not in TIERS:
        raise ValueError("TIER must be one of %s, not %r" % (", ".join(TIERS), tier))
    _STATE["tier"] = tier
    os.environ[TIER_ENV] = tier


def violations() -> list[str]:
    """Every live-tree access refused so far in this process (kept even if the test swallowed the error)."""
    return list(_STATE["violations"])


def _refuse(what: str) -> None:
    msg = ("%s: a fixture-tier test may not touch the live tree %s - build a FixtureTree/GitFixture, "
           "or declare TIER = \"smoke\"" % (what, LIVE_ROOT.as_posix()))
    _STATE["violations"].append(msg)
    raise LiveTreeError(msg)


def assert_live_allowed(what: str) -> None:
    """Raise `LiveTreeError` when the running test is fixture-tier.

    The seam for every resolver of the live tree: `lib.repo.repo_root()` called without `start=` calls this
    (design.md section 7), so a fixture test cannot resolve the real repository even through a tool.
    """
    if current_tier() == "fixture":
        _refuse(what)


def live_root() -> Path:
    """The live repository root, for a smoke-tier test; refused under the fixture tier."""
    assert_live_allowed("testing.live_root()")
    return LIVE_ROOT


def assert_path_allowed(path: Any, what: str) -> None:
    """Raise `LiveTreeError` when the running test is fixture-tier and `path` lies in the live repository.

    The seam for what the audit hook cannot see (`os.stat`/`os.path.exists`, a root bound at import time
    and used later): `lib.repo` calls it on every root it resolves or is handed.
    """
    if current_tier() == "fixture" and _live_parts(path) is not None:
        _refuse(what)


class refusals_expected:
    """`with testing.refusals_expected():` - for a test OF a refusal: the refusals raised inside the block are
    the point of the test, so they are not counted against the run. Greppable on purpose."""

    def __enter__(self) -> "refusals_expected":
        self._mark = len(_STATE["violations"])
        return self

    def __exit__(self, *exc: Any) -> None:
        self.refused = _STATE["violations"][self._mark:]
        del _STATE["violations"][self._mark:]


# --- the live-tree guard (an audit hook) ------------------------------------------------------------------

def _norm(path: Any) -> str | None:
    if isinstance(path, int) or path is None:
        return None
    try:
        p = os.fsdecode(path)
    except TypeError:
        return None
    return os.path.normcase(os.path.abspath(p))


def _live_parts(path: Any) -> tuple[str, ...] | None:
    """The path's parts relative to the live root, or None when it is outside it."""
    p = _norm(path)
    if p is None:
        return None
    root = os.path.normcase(str(LIVE_ROOT))
    if p != root and not p.startswith(root + os.sep):
        return None
    rel = p[len(root):].lstrip(os.sep)
    return tuple(x for x in rel.split(os.sep) if x)


def _is_code(parts: tuple[str, ...]) -> bool:
    """Reading the tools' own code is how a test imports what it tests; it is not a live-data read."""
    if not parts or parts[0] != "tools":
        return False
    return parts[-1].endswith((".py", ".pyc")) or "__pycache__" in parts


def _is_listable(parts: tuple[str, ...]) -> bool:
    """The import system lists `sys.path` directories: the root (the prologue's entry) and `tools/**`."""
    return parts == () or (parts[0] == "tools" and not parts[-1].endswith((".json", ".txt", ".md")))


def _argv_items(argv: Any) -> list:
    """The arguments of a process start. On Windows the audit event carries the command line as ONE string
    (`list2cmdline` has run), so it is split back into its (unquoted) tokens; a list is taken as it is."""
    if isinstance(argv, (str, bytes)):
        try:
            return [t[1:-1] if len(t) > 1 and t[0] == t[-1] == '"' else t
                    for t in shlex.split(os.fsdecode(argv), posix=False)]
        except ValueError:
            return [os.fsdecode(argv)]
    return list(argv or ())


def _hook(event: str, args: tuple) -> None:
    if not _STATE["guard"] or _STATE["in_hook"]:
        return
    _STATE["in_hook"] = True
    try:
        if event == "open":
            parts = _live_parts(args[0])
            if parts is not None and not _is_code(parts):
                _refuse("open(%s)" % "/".join(parts))
        elif event in ("os.listdir", "os.scandir"):
            target = args[0] if args and args[0] is not None else "."
            parts = _live_parts(target)
            if parts is not None and not _is_listable(parts):
                _refuse("%s(%s)" % (event, "/".join(parts)))
        elif event == "os.chdir":
            parts = _live_parts(args[0])
            if parts is not None:
                _refuse("os.chdir(%s)" % "/".join(parts))
        elif event == "subprocess.Popen":
            _executable, argv, cwd, _env = args
            parts = _live_parts(cwd if cwd is not None else os.getcwd())
            if parts is not None:
                _refuse("a process started in %s" % ("/".join(parts) or "the repository root"))
            for item in _argv_items(argv):
                if isinstance(item, (str, bytes, os.PathLike)) and os.path.isabs(os.fsdecode(item)):
                    parts = _live_parts(item)
                    if parts is not None and not _is_code(parts):
                        _refuse("a process given %s" % ("/".join(parts) or "the repository root"))
    finally:
        _STATE["in_hook"] = False


def install_guard() -> None:
    """Refuse, for the rest of this process, every open/list/chdir/process start inside the live tree.

    Reading `tools/**/*.py` (imports) and listing `tools/` and the root (the import system) stay allowed.
    An audit hook cannot be removed, so the guard is switched by state; `run()` turns it on for the fixture
    tier. What it cannot see: `os.stat`/`os.path.exists` (not audited) - a resolver that only *computes* a
    live path is refused by `assert_live_allowed`, not by the hook.
    """
    if not _STATE.get("hook_added"):
        sys.addaudithook(_hook)
        _STATE["hook_added"] = True
    _STATE["guard"] = True


# --- Checker ----------------------------------------------------------------------------------------------

class Checker:
    """Counts checks and prints one shape: `FAIL: <name>: <detail>` per failure, then the summary line
    `ok - N checks` (or `FAIL - M of N checks failed`), which `tools/selftest.py`'s count parser reads."""

    def __init__(self, name: str = "", out: Any = None) -> None:
        self.name = name
        self.out = out
        self.count = 0
        self.failures: list[str] = []
        self.skipped: list[str] = []

    def _print(self, text: str) -> None:
        print(text, file=self.out or sys.stdout)

    def fail(self, name: str, detail: str) -> bool:
        self.count += 1
        self.failures.append("%s: %s" % (name, detail))
        self._print("FAIL: %s: %s" % (name, detail))
        return False

    def check(self, name: str, got: Any, want: Any) -> bool:
        """`got == want`."""
        if got == want:
            self.count += 1
            return True
        return self.fail(name, "got %r != want %r" % (got, want))

    def expect(self, name: str, cond: Any, detail: str = "") -> bool:
        """A truthy `cond`."""
        if cond:
            self.count += 1
            return True
        return self.fail(name, detail or "expected a true value, got %r" % (cond,))

    def contains(self, name: str, haystack: Any, needle: Any) -> bool:
        """`needle in haystack`."""
        try:
            hit = needle in haystack
        except TypeError as exc:
            return self.fail(name, "cannot test membership: %s" % exc)
        if hit:
            self.count += 1
            return True
        shown = repr(haystack)
        if len(shown) > 400:
            shown = shown[:400] + "..."
        return self.fail(name, "%r not in %s" % (needle, shown))

    def raises(self, name: str, exc: type[BaseException] | tuple, fn: Callable, *args: Any,
               **kwargs: Any) -> BaseException | None:
        """`fn(*args, **kwargs)` raises `exc`; returns the exception (None when the check failed)."""
        try:
            fn(*args, **kwargs)
        except exc as caught:  # noqa: BLE001 - the point of the check
            self.count += 1
            return caught
        except Exception as other:  # noqa: BLE001
            self.fail(name, "raised %s: %s, not %s" % (type(other).__name__, other, exc))
            return None
        self.fail(name, "did not raise %s" % (exc,))
        return None

    def skip(self, name: str, reason: str) -> None:
        """A smoke check whose input is absent: reported, never counted, never a failure."""
        self.skipped.append(name)
        self._print("skip - %s: %s" % (name, reason))

    def summary(self) -> int:
        """Print the summary line; 0 when every check passed, 1 otherwise."""
        if self.failures:
            self._print("FAIL - %d of %d checks failed" % (len(self.failures), self.count))
            return 1
        self._print("ok - %d checks" % self.count)
        return 0


def run(ns: dict, argv: Iterable[str] | None = None) -> int:
    """Run a test module: set its tier, guard the fixture tier, call every `test_*(checker)` in order.

    `ns` is the module's `globals()`. An exception in one test is a failure of that test, not of the run.
    A live-tree access refused under the fixture tier fails the run even when the test swallowed it.
    """
    tier = ns.get("TIER", "fixture")
    set_tier(tier)
    if tier == "fixture":
        install_guard()
    c = Checker(os.path.basename(str(ns.get("__file__", "test"))))
    tests = [(n, f) for n, f in list(ns.items()) if n.startswith("test_") and callable(f)]
    for name, fn in tests:
        before = len(_STATE["violations"])
        try:
            fn(c)
        except Exception as exc:  # noqa: BLE001 - one broken test must not hide the others
            tail = traceback.format_exc().strip().splitlines()[-6:]
            c.fail(name, "raised %s: %s\n    %s" % (type(exc).__name__, exc, "\n    ".join(tail)))
        for msg in _STATE["violations"][before:]:
            c.fail(name, "live-tree access: " + msg)
    _STATE["guard"] = False
    return c.summary()


# --- the allow-lists a test shrinks (`--prune`) ---------------------------------------------------------------

def json_indent(text: str, default: int = 1) -> int:
    """The indent unit a JSON document was written with: the leading spaces of its first indented line."""
    for line in text.splitlines()[1:]:
        stripped = line.lstrip(" ")
        if stripped and len(stripped) < len(line):
            return len(line) - len(stripped)
    return default


def rewrite_json(path: str | os.PathLike, data: Any) -> None:
    """Write `data` back to an existing JSON file in that file's own shape - its indent unit, raw non-ASCII kept
    raw when the file already has it, LF and a trailing newline - so a `--prune` is a minimal diff (the dropped
    lines and at most one moved comma), never a reformat of the whole file."""
    p = Path(path)
    old = p.read_text(encoding="utf-8") if p.exists() else ""
    raw = any(ord(ch) > 127 for ch in old)
    p.write_text(json.dumps(data, indent=json_indent(old), ensure_ascii=not raw) + "\n", encoding="utf-8",
                 newline="\n")


# --- FixtureTree ------------------------------------------------------------------------------------------

SECTIONS = (".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data", ".bss", ".sdata",
            ".sbss", ".sdata2", ".sbss2")
_SECTION_TYPES = {".init": "code", ".text": "code", ".data": "data", ".sdata": "data", ".bss": "bss",
                  ".sbss": "bss", ".sbss2": "bss"}


def _outside_live(path: Path, what: str) -> Path:
    p = Path(os.path.abspath(path))
    if _live_parts(p) is not None:
        raise ValueError("%s %s is inside the live repository %s - fixtures live under the system temp"
                         % (what, p.as_posix(), LIVE_ROOT.as_posix()))
    return p


@dataclass(frozen=True)
class FixtureObject:
    """One `Object(flag, path)` row of the fixture's `configure.py`."""
    path: str
    flag: str = "NonMatching"


@dataclass(frozen=True)
class FixtureSymbol:
    """One `symbols.txt` row."""
    name: str
    section: str
    address: int
    size: int
    type: str = "function"
    scope: str = "global"

    def line(self) -> str:
        return "%s = %s:0x%08X; // type:%s size:0x%X scope:%s" % (
            self.name, self.section, self.address, self.type, self.size, self.scope)


class FixtureTree:
    """A fake repository the tools accept as a tree, always created outside the live repository.

    Layout: `configure.py` (real, runnable Python with `config.libs` and `Object(...)` rows),
    `config/RMHE08/{symbols.txt,splits.txt,config.yml}`, `src/`, `include/`, `build/RMHE08/{obj,src}/`,
    and on demand `build/RMHE08/report.json` and `build/RMHE08/asm/`. Every helper re-renders the file it
    owns, so the files always reflect the calls made.
    """

    VERSION = "RMHE08"

    def __init__(self, tmp: str | os.PathLike | None = None) -> None:
        self._owned = tmp is None
        base = Path(tempfile.mkdtemp(prefix="fixture-tree-")) if tmp is None else Path(tmp)
        self.root = _outside_live(base, "FixtureTree")
        self.root.mkdir(parents=True, exist_ok=True)
        self.libs: dict[str, dict] = {}
        self.cflags: dict[str, list[str]] = {"cflags_base": ["-O4,p", "-inline auto"]}
        self.symbols: list[FixtureSymbol] = []
        self.splits: dict[str, list[tuple[str, int, int]]] = {}
        for d in ("src", "include", "build/%s/obj" % self.VERSION, "build/%s/src" % self.VERSION):
            (self.root / d).mkdir(parents=True, exist_ok=True)
        self.write(self.config_dir / "config.yml",
                   "object: orig/%s/sys/main.dol\nsymbols: config/%s/symbols.txt\nsplits: config/%s/splits.txt\n"
                   % (self.VERSION, self.VERSION, self.VERSION))
        self._render()

    # paths
    @property
    def config_dir(self) -> Path:
        return self.root / "config" / self.VERSION

    @property
    def build_dir(self) -> Path:
        return self.root / "build" / self.VERSION

    def path(self, rel: str | os.PathLike) -> Path:
        p = Path(rel)
        return p if p.is_absolute() else self.root / p

    def write(self, rel: str | os.PathLike, data: str | bytes) -> Path:
        """Write a file (LF, UTF-8 for text) under the tree, creating directories."""
        p = self.path(rel)
        p.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(data, bytes):
            p.write_bytes(data)
        else:
            p.write_text(data, encoding="utf-8", newline="\n")
        return p

    def read(self, rel: str | os.PathLike) -> str:
        return self.path(rel).read_text(encoding="utf-8")

    # registrations
    def add_cflags(self, group: str, flags: list[str]) -> None:
        self.cflags[group] = list(flags)
        self._render()

    def add_unit(self, path: str, lib: str = "game", flag: str = "NonMatching", mw_version: str = "Wii/1.3",
                 cflags: str = "cflags_base", ranges: dict[str, tuple[int, int]] | None = None,
                 source: str | None = None) -> FixtureObject:
        """Register `path` (e.g. `"Dir/file.c"`) in `configure.py`, claim its `ranges`, write its source."""
        if cflags not in self.cflags:
            raise ValueError("unknown cflags group %r (add_cflags first)" % cflags)
        entry = self.libs.setdefault(lib, {"mw_version": mw_version, "cflags": cflags, "objects": []})
        obj = FixtureObject(path, flag)
        entry["objects"] = [o for o in entry["objects"] if o.path != path] + [obj]
        for section, (start, end) in (ranges or {}).items():
            self._claim(path, section, start, end)
        if source is None:
            source = "/* %s - fixture unit written by FixtureTree. */\n" % path
        self.write(Path("src") / path, source)
        self._render()
        return obj

    def add_symbol(self, name: str, section: str, address: int, size: int, type: str = "function",
                   scope: str = "global") -> FixtureSymbol:
        """Add one map row (replacing a row of the same name)."""
        sym = FixtureSymbol(name, section, address, size, type, scope)
        self.symbols = [s for s in self.symbols if s.name != name] + [sym]
        self._render()
        return sym

    def claim(self, unit: str, section: str, start: int, end: int) -> None:
        """Add one `splits.txt` range to `unit`'s block."""
        self._claim(unit, section, start, end)
        self._render()

    def _claim(self, unit: str, section: str, start: int, end: int) -> None:
        if section not in SECTIONS:
            raise ValueError("unknown section %r" % section)
        if end <= start:
            raise ValueError("empty range 0x%X..0x%X" % (start, end))
        rows = [r for r in self.splits.setdefault(unit, []) if r[0] != section]
        self.splits[unit] = rows + [(section, start, end)]

    # build artefacts
    def add_object(self, rel: str, data: Any, side: str = "obj") -> Path:
        """Place an object under `build/RMHE08/obj` (the target) or `build/RMHE08/src` (ours); `data` is the
        object's bytes or a builder with `build()` (`lib.binary.build.ElfBuilder`)."""
        if side not in ("obj", "src"):
            raise ValueError("side is 'obj' (target) or 'src' (ours), not %r" % side)
        if hasattr(data, "build"):
            data = data.build()
        return self.write(self.build_dir / side / rel, data)

    def set_report(self, units: dict[str, dict]) -> Path:
        """Write `build/RMHE08/report.json` in objdiff's shape.

        `units` maps a unit name to `{"fuzzy_match_percent": float, "total_code": int, "functions":
        {name: (size, percent_or_None)}}`; a function whose percent is None carries no
        `fuzzy_match_percent` key (the report's 0 % convention).
        """
        out = []
        for name, u in units.items():
            funcs = []
            for fname, (size, pct) in u.get("functions", {}).items():
                row: dict[str, Any] = {"name": fname, "size": str(size)}
                if pct is not None:
                    row["fuzzy_match_percent"] = pct
                funcs.append(row)
            out.append({"name": name, "measures": {"fuzzy_match_percent": u.get("fuzzy_match_percent", 0.0),
                                                   "total_code": str(u.get("total_code", 0))},
                        "functions": funcs})
        return self.write(self.build_dir / "report.json", json.dumps({"units": out}, indent=1) + "\n")

    def add_asm(self, rel: str, text: str) -> Path:
        """Place an asm-dump file under `build/RMHE08/asm/`."""
        return self.write(self.build_dir / "asm" / rel, text)

    # rendering
    def _render(self) -> None:
        self.write("configure.py", self.configure_text())
        self.write(self.config_dir / "symbols.txt", self.symbols_text())
        self.write(self.config_dir / "splits.txt", self.splits_text())

    def configure_text(self) -> str:
        lines = ['"""Fixture configure.py written by tools/lib/testing.FixtureTree (not a real build)."""',
                 "Matching = True", "NonMatching = False", "Equivalent = False", "", "",
                 "class _Config:", "    pass", "", "", "config = _Config()",
                 'config.version = "%s"' % self.VERSION, "", "",
                 "def Object(completed, name, **options):",
                 '    return {"completed": completed, "name": name, **options}', "", ""]
        for group, flags in self.cflags.items():
            lines.append("%s = [%s]" % (group, ", ".join(json.dumps(f) for f in flags)))
        lines += ["", "config.libs = ["]
        for lib, entry in self.libs.items():
            lines += ["    {", '        "lib": %s,' % json.dumps(lib),
                      '        "mw_version": %s,' % json.dumps(entry["mw_version"]),
                      '        "cflags": %s,' % entry["cflags"], '        "progress_category": "game",',
                      '        "objects": [']
            lines += ['            Object(%s, %s),' % (o.flag, json.dumps(o.path)) for o in entry["objects"]]
            lines += ["        ],", "    },"]
        lines.append("]")
        return "\n".join(lines) + "\n"

    def symbols_text(self) -> str:
        rows = sorted(self.symbols, key=lambda s: (s.address, s.name))
        return "".join(s.line() + "\n" for s in rows)

    def splits_text(self) -> str:
        out = ["Sections:"]
        for s in SECTIONS:
            out.append("\t%-11s type:%s align:4" % (s, _SECTION_TYPES.get(s, "rodata")))
        for unit, rows in self.splits.items():
            out += ["", "%s:" % unit]
            order = {s: i for i, s in enumerate(SECTIONS)}
            for section, start, end in sorted(rows, key=lambda r: order[r[0]]):
                out.append("\t%-11s start:0x%08X end:0x%08X" % (section, start, end))
        return "\n".join(out) + "\n"

    # lifetime
    def cleanup(self) -> None:
        shutil.rmtree(self.root, ignore_errors=True)

    def __enter__(self) -> "FixtureTree":
        return self

    def __exit__(self, *exc: Any) -> None:
        if self._owned:
            self.cleanup()


# --- GitFixture -------------------------------------------------------------------------------------------

def _git_env() -> dict[str, str]:
    """A git environment that sees neither the caller's repository nor its global/system configuration."""
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    env.update(GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_NOSYSTEM="1", GIT_TERMINAL_PROMPT="0")
    return env


class GitFixture:
    """A throwaway git repository outside the live tree, for the landing/merge/lane tests."""

    IDENTITY = ("-c", "user.name=fixture", "-c", "user.email=fixture@example.invalid", "-c",
                "commit.gpgsign=false", "-c", "core.autocrlf=false", "-c", "init.defaultBranch=main")

    def __init__(self, tmp: str | os.PathLike | None = None) -> None:
        self._owned = tmp is None
        base = Path(tempfile.mkdtemp(prefix="git-fixture-")) if tmp is None else Path(tmp)
        self.root = _outside_live(base, "GitFixture")
        self.root.mkdir(parents=True, exist_ok=True)

    def run(self, *args: str, cwd: str | os.PathLike | None = None) -> subprocess.CompletedProcess:
        """Run git in the fixture (or `cwd`) with the fixture identity; never raises on a git failure."""
        return subprocess.run(["git", *self.IDENTITY, *args], cwd=str(cwd or self.root), env=_git_env(),
                              capture_output=True, text=True, encoding="utf-8", errors="replace")

    def git(self, *args: str, cwd: str | os.PathLike | None = None, check: bool = True) -> str:
        """Run git in the fixture (or `cwd`); return stdout. A failure raises with git's stderr."""
        p = self.run(*args, cwd=cwd)
        if check and p.returncode != 0:
            raise RuntimeError("git %s failed (%d): %s" % (" ".join(args), p.returncode, p.stderr.strip()))
        return p.stdout

    def init(self, branch: str = "main") -> "GitFixture":
        self.git("init", "-q", "-b", branch)
        return self

    def commit(self, files: dict[str, str | bytes | None], message: str) -> str:
        """Write (`None` deletes) and commit `files`; return the new commit id."""
        for rel, data in files.items():
            p = self.root / rel
            if data is None:
                if p.exists():
                    p.unlink()
                self.git("rm", "-q", "--cached", "--ignore-unmatch", rel)
                continue
            p.parent.mkdir(parents=True, exist_ok=True)
            if isinstance(data, bytes):
                p.write_bytes(data)
            else:
                p.write_text(data, encoding="utf-8", newline="\n")
            self.git("add", "--", rel)
        self.git("commit", "-q", "--allow-empty", "-m", message)
        return self.head()

    def head(self, ref: str = "HEAD") -> str:
        return self.git("rev-parse", ref).strip()

    def current_branch(self) -> str:
        return self.git("rev-parse", "--abbrev-ref", "HEAD").strip()

    def branch(self, name: str, start: str = "HEAD", checkout: bool = False) -> str:
        self.git("branch", name, start)
        if checkout:
            self.checkout(name)
        return name

    def checkout(self, ref: str) -> None:
        self.git("checkout", "-q", ref)

    def worktree(self, path: str | os.PathLike, branch: str, new: bool = True) -> Path:
        """Add a worktree at `path` (outside the live tree) on `branch` (created when `new`)."""
        wt = _outside_live(Path(path), "worktree")
        if new:
            self.git("worktree", "add", "-q", "-b", branch, str(wt))
        else:
            self.git("worktree", "add", "-q", str(wt), branch)
        return wt

    def conflict(self, path: str, a: str, b: str, base: str = "base\n",
                 names: tuple[str, str] = ("conflict-a", "conflict-b")) -> tuple[str, str]:
        """Two branches that change `path` differently from a common `base` commit; merging one into the
        other conflicts. Returns the branch names; the current branch is left where it was."""
        here = self.current_branch()
        self.commit({path: base}, "base for %s" % path)
        for name, text in zip(names, (a, b)):
            self.branch(name, checkout=True)
            self.commit({path: text}, "%s edits %s" % (name, path))
            self.checkout(here)
        return names

    def cleanup(self) -> None:
        def onexc(func: Callable, p: str, _exc: BaseException) -> None:
            try:
                os.chmod(p, 0o700)
                func(p)
            except OSError:
                pass
        shutil.rmtree(self.root, onexc=onexc)

    def __enter__(self) -> "GitFixture":
        return self

    def __exit__(self, *exc: Any) -> None:
        if self._owned:
            self.cleanup()


def rmtree_retry(path: str | os.PathLike, attempts: int = 8, sleep: Callable[[float], None] | None = None) -> None:
    """`shutil.rmtree` that survives Windows holding a file for a moment: retries with a growing backoff, clears
    the read-only bit git sets on `.git/objects`, and finally gives up silently (a leftover temp directory must
    never fail a test)."""
    import time
    sleep = sleep or time.sleep

    def onexc(func, p, _exc):
        try:
            os.chmod(p, 0o700)
            func(p)
        except OSError:
            pass
    for attempt in range(attempts):
        if not os.path.exists(path):
            return
        try:
            shutil.rmtree(path, onexc=onexc)
        except OSError:
            pass
        if not os.path.exists(path):
            return
        sleep(0.1 * (attempt + 1))
    shutil.rmtree(path, ignore_errors=True)


class temp_dir:
    """`tempfile.TemporaryDirectory()` with `rmtree_retry` cleanup: `with testing.temp_dir() as tmp:`."""

    def __init__(self, prefix: str = "mhtri-") -> None:
        self.name = tempfile.mkdtemp(prefix=prefix)

    def __enter__(self) -> str:
        return self.name

    def __exit__(self, *exc: Any) -> None:
        rmtree_retry(self.name)

    def cleanup(self) -> None:
        rmtree_retry(self.name)


def isolate_live_state() -> str:
    """Point `CLAUDE_CONFIG_DIR` at an empty temp dir for the rest of this process and return it, so a check of
    a slot guard never sees the live lanes in `~/.claude/sessions`."""
    import atexit
    path = tempfile.mkdtemp(prefix="claude-config-")
    os.environ["CLAUDE_CONFIG_DIR"] = path
    atexit.register(rmtree_retry, path)
    return path


def capture(fn: Callable, *args: Any, **kwargs: Any) -> tuple[Any, str]:
    """Call `fn` with stdout captured; return (result, text). For checking what a function prints."""
    buf, real = io.StringIO(), sys.stdout
    sys.stdout = buf
    try:
        result = fn(*args, **kwargs)
    finally:
        sys.stdout = real
    return result, buf.getvalue()
