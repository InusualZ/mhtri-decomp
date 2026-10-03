"""The three project files and ownership: the symbol map, the splits, the configure registry, one owner lookup.
Spec: docs/tools/spec/lib-project.md. CLI: none (library)."""
from __future__ import annotations

from tools.lib.project.configure import Configure, ObjectCall, ObjectRow, object_calls
from tools.lib.project.ownership import AutoObjects, Owner, Ownership
from tools.lib.project.splits import Range, Splits
from tools.lib.project.symbols import Refused, ShapeError, Symbol, SymbolMap, parse_line

__all__ = ["AutoObjects", "Configure", "ObjectCall", "ObjectRow", "Owner", "Ownership", "Range", "Refused",
           "ShapeError", "Splits", "Symbol", "SymbolMap", "object_calls", "parse_line"]
