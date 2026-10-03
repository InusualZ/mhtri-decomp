"""Shim: the staleness rule (a prebuilt object or the report older than its sources) lives in lib.report (removed in WP6).
Spec: docs/tools/spec/lib-report.md. CLI: none (library; importers must have the repository root on sys.path)."""
from __future__ import annotations

from tools.lib.report import (INCLUDE_RE, MAX_INCLUDE_DEPTH, freshness, includes_of, mtime, newest,  # noqa: F401
                              report_reasons, rel_path, resolve_include, source_closure, stamp, stamp_json,
                              unit_reasons)
from tools.lib.report import _inside  # noqa: F401
