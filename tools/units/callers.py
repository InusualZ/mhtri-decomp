#!/usr/bin/env python3
"""Who calls this function / who reads this data: the whole-DOL caller index, keyed on addresses.

    python tools/units/callers.py <address|name> [--json] [--code|--data] [--kind K[,...]]
                                  [--pointers] [--limit N] [--rebuild]
    python tools/units/callers.py --range 0x80799F98 0x80799FDC [--step 4] [--each] [--json]
                                  # the per-address referrer runs over a range (the .sdata2 seam)
    python tools/units/callers.py --stats          # what the index is, and how old the answer is
    python tools/units/callers.py --selftest       # this file + callers_selftest.py (fixtures only)

**The question nothing else answered.** `tools/units/callees.py` answers "what does this unit call"; the
inverse - "who calls this function, who reads this data" - had no tool at all, so it was answered with a
hand-written grep over the dump (2800 `.s` files / 89 MB in this tree, one per split unit), and the quest
recon lane wrote the 30-line version of *this*, calling it the most useful thing it built. This is that
tool, supported, cached and self-tested.

**The trap this tool exists to survive: the dump is stale, and its labels are printed stale.**
`build/RMHE08/asm/` is dtk's disassembly of the symbol map *as of the split* (see
`tools/splits/dump_asm.py`); a `bl` whose callee was renamed afterwards still prints the OLD label, so
grepping the dump for the new name finds nothing. The dump in this tree carries the proof: at 0x803A146C
the canonical `menu/multi_result.s` prints `bl game_mode_sub_state_set1`, while the stale top-level copy
`auto_fn_803A13B4_text.s` still prints `bl fn_803A1680` - one instruction, two labels, and only one of
them is in `symbols.txt`. Therefore:

* **the graph is built on addresses.** A `bl`/`b` target is decoded from the instruction's own
  displacement (`NIA = CIA + EXTS(LI||0b00)`), so it is exact and needs no map at all; a data reference
  (`X@ha`/`@l`/`@sda21`/...) resolves its name through *the dump's own label table* (the `# <section>:0xOFF
  | 0xADDR | size:` headers), which is stale in exactly the same way the reference is, so an old name still
  lands on the right address;
* **names are resolved per run** through the current map, via `symedit`-style streaming access (never
  printing `symbols.txt` - non-negotiable 7), so a caller renamed since the dump is named correctly and a
  query on a *new* name is answered by address;
* **the index is cached** (`build/tmp/callers/graph.json`) and rebuilt only when the dump changes - a
  signature over every `.s` file's path, size and mtime. A `symbols.txt` edit does **not** rebuild it: the
  graph holds no name from the map. In this tree the build is 245 258 references over 54 256 target
  addresses and takes 8-10 s; a cached query answers in ~1.3 s.

**What it reports, and what each column is evidence for.**

* `call` - a `bl`/`bla` whose target address the encoding decodes. This is the "who calls this" answer.
* `branch` - `b`/`ba` to a *symbol* (a tail call, or a jump into another function). The `.L_ADDR` branches
  inside one function are not callers and are not indexed.
* `addr` - the symbol's address is materialised (`lis r3, X@ha` + `addi r3, r3, X@l`). For a function this
  is the "who installs this as a task, who puts it in a table" answer, which a call-only grep misses: the
  quest field task `fn_8028BF1C` has no direct caller at all - it is only ever *taken* and handed to
  `Tsk_Change`.
* `read` / `write` - a load from / store to the symbol's address (the `.sdata` accesses that carry most
  game state). Classified from the mnemonic (`l*` loads, `st*` stores); `li`/`lis` and every other form
  that names the symbol is `addr`.
* `pointer` - a `.4byte X` entry inside a data object (run `--pointers` to list): a function-pointer table,
  a vtable, an `@eti_` exception-table index. The *site* is the containing object's address, not the exact
  word: the dump prints no address on a bare `.4byte`, and guessing one from the object's directive stream
  would be a second parser for a number nobody needs.
* `arg` - what the caller materialises in `r3` before a `bl`, inferred from the instructions immediately
  before the call (through `callees.decode_rw`, the tree's one register read/write decode): `0x0`,
  `&some_label`, `0 (r3 live-in)` when the preceding call's return flows in, `0?` when a branch or a
  truncated window separates them, and `? (opcode)` when it cannot be judged. It is an inference, and it
  says which way it is unsure instead of guessing.

**Limits, stated so no count is over-read.** An indirect call (`bctrl`, or through a table) has no static
target and is invisible *as a call*: it shows up as an `addr`/`pointer` reference to the callee when the
address is materialised, and as nothing when it is loaded from memory. A data reference is coalesced at its
`lis`+`addi` pair into one site. The dump's state is always printed; when it is stale the *texts* of the
instructions are too (never the addresses), and `python tools/splits/dump_asm.py` refreshes it.

**No dump is not "0 callers"** (the defect `tudiscover` was fixed for): a missing dump is its own message
with the remedy, `exit 2`, and `"error": "no asm dump"` under `--json`.

**It is a reader.** No `src/` edits, no renames, no writes outside `build/tmp/callers/`.
"""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import bisect
import collections
import hashlib
import json
import os
import re
import sys
import time
from tools.lib import cache as libcache

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
ROOT = os.path.dirname(TOOLS)
for _path in (TOOLS, HERE, os.path.join(TOOLS, "symbols"), os.path.join(TOOLS, "splits")):
    if _path not in sys.path:
        sys.path.insert(0, _path)

import tudiscover as td  # noqa: E402  (GAME, ASM_DIR, and the dump's own stamp: one definition of stale)
import unitutil as uu  # noqa: E402  (resolve_input: MAIN's build/ by path when the tree has none)
from tools.lib.project import Ownership  # noqa: E402  (the one ownership index; the rule-2 lint's too)
from units import callees as cl  # noqa: E402  (decode_rw: the tree's one register read/write decode)
from units import dossier as dossier_mod  # noqa: E402  (parse_elf: the tree's one relocation scan)

SCHEMA = 1                      # bump on any change to what the graph stores
GAME = td.GAME
DUMP_TOOL = "python tools/splits/dump_asm.py"
CODE_SECTIONS = (".text", ".init")
KINDS = ("call", "branch", "addr", "read", "write", "pointer")
CODE_KINDS = ("call", "branch")                     # a control transfer: a caller
DATA_KINDS = ("addr", "read", "write", "pointer")   # the address is used, not entered
ARG_WINDOW = 12                                     # instructions scanned back for the r3 argument

# --------------------------------------------------------------------------------------------------
# the dump: one `.s` per unit (~2800 files / 92 MB), the address of every line in its own comment
# --------------------------------------------------------------------------------------------------
LOAD_MNEMONICS = frozenset((
    "li lis lwz lwzu lbz lbzu lhz lhzu lha lhau lmw lfs lfsu lfd lfdu lwzx lwzux lbzx lbzux lhzx lhax "
    "lhaux lfsx lfdx lwarx psq_l psq_lx").split())


def _scan_re():
    """One regex for a whole `.s` file: header comments, `.fn`/`.obj` blocks, instructions, `.4byte`.

    Named groups (not positions) keep the dispatch readable; `re.M` makes every alternative's `^` a line
    start. The instruction form is dtk's `/* ADDR OFFSET BYTES */\ttext`.
    """
    return re.compile(
        r"(?m)^#\s*(?P<sec>[.\w]+):(?:0x)?[0-9A-Fa-f]+\s*\|\s*(?P<haddr>0x[0-9A-Fa-f]+)"
        r"\s*\|\s*size:\s*(?P<hsize>0x[0-9A-Fa-f]+)\s*$"
        r"|^\.fn\s+(?P<fn>[^\s,]+)"
        r'|^\.obj\s+"?(?P<obj>[^\s,"]+)"?'
        r"|^/\*\s+(?P<iaddr>[0-9A-Fa-f]{8})\s+(?:[0-9A-Fa-f]+)\s+(?P<ibytes>[0-9A-Fa-f ]+?)"
        r"\s*\*/\s*(?P<itext>.+?)\s*$"
        r"|^[ \t]*\.(?:4byte|long)\s+(?P<ptr>\S+)\s*$")


SCAN_RE = _scan_re()
# A symbol as an operand: a name, an optional `"`-quoted name - never a register, an immediate, a local
# `.L_ADDR` label or a section name.
SYM_RE = re.compile(r'^(?:"([^"]+)"|([A-Za-z_$][\w$.]*))')
# The `@` modifier that turns an operand into a reference to a symbol's address.
MOD_RE = re.compile(r'(?:"([^"]+)"|([A-Za-z_$][\w$.]*))@(ha|h|l|sda21|sda2)\b')


def asm_dir_of(root=ROOT):
    """The dump to READ: the tree's own `build/<game>/asm`, else MAIN's by path when the tree has none (a fresh worktree)."""
    return uu.resolve_input(os.path.join("build", GAME, "asm"), root, td.has_dump)


def cache_of(root=ROOT):
    return os.path.join(root, "build", "tmp", "callers", "graph.json")


def all_asm_files(asm_dir):
    """Every `.s` of the dump (`.stamp.json` is not a unit). Sorted, so a build is deterministic."""
    out = []
    for dirpath, _dirs, names in os.walk(asm_dir):
        for n in names:
            if n.endswith(".s"):
                out.append(os.path.join(dirpath, n))
    out.sort()
    return out


def dump_signature(asm_dir, files):
    return libcache.stat_digest(files, asm_dir)


def is_scaffolding(path, asm_dir):
    """Whether a dump file is a top-level `auto_*` scaffolding copy rather than a unit's own file."""
    return os.path.dirname(os.path.abspath(path)) == os.path.abspath(asm_dir)


def file_rank(path, asm_dir):
    """How much a duplicate copy of one unit's asm is trusted; low wins.

    The dump can hold the same instruction twice: a top-level `auto_*` scaffolding file from an older run
    beside the unit's own file. Their *addresses* agree (which is why the graph survives either way), but
    their labels do not - `auto_fn_803A13B4_text.s` prints `bl fn_803A1680` where `menu/multi_result.s`
    prints `bl game_mode_sub_state_set1` - so the copy that is not the top-level scaffolding wins, and
    among equals the newest mtime does. The same rule `tudiscover.asm_files()` uses, so the two tools
    cannot disagree about which copy is the live one.
    """
    return (1 if is_scaffolding(path, asm_dir) else 0, -os.stat(path).st_mtime_ns)


def branch_target(addr, byte_text):
    """The address a relative `b`/`bl` branches to, or None: `NIA = CIA + EXTS(LI||0b00)`.

    This is why the graph needs no symbol map at all - the operand's label may be stale, the displacement
    is not. None for an absolute (`AA=1`) branch or an encoding that is not a branch, so the caller falls
    back to the dump's own label table.
    """
    try:
        word = int(byte_text.replace(" ", ""), 16)
    except ValueError:
        return None
    if (word >> 26) not in (16, 18) or (word & 2):
        return None
    disp = word & 0x03FFFFFC
    if disp & 0x02000000:
        disp -= 0x04000000
    return addr + disp


