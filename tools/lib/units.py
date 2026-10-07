"""The unit value type (every spelling), its paths, its real compile command, and a fresh compile.
Spec: docs/tools/spec/lib-units.md. CLI: none (library)."""
from __future__ import annotations

import json
import os
import re
import subprocess
import time
from collections.abc import Mapping
from dataclasses import dataclass
from functools import cached_property
from typing import Callable

from tools.lib.repo import VERSION, include_roots

#: Every source extension a unit can carry.
SOURCE_EXT = (".c", ".cc", ".cp", ".cpp", ".cxx", ".c++")
#: The order a missing extension is inferred in: `.cpp` first, so the old default is unchanged.
PROBE_EXT = (".cpp", ".c", ".cp", ".cxx", ".cc")
#: The objdiff unit-name prefix.
REPORT_PREFIX = "main/"
SYMBOLS_REL = os.path.join("config", VERSION, "symbols.txt")
SPLITS_REL = os.path.join("config", VERSION, "splits.txt")
#: dtk's retired run objects: `auto_<nn>_<start>_text`.
AUTO_RUN_RE = re.compile(r"^auto_\d+_([0-9A-Fa-f]{8})_text$")
#: The flag configure.py's cflags use for a search directory.
INCLUDE_FLAG = "-i"
#: The post-compile helpers project.py chains after MWCC; each takes the object as its next argument.
OBJECT_HELPERS = ("objalign.py", "objextab.py")
#: Tokens with these prefixes are switches, never MAIN-relative paths (`cmd /c` included).
SWITCH_PREFIXES = ("-", "/")


# --------------------------------------------------------------------------------------------------
# spellings: one unit, every way a lane or a file names it
# --------------------------------------------------------------------------------------------------

def _clean(spec: str) -> str:
    u = (spec or "").replace("\\", "/").strip()
    while u.startswith("./"):
        u = u[2:]
    return u


def stem(spec: str) -> str:
    """The key every spelling shares: `src/hud/x.cpp`, `main/hud/x`, `build/RMHE08/obj/hud/x.o` -> `hud/x`."""
    u = _clean(spec).strip("/")
    for pre in ("build/%s/src/" % VERSION, "build/%s/obj/" % VERSION, "src/", "main/"):
        if u.startswith(pre):
            u = u[len(pre):]
    for ext in SOURCE_EXT + (".C", ".o"):
        if u.endswith(ext):
            return u[: -len(ext)]
    return u


def normalize(spec: str) -> str:
    """A unit argument as a path from `src/`, extension kept: the prefixes a lane pastes are stripped.

    `./`, `build/RMHE08/{src,obj}/`, `build/` and `src/` (repeatedly), then one objdiff `main/` prefix
    when a path follows it (`main.cpp` and a bare `main` are the top-level unit and stay).
    """
    u = _clean(spec)
    pres = ("build/%s/src/" % VERSION, "build/%s/obj/" % VERSION, "build/", "src/")
    changed = True
    while changed:
        changed = False
        for pre in pres:
            if u.startswith(pre):
                u = u[len(pre):]
                changed = True
                break
    if u.startswith(REPORT_PREFIX) and len(u) > len(REPORT_PREFIX):
        u = u[len(REPORT_PREFIX):]
    return u.strip("/")


def report_name(spec: str) -> str:
    """The objdiff report's name for a unit: `main/<stem>`."""
    return REPORT_PREFIX + stem(spec)


def has_ext(spelling: str) -> bool:
    return spelling.endswith(SOURCE_EXT)


def with_ext(spelling: str) -> str:
    """The spelling with its extension, `.cpp` when it names none."""
    return spelling if has_ext(spelling) else spelling + ".cpp"


def obj_rel(spelling: str, side: str = "src") -> str:
    """`build/RMHE08/<side>/<stem>.o` (OS separators); `side` is `src` (ours) or `obj` (the target)."""
    return os.path.join("build", VERSION, side, *stem(with_ext(spelling)).split("/")) + ".o"


def target_rel(spelling: str) -> str:
    """The registered split object's path relative to a tree root."""
    return obj_rel(spelling, "obj")


def source_path(tree: str, spelling: str) -> str:
    """`<tree>/src/<spelling>` with the `.cpp` default."""
    return os.path.join(tree, "src", *with_ext(spelling).split("/"))


def source_spelling(spec: str, roots: list[str | None] | tuple = (), source: str | None = None) -> str:
    """The unit spelling **with its real extension**, from a spec and the trees to look in (first wins).

    In order: `source` (an explicit source path, stripped of a tree prefix); a spelling that already has an
    extension; the `configure.py` registration in each tree; the file that exists under each tree's `src/`
    (`PROBE_EXT` order); `.cpp`.
    """
    if source:
        s = source.replace("\\", "/").strip()
        for root in roots:
            if not root:
                continue
            base = os.path.join(os.path.abspath(root), "src").replace("\\", "/") + "/"
            if os.path.normcase(s).startswith(os.path.normcase(base)):
                s = s[len(base):]
                break
        for pre in ("build/%s/src/" % VERSION, "src/", "./"):
            if s.startswith(pre):
                s = s[len(pre):]
        return s.strip("/")
    unit = normalize(spec)
    if has_ext(unit):
        return unit
    want = stem(unit)
    for root in roots:
        if not root:
            continue
        try:
            _lib, names = lib_block(root, unit)
        except Exception:
            names = []
        for name in names:
            if stem(name) == want:
                return normalize(name)
    for root in roots:
        if not root:
            continue
        base = os.path.join(root, "src", *unit.split("/"))
        for ext in PROBE_EXT:
            if os.path.isfile(base + ext):
                return unit + ext
    return unit + ".cpp"


