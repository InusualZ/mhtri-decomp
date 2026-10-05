"""Integrator requests: the schema, the legacy free-text loader, static classification, owner resolution, status.
Spec: docs/tools/spec/lib-requests.md. CLI: none (library; `tools/units/integrate.py` is the CLI)."""
from __future__ import annotations

import datetime
import glob
import json
import os
import re
from dataclasses import dataclass, field
from typing import Any, Iterable

from tools.lib import names as libnames
from tools.lib import text as libtext
from tools.lib.project.ownership import band_root
from tools.lib.repo import header_root

# --- the schema (one definition: brief.py renders it, handoff.py validates with it, integrate.py reads it) ----

#: Every request kind, with what it asks for and the fields it needs beyond `id`, `kind` and `evidence`.
SCHEMA = (
    {"kind": "rename", "needs": ("symbol|address", "proposed_name"),
     "means": "a map rename of a symbol another unit owns (the integrator sweeps every reference)"},
    {"kind": "decl", "needs": ("symbol|address",),
     "means": "declare an owner's symbol in the owner's header (a generated name also needs proposed_name)"},
    {"kind": "decl-move", "needs": ("symbol|address",),
     "means": "move or drop a declaration that sits in the wrong header (rule 2), owner unchanged"},
    {"kind": "field", "needs": ("symbol",),
     "means": "a field, size or signature change in a type another lane owns"},
    {"kind": "seam", "needs": ("symbol|address",),
     "means": "a translation-unit boundary that should be re-drawn (registration move, unit merge or split)"},
    {"kind": "unit-rename", "needs": ("symbol",),
     "means": "a registered unit's file should take another name (`land.py --unit-rename`)"},
    {"kind": "config", "needs": ("proposed",),
     "means": "a config.yml relocation-analysis change (block_relocations / add_relocations only)"},
    {"kind": "info", "needs": (),
     "means": "a finding to record, no edit asked (a `NO MOVE` verdict, a residual)"},
)
KINDS = tuple(r["kind"] for r in SCHEMA)
CONFIDENCES = ("certain", "evidence", "guess")
CLASSES = ("mechanical", "semi", "judgement")
STATUSES = ("open", "applied", "deferred", "judgement", "rejected")
#: A request id: the lane's slug, `#`, the 1-based line of the request file.
ID_RE = re.compile(r"^[A-Za-z0-9][\w.-]*#\d+$")
#: The STOPGAP block a lane wraps its pilot-only foreign declarations in; `<id>` is the request that clears it.
STOPGAP_BEGIN_RE = re.compile(r"/\*\s*STOPGAP-BEGIN\(([^)\s]+)\)\s*\*/")
STOPGAP_END_RE = re.compile(r"/\*\s*STOPGAP-END\(([^)\s]+)\)\s*\*/")
STOPGAP_BLOCK_RE = re.compile(r"[ \t]*/\*\s*STOPGAP-BEGIN\((?P<id>[^)\s]+)\)\s*\*/[\s\S]*?"
                              r"/\*\s*STOPGAP-END\((?P=id)\)\s*\*/[ \t]*\r?\n?")
REQUESTS_SUFFIX = "-requests.json"
STATUS_SUFFIX = "-requests.status.json"

_IDENT = r"[A-Za-z_~][\w]*(?:::[A-Za-z_~]\w*)*"
_IDENT_RE = re.compile(_IDENT)
_C_IDENT_RE = re.compile(r"^[A-Za-z_]\w*$")
_ADDR_RE = re.compile(r"(?:(\.[a-z0-9]+)[: ])?0x([0-9A-Fa-f]{8})\b")
#: Words in a decl/field request's `proposed` that ask for a type, class or linkage change (judgement), and the
#: smaller set that asks only for a signature tweak of one existing declaration (semi).
JUDGEMENT_WORDS = ("virtual", "class", "methodize", "constructor", "placement", "view", "replace", "polymorphic",
                   "vptr", "size 0x", "unified", "member")
SEMI_WORDS = ("returns", "parameter", "signed")
#: A member, constructor or destructor mangling (`name__<n><Class>F...`, `__ct__`, `__dt__`) - `lib.names.is_mangled`
#: reads only `__F`/`__Q`, which a member's `__15sNetworkLibraryFv` does not carry.
MEMBER_MANGLING_RE = re.compile(r"__(?:F|Q\d|\d+[A-Za-z_]|ct|dt)")
CTOR_WORDS = ("destructor", "constructor", "ctor", "dtor")
#: Common English words that start a prose `proposed` (never a proposed symbol name).
PROSE_WORDS = frozenset("the a an name ctor dtor rename declare drop make no keep move split merge seam make "
                        "size it its this that".split())


@dataclass(frozen=True)
class Target:
    """One symbol a request is about: the spelling filed, its address, a second spelling (a mangled alias),
    the proposed name and the prototype when the request carries them."""
    symbol: str
    address: int | None = None
    section: str | None = None
    proposed_name: str | None = None
    prototype: str | None = None
    alias: str | None = None

    def to_dict(self) -> dict:
        d = {"symbol": self.symbol, "address": "0x%08X" % self.address if self.address is not None else None,
             "section": self.section, "proposed_name": self.proposed_name, "prototype": self.prototype}
        if self.alias:
            d["alias"] = self.alias
        return d


