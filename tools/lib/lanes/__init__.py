"""The lane model: naming, the claim registry, live sessions, the slot pool, seeding, rescue refs, one teardown,
the launch line and the landing log. Spec: docs/tools/spec/lib-lanes.md. CLI: none (library).

Import the module you need (`from tools.lib.lanes import naming, registry`); nothing is imported here, so a tool
that only needs `naming` does not pay for the pool's git plumbing at start-up.
"""
from __future__ import annotations
