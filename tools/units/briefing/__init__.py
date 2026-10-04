"""The worker brief, split by concern: `sources` gathers (the one module that reads other tools), `render`
writes the text, `pool` keeps the pre-written briefs. Spec: docs/tools/spec/briefing.md. CLI: tools/units/brief.py."""
from tools.units.briefing import sources  # noqa: E402,F401  (the order matters: render and pool read sources)
from tools.units.briefing import render  # noqa: E402,F401
from tools.units.briefing import pool  # noqa: E402,F401