@dataclass
class Request:
    """One integrator request, normalised. `legacy` is True for a free-text line the loader interpreted."""
    id: str
    kind: str
    targets: list[Target]
    evidence: str = ""
    proposed: str = ""
    confidence: str = "evidence"
    owner_unit: str | None = None
    owner_text: str = ""
    header: str | None = None
    stopgap: dict | None = None
    lane: str = ""
    legacy: bool = False
    raw: dict = field(default_factory=dict)

    @property
    def atomic(self) -> bool:
        """A request filed with `prototypes`: its declarations apply together or not at all (one STOPGAP block)."""
        return isinstance(self.raw.get("prototypes"), list)

    def symbol_text(self) -> str:
        """The targets' spellings and the raw `symbol` field, one string (what a keyword test reads)."""
        return " ".join(t.symbol for t in self.targets) + " " + str(self.raw.get("symbol") or "")

    def to_dict(self) -> dict:
        return {"id": self.id, "kind": self.kind, "targets": [t.to_dict() for t in self.targets],
                "confidence": self.confidence, "owner_unit": self.owner_unit, "header": self.header,
                "stopgap": self.stopgap, "lane": self.lane, "legacy": self.legacy,
                "proposed": self.proposed, "evidence": self.evidence}


# --- validation (the new schema) ----------------------------------------------------------------------------

def validate(entry: dict) -> list[str]:
    """Every problem with one new-schema request object (empty: valid)."""
    out = []
    rid, kind = entry.get("id"), entry.get("kind")
    if not isinstance(rid, str) or not ID_RE.match(rid):
        out.append("id %r is not `<slug>#<n>`" % (rid,))
    if kind not in KINDS:
        out.append("kind %r is not one of %s" % (kind, ", ".join(KINDS)))
        return out
    if kind != "info" and not str(entry.get("evidence") or "").strip():
        out.append("%s: no evidence" % kind)
    conf = entry.get("confidence", "evidence")
    if conf not in CONFIDENCES:
        out.append("confidence %r is not one of %s" % (conf, "/".join(CONFIDENCES)))
    sym, addr = entry.get("symbol"), entry.get("address")
    needs = next(r["needs"] for r in SCHEMA if r["kind"] == kind)
    if "symbol|address" in needs and not (sym or addr) and not entry.get("prototypes"):
        out.append("%s: needs `symbol` or `address`" % kind)
    if "symbol" in needs and not sym:
        out.append("%s: needs `symbol`" % kind)
    if "proposed" in needs and not entry.get("proposed"):
        out.append("%s: needs `proposed`" % kind)
    if addr is not None and parse_address(addr) is None:
        out.append("address %r is not a hex address" % (addr,))
    pname = entry.get("proposed_name")
    if kind == "rename" and not pname:
        out.append("rename: needs `proposed_name`")
    if pname is not None and not _C_IDENT_RE.match(str(pname)):
        out.append("proposed_name %r is not a C identifier" % (pname,))
    if kind == "decl" and sym and libnames.is_generated(str(sym)) and not pname:
        out.append("decl of the generated name %s needs `proposed_name` (rule 7: the integrator names it)" % sym)
    if entry.get("prototypes") is not None:
        out.extend(_validate_prototypes(entry))
    proto = entry.get("prototype")
    if proto is not None:
        if "\n" in str(proto) or not str(proto).strip().endswith(";"):
            out.append("prototype must be one C line ending in `;`")
        elif kind != "field":
            # a `field` prototype is a member-slot fragment (`u8 net_active;`) of the type `symbol` names - the
            # symbol lives in `symbol` (required above), never in the fragment
            want = pname or sym
            if want and not re.search(r"\b%s\b" % re.escape(str(want)), str(proto)):
                out.append("prototype does not declare %s" % want)
    gap = entry.get("stopgap")
    if gap is not None and (not isinstance(gap, dict) or not gap.get("file") or not gap.get("id")):
        out.append("stopgap must be {file, id}")
    return out


def _one_line(proto: Any) -> bool:
    return isinstance(proto, str) and "\n" not in proto and proto.strip().endswith(";")


def declared_name(proto: str) -> str | None:
    """The identifier one C declaration line declares: the name before the parameter list of a function, else the
    last identifier of a variable (`extern u8 table[4];` -> `table`); None when the line is no declaration."""
    c = re.sub(r"\s+", " ", str(proto or "").strip().rstrip(";").strip())
    c = re.sub(r'^extern\s+"C"\s+', "", c)
    m = re.match(r"^[^()]*?([A-Za-z_]\w*)\s*\(", c)
    if m:
        return m.group(1)
    names = re.findall(r"[A-Za-z_]\w*", re.sub(r"\[[^\]]*\]", " ", c))
    return names[-1] if len(names) >= 2 else None