def same_tree(a: str, b: str) -> bool:
    """Whether two paths name the same tree."""
    return os.path.normcase(os.path.abspath(a)) == os.path.normcase(os.path.abspath(b))


# --------------------------------------------------------------------------------------------------
# the Unit value type
# --------------------------------------------------------------------------------------------------

def versions(root: str) -> list[str]:
    """The `build/<version>` directories that hold an `obj/` tree."""
    build = os.path.join(root, "build")
    return sorted(d for d in os.listdir(build)
                  if os.path.isdir(os.path.join(build, d, "obj"))) if os.path.isdir(build) else []


def _find_ext(root: str, key: str) -> str | None:
    for ext in SOURCE_EXT:
        if os.path.exists(os.path.join(root, "src", *key.split("/")) + ext):
            return ext
    return None


@dataclass(frozen=True)
class Unit:
    """One translation unit in one tree: `key` (`Pl/pl_act`, `main`) and the extension of its source."""
    key: str
    ext: str
    root: str
    version: str = VERSION

    @property
    def module(self) -> str:
        """The directory under `src/` ("" for a top-level unit)."""
        return self.key.rsplit("/", 1)[0] if "/" in self.key else ""

    @property
    def file(self) -> str:
        return self.key.rsplit("/", 1)[-1]

    @property
    def spelling(self) -> str:
        """`Pl/pl_act.cpp` - the `splits.txt` / `configure.py` key."""
        return self.key + self.ext

    splits_key = spelling

    @property
    def report_name(self) -> str:
        return REPORT_PREFIX + self.key

    @property
    def source(self) -> str:
        return os.path.join(self.root, "src", *self.key.split("/")) + self.ext

    @property
    def obj_dir(self) -> str:
        return os.path.join(self.root, "build", self.version, "src", *self.key.split("/")[:-1])

    @property
    def obj_ours(self) -> str:
        return os.path.join(self.root, "build", self.version, "src", *self.key.split("/")) + ".o"

    @property
    def obj_target(self) -> str:
        return os.path.join(self.root, "build", self.version, "obj", *self.key.split("/")) + ".o"

    @property
    def language(self) -> str:
        return "c" if self.ext.lower() == ".c" else "c++"

    @cached_property
    def registration(self):
        """The `configure.py` `ObjectRow` for this unit, or None (read on first use)."""
        from tools.lib.project.configure import Configure
        path = os.path.join(self.root, "configure.py")
        if not os.path.exists(path):
            return None
        try:
            objects = Configure.load(path).objects()
        except SyntaxError:
            return None
        return next((o for o in objects if stem(o.path) == self.key), None)

    @property
    def lib(self) -> str | None:
        """The `config.libs` entry that registers the unit."""
        reg = self.registration
        return reg.lib if reg else None

    @property
    def flag(self) -> str | None:
        """`Matching` / `NonMatching` / ... as registered."""
        reg = self.registration
        return reg.flag if reg else None

    @classmethod
    def make(cls, key: str, root: str, version: str = VERSION) -> "Unit":
        """The unit `key` in `root`; `SystemExit` when it has no source under `src/`."""
        ext = _find_ext(root, key)
        if ext is None:
            raise SystemExit("no source for unit %s under src/" % key)
        return cls(key, ext, root, version)

    @classmethod
    def list(cls, root: str) -> list["Unit"]:
        """Every unit with source in `root`'s `src/` (one directory deep, plus top-level), per build version."""
        out = []
        for version in versions(root):
            for entry in sorted(os.listdir(os.path.join(root, "src"))):
                d = os.path.join(root, "src", entry)
                if os.path.isdir(d):
                    for sub in sorted(os.listdir(d)):
                        if sub.endswith(SOURCE_EXT):
                            out.append(cls.make(entry + "/" + os.path.splitext(sub)[0], root, version))
                elif entry.endswith(SOURCE_EXT):
                    out.append(cls.make(os.path.splitext(entry)[0], root, version))
        return out

    @classmethod
    def resolve(cls, spec: str | None, root: str) -> "Unit":
        """Resolve any spelling in `root` (spec: docs/tools/spec/lib-units.md); `SystemExit` when it cannot.

        A bare file name names a unit by its stem and is refused (with the candidates) when two units share it;
        with no spec, the only unit there is.
        """
        units = cls.list(root)
        if spec is None:
            if len(units) == 1:
                return units[0]
            raise SystemExit("--unit is required; candidates:\n  " + "\n  ".join(u.report_name for u in units))
        s = spec.replace("\\", "/").strip()
        qualified = False
        for pre in ("build/", "src/"):
            if s.startswith(pre):
                s = s[len(pre):]
                qualified = True
        parts = [p for p in s.split("/") if p not in ("", ".")]
        if len(parts) > 1 and parts[0] == "main":
            parts = parts[1:]
            qualified = True
        vers = versions(root)
        if len(parts) >= 3 and parts[0] in vers:
            parts = parts[2:]
            qualified = True
        if not parts:
            raise SystemExit("cannot parse unit spec %r" % spec)
        file = os.path.splitext(parts[-1])[0]
        if len(parts) == 1:
            hits = [u for u in units if u.file == file and (u.module == "" or not qualified)]
            if len(hits) == 1:
                return hits[0]
            if hits:
                raise SystemExit("unit spec %r is ambiguous; name the directory:\n  %s"
                                 % (spec, "\n  ".join(u.report_name for u in hits)))
            raise SystemExit("no unit with file name %r" % file)
        key = parts[-2] + "/" + file
        for u in units:
            if u.key == key:
                return u
        if not vers:
            raise SystemExit("no build/<version>/obj tree under %s - build first" % root)
        return cls.make(key, root, units[0].version if units else vers[0])


