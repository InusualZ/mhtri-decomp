"""The knowledge-delta row: a batch that improved the ledger shows it in a doc or a unit header (7.10).
Spec: docs/tools/spec/landing.md. CLI: none (a row of `land.py verify`)."""
from __future__ import annotations

from tools.units.landing.common import Batch


def knowledge_row(b: Batch, before: dict, after: dict) -> None:
    """20. a batch that moved the ledger's closed/matched/bytes up changed `docs/`, `CLAUDE.md` or a unit source."""
    improved = any(isinstance(before.get(k), (int, float)) and isinstance(after.get(k), (int, float))
                   and after[k] > before[k] for k in ("closed", "matched", "bytes"))
    docs_changed = any(p.startswith(("docs/", "CLAUDE.md")) for p in b.paths)
    headers_changed = any(p.startswith("src/") and p.endswith((".c", ".cpp", ".cp")) for p in b.paths)
    b.check("knowledge delta present if the batch improved something",
            (not improved) or docs_changed or headers_changed,
            "a unit improved and no docs/CLAUDE.md/unit header changed in this batch (7.10)",
            remedy="record the improvement: touch the unit's header or a docs/ / CLAUDE.md file in the batch")