def prototype_items(entry: dict) -> list[dict]:
    """The declarations a request carries, one dict each (`prototype`, and the `symbol`/`address`/`section`/
    `proposed_name` that belong to it): `prototypes` (a string - the symbol is the name it declares - or an object),
    else the one-item form `prototype` with the request's own fields; `[]` when it carries none."""
    many = entry.get("prototypes")
    if isinstance(many, list):
        out = []
        for item in many:
            d = {"prototype": item} if isinstance(item, str) else dict(item) if isinstance(item, dict) else {}
            if not d.get("symbol") and d.get("address") is None and isinstance(d.get("prototype"), str):
                d["symbol"] = d.get("proposed_name") or declared_name(d["prototype"])
            out.append(d)
        return out
    if entry.get("prototype") is not None:
        return [{k: entry.get(k) for k in ("prototype", "symbol", "address", "section", "proposed_name")}]
    return []


def _validate_prototypes(entry: dict) -> list[str]:
    """The `prototypes` form: a `decl` only, never beside `prototype` or a top-level `symbol`/`address`/
    `proposed_name` (each declaration carries its own), each item one C line that declares its symbol, no name
    twice."""
    out = []
    many = entry.get("prototypes")
    if entry.get("kind") != "decl":
        return ["prototypes: only a `decl` request carries several declarations"]
    if entry.get("prototype") is not None:
        out.append("prototype and prototypes are exclusive (`prototype` is the one-item form)")
    stray = [k for k in ("symbol", "address", "proposed_name") if entry.get(k) is not None]
    if stray:
        out.append("prototypes: %s belong in the items (each declaration carries its own)" % "/".join(stray))
    if not isinstance(many, list) or not many:
        return out + ["prototypes must be a non-empty list"]
    seen: set[str] = set()
    for i, item in enumerate(prototype_items(entry), 1):
        proto = item.get("prototype")
        if not _one_line(proto):
            out.append("prototypes[%d]: one C line ending in `;` (a string, or an object with `prototype`)" % i)
            continue
        if item.get("address") is not None and parse_address(item["address"]) is None:
            out.append("prototypes[%d]: address %r is not a hex address" % (i, item["address"]))
        pname, sym = item.get("proposed_name"), item.get("symbol")
        if pname is not None and not _C_IDENT_RE.match(str(pname)):
            out.append("prototypes[%d]: proposed_name %r is not a C identifier" % (i, pname))
        if sym and libnames.is_generated(str(sym)) and not pname:
            out.append("prototypes[%d]: the generated name %s needs `proposed_name` (rule 7)" % (i, sym))
        want = pname or sym
        if not want:
            out.append("prototypes[%d]: declares no name" % i)
        elif not re.search(r"\b%s\b" % re.escape(str(want)), proto):
            out.append("prototypes[%d]: does not declare %s" % (i, want))
        elif want in seen:
            out.append("prototypes[%d]: %s is declared twice" % (i, want))
        seen.add(str(want))
    return out


def parse_address(value: Any) -> int | None:
    """An address from `0x803CB958`, `803CB958` or an int; None when it is not one."""
    if isinstance(value, int):
        return value
    m = re.fullmatch(r"\s*(?:0x)?([0-9A-Fa-f]{1,8})\s*", str(value or ""))
    return int(m.group(1), 16) if m else None


# --- the legacy free-text loader ------------------------------------------------------------------------------

def _split_targets(text: str) -> list[str]:
    """`a, b` / `a / b` into parts; a path (`a/b.cpp`, no spaces) is one part."""
    return [p for p in re.split(r"\s*,\s*|\s+/\s+", text.strip()) if p.strip()]


def parse_target(part: str) -> Target:
    """One legacy symbol spelling: `fn_X`, `name (.text:0x...)`, `0x8079 name`, `name (mangled)`, `class X`."""
    s = part.strip()
    address = section = alias = None
    lead = re.match(r"^0x([0-9A-Fa-f]{8})\s+", s)
    if lead:
        address = int(lead.group(1), 16)
        s = s[lead.end():]
    for paren in re.findall(r"\(([^)]*)\)", s):
        m = _ADDR_RE.search(paren)
        if m and address is None:
            address, section = int(m.group(2), 16), m.group(1)
        elif _C_IDENT_RE.match(paren.strip()) and libnames.is_mangled(paren.strip()):
            alias = paren.strip()
    s = re.sub(r"\([^)]*\)", " ", s)
    s = re.sub(r"^(?:class|struct|unit stem)\s+", "", s.strip())
    m = _IDENT_RE.search(s)
    symbol = m.group(0) if m else s.strip()
    if address is None:
        address = libnames.address_of(symbol)
        if address is not None and symbol.startswith("lbl_"):
            section = None
    return Target(symbol, address, section, alias=alias)


