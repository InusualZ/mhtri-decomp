#!/usr/bin/env python3
"""Replay MWCC's GPR colouring from a mwcc-debugger dump and search the declaration order retail's registers need.
Spec: docs/tools/spec/regsim.md. CLI: regsim.py verify DIR | want DIR -u UNIT | search DIR -u UNIT [--tryvar [--keep]]
[--apply-local [--dry-run]] [--max-evals N] [--top N] [--json] | run UNIT FUNCTION [--no-search] [--tryvar] [--apply-local]."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import json
import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

from tools.lib import localedit, objcompare, regsim, regsimrun, repo, units


def fail(msg: str) -> "SystemExit":
    return SystemExit("regsim: " + msg)


def reg(r: int | None) -> str:
    return "none" if r is None or r < 0 else "r%d" % r


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
        print("  %-22s %-8s scan %d degree %2d cost %4d  model %-4s dump %-4s %s"
              % (n.label(), n.tier, p.scan, p.degree, n.cost, reg(res.colour[p.vreg]), reg(n.phys),
                 "" if res.colour[p.vreg] == n.phys else "<- differs"), mark)
    if not args.all:
        print("  (volatile nodes coloured in order hidden; --all shows them)")
    if res.spilled:
        print("spilled (stack slots in this order): %s" % ", ".join(g.nodes[v].label() for v in res.spilled))
    if g.cost_source == "computed" and any(p.forced for p in res.pushes):
        print("note: the dump printed no spill costs; the stall picks used the costs computed from %s"
              % regsim.cost_input(args.dir, args.pass_no).name)
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


@dataclass
class Searched:
    """What one search found: the graph, the source it read (`path`, `text`, the `mapping` of vregs to its locals),
    the targets, the simulator (`s`), the current order's layout (`base`) and the ranked `plans`."""
    g: regsim.Graph
    path: str | None
    text: str | None
    params: int
    mapping: dict
    want: dict
    notes: list
    s: regsim.Searcher
    base: regsim.Plan
    plans: list


def search_result(args) -> Searched:
    """`search` without the printing (`cmd_search` and `tools/units/applyorder.py` both read it)."""
    g = load_graph(args)
    path, text, params, mapping = source_context(args, g)
    if params is None:
        raise fail("cannot tell the parameters from the locals: give --params N (or -u UNIT / --source FILE)")
    if args.want:
        want, notes = parse_want(args.want), []
    else:
        want, notes, _ = retail_want(args, g)
    want, dropped = simulated_targets(g, want)
    if dropped:
        notes.append("not a coloured node of pass %d's graph, not used as a target: %s (a later pass's temp: "
                     "--pass N searches that pass's graph)" % (args.pass_no, ", ".join("r%d" % v for v in dropped)))
    if args.saved_only:
        want = {v: r for v, r in want.items() if r >= regsim.LOWEST_SAVED}
    if not want:
        raise fail("no retail register to aim at")
    scope = {v: loc.scope for v, loc in mapping.items()}
    s = regsim.Searcher(g, want, params, max_evals=args.max_evals, seed=args.seed, scope=scope)
    base = s.evaluate(s.base)
    plans = s.run(edits=not args.no_edits, top=args.top)
    return Searched(g, path, text, params, mapping, want, notes, s, base, plans)


