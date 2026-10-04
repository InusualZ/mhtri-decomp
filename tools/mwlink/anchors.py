"""Anchors derived from the linker PE: the ctor/dtor order table, string anchors, the phase table, gdb proofs.
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import functools
import re
import struct
import sys
from pathlib import Path

from tools.lib import proc as lib_proc
from tools.mwlink.catalogue import message_catalogue
from tools.mwlink.link import find_gdb


def _string_targets(pe, min_len=6, max_len=120):
    """``{va: text}`` for every null-terminated printable string in the image.

    These are the strings the linker itself passes to its message printer, so
    an instruction whose immediate operand is one of these VAs is a place the
    linker used that string.  ``.rdata`` and ``.data`` are both scanned: the
    option tables and the ctor/dtor name pool live in ``.data`` in these
    binaries, while the diagnostic text is in ``.rdata``.
    """
    out = {}
    for sec_name in (".rdata", ".data"):
        sec = pe.section(sec_name)
        if sec is None or sec.raw_size == 0:
            continue
        data = pe.read_rva(sec.va, sec.vsize)
        for m in re.finditer(rb"[\x20-\x7e]{%d,%d}\x00" % (min_len, max_len), data):
            va = pe.image_base + sec.va + m.start()
            out[va] = m.group()[:-1].decode("latin-1")
    return out


def _ctor_dtor_names(pe):
    """``[(rva, name), ...]`` for the string-pool entries that name a
    ctor/dtor section (``.ctors``, ``.ctors$10``, ``.dtors$15``, ...)."""
    rx = re.compile(r"^\.(ctors|dtors)(\$\w+)?$")
    out = []
    for va, text in _string_targets(pe, min_len=6, max_len=16).items():
        if rx.match(text):
            out.append((va - pe.image_base, text))
    out.sort()
    return out


def derive_order(pe):
    """The linker's fixed ctor/dtor order list, derived from a pointer array.

    The linker identifies each special section by name; the *order* those names
    are laid out in is a fixed array of ``{name, ...}`` records in ``.data``
    (stride 0x2e in the Wii linkers), and the record order *is* the priority
    order.  Find every pointer into the ctor/dtor name pool, sort the cells by
    address, and that sequence is the answer - no address is transcribed.

    This is the mechanism behind Row 46: a unit whose object is byte-identical
    still moves by its ``.ctors$10`` / ``.dtors$15`` words because a fragment
    named ``.ctors$10`` is not ordered like a fragment named ``.ctors``; they
    are two distinct fixed slots in this list.
    """
    pool = {rva: name for rva, name in _ctor_dtor_names(pe)}
    if not pool:
        return None
    base = pe.image_base
    cells = []
    for sec in (pe.section(".data"), pe.section(".rdata")):
        if sec is None or sec.raw_size == 0:
            continue
        blob = pe.read_rva(sec.va, sec.vsize)
        # NOTE: scan every byte offset: the record stride is 0x2e (not a
        # multiple of 4), so the pointer field of every second record is only
        # 2-byte aligned and a 4-byte-stepped scan misses half of the table.
        for o in range(0, len(blob) - 4):
            v = struct.unpack_from("<I", blob, o)[0]
            if base <= v < base + pe.size_image and v - base in pool:
                cells.append((sec.va + o, pool[v - base]))
    cells.sort()
    # Keep only the longest stride-consistent run referencing distinct names:
    # a stray pointer elsewhere in .data is not part of the order table.
    diffs = [cells[i + 1][0] - cells[i][0] for i in range(len(cells) - 1)]
    stride = max(set(diffs), key=diffs.count) if diffs else None
    runs, cur = [], []
    for rva, name in cells:
        if cur and (rva - cur[-1][0] != stride or name in [n for _r, n in cur]):
            runs.append(cur)
            cur = []
        cur.append((rva, name))
    if cur:
        runs.append(cur)
    best = max(runs, key=len) if runs else []
    names = [n for _r, n in best]
    if not names:
        names = [n for _r, n in cells]
    return {"names": names, "cell_rvas": [r for r, _n in best],
            "stride": stride}


def _capstone():
    try:
        import capstone  # noqa: F401
    except ImportError:
        return None
    import capstone
    return capstone


def derive_anchors(pe, with_capstone=True):
    """``[{anchor, va, target, string, kind}, ...]`` derived from the image.

    Every ``.text`` instruction whose immediate operand is the VA of a string
    in ``.rdata``/``.data`` is a place the linker uses that string.  The
    ctor/dtor name compares (``mov edi, <va>; mov ecx, <len>; repe cmpsb``)
    are flagged as ``section-name`` anchors, because those are the Row 46
    decision points; everything else is a ``message`` anchor.
    """
    cs = _capstone() if with_capstone else None
    if cs is None:
        return None
    targets = _string_targets(pe)
    ctor_rvas = {pe.image_base + rva: name for rva, name in _ctor_dtor_names(pe)}

    insns = _text_insns(pe)
    anchors = []
    for i, ins in enumerate(insns):
        for op in ins.operands:
            if op.type != cs.x86.X86_OP_IMM or op.imm not in targets:
                continue
            rva = ins.address - pe.image_base
            is_name = op.imm in ctor_rvas
            entry = {
                "anchor": rva,
                "target_va": op.imm,
                "string": targets[op.imm],
                "kind": "section-name" if is_name else "message",
                "instruction": f"{ins.mnemonic} {ins.op_str}",
            }
            if is_name:
                # The exact compare length is the proof the match is exact.
                for j in (i + 1, i + 2):
                    if j < len(insns) and insns[j].mnemonic == "mov" \
                            and insns[j].operands and insns[j].operands[0].reg \
                            == cs.x86.X86_REG_ECX and insns[j].operands[1].type \
                            == cs.x86.X86_OP_IMM:
                        entry["compare_len"] = insns[j].operands[1].imm
                        break
            anchors.append(entry)
    anchors.sort(key=lambda a: a["anchor"])
    return anchors


def prove_anchors(pe, anchors, args):
    """Break on each derived anchor in a real link and count the hits.

    A breakpoint that never fires is *not* an anchor: it is reported as
    unproven.  This is the linker analogue of ``verify_pcode`` - the derived
    table is only believed after the binary is seen to reach it.
    """
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb or run tools/mwcc-debugger/fetch_gdb.py",
              file=sys.stderr)
        return 2
    if not args.args:
        print("--prove needs the linker argument list (--args '<link line>')", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-prove.gdb"
    # A gdb Python stop handler is used rather than `commands` + `printf`, so a
    # bad candidate pointer at one anchor cannot abort the whole run.
    table = "\n".join(
        "  (0x%x, '%08x', %r, %s)," % (
            pe.image_base + a["anchor"], a["anchor"], a["string"],
            "True" if a.get("compare_len") else "False",
        )
        for a in anchors
    )
    script.write_text(
        "set pagination off\n"
        "set confirm off\n"
        "set width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + args.args + "\n"
        "python\n"
        "import gdb\n"
        "ANCHORS = [\n" + table + "\n]\n"
        "class Anchor(gdb.Breakpoint):\n"
        "    def __init__(self, addr, tag, text, candidate):\n"
        "        super().__init__('*0x%x' % addr, internal=True)\n"
        "        self.tag, self.text, self.candidate = tag, text, candidate\n"
        "    def stop(self):\n"
        "        sec = ''\n"
        "        if self.candidate:\n"
        "            try:\n"
        "                p = int(gdb.parse_and_eval('$esi'))\n"
        "                raw = gdb.selected_inferior().read_memory(p, 32).tobytes()\n"
        "                s = raw.split(b'\\0')[0].decode('latin-1')\n"
        "                if s and all(32 <= ord(c) < 127 for c in s):\n"
        "                    sec = \" sec='%s'\" % s\n"
        "            except Exception:\n"
        "                sec = ' sec=?'\n"
        "        print('MWLINK-HIT %s%s' % (self.tag, sec))\n"
        "        return False\n"
        "for _a in ANCHORS:\n"
        "    Anchor(*_a)\n"
        "end\n"
        "run\n"
        "printf \"MWLINK-DONE\\n\"\n"
        "quit\n"
    )
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = lib_proc.run(argv)
    counts, sections = {}, {}
    for line in proc.stdout.splitlines():
        m = re.match(r"^MWLINK-HIT ([0-9a-f]{8})(?: sec='([^']*)')?", line.strip())
        if m:
            counts[m.group(1)] = counts.get(m.group(1), 0) + 1
            if m.group(2) is not None:
                sections.setdefault(m.group(1), set()).add(m.group(2))
    fired = 0
    print("proven on a real link (FIRED) / not reached by this link (UNPROVEN):")
    for a in anchors:
        key = f"{a['anchor']:08x}"
        n = counts.get(key, 0)
        fired += 1 if n else 0
        seen = sections.get(key)
        sec = ("  matched " + ", ".join(f"'{s}'" for s in sorted(seen))) if seen else ""
        print(f"  {'FIRED   ' if n else 'UNPROVEN'} {key} x{n:<5} {a['string']}{sec}")
    print(f"# {fired}/{len(anchors)} anchors fired")
    if not fired and proc.stderr.strip():
        print("# gdb stderr:\n" + proc.stderr.strip()[:1000], file=sys.stderr)
    # Report the artifact the run produced, if the command line says where.
    m = re.search(r"(?:^|\s)-o\s+(\S+)", args.args)
    if m:
        out = Path(m.group(1))
        if out.exists():
            import hashlib
            digest = hashlib.sha256(out.read_bytes()).hexdigest()
            print(f"# artifact {out} sha256 {digest}")
        else:
            print(f"# artifact {out} was NOT written", file=sys.stderr)
    if args.require_all:
        return 0 if fired == len(anchors) else 1
    return 0 if fired else 1


LOADSTRING = "LoadStringA"


def _imm(op_str):
    """The integer value of an immediate operand string, or None."""
    if op_str and re.fullmatch(r"0x[0-9a-f]+", op_str or ""):
        return int(op_str, 16)
    return None


def _function_start(insns, idx):
    """The address of the function containing ``insns[idx]``.

    These binaries carry no symbols, so the boundary has to come from the
    layout: Metrowerks pads *between* functions with `nop`/`int3` runs, so the
    entry is the first real instruction after the closest such run at or before
    ``idx``.  The derivation checks itself: ``derive_message_io`` reports how
    many of the entries it found are the target of a `call`, and an entry that
    nothing calls is reported as unproven rather than used.
    """
    j = idx
    while j > 0:
        if insns[j - 1].mnemonic in ("nop", "int3") and \
                insns[j].mnemonic not in ("nop", "int3"):
            return insns[j].address
        j -= 1
    return insns[0].address


def derive_message_io(pe):
    """The linker's message loader, its observation anchor, and the formatters."""
    cs = _capstone()
    if cs is None:
        return None
    slots = pe.iat_slots()
    slot = slots.get(LOADSTRING)
    if slot is None:
        return None
    insns = _text_insns(pe)
    sites = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type == cs.x86.X86_OP_MEM and op.mem.base == cs.x86.X86_REG_INVALID \
                and op.mem.index == cs.x86.X86_REG_INVALID and op.mem.disp == slot:
            sites.append(i)
    if not sites:
        return None

    def push_stream(i, n):
        """The last `n` pushes before `i`, in **instruction order** (first push first).

        ``argN`` of the callee is the *last* push, so ``stream[-N]`` is the
        argument - and saying it that way is what keeps the id derivation
        honest: nothing here assumes the pushes are immediates.
        """
        near = []
        k = i - 1
        while k >= 0 and len(near) < n:
            if insns[k].mnemonic == "push":
                near.append(insns[k].op_str)
            k -= 1
        return list(reversed(near))

    loaders = []
    for i in sites:
        start = _function_start(insns, i)
        args = push_stream(i, 4)
        # `LoadStringA(HINSTANCE, UINT uID, LPSTR, int)`: arg2 is the id, i.e.
        # the second push from the call.
        loaders.append({
            "call_rva": insns[i].address - pe.image_base,
            "after_rva": (insns[i].address + insns[i].size) - pe.image_base,
            "entry_rva": start - pe.image_base,
            "pushed": args,
            "msgid_arg2": _imm(args[-2]) if len(args) >= 2 else None,
            # The message loader takes the id as an *argument*; the other site
            # loads one fixed string for its own use, so its arg2 is a
            # constant.  That is the rule that tells them apart.
            "arg2_is_constant": _imm(args[-2]) is not None if len(args) >= 2 else False,
        })
    # The message loader is the one with many callers; a one-off `LoadStringA`
    # with a constant id is a different thing and is reported as such.
    call_targets = {}
    for ins in insns:
        if ins.mnemonic == "call" and ins.operands \
                and ins.operands[0].type == cs.x86.X86_OP_IMM:
            call_targets.setdefault(ins.operands[0].imm, 0)
            call_targets[ins.operands[0].imm] += 1
    for ld in loaders:
        callers = [ins.address - pe.image_base for ins in insns
                   if ins.mnemonic == "call" and ins.operands
                   and ins.operands[0].type == cs.x86.X86_OP_IMM
                   and ins.operands[0].imm == pe.image_base + ld["entry_rva"]]
        ld["callers"] = sorted(callers)
        ld["entry_is_call_target"] = call_targets.get(pe.image_base + ld["entry_rva"], 0)
    loader = max([l for l in loaders if not l["arg2_is_constant"]] or loaders,
                 key=lambda l: (len(l["callers"]), -l["call_rva"]))
    # The id is the *third* push in source order: the loader reads arg3 (its
    # `movsx ecx, word ptr [esp+0x18]` after two pushes, with arg1 in EBX as
    # the buffer) and hands it to `LoadStringA` as uID.  That reading is
    # falsifiable and `--prove` checks it against a real link.
    formatters = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type != cs.x86.X86_OP_IMM or op.imm - pe.image_base != loader["entry_rva"]:
            continue
        pushed = push_stream(i, 3)
        formatters.append({
            "func_rva": _function_start(insns, i) - pe.image_base,
            "call_rva": ins.address - pe.image_base,
            "msgid": _imm(pushed[-3]) if len(pushed) >= 3 else None,
            "pushed": pushed,
        })
    # Self-check: a printer entry has to be something the binary calls.
    for f in formatters:
        f["entry_is_call_target"] = call_targets.get(pe.image_base + f["func_rva"], 0)
    return {"iat_slot": slot, "loader": loader, "loaders": loaders,
            "formatters": formatters,
            "unproven_entries": sorted({f["func_rva"] for f in formatters
                                        if not f["entry_is_call_target"]})}


