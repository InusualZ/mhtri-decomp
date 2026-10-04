#!/usr/bin/env python3
"""Fetch a native mingw-w64 gdb (plus its runtime DLLs) without MSYS2/pacman, for the mwcc-debugger port.
Spec: docs/tools/spec/mwcc-debugger.md. CLI: fetch_gdb.py --dest DIR."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import sys
import tarfile
import urllib.request

import zstandard

DEFAULT_URL = "https://repo.msys2.org/mingw/mingw64"

# What we actually want, and the packages the .db's DEPENDS fields do not
# mention: in the current GCC 16 packaging `cc-libs`/`gcc-libs` are empty
# meta-packages whose real contents are these split packages, so a resolver
# that trusts DEPENDS alone ends up with a gdb that cannot start (missing
# libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll).
WANTED = ["mingw-w64-x86_64-gdb"]
EXTRA = [
    "mingw-w64-x86_64-libstdc++",
    "mingw-w64-x86_64-libgcc",
    "mingw-w64-x86_64-libwinpthread",
    "mingw-w64-x86_64-libatomic",
    "mingw-w64-x86_64-libgomp",
    "mingw-w64-x86_64-libquadmath",
]


def parse_db(path):
    """{package name: {FIELD: [values]}} from an MSYS2 .db archive."""
    pkgs = {}
    with open(path, "rb") as fh:
        dctx = zstandard.ZstdDecompressor()
        with dctx.stream_reader(fh) as reader:
            with tarfile.open(fileobj=reader, mode="r|") as tf:
                for member in tf:
                    if not member.isfile() or not member.name.endswith("/desc"):
                        continue
                    data = tf.extractfile(member).read().decode("utf-8", "replace")
                    fields, cur = {}, None
                    for line in data.splitlines():
                        line = line.strip()
                        if not line:
                            continue
                        if line.startswith("%") and line.endswith("%"):
                            cur = line.strip("%")
                        elif cur:
                            fields.setdefault(cur, []).append(line)
                    pkgs[fields.get("NAME", [member.name.split("/")[0]])[0]] = fields
    return pkgs


def resolve(db, roots):
    order, seen = [], set()
    queue = list(roots)
    while queue:
        name = queue.pop(0)
        if name in seen:
            continue
        seen.add(name)
        if name not in db:
            print(f"WARNING: {name} is not in the package database", file=sys.stderr)
            continue
        order.append(name)
        queue.extend(db[name].get("DEPENDS", []))
    return order


def fetch(url, filename, cache):
    os.makedirs(cache, exist_ok=True)
    path = os.path.join(cache, filename)
    if not os.path.exists(path):
        print("GET", f"{url}/{filename}")
        urllib.request.urlretrieve(f"{url}/{filename}", path)
    return path


def extract(path, dest):
    print("extract", os.path.basename(path))
    dctx = zstandard.ZstdDecompressor()
    try:
        with open(path, "rb") as fh, dctx.stream_reader(fh) as reader:
            with tarfile.open(fileobj=reader, mode="r|") as tf:
                tf.extractall(dest, filter="data")
    except Exception as exc:  # symlink-heavy packages (tzdata) on Windows
        print("  warning:", exc, file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dest", required=True, help="prefix directory to unpack into")
    ap.add_argument("--url", default=DEFAULT_URL, help="MSYS2 mingw64 repository URL")
    ap.add_argument(
        "--packages", nargs="*", default=WANTED, help="packages to install"
    )
    ap.add_argument(
        "--cache", default="mwcc-dbg-pkgs", help="where to keep the downloaded packages"
    )
    args = ap.parse_args()

    db_path = os.path.join(args.cache, "mingw64.db")
    os.makedirs(args.cache, exist_ok=True)
    if not os.path.exists(db_path):
        print("GET", args.url + "/mingw64.db")
        urllib.request.urlretrieve(args.url + "/mingw64.db", db_path)

    db = parse_db(db_path)
    print(f"{len(db)} packages in the database")
    resolved = resolve(db, list(args.packages) + EXTRA)
    print(f"{len(resolved)} packages to install")

    for name in resolved:
        fields = db[name]
        filename = fields.get(
            "FILENAME", [f"{name}-{fields['VERSION'][0]}-any.pkg.tar.zst"]
        )[0]
        extract(fetch(args.url, filename, args.cache), args.dest)

    gdb = os.path.join(args.dest, "mingw64", "bin", "gdb.exe")
    print()
    print("gdb:", gdb, "(exists)" if os.path.exists(gdb) else "(MISSING)")
    if not os.path.exists(gdb):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