def cmd_search(args) -> int:
    found = search_result(args)
    g, path, text, params, mapping, want, notes, s, base, plans = (
        found.g, found.path, found.text, found.params, found.mapping, found.want, found.notes, found.s, found.base,
        found.plans)
    if getattr(args, "plans_json", None):
        Path(args.plans_json).write_text(json.dumps({"function": g.function, "params": params, "mapped": len(mapping),
                                                     "plans": [plan_json(g, p, mapping) for p in plans]}, indent=1),
                                         encoding="utf-8")
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
                print("  still differs:", ", ".join("%s r%s (retail r%d)" % (g.nodes[v].label(),
                                                                             p.colour.get(p.layout.merged.get(v, v)),
                                                                             want[v])
                                                   for v in missed[:8]))
            if p.stalled:
                print("  stalled:", ", ".join(g.nodes[v].label() for v in p.stalled))
        for note in notes:
            print("note:", note)
        if plans and not plans[0].saved_exact:
            print("\nno plan reproduces every callee-saved target")
    if getattr(args, "apply_local", False) and plans:
        return cmd_apply_local(args, g, plans[0], s.base, want, mapping, path, text)
    if getattr(args, "apply_order", False) and plans:
        return cmd_apply_order(args, g, plans, s.base, mapping, text)
    if args.tryvar and plans:
        return hand_to_tryvar(args, g, plans[0], s.base, mapping, path, text)
    if args.require_exact:
        return 0 if plans and plans[0].saved_exact else 1
    return 0 if plans else 1


def simulated_targets(g: regsim.Graph, want: dict[int, int]) -> tuple[dict[int, int], list[int]]:
    """(the targets on nodes the replay colours, the vregs dropped): retail's registers are read from the last pass's
    code, so a temp a later pass made (a spill reload) is not a node of an earlier pass's graph."""
    coloured = set(g.virtual())
    return {v: r for v, r in want.items() if v in coloured}, sorted(v for v in want if v not in coloured)


def plan_json(g, p: regsim.Plan, mapping) -> dict:
    return {"saved": list(p.saved), "volatile": list(p.volatile), "exact": p.exact, "distance": p.distance,
            "edits": list(p.layout.edits), "stalled": ["r%d" % v for v in p.stalled],
            "decl": [{"vreg": v, "name": g.nodes[v].name,
                      "line": mapping[v].line if v in mapping else None,
                      "scope": mapping[v].scope if v in mapping else None} for v in p.layout.decl]}


# --- run: the dump, the stand-in check, verify and search in one command ---------------------------------------------

def run_dir(root: str, unit: units.Unit, function: str) -> Path:
    """`.pi/tmp/regsim-run/<unit>/<function>/` of this tree."""
    return Path(root).joinpath(*regsimrun.RUN_DIR, re.sub(r"\W", "_", unit.key), re.sub(r"\W", "_", function)[:80])


def clean_run_dir(root: str, path: Path) -> None:
    """Empties a previous run's directory - only ever one under this tree's own `.pi/tmp/regsim-run/`."""
    base = Path(root).joinpath(*regsimrun.RUN_DIR).resolve()
    if base not in path.resolve().parents:
        raise fail("refusing to clear %s: not under %s" % (path, base))
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True)


def search_namespace(args, dump: Path, symbol: str, ours: str) -> argparse.Namespace:
    """The `search` arguments a `run` implies."""
    return argparse.Namespace(
        dir=str(dump), pass_no=args.pass_no, json=False, unit=args.unit, symbol=symbol, ours=ours, retail=None,
        source=None, function=None, params=args.params, want=args.want, saved_only=args.saved_only,
        no_edits=args.no_edits, max_evals=args.max_evals, top=args.top, seed=args.seed, require_exact=False,
        tryvar=args.tryvar, keep=args.keep, apply_local=args.apply_local, apply_order=args.apply_order,
        local_name=args.local_name, dry_run=args.dry_run, plans_json=args.plans_json)


