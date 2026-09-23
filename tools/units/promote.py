#!/usr/bin/env python3
"""Promote a `src/auto` unit to a real name and location: map, source, `configure.py`, `splits.txt`.

docs/plan.md, "An `auto` unit stops being scaffolding once matching work starts" (owner's rule,
2026-09-23): the `auto/*` bucket exists to *attribute* symbols, and the moment real matching work starts
on one of its units it must stop looking like scaffolding - a proper name and the corresponding
`src/<module>/` location, in one change. That change is six edits across four files (source name, file
move, `configure.py` object path and lib, `splits.txt` unit line, the symbol map) and each one can
silently break something, so it is a tool.

    python tools/units/promote.py plan  <unit> --name <stem> --module <dir> [--lib <lib>] [options]
    python tools/units/promote.py apply <unit> --name <stem> --module <dir> [options] [--dry-run]
    python tools/units/promote.py check <unit> --name <stem> --module <dir> [options]
    python tools/units/promote.py --selftest

`<unit>` is the registered path (`auto/80324F7C_fn_80324F7C`, with or without the extension, with or
without `src/`). `plan` is read-only and prints the whole change first; `apply` performs it, and
`apply --dry-run` is `plan`. `check` verifies the object's bytes across the move.

**What `apply` edits, in order** (every gate runs before the first byte is written):

1. the **symbol map**, through `tools/symbols/symedit.py` (`plan_rename` + `apply_rename`) - never by
   hand, and never opened as text here: the map is 4.5 MB;
2. the **source**, the other half of that rename: every whole-word occurrence of an old symbol name in
   the unit's own file becomes the new one (the header comment included);
3. **`git mv src/auto/<file> src/<module>/<file>`** - the compile rule's `-o` is the *source* directory,
   so the object follows the file name; the destination directory is created when it does not exist;
4. **`splits.txt`**: the unit's key line, renamed in place - same ranges, same order, comments untouched;
5. **`configure.py`**: the `Object(...)` line leaves the source lib (the `auto` lib, flag and any
   keyword arguments preserved exactly) and is appended to the target lib's object list, or a new lib
   block is created when the module has none;
6. the **brief pool**: the stale pooled brief for the old unit is removed (`brief.py --pool` would prune
   it, `queue.py list` would call it stale) - under `tools/units/briefs/`, which is gitignored.

Both shared files go through `tools/units/sharedfiles.py` (docs/plan.md 7.12): line endings preserved,
anchors asserted, one temp+rename transaction. The map rename and `git mv` are their own steps; a
failure reports what has already happened and how to undo it, and a successful `git mv` is undone by
`git mv`-ing back. Nothing is measured by `apply`; the re-split and the gate are the orchestrator's
(docs/plan.md 5.4), and a rename *and* a move each cost the split, so promotions ride one batch (5.6).

**The lib.** The target lib is the module's existing one - `Pl/pl_act.cpp` sits in lib `Pl`, so
`--module Pl` finds it. When the module has no lib yet, `--lib` (default: the module name) creates one
whose `mw_version` and `cflags` **default to the moved unit's own**, so the compile command - and with
it the object - is unchanged by the creation; the module's real flags are a later, measured edit.

**Byte-identity (`check`).** A name and a path change no instructions, so the object must survive the
move byte for byte. That holds for the *meaningful* object only, because three things are metadata:

* the ELF file symbol (`STT_FILE`, symbol 1) *is* the source basename - renaming the file renames it by
  construction, and it is the first entry of `.strtab`, so `.strtab`'s length and every later name
  offset in `.symtab` shift too;
* a symbol rename changes the name text of the symbols it names, nothing else;
* hence `check` compares section bytes (`.text`, the data sections, `extab`/`extabindex`, `.comment`,
  `.rela*`), the symbol table's *shape* (value, size, info, other, section - not the name), and the
  relocations by the identity of their target (value, size, section, binding) rather than by index or
  name. A whole-file `cmp` is therefore expected to differ; a section difference is not.

For the move to keep *the same compile command*, all four of these have to hold, and `plan` prints the
delta for each:

* the same `mw_version` (else a different compiler);
* the same `cflags` (a lib's flag group, e.g. `cflags_main` vs `cflags_pl`);
* the same **extension** - `project.py` derives `-lang=c` / `-lang=c++` from it, so `.c -> .cpp` is a
  different language, not a rename (`--allow-ext-change` overrides, and byte-identity is then off);
* the same *section layout*: a flag that implies one, e.g. `-O4,p`'s `-func_align 16`, pads functions
  and moves every later one, and the `-sdata`/`-sdata2` thresholds move data between sections.

`check` proves the first three empirically by comparing the object built before the promotion with the
one built after it (the re-split rebuilds the object at its new path). The "before" object is snapshotted
into `MAIN/.pi/promote/` by `apply` (and by `check --save`) because a clean `build/` would otherwise
lose it. It exits non-zero on any meaningful difference.

`plan` also reports two consequences that are not edits: the source leaves the `src/auto/` rule-7
exemption, so every `fn_XXXXXXXX` left in it becomes a `stylelint.py` finding at the new path (which
`land.py` refuses a batch for adding) - rename them with `--symbol old=new`; and a live claim on the
unit makes `apply` refuse, because the claim's brief and worktree name the old path.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "units"))
sys.path.insert(0, str(ROOT / "tools" / "symbols"))

import sharedfiles as sf  # noqa: E402  the one writer for shared files - docs/plan.md 7.12
import symedit  # noqa: E402  the only way this tool touches symbols.txt

SRC_EXTS = (".c", ".cpp", ".cp", ".cxx", ".cc")
CPP_EXTS = (".cpp", ".cp", ".cxx", ".cc")
NAME_SECTIONS = (".strtab", ".shstrtab", ".symtab")   # the sections that *are* symbol names
MAPFILE = "config/RMHE08/symbols.txt"
SPLITS = "config/RMHE08/splits.txt"
CONFIGURE = "configure.py"
BUILD_NINJA = "build.ninja"
OBJECT_DIR = "build/RMHE08/src"
SCRATCH = ".pi/promote"
POOL = "tools/units/briefs/pool"
AUTO_LIB = "auto"
CONFIG_LIBS_ANCHOR = "config.libs = ["
FORMAT_ONLY = ("-func_align", "-sdata", "-sdata2")

LIB_RE = re.compile(r'^\s*"lib":\s*"(?P<name>[^"]+)"\s*,\s*$')
KV_RE = re.compile(r'^\s*"(?P<key>mw_version|cflags|progress_category)":\s*(?P<value>[^,]+?)\s*,\s*$')
OBJ_RE = re.compile(r'^(?P<indent>\s*)Object\(\s*(?P<flag>\w+)\s*,\s*"(?P<path>[^"]+)"\s*'
                    r'(?P<rest>,[^)]*)?\),\s*$')
SPLIT_KEY_RE = re.compile(r"^(?P<key>\S+):\s*$")
NINJA_OUT_RE = re.compile(r"^build\s+(?P<out>\S+):\s+mwcc")


# --------------------------------------------------------------------------------------------------
# small shared helpers
# --------------------------------------------------------------------------------------------------
def read_text(path: Path) -> str:
    return sf.read_text(path)


def git(root: Path, *args: str, check: bool = True) -> str:
    out = subprocess.run(["git", *args], cwd=str(root), capture_output=True, text=True,
                         errors="replace")
    if check and out.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), root, out.stderr.strip()))
    return out.stdout


def norm_unit(unit: str) -> str:
    """`src/auto/X.c`, `auto/X`, `X` -> `auto/X` (the registered spelling, extension stripped)."""
    u = unit.strip().replace("\\", "/")
    if u.startswith("src/"):
        u = u[4:]
    for ext in SRC_EXTS:
        if u.endswith(ext):
            u = u[: -len(ext)]
            break
    return u


def source_name(root: Path, unit: str) -> str | None:
    """The registered `src/`-relative path of `unit`, by trying the extensions on disk."""
    stem = norm_unit(unit)
    for ext in SRC_EXTS:
        if (root / "src" / (stem + ext)).is_file():
            return stem + ext
    return None


def lang_flag(ext: str) -> str:
    return "-lang=c++" if ext in CPP_EXTS else "-lang=c"


def normalise_path(value) -> str:
    """Ninja writes Windows paths with backslashes; `Wii\1.3` and `Wii/1.3` are the same value."""
    return str(value).replace("\\", "/") if value else ""


def flag_units(cflags: str) -> list[str]:
    """A compile flag string as its option units: `-x` together with its value (`-Cpp_exceptions off`).

    Quoted values stay one unit (`-pragma "cats off"`), and `-lang=` is dropped: it is derived from the
    source extension by `project.py`, so it is compared separately.
    """
    toks = re.findall(r'"[^"]*"|\S+', cflags)
    units, i = [], 0
    while i < len(toks):
        t = toks[i]
        if t.startswith("-") and i + 1 < len(toks) and not toks[i + 1].startswith("-"):
            units.append(t + " " + toks[i + 1])
            i += 2
        else:
            units.append(t)
            i += 1
    return [u for u in units if not u.startswith("-lang=")]


def fmt_delta(before: list[str], after: list[str]) -> list[str]:
    """A readable `-x` / `+x` list for two flag lists, so a flag change is not just 'differs'."""
    from collections import Counter
    cb, ca = Counter(before), Counter(after)
    return sorted((cb - ca).elements()) + \
           ["+%s" % u for u in sorted((ca - cb).elements())]


# --------------------------------------------------------------------------------------------------
# configure.py - read the libs, plan the object move
# --------------------------------------------------------------------------------------------------
def configure_libs(text: str) -> dict:
    """`{lib name: {mw_version, cflags, category, key_line, close_line, objects: [...]}}`.

    A lib block is a flat dict, so the block ends at the first `},` - there is no nesting to track. The
    object rows carry their line index and exact text, because the move deletes one and inserts another.
    """
    lines = text.split(sf.line_ending(text))
    libs, cur = {}, None
    for i, line in enumerate(lines):
        m = LIB_RE.match(line)
        if m:
            cur = {"lib": m.group("name"), "mw_version": None, "cflags": None, "category": None,
                   "key_line": i, "close_line": None, "objects": [], "lines": lines}
            libs[cur["lib"]] = cur
            continue
        if cur is not None:
            if line.strip() == "},":
                cur["close_line"] = i
                cur = None
                continue
            kv = KV_RE.match(line)
            if kv:
                key = {"mw_version": "mw_version", "cflags": "cflags",
                       "progress_category": "category"}[kv.group("key")]
                cur[key] = kv.group("value").strip().strip('"')
                continue
            om = OBJ_RE.match(line)
            if om:
                cur["objects"].append({"path": om.group("path"), "flag": om.group("flag"),
                                       "rest": (om.group("rest") or "").strip(),
                                       "line": i, "text": line})
    return libs


def object_line_text(row: dict, new_path: str) -> str:
    indent = OBJ_RE.match(row["text"]).group("indent")
    return '%sObject(%s, "%s"%s),' % (indent, row["flag"], new_path, row["rest"])


def lib_of_module(libs: dict, module: str) -> str | None:
    """The lib that already owns files in `module/`, when exactly one does."""
    if module in ("", "."):
        return None
    want = module.strip("/") + "/"
    hits = {name for name, blk in libs.items()
            if any(o["path"].startswith(want) for o in blk["objects"])}
    return hits.pop() if len(hits) == 1 else None


def preserving_libs(libs: dict, mw_version: str, group: str | None) -> list[str]:
    """The existing libs whose group/mw_version reproduce the unit's command line exactly."""
    want = normalise_path(mw_version)
    return sorted(name for name, blk in libs.items()
                  if blk["cflags"] == group and normalise_path(blk["mw_version"]) == want)