# --------------------------------------------------------------------------------------------------
# the compile command: MAIN's ninja, the worktree's, or a same-lib sibling's
# --------------------------------------------------------------------------------------------------

def unquote(tokens: list[str]) -> list[str]:
    """Merge `"cats off"` (split in two by `.split()`) back into one token, without quotes."""
    out, i = [], 0
    while i < len(tokens):
        t = tokens[i]
        if t.startswith('"') and not t.endswith('"'):
            j = i
            while j < len(tokens) and not tokens[j].endswith('"'):
                j += 1
            out.append(" ".join(tokens[i:j + 1]).strip('"'))
            i = j + 1
        else:
            out.append(t.strip('"'))
            i += 1
    return out


def ninja_lines(tree: str, target: str, runner: Callable = subprocess.run):
    """(mwcceppc lines, completed process) of `ninja -t commands <target>` in `tree`, never raising."""
    p = runner(["ninja", "-t", "commands", target], cwd=tree, capture_output=True, text=True,
               encoding="utf-8", errors="replace")
    return [l for l in (p.stdout or "").splitlines() if "mwcceppc" in l], p


def ninja_target(spelling: str) -> str:
    """The ninja target of a unit's object: `build/RMHE08/src/<stem>.o` (forward slashes)."""
    return "build/%s/src/" % VERSION + os.path.splitext(with_ext(spelling))[0] + ".o"


def ninja_command(tree: str, spelling: str, runner: Callable = subprocess.run) -> list[str]:
    """The exact compile command `tree`'s ninja would run for the unit, as tokens."""
    target = ninja_target(spelling)
    lines, p = ninja_lines(tree, target, runner)
    if not lines:
        raise SystemExit("could not get the compile command for %s from ninja in %s:\n%s%s"
                         % (target, tree, p.stdout, p.stderr))
    return unquote(lines[-1].split())


def lib_block(tree: str, spelling: str):
    """(lib name, [object paths]) of the `config.libs` block of `tree`'s configure.py that registers the unit."""
    from tools.lib.project.configure import Configure
    path = os.path.join(tree, "configure.py")
    if not os.path.exists(path):
        return None, []
    try:
        libs = Configure.load(path).libs()
    except SyntaxError:
        return None, []
    want = stem(spelling)
    for lib in libs:
        names = [o.path for o in lib.objects]
        if any(stem(n) == want for n in names):
            return lib.name, names
    return None, []


def retarget(tokens: list[str], spelling: str) -> list[str]:
    """Point a borrowed sibling's command line at this unit's object directory and `-lang`."""
    src = with_ext(spelling)
    obj_dir = os.path.join("build", VERSION, "src", *src.split("/")[:-1])
    lang = "-lang=c" if src.lower().endswith(".c") else "-lang=c++"
    out, i = [], 0
    while i < len(tokens):
        tok = tokens[i]
        if tok == "-o" and i + 1 < len(tokens):
            out += [tok, obj_dir]
            i += 2
            continue
        out.append(lang if tok.startswith("-lang=") else tok)
        i += 1
    return out


def sibling_for(main: str, wt: str, spelling: str, runner: Callable = subprocess.run):
    """(sibling stem, tokens): a registered unit in the worktree's lib block that MAIN's ninja can build."""
    lib, names = lib_block(wt, spelling)
    want = stem(spelling)
    if not lib:
        raise SystemExit(
            "%s is not registered in %s/configure.py, and MAIN has no compile command for it - a proposal "
            "unit is measurable only once its own `Object(...)` line and `splits.txt` block are there "
            "(docs/plan.md: registration comes before the bodies)" % (spelling, wt))
    want_dir, want_ext = os.path.dirname(want), os.path.splitext(with_ext(spelling))[1].lower()

    def rank(name: str):
        s = stem(name)
        return (0 if os.path.dirname(s) == want_dir else 1,
                0 if os.path.splitext(name)[1].lower() == want_ext else 1, s)

    for name in sorted(names, key=rank):
        s = stem(name)
        if s == want:
            continue
        lines, _p = ninja_lines(main, ninja_target(s), runner)
        if lines:
            return s, unquote(lines[-1].split())
    raise SystemExit(
        "no unit in lib %r (the one %s/configure.py registers %s in) has a compile command in MAIN's "
        "ninja - a brand-new lib cannot be measured until its registration lands on MAIN"
        % (lib, wt, spelling))


