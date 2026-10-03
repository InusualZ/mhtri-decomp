"""Who references what: the address-keyed reference index (asm dump or target objects), the census, the text scan.
Spec: docs/tools/spec/lib-refs.md. CLI: none (library)."""
from __future__ import annotations

import collections
import json
import os
import re
import struct
import time
from dataclasses import dataclass, field

from tools.lib import cache as libcache
from tools.lib import ppc
from tools.lib import text as libtext
from tools.lib.binary.elf import Elf, ElfError, reloc_name

#: bump on any change to what the reference index stores (the on-disk format `callers.py` reads)
SCHEMA = 1
CODE_SECTIONS = (".text", ".init")
KINDS = ("call", "branch", "addr", "read", "write", "pointer")
CODE_KINDS = ("call", "branch")
DATA_KINDS = ("addr", "read", "write", "pointer")
#: instructions scanned back from a `bl` for what r3 holds
ARG_WINDOW = 12
#: the relocation types that are a call (`R_PPC_REL24`, the `bl` form)
CALL_TYPES = (10,)
#: relocation targets that are unwind bookkeeping, never a reference the code makes
BOOKKEEPING = ("extab", "extabindex")
#: the sections the data census classifies
CENSUS_SECTIONS = (".rodata", ".data", ".bss", ".sdata", ".sbss", ".sdata2", ".sbss2")

LOAD_MNEMONICS = frozenset((
    "li lis lwz lwzu lbz lbzu lhz lhzu lha lhau lmw lfs lfsu lfd lfdu lwzx lwzux lbzx lbzux lhzx lhax "
    "lhaux lfsx lfdx lwarx psq_l psq_lx").split())

#: one regex for a whole dump `.s` file: header comments, `.fn`/`.obj` blocks, instructions, `.4byte`
SCAN_RE = re.compile(
    r"(?m)^#\s*(?P<sec>[.\w]+):(?:0x)?[0-9A-Fa-f]+\s*\|\s*(?P<haddr>0x[0-9A-Fa-f]+)"
    r"\s*\|\s*size:\s*(?P<hsize>0x[0-9A-Fa-f]+)\s*$"
    r"|^\.fn\s+(?P<fn>[^\s,]+)"
    r'|^\.obj\s+"?(?P<obj>[^\s,"]+)"?'
    r"|^/\*\s+(?P<iaddr>[0-9A-Fa-f]{8})\s+(?:[0-9A-Fa-f]+)\s+(?P<ibytes>[0-9A-Fa-f ]+?)"
    r"\s*\*/\s*(?P<itext>.+?)\s*$"
    r"|^[ \t]*\.(?:4byte|long)\s+(?P<ptr>\S+)\s*$")
#: a symbol as an operand (never a register, an immediate, a `.L_` label or a section name)
SYM_RE = re.compile(r'^(?:"([^"]+)"|([A-Za-z_$][\w$.]*))')
#: the `@` modifier that turns an operand into a reference to a symbol's address
MOD_RE = re.compile(r'(?:"([^"]+)"|([A-Za-z_$][\w$.]*))@(ha|h|l|sda21|sda2)\b')


# --- the asm dump ---------------------------------------------------------------------------------------------

def dump_files(asm_dir: str) -> list[str]:
    """Every `.s` of a dump directory, sorted (`.stamp.json` is not a unit)."""
    out = []
    for dirpath, _dirs, names in os.walk(asm_dir):
        for n in names:
            if n.endswith(".s"):
                out.append(os.path.join(dirpath, n))
    out.sort()
    return out


def is_scaffolding(path: str, asm_dir: str) -> bool:
    """Whether a dump file is a top-level `auto_*` scaffolding copy rather than a unit's own file."""
    return os.path.dirname(os.path.abspath(path)) == os.path.abspath(asm_dir)


def file_rank(path: str, asm_dir: str) -> tuple[int, int]:
    """How much a copy of one unit's asm is trusted (low wins): not the top-level scaffolding, then newest mtime."""
    return (1 if is_scaffolding(path, asm_dir) else 0, -os.stat(path).st_mtime_ns)


def dump_branch_target(address: int, byte_text: str) -> int | None:
    """The branch target of a dump instruction from its printed bytes (`48 00 02 79`), or None."""
    try:
        word = int(byte_text.replace(" ", ""), 16)
    except ValueError:
        return None
    return ppc.branch_target(address, word)


