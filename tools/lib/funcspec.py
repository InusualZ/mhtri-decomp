"""Which function the mwcc debugger dumps: a plain name, a mangled name, `Class::method` or an address, as one `Selector`.
Pure (no gdb): the launcher parses, the gdb side matches the compiler's current function. Spec: docs/tools/spec/lib-funcspec.md.
CLI: none (a library; `tools/mwcc-debugger/mwcc_debugger.py` takes the selector as FUNC or `--symbol`)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from tools.lib import names

_ADDRESS = re.compile(r"^(?:0x[0-9A-Fa-f]{5,8}|8[0-9A-Fa-f]{7})$")
MAX_LISTED = 6


class SelectorError(ValueError):
    """The text names no function, or more than one."""


@dataclass(frozen=True)
class Selector:
    """The function to dump: `name` is its source identifier (`__ct`/`__dt` for constructors/destructors), `scope`
    its owner chain outermost first, `exact` whether `scope` is the whole chain (a mangled name) or only its tail
    (`Class::method`), `mangled` its link name when known, `size`/`address` its `symbols.txt` row when resolved."""
    text: str
    name: str
    scope: tuple = ()
    exact: bool = False
    mangled: str = ""
    size: int = 0
    address: int = 0
    notes: tuple = field(default=(), compare=False)

    @property
    def label(self) -> str:
        """What a dump is stamped with: the link name when known (regsim reads it as the object's symbol)."""
        return self.mangled or "::".join(self.scope + (self.name,))

    @property
    def extract_name(self) -> str:
        """What `lib.cextract` is asked for: the link name, else the source name."""
        return self.mangled or self.name

    def to_dict(self) -> dict:
        return {"text": self.text, "name": self.name, "scope": list(self.scope), "exact": self.exact,
                "mangled": self.mangled, "size": self.size, "address": self.address}

    @classmethod
    def from_dict(cls, d: dict) -> "Selector":
        return cls(d["text"], d["name"], tuple(d.get("scope", ())), bool(d.get("exact")), d.get("mangled", ""),
                   int(d.get("size", 0)), int(d.get("address", 0)))

    def matches(self, name: str, linkname: str | None, scope) -> bool:
        """Whether the compiler's current function (source `name`, cached `linkname`, owner `scope`, None when unknown) is this one."""
        if name != self.name:
            return False
        if self.mangled and linkname:
            return linkname == self.mangled
        if scope is None:       # a build whose record layout hides the owner scope: the name is all there is
            return True
        scope = tuple(scope)
        if self.exact:
            return scope == self.scope
        if self.scope:
            return len(scope) >= len(self.scope) and scope[len(scope) - len(self.scope):] == self.scope
        return True


def parse_size(text) -> int:
    """`0x2A4` or `676` -> int (0 for None/empty)."""
    return int(str(text), 0) if text not in (None, "") else 0


def _member_text(text: str) -> tuple[str, tuple]:
    """`A::B::m` -> `("m", ("A", "B"))`; `A::~A` is `__dt` and `A::A` is `__ct`, the names the compiler uses."""
    parts = [p.strip() for p in text.split("::")]
    if not all(parts):
        raise SelectorError("%r is not a qualified name" % text)
    name, scope = parts[-1], tuple(parts[:-1])
    if name.startswith("~"):
        name = "__dt"
    elif name == scope[-1]:
        name = "__ct"
    return name, scope


def _rows_named(rows, name: str, scope: tuple):
    """Function rows `(symbol, address, size)` whose mangled name carries `name` and ends in `scope`."""
    out = []
    for sym, address, size in rows:
        got = names.split_member(sym)
        if got is None:
            if not scope and sym == name:
                out.append((sym, address, size))
            continue
        base, chain = got
        if base == name and chain[len(chain) - len(scope):] == scope and len(chain) >= len(scope):
            out.append((sym, address, size))
    return out


def _listed(found) -> str:
    shown = ["%s (0x%08X, 0x%X)" % row for row in found[:MAX_LISTED]]
    return ", ".join(shown) + (", ... %d more" % (len(found) - MAX_LISTED) if len(found) > MAX_LISTED else "")


def _row_selector(row, text: str) -> Selector:
    """The `Selector` of one resolved map row `(symbol, address, size)`: a mangled name keeps its owner chain, a plain
    (C-linkage) name has none. Never re-enters `parse`, so a row whose name is its own plain name cannot loop."""
    sym, address, size = row
    got = names.split_member(sym)
    if got is not None:
        return Selector(text, got[0], got[1], True, sym, size, address)
    return Selector(text, sym, (), False, "", size, address)


def parse(text: str, rows=(), size=0) -> Selector:
    """The `Selector` for `text` (a mangled name, `Class::method`, an address `0x8009792C`, or a plain name).

    `rows` is the map's function rows `(symbol, address, size)` (`function_rows`); with it an address names its row,
    a `Class::method` (or a plain name with `size`) is upgraded to the one row it identifies, and an ambiguity is a
    `SelectorError` listing the candidates. Without `rows` the text is used as it stands.
    """
    text = (text or "").strip()
    if not text:
        raise SelectorError("empty function selector")
    rows = list(rows)
    if _ADDRESS.match(text):
        address = int(text, 16)
        hit = [r for r in rows if r[1] == address]
        if not hit:
            raise SelectorError("no function row at 0x%08X in the symbol map%s" % (
                address, "" if rows else " (no map given: pass --symbols)"))
        return _row_selector(hit[0], text)
    if "::" in text:
        name, scope = _member_text(text)
        found = _rows_named(rows, name, scope)
        if size:
            found = [r for r in found if r[2] == size]
        if len(found) > 1:
            raise SelectorError("%s names %d functions: %s - pass the mangled name, an address or --size" % (
                text, len(found), _listed(found)))
        if found:
            return _row_selector(found[0], text)
        return Selector(text, name, scope, False, "", size, 0)
    got = names.split_member(text)
    if got is not None:
        return Selector(text, got[0], got[1], True, text, size)
    wide = [r for r in rows if (names.split_member(r[0]) or (r[0],))[0] == text] if rows else []
    if size:
        pick = [r for r in wide if r[2] == size]
        if len(pick) > 1:
            raise SelectorError("%s with size 0x%X names %d functions: %s - pass the mangled name or an address" % (
                text, size, len(pick), _listed(pick)))
        if pick:
            return _row_selector(pick[0], text)
        raise SelectorError("no function named %s has size 0x%X in the symbol map" % (text, size))
    notes = ()
    if len(wide) > 1:
        notes = ("%d symbols share the name %s (%s): the first function of that name is dumped; pass its mangled "
                 "name, Class::%s, an address or --size to pick one" % (len(wide), text, _listed(wide), text),)
    return Selector(text, text, (), False, "", 0, 0, notes)


def function_rows(symbols_path: str):
    """The `.text` function rows `(symbol, address, size)` of a `symbols.txt`, or `[]` when it cannot be read."""
    import os
    if not os.path.isfile(symbols_path):
        return []
    from tools.lib import project
    out = []
    for row in project.SymbolMap(symbols_path).rows():
        e = row.to_dict()
        if e.get("section") == ".text" and e.get("type") == "function":
            out.append((e["name"], e["address"], int(e.get("size") or 0)))
    return out


def scope_chain(read_u32, read_string, node: int, limit: int = 16) -> tuple:
    """The owner chain (outermost first) of the compiler's scope record at `node`: `+0x00` is the parent scope and
    `+0x04` a name node whose text sits at `+0x0A`; the global scope has no name and no parent. Pure given the readers."""
    chain: list[str] = []
    for _ in range(limit):
        if not node:
            break
        name_node = read_u32(node + 4)
        if name_node:
            chain.insert(0, read_string(name_node + 0x0A))
        node = read_u32(node)
    return tuple(chain)
