"""Shim: the Windows spawn retry lives in tools.lib.proc (removed in WP6).
Spec: docs/tools/spec/lib-proc.md. CLI: none (library)."""
from __future__ import annotations

from tools.lib.proc import SPAWN_ATTEMPTS as ATTEMPTS  # noqa: F401
from tools.lib.proc import SPAWN_BACKOFF_S as BACKOFF_S  # noqa: F401
from tools.lib.proc import install_spawn_retry as install  # noqa: F401
from tools.lib.proc import is_transient, retrying  # noqa: F401