def symbol_operand(operands: str) -> str | None:
    """The symbol an operand names, or None for a register/immediate/local `.L_` label."""
    tok = operands.split(",")[0].strip() if operands else ""
    m = SYM_RE.match(tok)
    if not m:
        return None
    name = m.group(1) or m.group(2)
    if name.startswith(".") or re.fullmatch(r"(?:r|f|cr|vs|vr)\d+", name):
        return None
    return name


def mem_kind(mnemonic: str) -> str:
    """`read` / `write` / `addr` for an instruction that names a symbol's address, from the mnemonic's shape."""
    if mnemonic in LOAD_MNEMONICS:
        return "addr" if mnemonic in ("li", "lis") else "read"
    if mnemonic.startswith("st") or mnemonic in ("psq_st", "psq_stx"):
        return "write"
    return "addr"


def arg_hint(window, truncated: bool) -> str:
    """What r3 holds at a call, from the `[(mnemonic, operands)]` before it (`truncated`: the window lost its
    oldest instruction, so a missing writer can no longer be read as "0")."""
    for mnemonic, operands in reversed(window):
        toks = [t.strip() for t in operands.split(",")] if operands else []
        _reads, writes, is_branch, is_call, decoded = ppc.decode_rw(mnemonic, operands)
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


def parse_dump_file(path: str):
    """`(labels, funcs, refs)` for one dump `.s`: `labels` is `name -> (address, size, section)` from the header
    comments (the dump's own label table), `funcs` is `function address -> .fn label`, `refs` are unresolved sites
    `(site, kind, name, encoded_target_or_None, func_addr, text, arg)`."""
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    labels, funcs, refs = {}, {}, []
    pending = None
    cur = None
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
            refs.append((addr, kind, name, dump_branch_target(addr, m.group("ibytes")), func, asm, arg))
        elif "@" in operands:
            for mod in MOD_RE.finditer(operands):
                refs.append((addr, mem_kind(mnemonic), mod.group(1) or mod.group(2), None, func, asm, ""))
        window.append((mnemonic, operands))
        if len(window) > ARG_WINDOW:
            del window[0]
            truncated = True
    return labels, funcs, refs


def key_of(ref) -> tuple:
    """A reference site's identity: `(site, kind)`, plus the name for a `.4byte` entry (one object holds many)."""
    site, kind, name = ref[0], ref[1], ref[2]
    return (site, kind, name) if kind == "pointer" else (site, kind)


def _index_body(source: str | None, game: str, t0: float, files: int, nbytes: int, labels: dict, funcs: dict,
                refs: dict) -> dict:
    out = {"schema": SCHEMA, "game": game}
    if source:
        out["source"] = source
    out.update({
        "built_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "seconds": round(time.time() - t0, 1),
        "files": files,
        "bytes": nbytes,
        "labels": {n: list(v) for n, v in labels.items()},
        "funcs": {str(a): n for a, n in sorted(funcs.items())},
        "refs": refs,
    })
    return out


def _stats(by_target, labels: dict, dropped: int, conflicts: list, mismatch: int) -> dict:
    return {
        "refs": sum(len(v) for v in by_target.values()),
        "targets": sum(1 for k in by_target if k.startswith("0x")),
        "labels": len(labels),
        "unresolved": sum(1 for k in by_target if not k.startswith("0x")),
        "duplicates": dropped,
        "label_conflicts": conflicts,
        "label_mismatch": mismatch,
        "biggest": sorted(((len(v), k) for k, v in by_target.items() if k.startswith("0x")), reverse=True)[:3],
    }


def build_dump_index(asm_dir: str, files: list[str], game: str) -> tuple[dict, dict]:
    """`(index, stats)` from every dump file; each site keeps its best-ranked copy (`file_rank`), and a target is
    the encoded branch displacement or the dump's own label table (never the printed name)."""
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
                    labels[name] = (addr, size, section)
            elif old[2] in CODE_SECTIONS and section not in CODE_SECTIONS:
                pass
            elif len(conflicts) < 8:
                conflicts.append((name, "0x%08X" % old[0], "0x%08X" % addr, os.path.relpath(path, asm_dir)))
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
            mismatch += 1
        row = [site, kind, func, asm, arg]
        if target is None:
            by_target[name].append(row)
            continue
        by_target[("0x%08X" % target)].append(row)
    index = _index_body(None, game, t0, len(files), sum(os.path.getsize(p) for p in files), labels, funcs,
                        {k: sorted(v, key=lambda r: (r[0], r[1])) for k, v in by_target.items()})
    return index, _stats(by_target, labels, dropped, conflicts, mismatch)


