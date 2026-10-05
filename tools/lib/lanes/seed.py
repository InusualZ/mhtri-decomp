"""Seed a worktree or slot from MAIN: the toolchain and build tree by copy, `orig/` by copy (or junction when
huge), the ninja state with `.ninja_deps` re-pointed, and the staleness guard the slot pool shares.
Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import os
import shutil
import struct
import subprocess

from tools.lib import artifacts, proc
from tools.lib.lanes import teardown

#: `build/` inputs: ninja `download_tool` outputs, so always COPIED (a junction writes through to MAIN).
SEED_COPY_DIRS = (os.path.join("build", "tools"), os.path.join("build", "compilers"), os.path.join("build", "binutils"))
ORIG_REL = os.path.join("orig", "RMHE08")
#: `orig/` is copied below this size and junctioned above it (measured 2026-09-27: 6.6 MB here).
ORIG_JUNCTION_MIN_BYTES = 64 * 1024 * 1024
RMHE08_REL = os.path.join("build", "RMHE08")
#: The on-demand asm dump (92 MB, no ninja edge reads it) is never seeded.
SEED_SKIP_DIRS = ("asm",)
#: The generated files ninja reads to decide what is built; `.ninja_deps` is path-rewritten onto the target.
NINJA_STATE_FILES = (".ninja_deps", ".ninja_log", "build.ninja", "objdiff.json", "compile_commands.json")
_NINJA_DEPS_SIG = b"# ninjadeps\n"


def copy_missing_files(src: str, dst: str) -> int:
    """Copy every regular file under `src` into `dst`, skipping what exists. -> files copied."""
    copied = 0
    for dirpath, _dirnames, filenames in os.walk(src):
        rel = os.path.relpath(dirpath, src)
        target_dir = dst if rel == "." else os.path.join(dst, rel)
        os.makedirs(target_dir, exist_ok=True)
        for name in sorted(filenames):
            s, d = os.path.join(dirpath, name), os.path.join(target_dir, name)
            if os.path.exists(d):
                continue
            shutil.copy2(s, d)
            copied += 1
    return copied


def tree_size(path: str) -> int:
    """Bytes under `path`, never following a reparse point."""
    total = 0
    for root_dir, dirs, files in os.walk(path):
        dirs[:] = [d for d in dirs if not teardown.is_reparse_point(os.path.join(root_dir, d))]
        for name in files:
            try:
                total += os.path.getsize(os.path.join(root_dir, name))
            except OSError:
                pass
    return total


def copy_tree_missing(src: str, dst: str) -> int:
    """Copy `src` into `dst` recursively, skipping what exists and never walking a reparse point."""
    n = 0
    for root_dir, dirs, files in os.walk(src):
        dirs[:] = [d for d in dirs if not teardown.is_reparse_point(os.path.join(root_dir, d))]
        rel = os.path.relpath(root_dir, src)
        target = dst if rel == "." else os.path.join(dst, rel)
        os.makedirs(target, exist_ok=True)
        for name in files:
            s, d = os.path.join(root_dir, name), os.path.join(target, name)
            if os.path.exists(d) or teardown.is_reparse_point(s):
                continue
            shutil.copy2(s, d)
            n += 1
    return n


# --- the ninja deps log v4 --------------------------------------------------------------------------------------

def ninja_deps_serialize(version: int, records: list) -> bytes:
    """Re-serialize a parsed deps log: `("path", bytes)` / `("deps", out_id, mtime, [dep ids])` records."""
    out = bytearray(_NINJA_DEPS_SIG) + struct.pack("<I", version)
    seq = 0
    for rec in records:
        if rec[0] == "path":
            path = rec[1]
            pad = (4 - len(path) % 4) % 4
            out += struct.pack("<I", len(path) + pad + 4) + path + b"\0" * pad
            out += struct.pack("<i", -(seq + 1))
            seq += 1
        else:
            _kind, out_id, mtime, deps = rec
            payload = struct.pack("<IQ", out_id, mtime) + b"".join(struct.pack("<I", d) for d in deps)
            out += struct.pack("<I", 0x80000000 | len(payload)) + payload
    return bytes(out)


def ninja_deps_parse(data: bytes):
    """`(version, records)` for a deps log v4, or None when the bytes are not one."""
    if not data.startswith(_NINJA_DEPS_SIG):
        return None
    try:
        version = struct.unpack_from("<I", data, len(_NINJA_DEPS_SIG))[0]
        off = len(_NINJA_DEPS_SIG) + 4
        records = []
        while off < len(data):
            (size,) = struct.unpack_from("<I", data, off)
            off += 4
            if size & 0x80000000:
                n = size & 0x7FFFFFFF
                payload = data[off:off + n]
                off += n
                out_id, mtime = struct.unpack_from("<IQ", payload, 0)
                nd = (n - 12) // 4
                deps = list(struct.unpack_from("<%dI" % nd, payload, 12)) if nd else []
                records.append(("deps", out_id, mtime, deps))
            else:
                payload = data[off:off + size]
                off += size
                records.append(("path", payload[:-4].rstrip(b"\0")))
        if off != len(data):
            return None
        return version, records
    except (struct.error, ValueError):
        return None


def ninja_deps_rewrite(data: bytes, old_root: str, new_root: str) -> bytes | None:
    """The deps log with every source path under `old_root` moved to `new_root`; None on any surprise (bad
    signature, a log that does not round-trip byte for byte, a path that would still name MAIN)."""
    parsed = ninja_deps_parse(data)
    if parsed is None:
        return None
    version, records = parsed
    if ninja_deps_serialize(version, records) != data:
        return None
    roots = [os.path.abspath(old_root).replace("\\", "/").encode(),
             os.path.abspath(old_root).replace("/", "\\").encode()]
    news = {b"/": os.path.abspath(new_root).replace("\\", "/").encode(),
            b"\\": os.path.abspath(new_root).replace("/", "\\").encode()}
    out = []
    for rec in records:
        if rec[0] != "path":
            out.append(rec)
            continue
        raw = rec[1]
        path, moved = raw, False
        for old in roots:
            sep = b"\\" if b"\\" in old else b"/"
            if raw == old:
                path, moved = news[sep], True
                break
            if raw.startswith(old + sep):
                path, moved = news[sep] + raw[len(old):], True
                break
        if not moved:
            for old in roots:
                sep = b"\\" if b"\\" in old else b"/"
                if raw == old or raw.startswith(old + sep):
                    return None
        out.append(("path", path))
    return ninja_deps_serialize(version, out)


# --- the build tree ---------------------------------------------------------------------------------------------

def copy_build_outputs(src: str, dst: str, overwrite: bool = False):
    """Copy a build tree preserving mtimes -> `(files copied, oldest source mtime)`. `overwrite` (the slot
    shape) recopies a file whose size or mtime differs; the oldest mtime covers every file walked."""
    files, oldest = 0, None
    for root_dir, dirs, names in os.walk(src):
        dirs[:] = [d for d in dirs
                   if d not in SEED_SKIP_DIRS and not teardown.is_reparse_point(os.path.join(root_dir, d))]
        rel = os.path.relpath(root_dir, src)
        target = dst if rel == "." else os.path.join(dst, rel)
        os.makedirs(target, exist_ok=True)
        for name in sorted(names):
            s, d = os.path.join(root_dir, name), os.path.join(target, name)
            if teardown.is_reparse_point(s) or teardown.is_reparse_point(d):
                continue
            try:
                mt = os.path.getmtime(s)
                if oldest is None or mt < oldest:
                    oldest = mt
                if os.path.exists(d):
                    if not overwrite:
                        continue
                    st_s, st_d = os.stat(s), os.stat(d)
                    if st_s.st_size == st_d.st_size and st_s.st_mtime_ns == st_d.st_mtime_ns:
                        continue
                shutil.copy2(s, d)
            except OSError:
                continue
            files += 1
    return files, oldest


def seed_ninja_state(main: str, wt: str, overwrite: bool = False) -> list[str]:
    """Copy the ninja state a first `ninja` reads; `.ninja_deps` is re-pointed at `wt`, never copied raw."""
    seeded = []
    for name in NINJA_STATE_FILES:
        src, dst = os.path.join(main, name), os.path.join(wt, name)
        if not os.path.isfile(src):
            continue
        try:
            if name == ".ninja_deps":
                if os.path.exists(dst) and not overwrite:
                    continue
                with open(src, "rb") as fh:
                    moved = ninja_deps_rewrite(fh.read(), main, wt)
                if moved is None:
                    continue
                with open(dst, "wb") as fh:
                    fh.write(moved)
            else:
                if os.path.exists(dst):
                    if not overwrite:
                        continue
                    with open(src, "rb") as a, open(dst, "rb") as b:
                        if a.read() == b.read():
                            continue
                shutil.copy2(src, dst)
        except OSError:
            continue
        seeded.append(name)
    return seeded


def build_is_current(build_root: str, input_root: str | None = None) -> bool:
    """Whether `build_root`'s split (`config.json`) and manifest (`build.ninja`) are newer than the map/DOL and
    configure inputs under `input_root` (default: the same tree). One rule for the seeder, the slot guard and the
    artifact registry: `lib.artifacts.split_reasons` and `manifest_reasons` are it."""
    return not artifacts.split_reasons(build_root, input_root) and not artifacts.manifest_reasons(build_root, input_root)


def main_build_is_current(main: str) -> bool:
    """MAIN's own build tree against MAIN's own inputs."""
    return build_is_current(main, main)


def tracked_files(wt: str) -> list[str]:
    try:
        p = proc.run(["git", "-C", wt, "ls-files", "-z"])
    except OSError:
        return []
    if p.returncode != 0:
        return []
    return [f for f in (p.stdout or "").split("\0") if f]


def age_seeded_inputs(wt: str, t: float) -> int:
    """Stamp the worktree's tracked files and toolchain trees older than `t` (the oldest seeded output), so
    ninja sees the seeded objects as built. Content-preserving; returns the paths touched."""
    touched = 0
    for rel in tracked_files(wt):
        try:
            os.utime(os.path.join(wt, rel), (t, t))
            touched += 1
        except OSError:
            pass
    for rel in SEED_COPY_DIRS:
        top = os.path.join(wt, rel)
        if not os.path.isdir(top):
            continue
        for root_dir, _dirs, names in os.walk(top):
            try:
                os.utime(root_dir, (t, t))
                touched += 1
            except OSError:
                pass
            for name in names:
                try:
                    os.utime(os.path.join(root_dir, name), (t, t))
                    touched += 1
                except OSError:
                    pass
    return touched


def seed_worktree_build(main: str, wt: str, copy_orig: bool | None = None, overwrite: bool = False) -> str:
    """Seed `wt` from MAIN (never writing MAIN): toolchain copies, the `tools/m2c` submodule, `orig/RMHE08`
    (copied below `ORIG_JUNCTION_MIN_BYTES`, else junctioned), and - when MAIN's build is current - the build
    tree plus ninja state with the inputs aged behind it. `overwrite=True` is the slot refresh. Returns a
    one-line description of what was seeded."""
    parts: list[str] = []
    seeded = False
    for rel in SEED_COPY_DIRS:
        src, dst = os.path.join(main, rel), os.path.join(wt, rel)
        if not os.path.isdir(src):
            continue
        n = copy_missing_files(src, dst)
        seeded = True
        parts.append("%s: %d file(s)" % (rel.replace(os.sep, "/"), n))
    if os.path.isdir(os.path.join(wt, "tools")) and not os.path.exists(os.path.join(wt, "tools", "m2c", "m2c.py")):
        subprocess.run(["git", "-C", wt, "submodule", "update", "--init", "tools/m2c"],
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
        if os.path.exists(os.path.join(wt, "tools", "m2c", "m2c.py")):
            seeded = True
            parts.append("tools/m2c: submodule initialised")
    orig_src, orig_dst = os.path.join(main, ORIG_REL), os.path.join(wt, ORIG_REL)
    if os.path.isdir(orig_src):
        os.makedirs(orig_dst, exist_ok=True)
        size = tree_size(orig_src)
        if copy_orig is None:
            copy_orig = size < ORIG_JUNCTION_MIN_BYTES
        seeded = True
        if copy_orig:
            n = copy_tree_missing(orig_src, orig_dst)
            parts.append("orig/RMHE08: copied %d file(s), %.1f MB" % (n, size / 1048576.0))
        else:
            linked: list[str] = []
            copied = 0
            for name in sorted(os.listdir(orig_src)):
                s, d = os.path.join(orig_src, name), os.path.join(orig_dst, name)
                if os.path.exists(d):
                    continue
                if os.path.isdir(s):
                    if teardown.make_junction(d, s):
                        linked.append(name)
                elif os.path.isfile(s):
                    shutil.copy2(s, d)
                    copied += 1
            if linked:
                parts.append("orig/RMHE08: junction %s" % ", ".join(linked))
            if copied:
                parts.append("orig/RMHE08: %d file(s)" % copied)
    rmhe_src, rmhe_dst = os.path.join(main, RMHE08_REL), os.path.join(wt, RMHE08_REL)
    if os.path.isdir(rmhe_src) and main_build_is_current(main):
        os.makedirs(rmhe_dst, exist_ok=True)
        n, oldest = copy_build_outputs(rmhe_src, rmhe_dst, overwrite=overwrite)
        state = seed_ninja_state(main, wt, overwrite=overwrite)
        if oldest is not None:
            age_seeded_inputs(wt, oldest - 1.0)
        seeded = True
        parts.append("build/RMHE08: %d file(s)%s" % (n, " + " + ", ".join(state) if state else ""))
    if not seeded:
        return "skipped (MAIN has no build/tools yet - run `ninja tools`)"
    return "seeded " + "; ".join(parts)
