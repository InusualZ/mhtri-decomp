"""What a brief is made of: the unit's registration, ranges, symbols, scores, header, flags, shared headers, rule
sections and claim paths - the one briefing module that reads other tools. Spec: docs/tools/spec/briefing.md.
CLI: none (`tools/units/brief.py`)."""
from __future__ import annotations

import json
import os
import re
import sys

from tools.lib import project as _project
from tools.lib import units as _units
from tools.lib.lanes import naming, registry

SRC_EXT = (".c", ".cpp", ".cp", ".cxx", ".cc")
BAR = 80.0
#: A section a brief quotes is looked up by its title in both files (it may move between them).
RULE_DOCS = (("docs", "plan.md"), ("docs", "pipeline.md"))


# --- the other tools a brief reads, imported where they are used (each is a heavy module) ------------------------

def _langcheck():
    from tools.units import langcheck
    return langcheck


def _typeregistry():
    from tools.units import typeregistry
    return typeregistry


def _dossier():
    from tools.units import dossier
    return dossier


def _handoff():
    from tools.units import handoff
    return handoff


def language_cell(verdict) -> str:
    return _langcheck().language_cell(verdict)


def language_paragraph(verdict) -> str:
    return _langcheck().brief_paragraph(verdict)


def render_dossier(dossier: dict) -> str:
    return _dossier().render(dossier)


def config_schema_rows() -> list[dict]:
    return _handoff().config_schema_rows()


def flag_probe_verdicts():
    return _handoff().FLAG_PROBE_VERDICTS


def clear_caches() -> None:
    """Forget the type registry's cache (a test that rewrites `include/` between builds)."""
    _typeregistry().clear_cache()


# --- names and registration -------------------------------------------------------------------------------------

def source_name(unit: str, *roots: str) -> str:
    """`Pl/pl_act` -> `Pl/pl_act.cpp`: the real extension from the first root that has the file, else `.cpp`."""
    unit = naming.norm_unit(unit.strip("/"))
    if unit.endswith(SRC_EXT):
        return unit
    for root in roots:
        if not root:
            continue
        for ext in SRC_EXT:
            if os.path.exists(os.path.join(root, "src", *unit.split("/")) + ext):
                return unit + ext
    return unit + ".cpp"


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def text_has_bodies(text: str) -> bool:
    """A brace outside the comments - the cheap source-level signal that a body exists."""
    return "{" in strip_comments(text)


def has_bodies(path: str) -> bool:
    if not os.path.exists(path):
        return False
    with open(path, encoding="utf-8", errors="replace") as fh:
        return text_has_bodies(fh.read())


def registered_objects(main: str) -> list:
    """Every `Object(flag, "path")` row of `configure.py`'s `config.libs`, in registration order."""
    with open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace") as fh:
        return _project.object_calls(fh.read())


def registered_units(main: str) -> list[str]:
    """Every source file registered in `configure.py`, in registration order."""
    return [c.path for c in registered_objects(main)]


def pool_units(main: str) -> list[str]:
    """The pool: every registered unit that is not `Matching` and whose source exists - extensionless, sorted.
    (Until 2026-10-04 the pool was the body-less units only, which hid every NonMatching unit with a body.)"""
    out = []
    for c in registered_objects(main):
        if c.flag == "Matching":
            continue
        if os.path.exists(os.path.join(main, "src", *c.path.split("/"))):
            out.append(naming.norm_unit(c.path))
    return sorted(set(out))


# --- the claim --------------------------------------------------------------------------------------------------

def claim_for(main: str, unit: str) -> dict:
    """The claim holding `unit` (its own registry row, else the cluster claim listing it) - `{}` when unclaimed."""
    return registry.record_holding(main, unit)


def claim_slug(claim: dict) -> str | None:
    """The handoff slug: the claim's branch minus `worker/` (None for no branch - never invented)."""
    return naming.slug_of_branch((claim or {}).get("branch"))


def handoff_paths(main: str, unit: str, assume_claim: bool = False) -> dict:
    """Where a worker's artefacts go, from the claim; `assume_claim` renders against the branch `claims.py
    claim` will make (`worker/<slug(unit)>`) without writing the registry."""
    unit = unit.strip("/")
    claim = claim_for(main, unit)
    if not claim and assume_claim:
        claim = {"branch": naming.branch_for(naming.norm_unit(unit))}
    slug = claim_slug(claim)
    outbox = os.path.join(main, ".pi", "outbox", slug + ".json") if slug else None
    notes = os.path.join(main, ".pi", "notes", slug + ".md") if slug else None
    return {"claimed": bool(slug), "branch": claim.get("branch"), "slug": slug,
            "ack": registry.ack_path(main, unit), "rescue": "refs/rescue/%s" % naming.slug(unit),
            "outbox": outbox, "notes": notes}


