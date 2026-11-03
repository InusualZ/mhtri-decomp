# `lib/regsim` - MWCC Wii/1.3 GPR colouring: the dump, the replay, the renumbering search

## Purpose

Reads the register-allocation dumps `tools/mwcc-debugger/mwcc_debugger.py` writes, replays the compiler's GPR
simplify/select on the dumped interference graph, and searches renumberings of the function's virtual registers (the
declaration order, plus one source edit) whose replay gives retail's registers. The model is playbook ideas 243/244.

## Users

`tools/mwcc-debugger/regsim.py` (the CLI, `mt.py regsim`).

## Public API

* The dump: `load(dir, pass_no=1, cls="gpr") -> Graph` (`nodes {vreg: Node}`, `assigned` (the `-assigned.txt` order),
  `function`, `defs {vreg: first PCode line}`), `parse_nodes`, `parse_assigned`, `parse_defs`, `parse_blocks`;
  `regalloc_input(dir)` (the backend dump the last pass's graph was built from), `after_regalloc(dir)`,
  `final_dump(dir)`; `spill_costs(pcode) -> {vreg: cost}`, `cost_input(dir, pass_no)`, `fill_costs(graph, costs)`,
  `instruction_refs(mnemonic, operands) -> [(reg, defined, used)]`.
  `Node.tier` is `named` (a source variable), `frontend` (`@NNNN`) or `backend` (unnamed); `Node.phys` is -1 for
  `-> none` (a coalesced node). `Graph.virtual()` is every non-coalesced vreg; `Graph.coalesce(gone, into)` a copy
  with one node folded into another.
* The replay: `simulate(graph, order=None, k=29) -> Colouring(pushes, colour)`; `Push(vreg, scan, degree, stalled,
  forced)`; `verify(graph, colouring) -> [Divergence(kind, vreg, model, dump)]` (every register, then the first
  select-order difference).
* Retail's registers: `operand_vregs(before, after, final)` (per final instruction, `{physical GPR: vreg}`),
  `gpr_fields(word)`, `vreg_pairs(ours, retail, per_insn) -> ({vreg: {retail GPR: count}}, paired)`,
  `want_from_vregs(pairs) -> ({vreg: retail GPR}, notes)`.
* The source: `find_definition(text, function)`, `param_count(decl)`, `source_locals(text, function) ->
  [SourceLocal(name, line, scope, text, init)]`, `map_locals(graph, params, locals) -> {vreg: SourceLocal}`,
  `scope_breaks(decl, scope)`.
* The search: `tiers(graph, params) -> (params, decl, rest)`, `Searcher(graph, want, params, max_evals, seed,
  scope).run(edits=True, top=5) -> [Plan]`, `Plan(layout, saved, volatile, colour, stalled, distance)` with
  `exact`/`saved_exact`/`rank`; `Layout(params, decl, rest, edits, graph, merged).scan()`; `inversions`, `best`.

## Invariants and rules

* **The model** (measured on 9 dumps - 5 functions of `WPAD/wpad` and `ef/ef_drawsmoothstripestrategy`, 4 of them also in
  an earlier or edited source - 555 nodes, every register and the select order exact): simplify scans the vregs in
  ascending number with K = 29 (r0, r3-r12, r14-r31) and pushes each node whose current degree, physical neighbours
  included, is below K; a node not pushed in the first scan is **stalled** and is coloured before every node pushed
  earlier; a scan with no progress pushes the remaining node with the lowest cost / current degree, a tie to the latest
  in the scan (the highest vreg) (**forced**). Select pops in reverse: r0, r3..r12 first free, else the lowest
  callee-saved register handed out so far that no coloured neighbour holds, else the next new one down from r31; below
  r14 the node is spilled (-1, `Colouring.spilled`, ascending: the stack-slot order) and blocks no neighbour. The stall
  rule replays 24 GPR passes of 15 functions exactly (`menu/menu_infomation` ammo_grid_draw x2 with 3 passes each,
  ammo_table_draw x2, skill_table_body_draw; 13 `enemy/em_common` functions, em_target_select with 2 passes), 9 of them
  with stalls (8 replayed before the rule; ties to the lowest vreg replay 18 of 24).
* **Numbering**: parameters take the first vregs; the named locals follow in reverse textual declaration order (the
  first declared, inner blocks included, is the highest); then the frontend's `@NNNN` temporaries (a split web, an
  inlined function's local), then backend temporaries. A declaration whose address is taken has no node.
* **Retail's registers are attributed per vreg, never per physical register**: the colouring's input and output dumps
  have the same instructions, so each operand of the output dump names its vreg; a final-dump instruction inherits the
  mapping from the identical instruction of its block; each GPR field of our code word is then paired with the same
  field of retail's word, and only words equal outside their GPR fields are read. A vreg retail spells two ways is a
  note, never a target.
* **The spill cost** is the dump's `cost:` when the pass stalled; MWCC computes it lazily, so a pass that never stalled
  prints 0 and `load` fills it from the pass's PCode (`spill_costs`, `Graph.cost_source == "computed"`): every
  reference weighs its instruction's loop weight (the dump's first column, 1/8/64), a def once and a use twice; a web
  whose single def is `li`, `lis` or an `addi` of a fixup (a constant or an address) costs its use weights less its def
  weight. Pass N's PCode is the Nth of the last N `after-interference-graph` dumps (`cost_input`; one dump, and the FPR
  pass's, precede them). Measured: the computed cost equals the dump's on every node of the 9 stalled passes (2310
  nodes), and a replay on computed costs is exact on all 9.
* **The search** is a ranking, not a proof: the declaration order is searched exhaustively up to 8 locals and by a
  restarted insertion hill climb above; when no order is exact, each single edit (a node declared as a local at each
  position, a backend temp folded into a local it does not interfere with) is tried and the most promising get their
  own order search. Plans rank by callee-saved hits, volatile hits, then the rewrite cost (swapped declaration pairs +
  4 per edit + 4 per declaration moved out of its block); every plan is polished towards the source's order.
* It reads files and returns values; it never writes.

## Lib dependencies

ppc (field decode), cscan, cextract, declperm (the source side).

## Test contract

Tier: fixture - `tools/tests/mwcc-debugger/test_regsim.py`: a real dump (`wpadSendReadMemory`) replays exactly and a
corrupted register is reported; the stalled parameter is marked; select picks volatile, then the lowest handed-out
callee-saved, then a new one; `-> none` parses; the per-vreg attribution pairs a renamed register and skips an opcode
difference; the search finds the edit the readmem residual needs and finds none without edits; a declaration
permutation is found on a synthetic graph; source locals carry their scopes and map to the dump's vregs; a stalled
19-clique pushes (and spills) the cheapest node, a tie the highest vreg (the latest in the scan), and a 20-clique spills
two with no r13/r12; the spill cost's weights, uses and rematerialised constants, the pass's PCode dump, and `load`
filling costs only when the dump printed none.

## Known gaps

A demotion (a local turned back into a reused variable's `@` web) and two-edit plans are not searched; the cost of a
def-and-use operand (`rlwimi`, an update-form base) and the PCode of a function with several FPR passes are unmeasured;
a fold (`Graph.coalesce`) adds the two costs, unmeasured; the attribution reads only instructions a later pass left
unchanged (about half of them).
