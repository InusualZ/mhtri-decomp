#!/usr/bin/env python3
"""Inventory RSO modules: header, sections, and the export/import symbol tables. Spec: docs/tools/spec/rso.md.
CLI: inventory.py [--dir DIR] [--rso FILE] [--sections] [--symbols] [--json OUT]."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import json
import os
import struct
import sys

from tools.lib import repo


def cstr(data, off):
    if not (0 <= off < len(data)):
        return ""
    end = data.find(b"\0", off)
    return data[off:end if end >= 0 else len(data)].decode("latin-1")


def parse(path):
    data = open(path, "rb").read()
    if len(data) < 0x58:
        return None
    u32 = lambda o: struct.unpack_from(">I", data, o)[0]
    h = dict(path=path, size=len(data), num_sections=u32(0x08), section_info=u32(0x0C),
             name_offset=u32(0x10), name_size=u32(0x14), version=u32(0x18), bss_size=u32(0x1C),
             prolog_section=data[0x20], epilog_section=data[0x21], unresolved_section=data[0x22],
             prolog_offset=u32(0x24), epilog_offset=u32(0x28), unresolved_offset=u32(0x2C),
             internal_rel_offset=u32(0x30), internal_rel_size=u32(0x34),
             external_rel_offset=u32(0x38), external_rel_size=u32(0x3C),
             export_table_offset=u32(0x40), export_table_size=u32(0x44),
             export_table_name_offset=u32(0x48), import_table_offset=u32(0x4C),
             import_table_size=u32(0x50), import_table_name_offset=u32(0x54))
    h["name"] = cstr(data, h["name_offset"]) if h["name_offset"] else ""

    h["sections"] = []
    if 0 < h["num_sections"] < 256 and h["section_info"] + 8 * h["num_sections"] <= len(data):
        for i in range(h["num_sections"]):
            flags, size = struct.unpack_from(">II", data, h["section_info"] + 8 * i)
            off = flags & ~1
            # A section with offset 0 is allocated at runtime (bss), it has no bytes in the file.
            in_file = off != 0 and off + size <= len(data)
            h["sections"].append(dict(index=i, offset=off, exec=bool(flags & 1), size=size,
                                      in_file=in_file))

    def symbols(table_off, table_size, name_off, stride):
        out = []
        if not table_off or table_off + table_size > len(data):
            return out
        for o in range(table_off, table_off + table_size, stride):
            if o + stride > len(data):
                break
            vals = struct.unpack_from(">" + "I" * (stride // 4), data, o)
            out.append(dict(name=cstr(data, name_off + vals[0]), offset=vals[1], section=vals[2],
                            extra=vals[3] if stride == 16 else None))
        return out

    h["exports"] = symbols(h["export_table_offset"], h["export_table_size"],
                           h["export_table_name_offset"], 16)
    h["imports"] = symbols(h["import_table_offset"], h["import_table_size"],
                           h["import_table_name_offset"], 12)
    h["section_bytes"] = sum(s["size"] for s in h["sections"] if s["in_file"])
    # Retail RSOs do not set the section exec flag, so the code size is taken from the section that
    # holds the module's prolog entry point.
    code = [s for s in h["sections"] if s["index"] == h["prolog_section"]]
    h["code_bytes"] = code[0]["size"] if code else 0
    return h


def label(name):
    return os.path.basename(name.replace("\\", "/")) if name else "?"


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dir", default=os.path.join(repo.repo_root(), "orig", "RMHE08", "files"))
    ap.add_argument("--rso", action="append", default=[])
    ap.add_argument("--json")
    ap.add_argument("--sections", action="store_true", help="print each module's section table")
    ap.add_argument("--symbols", action="store_true", help="print each module's exports/imports")
    args = ap.parse_args()

    paths = list(args.rso)
    if not paths:
        for root, _d, files in os.walk(args.dir):
            paths += [os.path.join(root, f) for f in sorted(files) if f.lower().endswith(".rso")]
    mods = [m for m in (parse(p) for p in sorted(paths)) if m]
    if not mods:
        raise SystemExit("no RSO files found under %s" % args.dir)
    mods.sort(key=lambda m: -m["size"])

    print("%-18s %9s %4s %9s %9s %8s %6s  %s" % ("file", "size", "sec", "in-file", "code", "bss",
                                                 "exp/imp", "original build name"))
    for m in mods:
        print("%-18s %9d %4d %9d %9d %8d %3d/%-3d %s" % (
            os.path.basename(m["path"]), m["size"], len(m["sections"]), m["section_bytes"],
            m["code_bytes"], m["bss_size"], len(m["exports"]), len(m["imports"]), label(m["name"])))
    print("%-18s %9d  (%d modules)" % ("TOTAL", sum(m["size"] for m in mods), len(mods)))

    if args.sections:
        for m in mods:
            print("\n== %s  (%s)" % (os.path.basename(m["path"]), m["name"]))
            for s in m["sections"]:
                print("   [%2d] offset 0x%06x size 0x%06x %-4s %s" % (
                    s["index"], s["offset"], s["size"], "exec" if s["exec"] else "",
                    "" if s["in_file"] else "(not in file)"))
            print("   prolog %d:0x%x  epilog %d:0x%x  unresolved %d:0x%x  bss 0x%x" % (
                m["prolog_section"], m["prolog_offset"], m["epilog_section"], m["epilog_offset"],
                m["unresolved_section"], m["unresolved_offset"], m["bss_size"]))

    if args.symbols:
        for m in mods:
            print("\n== %s exports (%d)" % (os.path.basename(m["path"]), len(m["exports"])))
            for s in m["exports"]:
                print("   sec %2d +0x%-6x hash %08x  %s" % (s["section"], s["offset"], s["extra"],
                                                           s["name"]))
            if m["imports"]:
                print("   imports: " + ", ".join(s["name"] for s in m["imports"]))

    if args.json:
        os.makedirs(os.path.dirname(args.json), exist_ok=True)
        json.dump(mods, open(args.json, "w", encoding="utf-8"), indent=1)
        print("wrote", os.path.relpath(args.json, repo.repo_root()))


if __name__ == "__main__":
    main()