def expand_group(ctx: "Ctx", libs: dict, group: str) -> dict | None:
    """The real command flags of a `cflags` group, read from a unit that uses it unoverridden.

    The group is what the inserted `Object(...)` line will use (it carries no `cflags=` override), so the
    representative must be an object with no override either - `Pl`'s first object is `pl_skill.cpp`,
    whose own override is not the lib's group. The `mw_version` has to match the group's lib as well.
    """
    for blk in libs.values():
        if blk["cflags"] != group:
            continue
        for o in blk["objects"]:
            if "cflags=" in o["rest"]:
                continue
            obj = ctx.object_dir / (o["path"][: -len(Path(o["path"]).suffix)] + ".o")
            got = ninja_unit_flags(ctx.build_ninja, ctx.rel(obj))
            if got and got.get("cflags"):
                return {"from": o["path"], "mw_version": got.get("mw_version"),
                        "cflags": got.get("cflags", "")}
    return None


def new_lib_block(lib: str, mw_version: str, cflags: str, category: str, row: str, why: str) -> str:
    return ('    {\n'
            '        # %s\n'
            '        "lib": "%s",\n'
            '        "mw_version": "%s",\n'
            '        "cflags": %s,\n'
            '        "progress_category": "%s",\n'
            '        "objects": [\n'
            '%s'
            '        ],\n'
            '    },\n' % (why, lib, mw_version, cflags, category, row))


