#!/usr/bin/env python3
"""Move every tracked header from `include/` beside the sources under `src/`, proving no `#include` changes target.
Spec: docs/tools/spec/movehdr.md. CLI: movehdr.py [--dry-run] [--json] [--root TREE] [--exception OLD=NEW ...] [--selftest]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import json
import os
import posixpath
import re
import subprocess
import tempfile
from dataclasses import dataclass, field

from tools.lib import cli
from tools.lib import repo as _repo

TOOL = cli.Tool("movehdr", "docs/tools/spec/movehdr.md", tests="tools/tests/units/test_movehdr.py",
                description=(__doc__ or "").splitlines()[0], common=("json", "root", "dry_run"))

#: The default rule: `include/P` -> `src/P`.
OLD_ROOT = "include"
NEW_ROOT = "src"
#: The exceptions to the default rule (`lib.repo.HEADER_MOVE_EXCEPTIONS`, the one table): a header whose mirrored path
#: would put an owner-looking stem in the wrong directory.
EXCEPTIONS = _repo.HEADER_MOVE_EXCEPTIONS
#: The second include root every compile carries (`configure.py`: `-i build/<ver>/include`); read from disk.
BUILD_INCLUDE = "build/RMHE08/include"
#: Files whose `#include "..."` lines are simulated.
SOURCE_SUFFIXES = (".c", ".cp", ".cpp", ".h", ".hpp", ".inc", ".s")
#: The configure.py flag lines the move switches (compiler `-i`, assembler `-I`).
CONFIG_SWITCH = (('"-i include",', '"-i src",'), ('"-I include",', '"-I src",'))
INCLUDE_RE = re.compile(rb'^([ \t]*#[ \t]*include[ \t]*")([^"\r\n]+)(")', re.M)
ANGLE_RE = re.compile(rb'^[ \t]*#[ \t]*include[ \t]*<', re.M)
#: The two search orders the simulation proves both of: the including file's directory first, or last.
ORDERS = ("local-first", "roots-first")


@dataclass
class Plan:
    moves: list[tuple[str, str]] = field(default_factory=list)       # (old, new) still to do
    done: list[tuple[str, str]] = field(default_factory=list)        # (old, new) already moved
    rewrites: list[tuple[str, int, str, str]] = field(default_factory=list)  # (file as after the move, line, old spelling, new)
    collisions: list[str] = field(default_factory=list)
    changed: list[str] = field(default_factory=list)                 # an include whose target would change
    stem_clashes: list[str] = field(default_factory=list)            # a header stem = a unit in another directory
    config: list[tuple[str, str]] = field(default_factory=list)      # (old flag, new flag) still to switch
    config_problems: list[str] = field(default_factory=list)
    includes: int = 0
    unresolved: int = 0
    angle: int = 0

    @property
    def refused(self) -> bool:
        return bool(self.collisions or self.changed or self.config_problems)

    def as_dict(self) -> dict:
        return {"moves": len(self.moves), "done": len(self.done), "rewrites": [list(r) for r in self.rewrites],
                "collisions": self.collisions, "changed": self.changed, "stem_clashes": self.stem_clashes,
                "config": [list(c) for c in self.config], "config_problems": self.config_problems,
                "includes": self.includes, "unresolved": self.unresolved, "angle": self.angle,
                "refused": self.refused}


def tracked(root: str) -> list[str]:
    out = subprocess.run(["git", "ls-files", "-z"], cwd=root, capture_output=True, check=True).stdout
    return sorted(p for p in out.decode("utf-8").split("\0") if p)


def build_files(root: str) -> list[str]:
    """The files under the build include root (not tracked: dtk writes `macros.inc` there)."""
    base = os.path.join(root, *BUILD_INCLUDE.split("/"))
    out = []
    for dirpath, _dirs, names in os.walk(base):
        for n in names:
            out.append(posixpath.join(BUILD_INCLUDE, os.path.relpath(os.path.join(dirpath, n), base).replace("\\", "/")))
    return sorted(out)


def mapped(path: str, exceptions: dict[str, str]) -> str:
    """Where `path` lives after the move (unchanged when it is not under `include/`)."""
    if path in exceptions:
        return exceptions[path]
    if path.startswith(OLD_ROOT + "/"):
        return NEW_ROOT + path[len(OLD_ROOT):]
    return path


def _resolver(files: list[str], roots: tuple[str, ...]):
    index = {f.lower(): f for f in files}

    def resolve(includer: str, spelling: str, order: str) -> str | None:
        local = posixpath.dirname(includer)
        dirs = [local, *roots] if order == "local-first" else [*roots, local]
        for d in dirs:
            cand = posixpath.normpath(posixpath.join(d, spelling)) if d else posixpath.normpath(spelling)
            hit = index.get(cand.lower())
            if hit is not None:
                return hit
        return None
    return resolve


def read(root: str, rel: str) -> bytes:
    with open(os.path.join(root, *rel.split("/")), "rb") as fh:
        return fh.read()


def plan(root: str, exceptions: dict[str, str] | None = None) -> Plan:
    """The whole move as a plan. A tree with nothing tracked under `include/` is the finished state: the plan is
    empty and the simulation resolves the tree against itself, so a second run reports zero work."""
    exceptions = dict(EXCEPTIONS if exceptions is None else exceptions)
    p = Plan()
    files = tracked(root)
    present = {f.lower() for f in files}
    old_headers = [f for f in files if f.startswith(OLD_ROOT + "/")]
    moved = not old_headers                 # the finished state: nothing tracked under include/ any more
    seen: dict[str, str] = {}
    for old in old_headers:
        new = mapped(old, exceptions)
        key = new.lower()
        if key in present:
            p.collisions.append("%s -> %s: the destination exists" % (old, new))
        if key in seen:
            p.collisions.append("%s and %s both map to %s" % (seen[key], old, new))
        seen[key] = old
        p.moves.append((old, new))
    if moved:
        p.done = [(o, n) for o, n in sorted(exceptions.items()) if n.lower() in present]
    # unit stems: a header landing as src/D/stem.h when a unit src/D2/stem.<c|cp|cpp> exists with D2 != D
    unit_stems: dict[str, list[str]] = {}
    for f in files:
        if f.startswith(NEW_ROOT + "/") and f.endswith((".c", ".cp", ".cpp")):
            unit_stems.setdefault(posixpath.splitext(posixpath.basename(f))[0].lower(), []).append(f)
    for old, new in p.moves + p.done:
        stem = posixpath.splitext(posixpath.basename(new))[0].lower()
        for unit in unit_stems.get(stem, ()):
            if posixpath.dirname(unit).lower() != posixpath.dirname(new).lower():
                p.stem_clashes.append("%s -> %s: stem of unit %s" % (old, new, unit))
    # the include simulation: every quoted include of src/ and include/, resolved in the old and the new layout
    # under both search orders; in the finished state both layouts are the tree as it is (a consistency check)
    extra = build_files(root)
    new_files = [mapped(f, exceptions) for f in files]
    old_res = _resolver(files + extra, (NEW_ROOT if moved else OLD_ROOT, BUILD_INCLUDE))
    new_res = _resolver(new_files + extra, (NEW_ROOT, BUILD_INCLUDE))
    for f in files:
        if not f.endswith(SOURCE_SUFFIXES) or not f.startswith((OLD_ROOT + "/", NEW_ROOT + "/")):
            continue
        data = read(root, f)
        p.angle += len(ANGLE_RE.findall(data))
        after = mapped(f, exceptions)
        for m in INCLUDE_RE.finditer(data):
            spelling = m.group(2).decode("utf-8", "replace")
            line = data.count(b"\n", 0, m.start()) + 1
            p.includes += 1
            for order in ORDERS:
                was = old_res(f, spelling, order)
                now = new_res(after, spelling, order)
                want = mapped(was, exceptions) if was else None
                if was is None and now is None:
                    if order == ORDERS[0]:
                        p.unresolved += 1
                    continue
                if now == want:
                    continue
                if was in exceptions:
                    new_spelling = exceptions[was][len(NEW_ROOT) + 1:]
                    if new_res(after, new_spelling, order) == want:
                        if (after, line, spelling, new_spelling) not in p.rewrites:
                            p.rewrites.append((after, line, spelling, new_spelling))
                        continue
                p.changed.append("%s:%d \"%s\" (%s): %s -> %s" % (f, line, spelling, order, want, now))
    # configure.py
    cfg = os.path.join(root, "configure.py")
    if os.path.isfile(cfg):
        text = open(cfg, encoding="utf-8", newline="").read()
        for old, new in CONFIG_SWITCH:
            n_old, n_new = text.count(old), text.count(new)
            if n_old == 1 and n_new == 0:
                p.config.append((old, new))
            elif not (n_old == 0 and n_new == 1):
                p.config_problems.append("configure.py: %r x%d, %r x%d (want exactly one of them once)"
                                         % (old, n_old, new, n_new))
    return p


def apply(root: str, p: Plan) -> None:
    """Carry out `p`: move the files (filesystem rename, then one `git add -A` over both paths), rewrite the
    include lines, switch configure.py, and remove the emptied directories."""
    if p.refused:
        raise RuntimeError("the plan is refused; nothing written")
    paths = []
    for old, new in p.moves:
        src = os.path.join(root, *old.split("/"))
        dst = os.path.join(root, *new.split("/"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        os.replace(src, dst)
        paths += [old, new]
    by_file: dict[str, list[tuple[int, str, str]]] = {}
    for f, line, old, new in p.rewrites:
        by_file.setdefault(f, []).append((line, old, new))
    for f, edits in by_file.items():
        path = os.path.join(root, *f.split("/"))
        data = read(root, f)
        lines = data.split(b"\n")
        for line, old, new in edits:
            before = lines[line - 1]
            after = before.replace(b'"' + old.encode() + b'"', b'"' + new.encode() + b'"', 1)
            if after == before:
                raise RuntimeError("%s:%d: %r not found" % (f, line, old))
            lines[line - 1] = after
        with open(path, "wb") as fh:
            fh.write(b"\n".join(lines))
        paths.append(f)
    if p.config:
        cfg = os.path.join(root, "configure.py")
        text = open(cfg, encoding="utf-8", newline="").read()
        for old, new in p.config:
            text = text.replace(old, new, 1)
        with open(cfg, "w", encoding="utf-8", newline="") as fh:
            fh.write(text)
        paths.append("configure.py")
    if paths:
        with tempfile.NamedTemporaryFile("w", delete=False, suffix=".txt", encoding="utf-8") as fh:
            fh.write("\n".join(paths) + "\n")
            spec = fh.name
        try:
            subprocess.run(["git", "add", "-A", "--pathspec-from-file=" + spec], cwd=root, check=True,
                           capture_output=True)
        finally:
            os.unlink(spec)
    base = os.path.join(root, OLD_ROOT)
    for dirpath, _dirs, _names in sorted(os.walk(base, topdown=False), key=lambda t: -len(t[0])):
        try:
            os.rmdir(dirpath)
        except OSError:
            pass


def render(p: Plan, dry: bool) -> None:
    verb = "would" if dry else "did"
    print("movehdr: %d header(s) to move, %d already moved; %d include line(s) simulated (%d unresolved in both "
          "layouts, %d angle-bracket)" % (len(p.moves), len(p.done), p.includes, p.unresolved, p.angle))
    for f, line, old, new in p.rewrites:
        print("  rewrite %s:%d \"%s\" -> \"%s\"" % (f, line, old, new))
    for old, new in p.config:
        print("  configure.py %s -> %s" % (old, new))
    for s in p.stem_clashes:
        print("  stem clash (Ownership._owns matches the module-qualified stem, so ownership is unchanged - confirm): %s" % s)
    for s in p.collisions:
        print("  REFUSED collision: %s" % s)
    for s in p.changed:
        print("  REFUSED include target change: %s" % s)
    for s in p.config_problems:
        print("  REFUSED %s" % s)
    if p.refused:
        print("movehdr: REFUSED - nothing written")
    elif not (p.moves or p.rewrites or p.config):
        print("movehdr: nothing to do (already moved)")
    else:
        print("movehdr: %s move %d, rewrite %d include line(s), switch %d configure.py flag(s)"
              % (verb, len(p.moves), len(p.rewrites), len(p.config)))


def parse_exceptions(items: list[str]) -> dict[str, str]:
    out = dict(EXCEPTIONS)
    for item in items or ():
        old, sep, new = item.partition("=")
        if not sep or not old.startswith(OLD_ROOT + "/") or not new.startswith(NEW_ROOT + "/"):
            raise SystemExit("--exception wants include/OLD=src/NEW, not %r" % item)
        out[old] = new
    return out


def main(args) -> int:
    root = os.path.abspath(args.root)
    p = plan(root, parse_exceptions(args.exception))
    if args.json:
        print(json.dumps(p.as_dict(), indent=1))
    else:
        render(p, args.dry_run)
    if p.refused:
        return 1
    if not args.dry_run:
        apply(root, p)
    return 0


if __name__ == "__main__":
    ap = TOOL.parser()
    ap.add_argument("--exception", action="append", default=[], metavar="OLD=NEW",
                    help="one more exception to the include/P -> src/P rule (repeatable)")
    sys.exit(TOOL.run(main, parser=ap))
