"""Trees and paths: the invocation's tree, its MAIN, the build/orig/asm/scratch/state locations, the ground truth.
Spec: docs/tools/spec/lib-repo.md. CLI: none (library)."""
from __future__ import annotations

import atexit
import os
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path
from typing import Callable

from tools.lib import testing
from tools.lib.git import Git

#: The game version every path below is spelled for.
VERSION = "RMHE08"
#: The marker of a tree: a directory holding `configure.py`.
MARKER = "configure.py"
#: A slot is a sibling of MAIN named `<main>.slot<N>`.
SLOT_RE = re.compile(r"\.slot\d+$")
#: The one list of campaign state under `.pi/`; `state()` refuses any other name.
STATE_NAMES = ("claims.json", "backlog.json", "land-base.json", "data-requests.json", "merge-state.json",
               "ledger.json", "spend.json", "state.md", "slots", "lanes", "outbox", "notes", "tasks", "bin")
#: The originals `config.yml` pins, by the key that pins each.
GROUND_TRUTH_KEYS = (("hash", "orig/%s/sys/main.dol" % VERSION), ("selfile_hash", "orig/%s/files/mh3.sel" % VERSION))

#: This file's own tree - the "packaged copy" `repo_root()` falls back to when the cwd is not a tree.
PACKAGED_ROOT = Path(__file__).resolve().parents[2]


def guard(path: str | os.PathLike, what: str) -> None:
    """Refuse (under the fixture tier) any resolution that lands in the live repository.

    The choke point for reads the audit hook cannot see: `os.stat`/`os.path.exists` probes, and roots
    computed at import time and used later. Every function here that turns a root into a path calls it.
    """
    testing.assert_path_allowed(path, what)


def _walk_to_tree(start: str) -> str | None:
    d = os.path.abspath(start)
    while True:
        if os.path.exists(os.path.join(d, MARKER)):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def caller_worktree(start: str | os.PathLike | None = None) -> str | None:
    """The git worktree the caller (or `start`) is in, as git prints it; None when git cannot say."""
    try:
        return Git(start or os.getcwd()).toplevel()
    except OSError:
        return None


def cwd_tree() -> str | None:
    """`cwd` when it is itself a tree (holds `configure.py`), else None - the non-git invocation case."""
    try:
        cwd = os.getcwd()
    except OSError:
        return None
    return cwd if os.path.exists(os.path.join(cwd, MARKER)) else None


def repo_root(start: str | os.PathLike | None = None) -> str:
    """The tree the tools should read: walk up from `start` to `configure.py`.

    Without `start`, the invocation's tree wins (its git worktree, else a `cwd` that is a tree), and only a
    `cwd` that is no tree falls back to the packaged copy. Under the fixture tier a call without `start`
    raises: a fixture names its tree. Raises `SystemExit` when no tree is found.
    """
    if start is None:
        testing.assert_live_allowed("lib.repo.repo_root() without start=")
        begin = caller_worktree() or cwd_tree() or str(PACKAGED_ROOT)
    else:
        begin = os.path.abspath(start)
    found = _walk_to_tree(begin)
    if found is None:
        raise SystemExit("repo root (configure.py) not found above %s" % (start or begin))
    guard(found, "lib.repo.repo_root(%s)" % (start or ""))
    return found


def main_tree(root: str | os.PathLike, honour_env: bool = False) -> str | None:
    """MAIN, the primary checkout `root` was cut from: the parent of `git rev-parse --git-common-dir`.

    `honour_env` lets `$MHTRI_MAIN` (when it names a directory) override it - for the tree the tools
    serve, never a fixture. None when neither answers (a fixture, a non-git copy).
    """
    root = os.path.abspath(root)
    guard(root, "lib.repo.main_tree")
    env = os.environ.get("MHTRI_MAIN")
    if honour_env and env and os.path.isdir(env):
        return os.path.abspath(env)
    try:
        out = Git(root).run("rev-parse", "--git-common-dir", timeout=20).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return None
    if not out:
        return None
    out = os.path.normpath(out if os.path.isabs(out) else os.path.join(root, out))
    return os.path.dirname(out) if os.path.basename(out) == ".git" else None


def resolve_input(rel: str, root: str | os.PathLike, probe: Callable[[str], bool] = os.path.exists,
                  honour_env: bool = False) -> str:
    """A build/orig input: `rel` under `root`, else (read-only) under MAIN when `root` has none.

    `probe` says whether a candidate is usable; `honour_env` is `main_tree`'s. When neither has it the
    tree's own path is returned, so the error names the tree's path. Never for a path a tool writes.
    """
    root = os.path.abspath(root)
    guard(root, "lib.repo.resolve_input(%s)" % rel)
    local = os.path.join(root, rel)
    if probe(local):
        return local
    main = main_tree(root, honour_env)
    if main and os.path.normcase(os.path.abspath(main)) != os.path.normcase(root):
        cand = os.path.join(main, rel)
        if probe(cand):
            return cand
    return local