def derive_phase_anchors(pe):
    """``{anchor RVA: (msgid, what printed it)}`` for every phase call site."""
    io = derive_message_io(pe)
    if io is None:
        return None
    cs = _capstone()
    insns = _text_insns(pe)
    by_func = {}
    for f in io["formatters"]:
        by_func.setdefault(f["func_rva"], []).append(f)
    anchors = []
    for i, ins in enumerate(insns):
        if ins.mnemonic != "call" or not ins.operands:
            continue
        op = ins.operands[0]
        if op.type != cs.x86.X86_OP_IMM:
            continue
        rva = op.imm - pe.image_base
        for f in by_func.get(rva, []):
            anchors.append({
                "anchor": ins.address + ins.size - pe.image_base,
                "formatter": rva,
                "msgid": f["msgid"],
                "printer": _function_start(insns, i) - pe.image_base,
            })
    anchors.sort(key=lambda a: a["anchor"])
    return {"io": io, "anchors": anchors}


def prove_phases(pe, der, args):
    """Run a real link and observe (a) the ids the linker asks for and (b) the anchors."""
    gdb = find_gdb(args.gdb)
    if not gdb:
        print("no gdb found; pass --gdb", file=sys.stderr)
        return 2
    if not args.args:
        print("--prove needs --args '<the link argument list>'", file=sys.stderr)
        return 2
    work = Path(args.out).resolve()
    work.mkdir(parents=True, exist_ok=True)
    script = work / "mwlink-phases.gdb"
    ld = der["io"]["loader"]
    anchors = "\n".join("  (0x%x, '%x')," % (pe.image_base + a["anchor"], a["anchor"])
                        for a in der["anchors"])
    # The stack layout at the observation anchor was *measured* on a real link
    # (`[esp]`=hInstance, `[esp+4]`=the loader's return address, `[esp+8]`=arg1
    # the buffer, `[esp+0xC]`=arg2, `[esp+0x10]`=arg3, the uID) - so the uID is
    # read at +0x10 and the buffer from EBX, which the loader also holds.
    script.write_text(
        "set pagination off\nset confirm off\nset width 0\n"
        f"file {pe.path.as_posix()}\n"
        "set args " + args.args + "\n"
        "python\nimport gdb\n"
        f"LOADER = 0x{pe.image_base + ld['after_rva']:x}\n"
        "class Msg(gdb.Breakpoint):\n"
        "    def stop(self):\n"
        "        try:\n"
        "            esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff\n"
        "            uID = int(gdb.parse_and_eval('*(unsigned int*)%d' % (esp + 0x10)))\n"
        "            ebx = int(gdb.parse_and_eval('$ebx')) & 0xffffffff\n"
        "            raw = gdb.selected_inferior().read_memory(ebx, 96).tobytes().split(b'\\0')[0]\n"
        "            text = raw.decode('latin-1').replace('\\n', ' ').replace('\\t', ' ')\n"
        "            print('MWLINK-MSG %d %s' % (uID, text))\n"
        "        except Exception as exc:\n"
        "            print('MWLINK-MSGERR %s' % exc)\n"
        "        return False\n"
        "class Phase(gdb.Breakpoint):\n"
        "    def __init__(self, addr, tag):\n"
        "        super().__init__('*0x%x' % addr, internal=True)\n"
        "        self.tag = tag\n"
        "    def stop(self):\n"
        "        print('MWLINK-ANCHOR %s' % self.tag)\n"
        "        return False\n"
        "Msg('*0x%x' % LOADER)\n"
        "_n = 0\n"
        "for _a, _t in [\n" + anchors + "\n]:\n"
        "    Phase(_a, _t)\n"
        "    _n += 1\n"
        "print('MWLINK-BPS %d %d' % (_n, len(gdb.breakpoints())))\n"
        "end\nrun\nprintf \"MWLINK-DONE\\n\"\nquit\n")
    argv = [gdb, "-batch", "-nx", "-x", str(script)]
    print(f"# gdb {script}")
    proc = lib_proc.run(argv)
    stream, counts = [], {}
    last = [None]
    for line in proc.stdout.splitlines():
        body = line.strip()
        m = re.match(r"^MWLINK-ANCHOR ([0-9a-f]+)$", body)
        if m:
            counts[m.group(1)] = counts.get(m.group(1), 0) + 1
            last[0] = m.group(1)
            continue
        m = re.match(r"^MWLINK-MSG (\d+) (.*)$", body)
        if m:
            stream.append((last[0], int(m.group(1)), m.group(2)))
            last[0] = None
            continue
        m = re.match(r"^MWLINK-BPS (\d+) (\d+)$", body)
        if m:
            print(f"# breakpoints: {m.group(1)} phase anchor(s), {m.group(2)} total")
    if not stream and not counts:
        print("# the run produced no observation; gdb stderr follows", file=sys.stderr)
        print(proc.stderr.strip()[:2000], file=sys.stderr)
        return 1
    cat = message_catalogue(pe)
    print(f"the link's phase stream, as the linker's own message loader printed it:")
    for anchor, mid, text in stream:
        who = f"anchor {anchor}" if anchor else "(no anchor seen)"
        known = cat.get(mid)
        tag = ("  [catalogue agrees]" if known and known.split("%")[0][:24] == text.split("%")[0][:24]
               else (f"  [catalogue says {known!r}]" if known else "  [id not in the catalogue]"))
        print(f"  {who:<14} id={mid:<4} {text}{tag}")
    agree = sum(1 for _a, mid, text in stream
                if cat.get(mid) and cat[mid].split("%")[0][:24] == text.split("%")[0][:24])
    print(f"# catalogue cross-check: {agree} of {len(stream)} observed messages match "
          f"the catalogue's id -> text")
    fired = sum(1 for a in der["anchors"]
                if f"{a['anchor']:x}" in counts)
    print(f"phase anchors: {fired}/{len(der['anchors'])} fired in this link "
          f"(the other side of the count is the run's own verbosity)")
    seen_ids = {}
    for anchor, mid, _t in stream:
        if anchor:
            seen_ids.setdefault(anchor, []).append(mid)
    for a in der["anchors"]:
        key = f"{a['anchor']:x}"
        n = counts.get(key, 0)
        ids = sorted(set(seen_ids.get(key, [])))
        print(f"  {'FIRED   ' if n else 'UNPROVEN'} {key:>8} candidate-id {a['msgid']} "
              f"x{n}" + (f"  observed ids {ids}" if ids else ""))
    if args.require_all:
        return 0 if fired == len(der["anchors"]) else 1
    return 0 if fired else 1


def _disas_text(pe):
    """``(insns, md)`` for the linker's ``.text``, or ``(None, None)``."""
    cs = _capstone()
    if cs is None:
        return None, None
    if pe.section(".text") is None:
        return None, None
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    return _text_insns(pe), (md, cs)


@functools.lru_cache(maxsize=4)
def _text_insns(pe):
    """The linker's whole ``.text``, disassembled once per image (every derivation walks the same list, and the
    capstone pass over the 1.6 MB section is what each of them spent its time on)."""
    cs = _capstone()
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
    md.detail = True
    text = pe.section(".text")
    return list(md.disasm(pe.read_rva(text.va, text.vsize), pe.image_base + text.va))