def symbol_operand(operands):
    """The symbol an operand names, or None for a register/immediate/local `.L_` label."""
    tok = operands.split(",")[0].strip() if operands else ""
    m = SYM_RE.match(tok)
    if not m:
        return None
    name = m.group(1) or m.group(2)
    if name.startswith(".") or re.fullmatch(r"(?:r|f|cr|vs|vr)\d+", name):
        return None
    return name


def mem_kind(mnemonic):
    """`read` / `write` / `addr` for an instruction that names a symbol's address.

    From the mnemonic's shape: `l*` loads read the address, `st*` stores write it, and everything else
    (`li`, `lis`, `addi`, `or`, ...) only materialises it.
    """
    if mnemonic in LOAD_MNEMONICS:
        return "addr" if mnemonic in ("li", "lis") else "read"
    if mnemonic.startswith("st") or mnemonic in ("psq_st", "psq_stx"):
        return "write"
    return "addr"


def arg_hint(window, truncated):
    """What `r3` holds at a call, from the instructions before it, in `callees.call_shape`'s vocabulary.

    `window` is `[(mnemonic, operands)]` in order with the call excluded; `truncated` says the window lost
    its oldest instruction (the function start), where a missing writer can no longer be called "0".
    """
    for mnemonic, operands in reversed(window):
        toks = [t.strip() for t in operands.split(",")] if operands else []
        _reads, writes, is_branch, is_call, decoded = cl.decode_rw(mnemonic, operands)
        if not decoded:
            return "?"
        if is_call:
            return "0 (r3 live-in)"
        if is_branch:
            return "0?"
        if 3 in writes:
            if mnemonic == "li" and toks and toks[0] == "r3" and len(toks) > 1:
                return toks[1]
            m = MOD_RE.search(operands) if operands else None
            if mnemonic in ("lis", "addi", "addis", "ori", "oris") and toks and toks[0] == "r3" and m:
                return "&" + (m.group(1) or m.group(2))
            return "? (%s)" % mnemonic
    return "0?" if truncated else "0"


def parse_dump_file(path):
    """-> (labels, funcs, refs) for one `.s` file of the dump.

    `labels` is `name -> (address, size, section)` from the `# <section>:0xOFF | 0xADDR | size:` headers;
    `funcs` is `function address -> .fn label`; `refs` are unresolved sites
    `(site, kind, name, encoded_target_or_None, func_addr, text, arg)`.

    The file is read as one string and scanned once, so the order of the `.fn`/`.obj` blocks and of the
    instructions inside them is preserved without a second pass. `pending` is the header comment that
    names the address of the block which follows it - the dump's own label table, and the reason a stale
    name still resolves to the right address.
    """
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    labels, funcs, refs = {}, {}, []
    pending = None                       # (section, address, size) of the last header comment
    cur = None                           # the `.fn`/`.obj` block in force: (label, is_fn, addr, size, sec)
    window, truncated = [], False
    for m in SCAN_RE.finditer(text):
        if m.group("sec"):
            pending = (m.group("sec"), int(m.group("haddr"), 16), int(m.group("hsize"), 16))
            continue
        if m.group("fn") or m.group("obj"):
            is_fn = bool(m.group("fn"))
            label = m.group("fn") or m.group("obj")
            cur = [label, is_fn, pending[1] if pending else None,
                   pending[2] if pending else 0, pending[0] if pending else None]
            pending = None
            window, truncated = [], False
            if cur[2] is not None:
                labels[label] = (cur[2], cur[3], cur[4])
                if is_fn:
                    funcs[cur[2]] = label
            continue
        if m.group("ptr"):
            # A pointer entry: its own address is the containing object's, not the word's.
            tok = m.group("ptr")
            if tok.startswith(("0x", "-")) or tok[0].isdigit() or tok.startswith("."):
                continue
            nm = SYM_RE.match(tok)
            if not nm or (nm.group(1) or nm.group(2)).startswith("."):
                continue
            if cur is not None and cur[2] is not None:
                funcs.setdefault(cur[2], cur[0])
                refs.append((cur[2], "pointer", nm.group(1) or nm.group(2), None, cur[2],
                             ".4byte %s" % tok, ""))
            continue
        # an instruction line: it gives its own address, and it may be a block's first body line
        addr = int(m.group("iaddr"), 16)
        if cur is not None and cur[2] is None:
            cur[2] = addr
            labels[cur[0]] = (addr, cur[3], cur[4])
            if cur[1]:
                funcs[addr] = cur[0]
        func = cur[2] if cur is not None else None
        asm = m.group("itext")
        parts = asm.split(None, 1)
        if not parts:
            continue
        mnemonic, operands = parts[0].rstrip("."), (parts[1].strip() if len(parts) > 1 else "")
        name = symbol_operand(operands)
        if mnemonic in ("bl", "bla", "b", "ba", "bcl", "bcla") and name:
            kind = "call" if mnemonic in ("bl", "bla", "bcl", "bcla") else "branch"
            arg = arg_hint(window, truncated) if kind == "call" else ""
            refs.append((addr, kind, name, branch_target(addr, m.group("ibytes")), func, asm, arg))
        elif "@" in operands:
            for mod in MOD_RE.finditer(operands):
                refs.append((addr, mem_kind(mnemonic), mod.group(1) or mod.group(2), None, func, asm, ""))
        window.append((mnemonic, operands))
        if len(window) > ARG_WINDOW:
            del window[0]
            truncated = True
    return labels, funcs, refs


# --------------------------------------------------------------------------------------------------
# the index: parse every `.s` once, keep addresses, cache it
# --------------------------------------------------------------------------------------------------
def key_of(ref):
    """The identity of a reference site: one instruction per address, one entry per named table slot.

    Keying an *instruction* reference on `(site, kind)` and not on its printed symbol is what makes the
    stale copies harmless: 0x803A146C is one site, whether the copy that printed it called the callee
    `fn_803A1680` or `game_mode_sub_state_set1`. A `.4byte` entry is keyed on its name too, because one
    object legitimately holds several pointers to several symbols.
    """
    site, kind, name = ref[0], ref[1], ref[2]
    return (site, kind, name) if kind == "pointer" else (site, kind)


def build_index(asm_dir, files):
    """-> (index dict, stats dict). Every `.s` is parsed; each site keeps its preferred copy.

    Two independent kinds of duplicate are handled here: the *same instruction* in a stale copy of a unit
    (`key_of` dedupes the site, `file_rank` decides which text is kept) and the *same name* at two
    addresses (kept, counted, and reported by `--stats` rather than silently dropped).
    """
    t0 = time.time()
    labels, funcs, best = {}, {}, {}
    by_target = collections.defaultdict(list)
    conflicts, dropped, mismatch = [], 0, 0
    for path in files:
        try:
            labels_f, funcs_f, refs_f = parse_dump_file(path)
        except OSError:
            continue
        rank = file_rank(path, asm_dir)
        for name, (addr, size, section) in labels_f.items():
            old = labels.get(name)
            if old is None:
                labels[name] = (addr, size, section)
            elif old[0] == addr:
                if section in CODE_SECTIONS and old[2] not in CODE_SECTIONS:
                    labels[name] = (addr, size, section)   # a `.text` block beats its `@eti_` stub
            elif old[2] in CODE_SECTIONS and section not in CODE_SECTIONS:
                pass                                        # the code block's address wins
            elif len(conflicts) < 8:
                conflicts.append((name, "0x%08X" % old[0], "0x%08X" % addr,
                                  os.path.relpath(path, asm_dir)))
        funcs.update(funcs_f)
        for ref in refs_f:
            key = key_of(ref)
            prev = best.get(key)
            if prev is None:
                best[key] = (rank, ref)
            else:
                dropped += 1
                if rank < prev[0]:
                    best[key] = (rank, ref)
    for _key, (_rank, ref) in best.items():
        site, kind, name, encoded, func, asm, arg = ref
        lab = labels.get(name)
        target = encoded if encoded is not None else (lab[0] if lab else None)
        if encoded is not None and lab is not None and lab[0] != encoded:
            mismatch += 1      # the printed label is not the address the displacement points at
        row = [site, kind, func, asm, arg]
        if target is None:
            by_target[name].append(row)      # kept under its name: retried through the map per query
            continue
        by_target[("0x%08X" % target)].append(row)
    index = {
        "schema": SCHEMA,
        "game": GAME,
        "built_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "seconds": round(time.time() - t0, 1),
        "files": len(files),
        "bytes": sum(os.path.getsize(p) for p in files),
        "labels": {n: list(v) for n, v in labels.items()},
        "funcs": {str(a): n for a, n in sorted(funcs.items())},
        "refs": {k: sorted(v, key=lambda r: (r[0], r[1])) for k, v in by_target.items()},
    }
    stats = {
        "refs": sum(len(v) for v in by_target.values()),
        "targets": sum(1 for k in by_target if k.startswith("0x")),
        "labels": len(labels),
        "unresolved": sum(1 for k in by_target if not k.startswith("0x")),
        "duplicates": dropped,
        "label_conflicts": conflicts,
        "label_mismatch": mismatch,
        "biggest": sorted(((len(v), k) for k, v in by_target.items() if k.startswith("0x")),
                          reverse=True)[:3],
    }
    return index, stats


def attach_derived(index):
    """Add the in-memory lookups a query needs (never serialised with the cache).

    `_label_at` is `address -> [names the dump prints there]`, which is what a stale label is read
    through; `_funcs` is the same table with integer addresses.
    """
    by_addr = {}
    for name, row in index["labels"].items():
        by_addr.setdefault(row[0], []).append(name)
    for names in by_addr.values():
        names.sort()
    index["_label_at"] = by_addr
    index["_funcs"] = {int(k): v for k, v in index["funcs"].items()}
    index["refs"] = {k: [tuple(r) for r in v] for k, v in index["refs"].items()}
    return index


def dump_state(asm_dir, files, root=ROOT):
    """-> (state, message, remedy): `missing` is never answered with a count of zero callers.

    The fresh/stale verdict is `tudiscover.asm_stamp_status()`'s, so this tool, `dump_asm.py` and an
    attribution lane all read the dump's age the same way.
    """
    if not files:
        return ("missing", "%s holds no `.s` file" % rel(asm_dir, root), DUMP_TOOL)
    if os.path.abspath(asm_dir) != os.path.abspath(td.ASM_DIR):
        return ("present", "%d file(s) (not the repository's dump: %s)"
                % (len(files), rel(asm_dir, root)), None)
    try:
        state, msg = td.asm_stamp_status()
    except OSError as exc:
        # a fresh worktree has no `orig/**`: the dump is there, its stamp just cannot be checked
        return ("present", "%d file(s); the dump's stamp could not be checked (%s)"
                % (len(files), exc), None)
    remedy = DUMP_TOOL if state in ("stale", "missing", "truncated", "unstamped") else None
    return (state, msg, remedy)