# --------------------------------------------------------------------------------------------------
# splits.txt - read the unit's block, plan the key rewrite
# --------------------------------------------------------------------------------------------------
def splits_block(text: str, unit: str) -> dict | None:
    """The unit's block: the key line index and every range line under it, in file order."""
    lines = text.split(sf.line_ending(text))
    for i, line in enumerate(lines):
        if line[:1] in (" ", "\t"):
            continue
        m = SPLIT_KEY_RE.match(line)
        if not m or m.group("key") != unit:
            continue
        body, j = [], i + 1
        while j < len(lines) and lines[j].strip():
            body.append(j)
            j += 1
        return {"key_line": i, "body_lines": body,
                "ranges": [lines[k].strip() for k in body if "start:" in lines[k]]}
    return None


# --------------------------------------------------------------------------------------------------
# build.ninja - the *real* compile command, per unit
# --------------------------------------------------------------------------------------------------
def ninja_unit_flags(path: Path, out_obj: str) -> dict | None:
    """`{mw_version, cflags}` for the unit whose output object is `out_obj`, from build.ninja.

    This is the command line ninja would run, not a re-derivation from `configure.py` (playbook 5): the
    file is meant to be read, and reading it is all this does. Multi-line values are joined on `$` the
    way ninja joins them.
    """
    if not path.is_file():
        return None
    text = read_text(path)
    lines = text.split(sf.line_ending(text))
    want = normalise_path(out_obj)
    i = 0
    while i < len(lines):
        m = NINJA_OUT_RE.match(lines[i])
        if not m:
            i += 1
            continue
        out = normalise_path(m.group("out"))
        while i < len(lines) and lines[i].rstrip().endswith("$"):
            i += 1
        i += 1
        block: dict[str, str] = {}
        while i < len(lines) and lines[i].startswith("  "):
            line = lines[i]
            while line.rstrip().endswith("$") and i + 1 < len(lines):
                stripped = line.rstrip()
                i += 1
                line = stripped[:-1].rstrip() + " " + lines[i].strip()
            key, _, value = line.strip().partition(" = ")
            block[key] = value.strip()
            i += 1
        if out == want:
            return block
    return None


