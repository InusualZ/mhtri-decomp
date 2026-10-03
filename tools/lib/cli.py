"""The tool entry point: a parser with the common flags, the exit policy, JSON output and the selftest registry.
Spec: docs/tools/spec/lib-cli.md. CLI: none (library)."""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import traceback
from typing import Any, Callable, Iterable, Sequence

from tools.lib import findings as _findings

#: The common flags a tool may take, by option name.
COMMON = ("json", "root", "main", "limit", "dry_run", "quiet")
_FLAGS = {
    "json": (("--json",), {"action": "store_true", "help": "machine-readable output"}),
    "root": (("--root",), {"default": ".", "help": "the repository root to read (default: the working directory)"}),
    "main": (("--main",), {"default": None, "help": "the MAIN checkout (default: resolved from --root)"}),
    "limit": (("--limit",), {"type": int, "default": None, "help": "print at most N items"}),
    "dry_run": (("--dry-run",), {"action": "store_true", "help": "report what would change, change nothing"}),
    "quiet": (("--quiet",), {"action": "store_true", "help": "print only the verdict"}),
}

#: How `tools/selftest.py` recognises a tool that runs its own selftest: it registers `--selftest`, or it is a
#: `Tool` built with `tests=`.
SELFTEST_FLAG = re.compile(r"""add_argument\(\s*['"]--selftest['"]|\bTool\([^)]*\btests\s*=""")

_REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class Tool:
    """A tool's entry point. `tests` is the selftest it forwards `--selftest` to: a callable returning an exit
    status, or a path (relative to the repository root) run with this interpreter."""

    def __init__(self, name: str, spec: str = "", tests: str | Callable[[], int] | None = None,
                 description: str | None = None, common: Iterable[str] = COMMON):
        self.name = name
        self.spec = spec
        self.tests = tests
        self.description = description
        self.common = tuple(common)
        unknown = [c for c in self.common if c not in _FLAGS]
        if unknown:
            raise ValueError("unknown common flag(s): %s" % ", ".join(unknown))

    def parser(self, **kwargs: Any) -> argparse.ArgumentParser:
        """An `ArgumentParser` carrying the chosen common flags (and `--selftest` when the tool has tests)."""
        kwargs.setdefault("description", self.description)
        ap = argparse.ArgumentParser(**kwargs)
        for key in self.common:
            names, opts = _FLAGS[key]
            ap.add_argument(*names, **opts)
        if self.tests is not None:
            ap.add_argument("--selftest", action="store_true", help="run this tool's selftest and exit")
        return ap

    def selftest(self, cwd: str | None = None) -> int:
        """Run the forwarded selftest (a path runs as a child process in `cwd`); its exit status."""
        if callable(self.tests):
            return int(self.tests() or 0)
        path = os.path.join(_REPO, *str(self.tests).split("/"))
        return subprocess.run([sys.executable, path], cwd=cwd).returncode

    def run(self, main: Callable[[argparse.Namespace], Any], argv: Sequence[str] | None = None,
            parser: argparse.ArgumentParser | None = None) -> int:
        """Parse, forward `--selftest`, call `main(args)` and map its result to the exit convention.

        `main` returns a `Verdict` (printed as the one JSON schema on `--json`), an int (the exit status), a bool
        or None. An exception other than `SystemExit`/`KeyboardInterrupt` is reported on stderr and exits 2.
        """
        ap = parser or self.parser()
        args = ap.parse_args(argv)
        if self.tests is not None and getattr(args, "selftest", False):
            return self.selftest()
        try:
            result = main(args)
        except (SystemExit, KeyboardInterrupt):
            raise
        except Exception as exc:  # noqa: BLE001 - the "could not run" exit
            traceback.print_exc(file=sys.stderr)
            print("%s: could not run: %s" % (self.name, exc), file=sys.stderr)
            return _findings.EXIT_ERROR
        if isinstance(result, _findings.Verdict) and getattr(args, "json", False):
            print(_findings.render_json(self.name, result))
        return _findings.exit_code(result)
