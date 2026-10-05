"""Rule 2: a declaration lives with the unit that owns the symbol (src/, module headers, the unsplit band, STOPGAP blocks).
Spec: docs/tools/spec/stylelint_rules.md. CLI: none (the stylelint package; `stylelint.py` is the CLI)."""
from __future__ import annotations

import os
import re

from tools.lib import cscan
from tools.lib import requests as _requests
from tools.lib.project.ownership import Ownership as _Ownership
from tools.units.stylelint_rules.common import (
    HEADER_SUFFIXES, SRC, Source, UNSPLIT_UNRESOLVED, _finding, _rule2_finding, is_unsplit_header,
)
from tools.units.stylelint_rules.context import open_request_ids


EXTERN_RE = re.compile(r"\bextern\b")


def _declared_name(segment: str) -> "str | None":
    """The identifier an `extern` declaration introduces (`lib.cscan.declared_name`)."""
    return cscan.declared_name(segment)


def extern_declarations(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every `extern` function/variable declaration in the file.

    A definition (`extern "C" void f(void) { ... }`) is skipped: it defines the symbol, so the file owns
    it by construction. A linkage block (`extern "C" {`) has no name and is skipped too.
    """
    out = []
    for m in EXTERN_RE.finditer(src.code):
        j = m.end()
        while j < len(src.code) and src.code[j] not in ";{}":
            j += 1
        if j < len(src.code) and src.code[j] == "{":
            continue  # a definition, not a declaration
        name = _declared_name(src.code[m.end():j])
        if name is None:
            continue
        out.append((name, m.start(), src.line_of(m.start())))
    return out


def _owns(rel: str, unit: str) -> bool:
    """Whether `rel` is `unit`'s own source file or its own header.

    Two spellings: the translation unit itself (`src/<unit>`) and the unit's header - beside it
    (`src/<stem>.h`, every header's home since the owner's 2026-10-05 move) or at any depth whose path ends in
    `<stem>.h`, where `<stem>` is the unit's module-qualified stem (`include/**/<stem>.h` in a pre-move ref).
    The any-depth case is the one the original test missed: `_owns('include/NHTTP/NHTTP_bgnend.h', 'NHTTP/NHTTP_bgnend.c')` was False, so an owner's
    own header read as a foreign declaration and rule 2 could not be extended to headers without reporting
    ~30 owners' own headers in the Network scope alone (2026-09-28).  The match is by the module-qualified
    stem suffix, not the bare basename, so a same-named header in another module stays foreign.
    """
    rel = rel.replace("\\", "/")
    if rel == SRC + "/" + unit:
        return True
    stem = os.path.splitext(unit)[0]
    # the unit's header, wherever headers live: its path ends in the unit's module-qualified stem
    # (`src/Network/network_state.h` beside the source, `include/Network/network_state.h` before the move, for
    # `Network/network_state.cpp`). The suffix match keeps a same-basename header in another module
    # (`src/other/network_state.h`) out of the owner's set, which a bare-basename match would admit. The band
    # is never an owner's header.
    if is_unsplit_header(rel):
        return False
    return any(rel == SRC + "/" + stem + ext or rel.endswith("/" + stem + ext) for ext in HEADER_SUFFIXES)


_LINKAGE_OPEN_RE = cscan.LINKAGE_OPEN_RE
_TYPE_ONLY_RE = re.compile(r"^\s*(?:typedef\s+)?(?:struct|class|union|enum)\b")


def _file_scope_declarations(src: Source) -> list[tuple[str, str, int]]:
    """`(name, segment, segment_start)` for every file-scope statement that introduces a name.

    The walk `header_declarations` and `prototype_declarations` share.  An unsplit-band prototype sits
    inside `extern "C" { ... }`; `strip` blanks the `"C"`, so the linkage block is just a brace opened by
    an `extern`, and it is *transparent*: its contents are file scope, which is where declarations live.
    A file-scope statement ending in `;` that introduces a name is returned; a type forward declaration
    (`struct Foo;`) introduces no symbol from the map and is skipped; a function body's `{` is a real
    scope, so a definition is never returned.  Preprocessor lines are blanked first (`lib.cscan.mask_preproc`): a
    header's guard or `#include` would otherwise open the first statement's segment, so a forward declaration right
    after it (`#include "types.h"` / `struct Foo;`) no longer read as type-only and named `Foo` as a declaration.
    """
    out: list[tuple[str, str, int]] = []
    code = cscan.mask_preproc(src.code)
    depth = 0
    transparent: list[bool] = []
    stmt_start = 0
    for i, c in enumerate(code):
        if c == "{":
            linkage = bool(_LINKAGE_OPEN_RE.search(code[stmt_start:i]))
            transparent.append(linkage)
            if not linkage:
                depth += 1
            stmt_start = i + 1
        elif c == "}":
            if transparent:
                if not transparent.pop():
                    depth -= 1
            stmt_start = i + 1
        elif c == ";" and depth == 0:
            seg = code[stmt_start:i]
            seg_start = stmt_start
            stmt_start = i + 1
            if _TYPE_ONLY_RE.match(seg) and "(" not in seg:
                continue
            name = _declared_name(seg)
            if name is None:
                continue
            out.append((name, seg, seg_start))
    return out


def _seg_line(src: Source, name: str, seg: str, seg_start: int) -> int:
    """The line of `name` inside `seg`, falling back to the statement's first line."""
    m = re.search(r"\b%s\b" % re.escape(name), seg)
    return src.line_of(seg_start + (m.start() if m else 0))


def header_declarations(src: Source) -> list[tuple[str, int]]:
    """`(name, line)` for every declaration a header makes at file scope.

    See `_file_scope_declarations` for the walk.  A declaration here needs no `extern` keyword - an
    ordinary `void foo(void);` is the header's normal shape.
    """
    return [(name, _seg_line(src, name, seg, seg_start))
            for name, seg, seg_start in _file_scope_declarations(src)]


def prototype_declarations(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every plain function prototype at file scope (no `extern` keyword).

    The `extern`-keyword form is `extern_declarations`' job; this is the shape that scanner could not
    see - a plain prototype in a `.cpp`/`.c` (`void foo(void);`), which is how the Network scope's
    `memset`/`memcpy`/`DWCi_GetStringLength` sites are written.  A statement with no `(` introduces a
    variable, not a callable, and is left to the other rules; a statement that carries the `extern`
    keyword is excluded so the two scanners never report one site twice.
    """
    out = []
    for name, seg, seg_start in _file_scope_declarations(src):
        if "(" not in seg or EXTERN_RE.search(seg):
            continue
        out.append((name, seg_start, _seg_line(src, name, seg, seg_start)))
    return out


def declaration_sites(src: Source) -> list[tuple[str, int, int]]:
    """`(name, pos, line)` for every declaration rule 2 judges in a `src/` file: the union of the
    `extern`-keyword form and the plain function prototype, with one site never counted twice."""
    seen: set[tuple[str, int]] = set()
    out = []
    for name, pos, line in list(extern_declarations(src)) + prototype_declarations(src):
        if (name, pos) in seen:
            continue
        seen.add((name, pos))
        out.append((name, pos, line))
    return out


def rule2_band_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Declarations in `src/unsplit/<band>.h` of a symbol a registered unit already owns.

    The band exists for a symbol with no owner, so only an `owned` resolution is a finding; an unsplit
    name stays (that is the band's purpose), and a name the map cannot judge is left alone rather than
    counted - the band is where such names legitimately live. The message matches the `src/` rule
    exactly; a definition here is not a declaration (`header_declarations` never returns one).
    """
    out = []
    for name, line in header_declarations(src):
        r = ownership.resolve(name)
        if r is None or r["kind"] != "owned":
            continue
        out.append(_rule2_finding(src, line, name,
                                  "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                  "#include it" % (name, r["unit"])))
    return out


def rule2_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Every declaration the file makes for a symbol it does not own.

    Both declaration shapes are judged (`declaration_sites`): the `extern` keyword and the plain function
    prototype.  Three outcomes: `owned` by this file (no finding), owned by another registered unit (move
    the declaration to that unit's header and `#include` it), or `unsplit` (the symbol has no registered
    owner: move the declaration to `src/unsplit/<module>.h`, or - when the bracketing bands name
    different modules so no module is sound - to a header under `src/unsplit/`). A name not in the
    map and a name with duplicate map rows are left as counted gaps rather than guessed.
    """
    out = []
    for name, _pos, line in declaration_sites(src):
        r = ownership.resolve(name)
        if r is None:
            ownership.gaps["not in symbols.txt"] += 1
            continue
        if r["kind"] == "dup":
            ownership.gaps["duplicate symbol name in the map"] += 1
            continue
        if r["kind"] == "owned":
            if _owns(src.rel, r["unit"]):
                continue
            ownership.foreign_units[r["unit"]] += 1
            out.append(_rule2_finding(src, line, name,
                                      "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                      "#include it" % (name, r["unit"])))
            continue
        module = r["module"]
        if module is None:
            # The band cannot place this address: the registered units bracketing it name different
            # modules (a `sound` unit inside the `ef` band is the canonical case), so there is no sound
            # `<module>.h` to name. The local declaration is still the defect - a symbol with no
            # registered owner belongs in the band, not in a `src/` file - so it is reported without
            # guessing the module header (this used to be a counted `Ownership.gaps` entry).
            module = UNSPLIT_UNRESOLVED
        ownership.unsplit_modules[module] += 1
        ownership.unsplit_symbols.setdefault(module, set()).add(name)
        # the detail names the band by its MODULE, never by its path: a finding's identity carries the detail
        # (`lib.findings.identity`), and the band's directory moves (2026-10-05) - a path here would turn the move
        # into one removal plus one addition per finding.
        if module == UNSPLIT_UNRESOLVED:
            out.append(_rule2_finding(src, line, name,
                                      "`%s` has no registered owner - declare it in an unsplit band header "
                                      "(the bracketing bands name different modules)" % name))
        else:
            out.append(_rule2_finding(src, line, name,
                                      "`%s` has no registered owner - declare it in the `%s` unsplit band header"
                                      % (name, module)))
    return out


def leaf_header_owner(src: Source, names: list[str], ownership: "Ownership") -> "str | None":
    """The unit that owns `src` as a **leaf header**, or None.

    A leaf header is `src/<module>/<symbol>.h`: named for a symbol it declares, and declaring only
    symbols that one registered unit defines (map row inside the unit's ranges).  It exists because the
    owner's full header can redefine shared types and so cannot be included beside the consumer's.  The
    test is symbol-based and strict: one unresolved, unowned or duplicate name, or symbols of two units,
    and the header is not a leaf (no per-file exemption); the path-stem rule in `_owns` is unchanged.
    """
    return _Ownership.leaf_header_owner(ownership, src.rel, names)   # the one implementation (lib.project)


def rule2_header_findings(src: Source, ownership: "Ownership") -> list[dict]:
    """Declarations in an ordinary header for a symbol another registered unit owns.

    A header is not a unit - it is the public face of the unit(s) in its module - so a declaration there
    is clean only when `_owns` recognises the file as the owner's own header (`src/<module>/<stem>.h`,
    which the `_owns` fix made it do).  A declaration of a symbol another unit owns is a finding: the
    declaration belongs in that unit's header.

    A symbol with no registered owner is left alone here (unlike the `src/` reading): the detector for
    those is the unsplit band, and a module header carries many public names whose addresses the splits
    map has not registered yet - reporting them would bury the real foreign declarations.
    """
    out = []
    decls = header_declarations(src)
    leaf = leaf_header_owner(src, [n for n, _l in decls], ownership)
    for name, line in decls:
        r = ownership.resolve(name)
        if r is None or r["kind"] != "owned":
            continue
        if _owns(src.rel, r["unit"]) or r["unit"] == leaf:
            continue
        ownership.foreign_units[r["unit"]] += 1
        out.append(_rule2_finding(src, line, name,
                                  "`%s` is owned by `src/%s` - declare it in that unit's header and "
                                  "#include it" % (name, r["unit"])))
    return out


def stopgap_findings(src: "Source") -> list[dict]:
    """A STOPGAP block whose id is no open integrator request, and a BEGIN/END without its partner."""
    if "STOPGAP-" not in src.text:
        return []
    out = []
    open_ids = None
    for rid, start, _end in _requests.stopgap_blocks(src.text):
        if open_ids is None:
            open_ids = open_request_ids()
        if rid not in open_ids:
            out.append(_finding(src, 2, src.line_of(start),
                                "STOPGAP block `%s` names no open integrator request - file the request (or the "
                                "declarations go to their owner's header now)" % rid, token=rid))
    for rid, pos in _requests.unpaired_stopgaps(src.text):
        out.append(_finding(src, 2, src.line_of(pos), "STOPGAP marker `%s` has no matching BEGIN/END" % rid,
                            token=rid))
    return out