def first_name(text: str) -> str | None:
    """The symbol name a prose `proposed` starts with, or None when it starts with prose (`the member...`)."""
    s = (text or "").strip().strip("`")
    m = re.match(r"^([A-Za-z_~][\w:~]*)", s)
    if not m:
        return None
    tok, rest = m.group(1), s[m.end():]
    if tok.lower() in PROSE_WORDS:
        return None
    if not rest.strip() or re.match(r"^\s*(?:\(|,|;|/|$)", rest):
        return tok
    if re.search(r"[A-Z_0-9]", tok[1:]) or "_" in tok:
        return tok
    return None


def proposed_names(text: str, count: int) -> list[str | None]:
    """Per-target names out of `proposed` (`a / b`, `a, b`): exactly `count` names, else the first for one target."""
    s = re.sub(r"\([^)]*\)", " ", text or "")
    parts = [first_name(p) for p in re.split(r"\s*,\s*|\s+/\s+", s.split(";")[0]) if p.strip()]
    parts = [p for p in parts if p]
    if count > 1:
        return parts[:count] if len(parts) >= count else [None] * count
    return [first_name(text)]


_STRICT_FN = re.compile(r"^(?:extern\s+)?((?:[A-Za-z_]\w*[\s\*&]+){1,4}?)\**\s*([A-Za-z_]\w*)\s*\(([^()]*)\)\s*$")
_STRICT_VAR = re.compile(r"^extern\s+((?:[A-Za-z_]\w*[\s\*]+){1,4}?)\**\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*$")


def prototype_in(text: str) -> str | None:
    """A C declaration a `proposed` spells - in backticks, or as a `;`-terminated segment - or None. Strict: at
    most four type tokens before the name, none of them prose, so a sentence with a parenthesis is never read as
    a prototype."""
    candidates = [c[:-1] for c in re.findall(r"`([^`]+;)`", text or "")]
    candidates += [c.strip() for c in (text or "").split(";")]
    for c in candidates:
        c = re.sub(r"\s+", " ", c.strip())
        for rx in (_STRICT_FN, _STRICT_VAR):
            m = rx.match(c)
            if m and not set(m.group(1).lower().split()) & PROSE_WORDS:
                return c + ";"
    return None


def header_in(text: str, root: str | None = None) -> str | None:
    """The header a request names (`src/X/y.h`, `include/X/y.h` - the pre-move spelling -, `X/y.h`, `y.h`), as a
    path under the tree's include root (`lib.repo.header_root`)."""
    m = re.search(r"\b([\w./]+\.h)\b", text or "")
    if not m:
        return None
    h = m.group(1)
    for pre in ("include/", "src/"):
        if h.startswith(pre):
            h = h[len(pre):]
            break
    return "%s/%s" % (header_root(root), h)


def _confidence(*texts: str) -> str:
    joined = " ".join(t or "" for t in texts)
    if re.search(r"\bGUESS\b", joined):
        return "guess"
    if re.search(r"\b(SDK|dump names|certain|retail symbol|the strings? )", joined):
        return "certain"
    return "evidence"


def normalise_legacy(raw: dict, slug: str, n: int) -> Request:
    """A free-text request line (`kind`/`symbol`/`owner`/`proposed`/`evidence`) as a Request."""
    kind = str(raw.get("kind") or "info").strip()
    symbol = str(raw.get("symbol") or "")
    proposed = str(raw.get("proposed") or "")
    evidence = str(raw.get("evidence") or "")
    owner = str(raw.get("owner") or "")
    low = proposed.lower()
    if kind == "rename" and symbol.lower().startswith("unit stem"):
        kind = "unit-rename"
    elif kind == "move":
        if low.startswith("no move"):
            kind = "info"
        elif "drop the declaration" in low or "declaration from" in low:
            kind = "decl-move"
        elif "comment" in symbol.lower():
            kind = "info"
        else:
            kind = "seam"
    elif kind not in KINDS:
        kind = "info"
    if kind in ("unit-rename", "seam", "config", "info"):
        targets = [Target(symbol.strip())] if symbol.strip() else []
    else:
        parts = [symbol] if kind == "field" else _split_targets(symbol)
        targets = [parse_target(p) for p in parts] if symbol else []
    if kind == "decl-move":
        m = re.search(r"\b([A-Za-z_]\w+)\b", symbol)
        targets = [Target(m.group(1))] if m else targets
    names = proposed_names(proposed, len(targets)) if kind in ("rename",) else [None] * len(targets)
    if kind == "decl":
        m = re.match(r"\s*rename to ([A-Za-z_]\w*)", proposed)
        if m and len(targets) == 1:
            names = [m.group(1)]
    proto = prototype_in(proposed) if kind == "decl" and len(targets) == 1 else None
    targets = [Target(t.symbol, t.address, t.section, nm, proto if i == 0 else None, t.alias)
               for i, (t, nm) in enumerate(zip(targets, names))]
    owner_unit = None
    m = re.search(r"\b([\w.]+/[\w.]+\.(?:cpp|c))\b", owner) or re.search(r"\b([\w]+\.(?:cpp|c))\b", owner)
    if m:
        owner_unit = m.group(1)
    return Request(id="%s#%d" % (slug, n), kind=kind, targets=targets, evidence=evidence, proposed=proposed,
                   confidence=_confidence(proposed, evidence), owner_unit=owner_unit, owner_text=owner,
                   header=header_in(proposed) or (header_in(owner) if kind in ("decl", "decl-move") else None),
                   lane=str(raw.get("lane") or ""), legacy=True, raw=raw)