def unit_tokens(main: str, wt: str, spelling: str, runner: Callable = subprocess.run):
    """(tokens, source): MAIN's ninja line, else the worktree's, else a retargeted same-lib sibling's."""
    lines, _p = ninja_lines(main, ninja_target(spelling), runner)
    if lines:
        return unquote(lines[-1].split()), "main"
    lines, _p = ninja_lines(wt, ninja_target(spelling), runner)
    if lines:
        return unquote(lines[-1].split()), "worktree"
    sibling, tokens = sibling_for(main, wt, spelling, runner)
    return retarget(tokens, spelling), "sibling %s (same lib)" % sibling


def configured_flags(tree: str, spelling: str) -> tuple[list[str], str | None] | None:
    """(flag tokens, mw_version) `tree`'s configure.py gives the unit, evaluated now (`lib.project.Configure`),
    or None when the file is missing, does not parse, or does not register the unit with resolved cflags.

    The tokens are the ones ninja's command carries: each cflags entry split the way the shell splits it, so
    `-pragma "cats off"` is `-pragma`, `cats off` (the shape `unquote` gives a ninja line)."""
    import shlex
    from tools.lib.project.configure import Configure
    path = os.path.join(tree, "configure.py")
    if not os.path.isfile(path):
        return None
    try:
        row = Configure.load(path).object(with_ext(spelling))
    except SyntaxError:
        return None
    if row is None or not row.cflags:
        return None
    out: list[str] = []
    for entry in row.cflags:
        out += shlex.split(entry)
    return out, row.mw_version


def compiler_version(head: list[str]) -> str | None:
    """`Wii/1.3` from a command head's `.../compilers/Wii/1.3/mwcceppc.exe`, None when there is no compiler."""
    try:
        parts = re.split(r"[\\/]", compiler_token(head))
    except StopIteration:
        return None
    return "/".join(parts[-3:-1]) if len(parts) >= 3 else None


def reconcile_flags(tokens: list[str], tree: str, spelling: str) -> tuple[list[str], list[str]]:
    """(tokens, notes): a ninja command line with its flags and compiler version re-read from `tree`'s
    configure.py when the build graph disagrees with it, unchanged (no notes) when they agree.

    `build.ninja` is only regenerated by `python configure.py`, and a worktree compiles with MAIN's graph, so a
    row switched from `-O4,p` to `-O3` would otherwise be measured at the old level. The `-lang=` token is
    ninja's own (project.py derives it from the extension) and is kept as it is."""
    want = configured_flags(tree, spelling)
    if want is None:
        return tokens, []
    flags_want, version_want = want
    try:
        head, flags, tail = split_flags(tokens)
    except StopIteration:
        return tokens, []
    lang = [t for t in flags if t.startswith("-lang=")]
    have = [t for t in flags if not t.startswith("-lang=")]
    notes = []
    if have != flags_want:
        gone = [t for t in have if t not in flags_want]
        new = [t for t in flags_want if t not in have]
        notes.append("flags re-read from %s (the build graph predates it): -[%s] +[%s]%s"
                     % (os.path.join(tree, "configure.py"), " ".join(gone), " ".join(new),
                        "" if gone or new else " (order)"))
        flags = flags_want + lang
    version_have = compiler_version(head)
    if version_want and version_have and version_want != version_have:
        notes.append("compiler re-read from configure.py: the build graph had %s, configure.py says %s"
                     % (version_have, version_want))
        head = with_compiler_version(head, version_want)
    return (head + flags + tail if notes else tokens), notes


def retarget_object_helpers(tokens: list[str], obj_path: str) -> list[str]:
    """Point every chained `<helper>.py <object>` argument at `obj_path` (absolute); unchanged otherwise."""
    out = list(tokens)
    changed = False
    for i, tok in enumerate(out):
        if any(tok.replace("\\", "/").endswith(h) for h in OBJECT_HELPERS) and i + 1 < len(out):
            out[i + 1] = os.path.abspath(obj_path)
            changed = True
    return out if changed else tokens


def include_pairs(tokens: list[str]) -> list[tuple[int, str]]:
    """[(index of the flag, its directory)] for every `-i <dir>`."""
    pairs: list[tuple[int, str]] = []
    i = 0
    while i < len(tokens):
        if tokens[i] == INCLUDE_FLAG and i + 1 < len(tokens):
            pairs.append((i, tokens[i + 1]))
            i += 2
        else:
            i += 1
    return pairs