def dump_function(args) -> tuple[Path, str, str]:
    """`run`'s first half: compile the unit, dump `args.function` with the debugger into this tree's
    `.pi/tmp/regsim-run/` and print the stand-in check -> (dump directory, the dumped symbol, our object for the search)."""
    root = repo.repo_root()
    unit = units.Unit.resolve(args.unit, root)
    comp, cmd, notes = regsimrun.unit_compile(unit.spelling, root, repo.main_checkout(root))
    gdb, how = regsimrun.locate_gdb(root, args.gdb)
    out = run_dir(root, unit, args.function)
    clean_run_dir(root, out)
    print("== run %s %s: compiler %s, gdb %s (%s)" % (unit.key, args.function, comp.version, gdb, how))
    for note in notes:
        print("[flags]", note)
    # the unit's own compile (what retail's compiler makes of the current source), then the stand-in's
    rc, log, real_obj, _ = regsimrun.compile_into(cmd, root, str(out / "real"))
    if rc != 0:
        print(log[-1200:])
        raise fail("the unit does not compile with its own compiler (%s)" % comp.version)
    stand_in = comp.version != regsimrun.STAND_IN
    compiler, flags, ours = None, None, real_obj
    if stand_in:
        compiler = regsimrun.stand_in_compiler(comp.compiler)
        if not os.path.exists(compiler):
            raise fail("the stand-in compiler %s is not installed" % compiler)
        rc, log, ours, dropped = regsimrun.compile_into(cmd, root, str(out / "standin"), compiler, retry_unknown=True)
        if rc != 0:
            print(log[-1200:])
            raise fail("the unit does not compile with the stand-in %s either" % regsimrun.STAND_IN)
        flags = [f for f in comp.flags if f not in dropped]
        print("stand-in: %s compiles the unit with its flags%s" % (
            regsimrun.STAND_IN, " minus the unknown option(s) " + " ".join(dropped) if dropped else ""))
    rc, text = regsimrun.run_dump(gdb, comp.tokens(str(out / "dbg-obj"), compiler, flags), args.function, str(out),
                                  root, args.size, args.timeout)
    symbol = regsimrun.dumped_symbol(out)
    if rc != 0 or symbol is None:
        print(text[-1500:])
        raise fail("the debugger produced no dump of %s (exit %d)" % (args.function, rc))
    print("dump: %s  (function %s)" % (out, symbol))
    if stand_in:
        c = regsimrun.compare_function(real_obj, ours, symbol)
        if c.notes:
            print("stand-in check: %s - cannot tell whether the dump is retail-compiler's" % "; ".join(c.notes))
        elif c.identical:
            print("stand-in check: IDENTICAL - %s is %d bytes, the same bytes and relocation targets with %s and %s: "
                  "trust the dump" % (symbol, c.real_size, comp.version, regsimrun.STAND_IN))
        else:
            why = ("sizes %s vs %s" % (c.real_size, c.other_size)) if c.real_size != c.other_size else (
                "first byte difference at +0x%X" % c.first_diff if c.first_diff is not None else "relocation targets")
            print("stand-in check: DIFFERS (%s) - %s compiles %s differently from %s; the plan is a hint only, "
                  "measure each change with the unit's own compiler" % (why, regsimrun.STAND_IN, symbol, comp.version))
    else:
        print("stand-in check: not needed, the unit's own compiler is %s" % regsimrun.STAND_IN)
    return out, symbol, ours


def applyorder_command(args, root: str | None = None) -> list[str]:
    """`tools/units/applyorder.py row|batch` with the options `run` was given (a process, never an import: a tool
    does not import a tool)."""
    cmd = [sys.executable, str(Path(root or repo.repo_root()) / "tools" / "units" / "applyorder.py")]
    cmd += ["batch", args.rows_file] if args.rows_file else ["row", args.unit, args.function]
    cmd += ["--plans", str(args.plans), "--rounds", str(args.rounds), "--top", str(args.top), "--max-evals",
            str(args.max_evals), "--pass", str(args.pass_no), "--seed", str(args.seed), "--timeout", str(args.timeout),
            "--workers", str(args.workers), "--row-timeout", str(args.row_timeout)]
    for flag, value in (("--gdb", args.gdb), ("--size", args.size), ("--params", args.params), ("--want", args.want)):
        if value is not None:
            cmd += [flag, str(value)]
    cmd += [f for f, on in (("--saved-only", args.saved_only), ("--no-edits", args.no_edits), ("--dry-run", args.dry_run),
                            ("--restore", args.restore), ("--no-refresh", args.no_refresh), ("--json", args.json)) if on]
    return cmd