def _target(d: dict, section: str | None = None) -> Target | None:
    addr = parse_address(d.get("address")) if d.get("address") is not None else None
    sym = str(d.get("symbol") or "")
    if not sym and addr is not None:
        sym = "0x%08X" % addr
    if addr is None and sym:
        addr = libnames.address_of(sym)
    if not sym and addr is None:
        return None
    return Target(sym, addr, d.get("section") or section, d.get("proposed_name"), d.get("prototype"))


def from_entry(entry: dict) -> Request:
    """A new-schema object as a Request: one target, or one per item of `prototypes`."""
    if isinstance(entry.get("prototypes"), list):
        targets = [t for t in (_target(d, entry.get("section")) for d in prototype_items(entry)) if t]
    else:
        t = _target(entry)
        targets = [t] if t else []
    return Request(id=entry["id"], kind=entry["kind"], targets=targets,
                   evidence=str(entry.get("evidence") or ""), proposed=str(entry.get("proposed") or ""),
                   confidence=entry.get("confidence", "evidence"), owner_unit=entry.get("owner_unit"),
                   header=entry.get("header"), stopgap=entry.get("stopgap"), lane=str(entry.get("lane") or ""),
                   raw=entry)


def slug_of(path: str) -> str:
    base = os.path.basename(path)
    return base[:-len(REQUESTS_SUFFIX)] if base.endswith(REQUESTS_SUFFIX) else os.path.splitext(base)[0]


def load_file(path: str) -> list[Request]:
    """Every request of one `<slug>-requests.json` (JSON lines, or a JSON list); a bad line is skipped with an
    `info` request that says why, so a count never silently drops a filing."""
    slug = slug_of(path)
    text = libtext.read_text(path)
    stripped = text.strip()
    rows: list[Any]
    if stripped.startswith("["):
        rows = json.loads(stripped)
    else:
        rows = []
        for line in text.splitlines():
            if not line.strip():
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError as exc:
                rows.append({"kind": "info", "evidence": "unparsable line: %s" % exc})
    out = []
    for n, row in enumerate(rows, 1):
        if isinstance(row, dict) and ID_RE.match(str(row.get("id") or "")) and row.get("kind") in KINDS:
            out.append(from_entry(row))
        elif isinstance(row, dict):
            out.append(normalise_legacy(row, slug, n))
    return out


def request_files(directory: str) -> list[str]:
    return sorted(glob.glob(os.path.join(directory, "*" + REQUESTS_SUFFIX)))


# --- static classification --------------------------------------------------------------------------------------

def mangled(name: str | None) -> bool:
    """A C++ spelling (`lib.names.is_mangled`, or a member/ctor/dtor mangling)."""
    return bool(name) and (libnames.is_mangled(name) or MEMBER_MANGLING_RE.search(name) is not None)


def _words(text: str, words: Iterable[str]) -> str:
    """The first of `words` in `text` as a whole word ("signed" is not in "unsigned"), or ""."""
    return next((w for w in words if re.search(r"(?<![A-Za-z])%ss?(?![a-z])" % re.escape(w), text)), "")


def _plain_name(name: str | None) -> bool:
    return bool(name) and bool(_C_IDENT_RE.match(name)) and not mangled(name)


def classify(req: Request, decisions: dict | None = None) -> tuple[str, str]:
    """`(class, reason)`: `mechanical` (the integrator can apply it from the tree, a GUESS name once a decision
    supplies it), `semi` (a one-token signature tweak of an existing declaration) or `judgement`. Static: it reads
    the request (and the decisions), never the tree."""
    decisions = decisions or {}
    low = (req.proposed or "").lower()
    if req.kind in ("seam", "unit-rename"):
        return "judgement", "%s: a registration change (splits.txt / configure.py / a file move)" % req.kind
    if req.kind == "info":
        return "judgement", "information only: no edit is asked (%s)" % (req.proposed[:60] or "a note")
    if req.kind == "config":
        # read from `proposed` alone: the old test glued `symbol` to it, so `updateSessionblock_relocations` had no
        # word boundary and the L2 round-2 #42 read as judgement
        key, _items, why = config_items(req.proposed, (req.targets[0].section if req.targets else None))
        if key:
            return "mechanical", "config.yml relocation-analysis keys (the guard's door): %s" % why
        if _CFG_KEY_RE.search(req.proposed or ""):
            return "judgement", "a relocation-key proposal the integrator cannot read: %s" % why
        return "judgement", "config.yml outside the relocation-analysis keys"
    if not req.targets:
        return "judgement", "no symbol could be read from the request"
    member = [t.symbol for t in req.targets if "::" in t.symbol]
    if req.kind == "field":
        if _words(low, SEMI_WORDS) and not _words(low, JUDGEMENT_WORDS):
            return "semi", "a signature tweak of one existing declaration (%s)" % _words(low, SEMI_WORDS)
        return "judgement", "a type or field change in another lane's type"
    if req.kind == "decl-move":
        return "mechanical", "drop or move a declaration to its owner's header"
    if req.kind == "decl":
        if member:
            return "judgement", "a member declaration (%s): the class changes" % member[0]
        if _words(low, JUDGEMENT_WORDS):
            return "judgement", "the request asks for a type/class/linkage change (%s)" % _words(low, JUDGEMENT_WORDS)
        if _words(low, SEMI_WORDS):
            return "semi", "a signature tweak of one existing declaration (%s)" % _words(low, SEMI_WORDS)
        return "mechanical", "declare in the owner's header (prototype from the request, the owner or the lane)"
    if req.kind == "rename":
        names = [decisions.get(t.symbol) or decisions.get(_addr_key(t.address)) or t.proposed_name
                 for t in req.targets]
        if all(_plain_name(n) for n in names) and not member:
            return "mechanical", "rename to %s" % ", ".join(names)
        if any(n and (mangled(n) or "::" in n or n.startswith("~")) for n in names) or member:
            return "judgement", "the proposed name is a member/mangled spelling: the class must be declared first"
        if _words(low, CTOR_WORDS):
            return "judgement", "a constructor/destructor: its name is the class's mangling (a --names decision "                                 "with a C name makes it mechanical)"
        return "mechanical", "rename once a --names decision supplies the name (none is spelled in the request)"
    return "judgement", "kind %s" % req.kind


