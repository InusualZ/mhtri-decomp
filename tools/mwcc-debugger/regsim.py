#!/usr/bin/env python3
"""Replay MWCC's GPR colouring from a mwcc-debugger dump and search the declaration order retail's registers need.
Spec: docs/tools/spec/regsim.md. CLI: regsim.py verify DIR | want DIR -u UNIT | search DIR -u UNIT [--params N]
[--want rV=rP,..] [--saved-only] [--no-edits] [--max-evals N] [--top N] [--tryvar [--keep]] [--json]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import subprocess
from pathlib import Path

from tools.lib import objcompare, regsim, repo, units


def fail(msg: str) -> "SystemExit":
    return SystemExit("regsim: " + msg)


def load_graph(args) -> regsim.Graph:
    try:
        return regsim.load(args.dir, args.pass_no)
    except FileNotFoundError as exc:
        raise fail(str(exc)) from None


# --- verify -----------------------------------------------------------------------------------------------------

def cmd_verify(args) -> int:
    g = load_graph(args)
    res = regsim.simulate(g)
    div = regsim.verify(g, res)
    if args.json:
        print(json.dumps({"function": g.function, "nodes": len(g.virtual()), "exact": not div,
                          "divergences": [d.__dict__ for d in div],
                          "select": [{"vreg": p.vreg, "name": g.nodes[p.vreg].name, "scan": p.scan,
                                      "degree": p.degree, "stalled": p.stalled, "forced": p.forced,
                                      "model": res.colour[p.vreg], "dump": g.nodes[p.vreg].phys}
                                     for p in reversed(res.pushes)]}, indent=1))
        return 1 if div else 0
    print("== %s: %d nodes, select order (first coloured first)" % (g.function or args.dir, len(g.virtual())))
    for p in reversed(res.pushes):
        n = g.nodes[p.vreg]
        if not args.all and res.colour[p.vreg] < regsim.LOWEST_SAVED and not p.stalled:
            continue
        mark = "FORCED" if p.forced else ("stalled" if p.stalled else "")
        dump = "r%d" % n.phys
        print("  %-22s %-8s scan %d degree %2d  model r%-2d dump %-4s %s"
              % (n.label(), n.tier, p.scan, p.degree, res.colour[p.vreg], dump,
                 "" if res.colour[p.vreg] == n.phys else "<- differs"), mark)
    if not args.all:
        print("  (volatile nodes coloured in order hidden; --all shows them)")
    if div:
        for d in div:
            print("DIVERGES %s r%d: model %s, dump %s" % (d.kind, d.vreg, d.model, d.dump))
        return 1
    print("EXACT: every register and the select order reproduce the dump")
    return 0


# --- retail's registers ------------------------------------------------------------------------------------------

def function_bytes(obj: str, symbol: str) -> bytes:
    syms = objcompare.section_symbols(obj, ".text")
    if symbol not in syms:
        raise fail("%s is not in %s" % (symbol, obj))
    data = objcompare.section_data(obj, ".text") or b""
    off, size = syms[symbol]
    return data[off:off + size]


def retail_want(args, g: regsim.Graph) -> tuple[dict[int, int], list[str], dict]:
    """{vreg: retail register}, notes, and the counts behind them."""
    d = Path(args.dir)
    pre, post, final = regsim.regalloc_input(d), regsim.after_regalloc(d), regsim.final_dump(d)
    if not (pre and post and final):
        raise fail("%s lacks the backend dumps around the colouring (backend-*-after-regalloc.txt)" % d)
    per = regsim.operand_vregs(pre.read_text(encoding="utf-8", errors="replace"),
                               post.read_text(encoding="utf-8", errors="replace"),
                               final.read_text(encoding="utf-8", errors="replace"))
    ours_obj, retail_obj = objects(args)
    symbol = args.symbol or g.function
    ours, retail = function_bytes(ours_obj, symbol), function_bytes(retail_obj, symbol)
    notes = []
    if len(ours) != 4 * len(per):
        notes.append("our object's %s is %d instructions, the dump's final code %d: rebuild the object from the "
                     "source the dump was taken from" % (symbol, len(ours) // 4, len(per)))
    if len(ours) != len(retail):
        notes.append("%s is %d bytes, retail %d: only the aligned prefix pairs registers" % (symbol, len(ours),
                                                                                             len(retail)))
    pairs, used = regsim.vreg_pairs(ours, retail, per)
    want, more = regsim.want_from_vregs(pairs)
    return want, notes + more, {"instructions": len(per), "paired": used}


def objects(args) -> tuple[str, str]:
    if args.ours and args.retail:
        return args.ours, args.retail
    if not args.unit:
        raise fail("give -u UNIT (or --ours OBJ --retail OBJ) to read retail's registers")
    u = units.Unit.resolve(args.unit, repo.repo_root())
    return args.ours or u.obj_ours, args.retail or u.obj_target


def parse_want(text: str) -> dict[int, int]:
    out = {}
    for item in filter(None, (t.strip() for t in text.split(","))):
        m = re.fullmatch(r"r?(\d+)=r?(\d+)", item)
        if not m:
            raise fail("--want takes rVREG=rPHYS,...: %r" % item)
        out[int(m.group(1))] = int(m.group(2))
    return out


def cmd_want(args) -> int:
    g = load_graph(args)
    want, notes, stats = retail_want(args, g)
    if args.json:
        print(json.dumps({"function": g.function, "want": {"r%d" % v: "r%d" % r for v, r in want.items()},
                          "notes": notes, **stats}, indent=1))
        return 0
    print("== %s: %d of %d final instructions paired with retail" % (g.function, stats["paired"],
                                                                    stats["instructions"]))
    for v, r in sorted(want.items()):
        n = g.nodes.get(v)
        ours = n.phys if n else -1
        print("  %-22s ours r%-2d retail r%-2d %s" % (n.label() if n else "r%d" % v, ours, r,
                                                     "" if ours == r else "<- differs"))
    for note in notes:
        print("note:", note)
    return 0


# --- search -----------------------------------------------------------------------------------------------------

def source_context(args, g: regsim.Graph):
    """(source path, its text, parameter count, {vreg: SourceLocal}) - what the source can tell; Nones without it."""
    path = None
    if args.source:
        path = args.source
    elif args.unit:
        path = units.Unit.resolve(args.unit, repo.repo_root()).source
    text = Path(path).read_text(encoding="utf-8") if path and os.path.exists(path) else None
    fn = args.function or g.function
    params = args.params
    mapping: dict = {}
    if text is not None:
        decl = regsim.find_definition(text, fn)
        if decl is not None and params is None:
            params = regsim.param_count(decl) + (1 if g.nodes.get(regsim.FIRST_VIRTUAL, regsim.Node(0, 0)).name
                                                 == "this" else 0)
        if params is not None:
            mapping = regsim.map_locals(g, params, regsim.source_locals(text, fn))
    return path, text, params, mapping


def describe(g: regsim.Graph, v: int, mapping: dict) -> str:
    loc = mapping.get(v)
    if loc is None:
        seen = g.defs.get(v)
        return g.nodes[v].label() + ("  first seen: %s" % seen if seen else "")
    where = "function scope" if loc.scope == 0 else "block %d" % loc.scope
    return "%-10s line %d, %s  (%s)" % (loc.name, loc.line, where, g.nodes[v].label())


def cmd_search(args) -> int:
    g = load_graph(args)
    path, text, params, mapping = source_context(args, g)
    if params is None:
        raise fail("cannot tell the parameters from the locals: give --params N (or -u UNIT / --source FILE)")
    if args.want:
        want, notes = parse_want(args.want), []
    else:
        want, notes, _ = retail_want(args, g)
    if args.saved_only:
        want = {v: r for v, r in want.items() if r >= regsim.LOWEST_SAVED}
    if not want:
        raise fail("no retail register to aim at")
    scope = {v: loc.scope for v, loc in mapping.items()}
    s = regsim.Searcher(g, want, params, max_evals=args.max_evals, seed=args.seed, scope=scope)
    base = s.evaluate(s.base)
    plans = s.run(edits=not args.no_edits, top=args.top)
    if args.json:
        print(json.dumps({"function": g.function, "params": params, "evals": s.evals,
                          "current": plan_json(g, base, mapping), "plans": [plan_json(g, p, mapping) for p in plans],
                          "notes": notes}, indent=1))
    else:
        print("== %s: %d parameters, %d locals, %d targets (%d callee-saved); %d simulations"
              % (g.function, params, len(s.base.decl), len(want), base.saved[1], s.evals))
        print("current order: callee-saved %d/%d, volatile %d/%d; stalled: %s"
              % (base.saved + base.volatile + (", ".join(g.nodes[v].label() for v in base.stalled) or "none",)))
        for i, p in enumerate(plans, 1):
            print("\nplan %d: callee-saved %d/%d, volatile %d/%d, rewrite cost %d%s"
                  % ((i,) + p.saved + p.volatile + (p.distance, "  EXACT" if p.exact else "")))
            for e in p.layout.edits:
                print("  edit:", e)
                for v in map(int, re.findall(r"r(\d+)\(", e)):
                    if v in g.defs:
                        print("        r%d first seen: %s" % (v, g.defs[v]))
            if p.layout.decl:
                print("  declare in this order (first declared = highest number):")
                for k, v in enumerate(p.layout.decl, 1):
                    print("    %2d. %s" % (k, describe(g, v, mapping)))
            missed = [v for v, r in want.items() if p.colour.get(p.layout.merged.get(v, v)) != r]
            if missed:
                print("  still differs:", ", ".join("%s r%s (retail r%d)" % (g.nodes[v].label(), p.colour.get(v), want[v])
                                                   for v in missed[:8]))
            if p.stalled:
                print("  stalled:", ", ".join(g.nodes[v].label() for v in p.stalled))
        for note in notes:
            print("note:", note)
    if args.tryvar and plans:
        return hand_to_tryvar(args, g, plans[0], s.base, mapping, path, text)
    return 0 if plans and plans[0].saved_exact else 1


def plan_json(g, p: regsim.Plan, mapping) -> dict:
    return {"saved": list(p.saved), "volatile": list(p.volatile), "exact": p.exact, "distance": p.distance,
            "edits": list(p.layout.edits), "stalled": ["r%d" % v for v in p.stalled],
            "decl": [{"vreg": v, "name": g.nodes[v].name,
                      "line": mapping[v].line if v in mapping else None,
                      "scope": mapping[v].scope if v in mapping else None} for v in p.layout.decl]}


# --- the hand-off to unittry -------------------------------------------------------------------------------------

def rewrite_runs(plan: regsim.Plan, base: regsim.Layout, mapping: dict, text: str) -> list[tuple[str, str]]:
    """[(old block, new block)] spelling `plan`'s order, one per scope whose order changes. The scope's declaration
    lines are slots: with no initialiser among them each slot takes the declaration the plan puts there; otherwise
    every plain declaration goes to the first slot in the plan's order and each initialiser stays on its own line as
    an assignment. `SystemExit` when the plan needs an edit or a block move, or moves a `for` or `const` local."""
    if plan.layout.edits:
        raise fail("the plan needs an edit (%s): write it by hand, then measure" % plan.layout.edits[0])
    if any(v not in mapping for v in plan.layout.decl):
        raise fail("not every local maps to a declaration in the source")
    if [mapping[v].scope for v in plan.layout.decl] != [mapping[v].scope for v in base.decl]:
        raise fail("the plan moves declarations between blocks: write it by hand, then measure")
    lines = text.split("\n")
    out = []
    for sc in sorted({mapping[v].scope for v in plan.layout.decl}):
        old_order = [v for v in base.decl if mapping[v].scope == sc]
        new_order = [v for v in plan.layout.decl if mapping[v].scope == sc]
        if old_order == new_order:
            continue
        for v in old_order:
            stripped = lines[mapping[v].line - 1].strip()
            if stripped.startswith("for") or (mapping[v].init and stripped.startswith("const ")):
                raise fail("`%s` cannot move: write the order by hand" % stripped)
        slots = [mapping[v].line for v in old_order]
        first, last = min(slots), max(slots)
        new = {n: lines[n - 1] for n in range(first, last + 1)}
        indent = lines[first - 1][:len(lines[first - 1]) - len(lines[first - 1].lstrip())]
        if all(not mapping[v].init for v in old_order):
            for slot, v in zip(slots, new_order):
                new[slot] = lines[mapping[v].line - 1]
        else:
            plain = []
            for v in new_order:
                line = lines[mapping[v].line - 1].strip()
                plain.append(indent + (line[:line.index("=")].rstrip() + ";" if mapping[v].init else line))
            for v in old_order:
                new[mapping[v].line] = ("%s%s = %s;" % (indent, mapping[v].name, mapping[v].init)
                                        if mapping[v].init else None)
            new[first] = "\n".join(plain + ([new[first]] if new[first] is not None else []))
        old_block = "\n".join(lines[first - 1:last])
        new_block = "\n".join(new[n] for n in range(first, last + 1) if new[n] is not None)
        out.append((old_block, new_block))
    return out


def hand_to_tryvar(args, g, plan, base, mapping, path, text) -> int:
    if not args.unit or text is None:
        raise fail("--tryvar needs -u UNIT")
    if plan.distance == 0:
        print("\ntryvar: the best plan is the source as it stands - nothing to measure")
        return 0
    runs = rewrite_runs(plan, base, mapping, text)
    scratch = Path(repo.repo_root()) / ".pi" / "tmp" / "regsim"
    scratch.mkdir(parents=True, exist_ok=True)
    spec = scratch / ("%s.spec" % re.sub(r"\W", "_", g.function or "function"))
    body = ["//@@ trial regsim-%s" % (g.function or "order")]
    for old, new in runs:
        body += ["//@@ replace", old, "//@@ with", new]
    spec.write_text("\n".join(body) + "\n", encoding="utf-8", newline="\n")
    cmd = [sys.executable, str(Path(repo.repo_root()) / "tools" / "units" / "unittry.py"), args.unit, "--spec",
           str(spec)] + ([] if args.keep else ["--restore"])
    print("\ntryvar:", " ".join(cmd), flush=True)
    return subprocess.call(cmd, cwd=repo.repo_root())


# --- the command line -------------------------------------------------------------------------------------------

def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("verify", "want", "search"):
        p = sub.add_parser(name)
        p.add_argument("dir", help="a mwcc_debugger.py output directory")
        p.add_argument("--pass", dest="pass_no", type=int, default=1, help="the colouring pass (default 1)")
        p.add_argument("--json", action="store_true")
        if name == "verify":
            p.add_argument("--all", action="store_true", help="list the volatile nodes too")
            continue
        p.add_argument("-u", "--unit", help="the unit: its objects give retail's registers, its source the locals")
        p.add_argument("--symbol", help="the function's symbol in the objects (default: the dump's function)")
        p.add_argument("--ours", help="our object (default: the unit's build/.../src object)")
        p.add_argument("--retail", help="retail's object (default: the unit's build/.../obj object)")
        if name == "search":
            p.add_argument("--source", help="the source file (default: the unit's)")
            p.add_argument("--function", help="the function's name in the source (default: the dump's)")
            p.add_argument("--params", type=int, help="how many parameters (default: read from the source)")
            p.add_argument("--want", help="targets by hand, rVREG=rPHYS,... (instead of retail's object)")
            p.add_argument("--saved-only", action="store_true", help="aim at the callee-saved registers only")
            p.add_argument("--no-edits", action="store_true", help="declaration orders only, no source edits")
            p.add_argument("--max-evals", type=int, default=20000, help="simulations per order search")
            p.add_argument("--top", type=int, default=3, help="plans to print")
            p.add_argument("--seed", type=int, default=0)
            p.add_argument("--tryvar", action="store_true", help="measure the best plan with tools/units/unittry.py")
            p.add_argument("--keep", action="store_true", help="with --tryvar: keep the rewrite when it scores better")
    args = ap.parse_args(argv)
    return {"verify": cmd_verify, "want": cmd_want, "search": cmd_search}[args.cmd](args)


if __name__ == "__main__":
    raise SystemExit(main())