def load_index(root=ROOT, rebuild=False, asm_dir=None, cache=None):
    """-> (index, info) - from the cache when it is the same dump, else built.

    `info` records whether the cache was used and why not, so `--stats` and `--json` can say how old the
    answer is instead of implying it is fresh. A cache with a different schema, or one built from another
    dump signature, is rebuilt rather than trusted.
    """
    asm_dir = asm_dir or asm_dir_of(root)
    cache = cache or cache_of(root)
    files = all_asm_files(asm_dir)
    info = {"asm_dir": asm_dir, "cache": cache, "files": len(files), "cached": False,
            "rebuilt": False, "reason": "no cache", "stats": {}}
    if not files:
        info["reason"] = "no dump"
        return None, info
    signature = dump_signature(asm_dir, files)
    info["signature"] = signature
    if not rebuild and os.path.exists(cache):
        try:
            with open(cache, "r", encoding="utf-8") as fh:
                index = json.load(fh)
        except (ValueError, OSError) as exc:
            info["reason"] = "unreadable cache (%s)" % exc
            index = None
        else:
            if index.get("schema") != SCHEMA:
                info["reason"] = "cache schema %s, this tool writes %s" % (index.get("schema"), SCHEMA)
                index = None
            elif index.get("signature") != signature or index.get("files") != len(files):
                info["reason"] = "the dump changed since the index was built"
                index = None
            else:
                info.update(cached=True, reason="cache hit", index=index,
                            stats=index.get("stats", {}))
                info["cache_bytes"] = os.path.getsize(cache)
                return attach_derived(index), info
    elif rebuild:
        info["reason"] = "forced rebuild"
    index, stats = build_index(asm_dir, files)
    index["signature"] = signature
    index["stats"] = stats
    info.update(rebuilt=True, index=index, stats=stats)
    try:
        os.makedirs(os.path.dirname(cache), exist_ok=True)
        tmp = cache + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            json.dump(index, fh, separators=(",", ":"), sort_keys=True)
        os.replace(tmp, cache)
        info["cache_bytes"] = os.path.getsize(cache)
    except OSError as exc:
        info["cache_error"] = str(exc)
    return attach_derived(index), info


# --------------------------------------------------------------------------------------------------
# the fallback: no asm dump -> the split objects' own relocations
# --------------------------------------------------------------------------------------------------
# `build/<game>/asm` is written only on demand (a `dol split`, ~3 min), so a tree that has built the
# project but not dumped it has no dump and `callers.py` used to exit 2 with "no asm dump". The
# objects it *does* have are the split pieces under `build/<game>/obj`, and their relocations are
# exactly what the question needs: a `bl` is an `R_PPC_REL24` to the callee, a data access is an
# `R_PPC_ADDR16_*`/`@sda21` relocation to the data symbol. This builds the same address-keyed graph
# from them, so `query`/`print_report` are unchanged.
def _has_objects(d):
    return any(n.endswith(".o") for _dp, _dirs, names in os.walk(d) for n in names)


def obj_dir_of(root=ROOT):
    """The split objects to READ: the tree's own `build/<game>/obj`, else MAIN's by path when the tree has none."""
    return uu.resolve_input(os.path.join("build", GAME, "obj"), root, _has_objects)


def all_object_files(root=ROOT):
    """Every split object under `build/<game>/obj` (the target pieces dtk wrote), sorted."""
    base = obj_dir_of(root)
    out = []
    for dirpath, _dirs, names in os.walk(base):
        for n in names:
            if n.endswith(".o"):
                out.append(os.path.join(dirpath, n))
    out.sort()
    return out


def object_signature(obj_dir, files):
    """The fallback cache's key: every object's path, size and mtime."""
    h = hashlib.sha1()
    for path in files:
        st = os.stat(path)
        h.update(("%s\0%d\0%d\n" % (os.path.relpath(path, obj_dir).replace("\\", "/"),
                                      st.st_size, st.st_mtime_ns)).encode("utf-8"))
    return h.hexdigest()


def build_elf_index(obj_dir, files, cmap):
    """The address-keyed graph from the split objects' relocations - the no-dump fallback.

    A split object is relocatable: its sections sit at 0 and every symbol value is an *offset*. Two
    things recover absolute addresses without the dump: one anchor per section (a defined symbol the
    map knows, whose `map_address - object_offset` is that section's base) and `dossier.parse_elf`'s
    relocation scan. A reference's *kind* is coarser than the dump's - there is no instruction text to
    tell a read from a write - so every non-call is `addr`; the caller, the site and the target are
    exact, and the index says `source: elf` so a count is never read as the dump's.
    """
    t0 = time.time()
    labels, funcs = {}, {}
    # key -> {(site, kind, func, text): row}. The same site is dumped by two objects when a registered
    # unit's piece and the retired `auto_*` piece cover the same bytes; the tuple is identical, so the
    # dict keys the site once (the asm path's `key_of` dedupe, without the dump's `file_rank`).
    by_target = collections.defaultdict(dict)
    scanned, dropped = 0, 0
    for path in files:
        try:
            with open(path, "rb") as fh:
                _sections, symbols, relocs = dossier_mod.parse_elf(fh.read())
        except (OSError, ValueError):
            continue
        scanned += 1
        by_sec = collections.defaultdict(list)
        for s in symbols:
            if s["shndx"] and s["name"]:
                by_sec[s["section"]].append(s)
        base = {}
        for sec, lst in by_sec.items():
            for s in sorted(lst, key=lambda x: x["value"]):
                row = cmap.symbols.get(s["name"])
                if row:
                    base[sec] = row[0][1] - s["value"]
                    break
            if sec in base:
                for s in sorted(lst, key=lambda x: x["value"]):
                    labels.setdefault(s["name"], (base[sec] + s["value"], s["size"] or 0, sec))
                    if sec in CODE_SECTIONS and s["type"] == 2:
                        funcs.setdefault(base[sec] + s["value"], s["name"])
        ordered = {sec: sorted(lst, key=lambda x: x["value"]) for sec, lst in by_sec.items()}

        def holder(sec, off):
            best = None
            for s in ordered.get(sec, ()):
                if s["value"] <= off and (s["size"] == 0 or off < s["value"] + s["size"]):
                    best = s
                elif s["value"] > off:
                    break
            return best

        for r in relocs:
            name, site_sec = r.get("symbol"), r.get("target")
            if not name or not site_sec or site_sec not in base or name.startswith("."):
                continue
            h = holder(site_sec, r["offset"])
            site = base[site_sec] + r["offset"]
            func_addr = base[site_sec] + h["value"] if h is not None else None
            if func_addr is not None:
                funcs.setdefault(func_addr, h["name"])
            if site_sec in CODE_SECTIONS:
                kind = "call" if r["type"] in cl.CALL_TYPES else "addr"
                text = ("bl %s" % name) if kind == "call" else "%s %s" % (r["type_name"], name)
            else:
                kind = "pointer"
                text = "%s %s" % (r["type_name"], name)
            row_map = cmap.symbols.get(name)
            key = ("0x%08X" % (row_map[0][1] + (r.get("addend") or 0))) if row_map else name
            slot = (site, kind, func_addr, text)
            if slot in by_target[key]:
                dropped += 1
            by_target[key][slot] = [site, kind, func_addr, text, ""]
    index = {
        "schema": SCHEMA,
        "game": GAME,
        "source": "elf",
        "built_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "seconds": round(time.time() - t0, 1),
        "files": scanned,
        "bytes": sum(os.path.getsize(p) for p in files),
        "labels": {n: list(v) for n, v in labels.items()},
        "funcs": {str(a): n for a, n in sorted(funcs.items())},
        "refs": {k: sorted(v.values(), key=lambda r: (r[0], r[1])) for k, v in by_target.items()},
    }
    stats = {
        "refs": sum(len(v) for v in by_target.values()),
        "targets": sum(1 for k in by_target if k.startswith("0x")),
        "labels": len(labels),
        "unresolved": sum(1 for k in by_target if not k.startswith("0x")),
        "duplicates": dropped,
        "label_conflicts": [],
        "label_mismatch": 0,
        "biggest": sorted(((len(v), k) for k, v in by_target.items() if k.startswith("0x")),
                          reverse=True)[:3],
    }
    return index, stats


def load_elf_index(root=ROOT, rebuild=False, cmap=None, cache=None):
    """`load_index`'s fallback: the same index shape, built from the split objects, cached the same way.

    `info["source"] == "elf"` and `info["reason"]`/`stats` carry exactly what `load_index` does, so
    `--stats` and `--json` report which graph answered without a second code path.
    """
    obj_dir = obj_dir_of(root)
    cache = cache or os.path.join(root, "build", "tmp", "callers", "elf-graph.json")
    files = all_object_files(root)
    info = {"asm_dir": obj_dir, "cache": cache, "files": len(files), "cached": False,
            "rebuilt": False, "reason": "no cache", "stats": {}, "source": "elf"}
    if not files:
        info["reason"] = "no objects"
        return None, info
    cmap = cmap if cmap is not None else load_map(root)
    signature = object_signature(obj_dir, files)
    info["signature"] = signature
    if not rebuild and os.path.exists(cache):
        try:
            with open(cache, "r", encoding="utf-8") as fh:
                index = json.load(fh)
        except (ValueError, OSError) as exc:
            info["reason"] = "unreadable cache (%s)" % exc
            index = None
        else:
            if index.get("schema") != SCHEMA:
                info["reason"] = "cache schema %s, this tool writes %s" % (index.get("schema"),
                                                                           SCHEMA)
                index = None
            elif index.get("signature") != signature or index.get("files") != len(files):
                info["reason"] = "the objects changed since the index was built"
                index = None
            else:
                info.update(cached=True, reason="cache hit", index=index,
                            stats=index.get("stats", {}))
                info["cache_bytes"] = os.path.getsize(cache)
                return attach_derived(index), info
    elif rebuild:
        info["reason"] = "forced rebuild"
    index, stats = build_elf_index(obj_dir, files, cmap)
    index["signature"] = signature
    index["stats"] = stats
    info.update(rebuilt=True, index=index, stats=stats)
    try:
        os.makedirs(os.path.dirname(cache), exist_ok=True)
        tmp = cache + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            json.dump(index, fh, separators=(",", ":"), sort_keys=True)
        os.replace(tmp, cache)
        info["cache_bytes"] = os.path.getsize(cache)
    except OSError as exc:
        info["cache_error"] = str(exc)
    return attach_derived(index), info