# --- the target objects ---------------------------------------------------------------------------------------

def object_files(obj_dir: str) -> list[str]:
    """Every `.o` under a directory, sorted."""
    out = []
    for dirpath, _dirs, names in os.walk(obj_dir):
        for n in names:
            if n.endswith(".o"):
                out.append(os.path.join(dirpath, n))
    out.sort()
    return out


def object_signature(obj_dir: str, files: list[str]) -> str:
    """The object index's cache key: every object's relative path, size and mtime."""
    return libcache.stat_digest(files, obj_dir)


def build_object_index(obj_dir: str, files: list[str], symbols, game: str) -> tuple[dict, dict]:
    """`(index, stats)` from the split objects' relocations - the same shape as `build_dump_index`.

    `symbols` is `{name: [(section, address, type), ...]}` (`Ownership.symbols`): one map-known symbol per section
    anchors that section's base. Every non-call is `addr`, a data section's relocation is `pointer`, `source: elf`."""
    t0 = time.time()
    labels, funcs = {}, {}
    by_target = collections.defaultdict(dict)
    scanned, dropped = 0, 0
    for path in files:
        try:
            elf = Elf.read(path)
        except (OSError, ElfError):
            continue
        scanned += 1
        by_sec = collections.defaultdict(list)
        for s in elf.symbols:
            if s.shndx and s.name:
                by_sec[s.section].append(s)
        base = {}
        for sec, lst in by_sec.items():
            for s in sorted(lst, key=lambda x: x.value):
                row = symbols.get(s.name)
                if row:
                    base[sec] = row[0][1] - s.value
                    break
            if sec in base:
                for s in sorted(lst, key=lambda x: x.value):
                    labels.setdefault(s.name, (base[sec] + s.value, s.size or 0, sec))
                    if sec in CODE_SECTIONS and s.type == 2:
                        funcs.setdefault(base[sec] + s.value, s.name)
        ordered = {sec: sorted(lst, key=lambda x: x.value) for sec, lst in by_sec.items()}

        def holder(sec, off):
            best = None
            for s in ordered.get(sec, ()):
                if s.value <= off and (s.size == 0 or off < s.value + s.size):
                    best = s
                elif s.value > off:
                    break
            return best

        for r in elf.relocs():
            name, site_sec = r.symbol_name, r.section
            if not name or not site_sec or site_sec not in base or name.startswith("."):
                continue
            h = holder(site_sec, r.offset)
            site = base[site_sec] + r.offset
            func_addr = base[site_sec] + h.value if h is not None else None
            if func_addr is not None:
                funcs.setdefault(func_addr, h.name)
            type_name = reloc_name(r.type, "type-%d")
            if site_sec in CODE_SECTIONS:
                kind = "call" if r.type in CALL_TYPES else "addr"
                text = ("bl %s" % name) if kind == "call" else "%s %s" % (type_name, name)
            else:
                kind = "pointer"
                text = "%s %s" % (type_name, name)
            row_map = symbols.get(name)
            key = ("0x%08X" % (row_map[0][1] + (r.addend or 0))) if row_map else name
            slot = (site, kind, func_addr, text)
            if slot in by_target[key]:
                dropped += 1
            by_target[key][slot] = [site, kind, func_addr, text, ""]
    index = _index_body("elf", game, t0, scanned, sum(os.path.getsize(p) for p in files), labels, funcs,
                        {k: sorted(v.values(), key=lambda r: (r[0], r[1])) for k, v in by_target.items()})
    return index, _stats(by_target, labels, dropped, [], 0)


# --- the cache and the in-memory lookups ----------------------------------------------------------------------

def attach_derived(index: dict) -> dict:
    """Add the lookups a query needs (never serialised): `_label_at` (address -> the names printed there, sorted)
    and `_funcs` (integer-keyed `funcs`); reference rows become tuples."""
    by_addr = {}
    for name, row in index["labels"].items():
        by_addr.setdefault(row[0], []).append(name)
    for names in by_addr.values():
        names.sort()
    index["_label_at"] = by_addr
    index["_funcs"] = {int(k): v for k, v in index["funcs"].items()}
    index["refs"] = {k: [tuple(r) for r in v] for k, v in index["refs"].items()}
    return index