# --- ranges, symbols, scores ------------------------------------------------------------------------------------

def splits_range(main: str, unit: str) -> dict:
    """The unit's section ranges from `splits.txt` as `{section: (start, end, size)}`, matched by stem."""
    path = os.path.join(main, "config", "RMHE08", "splits.txt")
    want = os.path.splitext(source_name(unit, main))[0]
    out = {}
    if not os.path.exists(path):
        return out
    for block in _project.Splits.cached(path).blocks:
        if os.path.splitext(block.unit.lstrip("/"))[0] == want:
            for r in block.ranges:
                out[r.section] = (r.start, r.end, r.size)
    return out


def registered_text_ranges(main: str) -> list[tuple[int, int, str]]:
    """Every registered unit's `.text` range as `(start, end, unit)`, sorted."""
    out = []
    for unit in registered_units(main):
        rng = splits_range(main, unit)
        if ".text" in rng:
            out.append((rng[".text"][0], rng[".text"][1], unit))
    return sorted(out)


_MAP_CACHE: dict = {}


def map_rows(main: str) -> list[dict] | None:
    """Every typed map row, parsed once per (path, mtime); None when the map is missing."""
    path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    if not os.path.exists(path):
        return None
    key = (path, os.path.getmtime(path))
    if key not in _MAP_CACHE:
        rows = [{"name": e.name, "section": e.section, "address": e.address, "size": e.size, "type": e.type}
                for e in _project.SymbolMap(path).rows() if e.type]
        _MAP_CACHE.clear()
        _MAP_CACHE[key] = rows
    return _MAP_CACHE[key]


def symbols_in_range(main: str, start: int, end: int) -> list[dict]:
    """The map rows in `[start, end)`; an empty result is warned about, never silent."""
    path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    if not os.path.exists(path):
        print("WARNING: no %s" % path, file=sys.stderr)
        return []
    rows = [dict(r) for r in map_rows(main) if start <= r["address"] < end]
    if not rows:
        print("WARNING: no symbols found in 0x%X-0x%X - check the split range" % (start, end), file=sys.stderr)
    return rows


_REPORT_CACHE: dict = {}


def report_scores(main: str, unit: str) -> dict:
    """`{symbol: percent}` for the unit from `report.json` (cached per (path, mtime))."""
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    key = (path, os.path.getmtime(path))
    if key not in _REPORT_CACHE:
        _REPORT_CACHE.clear()
        with open(path, encoding="utf-8") as fh:
            _REPORT_CACHE[key] = json.loads(fh.read())
    data = _REPORT_CACHE[key]
    want = source_name(unit.strip("/"))
    stem = os.path.splitext(want)[0]
    best = {}
    for u in data.get("units", []):
        name = (u.get("name") or "").replace("\\", "/")
        if not (name.endswith(want) or name.endswith(stem) or name.endswith("/" + stem)):
            continue
        for fn in u.get("functions") or []:
            pct = fn.get("fuzzy_match_percent", fn.get("match_percent"))
            if fn.get("name") and pct is not None:
                best[fn["name"]] = float(pct)
    return best


def header_comment(main: str, wt: str, unit: str) -> str:
    """The unit's file-header comment (the worktree's copy first)."""
    name = source_name(unit, wt, main)
    for root in (wt, main):
        path = os.path.join(root, "src", *name.split("/"))
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        m = re.match(r"\s*(/\*.*?\*/|//[^\n]*(?:\n//[^\n]*)*)", text, re.S)
        if m:
            return m.group(1).strip()
    return "(no source file yet)"


def flags_for(main: str, unit: str) -> tuple[list[str], str]:
    """The unit's real flags with their values (`lib.units.split_flags` of MAIN's ninja command)."""
    try:
        tokens = _units.ninja_command(main, unit)
    except (SystemExit, OSError) as exc:
        return [], str(exc)
    _head, flags, _tail = _units.split_flags(tokens)
    return flags, ""


def lib_for(main: str, unit: str) -> str:
    with open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    want = "\"%s\"" % source_name(unit, main)
    for block in re.finditer(r"\{\s*\n\s*\"lib\": \"([^\"]+)\"[^{}]*?\"objects\": \[(.*?)\]\s*,\s*\n\s*\}", text, re.S):
        if want in block.group(2):
            return block.group(1)
    return "(unknown)"