# --------------------------------------------------------------------------------------------------
# the current symbol map, through symedit-style access (never printed - non-negotiable 7)
# --------------------------------------------------------------------------------------------------
class Map:
    """`symbols.txt` + `splits.txt` as an address index: current names, owners, function extents.

    Names come through the same `Ownership` index `tools/units/stylelint.py` lints with (parsed once per
    mtime, never printed), so this tool and the rule-2 lint cannot disagree about who owns an address.
    `by_addr` and `funcs` are inverted from it for the query: `address -> current name`.
    """

    def __init__(self, ownership, root=ROOT):
        self.own = ownership
        self.root = root
        self.symbols = ownership.symbols if ownership else {}
        self.by_addr, self.funcs = {}, []
        for name, rows in self.symbols.items():
            for section, addr, type_ in rows:
                self.by_addr.setdefault(addr, (name, section, type_))
                if section in CODE_SECTIONS and type_ == "function":
                    self.funcs.append((addr, name))
        self.funcs.sort()
        self.func_addrs = [a for a, _n in self.funcs]

    def available(self):
        return bool(self.symbols)

    def name_at(self, addr):
        """-> (name, section, type) of the map row *exactly* at `addr`, or None."""
        return self.by_addr.get(addr)

    def function_at(self, addr):
        """-> (address, name) of the function containing a code `addr`, or None.

        Bisect over the map's own function rows, so no size is guessed. It is the fallback for an
        instruction outside any `.fn` block (`.init` scaffolding); everywhere else the container is the
        dump's own block, resolved by `caller_of`.
        """
        if not self.funcs:
            return None
        i = bisect.bisect_right(self.func_addrs, addr) - 1
        return self.funcs[i] if i >= 0 else None

    def source_exists(self, unit):
        if not unit:
            return False
        return os.path.exists(os.path.join(self.root, "src", unit.replace("\\", "/")))

    def owner(self, name):
        """-> (label, state, unit) for a name, in `callees.classify_owner`'s vocabulary."""
        return cl.classify_owner(self.own.resolve(name) if (self.own and name) else None,
                                 self.source_exists)

    def owner_at(self, addr, section=None, name=None):
        """-> (label, state, unit) for a caller, by its name when the map has it, else by its address."""
        if name and name in self.symbols:
            return self.owner(name)
        rows = self.own.ranges.get(section) if (self.own and section) else None
        for start, end, unit in rows or ():
            if start <= addr < end:
                exists = self.source_exists(unit)
                if not exists:
                    return (unit + " (no source yet)", "registered", unit)
                return (unit, "reconstructed", unit)
        hit = self.by_addr.get(addr)
        if hit:
            return self.owner(hit[0])
        return ("unsplit address", "unsplit", None)


def load_map(root=ROOT):
    """The map, or an empty one when the tree has no `symbols.txt` (a scratch fixture)."""
    return Map(Ownership.load(root), root)


def plain_name(name):
    """A mangled name's plain form: `quest_init__FUc` -> `quest_init` (a query convenience)."""
    return name.split("__", 1)[0]


def find_target(query, index, cmap):
    """-> (address or None, candidates, how): an address, or the names a name query matched.

    `how` is `address`, `map`, `dump` (the name is only in the *stale* dump - see the module docstring)
    or the tier of the name match. More than one candidate is returned as a list, deduplicated with the
    current map's names first: this never guesses between two symbols, it prints them.
    """
    m = re.fullmatch(r"(?:0[xX])?([0-9A-Fa-f]{1,8})", query)
    if m and (query[:2].lower() == "0x" or len(query) == 8):
        return int(m.group(1), 16), [], "address"
    labels = index["labels"]
    if query in cmap.symbols:
        return None, [query], "map"
    if query in labels:
        return None, [query], "dump"
    for pool in (cmap.symbols, labels):
        plain = [n for n in pool if plain_name(n) == query]
        if plain:
            return None, plain, "plain name"
    return None, list(dict.fromkeys(n for n in list(cmap.symbols) + list(labels)
                                    if query in n))[:200], "substring"


# --------------------------------------------------------------------------------------------------
# the query
# --------------------------------------------------------------------------------------------------
def coalesce(rows):
    """Merge a `lis X@ha` + `addi X@l` pair into one site; every other site stays as it is.

    The compiler materialises a data address as two instructions, and reporting them as two references
    doubles every count; they are one site - the same arithmetic that makes the quest dossier read "two
    referrers" for a table whose two pointers each take two instructions.
    """
    out = []
    for row in sorted(rows, key=lambda r: r[0]):
        if out and row[0] == out[-1][0] + 4 and "@ha" in out[-1][3] and "@l" in row[3] \
                and out[-1][2] == row[2] and out[-1][1] == row[1]:
            prev = out[-1]
            out[-1] = (prev[0], prev[1], prev[2], prev[3] + " + " + row[3], prev[4])
            continue
        out.append(tuple(row))
    return out


def fmt_addr(addr):
    return "0x%08X" % addr if addr is not None else "-"


def rel(path, root=ROOT):
    """`path` relative to `root` with forward slashes - a stable string for output and for JSON."""
    try:
        return os.path.relpath(path, root).replace("\\", "/")
    except ValueError:
        return path.replace("\\", "/")


def query(query, index, cmap, kinds=None, limit=40, pointers=False):
    """-> the report dict for one query: the target, its references, and why each is named as it is."""
    addr, candidates, how = find_target(query, index, cmap)
    rep = {"query": query, "resolved": None, "candidates": candidates, "how": how,
           "references": [], "counts": {k: 0 for k in KINDS}, "notes": [], "limit": limit,
           "pointer_hidden": 0}
    if addr is None:
        if len(candidates) != 1:
            rep["error"] = ("%d name(s) match %r" % (len(candidates), query)) if candidates else \
                ("%r is neither in the symbol map nor in the asm dump" % query)
            return rep
        name = candidates[0]
        hit = cmap.symbols.get(name)
        if hit:
            if len(hit) != 1:
                rep["error"] = "%r has %d rows in the symbol map (a duplicate): pass an address" \
                    % (name, len(hit))
                return rep
            addr, how = hit[0][1], "map"
        else:
            addr, how = index["labels"][name][0], "dump"
    rep["how"] = how

    names = index["_label_at"].get(addr, [])
    map_hit = cmap.name_at(addr)
    name = map_hit[0] if map_hit else (names[0] if names else fmt_addr(addr))
    lab = index["labels"].get(name) or (index["labels"].get(names[0]) if names else None)
    section = map_hit[1] if map_hit else (lab[2] if lab else None)
    asm_label = next((n for n in names if n != name), None) if map_hit else None
    if map_hit:
        label, state, unit = cmap.owner(name)
    else:
        label, state, unit = cmap.owner_at(addr, section)
    rep["resolved"] = {
        "address": fmt_addr(addr), "name": name, "section": section, "kind":
        "code" if section in CODE_SECTIONS else ("data" if section else "unknown"),
        "size": lab[1] if lab else 0, "owner": label, "owner_state": state, "unit": unit,
        "asm_label": asm_label, "in_map": bool(map_hit), "name_source": "map" if map_hit else "dump",
    }
    if how == "dump" and map_hit and map_hit[0] != query:
        rep["notes"].append("the queried name is the dump's (stale) label: the current map calls %s %s"
                            % (fmt_addr(addr), map_hit[0]))

    rows = [tuple(r) for r in index["refs"].get(fmt_addr(addr), [])]
    # a reference the dump could not resolve by name was left keyed on that name: the current map may
    # name the address now (the rename-since-the-dump case, in both directions), so retry it here
    retried = set()
    for nm in {name, query, asm_label} - {None, ""}:
        for row in index["refs"].get(nm, []):
            if row not in rows:
                rows.append(row)
                retried.add(nm)
    for nm in sorted(retried):
        rep["notes"].append("a reference printed as %r (in neither the dump's label table nor the map) "
                            "was resolved through the current map" % nm)

    if not pointers:
        rep["pointer_hidden"] = sum(1 for r in rows if r[1] == "pointer")
        rows = [r for r in rows if r[1] != "pointer"]
    # the `lis`+`addi` pairs become one site before anything is counted, so every number below (and in
    # the "no site matched the filter" note) is the same kind of site
    rows = coalesce([r for r in rows if r[1] not in ("addr", "read", "write")]) + \
        coalesce([r for r in rows if r[1] in ("addr", "read", "write")])
    rep["all_kinds"] = dict(collections.Counter(r[1] for r in rows))
    if kinds:
        rows = [r for r in rows if r[1] in kinds]
    if not rows and rep["all_kinds"]:
        rep["notes"].append("no %s reference here, but %d other reference(s): drop the filter"
                            % ("/".join(kinds or ()), sum(rep["all_kinds"].values())))
    rows.sort(key=lambda r: r[0])

    refs = []
    for site, kind_, func, text, arg in rows:
        faddr, fname = caller_of(index, cmap, site, func)
        owner, ostate, _unit = cmap.owner_at(faddr, None, fname) if faddr is not None else ("", "", None)
        refs.append({"kind": kind_, "site": fmt_addr(site),
                     "caller": {"address": fmt_addr(faddr), "name": fname, "owner": owner,
                                "owner_state": ostate} if faddr is not None else None,
                     "instruction": text, "arg": arg or None})
    rep["references"] = refs
    counts = collections.Counter(r["kind"] for r in refs)
    for k in KINDS:
        rep["counts"][k] = counts.get(k, 0)
    rep["counts"]["sites"] = len(refs)
    rep["counts"]["functions"] = len({r["caller"]["address"] for r in refs if r["caller"]})
    return rep


def caller_of(index, cmap, site, func):
    """-> (address, name) of who holds a reference, current name first, the dump's label second.

    The container is the `.fn`/`.obj` block the dump put the instruction in, so it needs no size guess.
    The *current* map wins only when it has a symbol exactly at that block's start: where it does not
    (an unsplit band, or a function the map has not named) the previous map row is not this function, so
    the dump's own block label is the only honest answer - and it is exactly the caller name the strip
    would have used anyway, at the same address.
    """
    if func is None:                       # an instruction outside any block (`.init` scaffolding)
        hit = cmap.function_at(site)
        return hit if hit else (None, None)
    hit = cmap.name_at(func)
    if hit:
        return func, hit[0]
    name = index["_funcs"].get(func) or (index["_label_at"].get(func) or [None])[0]
    return func, name or fmt_addr(func)


def _owner_column(rep):
    """-> {site: owner} for the printed table (already resolved in the query, so no second index)."""
    return {r["site"]: (r["caller"] or {}).get("owner", "") for r in rep["references"]}