def cmd_run(args) -> int:
    if args.rows_file or (args.apply_order and not args.no_search):
        return subprocess.call(applyorder_command(args), cwd=repo.repo_root())
    out, symbol, ours = dump_function(args)
    print()
    rc = cmd_verify(argparse.Namespace(dir=str(out), pass_no=args.pass_no, json=False, all=False))
    if args.no_search:
        return rc
    print()
    return cmd_search(search_namespace(args, out, symbol, ours))


# --- the hand-off to unittry -------------------------------------------------------------------------------------

def rewrite_runs(plan: regsim.Plan, base: regsim.Layout, mapping: dict, text: str,
                 extra: "dict[int, localedit.NewLocal] | None" = None) -> list[tuple[str, str]]:
    """[(old block, new block)] spelling `plan`'s order, one per scope whose order changes. The scope's declaration
    lines are slots: with no initialiser among them each slot takes the declaration the plan puts there; otherwise
    every plain declaration goes to the first slot in the plan's order and each initialiser stays on its own line as
    an assignment. `extra` holds the locals the plan adds (`localedit`): each is a declaration line of the scope's
    run, whose lines must then be consecutive. `SystemExit` when the plan needs an edit nobody wrote, a block move,
    or moves a `for` or `const` local."""
    extra = extra or {}
    if plan.layout.edits and not extra:
        raise fail("the plan needs an edit (%s): write it by hand, then measure" % plan.layout.edits[0])
    if any(v not in mapping and v not in extra for v in plan.layout.decl):
        raise fail("not every local maps to a declaration in the source")
    kept = [v for v in plan.layout.decl if v not in extra]
    if [mapping[v].scope for v in kept] != [mapping[v].scope for v in base.decl]:
        raise fail("the plan moves declarations between blocks: write it by hand, then measure")
    lines = text.split("\n")
    out = []
    scopes = {mapping[v].scope for v in kept} | {e.scope for e in extra.values()}
    for sc in sorted(scopes):
        old_order = [v for v in base.decl if mapping[v].scope == sc]
        new_order = [v for v in plan.layout.decl if (mapping[v].scope if v in mapping else extra[v].scope) == sc]
        if old_order == new_order:
            continue
        for v in old_order:
            stripped = lines[mapping[v].line - 1].strip()
            if stripped.startswith("for") or (mapping[v].init and stripped.startswith("const ")):
                raise fail("`%s` cannot move: write the order by hand" % stripped)
        slots = [mapping[v].line for v in old_order]
        if not slots:
            raise fail("the new local goes into a block with no declaration of its own: write it by hand")
        first, last = min(slots), max(slots)
        indent = lines[first - 1][:len(lines[first - 1]) - len(lines[first - 1].lstrip())]
        if any(v in extra for v in new_order) and sorted(slots) != list(range(first, last + 1)):
            raise fail("the declarations of the block are not consecutive lines: write the new local by hand")

        def text_of(v: int) -> str:
            return indent + extra[v].line if v in extra else lines[mapping[v].line - 1]

        new = {n: lines[n - 1] for n in range(first, last + 1)}
        if all(not mapping[v].init for v in old_order):
            if any(v in extra for v in new_order):
                new = {n: None for n in range(first, last + 1)}
                new[first] = "\n".join(text_of(v) for v in new_order)
            else:
                for slot, v in zip(slots, new_order):
                    new[slot] = lines[mapping[v].line - 1]
        else:
            plain = []
            for v in new_order:
                line = text_of(v).strip()
                plain.append(indent + (line[:line.index("=")].rstrip() + ";" if v in mapping and mapping[v].init else line))
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
    return run_unittry(args, g, rewrite_runs(plan, base, mapping, text), g.function or "order", not args.keep)