# --------------------------------------------------------------------------------------------------
# ELF - the byte-identity comparator
# --------------------------------------------------------------------------------------------------
class Elf:
    """A 32-bit big-endian ELF object as the pieces `check` compares: sections, symbols, relocations."""

    def __init__(self, path: Path):
        self.path = path
        data = open(path, "rb").read()
        if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 2:
            raise ValueError("%s is not a 32-bit big-endian ELF object" % path)
        shoff, = struct.unpack_from(">I", data, 0x20)
        shentsize, = struct.unpack_from(">H", data, 0x2E)
        shnum, = struct.unpack_from(">H", data, 0x30)
        shstrndx, = struct.unpack_from(">H", data, 0x32)
        raw = []
        for i in range(shnum):
            name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from(
                ">10I", data, shoff + i * shentsize)
            raw.append({"name_off": name, "type": typ, "flags": flags, "addr": addr,
                        "offset": offset, "size": size, "link": link, "info": info,
                        "align": align, "entsize": entsize})
        shstr = data[raw[shstrndx]["offset"]:raw[shstrndx]["offset"] + raw[shstrndx]["size"]]

        def name_at(o: int) -> str:
            end = shstr.find(b"\0", o)
            return shstr[o:end].decode("latin1")

        for s in raw:
            s["name"] = name_at(s["name_off"])
            s["data"] = data[s["offset"]:s["offset"] + s["size"]]
        self.sections = raw
        self.by_name = {s["name"]: s for s in raw}
        self.symbols = self._symbols()
        self.relocs = self._relocs()

    def _symbols(self) -> list[dict]:
        symtab, strtab = self.by_name.get(".symtab"), self.by_name.get(".strtab")
        if symtab is None or strtab is None:
            return []
        st = strtab["data"]
        out = []
        for i in range(symtab["size"] // 16):
            name, value, size, info, other, shndx = struct.unpack_from(">IIIBBH", symtab["data"],
                                                                       i * 16)
            end = st.find(b"\0", name) if name < len(st) else -1
            out.append({"name": st[name:end].decode("latin1") if end != -1 else "",
                        "value": value, "size": size, "info": info, "other": other,
                        "shndx": shndx})
        return out

    def _relocs(self) -> dict:
        out: dict[str, list] = {}
        for s in self.sections:
            if s["type"] != 4:                       # SHT_RELA
                continue
            target = self.sections[s["info"]]["name"] if s["info"] < len(self.sections) else "?"
            rows = []
            for i in range(s["size"] // 12):
                off, info, addend = struct.unpack_from(">IIi", s["data"], i * 12)
                rows.append((off, info & 0xFF, info >> 8, addend))
            out.setdefault(target, []).extend(rows)
        return out

    def identity(self, index: int) -> tuple:
        """A relocation target by what it *is*, not by its index or name."""
        if index >= len(self.symbols):
            return ("?",)
        s = self.symbols[index]
        return (s["value"], s["size"], s["info"] & 0xF, s["shndx"])

    def sem_relocs(self) -> dict:
        out = {}
        for target, rows in self.relocs.items():
            out[target] = sorted((off, typ, self.identity(idx), addend)
                                 for off, typ, idx, addend in rows)
        return out


def compare_objects(before: Path, after: Path) -> dict:
    """Section bytes, symbol shape and relocation targets of two objects - names excluded.

    Returns `{verdict, sections, diffs, names}`. `verdict` is `identical` (nothing differs),
    `names-only` (only symbol/file names differ - what a promotion must look like) or `differs`
    (anything else is a codegen or flag change, i.e. a bug in the move).
    """
    a, b = Elf(before), Elf(after)
    diff, names, sections = [], [], []
    order = list(a.by_name) + [n for n in b.by_name if n not in a.by_name]
    for name in order:
        sa, sb = a.by_name.get(name), b.by_name.get(name)
        if name in NAME_SECTIONS:
            if sa and sb and sa["data"] != sb["data"] and name != ".symtab":
                names.append("%s differs (symbol names live here)" % name)
            sections.append((name, sa["size"] if sa else None, sb["size"] if sb else None, "names"))
            continue
        if sa is None or sb is None:
            diff.append("%s: %s" % (name, "absent before" if sa is None else "absent after"))
            sections.append((name, sa["size"] if sa else None, sb["size"] if sb else None, "absent"))
            continue
        equal = sa["data"] == sb["data"]
        if not equal:
            at = next((i for i in range(min(len(sa["data"]), len(sb["data"])))
                       if sa["data"][i] != sb["data"][i]), min(len(sa["data"]), len(sb["data"])))
            diff.append("%s: %d vs %d bytes, first difference at 0x%X"
                        % (name, sa["size"], sb["size"], at))
        sections.append((name, sa["size"], sb["size"], "equal" if equal else "DIFFERS"))
    sha = [(s["value"], s["size"], s["info"], s["other"], s["shndx"]) for s in a.symbols]
    shb = [(s["value"], s["size"], s["info"], s["other"], s["shndx"]) for s in b.symbols]
    if sha != shb:
        diff.append("symbol table shape differs: %d vs %d entries%s"
                    % (len(sha), len(shb),
                       " - a symbol was added or removed" if len(sha) != len(shb)
                       else " (same count: a value, size, binding or section moved)"))
    if len(sha) == len(shb):
        for x, y in zip(a.symbols, b.symbols):
            if x["name"] != y["name"]:
                names.append("%s -> %s" % (x["name"] or "(file symbol)",
                                            y["name"] or "(file symbol)"))
    else:
        names.append("%d vs %d symbol names (the table was reordered)"
                     % (len(a.symbols), len(b.symbols)))
    ra, rb = a.sem_relocs(), b.sem_relocs()
    if sorted(ra) != sorted(rb):
        diff.append("relocation sections differ: %s vs %s" % (sorted(ra), sorted(rb)))
    for target in sorted(set(ra) & set(rb)):
        if ra[target] != rb[target]:
            diff.append("%s: %d vs %d relocations (targets resolved by value/size/section)"
                        % (target, len(ra[target]), len(rb[target])))
    verdict = "differs" if diff else ("names-only" if names else "identical")
    return {"verdict": verdict, "sections": sections, "diffs": diff, "names": names,
            "before": str(before), "after": str(after)}


# --------------------------------------------------------------------------------------------------
# context and the plan
# --------------------------------------------------------------------------------------------------
@dataclass
class Ctx:
    root: Path = ROOT
    splits: Path = field(default=None)          # type: ignore[assignment]
    configure: Path = field(default=None)       # type: ignore[assignment]
    src: Path = field(default=None)             # type: ignore[assignment]
    pool: Path = field(default=None)            # type: ignore[assignment]
    mapfile: Path = field(default=None)         # type: ignore[assignment]
    build_ninja: Path = field(default=None)     # type: ignore[assignment]
    object_dir: Path = field(default=None)      # type: ignore[assignment]
    scratch: Path = field(default=None)         # type: ignore[assignment]

    def __post_init__(self):
        r = self.root
        self.splits = self.splits or r / SPLITS
        self.configure = self.configure or r / CONFIGURE
        self.src = self.src or r / "src"
        self.pool = self.pool or r / POOL
        self.mapfile = self.mapfile or r / MAPFILE
        self.build_ninja = self.build_ninja or r / BUILD_NINJA
        self.object_dir = self.object_dir or r / OBJECT_DIR
        self.scratch = self.scratch or r / SCRATCH

    def rel(self, path) -> str:
        return os.path.relpath(str(path), str(self.root)).replace("\\", "/")


def parse_symbol_args(symbols: list[str], own: str | None) -> list[tuple[str, str]]:
    """`--symbol` rows: `old=new`, or a bare `new` for the unit's own `fn_XXXXXXXX` symbol."""
    out = []
    for row in symbols or []:
        if "=" in row:
            old, new = row.split("=", 1)
            out.append((old.strip(), new.strip()))
        else:
            if not own:
                raise SystemExit("refusing: --symbol %s has no `old=`: the unit's own symbol was not "
                                 "resolved (a bare name only works when the range names a function)"
                                 % row)
            out.append((own, row.strip()))
    return out


def symbols_in_range(ctx: Ctx, start: int, end: int) -> list[dict]:
    return sorted((e for e in symedit.entries(str(ctx.mapfile))
                   if e["section"] == ".text" and start <= e["address"] < end),
                  key=lambda e: e["address"])


def plan(ctx: Ctx, unit: str, name: str, module: str, lib: str | None,
         symbols: list[str], allow_ext_change: bool = False) -> dict:
    """Every input the change depends on, read and checked - nothing is written."""
    registered = source_name(ctx.root, unit)
    if registered is None:
        raise SystemExit("refusing: %s is not a registered source under %s" % (unit, ctx.src))
    src_file = ctx.src / registered
    old_text = read_text(src_file)
    nl_src = sf.line_ending(old_text)

    # -- the new name and path -----------------------------------------------------------------
    raw_name = Path(name).name.replace("\\", "/")
    ext = next((e for e in SRC_EXTS if raw_name.endswith(e)), "")
    old_ext = Path(registered).suffix
    if not ext:
        ext = old_ext
    stem = raw_name[: -len(ext)] if ext and raw_name.endswith(ext) else raw_name
    if ext != old_ext and not allow_ext_change:
        raise SystemExit("refusing: %s -> %s changes the language (-lang= is derived from the "
                         "extension, so the object cannot stay byte-identical); pass "
                         "--allow-ext-change to do it anyway" % (old_ext, ext))
    if not re.fullmatch(r"[A-Za-z_]\w*", stem):
        raise SystemExit("refusing: %r is not a valid source stem" % stem)
    new_name = stem + ext
    module = module.strip().strip("/").replace("\\", "/")
    new_unit = (module + "/" if module not in ("", ".") else "") + new_name
    new_file = ctx.src / new_unit

    # -- configure.py: the object, its lib, the lib's flags ------------------------------------
    conf = read_text(ctx.configure)
    nl_conf = sf.line_ending(conf)
    libs = configure_libs(conf)
    row, src_lib = None, None
    for blk in libs.values():
        for o in blk["objects"]:
            if o["path"] == registered:
                row, src_lib = o, blk["lib"]
                break
        if row:
            break
    if row is None:
        raise SystemExit("refusing: %s has no Object(...) line in %s" % (registered, ctx.configure))

    target_lib = lib or lib_of_module(libs, module) or (module if module not in ("", ".") else None)
    if not target_lib:
        raise SystemExit("refusing: the target lib cannot be derived from module %r; pass --lib"
                         % module)
    creating = target_lib not in libs
    same_lib = not creating and target_lib == src_lib
    if same_lib and src_lib == AUTO_LIB:
        raise SystemExit("refusing: the target lib is the auto bucket itself (%s) - nothing is "
                         "promoted; pass a real module or --lib" % src_lib)

    # -- splits.txt: the unit's block ----------------------------------------------------------
    splits = read_text(ctx.splits)
    block = splits_block(splits, registered)
    if block is None:
        raise SystemExit("refusing: %s has no block in %s" % (registered, ctx.splits))
    if splits_block(splits, new_unit) is not None:
        raise SystemExit("refusing: %s already has a block in %s" % (new_unit, ctx.splits))

    # -- the map rename, planned by symedit (never by hand) -------------------------------------
    text_range = None
    for rng in block["ranges"]:
        m = re.match(r"\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", rng)
        if m:
            text_range = (int(m.group(1), 16), int(m.group(2), 16))
    own = None
    if any("=" not in s for s in symbols or []) and text_range:
        syms = symbols_in_range(ctx, *text_range)
        own = syms[0]["name"] if syms else None
    pairs = parse_symbol_args(symbols, own)
    if pairs:
        map_text, map_nl, map_lines, map_changed, map_applied = symedit.plan_rename(
            str(ctx.mapfile), pairs)
    else:
        map_text, map_nl, map_lines, map_changed, map_applied = None, nl_conf, None, [], []

    # -- the source: the other half of the rename ----------------------------------------------
    source_edits, new_text = [], old_text
    for old, new in pairs:
        rx = re.compile(r"\b%s\b" % re.escape(old))
        for i, line in enumerate(new_text.split(nl_src), 1):
            if rx.search(line):
                source_edits.append((i, old, new, line.strip()[:120]))
        new_text = rx.sub(new, new_text)

    # -- the compile command, from build.ninja -------------------------------------------------
    old_obj = ctx.object_dir / (registered[: -len(old_ext)] + ".o")
    new_obj = ctx.object_dir / (new_unit[: -len(ext)] + ".o")
    src_ninja = ninja_unit_flags(ctx.build_ninja, ctx.rel(old_obj)) or {}
    cmd_before = {"mw_version": normalise_path(src_ninja.get("mw_version")) or "?",
                  "cflags": src_ninja.get("cflags", ""), "lang": lang_flag(old_ext),
                  "group": (libs[src_lib]["cflags"] if src_lib in libs else None),
                  "source": "auto lib", "from": registered}
    if creating:
        group = cmd_before["group"] or "cflags_main"
        expand = expand_group(ctx, libs, group)
        cmd_after = {"mw_version": normalise_path((expand or {}).get("mw_version"))
                     or cmd_before["mw_version"],
                     "cflags": (expand or {}).get("cflags") or cmd_before["cflags"],
                     "lang": lang_flag(ext), "group": group,
                     "source": "new lib %s, %s" % (target_lib, group),
                     "from": (expand or {}).get("from")}
    else:
        group = libs[target_lib]["cflags"] or "?"
        expand = expand_group(ctx, libs, group)
        cmd_after = {"mw_version": normalise_path((expand or {}).get("mw_version"))
                     or normalise_path(libs[target_lib]["mw_version"]) or "?",
                     "cflags": (expand or {}).get("cflags", ""), "lang": lang_flag(ext),
                     "group": group, "source": "%s lib, %s" % (target_lib, group),
                     "from": (expand or {}).get("from")}
    changes = []
    if cmd_before["mw_version"] != cmd_after["mw_version"]:
        changes.append("mw_version %s -> %s" % (cmd_before["mw_version"], cmd_after["mw_version"]))
    if not cmd_after["cflags"]:
        changes.append("the command line could not be read (no built object for %s); run the re-split "
                       "and then `check`" % (group or "the target lib"))
    else:
        for token in fmt_delta(flag_units(cmd_before["cflags"]), flag_units(cmd_after["cflags"])):
            changes.append("cflags %s" % token)
    if cmd_before["lang"] != cmd_after["lang"]:
        changes.append("lang %s -> %s (derived from the extension)"
                       % (cmd_before["lang"], cmd_after["lang"]))
    for flag in FORMAT_ONLY:
        got = any(t.startswith(flag) for t in flag_units(cmd_after["cflags"]))
        had = any(t.startswith(flag) for t in flag_units(cmd_before["cflags"]))
        if got and not had:
            changes.append("cflags gained %s (implies a different section layout)" % flag)
    if cmd_before["cflags"] and cmd_after["cflags"] and not changes:
        cmd_note = "identical command line"
    else:
        cmd_note = "the command line changes" if changes else "command line not comparable"

    # -- the brief pool and the claim ----------------------------------------------------------
    pooled = pool_briefs(ctx, registered)
    claim = {}
    try:
        from units import claims as claims_mod
        claim = claims_mod.registry_record(str(ctx.root), registered) or {}
    except Exception:                                 # the registry is scratch; never fail on it
        claim = {}

    return {
        "unit": registered, "src_file": src_file, "old_text": old_text, "text": new_text,
        "name": stem, "ext": ext, "module": module, "new_unit": new_unit, "new_file": new_file,
        "src_lib": src_lib, "lib": target_lib, "creating_lib": creating, "same_lib": same_lib,
        "flag": row["flag"], "object_row": row, "libs": libs, "conf_text": conf, "conf_nl": nl_conf,
        "splits_text": splits, "splits_nl": sf.line_ending(splits), "splits_block": block,
        "pairs": pairs, "map_text": map_text, "map_nl": map_nl, "map_lines": map_lines,
        "map_changed": map_changed, "map_applied": map_applied, "source_edits": source_edits,
        "flags": {"before": cmd_before, "after": cmd_after, "changes": changes, "note": cmd_note},
        "new_obj": new_obj, "old_obj": old_obj, "pool": pooled, "claim": claim,
        "preserving_libs": preserving_libs(libs, cmd_before["mw_version"], cmd_before["group"]),
        "lint": rule7_report(ctx, new_file, new_text),
        "new_lib_settings": {"mw_version": cmd_before["mw_version"],
                             "cflags": (libs[src_lib]["cflags"] if src_lib in libs else None)
                                       or "cflags_main",
                             "category": "game"},
    }


def pool_briefs(ctx: Ctx, unit: str) -> list[Path]:
    """The pooled brief(s) that name this unit - `tools/units/briefs/` is gitignored scratch.

    A pooled brief's title is the unit without its source extension (`brief.py` writes
    `# Brief: <unit>`), so both spellings are matched. A *claim* brief under `tools/units/briefs/`
    itself is the worker's record of a finished round and is left alone; only the pool is pruned.
    """
    out = []
    if not ctx.pool.is_dir():
        return out
    try:
        from units import brief as brief_mod
    except Exception:
        return out
    want = {unit, norm_unit(unit)}
    for p in sorted(ctx.pool.glob("*.md")):
        try:
            if brief_mod.brief_unit(str(p)) in want:
                out.append(p)
        except Exception:
            continue
    return out


def rule7_report(ctx: Ctx, new_file: Path, text: str) -> dict:
    """The rule-7 debt the moved file would carry into the non-`auto` regime.

    Rule 7 is the one §6.5 rule a promotion changes: it is exempt under `src/auto/` and enforced
    everywhere else, and `land.py` refuses a batch that *adds* a finding. The count is the real
    `stylelint.py` count (comments and string literals stripped), not a grep: a header comment that
    mentions a neighbour's `fn_XXXXXXXX` is not a finding. `count` is what the lint gate counts;
    `names` is the distinct identifiers behind it.
    """
    rel = ctx.rel(new_file)
    try:
        from units import stylelint
        enforced = stylelint.rule_enforced(7, rel)
        if not enforced:
            return {"count": 0, "names": [], "enforced": False, "lines": []}
        findings = [f for f in stylelint.lint_source(stylelint.Source(str(new_file), rel, text))
                    if f["rule"] == 7]
        names = sorted({m.group(0) for m in stylelint.RULE7_FN_RE.finditer(stylelint.strip(text)[0])})
        return {"count": len(findings), "names": names, "enforced": True,
                "lines": [f["line"] for f in findings]}
    except Exception:                                 # a fallback that never breaks a plan
        code = re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)
        code = re.sub(r'"(?:[^"\\]|\\.)*"', "", code)
        names = sorted(set(re.findall(r"\bfn_[0-9A-Fa-f]{8}\b", code)))
        return {"count": len(re.findall(r"\bfn_[0-9A-Fa-f]{8}\b", code)), "names": names,
                "enforced": True, "lines": []}


# --------------------------------------------------------------------------------------------------
# printing the plan
# --------------------------------------------------------------------------------------------------
def human_plan(p: dict, ctx: Ctx) -> None:
    def r(path):
        return ctx.rel(Path(path)) if path else "?"

    print("promote %s -> %s" % (p["unit"], p["new_unit"]))
    text_ranges = [r for r in p["splits_block"]["ranges"] if r.startswith(".text")]
    span = ""
    if text_ranges:
        m = re.match(r"\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", text_ranges[0])
        if m:
            span = " (%d B)" % (int(m.group(2), 16) - int(m.group(1), 16))
    print("  source     %s  (%s lib, %s, %s%s)"
          % (r(p["src_file"]), p["src_lib"], p["flag"],
             text_ranges[0] if text_ranges else "no .text range", span))
    print("             -> %s   (git mv, bytes kept)" % r(p["new_file"]))
    if p["pairs"]:
        for old, new in p["pairs"]:
            applied = any(a == (old, new) for a in p["map_applied"])
            n = len([e for e in p["source_edits"] if e[1] == old])
            print("  symbol     %-24s -> %-24s %s map rename, %d source line(s)"
                  % (old, new, "already-applied" if applied else "would do", n))
        for i, old, new, line in p["source_edits"][:6]:
            print("               %s:%d  %s" % (r(p["src_file"]), i, line))
        if len(p["source_edits"]) > 6:
            print("               ... %d more" % (len(p["source_edits"]) - 6))
    else:
        print("  symbol     unchanged - no --symbol given (the naming rule forbids inventing a name)")
    blk = p["splits_block"]
    print("  splits     %s line %d: %s:  ->  %s:"
          % (r(ctx.splits), blk["key_line"] + 1, p["unit"], p["new_unit"]))
    for rng in blk["ranges"]:
        print("               %s" % rng)
    print("  configure  %s lib line %d: remove  %s"
          % (p["src_lib"], p["object_row"]["line"] + 1, p["object_row"]["text"].strip()))
    new_row_text = object_line_text(p["object_row"], p["new_unit"]).strip()
    if p["creating_lib"]:
        s = p["new_lib_settings"]
        print("             new lib %r (%s, %s, category %s) - mw_version and cflags default to the "
              "moved unit's own" % (p["lib"], s["mw_version"], s["cflags"], s["category"]))
    elif p["same_lib"]:
        print("             %s lib line %d: rewrite in place  %s"
              % (p["lib"], p["object_row"]["line"] + 1, new_row_text))
    else:
        print("             %s lib: append  %s"
              % (p["lib"], object_line_text(p["object_row"], p["new_unit"]).strip()))
    b, a = p["flags"]["before"], p["flags"]["after"]
    print("  compile    before  %s  [%s, %s]" % (b["mw_version"], b["group"], b["lang"]))
    print("             after   %s  [%s, %s]%s"
          % (a["mw_version"], a["group"], a["lang"],
             "  - read from %s" % a["from"] if a.get("from") else ""))
    for c in p["flags"]["changes"]:
        print("             CHANGE  %s" % c)
    if p["flags"]["note"] == "identical command line":
        print("             => same mw_version, same cflags group, same -lang: byte-identity holds "
              "unless the section layout changes")
    else:
        print("             => %s: run `check` after the re-split%s"
              % (p["flags"]["note"],
                 "; this unit is LINKED, so main.dol is at risk" if p["flag"] == "Matching" else ""))
        keep = p["preserving_libs"]
        if keep:
            print("             to keep the bytes: the same command line is the one %s uses - "
                  "--lib %s (or a new lib, which defaults to it)"
                  % (", ".join(keep), keep[0]))
        else:
            print("             to keep the bytes: no existing lib has that command line - a new "
                  "lib (--module <dir> --lib <name>) defaults to the moved unit's own flags")
    if p["lint"]["enforced"] is False:
        print("  lint       rule 7 not enforced here (the src/auto/ exemption does not move)")
    elif p["lint"]["count"]:
        print("  lint       rule 7: %d finding(s) at %s (%s) - the src/auto/ exemption no longer "
              "applies and land.py refuses a batch that adds one; rename them with --symbol"
              % (p["lint"]["count"], p["new_unit"], ", ".join(p["lint"]["names"][:6])))
    else:
        print("  lint       rule 7: clean (no fn_XXXXXXXX left in code at %s)" % p["new_unit"])
    if p["claim"]:
        print("  claim      live (%s) - apply refuses unless --allow-claimed"
              % ", ".join(sorted(k for k in p["claim"] if k != "progress")))
    else:
        print("  claim      none")
    for path in p["pool"]:
        print("  pool       %s (remove)" % r(path))
    print("  object     before %s%s" % (r(p["old_obj"]), "" if p["old_obj"].is_file() else " (absent)"))
    print("             after  %s (not built yet - the re-split builds it)" % r(p["new_obj"]))
    print("  check      python tools/units/promote.py check %s --name %s --module %s"
          % (p["unit"], p["name"], p["module"] or "."))
    print("next: python configure.py && ninja   (one re-split for the whole batch of promotions)")
    print("      ninja build/RMHE08/ok        (the gate)")
    print("      the `check` above            (byte-identity of the object across the move)")


# --------------------------------------------------------------------------------------------------
# apply
# --------------------------------------------------------------------------------------------------
def snapshot(ctx: Ctx, p: dict) -> Path | None:
    """Keep the pre-promotion object, because a clean `build/` would otherwise lose the baseline."""
    src = p["old_obj"]
    if not src.is_file():
        return None
    ctx.scratch.mkdir(parents=True, exist_ok=True)
    dst = ctx.scratch / (p["unit"].replace("/", "_") + ".before.o")
    shutil.copyfile(src, dst)
    return dst


def configure_move(p: dict) -> str:
    """`configure.py` with the object line moved out of the source lib and into the target lib.

    The object line is deleted first and the libs re-parsed from the *new* text, so the insertion index
    cannot be off by one when the target lib sits below the source lib in the file.
    """
    lines = p["conf_text"].split(p["conf_nl"])
    new_row = object_line_text(p["object_row"], p["new_unit"])
    if p["same_lib"]:
        # the unit stays in its own lib: the line is rewritten where it is, so the object order in
        # the lib (and anything a reader assumes about it) does not move
        lines[p["object_row"]["line"]] = new_row
        return p["conf_nl"].join(lines)
    del lines[p["object_row"]["line"]]
    joined = p["conf_nl"].join(lines)
    if p["creating_lib"]:
        s = p["new_lib_settings"]
        block = new_lib_block(p["lib"], s["mw_version"], s["cflags"], s["category"],
                              "            " + new_row + "\n",
                              "Promoted from %s (docs/plan.md: an auto unit stops being scaffolding)."
                              % p["unit"])
        out, inserted = sf.insert_after_anchor(joined, CONFIG_LIBS_ANCHOR, block)
        if not inserted:
            raise sf.AnchorError("refusing: %r not found in configure.py" % CONFIG_LIBS_ANCHOR)
        return out
    libs = configure_libs(joined)
    target = libs[p["lib"]]
    insert_at = target["close_line"]
    for i in range(target["key_line"] + 1, target["close_line"] + 1):
        if lines[i].strip() == '"objects": [':
            j = i + 1
            while j <= target["close_line"] and lines[j].strip() != "],":
                j += 1
            insert_at = j
            break
    else:
        raise sf.AnchorError("refusing: the %s lib has no multi-line objects list" % p["lib"])
    lines.insert(insert_at, new_row)
    out = p["conf_nl"].join(lines)
    if out.count('"%s"' % p["new_unit"]) != 1:
        raise sf.AnchorError("refusing: the object line did not insert exactly once")
    if p["object_row"]["text"] in out:
        raise sf.AnchorError("refusing: the old object line is still in configure.py")
    return out


def apply_plan(ctx: Ctx, p: dict, dry_run: bool = False, allow_claimed: bool = False,
               rename=None) -> int:
    """Perform the plan: map, source, `git mv`, splits, configure, pool. Nothing before validation."""
    if p["claim"] and not allow_claimed:
        print("refusing: %s has a live claim (%s) - its brief and worktree name the old path; release "
              "it or pass --allow-claimed" % (p["unit"], ", ".join(sorted(p["claim"]))))
        return 1
    if p["new_file"].exists():
        print("refusing: %s already exists" % ctx.rel(p["new_file"]))
        return 1
    dirty = git(ctx.root, "status", "--porcelain", "--", ctx.rel(p["src_file"]), ctx.rel(ctx.splits),
                ctx.rel(ctx.configure), check=False).strip()
    if dirty:
        print("note: already dirty (the move keeps the current content):\n  " +
              "\n  ".join(dirty.splitlines()))
    if dry_run:
        print("--- dry run of apply: nothing written")
        steps = ["snapshot the pre-move object -> %s" % ctx.rel(ctx.scratch),
                 "map rename (%d pair(s))" % len(p["map_changed"]),
                 "source edit (%d line(s))" % len(p["source_edits"]),
                 "git mv %s -> %s" % (ctx.rel(p["src_file"]), ctx.rel(p["new_file"])),
                 "splits.txt key line %d" % (p["splits_block"]["key_line"] + 1),
                 "configure.py: remove line %d, %s" % (p["object_row"]["line"] + 1,
                                                       "create lib %s" % p["lib"]
                                                       if p["creating_lib"] else "append to lib %s"
                                                       % p["lib"]),
                 "pool brief removal (%d)" % len(p["pool"])]
        for s in steps:
            print("    would do: %s" % s)
        return 0
    undo: list[tuple[str, object]] = []
    snap = snapshot(ctx, p)

    def write_text(path: Path, text: str) -> None:
        tx = sf.Transaction(rename=rename)
        try:
            tx.write(path, text)
        except BaseException:
            tx.rollback()
            raise
        finally:
            tx.cleanup()

    try:
        if p["map_changed"]:
            symedit.apply_rename(str(ctx.mapfile), p["map_nl"], p["map_lines"], p["map_changed"])
            undo.append(("the map renames (%s)" % ", ".join("%s -> %s" % (o, n) for o, n, _i, _l
                                                              in p["map_changed"]),
                         lambda: write_text(ctx.mapfile, p["map_text"])))
        if p["text"] != p["old_text"]:
            write_text(p["src_file"], p["text"])
            undo.append(("the symbol rename in %s" % ctx.rel(p["src_file"]),
                         lambda: write_text(p["src_file"], p["old_text"])))
        p["new_file"].parent.mkdir(parents=True, exist_ok=True)
        git(ctx.root, "mv", ctx.rel(p["src_file"]), ctx.rel(p["new_file"]))
        undo.append(("the move of %s" % ctx.rel(p["new_file"]),
                     lambda: git(ctx.root, "mv", ctx.rel(p["new_file"]),
                                 ctx.rel(p["src_file"]))))
        lines = p["splits_text"].split(p["splits_nl"])
        lines[p["splits_block"]["key_line"]] = "%s:" % p["new_unit"]
        tx = sf.Transaction(rename=rename)
        try:
            tx.write(ctx.splits, p["splits_nl"].join(lines))
            tx.write(ctx.configure, configure_move(p))
        except BaseException:
            tx.rollback()
            raise
        finally:
            tx.cleanup()
        undo.append(("splits.txt and configure.py",
                     lambda: (write_text(ctx.splits, p["splits_text"]),
                              write_text(ctx.configure, p["conf_text"]))))
        for path in p["pool"]:
            try:
                path.unlink()
            except OSError:
                pass
    except BaseException as exc:                      # noqa: BLE001 - undo, then say what is left
        print("FAILED after %d step(s): %s" % (len(undo), exc))
        for label, action in reversed(undo):
            try:
                action()
                print("  undone: %s" % label)
            except BaseException as undo_exc:          # noqa: BLE001 - keep going, report at the end
                print("  COULD NOT UNDO: %s (%s)" % (label, undo_exc))
        print("  check the tree: git status --short")
        return 1
    print("promoted %s -> %s (lib %s)" % (p["unit"], p["new_unit"], p["lib"]))
    if snap:
        print("object before: %s  (the `check` baseline)" % ctx.rel(snap))
    print("next: python configure.py && ninja   (one re-split)   then   ninja build/RMHE08/ok")
    print("      python tools/units/promote.py check %s --name %s --module %s"
          % (p["unit"], p["name"], p["module"] or "."))
    return 0


# --------------------------------------------------------------------------------------------------
# check
# --------------------------------------------------------------------------------------------------
def check(ctx: Ctx, p: dict, before: str | None, after: str | None, save: bool,
          json_out: bool) -> int:
    """Compare the object before and after the promotion - the byte-identity gate."""
    snap = ctx.scratch / (p["unit"].replace("/", "_") + ".before.o")
    if save:
        if snapshot(ctx, p):
            print("snapshot: %s" % ctx.rel(snap))
    b = Path(before) if before else (snap if snap.is_file() else p["old_obj"])
    a = Path(after) if after else p["new_obj"]
    if not b.is_file():
        print("no 'before' object: %s and %s are both absent - snapshot it with `check --save` "
              "before the re-split, or pass --before" % (ctx.rel(b), ctx.rel(p["old_obj"])))
        return 2
    if not a.is_file():
        print("no 'after' object: %s is not built yet - the re-split builds it (`python configure.py "
              "&& ninja`), then re-run this check" % ctx.rel(a))
        return 2
    res = compare_objects(b, a)
    if json_out:
        print(json.dumps(res, indent=2))
    else:
        print("check %s -> %s" % (ctx.rel(b), ctx.rel(a)))
        for name, sa, sb, verdict in res["sections"]:
            print("  %-14s %-8s %-8s %s" % (name, "-" if sa is None else "%d" % sa,
                                            "-" if sb is None else "%d" % sb, verdict))
        for line in res["names"][:12]:
            print("  name  %s" % line)
        if len(res["names"]) > 12:
            print("  name  ... %d more" % (len(res["names"]) - 12))
        for line in res["diffs"]:
            print("  DIFF  %s" % line)
        print("verdict: %s" % res["verdict"])
    return 0 if res["verdict"] in ("identical", "names-only") else 1


# --------------------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------------------
def add_common(a):
    a.add_argument("unit", help="the registered unit path, e.g. auto/802B2978_fn_802B2978")
    a.add_argument("--name", required=True, help="the new file stem, e.g. colour_blend")
    a.add_argument("--module", required=True, help="the target source directory, e.g. Pl")
    a.add_argument("--lib", default=None, help="the target lib (default: the module's existing one)")
    a.add_argument("--symbol", action="append", default=[],
                   help="old=new (repeatable); a bare name renames the unit's own fn_XXXXXXXX")
    a.add_argument("--allow-ext-change", action="store_true",
                   help="allow .c -> .cpp (changes -lang, so byte-identity is off)")


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    p = sub.add_parser("plan", help="print the whole change, read-only")
    add_common(p)
    p.add_argument("--json", action="store_true")
    p = sub.add_parser("apply", help="perform it")
    add_common(p)
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--allow-claimed", action="store_true")
    p = sub.add_parser("check", help="compare the object before and after the move")
    add_common(p)
    p.add_argument("--before", default=None)
    p.add_argument("--after", default=None)
    p.add_argument("--save", action="store_true", help="snapshot the pre-move object first")
    p.add_argument("--json", action="store_true")
    return ap


def plan_json(p: dict, ctx: Ctx) -> dict:
    out = {k: p[k] for k in ("unit", "name", "ext", "module", "new_unit", "src_lib", "lib",
                             "creating_lib", "flag", "pairs", "source_edits")}
    out["src_file"] = ctx.rel(p["src_file"])
    out["new_file"] = ctx.rel(p["new_file"])
    out["old_obj"] = ctx.rel(p["old_obj"])
    out["new_obj"] = ctx.rel(p["new_obj"])
    out["splits"] = {"key_line": p["splits_block"]["key_line"] + 1,
                     "ranges": p["splits_block"]["ranges"]}
    out["configure"] = {"remove": p["object_row"]["text"].strip(),
                        "append": object_line_text(p["object_row"], p["new_unit"]).strip(),
                        "creating_lib": p["creating_lib"], "same_lib": p["same_lib"]}
    out["flags"] = p["flags"]
    out["lint"] = p["lint"]
    out["claim"] = p["claim"]
    out["pool"] = [ctx.rel(x) for x in p["pool"]]
    return out


def main(argv: list[str] | None = None) -> int:
    ap = build_parser()
    args = ap.parse_args(argv)
    if args.selftest:
        import promote_selftest
        return promote_selftest.selftest()
    if not args.cmd:
        ap.print_help()
        return 2
    ctx = Ctx()
    p = plan(ctx, args.unit, args.name, args.module, args.lib, args.symbol, args.allow_ext_change)
    if args.cmd == "plan":
        if args.json:
            print(json.dumps(plan_json(p, ctx), indent=2))
        else:
            human_plan(p, ctx)
        return 0
    if args.cmd == "apply":
        return apply_plan(ctx, p, args.dry_run, args.allow_claimed)
    return check(ctx, p, args.before, args.after, args.save, args.json)


if __name__ == "__main__":
    sys.exit(main())
