"""Subprocess with the codec rule, the Windows spawn retry and the process-tree kill.
Spec: docs/tools/spec/lib-proc.md. CLI: none (library)."""
from __future__ import annotations

import ast
import os
import subprocess
import time
from typing import Any, Callable, Sequence

#: The pair every text-mode subprocess call passes: the codec is said, never inherited from the locale.
TEXT_KWARGS = {"encoding": "utf-8", "errors": "replace"}
#: The keyword names that turn a subprocess call into text mode.
TEXT_KEYWORDS = ("text", "universal_newlines")
#: Launch attempts and the backoff step of the WinError 5 retry (sleeps 0.1, 0.2, ... between attempts).
SPAWN_ATTEMPTS = 6
SPAWN_BACKOFF_S = 0.1
#: Trees `trap_sites` never scans: not this repository's code to fix.
VENDORED = ("tools/m2c/", "tools/mwcc-debugger/")

Completed = subprocess.CompletedProcess


def run(args: Sequence[Any], cwd: str | os.PathLike | None = None, check: bool = False, input: str | None = None,
        timeout: float | None = None, **kwargs: Any) -> subprocess.CompletedProcess:
    """`subprocess.run` in text mode with UTF-8 and replacement, capturing output unless told otherwise.

    Extra keywords (`env`, `stdout`, `stderr`, ...) pass through; `capture_output` defaults to True and is
    dropped when the caller routes `stdout`/`stderr` itself.
    """
    if "stdout" not in kwargs and "stderr" not in kwargs:
        kwargs.setdefault("capture_output", True)
    for key, value in TEXT_KWARGS.items():
        kwargs.setdefault(key, value)
    return subprocess.run(list(args) if not isinstance(args, (str, bytes)) else args, cwd=cwd, check=check,
                          input=input, timeout=timeout, **kwargs)


def run_bytes(args: Sequence[Any], cwd: str | os.PathLike | None = None, check: bool = False,
              input: bytes | None = None, timeout: float | None = None, **kwargs: Any) -> subprocess.CompletedProcess:
    """`subprocess.run` in binary mode (stdout/stderr are bytes), capturing output unless told otherwise."""
    if "stdout" not in kwargs and "stderr" not in kwargs:
        kwargs.setdefault("capture_output", True)
    return subprocess.run(list(args) if not isinstance(args, (str, bytes)) else args, cwd=cwd, check=check,
                          input=input, timeout=timeout, **kwargs)


def kill_tree(proc: subprocess.Popen) -> None:
    """Kill `proc` and every process it started (`taskkill /T` on Windows, the process group elsewhere)."""
    if os.name == "nt":
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
    else:
        try:
            os.killpg(os.getpgid(proc.pid), 9)
        except OSError:
            proc.kill()


# --- the spawn retry ----------------------------------------------------------------------------------------

_SPAWN = {"installed": False}


def is_transient(exc: BaseException) -> bool:
    """A launch refusal Windows lifts a moment later: `PermissionError` with `winerror == 5`."""
    return isinstance(exc, PermissionError) and getattr(exc, "winerror", None) == 5


def retrying(init: Callable, sleep: Callable[[float], Any] = time.sleep, attempts: int = SPAWN_ATTEMPTS,
             backoff: float = SPAWN_BACKOFF_S) -> Callable:
    """`init` (a `Popen.__init__`-shaped callable) with a transient WinError 5 retried, then raised."""
    def wrapper(self, *args, **kwargs):
        for attempt in range(attempts):
            try:
                return init(self, *args, **kwargs)
            except PermissionError as exc:
                if not is_transient(exc) or attempt == attempts - 1:
                    raise
                sleep(backoff * (attempt + 1))
    wrapper.__wrapped_by_spawnretry__ = True
    return wrapper


def install_spawn_retry() -> bool:
    """Patch `subprocess.Popen.__init__` once (Windows only); True when this call installed it."""
    if os.name != "nt" or _SPAWN["installed"] or getattr(subprocess.Popen.__init__, "__wrapped_by_spawnretry__",
                                                         False):
        return False
    subprocess.Popen.__init__ = retrying(subprocess.Popen.__init__)
    _SPAWN["installed"] = True
    return True


# --- the trap scan ------------------------------------------------------------------------------------------

def _is_text_on(node: ast.AST) -> bool:
    return isinstance(node, ast.Constant) and node.value is True


def call_traps(node: ast.AST) -> str | None:
    """Why `node` (an `ast.Call`) breaks the codec rule, or None. A `**` splat is not claimed either way."""
    if not isinstance(node, ast.Call):
        return None
    keywords = {kw.arg: kw.value for kw in node.keywords if kw.arg}
    if any(kw.arg is None for kw in node.keywords) or "kwargs" in keywords:
        return None
    text_kw = [name for name in TEXT_KEYWORDS if name in keywords and _is_text_on(keywords[name])]
    if not text_kw or "encoding" in keywords:
        return None
    return "text mode (%s=True) without an explicit encoding=" % "/".join(text_kw)


def module_traps(tree: ast.AST) -> list[tuple[int, str]]:
    """`[(line, reason)]` for every text-mode call without a codec in one parsed module."""
    found = []
    for node in ast.walk(tree):
        reason = call_traps(node)
        if reason:
            found.append((getattr(node, "lineno", 0), reason))
    return sorted(found)


def trap_sites(root: str | os.PathLike, subdir: str = "tools", skip_selftests: bool = False) -> list[str]:
    """`["<path>:<line>: <reason>"]` for every breaking call under `root/<subdir>` (vendored trees skipped)."""
    out: list[str] = []
    base = os.path.join(os.fspath(root), subdir)
    for dirpath, dirnames, filenames in os.walk(base):
        dirnames[:] = [d for d in dirnames if d != "__pycache__"]
        for name in sorted(filenames):
            if not name.endswith(".py") or (skip_selftests and name.endswith("_selftest.py")):
                continue
            path = os.path.join(dirpath, name)
            rel = path.replace("\\", "/")
            if any("/%s" % v in rel or rel.endswith("/" + v.rstrip("/")) for v in VENDORED):
                continue
            try:
                with open(path, encoding="utf-8", errors="replace") as fh:
                    tree = ast.parse(fh.read(), filename=path)
            except (OSError, SyntaxError) as exc:
                out.append("%s: unreadable by the scanner: %s" % (rel, exc))
                continue
            for line, reason in module_traps(tree):
                out.append("%s:%d: %s" % (rel, line, reason))
    return out