def run_unittry(args, g, runs, label: str, restore: bool) -> int:
    """Writes `runs` as one unittry trial under `.pi/tmp/regsim/` and runs it on the unit; unittry keeps the
    rewrite only when it wins (`restore`: puts the source back whatever the score)."""
    return run_trials(args, g, [(label, runs)], restore)


def run_trials(args, g, trials: list[tuple[str, list[tuple[str, str]]]], restore: bool) -> int:
    """Writes each `(label, [(old, new)])` as one unittry trial of one spec under `.pi/tmp/regsim/` and runs the spec on
    the unit: each trial is applied to the state the earlier ones kept and stays only when it wins."""
    scratch = Path(repo.repo_root()) / ".pi" / "tmp" / "regsim"
    scratch.mkdir(parents=True, exist_ok=True)
    spec = scratch / ("%s.spec" % re.sub(r"\W", "_", g.function or "function"))
    body = []
    for label, runs in trials:
        body.append("//@@ trial regsim-%s" % label)
        for old, new in runs:
            body += ["//@@ replace", old, "//@@ with", new]
    spec.write_text("\n".join(body) + "\n", encoding="utf-8", newline="\n")
    cmd = [sys.executable, str(Path(repo.repo_root()) / "tools" / "units" / "unittry.py"), args.unit, "--spec",
           str(spec)] + (["--restore"] if restore else [])
    print("\ntryvar:", " ".join(cmd), flush=True)
    return subprocess.call(cmd, cwd=repo.repo_root())


def unmet_temps(g, plan, want: dict) -> list[int]:
    """The temps `plan` leaves in a callee-saved register where retail has a volatile one (a value held across a call
    that retail computes after it), ascending."""
    out = []
    for v, r in sorted(want.items()):
        got = plan.colour.get(plan.layout.merged.get(v, v))
        if got is not None and got >= regsim.LOWEST_SAVED > r and g.nodes[v].tier != "named":
            out.append(v)
    return out


def cmd_apply_local(args, g, plan, base, want, mapping, path, text) -> int:
    """`search --apply-local`: write the local the best plan asks for (for a declared temp by what defines it: a walking
    pointer for `addi`, a constant for `li`, a call result for `mr T,r3`, a sum for `add`; a hoisted call result for a
    value held across a call), measure it with unittry and keep it only when it wins; a plan it cannot write
    unambiguously is refused with the edit to make by hand (exit 3, nothing written)."""
    if not args.unit or text is None:
        raise fail("--apply-local needs -u UNIT")
    root = repo.repo_root()
    fn = args.function or g.function
    ours, _retail = objects(args)
    symbol = args.symbol or g.function
    pre = regsim.regalloc_input(Path(args.dir))
    instrs = localedit.flatten(pre.read_text(encoding="utf-8", errors="replace")) if pre else []
    src_root = os.path.join(root, "src")
    edit = localedit.parse_edit(plan.layout.edits)
    try:
        if edit and edit.kind == "fold":
            localedit.fold(g, edit)
        if edit:
            change = localedit.declare(g, edit, instrs, text, fn, path, src_root, mapping, plan.layout.decl, ours,
                                       symbol, args.local_name)
            runs = change.blocks + rewrite_runs(plan, base, mapping, text, {edit.vreg: change.new_local})
        else:
            if plan.exact:
                print("\napply-local: the best plan is exact with no edit - nothing to write")
                return 0
            temps = unmet_temps(g, plan, want)
            if not temps:
                raise localedit.Refused("the best plan needs no edit and leaves no temp in a callee-saved register that "
                                        "retail keeps volatile; the remaining difference is not a local's")
            refusals = []
            for v in temps:
                try:
                    change = localedit.hoist(g, v, instrs, ours, symbol, text, fn, path, src_root, args.local_name)
                    break
                except localedit.Refused as r:
                    refusals.append(r)
            else:
                raise refusals[0]
            runs = (rewrite_runs(plan, base, mapping, text) if plan.distance else []) + change.blocks
    except localedit.Refused as r:
        print("\napply-local: REFUSED - %s" % r.reason)
        if r.manual:
            print("  by hand: %s" % r.manual)
        return 3
    print("\napply-local: %s" % change.summary)
    for old, new in runs:
        print("  --- old\n%s\n  +++ new\n%s" % ("\n".join("  - " + x for x in old.split("\n")),
                                                "\n".join("  + " + x for x in new.split("\n"))))
    if args.dry_run:
        print("apply-local: --dry-run, nothing written or measured")
        return 0
    return run_unittry(args, g, runs, "%s-local" % (g.function or "function"), restore=False)


