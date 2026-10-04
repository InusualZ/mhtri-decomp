"""Rule 12: an `extern` of data no registered range claims is the finding - the user claims and matches it.
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

from tools.units.stylelint_rules.common import Source, _finding
from tools.units.stylelint_rules.r02_extern import extern_declarations


def rule12_findings(src: Source, ownership: "Ownership | None") -> list[dict]:
    """Rule 12 for one file: every `extern` of a **data** symbol no registered range claims.

    The owner's ruling (2026-09-28): data a unit reads or writes that nothing claims is the unit's to
    **claim and match** - claim the range in its own `splits.txt`, in the section the bytes live in, and
    reconstruct the bytes so they byte-match the target. The `extern` declaration is the finding, wherever
    it sits (`src/` file, `include/<module>/` header, or the `include/unsplit/` band: the band is rule 2's
    fallback for a symbol nobody can claim, not the answer for data a unit demonstrably uses).

    Only the `extern` keyword is read, and only symbols the map types as an object: a function declaration
    is rule 2's. The `Ownership` index is the same one rule 2 resolves through, so an `owned` address is
    clean - which is the declare-never-define carve-out (playbook 29: the range is already the unit's own,
    so defining the constants would rebuild the pool) - while a name absent from the map or a duplicate row
    is left alone (rule 2's `Ownership.gaps` is the one place those are counted). Rule 2 may name the same
    line: rule 2 says whose header the declaration belongs in, rule 12 says the bytes must be claimed.
    """
    if ownership is None:
        return []
    out = []
    for name, _pos, line in extern_declarations(src):
        r = ownership.resolve(name)
        if r is None or r["kind"] != "unsplit" or r.get("type") != "object":
            continue
        out.append(_finding(src, 12, line,
                            "`%s` is unowned data - no registered range covers `%s:0x%X`; the unit that "
                            "uses it claims the range in its own `splits.txt` and matches the bytes "
                            "(rule 12)" % (name, r["section"], r["address"]), token=name))
    return out