def order_includes(tokens: list[str], main: str, wt: str) -> list[str]:
    """Rebuild the `-i` list so the worktree's headers win: its own directories first, then MAIN's list with
    each entry pointed at the worktree's copy when it has one (MAIN's absolute path otherwise)."""
    pairs = include_pairs(tokens)
    dirs: list[str] = []
    seen: set[str] = set()

    def add(path: str) -> None:
        key = os.path.normcase(os.path.abspath(path))
        if key not in seen:
            seen.add(key)
            dirs.append(path)

    # the worktree's own copies of the command line's relative roots (`-i src` since the 2026-10-05 header move,
    # `-i include` before it), else of the layout's roots (`lib.repo.include_roots`, read from MAIN)
    rels = [v for _idx, v in pairs if not os.path.isabs(v)] or list(include_roots(main))
    for rel in rels:
        cand = os.path.join(wt, *rel.replace("\\", "/").split("/"))
        if os.path.isdir(cand):
            add(cand)
    for _idx, value in pairs:
        rel = os.path.normpath(value)
        wt_copy = os.path.join(wt, rel)
        if os.path.isdir(wt_copy):
            add(wt_copy)
            continue
        main_copy = os.path.join(main, rel)
        add(main_copy if os.path.isdir(main_copy) else value)

    block = [t for d in dirs for t in (INCLUDE_FLAG, d)]
    if pairs:
        at = pairs[0][0]
        skip = {i for idx, _v in pairs for i in (idx, idx + 1)}
    else:
        at = next((k for k, t in enumerate(tokens) if t.startswith("-")), len(tokens))
        skip = set()
    kept = [t for k, t in enumerate(tokens) if k not in skip]
    pos = at - sum(1 for k in skip if k < at)
    return kept[:pos] + block + kept[pos:]


def rewrite(tokens: list[str], spelling: str, main: str, wt: str) -> tuple[list[str], str]:
    """Point a command at the worktree's source, `-o` directory and headers; returns (tokens, object path)."""
    wt_src = os.path.join(wt, os.path.join("src", *with_ext(spelling).split("/")))
    out: list[str] = []
    obj_dir = None
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok == "-o":
            out.append(tok)
            i += 1
            if i < len(tokens):
                obj_dir = os.path.join(wt, tokens[i].lstrip("./\\"))
                out.append(obj_dir)
        elif tok == "-c":
            out.append(tok)
            i += 1
            if i < len(tokens):
                out.append(wt_src)
        else:
            out.append(tok)
        i += 1
    if obj_dir is None:
        raise SystemExit("no -o in the command line - refusing to guess where the object goes")
    out = order_includes(out, main, wt)
    obj_path = os.path.join(obj_dir, os.path.splitext(os.path.basename(wt_src))[0] + ".o")
    return retarget_object_helpers(out, obj_path), obj_path


def is_switch(tok: str) -> bool:
    """Whether a command token is a switch (`-o`, `cmd /c`), never a MAIN-relative path."""
    return tok.startswith(SWITCH_PREFIXES)


def absolutize(tokens: list[str], main: str) -> list[str]:
    """Resolve every token that names an existing file in MAIN to an absolute path; switches untouched."""
    out = []
    for tok in tokens:
        if not is_switch(tok) and ("\\" in tok or "/" in tok):
            candidate = os.path.join(main, tok.replace("\\", os.sep))
            if os.path.exists(candidate):
                out.append(os.path.abspath(candidate))
                continue
        out.append(tok)
    return out


# --------------------------------------------------------------------------------------------------
# the compile: object deleted first, its mtime must move, it must postdate the source
# --------------------------------------------------------------------------------------------------

def _stamp(seconds: float) -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(seconds))


def object_is_fresh(object_path: str, source: str) -> tuple[bool, str]:
    """(fresh, reason): the object exists and its mtime does not predate the source it claims to be built from."""
    if not os.path.exists(object_path):
        return False, ("STALE OBJECT: no object at %s - the compile wrote nothing, so there is no score to "
                       "read" % object_path)
    if not os.path.exists(source):
        return True, ""
    obj_m, src_m = os.stat(object_path).st_mtime_ns, os.stat(source).st_mtime_ns
    if obj_m < src_m:
        return False, (
            "STALE OBJECT: %s is older than its source %s (object %s, source %s) - the compile did not "
            "rewrite it; refusing to measure, because a score read from here would be last build's number "
            "dressed as this one's"
            % (object_path, source, _stamp(obj_m / 1e9), _stamp(src_m / 1e9)))
    return True, ""


def section_sizes(obj: str) -> dict:
    """`{section: size}` of an object's non-empty named sections ({} when it is not readable ELF)."""
    from tools.lib.binary.elf import Elf
    try:
        elf = Elf.read(obj)
    except Exception:
        return {}
    return {s.name: s.size for s in elf.sections if s.name and s.size}


def frames(obj: str) -> list[tuple[str, int, int | None]]:
    """`[(name, size, frame)]` for every defined function of an object, in address order.

    `frame` is the positive `N` of a leading `stwu rS,-N(rA)` (`lib.ppc.stwu_frame`), None without one.
    """
    from tools.lib.binary.elf import Elf
    from tools.lib.ppc import stwu_frame
    elf = Elf.read(obj)
    out = []
    funcs = sorted((s for s in elf.symbols if s.name and s.shndx and s.type == 2), key=lambda s: s.value)
    for sym in funcs:
        data = elf.sections[sym.shndx].raw
        word = int.from_bytes(data[sym.value:sym.value + 4], "big") if len(data) >= sym.value + 4 else None
        out.append((sym.name, sym.size, stwu_frame(word) if word is not None else None))
    return out