def changed_block(old: str, new: str) -> tuple[str, str]:
    """`(old lines, new lines)` of the smallest line range that differs, with one line of context so it is unique."""
    a, b = old.split("\n"), new.split("\n")
    head = 0
    while head < min(len(a), len(b)) and a[head] == b[head]:
        head += 1
    tail = 0
    while tail < min(len(a), len(b)) - head and a[len(a) - 1 - tail] == b[len(b) - 1 - tail]:
        tail += 1
    lo = max(0, head - 1)
    return "\n".join(a[lo:len(a) - tail]), "\n".join(b[lo:len(b) - tail])


def cmd_apply_order(args, g, plans, base, mapping, text) -> int:
    """`search --apply-order`: every spelling of the plans' declaration orders (`lib/regsimorder`: per-scope reorder,
    function-scope hoist, hoist with the `for` variables) as one unittry trial each, kept only when they win; refused with
    the reasons when no plan can be written (exit 3, nothing written). One round: `run --apply-order` re-searches."""
    from tools.lib import regsimorder
    if not args.unit or text is None:
        raise fail("--apply-order needs -u UNIT")
    fn = args.function or g.function
    spells, reasons = [], []
    for n, plan in enumerate(plans, 1):
        if plan.distance == 0:
            continue
        got, why = regsimorder.spellings(plan.layout.decl, mapping, text, fn, plan.layout.edits)
        spells += [(n, s) for s in got if all(s.text != t.text for _m, t in spells)]
        reasons += ["plan %d %s: %s" % (n, label, exc.reason) for label, exc in why]
    if not spells:
        print("\napply-order: REFUSED - no plan can be written (%s)"
              % ("; ".join(reasons) or "every plan is the source's own order"))
        return 3
    trials = []
    for n, s in spells:
        old, new = changed_block(text, s.text)
        print("\napply-order: plan %d %s: %s" % (n, s.label, s.summary))
        print("  --- old\n%s\n  +++ new\n%s" % ("\n".join("  - " + x for x in old.split("\n")),
                                                "\n".join("  + " + x for x in new.split("\n"))))
        trials.append(("%s-plan%d-%s" % (g.function or "function", n, s.label), [(old, new)]))
    if args.dry_run:
        print("apply-order: --dry-run, nothing written or measured")
        return 0
    return run_trials(args, g, trials, restore=False)


