"""List function names declared more than once with *different text* in one source file's include closure.

Problem it solves: a unit that legitimately needs a callee owned by another unit must `#include` that
unit's header.  When the band headers (`include/unsplit/*.h`, `include/ef.h`, ...) still carry their own
copy of the same declaration, MWCC stops with `(10197) illegal function overloading` at the *first*
clashing line and `-maxerrors 1` hides the rest - so the cost of a cross-unit lane reads as one error
after another instead of a finite list.  This tool turns that into the list, before any source is edited.

It is descriptive, not authoritative: it compares declaration *text* (with parameter names and
`struct `/`extern ` spelling normalised away), so it is conservative in both directions.

  * `DIFFERENT` names are worth inspecting but may still be the same type (`VEC3*` vs
    `nw4r::math::VEC3*` is one type when `VEC3` is that typedef).
  * `SAME` names differ only in whitespace, parameter names or the `struct `/`extern ` keyword.
  * C vs C++ *linkage* cannot be seen here (a declaration inside an `extern "C"` block and one outside it
    are different symbols); read the file when a pair matters.

Usage:
    python tools/units/declclash.py src/menu/fn_802E4978.cpp
    python tools/units/declclash.py --only-different --json src/hud/layout.cpp
    python tools/units/declclash.py include/unsplit/menu.h
    python tools/units/declclash.py --selftest

Exit status is 0 for a report and 1 only with `--fail-on-different` when a `DIFFERENT` name was found.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import collections
import io
import json
import os
import re
import sys

from tools.lib import cli, cscan

INCLUDE_ROOTS = ("include", os.path.join("build", "RMHE08", "include"))

# A one-line function declaration: type(s), name, parameter list, `;` and an optional trailing comment.
# Definitions (a `{` on the line) and multi-line prototypes are deliberately not matched.
DECL_RE = re.compile(
    r'^\s*(?P<sig>(?:extern\s+|static\s+|inline\s+)*[A-Za-z_][\w :*&<>,]*?\b(?P<name>[A-Za-z_]\w*)\s*'
    r'\((?P<params>[^;{}\n]*)\))\s*;\s*(?:/\*.*)?$',
    re.M,
)

# A statement that starts like a declaration: `return fn(a);` matches DECL_RE's shape, and so does a
# bare macro call, so the head has to look like a type (two words, or a pointer star) and not like a
# keyword.  Scan of a source file's own body needs this; nothing else stops it.
STATEMENT_KEYWORDS = frozenset(
    """return if while for switch else do goto sizeof case break continue typedef using throw new
    delete static_assert catch try""".split()
)


def looks_like_declaration(sig):
    head = sig.split("(", 1)[0].strip()
    if " " not in head and "*" not in head and "&" not in head:
        return False  # a lone name is a call, not a declaration
    return head.split()[0] not in STATEMENT_KEYWORDS

NORMALISE = (
    (re.compile(r"\bstruct\s+"), ""),
    (re.compile(r"\bclass\s+"), ""),
    (re.compile(r"\benum\s+"), ""),
    (re.compile(r"\bextern\b"), ""),
    (re.compile(r"\s+"), " "),
)


def read_text(path):
    with io.open(path, "r", encoding="utf-8", newline="") as fh:
        return fh.read().replace("\r\n", "\n")


def resolve(header, roots):
    """The file an `#include "..."` names, searched in the source tree then in the generated tree."""
    return cscan.resolve_include(header, roots)


def closure(start, roots):
    """Every file reachable from `start` through `#include "..."`, `start` first, each file once."""
    return cscan.include_closure(start, lambda name, _includer: resolve(name, roots), read=read_text)


def shape(text):
    """The declaration's type shape: return type and parameter types, without parameter names."""
    text = text.strip()
    for pattern, replacement in NORMALISE:
        text = pattern.sub(replacement, text)
    open_paren = text.find("(")
    if open_paren < 0:
        return text
    head = text[:open_paren].strip()
    params = text[open_paren + 1 : text.rfind(")")].strip()
    if params in ("", "void"):
        return head + "()"
    shaped = []
    for param in params.split(","):
        param = param.strip()
        words = param.split(" ")
        # A trailing bare identifier is the parameter's name; everything in front of it is its type.
        if len(words) > 1 and re.fullmatch(r"[A-Za-z_]\w*", words[-1]):
            param = " ".join(words[:-1])
        shaped.append(param.strip())
    return "%s(%s)" % (head, ", ".join(shaped))


def declarations(files):
    """name -> {shape -> [file, ...]} for every one-line declaration in `files`."""
    found = collections.defaultdict(lambda: collections.defaultdict(list))
    for path in files:
        try:
            text = read_text(path)
        except OSError:
            continue
        for match in DECL_RE.finditer(text):
            if not looks_like_declaration(match.group("sig")):
                continue
            found[match.group("name")][shape(match.group("sig"))].append(path)
    return found


def report(paths, roots, only_different=False):
    """The findings for `paths`: one entry per name with more than one distinct declaration shape."""
    findings = []
    for path in paths:
        if not os.path.isfile(path):
            findings.append({"root": path, "error": "no such file", "names": []})
            continue
        files = closure(path, roots)
        for name, shapes in sorted(declarations(files).items()):
            # A name is a finding when it is declared more than once at all - twice with the same
            # shape is the SAME case, and it is what a duplicate-declaration cleanup deletes.
            if sum(len(f) for f in shapes.values()) < 2:
                continue
            different = len(shapes) > 1
            if only_different and not different:
                continue
            findings.append(
                {
                    "root": path,
                    "name": name,
                    "kind": "DIFFERENT" if different else "SAME",
                    "closure_size": len(files),
                    "shapes": [
                        {"shape": s, "files": sorted(set(f))} for s, f in sorted(shapes.items())
                    ],
                }
            )
    return findings


def _selftest():
    import declclash_selftest

    return declclash_selftest.selftest()


TOOL = cli.Tool("declclash", "docs/tools/spec/declclash.md", tests=_selftest, common=("root", "json"))


def main(argv=None):
    parser = TOOL.parser(description=__doc__.splitlines()[0])
    parser.add_argument("paths", nargs="*", help="source or header files to scan")
    parser.add_argument(
        "--only-different",
        action="store_true",
        help="hide names whose declarations differ only by parameter names/`struct`",
    )
    parser.add_argument(
        "--fail-on-different",
        action="store_true",
        help="exit 1 when a DIFFERENT name was found (for a gate)",
    )
    return TOOL.run(lambda args: _main(parser, args), argv, parser)


def _main(parser, args):
    if not args.paths:
        parser.error("at least one path is required (or --selftest)")

    roots = [os.path.join(args.root, r) for r in INCLUDE_ROOTS]
    findings = report(args.paths, roots, args.only_different)

    if args.json:
        print(json.dumps({"findings": findings}, indent=2))
    else:
        for finding in findings:
            if "error" in finding:
                print("%s: %s" % (finding["root"], finding["error"]))
                continue
            print("%s  (%d files in the closure)" % (finding["root"], finding["closure_size"]))
            print("--- %s  %s" % (finding["name"], finding["kind"]))
            for entry in finding["shapes"]:
                print("    %-60s <= %s" % (entry["shape"][:60], ", ".join(entry["files"])))
        counts = collections.Counter(f["kind"] for f in findings if "kind" in f)
        print(
            "%d name(s) with more than one declaration: %d DIFFERENT, %d SAME"
            % (sum(counts.values()), counts["DIFFERENT"], counts["SAME"])
        )

    if args.fail_on_different and any(f.get("kind") == "DIFFERENT" for f in findings):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