def print_report(rep, info=None, state=None, msg=None, remedy=None, root=ROOT, asm_dir=None):
    """The human answer. `sorted by site` throughout, one table per kind, the counts never hidden."""
    if rep.get("error"):
        print("== %s: no answer" % rep["query"])
        print()
        print("   %s" % rep["error"])
        if rep["candidates"]:
            print("   candidates:")
            for c in rep["candidates"][:12]:
                print("     %s" % c)
            print("   (%d shown; pass the exact name, or the address)" % min(12, len(rep["candidates"])))
        else:
            print("   try  python tools/symbols/symedit.py find <regex>   (the current map)")
        return 1
    hit = rep["resolved"]
    print("== %s  %s  %s%s  owner %s" % (
        hit["name"], hit["address"], hit["section"] or "?",
        (" size:0x%X" % hit["size"]) if hit["size"] else "", hit["owner"]))
    if hit["name_source"] == "dump":
        print("   (not in the current symbol map - this is the dump's own label)")
    if hit["asm_label"]:
        print("   the dump still prints it as %s (a stale label: the address is what was indexed)"
              % hit["asm_label"])
    if asm_dir is not None:
        print("   dump   %s - %s" % (rel(asm_dir, root), state))
        if msg:
            print("          %s" % msg)
        if remedy:
            print("          remedy: %s" % remedy)
    if info:
        print("   index  %s  %d reference(s) over %d target address(es), %d file(s) - %s" % (
            rel(info["cache"], root), info.get("stats", {}).get("refs", 0),
            info.get("stats", {}).get("targets", 0), info.get("files", 0),
            ("cached" if info.get("cached") else "built in %.1f s" % info["index"]["seconds"])))
    if not rep["references"]:
        print()
        if rep.get("all_kinds"):
            print("   no site matched the filter; this address has %s" % ", ".join(
                "%d %s reference(s)" % (n, k) for k, n in sorted(rep["all_kinds"].items())))
        else:
            print("   no reference to this address in the index.")
        if rep["pointer_hidden"]:
            print("   (%d pointer table entry(ies) - pass --pointers)" % rep["pointer_hidden"])
        for note in rep["notes"]:
            print("   note   %s" % note)
        return 0
    order = [k for k in KINDS if rep["counts"][k]]
    titles = {"call": "call site(s) (bl) - who calls this",
              "branch": "branch site(s) (b to a symbol) - a tail call, or a jump into this",
              "addr": "address-taken site(s) - the address is materialised and passed on",
              "read": "read(s) - the address is loaded from",
              "write": "write(s) - the address is stored to",
              "pointer": "pointer table(s) - a .4byte entry in a data object points here"}
    owners = _owner_column(rep)
    limit = rep.get("limit") or 0
    print()
    for kind in order:
        rows = [r for r in rep["references"] if r["kind"] == kind]
        print("   %d %s" % (len(rows), titles[kind]))
        hdr = "   #  %-10s %-18s %-22s %-28s %s" % ("site", "arg", "in function", "owner", "instruction")
        print(hdr)
        print("   " + "-" * (len(hdr) - 3))
        for i, r in enumerate(rows[:limit] if limit else rows, 1):
            caller = r["caller"] or {}
            print("   %-2d %-10s %-18s %-22s %-28s %s" % (
                i, r["site"], r["arg"] or "-", (caller.get("name") or "?")[:22],
                (owners.get(r["site"]) or (caller.get("name") and "not in the symbol map") or "")[:28],
                r["instruction"]))
        if limit and len(rows) > limit:
            print("   ... (%d more, raise --limit)" % (len(rows) - limit))
        print()
    print("   total  %d site(s) in %d function(s)/object(s)%s" % (
        rep["counts"]["sites"], rep["counts"]["functions"],
        ("  (+%d pointer table entry(ies), see --pointers)" % rep["pointer_hidden"])
        if rep["pointer_hidden"] else ""))
    for note in rep["notes"]:
        print("   note   %s" % note)
    return 0


# --------------------------------------------------------------------------------------------------
# the per-address referrer census, and its runs over a range - the `.sdata2`/`.data` seam evidence
# --------------------------------------------------------------------------------------------------
# A data pool's edges are not chosen, they are measured: two objects' pools are merged by MWLD, so the
# run one unit owns is exactly the maximal span of addresses whose *reader set* is constant, and the
# address where that set changes is the seam. The pool lanes pinned both edges with a separate
# `callers.py <address>` invocation per address (~1-2 s each off the cached graph); this is the same
# census in one load, grouped, so the edges fall out of one command.
def norm_reader(label):
    """A census owner label -> a unit name, or None for the labels that are not units.

    `Pl/pl_act_step.cpp` -> `Pl/pl_act_step`; an `unsplit address` or `unsplit (ef)` label is not a
    unit; a label is normalised so a reader set joins `splits.txt` and `dataclaim.py`'s sharer list.
    """
    if not label:
        return None
    label = label.strip()
    if label in ("unsplit address", "unsplit") or label.startswith("unsplit ("):
        return None
    if label.endswith(" (no source yet)"):
        label = label[: -len(" (no source yet)")]
    if label.endswith((".c", ".cpp", ".cp")):
        label = os.path.splitext(label)[0]
    return label


def reader_units(rep):
    """`{unit: site count}` for one query report - who references an address, in the unit vocabulary."""
    counts = {}
    for ref in rep.get("references", ()):
        unit = norm_reader((ref.get("caller") or {}).get("owner"))
        if unit:
            counts[unit] = counts.get(unit, 0) + 1
    return counts


def readers_of(index, cmap):
    """`readers_of(address) -> {unit: sites}` - the sharing census, built from this index and nothing else.

    A cached wrapper around the one `query` this tool already has, so the `--range` runs and
    `dataclaim.py`'s sharer census cannot disagree about who reads an address.
    """
    cache: dict[int, dict] = {}

    def readers(address):
        if address not in cache:
            cache[address] = reader_units(query("0x%08X" % address, index, cmap, limit=0))
        return cache[address]

    return readers


def range_report(index, cmap, lo, hi, step=4):
    """The per-address referrer runs over `[lo, hi]` (both ends inclusive), stepping `step` bytes.

    Every address's reader set comes from `readers_of`; contiguous addresses whose sets are equal
    collapse into one run, and the first address of every run but the first is a **seam** - the
    boundary a lane otherwise measures one `callers.py` invocation at a time.
    """
    census = readers_of(index, cmap)
    addresses = []
    address = lo
    while address <= hi:
        readers = census(address)
        addresses.append({"address": address, "readers": sorted(readers),
                          "sites": sum(readers.values())})
        address += step
    runs, seams = [], []
    for row in addresses:
        if runs and runs[-1]["readers"] == row["readers"]:
            runs[-1]["end"] = row["address"]
            runs[-1]["count"] += 1
            runs[-1]["sites"] += row["sites"]
        else:
            if runs:
                seams.append(row["address"])
            runs.append({"start": row["address"], "end": row["address"], "count": 1,
                         "readers": row["readers"], "sites": row["sites"]})
    return {"range": {"lo": lo, "hi": hi, "step": step, "addresses": len(addresses)},
            "addresses": addresses, "runs": runs, "seams": seams}


def print_range(payload, root=ROOT, asm_dir=None, state=None, each=False):
    """The human answer: one row per run, its readers, and the seams between them."""
    rng = payload["range"]
    print("== per-address referrer runs  %s..%s  (step 0x%X, %d address(es), %d run(s))" % (
        fmt_addr(rng["lo"]), fmt_addr(rng["hi"]), rng["step"], rng["addresses"],
        len(payload["runs"])))
    if asm_dir is not None:
        print("   dump   %s - %s" % (rel(asm_dir, root), state))
    print()
    header = "   %-21s %9s  %s" % ("run", "addresses", "readers")
    print(header)
    print("   " + "-" * (len(header) - 3))
    for run in payload["runs"]:
        lo, hi = fmt_addr(run["start"]), fmt_addr(run["end"])
        span = lo if run["start"] == run["end"] else "%s-%s" % (lo, hi)
        readers = ", ".join(run["readers"]) or "(no reader in the index)"
        print("   %-21s %9d  %s" % (span, run["count"], readers))
    print()
    if payload["seams"]:
        print("   seams (%d): %s" % (len(payload["seams"]),
                                    ", ".join(fmt_addr(a) for a in payload["seams"])))
    else:
        print("   seams: none - the reader set is constant across the range")
    if each:
        print()
        print("   %-12s %s" % ("address", "readers"))
        for row in payload["addresses"]:
            print("   %-12s %s" % (fmt_addr(row["address"]),
                                    ", ".join(row["readers"]) or "-"))
    return 0


def _no_dump(asm_dir, root, as_json, query=None):
    """The missing-dump answer, printed by both the human and the `--json` path; returns exit 2."""
    rel_asm = rel(asm_dir, root)
    if as_json:
        print(json.dumps({"error": "no asm dump", "query": query or "",
                          "dump": {"state": "missing", "asm_dir": rel_asm,
                                   "message": "%s holds no `.s` file" % rel_asm,
                                   "remedy": DUMP_TOOL}}, indent=2))
        return 2
    print("== no asm dump")
    print("   %s holds no `.s` file, so there is no caller data to read." % rel_asm)
    print("   This is not '0 callers' - the dump is the input. Build it first:")
    print("     %s   (a full `dol split`, ~3 min)" % DUMP_TOOL)
    return 2


def stats_report(index, info, cmap, state, msg, remedy, asm_dir, root=ROOT, as_json=False):
    """`--stats`: what the index holds, how it was obtained, and what it left unresolved."""
    out = {"dump": {"state": state, "message": msg, "remedy": remedy, "files": info["files"],
                    "asm_dir": rel(asm_dir, root)},
           "index": {"path": rel(info["cache"], root), "cached": info["cached"],
                     "reason": info["reason"], "rebuilt": info["rebuilt"],
                     "built": index["built_utc"], "seconds": index["seconds"],
                     "bytes": info.get("cache_bytes"), "signature": info.get("signature"),
                     "stats": info.get("stats", {})},
           "map": {"available": cmap.available(), "symbols": len(cmap.symbols)}}
    if as_json:
        print(json.dumps(out, indent=2, default=str))
        return 0
    print("== the caller index")
    print("   dump   %s - %s" % (rel(asm_dir, root), state))
    if msg:
        print("          %s" % msg)
    if remedy:
        print("          remedy: %s" % remedy)
    print("   index  %s  %d reference(s) over %d target address(es), %d label(s)" % (
        rel(info["cache"], root), out["index"]["stats"].get("refs", 0),
        out["index"]["stats"].get("targets", 0), len(index["labels"])))
    print("          %d file(s), %.1f MB, built in %.1f s at %s - %s" % (
        index["files"], index["bytes"] / 1e6, index["seconds"], index["built_utc"],
        "from the cache" if info["cached"] else "just rebuilt (%s)" % info["reason"]))
    st = out["index"]["stats"]
    print("          %d duplicate site(s) dropped (a stale copy of a unit's asm), %d label "
          "conflict(s), %d printed label(s) disagreeing with the displacement" % (
              st.get("duplicates", 0), len(st.get("label_conflicts", [])), st.get("label_mismatch", 0)))
    if st.get("unresolved"):
        print("          %d name(s) are in neither the dump's label table nor the map (retried per "
              "query through the map)" % st["unresolved"])
    for n, a in st.get("biggest", []):
        print("          most-referenced: %s (%d reference(s))" % (a, n))
    print("   map    %d symbol(s) from %s (%s)" % (
        len(cmap.symbols), os.path.join("config", GAME, "symbols.txt"),
        "available" if cmap.available() else "MISSING - names come from the dump"))
    return 0