def function_names(obj: str) -> list[str]:
    """The defined functions of an object, in address order."""
    return [f[0] for f in frames(obj)]


def text_size(obj: str) -> int:
    """The size of an object's `.text` section (`StopIteration` when it has none)."""
    from tools.lib.binary.elf import Elf
    return next(s.size for s in Elf.read(obj).sections if s.name == ".text")


@dataclass(frozen=True)
class CompileResult(Mapping):
    """What `compile` did: a value that also reads as the mapping of the keys it carries.

    `compiled` with `fresh`/`bytes`/`sections`/`log`; a failure with `error`; a dry run with `command`
    and `dry_run`. Unset fields are absent from the mapping, so `dict(result)` is the old return shape.
    """
    object: str
    compiled: bool | None = None
    fresh: bool | None = None
    bytes: int | None = None
    sections: dict | None = None
    log: str | None = None
    error: str | None = None
    command: list | None = None
    dry_run: bool | None = None

    def _items(self) -> dict:
        if self.dry_run:
            return {"command": self.command, "object": self.object, "dry_run": True}
        return {f: getattr(self, f) for f in self.__dataclass_fields__ if getattr(self, f) is not None}

    def __getitem__(self, key: str):
        items = self._items()
        if key not in items:
            raise KeyError(key)
        return items[key]

    def __iter__(self):
        return iter(self._items())

    def __len__(self) -> int:
        return len(self._items())


def compile(spelling: str, main: str, wt: str, dry_run: bool = False, runner: Callable = subprocess.run,
            tokens: list[str] | None = None) -> CompileResult:
    """Compile the unit in `wt` with `main`'s command line (or `tokens`) and prove the object is fresh.

    Returns a `CompileResult`: `{object, compiled, fresh, bytes, sections, log}`, `{object, compiled: False,
    error}`, or with `dry_run` `{command, object, dry_run}`.
    """
    if tokens is None:
        tokens = ninja_command(main, spelling, runner=runner)
    cmd, obj = rewrite(tokens, spelling, main, wt)
    cmd = absolutize(cmd, main)
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    existed = os.path.exists(obj)
    before = os.stat(obj).st_mtime_ns if existed else None
    if dry_run:
        return CompileResult(obj, command=cmd, dry_run=True)
    if existed:
        os.remove(obj)
    started = time.time_ns()
    p = runner(cmd, cwd=main, capture_output=True, text=True, encoding="utf-8", errors="replace")
    log = (p.stdout or "") + (p.stderr or "")
    if p.returncode != 0:
        return CompileResult(obj, compiled=False, error=log)
    if not os.path.exists(obj):
        return CompileResult(obj, compiled=False,
                             error="the compiler returned 0 but wrote no object - MWCC's -o is a DIRECTORY; "
                                   "digest:\n" + log)
    after = os.stat(obj).st_mtime_ns
    fresh = after >= started and after != before
    ok, why = object_is_fresh(obj, source_path(wt, spelling))
    if not ok:
        return CompileResult(obj, compiled=False, error=why)
    return CompileResult(obj, compiled=True, fresh=fresh, bytes=os.path.getsize(obj),
                         sections=section_sizes(obj), log=log)


