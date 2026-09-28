#!/usr/bin/env python3
"""Classify a PCode dump, then either verify it against the object or measure its delta.

The debugger writes one dump per optimizer pass (`backend-NN-<pass>.txt`), so the
question "does this dump reproduce the object?" only has an answer for the *last*
one - the state just before assembly.  An earlier dump is not claiming to be the
final code; what it is worth is the difference between it and the final code,
which is exactly what names the pass responsible for a residual.

So this tool classifies the dump first:

* **the final dump** (its pass name is the last dump point of the build's codegen
  driver, see `final_pass_name`) is compared against `powerpc-eabi-objdump -d` of
  the object the same command line produces, and reports MATCH or FAIL with the
  first divergence;
* **any earlier dump** reports a PASS-DELTA: the instruction-count delta and the
  concrete instruction changes from that pass forward, with the pass that first
  reaches the object's stream named when the sibling dumps are still on disk;
* a dump whose pass cannot be identified from its file name reports its delta too
  (never a bare FAIL: it does not claim to be final), and `--final` forces the
  strict comparison when the caller knows better.

    python locate/verify_pcode.py <backend-NN-....txt> <file.o> [--json]

Mnemonics are compared after folding the spelling differences between MWCC's
PCode and the disassembler (`clrlwi` is an `rlwinm`, `cmplwi` is a `cmpli`,
`bgt` is a `bt` on a condition bit, ...), and registers are compared as sets
per instruction, because the dump prints a memory operand as `rX,rY,disp`
where the disassembler prints `disp(rY)`.  The delta uses the same comparison,
so a pure operand reordering is not reported as a change.

Exit status: 0 for MATCH and for PASS-DELTA, 1 for a FAIL, 2 for an error
(a missing file, no objdump, an unknown build).  `--strict` makes a PASS-DELTA
exit 1 as well, for a caller that requires the final code.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

OBJDUMP = (
    Path(__file__).resolve().parents[3] / "build/binutils/powerpc-eabi-objdump.exe"
)
VERSIONS_DIR = Path(__file__).resolve().parents[1]

# MWCC PCode mnemonic -> the disassembler's spelling for the same encoding.
ALIASES = {
    "clrlwi": "rlwinm",
    "clrrwi": "rlwinm",
    "extlwi": "rlwinm",
    "extrwi": "rlwinm",
    "inslwi": "rlwimi",
    "insrwi": "rlwimi",
    "rotlwi": "rlwinm",
    "rotrwi": "rlwinm",
    "slwi": "rlwinm",
    "srwi": "rlwinm",
    "cmplwi": "cmpli",
    "cmpwi": "cmpi",
    "subi": "addi",
    "subic": "addic",
    "subis": "addis",
}

# bt/bf on cr0 map onto the disassembler's named conditions.
CR0_BRANCHES = {
    ("bt", 0): "blt",
    ("bt", 1): "bgt",
    ("bt", 2): "beq",
    ("bt", 3): "bso",
    ("bf", 0): "bge",
    ("bf", 1): "ble",
    ("bf", 2): "bne",
    ("bf", 3): "bns",
}

# Mnemonics whose immediate *fields* are laid out differently by each side
# (the disassembler prints `clrlwi r0,r4,16` for what the PCode record holds as
# `rlwinm r0,r4,0,16,31`), or whose operand is a raw address in the object and
# a label id in the dump.  Immediates are not compared for these; mnemonics and
# registers always are.
NO_IMM_COMPARE = {
    "rlwinm",
    "rlwimi",
    "cmpi",
    "cmpli",
    "b",
    "bl",
    "bc",
    "bdnz",
    "blt",
    "bgt",
    "beq",
    "bso",
    "bge",
    "ble",
    "bne",
    "bns",
}

DUMP_LINE = re.compile(r"^\s*\d+\s+([a-z0-9_.]+)\s*(.*)$")
OBJDUMP_LINE = re.compile(r"^\s*[0-9a-f]+:\s+([a-z0-9_.]+)\s*(.*)$")
DUMP_NAME = re.compile(r"^backend-(\d+)-(.+)\.txt$")
REG = re.compile(r"\b([rf])(\d+)\b")
IMM = re.compile(r"(?<![\w])(-?0x[0-9a-f]+|-?\d+)\b")

# How many hunks of a delta to print, and how many instructions per hunk.
MAX_SHOWN = 20
MAX_PER_HUNK = 4
# Aligning two streams costs O(n*m); past this, report the counts only.
MAX_ALIGN = 4000


# ---------------------------------------------------------------------------
# Reading the two sides
# ---------------------------------------------------------------------------


def read_dump(path):
    insns = []
    for line in Path(path).read_text(encoding="utf-8", errors="replace").splitlines():
        m = DUMP_LINE.match(line)
        if m:
            insns.append((m.group(1), m.group(2)))
    return insns


def read_objdump(obj_path, objdump=OBJDUMP):
    out = subprocess.run(
        [str(objdump), "-d", "--no-show-raw-insn", str(obj_path)],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    insns = []
    for line in out.splitlines():
        m = OBJDUMP_LINE.match(line)
        if m:
            insns.append((m.group(1), m.group(2)))
    return insns


def normalise(insns):
    out = []
    for mnemonic, operands in insns:
        text = operands.replace(" ", "")
        text = re.sub(r"<[^>]*>", "", text)  # symbolic branch targets
        if mnemonic in ("bt", "bf"):
            cr, bit = 0, None
            for part in text.split(","):
                cm = re.match(r"cr(\d+)", part)
                if cm:
                    cr = int(cm.group(1))
                if re.fullmatch(r"-?\d+", part):
                    bit = int(part)
            named = CR0_BRANCHES.get((mnemonic, (bit or 0) + 4 * cr))
            if named:
                mnemonic = named
        mnemonic = ALIASES.get(mnemonic, mnemonic)
        regs = {f"{a}{b}" for a, b in REG.findall(text)}
        imms = []
        for value in IMM.findall(text):
            try:
                imms.append(int(value, 0))
            except ValueError:
                pass
        out.append((mnemonic, regs, imms))
    return out


def render(insn) -> str:
    """One instruction as `<mnemonic> <operands>`, each side in its own notation."""
    mnemonic, operands = insn
    return f"{mnemonic} {operands}".strip()


def same(a, b) -> bool:
    """Whether the comparison calls two instructions the same one.

    This *is* the comparator: `positional_problems` reports a difference exactly
    when this is false, and the alignment below aligns exactly the instructions
    this calls different - so a delta never shows a change the MATCH check would
    not report (an `rlwinm`/`clrlwi` or a `bt`/`bgt` spelling is not a change).
    """
    if a[0] != b[0] or a[1] != b[1]:
        return False
    if a[0] in NO_IMM_COMPARE or not a[2] or not b[2] or a[2] == b[2]:
        return True
    return all(x in b[2] or x == 0 for x in a[2])


def positional_problems(dump_insns, obj_insns):
    """The strict, positional comparison - unchanged since the port landed."""
    problems = []
    if len(dump_insns) != len(obj_insns):
        problems.append(
            f"instruction count: dump {len(dump_insns)} vs object {len(obj_insns)}"
        )
    for i, (a, b) in enumerate(zip(dump_insns, obj_insns)):
        if same(a, b):
            continue
        if a[0] != b[0]:
            problems.append(f"#{i}: mnemonic {a[0]} (dump) vs {b[0]} (object)")
        elif a[1] != b[1]:
            problems.append(
                f"#{i} {a[0]}: registers {sorted(a[1])} vs {sorted(b[1])}"
            )
        else:
            problems.append(f"#{i} {a[0]}: immediates {a[2]} vs {b[2]}")
    return problems


def align(a_insns, b_insns):
    """Hunks of instructions that differ between two normalised streams.

    Returns `[(dump indices, object indices)]`, in stream order, aligned by the
    longest common subsequence under `same`.  This is what turns "86 vs 77, 76
    differences" into "these three instructions became that one", which is the
    whole point of reading an early dump.
    """
    n, m = len(a_insns), len(b_insns)
    if n > MAX_ALIGN or m > MAX_ALIGN:
        return None
    # dp[i][j] = length of the LCS of a[i:] and b[j:]
    dp = [[0] * (m + 1) for _ in range(n + 1)]
    for i in range(n - 1, -1, -1):
        row, nxt, ai = dp[i], dp[i + 1], a_insns[i]
        for j in range(m - 1, -1, -1):
            if same(ai, b_insns[j]):
                row[j] = nxt[j + 1] + 1
            else:
                row[j] = nxt[j] if nxt[j] >= row[j + 1] else row[j + 1]
    hunks, cur = [], None
    i = j = 0
    while i < n or j < m:
        if i < n and j < m and same(a_insns[i], b_insns[j]):
            if cur:
                hunks.append(cur)
                cur = None
            i += 1
            j += 1
            continue
        if cur is None:
            cur = (i, i, j, j)
        if i < n and (j >= m or dp[i + 1][j] >= dp[i][j + 1]):
            i += 1
            cur = (cur[0], i, cur[2], cur[3])
        else:
            j += 1
            cur = (cur[0], cur[1], cur[2], j)
    if cur:
        hunks.append(cur)
    return hunks


def hunk_text(dump_insns, obj_insns, hunk) -> dict:
    """One hunk as a report record: the dump instructions and the object ones."""
    i1, i2, j1, j2 = hunk
    return {
        "at": i1,
        "dump": [render(i) for i in dump_insns[i1:i2]],
        "object": [render(i) for i in obj_insns[j1:j2]],
    }


def render_change(change) -> str:
    def side(lines):
        if not lines:
            return "(nothing)"
        shown = " / ".join(lines[:MAX_PER_HUNK])
        if len(lines) > MAX_PER_HUNK:
            shown += f" / ... (+{len(lines) - MAX_PER_HUNK})"
        return shown

    return (
        f"[{len(change['dump'])}->{len(change['object'])}] "
        f"{side(change['dump'])}  ->  {side(change['object'])}"
    )


# ---------------------------------------------------------------------------
# Classification: which pass is this dump, and is it the final code?
# ---------------------------------------------------------------------------


def version_rows():
    """The build-data module, loaded without a compiler binary to detect."""
    if str(VERSIONS_DIR) not in sys.path:
        sys.path.insert(0, str(VERSIONS_DIR))
    import versions

    return versions


def final_pass_name(row):
    """The pass a *completed* run ends on, derived from the breakpoint table.

    The dumps are written in the order the breakpoints fire, and the run stops at
    `codegen_end_addr` (mwcc_debugger.py quits there), so the last dump is the
    greatest breakpoint RVA inside the codegen driver's own range.  For Wii/1.3
    that is 0x6ce60 `after-code-labels` - which is also what a real -O3 run of
    `src/fn_8004C9A0.cpp` writes last (35 dumps, `backend-34-after-code-labels.txt`).
    """
    lo = row.get("codegen_start_addr", 0)
    hi = row.get("codegen_end_addr", 0)
    inside = {
        rva: name
        for rva, name in row.get("pcode_breakpoints", {}).items()
        if lo <= rva < hi
    }
    if not inside:
        return None
    return inside[max(inside)]


def sibling_dumps(path: Path):
    """Every `backend-NN-<pass>.txt` next to this dump, in dump order."""
    out = []
    if path.parent.is_dir():
        for other in sorted(path.parent.iterdir()):
            m = DUMP_NAME.match(other.name)
            if m:
                out.append((int(m.group(1)), m.group(2), other))
    out.sort()
    return out


def analyse(dump_path, obj_path, version=None, force_final=False, objdump=OBJDUMP):
    """Classify the dump and compare it - the one function the CLI and the tests share."""
    dump_path = Path(dump_path)
    obj_path = Path(obj_path)
    m = DUMP_NAME.match(dump_path.name)
    pass_index, pass_name = (int(m.group(1)), m.group(2)) if m else (None, None)
    siblings = sibling_dumps(dump_path)

    dump_insns = read_dump(dump_path)
    obj_insns = read_objdump(obj_path, objdump)
    dump_keys = normalise(dump_insns)
    obj_keys = normalise(obj_insns)

    problems = positional_problems(dump_keys, obj_keys)
    hunks = align(dump_keys, obj_keys)
    changes = [hunk_text(dump_insns, obj_insns, h) for h in (hunks or [])]

    # Classification.  Equality first: a stream that *is* the object's is the
    # final code whatever its file name says (later passes can change nothing).
    versions = version_rows()
    known = versions.known()
    preferred = version or ("Wii/1.3" if "Wii/1.3" in known else known[0])
    final_pass = final_pass_name(versions.row(preferred))
    final_version = None
    for name in ([version] if version else known):
        if final_pass_name(versions.row(name)) == pass_name:
            final_version = name
            if not version:
                final_pass = final_pass_name(versions.row(name))
            break

    is_final = bool(force_final or (pass_name is not None and final_version))
    last_index = max((i for i, _n, _p in siblings), default=None)
    terminal_present = any(n == final_pass for _i, n, _p in siblings)

    if not problems:
        status = "match"
    elif is_final:
        status = "fail"
    else:
        status = "pass-delta"

    note = None
    if status == "match" and pass_name and pass_name != final_pass:
        note = (
            f"this is not the final pass ('{final_pass}'), but its instruction "
            "stream already is the object's - the remaining passes change nothing here"
        )
    elif status == "pass-delta":
        if pass_name is None:
            note = (
                "the file name is not backend-NN-<pass>.txt, so this dump's pass "
                "is unidentified and it does not claim to be final; pass --final "
                "to compare it strictly"
            )
        elif not siblings:
            note = (
                f"no sibling dumps in {dump_path.parent}: classified from the pass "
                "name alone"
            )
        elif pass_index == last_index and not terminal_present:
            note = (
                f"the run looks incomplete: the last dump written is "
                f"'{pass_name}' (dump {pass_index}), a completed run ends at "
                f"'{final_pass}'"
            )

    # Attribution: with the sibling dumps still on disk, name the pass that first
    # produced the object's stream and the change *that* pass made.
    attribution = None
    if status == "pass-delta" and siblings:
        for i, (idx, name, sib) in enumerate(siblings):
            if pass_index is not None and idx < pass_index:
                continue
            sib_insns = read_dump(sib)
            reached = normalise(sib_insns)
            if positional_problems(reached, obj_keys):
                continue
            attribution = {"pass": name, "index": idx, "dump": str(sib)}
            if i:
                prev_idx, prev_name, prev_path = siblings[i - 1]
                prev_insns = read_dump(prev_path)
                prev_hunks = align(normalise(prev_insns), reached) or []
                attribution["from"] = {"pass": prev_name, "index": prev_idx}
                attribution["changes"] = [
                    hunk_text(prev_insns, sib_insns, h) for h in prev_hunks
                ]
            break

    return {
        "status": status,
        "dump": str(dump_path),
        "object": str(obj_path),
        "pass": pass_name,
        "pass_index": pass_index,
        "final_pass": final_pass,
        "version": final_version,
        "is_final": is_final,
        "dump_instructions": len(dump_keys),
        "object_instructions": len(obj_keys),
        "delta": len(obj_keys) - len(dump_keys),
        "changes": changes,
        "aligned": hunks is not None,
        "differences": problems,
        "first_divergence": next((p for p in problems if p.startswith("#")), None),
        "attribution": attribution,
        "run": {
            "dir": str(dump_path.parent),
            "count": len(siblings),
            "last_index": last_index,
            "terminal_dump_present": terminal_present,
        },
        "note": note,
    }


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------


def pass_line(rep) -> str:
    if rep["pass"] is None:
        return "pass:    (unidentified: the file name is not backend-NN-<pass>.txt)"
    if rep["is_final"]:
        build = f" of {rep['version']}" if rep["version"] else ""
        return (
            f"pass:    {rep['pass']}  (the final pass{build}: the last dump point "
            "before codegen ends)"
        )
    return (
        f"pass:    {rep['pass']}  (dump {rep['pass_index']} of {rep['run']['count']} "
        f"in {rep['run']['dir']})"
    )


def render_report(rep, show_all=False) -> str:
    out = [
        f"dump:    {rep['dump']}",
        pass_line(rep),
        f"object:  {rep['object']}",
    ]
    counts = (
        f"instructions: dump {rep['dump_instructions']}, "
        f"object {rep['object_instructions']}"
    )
    if rep["delta"]:
        counts += f" ({rep['delta']:+d})"
    out.append(counts)

    if rep["status"] == "match":
        out.append("MATCH: every mnemonic, register and compared immediate agrees")
        if rep["note"]:
            out.append(f"note:    {rep['note']}")
        return "\n".join(out)

    if rep["status"] == "fail":
        if rep["pass"]:
            out.append(
                f"FAIL: this is the final code ('{rep['pass']}') and it does not "
                "reproduce the object"
            )
        else:
            out.append(
                "FAIL: this dump is the final code (--final) and it does not "
                "reproduce the object"
            )
        if rep["first_divergence"]:
            out.append("first divergence: " + rep["first_divergence"])
        out.append(f"{len(rep['differences'])} difference(s):")
        for problem in rep["differences"][:40]:
            out.append("  " + problem)
        if len(rep["differences"]) > 40:
            out.append(f"  ... and {len(rep['differences']) - 40} more")
        # With the counts unequal the positional list is mostly misalignment, so
        # add the aligned view - the same one the delta path prints.
        if rep["delta"] and rep["aligned"] and rep["changes"]:
            out.append(f"aligned difference ({len(rep['changes'])} change(s)):")
            for change in rep["changes"][:MAX_SHOWN]:
                out.append("  " + render_change(change))
        return "\n".join(out)

    # PASS-DELTA
    out.append(
        (
            f"PASS-DELTA: this dump is not the final code ('{rep['final_pass']}' is); "
            "the difference below is"
        )
        if rep["final_pass"]
        else "PASS-DELTA: this dump is not the final code; the difference below is"
    )
    out.append(
        f"            what the passes after '{rep['pass']}' do to reach it"
        if rep["pass"]
        else "            what the remaining passes do to reach it"
    )
    if rep["note"]:
        out.append(f"note:    {rep['note']}")
    attr = rep["attribution"]
    if attr:
        out.append(
            f"  attribution: the object's stream is first reached at dump "
            f"{attr['index']} '{attr['pass']}'"
        )
        if attr.get("changes") is not None:
            out.append(
                f"    that pass changed {len(attr['changes'])} instruction group(s):"
            )
            for change in attr["changes"][:MAX_SHOWN]:
                out.append("      " + render_change(change))
    if not rep["aligned"]:
        out.append(
            f"  the streams are too large to align ({rep['dump_instructions']} vs "
            f"{rep['object_instructions']} instructions); the count delta is above"
        )
        return "\n".join(out)
    shown = rep["changes"] if show_all else rep["changes"][:MAX_SHOWN]
    out.append(
        f"  {len(rep['changes'])} change(s) from here to the object"
        + ("" if show_all or len(shown) == len(rep["changes"]) else " (first "
           f"{len(shown)}; --all prints every change)")
        + ":"
    )
    for change in shown:
        out.append("    " + render_change(change))
    return "\n".join(out)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Classify a PCode dump and compare it against the object.",
    )
    parser.add_argument("dump", help="a backend-NN-<pass>.txt dump")
    parser.add_argument("object", help="the object the same command line emitted")
    parser.add_argument("--json", action="store_true", help="machine-readable output")
    parser.add_argument(
        "--all", action="store_true", help="print every change, not the first 20"
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="exit non-zero for a PASS-DELTA too (require the final code)",
    )
    parser.add_argument(
        "--final",
        action="store_true",
        help="the caller knows this dump is the final code: compare it strictly",
    )
    parser.add_argument(
        "--version",
        default=None,
        help="compiler build the dump came from (default: infer from the pass name)",
    )
    parser.add_argument(
        "--objdump", default=str(OBJDUMP), help="powerpc-eabi-objdump to use"
    )
    args = parser.parse_args(argv)

    try:
        rep = analyse(
            args.dump,
            args.object,
            version=args.version,
            force_final=args.final,
            objdump=Path(args.objdump),
        )
    except FileNotFoundError as exc:
        print(f"verify_pcode: {exc}", file=sys.stderr)
        return 2
    except KeyError as exc:
        print(
            f"verify_pcode: {exc} is not a compiler build this checker knows "
            "(--version takes one of: Wii/1.3, GC/1.1, GC/2.6)",
            file=sys.stderr,
        )
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"verify_pcode: objdump failed: {exc}", file=sys.stderr)
        return 2

    if args.json:
        print(json.dumps(rep, indent=2))
    else:
        print(render_report(rep, show_all=args.all))

    if rep["status"] == "fail":
        return 1
    if rep["status"] == "pass-delta" and args.strict:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