def main(argv=None, root=ROOT):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("query", nargs="?", help="an address (0x803AD47C) or a symbol name (quest_init)")
    ap.add_argument("--range", nargs=2, metavar=("LO", "HI"),
                    help="the per-address referrer runs across LO..HI (both inclusive), stepping "
                         "--step - the `.sdata2`/`.data` seam evidence, in one load")
    ap.add_argument("--step", type=lambda s: int(s, 16), default=4,
                    help="the address stride for --range (hex accepted; default 4)")
    ap.add_argument("--each", action="store_true",
                    help="with --range, list every address as well as the runs")
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--code", action="store_true", help="only caller sites (bl / branch)")
    ap.add_argument("--data", action="store_true",
                    help="only address references (read/write/addr/pointer)")
    ap.add_argument("--kind", help="an explicit filter: %s" % ",".join(KINDS))
    ap.add_argument("--pointers", action="store_true", help="list .4byte pointer-table entries too")
    ap.add_argument("--limit", type=int, default=40, help="max sites listed per kind (0 = all)")
    ap.add_argument("--rebuild", action="store_true", help="rebuild the index even if the cache is valid")
    ap.add_argument("--stats", action="store_true", help="report the index's state and exit")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    if args.code and args.data:
        ap.error("--code and --data are exclusive")
    if args.range and (args.stats or args.query):
        ap.error("--range takes no query and no --stats (it is its own report)")
    if not args.query and not args.stats and not args.range:
        ap.error("an address or a symbol name is required (or --range, or --stats, or --selftest)")
    lo = hi = None
    if args.range:
        try:
            lo, hi = (int(args.range[0], 16), int(args.range[1], 16))
        except ValueError:
            ap.error("both --range edges must be hex addresses (0x80799F98)")
        if hi < lo:
            ap.error("--range HI is below LO (0x%X < 0x%X)" % (hi, lo))
        if args.step <= 0:
            ap.error("--step must be positive")
    kinds = None
    if args.code:
        kinds = list(CODE_KINDS)
    if args.data:
        kinds = list(DATA_KINDS)
    if args.kind:
        kinds = [k.strip() for k in args.kind.split(",") if k.strip()]
        bad = [k for k in kinds if k not in KINDS]
        if bad:
            ap.error("unknown kind(s): %s (known: %s)" % (", ".join(bad), ", ".join(KINDS)))
    asm_dir = asm_dir_of(root)
    files = all_asm_files(asm_dir)
    cmap = load_map(root)
    if files:
        index, info = load_index(root=root, rebuild=args.rebuild, asm_dir=asm_dir)
        if index is None:
            return _no_dump(asm_dir, root, args.json, args.query)
        state, msg, remedy = dump_state(asm_dir, files, root)
    else:
        # No dump (it is written only on demand): answer from the split objects' relocations instead
        # of refusing. `--stats` and `--json` carry the same `info` shape, so the reader can see which
        # graph answered (`source: elf`).
        index, info = load_elf_index(root=root, rebuild=args.rebuild, cmap=cmap)
        if index is None:
            return _no_dump(asm_dir, root, args.json, args.query)
        obj_dir = obj_dir_of(root)
        state, msg = "elf fallback", (
            "no asm dump under %s - the graph is the %d split object(s)' relocations (coarser kinds, "
            "exact sites); run %s for the instruction-level dump" % (rel(asm_dir, root), info["files"],
                                                                    DUMP_TOOL))
        remedy, asm_dir = None, obj_dir
    if args.stats:
        return stats_report(index, info, cmap, state, msg, remedy, asm_dir, root, args.json)
    if args.range:
        payload = range_report(index, cmap, lo, hi, args.step)
        if args.json:
            payload["dump"] = {"state": state, "message": msg, "remedy": remedy,
                               "asm_dir": rel(asm_dir, root), "files": info.get("files", len(files)),
                               "source": info.get("source", "asm")}
            payload["index"] = {"path": rel(info["cache"], root), "cached": info["cached"],
                                "reason": info["reason"], "built": index["built_utc"],
                                "seconds": index["seconds"],
                                "refs": info.get("stats", {}).get("refs", 0)}
            print(json.dumps(payload, indent=2))
            return 0
        return print_range(payload, root=root, asm_dir=asm_dir, state=state, each=args.each)
    rep = query(args.query, index, cmap, kinds=kinds, limit=args.limit, pointers=args.pointers)
    if args.json:
        if rep.get("error"):
            print(json.dumps({"error": rep["error"], "query": args.query,
                              "candidates": rep["candidates"],
                              "dump": {"state": state, "message": msg, "remedy": remedy}}, indent=2))
            return 1
        out = dict(rep)
        out["dump"] = {"state": state, "message": msg, "remedy": remedy,
                       "asm_dir": rel(asm_dir, root), "files": info.get("files", len(files)),
                       "source": info.get("source", "asm")}
        out["index"] = {"path": rel(info["cache"], root), "cached": info["cached"],
                        "reason": info["reason"], "built": index["built_utc"],
                        "seconds": index["seconds"], "refs": info.get("stats", {}).get("refs", 0),
                        "targets": info.get("stats", {}).get("targets", 0)}
        print(json.dumps(out, indent=2))
        return 0
    return print_report(rep, info=info, state=state, msg=msg, remedy=remedy, root=root,
                        asm_dir=asm_dir)


# --------------------------------------------------------------------------------------------------
# self-test: fixtures only (a fake asm dump + a fake map in a temp dir), so the run is identical in
# MAIN, in a fresh worktree, and with no dump on disk at all.
# --------------------------------------------------------------------------------------------------
def _fixture_dump():
    """A three-file fake dump: a unit file, a stale top-level copy, and a data file.

    The encodings are real, so `branch_target` decodes them: 0x80001008 + 0x278 == 0x80001280. The
    orientation is the real one - the unit file (`menu/multi_result.s`) prints the *current* label, the
    top-level `auto_*` scaffolding copy prints the stale `fn_<ADDR>` - so `file_rank` has a real job.
    """
    unit = "\n".join([
        '.include "macros.inc"',
        '.file "menu/multi_result.cpp"',
        '',
        '# 0x80001000..0x80001120 | size: 0x120',
        '.text',
        '.balign 4',
        '',
        '# .text:0x0 | 0x80001000 | size: 0x1C',
        '# caller(unsigned char)',
        '.fn caller, global',
        '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
        '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
        '/* 80001008 00000008  48 00 02 79 */\tbl quest_init__FUc',
        '/* 8000100C 0000000C  90 7F 00 DC */\tstw r3, 0xdc(r31)',
        '/* 80001010 00000010  38 60 00 01 */\tli r3, 0x1',
        '/* 80001014 00000014  48 00 02 6D */\tbl quest_init__FUc',
        '/* 80001018 00000018  4E 80 00 20 */\tblr',
        '.endfn caller',
        '',
        '# .text:0x60 | 0x80001060 | size: 0x18',
        '.fn with_data, global',
        '/* 80001060 00000060  3C 60 80 50 */\tlis r3, lbl_80500000@ha',
        '/* 80001064 00000064  38 63 00 00 */\taddi r3, r3, lbl_80500000@l',
        '/* 80001068 00000068  80 A0 00 00 */\tlwz r5, lbl_80500020@sda21(r0)',
        '/* 8000106C 0000006C  90 C0 00 00 */\tstw r6, lbl_80500020@sda21(r0)',
        '/* 80001070 00000070  4E 80 00 20 */\tblr',
        '.endfn with_data',
        '',
        '# .text:0xA0 | 0x800010A0 | size: 0x14',
        '.fn taker, global',
        '/* 800010A0 000000A0  3C 60 80 00 */\tlis r3, quest_init__FUc@ha',
        '/* 800010A4 000000A4  38 63 12 80 */\taddi r3, r3, quest_init__FUc@l',
        '/* 800010A8 000000A8  4B FF FF 59 */\tbl caller',
        '/* 800010AC 000000AC  48 00 00 0D */\tbl fn_800010B8',
        '/* 800010B0 000000B0  4E 80 00 20 */\tblr',
        '.endfn taker',
        '',
        '# .text:0xB8 | 0x800010B8 | size: 0x8',
        '.fn fn_800010B8, global',
        '/* 800010B8 000000B8  4E 80 00 20 */\tblr',
        '.endfn fn_800010B8',
        '',
    ]) + "\n"
    stale = "\n".join([
        '.include "macros.inc"',
        '.file "auto_fn_80001000_text"',
        '',
        '# 0x80001000..0x80001280 | size: 0x280',
        '.text',
        '.balign 4',
        '',
        '# .text:0x0 | 0x80001000 | size: 0x1C',
        '.fn fn_80001000, global',
        '/* 80001000 00000000  94 21 FF F0 */\tstwu r1, -0x10(r1)',
        '/* 80001004 00000004  38 60 00 00 */\tli r3, 0x0',
        '/* 80001008 00000008  48 00 02 79 */\tbl fn_80001280',       # the same site, stale label
        '/* 8000100C 0000000C  90 7F 00 DC */\tstw r3, 0xdc(r31)',
        '/* 80001010 00000010  38 60 00 01 */\tli r3, 0x1',
        '/* 80001014 00000014  48 00 02 6D */\tbl fn_80001280',
        '/* 80001018 00000018  4E 80 00 20 */\tblr',
        '.endfn fn_80001000',
        '',
        '# .text:0x200 | 0x80001200 | size: 0x10',
        '.fn fn_80001200, global',                       # a site only the stale copy dumps
        '/* 80001200 00000200  38 60 00 00 */\tli r3, 0x0',
        '/* 80001204 00000204  48 00 00 7D */\tbl quest_init__FUc',
        '/* 80001208 00000208  4E 80 00 20 */\tblr',
        '.endfn fn_80001200',
        '',
        '# .text:0x280 | 0x80001280 | size: 0x40',                     # the stale label of the target
        '.fn fn_80001280, global',
        '/* 80001280 00000280  4E 80 00 20 */\tblr',
        '.endfn fn_80001280',
        '',
    ]) + "\n"
    data = "\n".join([
        '.include "macros.inc"',
        '.file "auto_07_80500000_data"',
        '',
        '# 0x80500000..0x80500020 | size: 0x20',
        '.data',
        '.balign 8',
        '',
        '# .data:0x0 | 0x80500000 | size: 0x10',
        '.obj lbl_80500000, global',
        '\t.4byte quest_init__FUc',
        '\t.4byte 0x00000000',
        '\t.4byte fn_80001000',
        '\t.4byte 0x00000000',
        '.endobj lbl_80500000',
        '',
        '# .data:0x10 | 0x80500010 | size: 0x4',
        '.obj lbl_80500010, global',
        '\t.4byte lbl_80500000',
        '.endobj lbl_80500010',
        '',
    ]) + "\n"
    return {"menu/multi_result.s": unit, "auto_fn_80001000_text.s": stale,
            "auto_07_80500000_data.s": data}


def _fixture_map():
    return "\n".join([
        "quest_init__FUc = .text:0x80001280; // type:function size:0x40 scope:global",
        "caller = .text:0x80001000; // type:function size:0x1C scope:global",
        "with_data = .text:0x80001060; // type:function size:0x18 scope:global",
        "taker = .text:0x800010A0; // type:function size:0x14 scope:global",
        "fn_800010B8 = .text:0x800010B8; // type:function size:0x8 scope:global",
        "lbl_80500000 = .data:0x80500000; // type:object size:0x10 scope:global",
        "lbl_80500020 = .data:0x80500020; // type:object size:0x4 scope:global",
        ""]) + "\n"


def _fixture_splits():
    return ("Sections:\n\t.text       type:code align:4\n\n"
            "menu/multi_result.cpp:\n\t.text       start:0x80001000 end:0x80001100\n")