def _addr_key(address: int | None) -> str:
    return "0x%08X" % address if address is not None else ""


# --- config proposals -------------------------------------------------------------------------------------------

_CFG_KEY_RE = re.compile(r"\b(block_relocations|add_relocations)\b")
_CFG_RANGE_RE = re.compile(r"(?:(\.[a-z0-9]+):)?(0x[0-9A-Fa-f]{1,8})\s*\.\.\s*(?:\.[a-z0-9]+:)?(0x[0-9A-Fa-f]{1,8})")
_CFG_ADDR_RE = re.compile(r"(?:(\.[a-z0-9]+):)?\b(0x[0-9A-Fa-f]{1,8})\b")


def config_items(proposed: str, section: str | None = None) -> tuple[str | None, list[str], str]:
    """`(key, [YAML list item], why)` for a `config` request's `proposed`, in any of the forms lanes file:

    * YAML - `block_relocations:` (or `add_relocations:`) then `- ...` items, taken as written;
    * a range - `block_relocations source .text:0xA..0xB` / `block_relocations: 0xA..0xB` -> `source`/`end`;
    * instruction addresses - `block_relocations: 0xA and 0xB` -> `source: <first>`, `end: <last + 4>` (a block covers
      the instructions it names);
    * `add_relocations source .text:0xA type R_PPC_ADDR16_HA target SYM` -> `source`/`type`/`target`.

    A section defaults to `section` (the request's) or `.text`. `key` is None (and `why` says why) when the text names
    no relocation key or no address, so the request stays judgement."""
    text = proposed or ""
    m = re.match(r"\s*(block_relocations|add_relocations)\s*:\s*\n", text)
    if m:
        items = [b.strip() for b in re.split(r"(?m)^-", text[m.end():]) if b.strip()]
        return m.group(1), ["- " + b for b in items], "YAML items"
    km = _CFG_KEY_RE.search(text)
    if not km:
        return None, [], "the proposal names no relocation-analysis key"
    key = km.group(1)
    rest = re.sub(r"\([^)]*\)", " ", text[km.end():])      # an aside's address (`(the constant 0x80060034)`) is not one
    sec = section or ".text"
    if key == "add_relocations":
        src = re.search(r"\bsource\s*:?\s*" + _CFG_ADDR_RE.pattern, rest)
        typ = re.search(r"\btype\s*:?\s*(R_PPC_\w+)", rest)
        tgt = re.search(r"\btarget\s*:?\s*([A-Za-z_.$@][\w.$@:+]*)", rest)
        if not (src and typ and tgt):
            return None, [], "add_relocations needs `source <addr> type <R_PPC_*> target <symbol>`"
        return key, ["- source: %s:0x%08X\n  type: %s\n  target: %s" % (src.group(1) or sec, int(src.group(2), 16),
                                                                        typ.group(1), tgt.group(1))], \
            "add_relocations prose"
    r = _CFG_RANGE_RE.search(rest)
    if r:
        s = r.group(1) or sec
        return key, ["- source: %s:0x%08X\n  end: %s:0x%08X" % (s, int(r.group(2), 16), s, int(r.group(3), 16))], \
            "a range"
    addrs = [(m.group(1), int(m.group(2), 16)) for m in _CFG_ADDR_RE.finditer(rest)]
    if not addrs:
        return None, [], "the proposal names no address"
    s = next((a for a, _ in addrs if a), None) or sec
    lo, hi = min(a for _, a in addrs), max(a for _, a in addrs) + 4
    return key, ["- source: %s:0x%08X\n  end: %s:0x%08X" % (s, lo, s, hi)], "instruction addresses"


