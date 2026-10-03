"""The shared concepts every tool stands on (a package; no CLI; never imports a tool).
Spec: docs/tools/design.md. CLI: none (library)."""
from __future__ import annotations

#: The one line every tool entry point carries (design.md section 2, "Import form"): it puts the repository
#: root on `sys.path` from any depth under `tools/`, so `from tools.lib import ...` works when a tool is run by
#: path. `tools/tests/lib/test_prologue.py` checks every entry point carries exactly this line and no other
#: `sys.path` mutation.
PROLOGUE = ('import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents '
            'if (p / "tools" / "__init__.py").is_file())))')
