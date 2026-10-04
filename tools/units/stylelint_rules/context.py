"""The tree-wide facts the rules read: the rule-2 `Ownership` index, rule 13's type registry, the open STOPGAP request ids.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import collections
import os

from tools.lib import requests as _requests
from tools.lib import project as _project
from tools.lib.project import ownership as _ownership


# --------------------------------------------------------------------------------------------------
# rule 2: an extern lives with the TU that owns it (symbols.txt + splits.txt)
# --------------------------------------------------------------------------------------------------
module_name = _ownership.module_name
#: the ownership cache (`lib.project.ownership`), exposed so a selftest can clear it between scenarios
_OWNERSHIP_CACHE = _ownership._CACHE


class Ownership(_project.Ownership):
    """The rule-2 index: `lib.project.Ownership` plus the counters a lint run accumulates.

    `gaps`, `unsplit_modules` and `foreign_units` record what the lookup can and cannot judge, so the report
    states the classes it leaves alone instead of guessing them.
    """

    def __init__(self, symbols: dict, ranges: dict, **kw):
        super().__init__(symbols, ranges, **kw)
        self.gaps: "collections.Counter" = collections.Counter()
        self.unsplit_modules: "collections.Counter" = collections.Counter()
        self.unsplit_symbols: dict[str, set] = {}
        self.foreign_units: "collections.Counter" = collections.Counter()


def _parse_symbols(path: str) -> dict:
    """`name -> [(section, address, type)]` from a map file (`lib.project.ownership.symbol_index`)."""
    return _ownership.symbol_index(_project.SymbolMap(path).rows())


def _parse_splits(path: str) -> dict:
    """`section -> [(start, end, unit)]` from a splits file (`lib.project.Splits.by_section`)."""
    return _project.Splits.read(path).by_section()


def load_ownership(root: str) -> "Ownership | None":
    """The tree's rule-2 index, parsed once per mtime; None when either file is absent (rule 2 unchecked)."""
    return Ownership.load(root)


# --------------------------------------------------------------------------------------------------
# the STOPGAP block: a lane's pilot-only foreign declarations, cleared by the integrator
# --------------------------------------------------------------------------------------------------
# `/* STOPGAP-BEGIN(<id>) */ ... /* STOPGAP-END(<id>) */` wraps declarations a lane could not put in their
# owner's header; `<id>` is the integrator request (`<slug>#<n>`) that clears it (`integrate.py` deletes the
# block when it applies the request). The marker exempts nothing - the declarations inside still report rule 2
# and rule 7 like any other - and a block whose id names no OPEN request (none filed, or already applied or
# rejected) is itself a rule-2 finding: it is a foreign declaration with no path to its owner.
_REQUEST_DIRS: "list[str] | None" = None
_OPEN_IDS: "set[str] | None" = None


def set_request_dirs(dirs: "list[str] | None") -> None:
    """The outbox directories whose `<slug>-requests.json` (+ status sidecar) define the open request ids; None:
    the invocation tree's and MAIN's `.pi/outbox`, read on first use."""
    global _REQUEST_DIRS, _OPEN_IDS
    _REQUEST_DIRS, _OPEN_IDS = dirs, None


def open_request_ids() -> set:
    global _OPEN_IDS, _REQUEST_DIRS
    if _OPEN_IDS is None:
        if _REQUEST_DIRS is None:
            from tools.lib import repo as _repo
            try:
                root = _repo.repo_root()
                dirs = [os.path.join(root, ".pi", "outbox"), os.path.join(_repo.main_checkout(root), ".pi", "outbox")]
            except Exception:                                    # noqa: BLE001 - outside a repository
                dirs = []
            _REQUEST_DIRS = dirs
        _OPEN_IDS = _requests.open_ids(_REQUEST_DIRS)
    return _OPEN_IDS