# --- decisions (--names FILE) ---------------------------------------------------------------------------------

def load_decisions(path: str | None) -> dict[str, str]:
    """`{filed spelling or 0xADDR: name}` from `old new` lines (`#` comments); the old side may be a generated
    name, an address or a request target's spelling."""
    out: dict[str, str] = {}
    if not path:
        return out
    for line in libtext.read_text(path).splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        old, new = line.split()[:2]
        out[old] = new
        a = libnames.address_of(old) if not old.lower().startswith("0x") else parse_address(old)
        if a is not None:
            out[_addr_key(a)] = new
    return out


def decision_for(t: Target, decisions: dict[str, str]) -> str | None:
    return decisions.get(t.symbol) or (decisions.get(_addr_key(t.address)) if t.address is not None else None)


# --- owner resolution -----------------------------------------------------------------------------------------

@dataclass
class Resolved:
    """A target against the tree: the map's current name, the owning unit or band, the header that should declare
    it, the language linkage the map implies, and why it cannot be applied (empty when it can)."""
    target: Target
    current: str | None = None
    section: str | None = None
    address: int | None = None
    owner: str | None = None
    owner_state: str | None = None
    header: str | None = None
    new_name: str | None = None
    stale: bool = False
    cpp_linkage: bool = False
    notes: list[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        return {"symbol": self.target.symbol, "current": self.current, "address": _addr_key(self.address) or None,
                "section": self.section, "owner": self.owner, "owner_state": self.owner_state,
                "header": self.header, "new_name": self.new_name, "stale": self.stale,
                "cpp_linkage": self.cpp_linkage, "notes": list(self.notes)}


def unit_stem(unit: str) -> str:
    return os.path.splitext(unit.replace("\\", "/"))[0]


def source_name(map_name: str) -> str:
    """The identifier source code spells for a map name: a free function's linkage stem (`ck_option_cfg` for
    `ck_option_cfg__FUc`), the name itself for a C symbol. A member's mangling keeps its full spelling - a member
    is called through its owner, never by a bare name."""
    if "__F" in map_name and libnames.is_mangled(map_name):
        stem = map_name.split("__F", 1)[0]
        if _C_IDENT_RE.match(stem) and not is_member(map_name):
            return stem
    return map_name


def is_member(map_name: str | None) -> bool:
    """A class member's mangling (`name__<n><Class>F...`, a ctor/dtor) rather than a free function's."""
    return bool(map_name) and re.search(r"__(?:\d+[A-Za-z_]|Q\d|ct__|dt__)", map_name) is not None


def owner_header(unit: str | None, band: str | None = None, root: str | None = None) -> str | None:
    """`src/<unit stem>.h` (beside the unit's source) for a unit, `src/unsplit/<Band>.h` for an unowned symbol in a
    band - the roots `lib.repo.header_root`/`lib.project.ownership.band_root` give for the tree."""
    if unit:
        return "%s/%s.h" % (header_root(root), unit_stem(unit))
    if band:
        return "%s/%s.h" % (band_root(root), band.split("/")[-1])
    return None


_MANGLED_INDEX: dict[int, dict[str, list[str]]] = {}


def mangled_index(ownership) -> dict[str, list[str]]:
    """`{linkage stem: [mangled map names]}` for a free function's mangling (`name__F...`), built once per index."""
    key = id(ownership)
    if key not in _MANGLED_INDEX:
        out: dict[str, list[str]] = {}
        for name in ownership.symbols:
            if "__F" in name and libnames.is_mangled(name):
                stem = name.split("__F", 1)[0]
                if stem and _C_IDENT_RE.match(stem):
                    out.setdefault(stem, []).append(name)
        _MANGLED_INDEX.clear()
        _MANGLED_INDEX[key] = out
    return _MANGLED_INDEX[key]


def resolve_target(t: Target, ownership, decisions: dict[str, str] | None = None) -> Resolved:
    """Who owns `t` today: by name when the map still has it, else by address (a lane's `fn_X` that the map has
    since renamed resolves to the new name, and is `stale`)."""
    decisions = decisions or {}
    r = Resolved(t)
    names_to_try = [t.symbol] + ([t.alias] if t.alias else [])
    hit = None
    for name in names_to_try:
        entries = ownership.symbols.get(name) if name else None
        if entries and len(entries) == 1:
            hit = (name, entries[0])
            break
    if hit is None and t.address is None and _C_IDENT_RE.match(t.symbol or ""):
        # a C++ function filed by its plain name (`ck_option_cfg` for `ck_option_cfg__FUc`): one mangled row
        stems = [n for n in mangled_index(ownership).get(t.symbol, ()) if len(ownership.symbols[n]) == 1]
        if len(stems) == 1:
            hit = (stems[0], ownership.symbols[stems[0]][0])
        elif len(stems) > 1:
            r.notes.append("%d C++ overloads named %s: %s" % (len(stems), t.symbol, ", ".join(stems[:3])))
    if hit is None and t.address is not None:
        for section in ([t.section] if t.section else []) + [".text", ".data", ".bss", ".sbss", ".sdata", ".rodata",
                                                             ".sdata2", ".sbss2"]:
            name = ownership.name_at(section, t.address)
            if name:
                hit = (name, ownership.symbols[name][0])
                break
    if hit is None:
        r.notes.append("not in the map (by name%s)" % (" or address" if t.address is not None else ""))
        r.new_name = decision_for(t, decisions) or t.proposed_name
        return r
    name, (section, address, _type) = hit
    r.current, r.section, r.address = name, section, address
    r.stale = name not in (t.symbol, t.alias) and source_name(name) != t.symbol
    r.cpp_linkage = libnames.is_mangled(name)
    owner = ownership.owner_of(section, address)
    r.owner_state = owner.state
    if owner.state in ("registered", "reconstructed"):
        r.owner = owner.unit
        r.header = owner_header(owner.unit, root=getattr(ownership, "root", None))
    elif owner.state == "unsplit":
        r.owner = None
        r.header = owner_header(None, owner.band, root=getattr(ownership, "root", None))
        if owner.band is None:
            r.notes.append("unowned and between two modules: no band header can be named")
    else:
        r.owner = owner.unit
        r.notes.append("owned by an auto object (%s)" % owner.unit)
    decided = decision_for(t, decisions)
    if decided:
        r.new_name = decided
    elif t.proposed_name and t.proposed_name != name:
        r.new_name = t.proposed_name
    return r


def resolve(req: Request, ownership, decisions: dict[str, str] | None = None) -> list[Resolved]:
    out = [resolve_target(t, ownership, decisions) for t in req.targets]
    if req.owner_unit:
        for r in out:
            if r.owner and unit_stem(r.owner) != unit_stem(req.owner_unit) \
                    and not unit_stem(r.owner).endswith(unit_stem(req.owner_unit)):
                r.notes.append("the request names owner %s; the map says %s (the map wins)" % (req.owner_unit, r.owner))
    return out


def live_owner(resolved: Iterable[Resolved], live_units: Iterable[str]) -> str | None:
    """The first owner unit among `resolved` a live lane holds (claims), or None."""
    live = {unit_stem(u) for u in live_units}
    for r in resolved:
        if r.owner and unit_stem(r.owner) in live:
            return r.owner
    return None


# --- status sidecar --------------------------------------------------------------------------------------------

def status_path(requests_path: str) -> str:
    base = requests_path[:-len(REQUESTS_SUFFIX)] if requests_path.endswith(REQUESTS_SUFFIX) \
        else os.path.splitext(requests_path)[0]
    return base + STATUS_SUFFIX


def load_status(requests_path: str) -> dict[str, dict]:
    path = status_path(requests_path)
    if not os.path.exists(path):
        return {}
    try:
        data = json.loads(libtext.read_text(path))
    except (OSError, json.JSONDecodeError):
        return {}
    return data if isinstance(data, dict) else {}


def write_status(requests_path: str, updates: dict[str, dict], now: str | None = None) -> dict:
    """Merge `{id: {status, note, ...}}` into the sidecar (atomic); the request file itself is never edited."""
    data = load_status(requests_path)
    stamp = now or datetime.datetime.now().isoformat(timespec="seconds")
    for rid, row in updates.items():
        if row.get("status") not in STATUSES:
            raise ValueError("status %r is not one of %s" % (row.get("status"), STATUSES))
        data[rid] = dict(row, at=stamp)
    libtext.atomic_write(status_path(requests_path), json.dumps(data, indent=1, sort_keys=True) + "\n")
    return data


def open_ids(directories: Iterable[str]) -> set[str]:
    """Every request id under `directories` whose sidecar status is not applied/rejected - the ids a STOPGAP
    block may name."""
    out: set[str] = set()
    for d in directories:
        if not d or not os.path.isdir(d):
            continue
        for path in request_files(d):
            status = load_status(path)
            for req in load_file(path):
                if status.get(req.id, {}).get("status") not in ("applied", "rejected"):
                    out.add(req.id)
    return out


def stopgap_blocks(text: str) -> list[tuple[str, int, int]]:
    """`(id, start, end)` for every complete STOPGAP-BEGIN/END pair in `text` (end exclusive, the END line's
    newline included)."""
    return [(m.group("id"), m.start(), m.end()) for m in STOPGAP_BLOCK_RE.finditer(text)]


def unpaired_stopgaps(text: str) -> list[tuple[str, int]]:
    """`(id, offset)` of a BEGIN without its END (or an END without its BEGIN) - a block integrate cannot delete."""
    blocks = stopgap_blocks(text)
    out = []
    for rx in (STOPGAP_BEGIN_RE, STOPGAP_END_RE):
        for m in rx.finditer(text):
            if not any(i == m.group(1) and s <= m.start() < e for i, s, e in blocks):
                out.append((m.group(1), m.start()))
    return sorted(out, key=lambda x: x[1])


def schema_rows() -> list[dict]:
    """The schema table as fresh dicts (what brief.py renders)."""
    return [dict(r) for r in SCHEMA]