def run_tokens(tokens: list[str], cwd: str, expect: str | None = None, scratch_dir: str | None = None,
               src: str | None = None, verbose: bool = False, runner: Callable = subprocess.run):
    """Run a compile command line in `cwd` -> (rc, output, object path); rc 2 when the object did not appear.

    `scratch_dir` redirects MWCC's `-o` (the real object is not clobbered); `src` replaces the `-c` source.
    The object is deleted first, and the run lands in a later whole second than the previous one.
    """
    tokens = list(tokens)
    obj = expect
    if src is not None:
        i = tokens.index("-c")
        tokens[i + 1] = src
        obj = os.path.join(scratch_dir, os.path.splitext(os.path.basename(src))[0] + ".o") \
            if scratch_dir else obj
    if scratch_dir is not None:
        i = tokens.index("-o")
        tokens[i + 1] = scratch_dir
        os.makedirs(scratch_dir, exist_ok=True)
        if src is None:
            j = tokens.index("-c")
            obj = os.path.join(scratch_dir, os.path.splitext(os.path.basename(tokens[j + 1]))[0] + ".o")
        # the chained `objalign.py`/`objextab.py` follow the object too, or they rewrite the tree's real one
        tokens = retarget_object_helpers(tokens, obj)
    if obj and os.path.exists(obj):
        os.remove(obj)
    time.sleep(1.05)
    p = runner(tokens, cwd=cwd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    out = (p.stdout or "") + (p.stderr or "")
    if verbose:
        print("$ " + " ".join(tokens))
        print(out)
    if obj and not os.path.exists(obj):
        out += "\n!! object was NOT regenerated (%s) -- check the -o argument" % obj
        return 2, out, obj
    return p.returncode, out, obj


# --------------------------------------------------------------------------------------------------
# the flag tools: one command line split, its flags overridden, its compiler swapped
# --------------------------------------------------------------------------------------------------

#: MWCC options that take a following value token (the count), as this project's configure.py spells them.
#: Used only to drop a conflicting earlier occurrence when `--flags-extra` overrides the same option.
VALUED = {
    "-proc": 1, "-align": 1, "-enum": 1, "-fp": 1, "-Cpp_exceptions": 1, "-inline": 1,
    "-pragma": 1, "-maxerrors": 1, "-RTTI": 1, "-fp_contract": 1, "-str": 1, "-i": 1, "-ir": 1,
    "-I": 1, "-use_lmw_stmw": 1, "-common": 1, "-lang": 1, "-opt": 1, "-pool": 1, "-schedule": 1,
    "-sdata": 1, "-sdata2": 1, "-model": 1, "-abi": 1, "-encoding": 1, "-D": 0, "-U": 0,
    "-func_align": 1, "-sym": 1, "-W": 1, "-gccinc": 0, "-nodefaults": 0, "-nosyspath": 0, "-multibyte": 0, "-gcc": 0, "-rostr": 0,
}


def split_flags(tokens: list[str]) -> tuple[list[str], list[str], list[str]]:
    """(head, flags, tail): head = wrapper + compiler, tail = `-MMD -c <src> -o <dir>` and what follows."""
    head_end = next(i for i, t in enumerate(tokens) if t.startswith("-") and i > 0)
    tail_start = next(i for i, t in enumerate(tokens) if t == "-MMD")
    return tokens[:head_end], tokens[head_end:tail_start], tokens[tail_start:]


def split_command(unit: Unit, runner: Callable = subprocess.run) -> tuple[list[str], list[str], list[str]]:
    """`split_flags` of the command the unit's own tree's ninja runs for it (`ninja_command`)."""
    return split_flags(ninja_command(unit.root, unit.spelling, runner=runner))


def family(tok: str) -> str | None:
    """The option family a flag token belongs to (`-O3`, `-O4,p` -> `-O`), or None."""
    if re.match(r"^-O\d", tok):
        return "-O"
    name = tok.split("=", 1)[0]
    return name if name in VALUED else None


def _value_len(tokens: list[str], i: int) -> int:
    if VALUED.get(tokens[i], 0) == 0:
        return 0
    j = i + 1
    if j < len(tokens) and tokens[j].startswith('"') and not tokens[j].endswith('"'):
        while j < len(tokens) and not tokens[j].endswith('"'):
            j += 1
        return j - i
    return VALUED[tokens[i]]


def override_flags(flags: list[str], extra: str) -> list[str]:
    """Apply `--flags-extra`: drop every earlier flag of a family `extra` names (with its value), then append."""
    extras = extra.split()
    families = {f for f in (family(t) for t in extras) if f}
    out, i = [], 0
    while i < len(flags):
        if family(flags[i]) in families:
            i += 1 + _value_len(flags, i)
            continue
        out.append(flags[i])
        i += 1
    return out + extras


def quiet(out: str) -> str:
    """MWCC's output without its informational `###` banner lines."""
    return "\n".join(l for l in out.splitlines() if not l.startswith("###"))


def compiler_token(head: list[str]) -> str:
    """The MWCC executable token of a split command line."""
    return next(t for t in head if t.endswith("mwcceppc.exe"))


def with_compiler_version(head: list[str], version: str) -> list[str]:
    """The same head with the compiler's version directory replaced: `1.3` (the unit's own family) or a
    cross-family `GC/3.0a3`."""
    fam, _, ver = version.partition("/")
    out = []
    for t in head:
        if t.endswith("mwcceppc.exe"):
            parts = re.split(r"[\\/]", t)
            parts[-2] = ver if ver else fam
            if ver:
                parts[-3] = fam
            sep = "\\" if "\\" in t else "/"
            out.append(sep.join(parts))
        else:
            out.append(t)
    return out


def drop_unknown_option(flags: list[str], log: str) -> list[str] | None:
    """`flags` without the option MWCC's `log` rejected as unknown (and its value), or None."""
    m = re.search(r"Unknown option '([^']+)'", log)
    if not m:
        return None
    bad = m.group(1)
    out = list(flags)
    for i, f in enumerate(out):
        if f == bad or (f.startswith("-") and bad in f):
            del out[i]
            if i < len(out) and not out[i].startswith("-"):
                del out[i]
            return out
    return None


def available_versions(head: list[str]) -> list[str]:
    """The compiler versions installed beside the one `head` names: the sibling directories of its version
    directory (`build/compilers/Wii/1.3/mwcceppc.exe` -> every `build/compilers/Wii/<v>/`)."""
    d = os.path.dirname(os.path.dirname(compiler_token(head)))
    return sorted(e for e in os.listdir(d) if os.path.isdir(os.path.join(d, e))) if os.path.isdir(d) else []


# --------------------------------------------------------------------------------------------------
# the target object: this tree's split, MAIN's, or the retired `auto_*_text` object for a proposal
# --------------------------------------------------------------------------------------------------

def resolve_map(wt: str, main: str, rel: str = SYMBOLS_REL):
    """(absolute path, kind) of a config file from the invocation's tree outward: `worktree-map`,
    `main-map` or `missing`."""
    p_wt, p_main = os.path.join(wt, rel), os.path.join(main, rel)
    if not same_tree(wt, main) and os.path.exists(p_wt):
        return os.path.abspath(p_wt), "worktree-map"
    if os.path.exists(p_main):
        return os.path.abspath(p_main), "main-map"
    if os.path.exists(p_wt):
        return os.path.abspath(p_wt), "worktree-map"
    return os.path.abspath(p_main), "missing"


def text_symbol_addresses(map_path: str) -> dict:
    """{name: address} for every `.text` symbol of a symbols.txt."""
    from tools.lib.project.symbols import SymbolMap
    if not os.path.exists(map_path):
        return {}
    return {e.name: e.address for e in SymbolMap(map_path).rows() if e.section == ".text"}


def auto_text_runs(tree: str) -> list:
    """[(start, size, object)] of the tree's retired `auto_<nn>_<address>_text` objects, ascending."""
    path = os.path.join(tree, "build", VERSION, "config.json")
    if not os.path.exists(path):
        return []
    try:
        with open(path, encoding="utf-8") as fh:
            units = json.load(fh).get("units") or []
    except (ValueError, OSError):
        return []
    out = []
    for u in units:
        m = AUTO_RUN_RE.match(u.get("name") or "")
        if m:
            out.append((int(m.group(1), 16), u.get("code_size") or 0, u.get("object") or ""))
    out.sort()
    return out


def symbol_addresses(wt: str, main: str):
    """({name: address}, map path, map kind) merged: MAIN's map first, the invocation's map wins."""
    path, kind = resolve_map(wt, main)
    out: dict = {}
    main_map = os.path.abspath(os.path.join(main, SYMBOLS_REL))
    if not same_tree(wt, main) and os.path.exists(main_map) \
            and os.path.normcase(main_map) != os.path.normcase(path):
        out.update(text_symbol_addresses(main_map))
    out.update(text_symbol_addresses(path))
    return out, path, kind


def retired_object_dirs(wt: str, main: str) -> list[str]:
    """The `obj/` directories a retired `auto_*_text.o` can live in: MAIN's first, then this tree's."""
    dirs = [os.path.join(main, "build", VERSION, "obj")]
    if not same_tree(wt, main):
        dirs.append(os.path.join(wt, "build", VERSION, "obj"))
    return dirs


def proposal_target(wt: str, main: str, symbol: str):
    """(target object, note) for `symbol` of a proposal unit, or (None, why not), located by address."""
    addresses, map_path, map_kind = symbol_addresses(wt, main)
    addr = addresses.get(symbol)
    if addr is None:
        return None, ("%s is not a `.text` symbol in the map this tree resolves (%s [%s]) or in MAIN's "
                      "copy, and the fallback locates the retired split object by address"
                      % (symbol, map_path, map_kind))
    names = [symbol] + sorted(n for n, a in addresses.items() if a == addr and n != symbol)
    for objdir in retired_object_dirs(wt, main):
        for name in names:
            cand = os.path.join(objdir, "auto_%s_text.o" % name[:20])
            if os.path.exists(cand):
                return cand, ("retired single-symbol split object %s (%s at 0x%X)"
                              % (os.path.basename(cand), name, addr))
    for tree in ([main] if same_tree(wt, main) else [main, wt]):
        for start, size, rel in auto_text_runs(tree):
            if start <= addr < start + size:
                path = os.path.join(tree, *rel.replace("\\", "/").split("/"))
                if os.path.exists(path):
                    return path, ("retired split object %s covers 0x%X-0x%X - the run that owns %s"
                                  % (os.path.basename(rel), start, start + size, symbol))
    return None, ("0x%X (%s) is not inside any retired `auto_*_text` object in this tree or MAIN - it "
                  "belongs to an already-registered unit or a gap, so there is no original object for it"
                  % (addr, symbol))


def resolve_target(wt: str, main: str, spelling: str, symbol: str):
    """(target object, kind, note) from the invocation's tree outward: `worktree-split`, `registered`,
    `auto-fallback` or `missing`."""
    rel = target_rel(spelling)
    same = same_tree(wt, main)
    p_wt, p_main = os.path.join(wt, rel), os.path.join(main, rel)
    if not same and os.path.exists(p_wt):
        return p_wt, "worktree-split", ""
    if os.path.exists(p_main):
        note = ""
        if not same:
            note = ("this tree has no split object for %s at %s - the score is MAIN's split object; "
                    "re-split this tree (ninja build/RMHE08/config.json, or a plain ninja) to score your own"
                    % (spelling, rel))
        return p_main, "registered", note
    found, note = proposal_target(wt, main, symbol)
    if found:
        return found, "auto-fallback", note
    map_path, map_kind = resolve_map(wt, main)
    return p_main, "missing", (
        "no original object for %s: no split object at %s in this tree or MAIN, and no retired "
        "`auto_*_text` object covers the address of %s (map: %s [%s])"
        % (spelling, rel, symbol, map_path, map_kind))