def load_index(cache: str, signature: str, files: int, build, rebuild: bool = False,
               changed: str = "the inputs changed since the index was built") -> tuple[dict, dict]:
    """`(index, info)` from `cache` when its schema, signature and file count agree, else `build()` (-> `(index,
    stats)`), written back atomically. `info` says whether the cache answered and why not (`reason`)."""
    info = {"cache": cache, "files": files, "cached": False, "rebuilt": False, "reason": "no cache", "stats": {},
            "signature": signature}
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
            elif index.get("signature") != signature or index.get("files") != files:
                info["reason"] = changed
                index = None
            else:
                info.update(cached=True, reason="cache hit", index=index, stats=index.get("stats", {}))
                info["cache_bytes"] = os.path.getsize(cache)
                return attach_derived(index), info
    elif rebuild:
        info["reason"] = "forced rebuild"
    index, stats = build()
    index["signature"] = signature
    index["stats"] = stats
    info.update(rebuilt=True, index=index, stats=stats)
    try:
        libtext.atomic_write(cache, json.dumps(index, separators=(",", ":"), sort_keys=True))
        info["cache_bytes"] = os.path.getsize(cache)
    except OSError as exc:
        info["cache_error"] = str(exc)
    return attach_derived(index), info


def coalesce(rows) -> list[tuple]:
    """Merge a `lis X@ha` + `addi X@l` pair (same function, same kind, adjacent) into one site."""
    out = []
    for row in sorted(rows, key=lambda r: r[0]):
        if out and row[0] == out[-1][0] + 4 and "@ha" in out[-1][3] and "@l" in row[3] \
                and out[-1][2] == row[2] and out[-1][1] == row[1]:
            prev = out[-1]
            out[-1] = (prev[0], prev[1], prev[2], prev[3] + " + " + row[3], prev[4])
            continue
        out.append(tuple(row))
    return out


def rows_at(index: dict, address: int, names=()) -> tuple[list[tuple], list[str]]:
    """`(rows, retried)`: the reference rows `(site, kind, func, text, arg)` at `address`, plus the rows the index
    kept under a name it could not resolve when one of `names` is it (the current map names the address now);
    `retried` lists those names, sorted."""
    rows = [tuple(r) for r in index["refs"].get("0x%08X" % address, [])]
    retried = set()
    for nm in set(names) - {None, ""}:
        for row in index["refs"].get(nm, []):
            if row not in rows:
                rows.append(row)
                retried.add(nm)
    return rows, sorted(retried)


def runs_over(readers, lo: int, hi: int, step: int = 4) -> dict:
    """The per-address reader runs over `[lo, hi]` (inclusive): `readers(address) -> {unit: sites}`; equal
    neighbouring reader sets collapse into one run, and the first address of every later run is a seam."""
    addresses = []
    address = lo
    while address <= hi:
        found = readers(address)
        addresses.append({"address": address, "readers": sorted(found), "sites": sum(found.values())})
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


# --- the census over target objects ---------------------------------------------------------------------------

def object_refs(path: str) -> dict[str, int]:
    """`{name: relocation count}` for the symbols an object references and does not define (bookkeeping
    relocations skipped); `{}` when the object cannot be read."""
    try:
        elf = Elf.read(path)
        defined = {s.name for s in elf.symbols if s.name and s.shndx}
        relocs = elf.relocs()
    except (OSError, ElfError, struct.error):
        return {}
    out: dict[str, int] = {}
    for r in relocs:
        name = r.symbol_name
        if not name or r.section.startswith(BOOKKEEPING) or name in defined:
            continue
        out[name] = out.get(name, 0) + 1
    return out


def classify_address(section: str, address: int, ranges: dict, unit: str) -> dict:
    """`{status, owner, prev, next}` for one data address against the claims `{section: [(start, end, unit)]}`
    (sorted): `own`, `other` (owner named) or `orphan` (`prev`/`next` the registered neighbours bracketing it)."""
    prev = nxt = None
    for start, end, owner in ranges.get(section, []):
        if start <= address < end:
            return {"status": "own" if owner == unit else "other", "owner": owner, "prev": None, "next": None}
        if end <= address:
            prev = (owner, start, end)
        elif start > address and nxt is None:
            nxt = (owner, start, end)
    return {"status": "orphan", "owner": None, "prev": prev, "next": nxt}


