"""Rule 3: a reconstructed struct/class states its size (`/* size: 0xNN */` near the definition).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.units.stylelint_rules.common import Source, _finding

SIZE_RE = re.compile(r"size\s*:\s*0x[0-9A-Fa-f]+")


def struct_has_size(src: Source, defs: list[dict]) -> set[int]:
    """Indices into `defs` that carry a `size: 0xNN` comment near their definition.

    Each annotation is assigned to its nearest definition (the following one wins a tie) so one comment
    cannot certify two adjacent types.
    """
    anns = [src.line_of(m.start()) for m in SIZE_RE.finditer(src.comments)]
    ok = set()
    for a in anns:
        best, best_key = None, None
        for idx, d in enumerate(defs):
            if d["line"] <= a <= d["end_line"]:
                dist = 0
            else:
                dist = min(abs(a - d["line"]), abs(a - d["end_line"]))
            key = (dist, abs(a - d["line"]))
            if best_key is None or key < best_key:
                best, best_key = idx, key
        if best is not None and best_key[0] <= 4:
            ok.add(best)
    return ok


def findings(src: Source, defs: list[dict]) -> list[dict]:
    """One finding per definition in `defs` (`struct_defs`) with no size annotation within four lines."""
    sized = struct_has_size(src, defs)
    return [_finding(src, 3, d["line"], "type `%s` has no `/* size: 0xNN */`" % d["name"], token=d["name"])
            for idx, d in enumerate(defs) if idx not in sized]
