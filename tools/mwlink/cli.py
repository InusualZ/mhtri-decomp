"""The mwlink command line: one cmd_* per subcommand, the parser and main().
Spec: docs/tools/spec/mwlink.md. CLI: python tools/mwlink_debugger.py <subcommand> (this module has none of its own)."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

from tools.lib import proc as lib_proc
from tools.lib.binary.pe import Pe
from tools.mwlink.align import _splits_starts, derive_alignment
from tools.mwlink.anchors import (LOADSTRING, _capstone, derive_anchors, derive_order, derive_phase_anchors,
                                  prove_anchors, prove_phases)
from tools.mwlink.catalogue import classify_diagnostics, classify_phase, message_catalogue, parse_timeline, phase_kinds
from tools.mwlink.link import ROOT, default_linker, report_link_failure, resolve_object, run_link
from tools.mwlink.mapfile import parse_map, validate_order_against_map, verify_map
from tools.mwlink.records import FILE_RECORD_FIELDS, derive_file_record, prove_records
from tools.mwlink.trace import MwObject, build_trace, is_ctor_dtor_name, render_trace


def cmd_info(args):
    pe = Pe(args.linker)
    print(f"linker:     {pe.path}")
    print(f"machine:    {pe.machine:#x}  image base {pe.image_base:#x}  "
          f"size {pe.size_image:#x}  entry {pe.entry:#x}")
    print("sections:")
    for s in pe.sections:
        print(f"  {s.name:<8} va={s.va:#08x} vs={s.vsize:#07x} "
              f"raw={s.raw_off:#08x} rs={s.raw_size:#07x} flags={s.flags:#x}")
    dbg = pe.debug_entries()
    print(f"debug directory: {len(dbg)} entr{'y' if len(dbg) == 1 else 'ies'}"
          + ("" if dbg else "  <- no CodeView blob; the compiler's symbol lever is absent"))
    for e in dbg:
        print(f"  type={e['type']} size={e['size']:#x} addr={e['addr']:#x}")
    imps = pe.imports()
    print(f"imports: {len(imps)} module(s)")
    for dll, funcs in imps:
        print(f"  {dll}: {len(funcs)} name(s)"
              + (f" [{', '.join(funcs[:4])}{', ...' if len(funcs) > 4 else ''}]"
                 if len(funcs) <= 12 else ""))
    blocks = pe.string_blocks()
    cat = message_catalogue(pe)
    print(f"RT_STRING resource blocks: {len(blocks)}  messages: {len(cat)} "
          "(the linker's own phase names live here)")
    if args.json:
        print(json.dumps({"path": str(pe.path), "image_base": pe.image_base,
                          "debug_entries": dbg, "messages": len(cat),
                          "imports": {d: len(f) for d, f in imps}}, indent=2))
    return 0


def cmd_messages(args):
    pe = Pe(args.linker)
    cat = message_catalogue(pe)
    if args.grep:
        rx = re.compile(args.grep, re.I)
        hit = {k: v for k, v in cat.items() if rx.search(v)}
    else:
        hit = cat
    for msgid in sorted(hit):
        print(f"msgid={msgid:<4} {hit[msgid]}")
    print(f"# {len(hit)} of {len(cat)} messages", file=sys.stderr)
    return 0


def cmd_order(args):
    pe = Pe(args.linker)
    info = derive_order(pe)
    if not info:
        print("no ctor/dtor name pool found", file=sys.stderr)
        return 2
    names = info["names"]
    print(f"linker:  {pe.path}")
    print(f"stride:  {info['stride'] and hex(info['stride'])}  "
          f"cells: {', '.join(hex(c) for c in info['cell_rvas'])}")
    print("the linker's fixed ctor/dtor order (record order in .data):")
    for i, name in enumerate(names):
        print(f"  {i}: {name}")
    if args.map:
        rep = validate_order_against_map(names, Path(args.map))
        print()
        print(f"cross-check against {args.map}:")
        print(f"  {rep['verdict']}")
        for line in rep["lines"]:
            print("  " + line)
        return 0 if rep["ok"] else 1
    return 0


def cmd_anchors(args):
    pe = Pe(args.linker)
    anchors = derive_anchors(pe)
    if anchors is None:
        print("anchors needs capstone (pip install capstone)", file=sys.stderr)
        return 2
    if args.kind:
        anchors = [a for a in anchors if a["kind"] == args.kind]
    names = [a for a in anchors if a["kind"] == "section-name"]
    msgs = [a for a in anchors if a["kind"] == "message"]
    print(f"linker: {pe.path}")
    print(f"derived anchors: {len(anchors)} ({len(names)} section-name, {len(msgs)} message)")
    print()
    print("section-name anchors (the Row 46 dispatch sites):")
    print(f"  {'anchor':<10} {'kind':<13} {'compare':<8} string")
    for a in names:
        cl = a.get("compare_len")
        print(f"  {a['anchor']:#08x}   {a['kind']:<13} "
              f"{(str(cl) + ' bytes') if cl else '-':<8} {a['string']}")
    if args.messages:
        print()
        print("message anchors (first 40):")
        for a in msgs[:40]:
            print(f"  {a['anchor']:#08x}   {a['string'][:60]}")
    if args.json:
        print(json.dumps(anchors, indent=2))
    if args.prove:
        return prove_anchors(pe, names if not args.all_anchors else anchors, args)
    return 0


def cmd_timeline(args):
    pe = Pe(args.linker)
    cat = message_catalogue(pe)
    argv = [str(pe.path)] + args.args.split()
    if "-v" not in argv and "-verbose" not in argv:
        argv.append("-v")
    print("# " + " ".join(argv))
    proc = lib_proc.run(argv)
    text = proc.stdout + proc.stderr
    for kind, line in parse_timeline(text, phase_kinds(cat)):
        if kind in ("Compiling", "Importing", "Lib Import"):
            continue
        c = classify_phase(line, cat)
        tag = f"  [msgid {c[0]}]" if c else ""
        print(f"{line}{tag}")
    return 0 if proc.returncode == 0 else 1


def cmd_verify(args):
    rep = verify_map(args.elf, args.map)
    print(f"map:   {args.map}")
    print(f"elf:   {args.elf}")
    if rep["status"] == "match":
        print(f"MATCH: {len(rep['sections_compared'])} section(s) - the map is this ELF")
        for s in rep["sections_compared"]:
            print(f"  {s['section']:<12} {s['start']:#010x} size {s['size']:#08x} "
                  f"({s['fragments']} fragments)")
    else:
        print("FAIL: the map does not describe this ELF")
        print(f"first divergence: {rep['first_divergence']}")
        for p in rep["problems"][:40]:
            print("  " + p)
    if args.identity:
        a = Path(args.elf).read_bytes()
        b = Path(args.identity).read_bytes()
        same = a == b
        print(f"identity: {args.identity} "
              + ("byte-identical" if same else f"DIFFERS ({len(a)} vs {len(b)} bytes)"))
        if not same:
            return 1
    return 0 if rep["status"] == "match" else 1


def cmd_phases(args):
    pe = Pe(args.linker)
    if _capstone() is None:
        print("phases needs capstone (pip install capstone)", file=sys.stderr)
        return 2
    der = derive_phase_anchors(pe)
    if der is None:
        print(f"{pe.path.name}: no message loader - this build does not import "
              f"LoadStringA (the GC 1.0..2.6 linkers do not), so the phase table "
              f"has nothing to hang on.  'messages' is empty for the same reason.",
              file=sys.stderr)
        return 2
    io = der["io"]
    ld = io["loader"]
    print(f"linker: {pe.path}")
    print(f"{LOADSTRING} import slot: {io['iat_slot']:#x}")
    print(f"message loader:  RVA {ld['entry_rva']:#x}  (calls {LOADSTRING} at "
          f"{ld['call_rva']:#x}; {len(ld['callers'])} caller(s))")
    print(f"  observation anchor: RVA {ld['after_rva']:#x} - the instruction after the "
          f"call; on a real link the uID reads at [esp+0x10] and the buffer "
          f"pointer is still in EBX (measured, not assumed)")
    for other in io["loaders"]:
        if other is ld:
            continue
        print(f"  not the loader: {LOADSTRING} at {other['call_rva']:#x} in the function "
              f"at {other['entry_rva']:#x}: arg2 is the constant {other['msgid_arg2']}, "
              f"so it loads one fixed string ({len(other['callers'])} caller(s))")
    print(f"message formatters: {len(io['formatters'])} (callers of the loader, one per "
          f"engine message; each pushes its id as arg3)")
    for f in sorted(io["formatters"], key=lambda f: (f["msgid"] is None, f["msgid"])):
        entry = ("" if f["entry_is_call_target"] else "  ENTRY UNPROVEN (nothing calls it)")
        print(f"  msgid={f['msgid']!s:<6} formatter RVA {f['func_rva']:#x} "
              f"pushes {f['pushed']}{entry}")
    if io["unproven_entries"]:
        print(f"  {len(io['unproven_entries'])} formatter entry/entries are not a call "
              f"target: " + ", ".join(hex(e) for e in io["unproven_entries"]), file=sys.stderr)
    print(f"phase anchors (return address of call <formatter>): {len(der['anchors'])}")
    for a in der["anchors"][:40]:
        print(f"  {a['anchor']:#08x}  msgid={a['msgid']}  printer RVA {a['printer']:#x}")
    if args.json:
        print(json.dumps(der, indent=2))
    if args.prove:
        return prove_phases(pe, der, args)
    return 0


def cmd_align(args):
    pe = Pe(args.linker)
    der = derive_alignment(pe)
    if der is None:
        print("align needs capstone, and a linker whose map printer emits '*fill*'",
              file=sys.stderr)
        return 2
    print(f"linker: {pe.path}")
    print("the alignment the linker enforces, derived from the code that prints "
          "the map's '*fill*' row:")
    print(f"  the '*fill*' literal is pushed at RVA {der['fill_push']['rva']:#x} "
          f"({der['fill_push']['text']}) in the function at {der['func_rva']:#x}")
    for load in der["align_loads"]:
        print(f"  the alignment it uses: RVA {load['rva']:#x}  {load['text']}")
    print("    -> offset 0x20 of an Elf32_Shdr is sh_addralign: the *input* "
          "section's own alignment")
    for r in der["roundups"]:
        print(f"  the round-up comparison: RVA {r['rva']:#x}  {r['text']} "
              f"(the `not` before it is ~(align-1))")
    if der["kind_test"]:
        print(f"  the row-kind test that guards the '*fill*' row: "
              f"RVA {der['kind_test']['rva']:#x}  {der['kind_test']['text']}")
    print()
    print("WHAT THIS MEANS: there is no refusal.  The linker aligns the")
    print("fragment's address up to the input section's sh_addralign -")
    print("    addr = (addr + align - 1) & ~(align - 1)")
    print("- and, when that moved it, emits a '*fill*' row for the residue.  A")
    print("section that claims a 4 mod 8 start with sh_addralign 8 therefore")
    print("lands 4 bytes late and every later fragment moves with it: that is")
    print("exactly what tools/elf/objalign.py lowers sh_addralign for.")

    spec = args.unit or args.object
    if not spec:
        if args.json:
            print(json.dumps(der, indent=2))
        return 0
    res = resolve_object(spec, args.link_out)
    if res["path"] is None:
        print(f"no object for '{spec}'", file=sys.stderr)
        return 2
    obj = MwObject(res["path"])
    starts = {}
    for path in (args.splits, "config/RMHE08/splits.txt"):
        if path:
            starts = _splits_starts(Path(path))
            if starts:
                break
    unit_starts = starts.get(res["unit"], {}) or starts.get(res["unit"] + ".cpp", {})
    print()
    print(f"object: {res['path']}")
    if not unit_starts:
        print(f"  (no splits.txt entry claims sections for '{res['unit']}'; "
              f"alignment is reported against the object only)")
    bad = 0
    alloc = [s for s in obj.sections
             if s["index"] != 0 and (s["flags"] & 0x2) and s["size"] > 0]
    counts = {}
    for s in alloc:
        counts[s["name"]] = counts.get(s["name"], 0) + 1
    for sec in alloc:
        start = unit_starts.get(sec["name"])
        align = sec["align"]
        if counts[sec["name"]] > 1:
            # A split target object can carry several sections with one name
            # (dol split fragments); the splits claim names a single address, so
            # a per-section comparison is not available - say that, do not guess.
            if sec is alloc[[s["name"] for s in alloc].index(sec["name"])]:
                print(f"  {sec['name']:<14} {counts[sec['name']]} sections with this "
                      f"name in the object - the splits claim gives one address, "
                      f"so no per-section comparison is possible")
            continue
        if start is None:
            print(f"  {sec['name']:<14} align {align:<3} (no claimed start)")
            continue
        slack = (-start) % align if align > 1 else 0
        low = start & -start
        tag = "honoured" if slack == 0 else f"NOT honoured: +{slack:#x} '*fill*'"
        extra = ""
        if slack and align > low:
            extra = (f"  [the address can only honour {low}; "
                     f"tools/elf/objalign.py lowers this to {low}]")
        if slack:
            bad += 1
        print(f"  {sec['name']:<14} align {align:<3} claim {start:#010x} "
              f"({start % align:#x} mod {align})  {tag}{extra}")
    print()
    if bad:
        print(f"# {bad} section(s) cannot be honoured at their claimed address: "
              f"the link will insert '*fill*' and shift what follows")
    else:
        print("# every allocatable section is honoured at its claimed address")
    if args.json:
        print(json.dumps(der, indent=2))
    return 1 if bad else 0


def cmd_records(args):
    pe = Pe(args.linker)
    der = derive_file_record(pe)
    if der is None:
        print("records needs capstone, and a linker whose '.comment' parser "
              "references the literal 'CodeWarrior'", file=sys.stderr)
        return 2
    print(f"linker: {pe.path}")
    print(f"the '.comment' parser: RVA {der['parser_rva']:#x} "
          f"(the function that memcmps the comment against 'CodeWarrior' at "
          f"{der['magic_rva']:#x})")
    print(f"the input-file record: stride {der['stride']:#x}, array at "
          f"[{der['table_va']:#x}], index = the link's input order")
    print()
    print("  offset size field            meaning")
    print("  " + "-" * 96)
    for off, size, name, meaning, evidence in FILE_RECORD_FIELDS:
        print(f"  +0x{off:02x}  {size}    {name:<16} {meaning}")
        print(f"         evidence: {evidence}")
    print()
    print("derived from the parser's own instructions (not transcribed):")
    print(f"  stride:          imul with immediate {der['stride']:#x}")
    print(f"  record array:    the `add reg, dword ptr [{der['table_va']:#x}]` "
          f"in the parser (the base pointer lives at that address)")
    if der["version_load_rva"] is not None:
        print(f"  comment version: load at {der['version_load_rva']:#x} "
              f"(from [comment+0xb]), store at {der['version_store_rva']:#x} "
              f"(to [record+0x1f])")
    for rva, txt in der["kind_stores"]:
        print(f"  comment kind:    {rva:#x}  {txt}")
    for rva, txt in der["flag_clear"]:
        print(f"  flag clear:      {rva:#x}  {txt}")
    for rva, txt in der["flag_set"]:
        print(f"  flag set:        {rva:#x}  {txt}")
    if args.json:
        print(json.dumps(der, indent=2, default=str))
    if args.prove:
        return prove_records(pe, der, args)
    return 0


def cmd_trace(args):
    """Trace one input object - or one UNIT NAME - through one real link.

    The map and the ELF are the ground truth, and they have to *be* the same
    link: if the ELF does not exist the trace still reports what the map says,
    but it says so (`not checked`) instead of pretending.  With ``--link`` the
    tool runs the build's own link command, redirected to a scratch path, so
    the artifact it traces is one it produced itself.  The build writes no map
    of its own, so when no map is available the tool links one (into
    ``build/scratch/``) rather than making the caller assemble a link line.
    """
    resolved = resolve_object(args.object, args.link_out)
    if resolved["path"] is None:
        print(f"no object for '{args.object}': it is not a path, and no link "
              f"input names that unit ({resolved['inputs']} inputs in the link)",
              file=sys.stderr)
        if resolved.get("near"):
            print("closest unit names: " + ", ".join(resolved["near"]), file=sys.stderr)
        return 2
    args.object = str(resolved["path"])
    print(f"# unit {resolved['unit']}: {resolved['path']}")
    print(f"#   resolved as: {resolved['how']}")

    # One obvious entry point: a unit name, and the tool links if it must.
    default_map = Path(args.map) if args.map else ROOT / "build/RMHE08/main.MAP"
    if not args.link and args.map is None and not default_map.exists():
        print(f"# no link map at {default_map} (the build does not write one); "
              f"linking into {args.out}/ instead")
        args.link = True

    if args.link:
        got = run_link(args)
        if got is None:
            print("cannot derive the link from build.ninja; pass --rsp", file=sys.stderr)
            return 2
        rc, argv, out, err, elf_out, map_out = got
        print("# " + " ".join(argv))
        if rc != 0:
            catalogue = message_catalogue(Pe(args.linker))
            report_link_failure(args, argv, out, err, catalogue)
            # The failed link may still have left a map from an earlier run;
            # tracing against it is allowed, but it is never presented as this
            # link's artifact, and a missing one is an honest stop.
            if args.map is None:
                args.map = str(default_map) if default_map.exists() else None
            if args.map is None or not Path(args.map).exists():
                print(f"the link failed (rc={rc}) and left no map; the link line "
                      f"above is the reproduction", file=sys.stderr)
                return 1
            print(f"# WARNING: the link failed; tracing against {args.map}, which "
                  f"is NOT the map of the link that just failed", file=sys.stderr)
        else:
            args.elf = args.elf or str(elf_out)
            args.map = str(map_out)
            print(f"# linked {elf_out} ({elf_out.stat().st_size} bytes)")
    else:
        args.map = args.map or str(default_map)
    if not Path(args.map).exists():
        print(f"no link map at {args.map} - the link that produced it failed before "
              f"writing one; the link line above is the reproduction", file=sys.stderr)
        return 1
    map_text = Path(args.map).read_text(encoding="utf-8", errors="replace")
    if args.elf and not Path(args.elf).exists():
        print(f"# WARNING: no ELF at {args.elf}; addresses and relocations stay unverified",
              file=sys.stderr)
        args.elf = None
    order = derive_order(Pe(args.linker)) if _linker_exists(args) else None
    rep = build_trace(args.object, map_text, args.elf, args.rsp, order)
    rep["map"] = str(args.map)
    rep["unit"] = resolved["unit"]
    rep["resolved"] = resolved["how"]
    if args.json:
        print(json.dumps(rep, indent=2))
        return 0 if rep["verdict"] != "FAIL" else 1
    print(f"# map {args.map}" + (f"   elf {args.elf}" if args.elf else ""))
    for line in render_trace(rep):
        print(line)
    if args.full_ctor:
        print()
        print("the whole merged .ctors/.dtors layout, as the linker laid it out:")
        for name in (".ctors", ".dtors"):
            info = parse_map(map_text).get(name)
            if not info:
                continue
            for block in info["blocks"]:
                for r in block["fragments"]:
                    if r["name"] in (name,) or is_ctor_dtor_name(r["name"]):
                        print(f"  {name:<8} +{r['offset']:#06x} {r['size']:#06x} "
                              f"{r['name']:<12} {r['source']}")
    return 0 if rep["verdict"] != "FAIL" else 1


def cmd_diagnose(args):
    """Run the build's own link with ``-v`` and report what it said, in order.

    This is the erroring-link path without an object to trace: the phase
    stream, then every diagnostic classified against the message catalogue and
    attributed to the phase that printed it.  It exits 0 when the link
    succeeded and 1 when it did not, and it always redirects the output into
    ``build/scratch/`` - the artifact it must never overwrite is ``main.elf``.
    """
    got = run_link(args, out_base="diagnose", verbose=True)
    if got is None:
        print("cannot derive the link from build.ninja; pass --rsp/--args", file=sys.stderr)
        return 2
    rc, argv, out, err, elf_out, map_out = got
    print("# " + " ".join(argv))
    text = (out or "") + "\n" + (err or "")
    catalogue = message_catalogue(Pe(args.linker))
    timeline = parse_timeline(text, phase_kinds(catalogue))
    print(f"link: rc={rc}" + (f"  ({elf_out.stat().st_size} bytes)"
                              if rc == 0 and elf_out.exists() else ""))
    print("phase stream:")
    for kind, line in timeline:
        print(f"  {kind:<12} {line}")
    if not timeline:
        print("  (none - the linker printed no phase line)")
    diags = classify_diagnostics(text, catalogue, timeline)
    print("diagnostics:")
    if not diags:
        print("  (none)")
    for d in diags:
        who = f"phase {d['phase']}" if d["phase"] else "(no phase seen)"
        mid = f"msgid={d['msgid']}" if d["msgid"] is not None else "msgid=? (not in the catalogue)"
        print(f"  [{who}] {mid}: {d['text']}")
    if args.json:
        print(json.dumps({"rc": rc, "phases": [k for k, _ in timeline],
                          "diagnostics": diags}, indent=2))
    return 0 if rc == 0 else 1


def _linker_exists(args):
    return getattr(args, "linker", None) is not None and Path(args.linker).exists()


#: What `--selftest` forwards to (design.md section 7: no tool defines its own selftest; the flag stays while the
#: docs name it): the package's fixture-tier modules, then the smoke-tier checks of the real linker.
TESTS_DIR = "tools/tests/mwlink"
SMOKE_TEST = "tools/tests/smoke/test_mwlink_live.py"


def selftest():
    """Run the test modules `--selftest` forwards to; the worst exit code wins."""
    paths = sorted((ROOT / TESTS_DIR).glob("test_*.py")) + [ROOT / SMOKE_TEST]
    rc = 0
    for path in paths:
        rc = max(rc, lib_proc.run([sys.executable, str(path)], stdout=None, stderr=None).returncode)
    return rc


def build_parser():
    ap = argparse.ArgumentParser(description="Interrogate the Metrowerks linker (``mwldeppc.exe``) about a real link.")
    ap.add_argument("--selftest", action="store_true",
                    help="run fixture checks (no gdb/compiler/linker)")
    sub = ap.add_subparsers(dest="cmd")

    def linker_arg(p):
        p.add_argument("linker", nargs="?", default=None,
                       help="path to mwldeppc.exe (default: the build's)")

    p = sub.add_parser("info", help="PE recon: sections, dirs, the no-blob check")
    linker_arg(p)
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("messages", help="decode the RT_STRING message catalogue")
    linker_arg(p)
    p.add_argument("--grep", default=None)

    p = sub.add_parser("order", help="the fixed ctor/dtor order (Row 46)")
    linker_arg(p)
    p.add_argument("--map", default=None, help="cross-check against this link map")

    p = sub.add_parser("anchors", help="derive {code address: string} anchors")
    linker_arg(p)
    p.add_argument("--kind", choices=("section-name", "message"), default=None)
    p.add_argument("--messages", action="store_true", help="also list message anchors")
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true", help="prove them with a gdb run")
    p.add_argument("--all-anchors", action="store_true",
                   help="with --prove, use every anchor not just section-name")
    p.add_argument("--args", default=None, help="the linker argument list")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--require-all", action="store_true",
                   help="with --prove, fail unless every anchor fired")

    p = sub.add_parser("records",
                       help="the linker's internal input-file record (stride, fields, "
                            "and --prove to read them back)")
    linker_arg(p)
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true",
                   help="run a real link under gdb and read the record array back")
    p.add_argument("--args", default=None, help="an explicit link line instead of build.ninja's")
    p.add_argument("--rsp", default=None, help="the link's response file")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--limit", type=int, default=24,
                   help="with --prove, how many records to dump")

    p = sub.add_parser("align",
                       help="where the linker aligns a fragment, and what it "
                            "compares (the objalign.py question)")
    linker_arg(p)
    p.add_argument("--unit", default=None, help="a unit name to check its claims for")
    p.add_argument("--object", default=None, help="an object path to check")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--splits", default=None,
                   help="the splits.txt that holds the claimed starts")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("timeline", help="run the linker's own verbose diagnostics")
    linker_arg(p)
    p.add_argument("--args", default="", help="the linker argument list")

    p = sub.add_parser("verify", help="health check: does the map describe the ELF")
    p.add_argument("map")
    p.add_argument("elf")
    p.add_argument("--identity", default=None,
                   help="also byte-compare the ELF against this file")

    p = sub.add_parser("trace", help="follow one UNIT NAME (or object) through a real link")
    p.add_argument("object", help="a unit name (Network/NetworkWiiMediator) or an "
                                   "object path (build/RMHE08/src/....o)")
    p.add_argument("--map", default=None,
                   help="the link map of the link to trace (default: "
                        "build/RMHE08/main.MAP; if it is not there the tool "
                        "links one, like --link)")
    p.add_argument("--elf", default=None, help="the ELF that link produced")
    p.add_argument("--rsp", default=None,
                   help="the link's response file (decides whether the object is an input)")
    p.add_argument("--link", action="store_true",
                   help="run the build's own link first, into a scratch path")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="with --link, the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="with --link, the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug",
                   help="scratch dir for --link (links never write build/RMHE08/main.elf)")
    p.add_argument("--full-ctor", action="store_true",
                   help="also print the whole merged .ctors/.dtors layout")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("diagnose",
                       help="run the build's link with -v; report the phase and "
                            "the diagnostic (the erroring-link path)")
    linker_arg(p)
    p.add_argument("--args", default=None,
                   help="an explicit link line instead of build.ninja's "
                        "( -o/-map are redirected into --out )")
    p.add_argument("--rsp", default=None, help="the link's response file")
    p.add_argument("--link-out", default="build/RMHE08/main.elf",
                   help="the build output whose input list to use")
    p.add_argument("--ldscript", default=None, help="the -lcf script")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--json", action="store_true")

    p = sub.add_parser("phases", help="the linker's phase table (msgid -> anchor)")
    linker_arg(p)
    p.add_argument("--json", action="store_true")
    p.add_argument("--prove", action="store_true",
                   help="run a real link and observe the message ids and anchors")
    p.add_argument("--args", default=None, help="the linker argument list")
    p.add_argument("--out", default="build/scratch/mwlink-debug", help="scratch dir")
    p.add_argument("--gdb", default=None)
    p.add_argument("--require-all", action="store_true",
                   help="with --prove, fail unless every anchor fired")
    return ap


def main(argv=None):
    ap = build_parser()
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 2
    if getattr(args, "linker", None) is None:
        args.linker = default_linker()
    if args.linker is None:
        print("no linker found; pass one explicitly", file=sys.stderr)
        return 2
    args.linker = Path(args.linker)
    if not args.linker.exists():
        print(f"linker not found: {args.linker}", file=sys.stderr)
        return 2
    fn = {"info": cmd_info, "messages": cmd_messages, "order": cmd_order,
          "anchors": cmd_anchors, "timeline": cmd_timeline, "verify": cmd_verify,
          "trace": cmd_trace, "phases": cmd_phases,
          "records": cmd_records, "align": cmd_align,
          "diagnose": cmd_diagnose}[args.cmd]
    try:
        return fn(args)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

