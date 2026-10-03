#!/usr/bin/env python3
"""Sweep landed units for **wrong-linkage** symbols: our object vs the target object.

Why this tool exists (measured 2026-09-24/25). A landed unit (`fn_80059550`) shipped emitting *seven
unmangled* symbols where the target has C++ manglings, because its callees were declared without the
`extern` keyword inside an `extern "C" {}` block. The score could not see it: this project's objdiff
runs with `functionRelocDiffs=none`, so a unit can sit at 98 % while referring to a symbol under the
wrong name. (The reverse direction is just as real and just as hidden: `extern void fn_XXXX(Vec3*)` in a
`.cpp` is *C++* linkage and mangles to `fn_XXXX__FP4Vec3`, while the map's name is the plain
`fn_XXXX`.) Linkage is decided by the *spelling* of an undefined symbol, and only the target object
records the true spelling.

What is compared. For each registered unit, the **undefined** symbol set of our object
(`build/RMHE08/src/<unit>.o`) against the target object (`build/RMHE08/obj/<unit>.o`):

* a name our object references that the target object neither references nor defines (under that
  spelling) is a mismatch. A symbol the target *defines* is not a mismatch - a partial unit may call a
  function that lives in the same original TU and is therefore a definition there;
* the mismatch is **linkage** when the target has a symbol with the same *linkage stem* (the name
  before MWCC's `__<args>` suffix, so `drawSpr2TF__FUc...` and `drawSpr2TF` share the stem
  `drawSpr2TF`). This is the wrong-linkage defect and the repair is mechanical: fix the declaration's
  linkage in the symbol's owner header;
* otherwise it is **other** - our object references a name the target never mentions, so it is a
  genuine extra/unwritten reference, not a spelling.

The same direction is checked for **defined** global symbols, cheaply: a unit that *defines* a name the
target does not define at that spelling (e.g. a definition that mangles while the map name is plain).

What is deliberately **excluded**, and why:

* **locals** (`STB_LOCAL`, and MWCC's lowercase `...data.N`/`.L...` rows) - a static/internal symbol
  cannot satisfy an external reference and its spelling is a compiler naming choice, not a linkage;
* **`@NNN` / `@etb_*` compiler labels** - MWCC-generated constant/jump-table and extab labels; they
  carry no linkage and move with register allocation and layout;
* **`STT_FILE`** - the synthesised source-file name (`ef_cube.cpp`); it echoes the configured
  extension and is circular evidence;
* **`.comment`** - the compiler version string; not a symbol;
* **section extab/extabindex naming and every other section-level row** - the objdiff metric already
  covers section layout, and the point of this tool is linkage, not section noise.

Usage:

    python tools/units/relocaudit.py                 # every registered unit, suspect list + counts
    python tools/units/relocaudit.py --json          # same, machine-readable
    python tools/units/relocaudit.py --unit ef/ef_cube.cpp
    python tools/units/relocaudit.py --no-decls      # skip the src/header declaration lookup
    python tools/units/relocaudit.py --selftest

`--main` points the sweep at a tree that holds `configure.py` and `build/` (default: the tree this file
lives in), which is how a worktree can audit MAIN's already-built objects without a build of its own.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os
import re
import struct
import sys
import time
from tools.lib import names as libnames

from tools.lib.binary.elf import Elf as LibElf, ElfError

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)                      # so `import langcheck` works from any cwd
ROOT = os.path.dirname(os.path.dirname(HERE))

# ELF bits we need. An object this project links is ELF32, big-endian, and carries a real `.symtab`.
STB_LOCAL, STB_GLOBAL, STB_WEAK = 0, 1, 2
STT_FILE = 4
SHN_UNDEF = 0


# EABI register-save/restore helpers. They are compiler-generated references that encode register
# allocation (a different prologue saves a different `_savegpr_N`), not linkage, so they are reported
# separately and never listed as a linkage repair.
COMPILER_HELPER_RE = re.compile(r"^_(?:save|rest)(?:gpr|fpr)_\d+$")


def mismatch_kind(name: str) -> str:
    """`linkage` is set by the caller; here `extra` or `compiler-helper` for an `other` row."""
    return "compiler-helper" if COMPILER_HELPER_RE.match(name) else "extra"

# The source/header extensions the declaration lookup reads.
DECL_EXT = {".c", ".cpp", ".cc", ".cxx", ".cp", ".h", ".hpp", ".hh"}
DECL_CAP = 4


# --------------------------------------------------------------------------------------------------
# the pure rule
# --------------------------------------------------------------------------------------------------
def linkage_stem(name: str) -> str:
    return libnames.linkage_stem(name)


def audit_sets(our_defined, our_undefined, tgt_defined, tgt_undefined) -> dict:
    """Classify the symbols our object emits that the target does not, under the same spelling.

    Pure, so the selftest drives it with plain sets. `*_defined` are global/weak definitions only;
    `*_undefined` are external references. A target **definition** satisfies one of our references - a
    partial unit may call a function that lives in the same original TU - so it is not a mismatch.

    Returns four sorted lists of `{"our": name, "target": [names with the same stem]}`:
    `linkage_undefined`, `other_undefined`, `linkage_defined`, `other_defined`. A `target` list that is
    non-empty is the linkage signal; an empty one is a name the target never mentions.
    """
    tgt_names = set(tgt_defined) | set(tgt_undefined)

    def classify(ours, target_has):
        linkage, other = [], []
        for s in sorted(ours - target_has):
            cands = sorted(t for t in tgt_names if t != s and linkage_stem(t) == linkage_stem(s))
            (linkage if cands else other).append({"our": s, "target": cands})
        return linkage, other

    link_u, other_u = classify(set(our_undefined), tgt_names)
    link_d, other_d = classify(set(our_defined), set(tgt_defined))
    return {"linkage_undefined": link_u, "other_undefined": other_u,
            "linkage_defined": link_d, "other_defined": other_d}


# --------------------------------------------------------------------------------------------------
# the object reader (pure Python; no binutils dependency)
# --------------------------------------------------------------------------------------------------
def read_symbols(path: str):
    """`[{name, value, size, bind, type, shndx}]` for an ELF32 big-endian object, or `None` if it is not one.

    `None` (not `[]`) distinguishes "unreadable/not an object" from "an object with no symbols", so a
    missing build artefact is reported as unbuilt rather than as an empty, falsely-clean unit.
    """
    try:
        elf = LibElf.read(path)
    except (OSError, ElfError):
        return None
    if elf.ei_class != 1 or elf.ei_data != 2:
        return None
    return [{"name": s.name, "value": s.value, "size": s.size, "bind": s.bind, "type": s.type, "shndx": s.shndx}
            for s in elf.symbols if s.name]





def object_sets(path: str):
    """`(defined_global, undefined)` for an object, or `None` when it is not an object.

    The exclusions live here: `STT_FILE` rows, `@`-prefixed compiler labels, `STB_LOCAL` definitions
    (so `...data.0`, `.L...` and other locals never enter), and any empty name. `defined_global` keeps
    `STB_GLOBAL` and `STB_WEAK` - the rows that can satisfy an external reference and therefore carry
    linkage. `_savegpr_N`-style EABI helpers *are* undefined here; `mismatch_kind` tags them so the
    report can say they are register allocation, not linkage.
    """
    syms = read_symbols(path)
    if syms is None:
        return None
    defined, undefined = set(), set()
    for s in syms:
        n = s["name"]
        if not n or n.startswith("@") or s["type"] == STT_FILE:
            continue
        if s["shndx"] == SHN_UNDEF:
            undefined.add(n)
        elif s["bind"] in (STB_GLOBAL, STB_WEAK):
            defined.add(n)
    return defined, undefined


# --------------------------------------------------------------------------------------------------
# the units and their objects
# --------------------------------------------------------------------------------------------------
def registered_units(main: str) -> list[dict]:
    """Every registered unit from `configure.py` (via `langcheck.registered_units`).

    Reused rather than re-parsed: the two tools must agree on what "a registered unit" is, and
    `langcheck` is the one place that reads `config.libs` with its inline `cflags=` overrides.
    """
    import langcheck
    return langcheck.registered_units(main)


def object_paths(main: str, source_path: str) -> tuple[str, str]:
    """`(our_object, target_object)` for a registered source path.

    The convention (verified to match every `base_path`/`target_path` in the tree's `objdiff.json`):
    the source path minus its extension, under `build/RMHE08/src/` (ours) and `build/RMHE08/obj/`
    (the target object `dtk` split out of the original).
    """
    stem = os.path.splitext(source_path.replace("\\", "/"))[0]
    return (os.path.join(main, "build", "RMHE08", "src", stem + ".o"),
            os.path.join(main, "build", "RMHE08", "obj", stem + ".o"))


# --------------------------------------------------------------------------------------------------
# where is this symbol declared? (mechanical repair aid)
# --------------------------------------------------------------------------------------------------
def declaration_index(main: str) -> dict:
    """`{token: [(relpath, lineno, text)]}` for every identifier in `src/` and `include/`.

    One walk, so a 100-symbol suspect list does not shell out to grep 100 times. The repair for a
    wrong-linkage row is "declare this callee in its owner header with the true linkage", and the rows
    this returns are where the callable identifier is spelled out today.
    """
    idx: dict[str, list] = {}
    for base in ("src", "include"):
        root = os.path.join(main, base)
        if not os.path.isdir(root):
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [d for d in dirnames if d != "__pycache__"]
            for fn in filenames:
                if os.path.splitext(fn)[1].lower() not in DECL_EXT:
                    continue
                p = os.path.join(dirpath, fn)
                try:
                    text = open(p, encoding="utf-8", errors="replace").read()
                except OSError:
                    continue
                rel = os.path.relpath(p, main).replace("\\", "/")
                for lineno, line in enumerate(text.splitlines(), 1):
                    stripped = line.strip()
                    # comment-only lines mention a symbol without declaring it, so they would send the
                    # repair to the wrong file: the declaration is a line with `(`/`;` and no leading `*`.
                    # (A trailing comment on a real declaration line is kept - the line right here.)
                    if not stripped or stripped[0] in "*/#" or ("(" not in line and ";" not in line):
                        continue
                    for tok in set(re.findall(r"[A-Za-z_]\w*", line)):
                        idx.setdefault(tok, []).append((rel, lineno, stripped))
    return idx


def declarations_for(idx: dict, name: str) -> list[dict]:
    """Up to `DECL_CAP` src/header rows that spell `name`'s linkage stem, header rows first."""
    rows = idx.get(linkage_stem(name), [])
    rows = sorted(rows, key=lambda r: (0 if r[0].endswith((".h", ".hpp", ".hh")) else 1, r[0], r[1]))
    return [{"file": f, "line": n, "text": t} for f, n, t in rows[:DECL_CAP]]


# --------------------------------------------------------------------------------------------------
# the sweep
# --------------------------------------------------------------------------------------------------
def audit_unit(main: str, source_path: str, idx: dict | None = None) -> dict:
    """One unit's audit record. `status` is `clean`, `suspect` or `unbuilt`."""
    our_obj, tgt_obj = object_paths(main, source_path)
    our = object_sets(our_obj)
    tgt = object_sets(tgt_obj)
    if our is None or tgt is None:
        missing = [p for p, s in ((our_obj, our), (tgt_obj, tgt)) if s is None]
        return {"unit": source_path, "status": "unbuilt", "our": our_obj, "target": tgt_obj,
                "missing": [os.path.relpath(p, main).replace("\\", "/") for p in missing],
                "linkage_undefined": [], "other_undefined": [],
                "linkage_defined": [], "other_defined": []}
    rec = audit_sets(our[0], our[1], tgt[0], tgt[1])
    for key in ("linkage_undefined", "linkage_defined"):
        for m in rec[key]:
            m["kind"] = "linkage"
    for key in ("other_undefined", "other_defined"):
        for m in rec[key]:
            m["kind"] = mismatch_kind(m["our"])
    suspect = any(rec[k] for k in rec)
    rec["status"] = "suspect" if suspect else "clean"
    rec["unit"] = source_path
    rec["our"] = os.path.relpath(our_obj, main).replace("\\", "/")
    rec["target"] = os.path.relpath(tgt_obj, main).replace("\\", "/")
    if idx is not None:
        rec["declarations"] = {
            m["our"]: declarations_for(idx, m["our"])
            for key in ("linkage_undefined", "other_undefined",
                        "linkage_defined", "other_defined")
            for m in rec[key]
        }
    return rec


def sweep(main: str, with_decls: bool = True, only: str | None = None) -> dict:
    """Audit every registered unit. Returns the counts, the suspect records and the wall-clock cost."""
    main = os.path.abspath(main)
    t0 = time.time()
    units = registered_units(main)
    if only:
        want = only.replace("\\", "/").strip("/")
        units = [u for u in units
                 if u["path"] == want or os.path.splitext(u["path"])[0] == os.path.splitext(want)[0]]
    idx = declaration_index(main) if with_decls else None
    records = [audit_unit(main, u["path"], idx) for u in units]
    suspects = [r for r in records if r["status"] == "suspect"]
    elapsed = time.time() - t0
    return {
        "root": main,
        "units_total": len(records),
        "units_built": sum(1 for r in records if r["status"] != "unbuilt"),
        "unbuilt": [r for r in records if r["status"] == "unbuilt"],
        "clean": sum(1 for r in records if r["status"] == "clean"),
        "undefined_suspects": sum(1 for r in suspects
                                  if r["linkage_undefined"] or r["other_undefined"]),
        "defined_suspects": sum(1 for r in suspects
                                if r["linkage_defined"] or r["other_defined"]),
        "suspects": suspects,
        "elapsed_s": round(elapsed, 3),
    }


# --------------------------------------------------------------------------------------------------
# the report
# --------------------------------------------------------------------------------------------------
def render_row(m: dict) -> str:
    link = ", ".join(m["target"]) or "-"
    return "    our %-56s -> target %s" % (m["our"], link)


def render(s: dict, out=sys.stdout) -> None:
    print("relocaudit: %d registered units, %d built, %d clean, %d suspect "
          "(elapsed %.2fs)"
          % (s["units_total"], s["units_built"], s["clean"], len(s["suspects"]), s["elapsed_s"]),
          file=out)
    print("  suspects with an UNDEFINED-set disagreement: %d" % s["undefined_suspects"], file=out)
    print("  suspects with a DEFINED-set disagreement:   %d" % s["defined_suspects"], file=out)
    if s["unbuilt"]:
        print("  unbuilt (no object to compare): %d" % len(s["unbuilt"]), file=out)
        for r in s["unbuilt"]:
            print("    %-48s missing %s" % (r["unit"], ", ".join(r["missing"])), file=out)
    if not s["suspects"]:
        print("  no suspect units.", file=out)
        return
    for r in s["suspects"]:
        print("", file=out)
        print("  %s" % r["unit"], file=out)
        for label, key in (("wrong linkage (undefined)", "linkage_undefined"),
                           ("extra reference, no target spelling (undefined)", "other_undefined"),
                           ("wrong linkage (defined)", "linkage_defined"),
                           ("stray definition, no target spelling (defined)", "other_defined")):
            for m in r[key]:
                if key.startswith("other") and m.get("kind") == "compiler-helper":
                    print("  compiler register-save helper (register allocation, not linkage):", file=out)
                else:
                    print("  %s:" % label, file=out)
                print(render_row(m), file=out)
                for d in (r.get("declarations", {}).get(m["our"]) or []):
                    print("      declared at %s:%d  %s" % (d["file"], d["line"], d["text"]), file=out)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--main", default=None, help="tree holding configure.py and build/ (default: this file's)")
    ap.add_argument("--unit", default=None, help="one registered unit (path, with or without extension)")
    ap.add_argument("--no-decls", action="store_true", help="skip the src/header declaration lookup")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        import relocaudit_selftest
        return relocaudit_selftest.selftest()
    s = sweep(args.main or ROOT, with_decls=not args.no_decls, only=args.unit)
    if args.json:
        print(json.dumps(s, indent=2))
    else:
        render(s)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
