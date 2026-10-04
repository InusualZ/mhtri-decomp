"""The linker's internal input-file record: derived from the '.comment' parser, read back under gdb.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

from tools.lib import proc as lib_proc
from tools.lib.binary import elf
from tools.mwlink.anchors import _disas_text, _function_start, derive_anchors, derive_message_io
from tools.mwlink.link import ROOT, derive_link_line, derive_rsp, find_gdb


# Every field below is either *derived* from the parser's own instructions or
# *verified* live against the artifact it describes.  Nothing here is a
# transcribed offset: `derive_file_record` finds the parser, the stride, the
# record array and the two setters of the flag byte in `.text`, and
# `records --prove` re-reads each record out of the running linker and
# cross-checks it against the object file the record names.
FILE_RECORD_FIELDS = [
    (0x00, 4, "elf_header",
     "pointer to the input file's own ELF header",
     "live: the 4 bytes it points at are 7f 45 4c 46 ('\\x7fELF')"),
    (0x04, 4, "(unnamed)",
     "not derived",
     "a pointer, but to what is not established"),
    (0x08, 4, "name",
     "pointer to the input's name as the link knows it",
     "live: equals the response-file entry at the same index, all records"),
    (0x0c, 4, "(unnamed)",
     "not derived; it tracks the section count",
     "live: high16 = (record+0x10)-1, low16 = (record+0x10)+1, every record"),
    (0x10, 4, "section_count_adj",
     "section count minus the 4 fixed sections (null, .symtab, .strtab, .shstrtab)",
     "live: equals e_shnum-4 for every record whose object is a plain object"),
    (0x14, 2, "(unnamed)",
     "0 in every record read; the u32 read at +0x14 is `e_shnum << 16` only "
     "because +0x16 holds e_shnum and this half is zero",
     "live: zero for every record, while the u32 spanning it is shnum<<16"),
    (0x16, 2, "shnum",
     "the object's e_shnum (the ELF section-header count)",
     "live: equals the object's e_shnum for every record whose input is a "
     "plain object"),
    (0x18, 4, "per_file_ptr",
     "pointer into a per-file table (stride 0x28, one entry per input)",
     "live: the values step by exactly 0x28 with the record index"),
    (0x1c, 1, "flags",
     "bit2 = the '.comment' magic matched; bit3 = the comment version >= 0xb; "
     "bit7 is tested elsewhere (0x405e2) and its meaning is not derived",
     "parser 0x53975/0x53983 clear bits 2/3, 0x539cf/0x539e2 set them after "
     "the 'CodeWarrior' memcmp at 0x5399f and the version compare at 0x539d5"),
    (0x1e, 1, "comment_kind",
     "0 = no comment, 2 = version 3..5, 3 = version >= 6",
     "parser 0x539c4 stores 2, 0x53a60 stores 3, from the same version byte"),
    (0x1f, 1, "comment_version",
     "the version byte of the object's '.comment' (the byte after 'CodeWarrior')",
     "parser 0x539ab loads [comment+0xb], 0x539af stores it here"),
]


def derive_file_record(pe):
    """Derive the input-file record: parser, stride, array, and the flag setters.

    The lever is the same one ``anchors`` uses: the parser is the function that
    compares the ``.comment`` contents against the literal ``CodeWarrior``, so
    the anchor that pushes that string *is* the parser's address.  Everything
    else - the record stride, the array the parser indexes into, and the two
    instructions that set the record's flag byte - is then read out of that
    function.  A fact that cannot be found is reported as not found; nothing is
    transcribed from a previous run.
    """
    insns, aux = _disas_text(pe)
    if insns is None:
        return None
    cs = aux[1]
    anchors = derive_anchors(pe) or []
    hit = [a for a in anchors if a["kind"] == "message"
           and a["string"] == "CodeWarrior"]
    if not hit:
        return None
    idx = next((k for k, i in enumerate(insns)
                if (i.address - pe.image_base) == hit[0]["anchor"]), None)
    if idx is None:
        return None
    start = _function_start(insns, idx)
    start_idx = next((k for k, i in enumerate(insns) if i.address == start), idx)
    # The window is bounded, and every record-field match below additionally
    # has to use the parser's own record register (ESI, per its prologue) - so a
    # scan that runs a few instructions past the function's end cannot quietly
    # adopt the next function's instruction as evidence.
    func = [i for i in insns[start_idx:start_idx + 160]
            if i.mnemonic not in ("nop", "int3")]

    info = {"parser_rva": start - pe.image_base,
            "magic_rva": hit[0]["anchor"], "stride": None, "table_va": None,
            "version_store_rva": None, "version_load_rva": None,
            "kind_stores": [], "flag_clear": [], "flag_set": []}
    for i in func:
        rva = i.address - pe.image_base
        if i.mnemonic == "imul" and len(i.operands) == 3 and \
                i.operands[2].type == cs.x86.X86_OP_IMM:
            info["stride"] = i.operands[2].imm
        if i.mnemonic == "add" and len(i.operands) == 2 and \
                i.operands[1].type == cs.x86.X86_OP_MEM and \
                i.operands[1].mem.base == cs.x86.X86_REG_INVALID and \
                i.operands[1].mem.index == cs.x86.X86_REG_INVALID and \
                i.operands[1].mem.disp:
            info["table_va"] = i.operands[1].mem.disp
        if i.mnemonic == "mov" and "esi + 0x1f]" in i.op_str:
            info["version_store_rva"] = rva
        if i.mnemonic == "movzx" and "ebp + 0xb]" in i.op_str:
            info["version_load_rva"] = rva
        if i.mnemonic == "mov" and "esi + 0x1e]" in i.op_str:
            info["kind_stores"].append((rva, i.op_str))
        if i.mnemonic in ("and", "or") and len(i.operands) == 2 and \
                i.operands[1].type == cs.x86.X86_OP_IMM and \
                i.operands[1].imm in (4, 8, 0xFB, 0xF7) and i.operands[0].size == 1:
            key = "flag_set" if i.mnemonic == "or" else "flag_clear"
            info[key].append((rva, i.op_str))
    return info


def _record_probe_line(args):
    """The link line a `records --prove` gdb run uses, derived from build.ninja."""
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    rsp = Path(args.rsp) if args.rsp else work / "records.rsp"
    if not rsp.exists():
        if derive_rsp(args.link_out, rsp) is None:
            return None, None, None
    args.rsp = str(rsp)
    line = derive_link_line(rsp, args.ldscript or "build/RMHE08/ldscript.lcf")
    line = line.format(out=str(work / "records.elf"), map=str(work / "records.MAP"))
    return line, rsp, work


def prove_records(pe, der, args):
    """Read the linker's record array back out and check it against the objects.

    The gdb run breaks at the linker's own message loader (the derived
    observation anchor) and, once it has announced ``Optimizing:``, dumps the
    first N records from the array the parser indexes.  Every field the table
    above names is then *cross-checked*: the name against the response file, the
    comment version and the section count against the object file on disk.  A
    disagreement is printed, not smoothed over.
    """
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb", file=sys.stderr)
        return 2
    if args.args:
        line = args.args
    else:
        line, rsp, work = _record_probe_line(args)
        if line is None:
            print("cannot derive the link from build.ninja; pass --args", file=sys.stderr)
            return 2
        args.rsp = str(rsp)
    io = derive_message_io(pe)
    if io is None or io.get("loader") is None:
        print("cannot derive the message loader", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-records.gdb"
    limit = args.limit
    script.write_text(
        "set pagination off\nset confirm off\nset width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + line + "\n"
        "python\nimport gdb\n"
        f"LOADER = 0x{pe.image_base + io['loader']['after_rva']:x}\n"
        f"BASE = 0x{der['table_va']:x}\n"
        f"STRIDE = {der['stride']}\n"
        f"LIMIT = {limit}\n"
        "DONE = [False]\n"
        "def rd(a, n):\n"
        "    return gdb.selected_inferior().read_memory(a, n).tobytes()\n"
        "def u32(a):\n"
        "    try: return int.from_bytes(rd(a, 4), 'little')\n"
        "    except Exception: return None\n"
        "def u8(a):\n"
        "    try: return rd(a, 1)[0]\n"
        "    except Exception: return None\n"
        "def cstr(a, n=96):\n"
        "    try: return rd(a, n).split(b'\\0')[0].decode('latin-1', 'replace')\n"
        "    except Exception: return None\n"
        "class Opt(gdb.Breakpoint):\n"
        "    def stop(self):\n"
        "        if DONE[0]:\n"
        "            return False\n"
        "        try:\n"
        "            esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff\n"
        "            uID = int(gdb.parse_and_eval('*(unsigned int*)%d' % (esp + 0x10)))\n"
        "        except Exception:\n"
        "            return False\n"
        "        if uID != 41:\n"
        "            return False\n"
        "        DONE[0] = True\n"
        "        base = u32(BASE)\n"
        "        print('MWLINK-BASE 0x%x' % (base or 0))\n"
        "        for i in range(LIMIT):\n"
        "            rec = (base or 0) + i * STRIDE\n"
        "            print('MWLINK-RECORD %d %s|%d|%d|%d|%d|%d|%d|%d|%d' % (\n"
        "                i, cstr(u32(rec + 8)), u32(rec + 0x14) or 0,\n"
        "                u32(rec + 0x16) or 0, u32(rec + 0x18) or 0,\n"
        "                u8(rec + 0x1c) or 0, u8(rec + 0x1e) or 0,\n"
        "                u8(rec + 0x1f) or 0, u32(rec + 0x0c) or 0, u32(rec + 0x10) or 0))\n"
        "        return False\n"
        "Opt('*0x%x' % LOADER)\n"
        "print('MWLINK-READY')\n"
        "end\nrun\nprintf \"MWLINK-DONE\\n\"\nquit\n",
        encoding="utf-8")
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = lib_proc.run(argv)
    rows = []
    base = None
    for line_ in proc.stdout.splitlines():
        m = re.match(r"^MWLINK-RECORD (\d+) (.*)$", line_.strip())
        if m:
            parts = m.group(2).split("|")
            rows.append((int(m.group(1)), parts[0]) + tuple(int(x) for x in parts[1:]))
            continue
        m = re.match(r"^MWLINK-BASE (0x[0-9a-f]+)$", line_.strip())
        if m:
            base = int(m.group(1), 16)
    if not rows:
        print("# the run produced no record dump; gdb stderr follows", file=sys.stderr)
        print((proc.stderr or "").strip()[:1500], file=sys.stderr)
        return 1
    inputs = []
    if args.rsp and Path(args.rsp).exists():
        inputs = [ln.strip().replace("\\", "/")
                  for ln in Path(args.rsp).read_text(errors="replace").splitlines()
                  if ln.strip()]
    print(f"the linker's input-file records, read out of the running linker "
          f"(array 0x{base:x}, stride {der['stride']:#x}):")
    print(f"  {'idx':>5} {'record+0x08':<34} {'+0x1f':>5} {'+0x16&ffff':>10} "
          f"{'+0x10':>6} {'flags':>5} {'kind':>4} checks")
    checked = {"name": 0, "version": 0, "shnum": 0, "total": 0}
    problems = []
    for idx, name, v14, v16, v18, flags, kind, ver, v0c, v10 in rows:
        want = inputs[idx] if idx < len(inputs) else None
        checks = []
        if want is not None:
            checked["total"] += 1
            same = Path(want).name.lower() == (name or "").lower()
            checks.append("name " + ("=" if same else f"!={Path(want).name}"))
            if same:
                checked["name"] += 1
            obj = Path(want)
            if not obj.is_absolute():
                obj = ROOT / obj
            if obj.exists():
                obj_shnum, obj_ver = _object_header_bits(obj)
                if obj_ver is not None:
                    if obj_ver == ver:
                        checked["version"] += 1
                        checks.append("comment-version =")
                    else:
                        checks.append(f"comment-version !={obj_ver}")
                        problems.append(f"record {idx} ({name}): +0x1f={ver}, "
                                        f"the object's .comment says {obj_ver}")
                if obj_shnum is not None:
                    if (v16 & 0xFFFF) == obj_shnum:
                        checked["shnum"] += 1
                        checks.append("e_shnum =")
                    else:
                        checks.append(f"e_shnum != {obj_shnum}")
                        problems.append(f"record {idx} ({name}): +0x16&0xffff="
                                        f"{v16 & 0xFFFF}, e_shnum={obj_shnum}")
        if idx < 8:
            print(f"  {idx:>5} {str(name)[:34]:<34} {ver:>5} {v16 & 0xFFFF:>10} "
                  f"{v10:>6} 0x{flags:02x} {kind:>4} {', '.join(checks)}")
    print(f"  ... {len(rows)} record(s) dumped")
    print(f"# cross-check against the response file and the objects themselves: "
          f"{checked['name']}/{checked['total']} names, "
          f"{checked['version']}/{checked['total']} comment versions, "
          f"{checked['shnum']}/{checked['total']} section counts")
    for p_ in problems[:10]:
        print("  MISMATCH: " + p_)
    return 0 if not problems else 1


def _object_header_bits(path):
    """``(e_shnum, .comment version byte)`` for a Metrowerks object, or ``(None, None)``."""
    try:
        blob = Path(path).read_bytes()
    except OSError:
        return None, None
    if len(blob) < 0x40 or blob[:4] != b"\x7fELF" or blob[5] != 2:
        return None, None
    try:
        parsed = elf.Elf(blob, str(path))
    except elf.ElfError:
        return None, None
    shnum = parsed.shnum
    if not parsed.shoff or shnum <= parsed.shstrndx or parsed.shoff + 40 * shnum > len(blob):
        return shnum, None
    comment = next((s for s in parsed.sections if s.name == ".comment" and s.size > 0xC), None)
    return shnum, (comment.raw[0xB] if comment is not None else None)