def census_records(unit_refs: dict, symbols: dict, ranges: dict) -> tuple[list[dict], dict]:
    """`(records, stats)`: one record per (unit, referenced data symbol). `unit_refs` is `{unit: {name: sites}}`,
    `symbols` `{name: {section, address, size, type}}`; an unmapped, code or non-data name is counted in `stats`."""
    records, stats = [], {"unmapped": 0, "code": 0, "other_section": 0}
    for unit in sorted(unit_refs):
        for name, sites in sorted(unit_refs[unit].items()):
            entry = symbols.get(name)
            if entry is None:
                stats["unmapped"] += 1
                continue
            if entry.get("type") == "function":
                stats["code"] += 1
                continue
            if entry["section"] not in CENSUS_SECTIONS:
                stats["other_section"] += 1
                continue
            verdict = classify_address(entry["section"], entry["address"], ranges, unit)
            records.append({"unit": unit, "name": name, "section": entry["section"],
                            "address": entry["address"], "size": int(entry.get("size") or 0),
                            "sites": sites, **verdict})
    return records, stats


def census(obj_dir: str, units, symbols: dict, ranges: dict) -> tuple[tuple[list[dict], dict], int]:
    """`((records, stats), read)` for each unit's target object `<obj_dir>/<unit>.o` that exists."""
    unit_refs = {}
    for unit in units:
        path = os.path.join(obj_dir, unit + ".o")
        if os.path.exists(path):
            unit_refs[unit] = object_refs(path)
    return census_records(unit_refs, symbols, ranges), len(unit_refs)


# --- the retail-text scan (the splitcheck context's readers) --------------------------------------------------

@dataclass(frozen=True)
class TextRefs:
    """What the decoded retail text references: `refs`/`loads`/`stores` are `{address: [site]}`, `passes`
    `{address: [(site, register, callee)]}`, `fn_edges` `[(site, function start, "call" | "addr")]` in scan order."""
    refs: dict = field(default_factory=dict)
    loads: dict = field(default_factory=dict)
    stores: dict = field(default_factory=dict)
    passes: dict = field(default_factory=dict)
    fn_edges: list = field(default_factory=list)


def text_refs(chunks, sda13, sda2, is_data, fn_starts, same_function) -> TextRefs:
    """Scan code `chunks` (`[(start, bytes)]`) with `ppc.scan_refs` (data references through r13/r2/`lis`) and
    `ppc.scan_calls`; a function address taken (`lis`/`addi`) counts as an `addr` edge unless
    `same_function(site, target)` (a function's own address is not a reference)."""
    raw = collections.defaultdict(list)
    loads, stores, passes = {}, {}, {}
    edges = []
    fnset = set(fn_starts)
    for lo, blob in chunks:
        if not blob:
            continue
        for t, sites in ppc.scan_refs(blob, lo, sda13, sda2, is_data, fn_starts, loads, stores, passes).items():
            raw[t].extend(sites)
        for site, t in ppc.scan_calls(blob, lo, fn_starts):
            edges.append((site, t, "call"))
        for t, sites in ppc.scan_refs(blob, lo, None, None, fnset.__contains__, fn_starts).items():
            for site in sites:
                if not same_function(site, t):
                    edges.append((site, t, "addr"))
    return TextRefs(dict(raw), loads, stores, passes, edges)


# --- the per-function graph over the dump (tudiscover) --------------------------------------------------------

TOKEN_RE = re.compile(r"[A-Za-z_@][A-Za-z0-9_@$.]*")
CALL_RE = re.compile(r"\bbl (\S+)")
SAVE_RE = re.compile(r"\b(?:_savegpr_|_restgpr_|stmw|lmw)\S*")
REC_RE = re.compile(r"\b(?:rlwinm|and|or|add|subf|subfc|neg|cntlzw|slw|srw|andc|xor|extsb|extsh)\.")
ETI_RE = re.compile(r'\.obj "@eti_([0-9A-Fa-f]{8})".*?\.endobj', re.S)
FOURBYTE_RE = re.compile(r"\.4byte\s+(\S+)")
#: a function is exactly its `.fn <name>, ...` .. matching `.endfn <name>` span
FN_BLOCK_RE = re.compile(r"(?ms)^\.fn\s+([^\s,]+)[^\n]*\n(.*?)^\.endfn\s+\1[ \t]*$")
FN_NAME_RE = re.compile(r"(?m)^\.fn\s+([^\s,]+)")
FIRST_INS_RE = re.compile(r"/\* ([0-9A-Fa-f]{8}) ")
REL_OBJ_RE = re.compile(r"(?ms)^\.obj\s+\"?([^\s,\"]+)\"?[^\n]*\n(.*?)^\.endobj\s")
REL_OWNER_RE = re.compile(r"^\s*\.rel\s+([^\s,]+)", re.M)
AT_SPELLED_RE = re.compile(r"^(@[^_]+)_([0-9A-Fa-f]{8})$")
#: sections whose labels are never TU-shared data (unwind fragments, boot code)
NO_REF_SECTIONS = ("extab", "extabindex", ".init")