_SESSION = {"dir": None}


def session_tmpdir() -> str:
    """One unique scratch directory per process, under the system temp, removed at exit."""
    if _SESSION["dir"] is None:
        _SESSION["dir"] = tempfile.mkdtemp(prefix="tools-session-")
        atexit.register(shutil.rmtree, _SESSION["dir"], ignore_errors=True)
    return _SESSION["dir"]


def scratch(tool: str, root: str | os.PathLike) -> Path:
    """`<root>/build/tmp/<tool>/`, created."""
    guard(root, "lib.repo.scratch(%s)" % tool)
    p = Path(root) / "build" / "tmp" / tool
    p.mkdir(parents=True, exist_ok=True)
    return p


def state(name: str, root: str | os.PathLike) -> Path:
    """`<root>/.pi/<name>` for a name on `STATE_NAMES` (the caller decides whether `root` is MAIN)."""
    if name not in STATE_NAMES:
        raise ValueError("%r is not campaign state (STATE_NAMES: %s)" % (name, ", ".join(STATE_NAMES)))
    guard(root, "lib.repo.state(%s)" % name)
    return Path(root) / ".pi" / name


def ground_truth(root: str | os.PathLike) -> dict[str, str]:
    """`{original's repo-relative path: lowercase sha1}` for every original `config.yml` pins."""
    guard(root, "lib.repo.ground_truth")
    cfg = Path(root) / "config" / VERSION / "config.yml"
    text = cfg.read_text(encoding="utf-8", errors="replace")
    out = {}
    for key, rel in GROUND_TRUTH_KEYS:
        m = re.search(r"^%s:\s*([0-9A-Fa-f]{40})\s*$" % key, text, re.M)
        if m:
            out[rel] = m.group(1).lower()
    return out


def verify_ground_truth(root: str | os.PathLike, want: dict[str, str] | None = None) -> list[tuple[str, str, str]]:
    """`[(path, want, got)]` for every pinned original that is missing or changed; empty means intact."""
    import hashlib
    want = ground_truth(root) if want is None else want
    bad = []
    for rel, expected in sorted(want.items()):
        p = Path(root) / rel
        got = hashlib.sha1(p.read_bytes()).hexdigest() if p.is_file() else "MISSING"
        if got != expected:
            bad.append((rel, expected, got))
    return bad


@dataclass(frozen=True)
class Tree:
    """A repository tree and the locations the tools read in it."""
    root: Path
    version: str = VERSION

    def __post_init__(self) -> None:
        object.__setattr__(self, "root", Path(os.path.abspath(self.root)))
        guard(self.root, "lib.repo.Tree(%s)" % self.root.as_posix())

    @classmethod
    def find(cls, start: str | os.PathLike | None = None) -> "Tree":
        """The tree `repo_root(start)` names."""
        return cls(Path(repo_root(start)))

    @cached_property
    def main(self) -> Path:
        """MAIN for a worktree or slot; the tree itself otherwise (or outside git)."""
        m = main_tree(self.root)
        return Path(m) if m else self.root

    @property
    def is_worktree(self) -> bool:
        """A linked git worktree: `.git` is a file."""
        return (self.root / ".git").is_file()

    @property
    def is_slot(self) -> bool:
        return self.is_worktree and bool(SLOT_RE.search(self.root.name))

    def build(self, version: str | None = None) -> Path:
        return self.root / "build" / (version or self.version)

    @property
    def obj_dir(self) -> Path:
        return self.build() / "obj"

    @property
    def src_obj_dir(self) -> Path:
        return self.build() / "src"

    @property
    def asm_dir(self) -> Path:
        return self.build() / "asm"

    @property
    def report_json(self) -> Path:
        return self.build() / "report.json"

    @property
    def orig_dol(self) -> Path:
        return self.root / "orig" / self.version / "sys" / "main.dol"

    @property
    def config_yml(self) -> Path:
        return self.root / "config" / self.version / "config.yml"

    @property
    def objdiff_json(self) -> Path:
        return self.root / "objdiff.json"

    def input(self, rel: str, probe: Callable[[str], bool] = os.path.exists) -> Path:
        """`resolve_input(rel, self.root, probe)` as a Path."""
        return Path(resolve_input(rel, self.root, probe))
