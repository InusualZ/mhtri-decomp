"""The live-lane signal: Claude Code's session registry (`<config dir>/sessions/<pid>.json`), a record counting
only while its pid is alive. Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import json
import os

SESSIONS_DIRNAME = "sessions"


def config_dir() -> str:
    """Claude Code's config directory: `$CLAUDE_CONFIG_DIR`, else `~/.claude`."""
    return os.environ.get("CLAUDE_CONFIG_DIR") or os.path.join(os.path.expanduser("~"), ".claude")


def run_registries(config: str | None = None) -> list[str]:
    """The session registry under the config dir as a one-element list; empty when there is none (no signal,
    which is not the same as "no lane")."""
    d = os.path.join(config or config_dir(), SESSIONS_DIRNAME)
    return [d] if os.path.isdir(d) else []


def _read_json(path: str) -> dict:
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            data = json.load(fh)
    except (OSError, ValueError):
        return {}
    return data if isinstance(data, dict) else {}


def pid_alive(pid) -> bool:
    """Whether a process with this pid runs. Never signals it: `os.kill(pid, 0)` on Windows would terminate it,
    so the Windows branch asks the kernel for the exit code (STILL_ACTIVE) instead."""
    if not isinstance(pid, int) or isinstance(pid, bool) or pid <= 0:
        return False
    if os.name == "nt":
        import ctypes
        kernel = ctypes.windll.kernel32
        handle = kernel.OpenProcess(0x1000, False, pid)        # PROCESS_QUERY_LIMITED_INFORMATION
        if not handle:
            return False
        code = ctypes.c_ulong()
        ok = kernel.GetExitCodeProcess(handle, ctypes.byref(code))
        kernel.CloseHandle(handle)
        return bool(ok) and code.value == 259                  # STILL_ACTIVE
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    except PermissionError:
        return True
    return True


def _record(record: dict, run_id: str, registry: str) -> dict:
    return {"run_id": record.get("sessionId") or run_id,
            "cwd": record.get("cwd") or "",
            "state": "running",
            "agent": record.get("agent") or record.get("kind") or "",
            "session": record.get("name") or "",
            "pid": record.get("pid"),
            "registry": registry}


def live_runs(registry: str | None = None, config: str | None = None) -> list[dict]:
    """Every live session from `<registry>/<pid>.json`; a record whose pid is dead (a crash) is skipped, and an
    unreadable record is skipped rather than guessed at."""
    runs = []
    for root in ([registry] if registry else run_registries(config)):
        try:
            names = sorted(os.listdir(root))
        except OSError:
            continue
        for name in names:
            if not name.endswith(".json"):
                continue
            record = _read_json(os.path.join(root, name))
            if not record or not pid_alive(record.get("pid")):
                continue
            runs.append(_record(record, name[:-5], root))
    return runs


def same_tree(a: str, b: str) -> bool:
    """Whether path `a` is `b` or sits inside it (case-insensitive on Windows, separator-agnostic)."""
    if not a or not b:
        return False
    x, y = os.path.normcase(os.path.abspath(a)), os.path.normcase(os.path.abspath(b))
    return x == y or x.startswith(y.rstrip(os.sep) + os.sep)


def runs_in(path: str, registry: str | None = None, config: str | None = None,
            runs: list[dict] | None = None) -> list[dict]:
    """Every live session whose cwd resolves into `path` (inside, not only equal)."""
    pool = live_runs(registry, config) if runs is None else runs
    return [run for run in pool if same_tree(run.get("cwd"), path)]


def run_label(run: dict) -> str:
    """`<run id> (<session name>)` - how a refusal names the run it protects."""
    name = (run.get("session") or "").strip()
    return "%s%s" % (run.get("run_id") or "?", " (%s)" % name[:60] if name else "")