def resolve_name(tok: str, names, labels=None) -> str | None:
    """An asm operand token -> a symbol-map name, or None: `name@modifier` drops the modifier unless the token starts
    with `@`, and dtk's `"@1841_80629B90"` resolves to the map's `@1841` only when the address agrees."""
    if tok in names:
        return tok
    if tok.startswith("@"):
        m = AT_SPELLED_RE.match(tok)
        if m and m.group(1) in names and (labels is None or labels[m.group(1)]["addr"] == int(m.group(2), 16)):
            return m.group(1)
        return None
    base = tok.split("@", 1)[0]
    return base if base in names else None


def rel_owners(txt: str, fns, labels) -> dict:
    """`data label -> {function names it relocates against}` from the dump's `.rel` lines (table ownership)."""
    if ".rel " not in txt:
        return {}
    out = {}
    for m in REL_OBJ_RE.finditer(txt):
        if m.group(1) not in labels:
            continue
        got = out.setdefault(m.group(1), set())
        for rel in REL_OWNER_RE.finditer(m.group(2)):
            if rel.group(1) in fns:
                got.add(rel.group(1))
    return out


def function_graph(files, fns: dict, labels: dict, rel_root: str) -> dict:
    """Per-function data references, calls and codegen fingerprint from the dump files: `{funcs, extab, owners,
    fn_check}`. `fns` is `{name: {addr, ...}}`, `labels` `{name: {section, addr, ...}}`; `rel_root` relativises the
    paths the `.fn` parse self-check names."""
    datanames = {n for n, l in labels.items() if l["section"] not in NO_REF_SECTIONS}
    graph, extab = {}, {}
    owners = collections.defaultdict(set)
    check = {"matched": 0, "mismatched_count": 0, "nocode_count": 0, "outside_map": 0,
             "unparsed_fn_lines": 0, "mismatched": [], "nocode": [], "parse_misses": []}
    for path in files:
        txt = open(path, "r", encoding="utf-8", errors="replace").read()
        blocks = list(FN_BLOCK_RE.finditer(txt))
        loose = len(re.findall(r"(?m)^[ \t]*\.fn[ \t]", txt))
        if loose != len(blocks):
            check["unparsed_fn_lines"] += loose - len(blocks)
            if len(check["parse_misses"]) < 20:
                check["parse_misses"].append([os.path.relpath(path, rel_root), loose, len(blocks)])
        for m in ETI_RE.finditer(txt):
            ops = FOURBYTE_RE.findall(m.group(0))
            if ops and ops[0] in fns:
                etb = int(ops[1], 16) if len(ops) > 1 and ops[1].startswith("0x") else None
                extab[ops[0]] = [int(m.group(1), 16), etb]
        for m in blocks:
            name, block = m.group(1), m.group(2)
            if name not in fns:
                check["outside_map"] += 1
                continue
            first = FIRST_INS_RE.search(block)
            got = int(first.group(1), 16) if first else None
            if got == fns[name]["addr"]:
                check["matched"] += 1
            elif got is None:
                check["nocode_count"] += 1
                if len(check["nocode"]) < 20:
                    check["nocode"].append(name)
            else:
                check["mismatched_count"] += 1
                if len(check["mismatched"]) < 20:
                    check["mismatched"].append([name, got, fns[name]["addr"]])
            refs, calls = set(), set()
            for tok in TOKEN_RE.findall(block):
                ref = resolve_name(tok, datanames, labels)
                if ref:
                    refs.add(ref)
            for callee in CALL_RE.findall(block):
                ref = resolve_name(callee, fns)
                if ref:
                    calls.add(ref)
            rec = len(REC_RE.findall(block))
            graph[name] = {"refs": sorted(refs), "calls": sorted(calls),
                           "fp": [1 if SAVE_RE.search(block) else 0, rec]}
        for name, own in rel_owners(txt, fns, labels).items():
            owners[name] |= own
    return {"funcs": graph, "extab": extab, "owners": {k: sorted(v) for k, v in owners.items()}, "fn_check": check}
