"""Rule 7: no generated name survives - a generator's stem or a DOL address in any identifier, a bare `unk*` - whoever
owns it, in every `.c`/`.cpp`/`.h`. Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package;
`stylelint.py` is the CLI)."""
from __future__ import annotations

import re

from tools.lib import findings as _findings
from tools.lib import names as _names
from tools.units.stylelint_rules.common import Field, Source, _finding

# The exact spellings of the three classic stems, kept for importers (`api`); the rule itself reads `lib.names`
# (`generated_name_kind`), the one implementation the file-name findings and the gate's new-unit row share.
RULE7_FN_RE = re.compile(r"\bfn_[0-9A-Fa-f]{8}\b")
RULE7_UNK_RE = re.compile(r"\bunk\w*\b")
RULE7_LBL_RE = re.compile(r"\b(?:lbl|loc)_[0-9A-Fa-f]{8}\b")
# An identifier token: not preceded by a word character, so the `x80276B58` inside the literal `0x80276B58` and the
# suffix of `1e5f` are never tokens.
TOKEN_RE = re.compile(r"(?<![\w$])[A-Za-z_]\w*")
# An `#include` line: its path is retired by renaming the file (a file-name finding), never counted as a name.
INCLUDE_LINE_RE = re.compile(r"^[ \t]*#[ \t]*include\b[^\n]*", re.M)
_UNK_RE = re.compile(r"unk\w*")


def name_detail(token: str, kind: str) -> str:
    """The detail text for a generated identifier - a function of the token alone, so the identity `(7, file, token,
    detail)` is `(7, file, token)` and a rename (`renamed_finding`) credits it."""
    if kind == "address":
        return ("address-named identifier `%s` - a name spelled from a DOL address is a generated name; name it from "
                "what it is" % token)
    stem = _names.RULE7_STEM_RE.search(token).group(0)
    if stem.startswith(("lbl_", "loc_")):
        return ("data label `%s` - name it from what it holds and where it is used, and rename the map row" % token)
    return "auto-generated name `%s`" % token


def path_detail(component: str, is_dir: bool) -> str:
    """The detail of a file-name finding - a function of the component alone, like `name_detail`."""
    return ("generated %s name `%s` - rename it from what it holds (a `git mv` to a named path retires it)"
            % ("directory" if is_dir else "file", component))


def path_findings(src: Source) -> list[dict]:
    """One rule-7 finding per generated component of the path the file really has (`src.origin`): each directory
    (`include/fn_8004CAD8/`) and the file's stem (`fn_805113B0.cpp` -> `fn_805113B0`), judged by
    `lib.names.generated_name_kind` exactly as an identifier is.

    The finding is filed under `src.rel` (the path the working tree spells) with the component as its token, so its
    identity is `(7, file now, component)`: a `git mv` to a clean stem removes it and adds nothing, a move that keeps
    the generated component (a header moved from `include/` to `src/`) keeps the identity, and a move to another
    generated name is that name's addition. Reported on line 1 with the path as its text: the defect is the name."""
    return [_findings.Finding(7, src.rel, 1, comp, path_detail(comp, is_dir), text=src.origin).to_dict()
            for comp, is_dir in _names.generated_path_components(src.origin)]


def findings(src: Source, fields: list[Field]) -> list[dict]:
    """Every generated identifier occurrence, unconditionally: no path is exempt, a file with no bodies is held to it,
    a comment exempts nothing - whoever owns the symbol. Comments, string literals and `#include` lines are not code.
    A field named `unk*` is rule 5's (`fields`); a bare `unk*` anywhere else is rule 7's, as before."""
    spans = [f.span for f in fields if f.span is not None]

    def in_field(pos: int) -> bool:
        return any(a <= pos < b for a, b in spans)

    code = INCLUDE_LINE_RE.sub(lambda m: " " * len(m.group(0)), src.code)
    out = []
    for m in TOKEN_RE.finditer(code):
        tok = m.group(0)
        if _UNK_RE.fullmatch(tok):
            if not in_field(m.start()):
                out.append(_finding(src, 7, src.line_of(m.start()), "bare `%s` identifier" % tok, token=tok))
            continue
        kind = _names.generated_name_kind(tok)
        if kind:
            out.append(_finding(src, 7, src.line_of(m.start()), name_detail(tok, kind), token=tok))
    return out
