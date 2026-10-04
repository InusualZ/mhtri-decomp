"""Rule 10 (lint half): a codegen pragma lives in the TU that needs it, never in a shared header.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import HEADERS, _finding


# The codegen-affecting pragma names for rule 10.  A `#pragma` is lexically scoped to the rest of the
# translation unit that reaches it, so one in a shared header leaks the pass onto every including TU -
# measured 2026-09-27 in both directions: a lane matched a file only because of a leaked
# `#pragma peephole off`, and another lost rows until it was restated where it was wanted.  The pragma
# belongs in the `.c`/`.cpp` that measured the dependency.  The list is section 6.5's codegen subset;
# extend it when a new codegen pragma is used.  Deliberately absent: `once` (include guard), `pack`
# (layout/ABI, judged elsewhere), `legacy_messages`/`warn*` (diagnostics only).
CODEGEN_PRAGMAS = (
    "peephole", "optimization_level", "fp_contract", "optimize_for_size", "inline",
    "pool", "scheduling", "scheduling_priority", "unroll", "vectorize", "ipa", "profile",
    "opt_propagation", "opt_common_subs", "opt_lifetimes",
)
CODEGEN_PRAGMA_RE = re.compile(
    r"^[ \t]*#[ \t]*pragma[ \t]+(" + "|".join(CODEGEN_PRAGMAS) + r")\b", re.M)


def codegen_pragma_findings(src: "Source") -> list[dict]:
    """Rule 10 for one file: a codegen pragma in a **shared header**.  The path gate is here, not in the
    caller, so a `.c`/`.cpp` can never be reported however this is invoked.  `src.code` blanks
    comments/literals, so a pragma *named* in a comment is not a finding - only a real directive is."""
    if not src.rel.replace("\\", "/").startswith(HEADERS + "/"):
        return []
    out = []
    for m in CODEGEN_PRAGMA_RE.finditer(src.code):
        out.append(_finding(src, 10, src.line_of(m.start()),
                            "codegen pragma `#pragma %s` in a shared header - state it in the "
                            "`.c`/`.cpp` that needs it, never in the header" % m.group(1), token=m.group(1)))
    return out