def run_options(run) -> None:
    """The options of `run` (and of `tools/units/applyorder.py`'s `row` and `batch`) but the unit and the function."""
    run.add_argument("--search", action="store_true", help="search the plan (the default; kept for the spelling)")
    run.add_argument("--no-search", action="store_true",
                     help="stop after the dump and `verify`: prints no plans (and so ignores --tryvar, --apply-local "
                          "and --apply-order)")
    run.add_argument("--gdb", help="the gdb to use (default: this tree's .pi/tmp/mwcc-dbg, PATH, else fetched)")
    run.add_argument("--size", help="the symbols.txt size that picks one of several functions of a name")
    run.add_argument("--timeout", type=float, default=900.0, help="seconds the debugger session may take")
    run.add_argument("--pass", dest="pass_no", type=int, default=1, help="the colouring pass (default 1)")
    run.add_argument("--params", type=int, help="how many parameters (default: read from the source)")
    run.add_argument("--want", help="targets by hand, rVREG=rPHYS,... (instead of retail's object)")
    run.add_argument("--saved-only", action="store_true", help="aim at the callee-saved registers only")
    run.add_argument("--no-edits", action="store_true", help="declaration orders only, no source edits")
    run.add_argument("--max-evals", type=int, default=20000, help="simulations per order search")
    run.add_argument("--top", type=int, default=3, help="plans to print")
    run.add_argument("--seed", type=int, default=0)
    run.add_argument("--tryvar", action="store_true", help="measure the best plan with tools/units/unittry.py")
    run.add_argument("--keep", action="store_true", help="with --tryvar: keep the rewrite when it scores better")
    run.add_argument("--apply-local", action="store_true",
                     help="write the best plan's named local into the source, measure it, keep it only if it wins")
    run.add_argument("--apply-order", action="store_true",
                     help="write the plans' declaration orders (function and block scope, `tools/units/applyorder.py`), "
                          "keep each only on a strict gain, round after round; with --rows-file: every row of the list")
    run.add_argument("--local-name", help="with --apply-local: the name of the new local")
    run.add_argument("--dry-run", action="store_true",
                     help="with --apply-local/--apply-order: print the edit, write nothing")
    run.add_argument("--plans-json", metavar="FILE", help="also write the plans (declarations with line and scope) as JSON")
    run.add_argument("--plans", type=int, default=3, help="--apply-order: write the best N plans of each round (default 3)")
    run.add_argument("--rounds", type=int, default=4,
                     help="--apply-order: dump, search and write again until a round keeps nothing, at most N (default 4)")
    run.add_argument("--restore", action="store_true", help="--apply-order: measure, then put the source back as found")
    run.add_argument("--no-refresh", action="store_true",
                     help="--apply-order: do not rebuild the object and report.json after a keep")
    run.add_argument("--rows-file", metavar="F",
                     help="--apply-order over a row list: lines `size,unit,function,pct` (a function may contain commas)")
    run.add_argument("--workers", type=int, default=3, help="--rows-file: unit-disjoint workers (default 3)")
    run.add_argument("--row-timeout", type=float, default=3600.0, help="--rows-file: seconds a row may take")
    run.add_argument("--json", action="store_true", help="--apply-order: a final `RESULT {...}` line")


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
            p.add_argument("--require-exact", action="store_true",
                           help="exit 1 unless the best plan reproduces every callee-saved target")
            p.add_argument("--tryvar", action="store_true", help="measure the best plan with tools/units/unittry.py")
            p.add_argument("--keep", action="store_true", help="with --tryvar: keep the rewrite when it scores better")
            p.add_argument("--apply-local", action="store_true",
                           help="write the best plan's named local into the source, measure it, keep it only if it wins")
            p.add_argument("--apply-order", action="store_true",
                           help="write the plans' declaration orders (per scope, hoisted, with `for` variables), "
                                "measure each, keep one only if it wins (one round; `run --apply-order` iterates)")
            p.add_argument("--local-name", help="with --apply-local: the name of the new local")
            p.add_argument("--dry-run", action="store_true",
                           help="with --apply-local/--apply-order: print the edit, write nothing")
            p.add_argument("--plans-json", metavar="FILE", help="also write the plans (declarations with line and scope) as JSON")
    run = sub.add_parser("run", help="dump one function of a unit, verify the replay and search, in one command")
    run.add_argument("unit", nargs="?", help="the unit (`fn_8004CAD8`, `homebutton/tiHKBManager`, a path)")
    run.add_argument("function", nargs="?",
                     help="a plain name, a mangled name, Class::method or an address (0x8004DA5C)")
    run_options(run)
    args = ap.parse_args(argv)
    if args.cmd == "run":
        if not args.rows_file and not (args.unit and args.function):
            ap.error("run needs UNIT FUNCTION (or --rows-file F)")
        return cmd_run(args)
    return {"verify": cmd_verify, "want": cmd_want, "search": cmd_search}[args.cmd](args)


if __name__ == "__main__":
    raise SystemExit(main())