def _fixture_root(tmp):
    """The fake tree: the dump under `build/<game>/asm`, the map under `config/<game>/`."""
    os.makedirs(os.path.join(tmp, "config", GAME), exist_ok=True)
    asm = asm_dir_of(tmp)
    with open(os.path.join(tmp, "config", GAME, "symbols.txt"), "w", encoding="utf-8",
              newline="") as fh:
        fh.write(_fixture_map())
    with open(os.path.join(tmp, "config", GAME, "splits.txt"), "w", encoding="utf-8", newline="") as fh:
        fh.write(_fixture_splits())
    for name, text in _fixture_dump().items():
        path = os.path.join(asm, *name.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="") as fh:
            fh.write(text)
    return asm


def selftest():
    """Fixture-driven checks for every reader, every query path and every exit; returns an exit code."""
    import contextlib
    import io
    import shutil
    import tempfile

    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s\n      got  %r\n      want %r" % (name, got, want))

    def check_in(name, needle, hay):
        nonlocal checks
        checks += 1
        if needle not in hay:
            fails.append("%s\n      %r is not in the output:\n%s" % (name, needle, hay[:3000]))

    def run(rep, info=None, root=None, asm=None, state="present", msg="fixture"):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = print_report(rep, info=info, state=state, msg=msg, remedy=None, root=root,
                                asm_dir=asm)
        return code, buf.getvalue()

    # --- the decoder the address graph rests on ---------------------------------------------------
    check("branch: a bl decodes its own displacement", branch_target(0x80001008, "48 00 02 79"),
          0x80001280)
    check("branch: a b decodes too", branch_target(0x800010AC, "48 00 00 0D"), 0x800010B8)
    check("branch: a negative displacement wraps", branch_target(0x800010A8, "4B FF FF 59"),
          0x80001000)
    check("branch: a non-branch encoding is refused", branch_target(0x80001000, "94 21 FF F0"), None)
    check("branch: an absolute branch is refused", branch_target(0x80001000, "48 00 00 02"), None)
    check("operand: a register is not a symbol", symbol_operand("r3, 0x0"), None)
    check("operand: a local label is not a symbol", symbol_operand(".L_800010A4"), None)
    check("operand: a name is", symbol_operand("quest_init__FUc"), "quest_init__FUc")
    check("operand: a quoted name is unwrapped", symbol_operand('"@eti_80038FAC"'), "@eti_80038FAC")
    check("mem: a load reads", mem_kind("lwz"), "read")
    check("mem: a store writes", mem_kind("stw"), "write")
    check("mem: li only materialises the address", mem_kind("li"), "addr")
    check("mem: lis only materialises the address", mem_kind("lis"), "addr")
    check("mem: addi only materialises the address", mem_kind("addi"), "addr")
    check("arg: li r3 before the call is the argument", arg_hint([("li", "r3, 0x1")], False), "0x1")
    check("arg: a lis/addi address is the argument",
          arg_hint([("lis", "r3, lbl_1@ha"), ("addi", "r3, r3, lbl_1@l")], False), "&lbl_1")
    check("arg: a preceding call leaves r3 live-in", arg_hint([("bl", "x")], False), "0 (r3 live-in)")
    check("arg: nothing writes r3", arg_hint([("li", "r4, 0x1")], False), "0")
    check("arg: a truncated window is not called zero", arg_hint([("li", "r4, 0x1")], True), "0?")
    check("arg: an unmodelled writer is named, not guessed",
          arg_hint([("lwz", "r3, 0x10(r31)")], False), "? (lwz)")
    check("rank: a top-level auto file is the scaffolding",
          is_scaffolding(os.path.join("x", "auto_fn_1_text.s"), "x"), True)
    check("rank: a unit directory file is not",
          is_scaffolding(os.path.join("x", "menu", "multi_result.s"), "x"), False)

    # --- the whole pipeline against the fixture tree ----------------------------------------------
    tmp = tempfile.mkdtemp(prefix="callers-fixture-")
    try:
        asm = _fixture_root(tmp)
        cache = cache_of(tmp)
        index, info = load_index(root=tmp, rebuild=True, asm_dir=asm, cache=cache)
        check("index: a build from scratch reports the rebuild", info["rebuilt"], True)
        check("index: every fixture file is read", index["files"], 3)
        check("index: the cache is written", os.path.exists(cache), True)
        check("index: a data label's address and size come from the dump's header",
              index["labels"]["lbl_80500000"], [0x80500000, 0x10, ".data"])
        check("index: a function's size comes from its .text block",
              index["labels"]["fn_80001280"][:2], [0x80001280, 0x40])
        check("index: the data sites and the pointer entry share one target key",
              sorted(row[1] for row in index["refs"]["0x80500000"]), ["addr", "addr", "pointer"])
        check("index: a duplicate site is dropped", index["stats"]["duplicates"], 2)
        check("index: a name only the map can resolve is kept by name",
              [row[1] for row in index["refs"]["quest_init__FUc"]], ["addr", "addr", "pointer"])
        cmap = load_map(tmp)
        check("map: the fixture map is read", len(cmap.symbols), 7)

        # a query for the *current* name: the dump prints the stale label at two of the four sites
        rep = query("quest_init", index, cmap)
        check("query: the plain name offers the mangled one",
              rep["candidates"], ["quest_init__FUc"])
        rep = query("quest_init__FUc", index, cmap)
        check("query: the target is the current map row", rep["resolved"]["address"], "0x80001280")
        check("query: the target's kind is code", rep["resolved"]["kind"], "code")
        check("query: every call site is found, from both copies of the dump",
              [r["site"] for r in rep["references"] if r["kind"] == "call"],
              ["0x80001008", "0x80001014", "0x80001204"])
        check("query: a caller the map does not know is named from the dump",
              [r["caller"]["name"] for r in rep["references"] if r["site"] == "0x80001204"],
              ["fn_80001200"])
        check("query: the address-taken site is found",
              [r["site"] for r in rep["references"] if r["kind"] == "addr"], ["0x800010A0"])
        check("query: a duplicated site is reported once",
              sum(1 for r in rep["references"] if r["site"] == "0x80001008"), 1)
        check("query: the canonical copy's text won",
              [r["instruction"] for r in rep["references"] if r["site"] == "0x80001008"],
              ["bl quest_init__FUc"])
        check("query: the caller is named from the map, not the dump's .fn",
              [r["caller"]["name"] for r in rep["references"] if r["site"] == "0x80001008"], ["caller"])
        check("query: the caller's owner comes from splits.txt",
              [r["caller"]["owner"] for r in rep["references"] if r["site"] == "0x80001008"],
              ["menu/multi_result.cpp (no source yet)"])
        check("query: the argument is inferred", rep["references"][0]["arg"], "0x0")
        check("query: the second call's argument follows the compiled code",
              [r["arg"] for r in rep["references"] if r["site"] == "0x80001014"], ["0x1"])
        check("query: a caller's argument can be an address",
              [r["arg"] for r in query("caller", index, cmap)["references"]], ["&quest_init__FUc"])
        rep_ptr = query("quest_init__FUc", index, cmap, pointers=True)
        check("query: a pointer entry only the map can name is found per query",
              [r["site"] for r in rep_ptr["references"] if r["kind"] == "pointer"], ["0x80500000"])
        check("query: and the retry says so",
              any("resolved through the current map" in n for n in rep_ptr["notes"]), True)
        check("query: the dump's stale label for the target is reported",
              rep["resolved"]["asm_label"], "fn_80001280")
        code, out = run(rep, info=info, root=tmp, asm=asm)
        check("report: it exits 0", code, 0)
        check_in("report: the stale label is called out", "stale label", out)
        check_in("report: the caller's owner is listed", "menu/multi_result.cpp", out)
        check_in("report: the counts are the header of each table", "3 call site(s) (bl)", out)

        # a query by address: the current name, never the dump's stale label
        rep = query("0x80001280", index, cmap)
        check("query by address: the name is the map's", rep["resolved"]["name"], "quest_init__FUc")
        check("query by address: the size is the dump's", rep["resolved"]["size"], 0x40)
        check("query by address: how=address", rep["how"], "address")

        # a query by the dump's own stale name: answered through the dump's label table, with a note
        rep = query("fn_80001280", index, cmap)
        check("query by a stale name: it resolves through the dump",
              rep["resolved"]["address"], "0x80001280")
        check("query by a stale name: how=dump", rep["how"], "dump")
        check("query by a stale name: the note names the current symbol",
              any("quest_init__FUc" in n for n in rep["notes"]), True)
        code, out = run(rep, root=tmp, asm=asm)
        check_in("query by a stale name: the answer is the same three calls", "3 call site(s)", out)
        # data: a read, a write and an address-taken pair (coalesced into one site)
        rep = query("lbl_80500000", index, cmap)
        check("data: the kind is data", rep["resolved"]["kind"], "data")
        check("data: the size is the object's", rep["resolved"]["size"], 0x10)
        check("data: the lis/addi pair is one site", rep["counts"]["addr"], 1)
        check("data: the coalesced site keeps both instructions",
              "lis r3, lbl_80500000@ha" in rep["references"][0]["instruction"], True)
        check("data: the pointer entry is hidden by default", rep["counts"]["pointer"], 0)
        check("data: the hidden pointer count is reported", rep["pointer_hidden"], 1)
        rep = query("lbl_80500000", index, cmap, pointers=True)
        check("data: --pointers lists the table", rep["counts"]["pointer"], 1)
        check("data: the pointer site is the containing object",
              [r["site"] for r in rep["references"] if r["kind"] == "pointer"], ["0x80500010"])
        check("data: the pointer's referrer is the object that holds it",
              [r["caller"]["name"] for r in rep["references"] if r["kind"] == "pointer"],
              ["lbl_80500010"])
        rep = query("lbl_80500020", index, cmap)
        check("data: a load is a read", rep["counts"]["read"], 1)
        check("data: a store is a write", rep["counts"]["write"], 1)
        check("data: the two sda21 accesses are two sites", rep["counts"]["sites"], 2)
        code, out = run(rep, root=tmp, asm=asm)
        check_in("data: the report names the reads", "read(s)", out)

        # --- the census and `--range`: the per-address referrer runs (the .sdata2/.data seam) --------
        check("reader: a source label is normalised to its unit", norm_reader("Pl/x.cpp"), "Pl/x")
        check("reader: an unsplit address is not a unit", norm_reader("unsplit address"), None)
        check("reader: a no-source label keeps the unit", norm_reader("Pl/x.cpp (no source yet)"),
              "Pl/x")
        census = readers_of(index, cmap)
        check("census: a data word's readers are its referencing units", census(0x80500000),
              {"menu/multi_result": 1})
        check("census: both sda21 accesses count", census(0x80500020), {"menu/multi_result": 2})
        check("census: an address nobody references has no reader", census(0x80500004), {})
        check("census: the same address is answered from the cache",
              census(0x80500000) is census(0x80500000), True)
        rng = range_report(index, cmap, 0x80500000, 0x8050000C, 4)
        check("range: four addresses are sampled", rng["range"]["addresses"], 4)
        check("range: the run boundaries", [(r["start"], r["end"], r["readers"])
                                             for r in rng["runs"]],
              [(0x80500000, 0x80500000, ["menu/multi_result"]),
               (0x80500004, 0x8050000C, [])])
        check("range: the reader-set change is the seam", rng["seams"], [0x80500004])
        check("range: a single-run range has no seam",
              range_report(index, cmap, 0x80500004, 0x8050000C, 4)["seams"], [])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = print_range(rng, root=tmp, asm_dir=asm, state="present")
        out = buf.getvalue()
        check("range: print_range exits 0", rc, 0)
        check_in("range: the runs are headed", "per-address referrer runs", out)
        check_in("range: the reader is listed", "menu/multi_result", out)
        check_in("range: the seam is named", "seams (1): 0x80500004", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--range", "0x80500000", "0x8050000C"], root=tmp)
        out = buf.getvalue()
        check("range: the CLI answers", rc, 0)
        check_in("range: the CLI prints the run span", "0x80500004-0x8050000C", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--range", "0x80500000", "0x8050000C", "--json", "--each"], root=tmp)
        payload = json.loads(buf.getvalue())
        check("range: --json parses", rc, 0)
        check("range: --json carries the runs", len(payload["runs"]), 2)
        check("range: --json carries every address", len(payload["addresses"]), 4)

        # filters
        rep = query("quest_init__FUc", index, cmap, kinds=list(CODE_KINDS))
        check("filter: --code keeps every caller", rep["counts"]["call"], 3)
        check("filter: --code drops the address-taken site", rep["counts"]["addr"], 0)
        rep = query("quest_init__FUc", index, cmap, kinds=list(DATA_KINDS))
        check("filter: --data drops the callers", rep["counts"]["call"], 0)
        check("filter: --data keeps the address-taken site", rep["counts"]["addr"], 1)
        check("find: an address query is recognised", find_target("0x80001280", index, cmap)[2],
              "address")
        check("find: a bare 8-digit address is recognised", find_target("80001280", index, cmap)[0],
              0x80001280)
        check("find: a substring query is offered as candidates",
              find_target("quest_init", index, cmap)[2], "plain name")
        check("find: a missing name has no candidate",
              find_target("no_such_symbol_anywhere", index, cmap)[1], [])
        rep = query("no_such_symbol_anywhere", index, cmap)
        check("query: a missing name is an error with no references",
              (bool(rep["error"]), rep["references"]), (True, []))
        code, out = run(rep, root=tmp, asm=asm)
        check("query: a missing name exits 1", code, 1)
        check_in("query: a missing name points at symedit", "symedit.py find", out)

        # the cache: the same dump -> no rebuild; a symbols.txt edit -> still no rebuild (address-keyed);
        # a dump edit -> rebuild
        _i2, info2 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a second load is a hit", info2["cached"], True)
        with open(os.path.join(tmp, "config", GAME, "symbols.txt"), "a", encoding="utf-8") as fh:
            fh.write("renamed_later = .text:0x80001280; // type:function size:0x40 scope:global\n")
        index3, info3 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a symbols.txt edit does not rebuild the graph", info3["cached"], True)
        check("cache: the address answers to the map's current name",
              query("quest_init__FUc", index3, load_map(tmp), kinds=list(CODE_KINDS))["counts"]["call"],
              3)
        with open(os.path.join(asm, "menu", "multi_result.s"), "a", encoding="utf-8") as fh:
            fh.write("\n")
        _i4, info4 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a dump edit rebuilds", (info4["cached"], info4["rebuilt"]), (False, True))
        check("cache: the rebuild says why", info4["reason"], "the dump changed since the index was built")
        index5, info5 = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: the rebuild is cached again", info5["cached"], True)
        check("cache: a forced rebuild ignores the cache",
              load_index(root=tmp, asm_dir=asm, cache=cache, rebuild=True)[1]["rebuilt"], True)
        check("cache: a fresh build reproduces the same targets",
              sorted(index5["refs"]) == sorted(index["refs"]), True)
        check("cache: a fresh build reproduces the stats",
              (info5["stats"]["refs"], info5["stats"]["duplicates"]),
              (index["stats"]["refs"], index["stats"]["duplicates"]))
        # a cache that cannot be trusted is rebuilt, never used: a half-written file, or another schema
        with open(cache, "w", encoding="utf-8") as fh:
            fh.write('{"schema": 1, "signature": ')
        _i, info_bad = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: a truncated file is rebuilt", (info_bad["cached"], info_bad["rebuilt"]),
              (False, True))
        check("cache: and the reason names it", info_bad["reason"].startswith("unreadable cache"), True)
        with open(cache, "w", encoding="utf-8") as fh:
            json.dump({"schema": SCHEMA - 1, "signature": info_bad["signature"], "files": 3}, fh)
        _i, info_schema = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: another schema is rebuilt", info_schema["cached"], False)
        check("cache: and the reason names the schema",
              info_schema["reason"].startswith("cache schema"), True)
        with open(cache, "w", encoding="utf-8") as fh:
            json.dump({"schema": SCHEMA, "signature": "not-this-dump", "files": 3}, fh)
        _i, info_sig = load_index(root=tmp, asm_dir=asm, cache=cache)
        check("cache: another dump's signature is rebuilt",
              (info_sig["cached"], info_sig["reason"]),
              (False, "the dump changed since the index was built"))

        # a missing dump is never answered with a count of zero callers
        empty = os.path.join(tmp, "empty-tree")
        os.makedirs(os.path.join(empty, "build", GAME, "asm"), exist_ok=True)
        state_e, msg_e, remedy_e = dump_state(asm_dir_of(empty), [], empty)
        check("missing dump: the state says so", state_e, "missing")
        check("missing dump: the remedy is dump_asm.py", DUMP_TOOL, remedy_e)
        check("missing dump: the message names no count", "0 caller" in msg_e, False)
        _ie, info_e = load_index(root=empty, cache=os.path.join(empty, "no.json"))
        check("missing dump: load_index returns nothing", _ie, None)
        check("missing dump: the reason is 'no dump'", info_e["reason"], "no dump")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init"], root=empty)
        out = buf.getvalue()
        check("missing dump: the exit is 2", rc, 2)
        check_in("missing dump: the remedy is printed", DUMP_TOOL, out)
        check_in("missing dump: it refuses to say 0 callers", "not '0 callers'", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init", "--json"], root=empty)
        check("missing dump: --json carries the error", '"error": "no asm dump"' in buf.getvalue(),
              True)

        # the CLI end to end on the fixture tree (as a human runs it)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init__FUc", "--code", "--limit", "2"], root=tmp)
        out = buf.getvalue()
        check("cli: --code --limit exits 0", rc, 0)
        check_in("cli: the header names the target", "== quest_init__FUc  0x80001280", out)
        check_in("cli: the limit is stated", "1 more, raise --limit", out)
        check_in("cli: the dump's state is printed", "present", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["quest_init__FUc", "--data", "--json"], root=tmp)
        payload = json.loads(buf.getvalue())
        check("cli: --json parses", rc, 0)
        check("cli: --json reports the dump's state", payload["dump"]["state"], "present")
        check("cli: --json reports the index as cached",
              payload["index"]["cached"], True)
        check("cli: --json keeps the address-taken site",
              [r["site"] for r in payload["references"]], ["0x800010A0"])
        check("cli: --json names the caller", payload["references"][0]["caller"]["name"], "taker")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--stats"], root=tmp)
        out = buf.getvalue()
        check("cli: --stats exits 0", rc, 0)
        check_in("cli: --stats counts the targets", "target address(es)", out)
        check_in("cli: --stats names the index", "graph.json", out)

        # --- the no-dump fallback: the same answer from the split objects' relocations ----------------
        # `build/<game>/asm` is written only on demand, so a built-but-not-dumped tree has no dump and
        # `callers.py` used to exit 2. The fallback builds the graph from `build/<game>/obj/**/*.o`'s
        # relocations (`dossier.parse_elf`), which is what the question actually needs: a `bl` is a
        # relocation to the callee. This tree is set up with objects and NO asm dir.
        elf_tree = os.path.join(tmp, "elf-tree")
        os.makedirs(os.path.join(elf_tree, "config", GAME), exist_ok=True)
        os.makedirs(os.path.join(elf_tree, "build", GAME, "obj", "probe"), exist_ok=True)
        with open(os.path.join(elf_tree, "config", GAME, "symbols.txt"), "w", encoding="utf-8",
                  newline="") as fh:
            fh.write("caller = .text:0x80001000; // type:function size:0x20 scope:global\n"
                     "callee = .text:0x80002000; // type:function size:0x10 scope:global\n"
                     "gData = .data:0x80500000; // type:object size:0x10 scope:global\n")
        with open(os.path.join(elf_tree, "config", GAME, "splits.txt"), "w", encoding="utf-8",
                  newline="") as fh:
            fh.write("probe/unit.c:\n\t.text       start:0x80001000 end:0x80001020\n")
        elf = cl._fixture_elf(b"\x48\x00\x00\x01" * 8,
                              [("", 0, 0, 0, 0), ("caller", 0, 0x20, 0x12, 1),
                               ("callee", 0, 0, 0x10, 0), ("gData", 0, 0, 0x12, 0)],
                              [(0x08, 2, 10), (0x0C, 3, 6)])
        with open(os.path.join(elf_tree, "build", GAME, "obj", "probe", "unit.o"), "wb") as fh:
            fh.write(elf)
        elf_cmap = load_map(elf_tree)
        elf_index, elf_info = load_elf_index(root=elf_tree, rebuild=True, cmap=elf_cmap,
                                             cache=os.path.join(elf_tree, "cache.json"))
        check("elf: no dump but objects builds a graph", elf_info["source"], "elf")
        check("elf: the call relocation lands on the callee's address",
              any(r[1] == "call" for r in elf_index["refs"].get("0x80002000", [])), True)
        check("elf: the call site is the anchored absolute address",
              [r[0] for r in elf_index["refs"].get("0x80002000", [])], [0x80001008])
        check("elf: the data relocation is an address reference",
              [r[1] for r in elf_index["refs"].get("0x80500000", [])], ["addr"])
        elf_rep = query("callee", elf_index, elf_cmap)
        check("elf: query resolves the callee through the map",
              elf_rep["resolved"]["address"], "0x80002000")
        check("elf: query reports the call", elf_rep["counts"]["call"], 1)
        check("elf: the caller is named from its own anchor",
              [r["caller"]["name"] for r in elf_rep["references"]], ["caller"])
        check("elf: the reference text names the target (no dump instruction text)",
              [r["instruction"] for r in elf_rep["references"]], ["bl callee"])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["callee", "--code"], root=elf_tree)
        out = buf.getvalue()
        check("elf: the CLI answers without a dump", rc, 0)
        check_in("elf: the CLI says which graph answered", "elf fallback", out)
        check_in("elf: the call is listed", "bl callee", out)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--stats"], root=elf_tree)
        check("elf: --stats exits 0", rc, 0)
        check_in("elf: --stats names the objects", "elf-graph.json", buf.getvalue())
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    sys.exit(main())
