# `lib/names` - Generated-name and mangling predicates, once

## Purpose

Says whether a symbol name is an address-derived placeholder, whether it is an MWCC C++ mangling, what its linkage stem is,
and estimates the mangling of a member function without the compiler.

## Users

`callees.is_generated` (`rule7`), stylelint rule 7 and the gate's new-unit row (`generated_name_kind`), `dumpmap.is_generated` (`map`), `ledger.GENERATED_RE` (`ledger`), `undefrefs` and
`relocaudit` `linkage_stem`, `langcheck.mangled`, `mangle.estimate_*` (and through it `methodize`, `stylelint` rule 13),
`typeregistry.tokens_of_name`.

## Public API

* `is_generated(name, scheme="default")`, `GENERATED = {scheme: regex}`; `address_of(name)` (the `_XXXXXXXX` tail).
* `generated_name_kind(name) -> 'generated' | 'address' | None` - section 6.5 rule 7 exact (owner, 2026-10-05): a `RULE7_STEM_RE` stem (`fn`/`lbl`/`loc`/`dtor`/`zz` + `_` + 8 hex) **anywhere** in the name, else an `ADDRESS_RUN_RE` run inside `DOL_SPAN` (`[0x80004000, 0x80800000)`). One verdict for stylelint's identifier and file-name findings and the gate's new-unit row.
* `is_mangled(name)`; `linkage_stem(name)`; `peel_tokens(name)` (the `<digits><chars>` components).
* `param_code(decl)`, `estimate_member_mangling(type, method, rest_params, const_self=False)`,
  `estimate_static_mangling(type, method, params)`; `PRIMITIVE_CODES`, `QUALIFIERS`.

## Invariants and rules

* **Several schemes, each written once.** The tools ask different questions and each answer is kept exactly:
  `default` = the placeholders dtk/MWCC emit (`fn_`/`lbl_`/`loc_`/`jumptable_`/`pad_`/`gap_` + hex, `@etb_`/`@eti_`, `@NNN`);
  `rule7` = section 6.5 rule 7's `fn_`/`lbl_`/`loc_` + hex; `map` = every address-derived name that can sit in a map
  (`dtor_`, Ghidra's `FUN_`, `sub_`, 6-8 hex digits, a bare `unkNN`); `ledger` = the ledger's "no information yet" test.
  Measured on the 65 692 names of the live map: each scheme agrees with the copy it replaced on every name.
* **`linkage_stem`**: everything before `__F<x>`/`__Q<n>`/`__ct`/`__dt`, and a name never starts with its suffix. The two
  copies disagreed (`undefrefs`: `__(?:F[A-Za-z0-9]|Q\d|ct|dt)`; `relocaudit`: `__(F|Q)`); on the live map the one rule
  changes 43 `undefrefs` stems (42 `__ct__`/`__dt__` names and `__FileWrite`, all of which had the empty stem, so every
  constructor was "one symbol") and 1 `relocaudit` stem (`__FileWrite`, the same empty stem). Both selftests pass unchanged.
* `is_mangled` is the loose test (`__F`/`__Q` anywhere after a `__`); `__start`, `_savegpr_14`, `lbl_...` are C/EABI names.
* The mangling estimate is an ESTIMATE: an unknown identifier is read as a class name (`<len><Name>`); a function pointer,
  an array, a `::` or a repeated class parameter (the compiler back-references it) returns None. `mangle.py <decl>` compiles
  for the exact answer.

## Absorbs (today's implementations)

`callees.is_generated`, `dumpmap.is_generated`, `ledger.GENERATED_RE`, `relocaudit.linkage_stem`, `undefrefs.linkage_stem`,
`langcheck.mangled`, `mangle._param_code/estimate_member_mangling/estimate_static_mangling/_PRIMITIVE_CODES/_QUALIFIERS`,
`typeregistry.tokens_of_name`'s peel.

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_names.py`). A table of names with each scheme's expected value; the stems the
relocaudit/undefrefs tests pin plus the constructor cases; `peel_tokens`; the member/static estimates and their refusals.

## Known gaps

* `linkage_stem` does not split a class member's mangling (`send__16NetworkSingleTcpFPCUcl` stays whole): neither copy did,
  and the relocaudit report depends on it; deciding it is WP3a's (`undefrefs --census`).
* `stylelint`'s rule 7/9 regexes search source *text* (`\bfn_[0-9A-Fa-f]{8}\b`), not names; they live in
  `stylelint_rules/r07_generated_name.py` and `r09_mangled.py` (WP3d). Rule 13's mangling estimate now calls
  `estimate_member_mangling`/`estimate_static_mangling` here directly (it went through `mangle`).