def shared_headers(main: str, wt: str, unit: str, symbol_names) -> list:
    """The shared `include/**` headers this unit should reuse (`typeregistry.relevant_headers`)."""
    tr = _typeregistry()
    try:
        reg = tr.registry(main)
    except OSError:
        return []
    name = source_name(unit, wt, main)
    rel = os.path.join("src", *name.split("/")).replace("\\", "/")
    text = ""
    for root in (wt, main):
        path = os.path.join(root, "src", *name.split("/"))
        if os.path.exists(path):
            with open(path, encoding="utf-8", errors="replace") as fh:
                text = fh.read()
            break
    return tr.relevant_headers(reg, text, symbol_names, unit_file=rel)


def _section_span(text: str, heading: str):
    """The span `heading` covers: literally first, otherwise by its title (the number may move)."""
    start = text.find(heading)
    if start >= 0:
        nxt = re.search(r"\n## ", text[start + len(heading):])
        return start, start + len(heading) + (nxt.start() if nxt else len(text))
    m = re.match(r"(#{2,4})\s+[0-9]+(?:\.[0-9]+)*\.?\s+(.*)$", heading.strip())
    if m is None:
        return None
    title = " ".join(m.group(2).split()).lower()
    for hit in re.finditer(r"(?m)^(#{2,4})\s+(?:[0-9]+(?:\.[0-9]+)*\.?\s+)?(.*?)\s*$", text):
        got = " ".join(hit.group(2).split()).lower()
        if got == title or got.startswith(title):
            nxt = re.search(r"(?m)^#{1,%d} " % len(hit.group(1)), text[hit.end():])
            return hit.start(), hit.end() + (nxt.start() if nxt else len(text))
    return None


def plan_section(main: str, heading: str) -> str:
    """One section of `docs/plan.md` or `docs/pipeline.md`, verbatim ("" when neither has it)."""
    for parts in RULE_DOCS:
        path = os.path.join(main, *parts)
        if not os.path.exists(path):
            continue
        with open(path, encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        span = _section_span(text, heading)
        if span:
            return text[span[0]:span[1]].strip()
    return ""


def data_queue_entries(main: str, unit: str) -> list[dict]:
    path = os.path.join(main, "tools", "units", "data-queue.json")
    if not os.path.exists(path):
        return []
    try:
        with open(path, encoding="utf-8") as fh:
            data = json.loads(fh.read())
    except json.JSONDecodeError:
        return []
    return [e for e in (data if isinstance(data, list) else data.get("entries", [])) if e.get("unit") == unit]


def build(main: str, wt: str, unit: str, task: str | None, assume_claim: bool = False) -> dict:
    """Everything the six parts render, for `unit` as seen from worktree `wt`."""
    unit = naming.norm_unit(unit.strip("/"))
    name = source_name(unit, wt, main)
    rng = splits_range(main, unit)
    text_range = rng.get(".text")
    syms = symbols_in_range(main, text_range[0], text_range[1]) if text_range else []
    scores = report_scores(main, unit)
    tokens, err = flags_for(main, unit)
    lib = lib_for(main, unit)
    obj = os.path.splitext(os.path.join(wt, "build", "RMHE08", "src", *name.split("/")))[0] + ".o"
    target = os.path.splitext(os.path.join(main, "build", "RMHE08", "obj", *name.split("/")))[0] + ".o"
    for sym in syms:
        sym["percent"] = scores.get(sym["name"])
    below = [s for s in syms if (s.get("percent") is None or s["percent"] < BAR)]
    handoff = handoff_paths(main, unit, assume_claim)
    return {
        "unit": unit, "slug": handoff["slug"], "claimed": handoff["claimed"],
        "branch": handoff["branch"], "handoff": handoff, "lib": lib, "worktree": wt, "main": main,
        "source": os.path.join(wt, "src", *name.split("/")),
        "object": obj, "target": target,
        "sections": rng, "flags": tokens, "flag_error": err,
        "language": _langcheck().object_verdict(target),
        "symbols": syms, "below_bar": len(below),
        "task": task, "task_symbols": [s["name"] for s in below] if not task else [],
        "header": header_comment(main, wt, unit),
        "data_queue": data_queue_entries(main, unit),
        "dossier": _dossier().build(main, unit, target, ranges=rng, map_symbols=syms),
        "shared_headers": shared_headers(main, wt, unit, [s["name"] for s in syms]),
    }
