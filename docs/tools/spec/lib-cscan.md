# `lib/cscan` - C/C++ lexical scanning for the source tools

## Purpose

Reads C/C++ text the way every source tool needs it: one lexer for comments and literals with its three views,
bracket matching, the preprocessor mask, struct definitions and their field chunks, function declarations with
declarator parsing, the names a file declares, and includes with their closure. No C parser: each scanner is
textual and conservative, and the line numbers it reports are the original file's.

## Users

`units/stylelint.py` (strip, `Source`, struct/field/declaration scanners, rule 13's include walk),
`units/typeregistry.py` (strip_comments, `declared_names`, includes), `units/declclash.py` (the include
closure), `flags/shapes.py` (`remove_comments`), and through `stylelint` `mergebranch` (strip) and `methodize`
(`mask_preproc`).

## Public API

* The lexer: `spans(text) -> [Span(kind, start, end)]` (`block`, `line`, `string`, `char`).
* Its views: `strip(text) -> (code, comments)`; `strip_comments(text)`; `remove_comments(text)`; `mask_preproc(code)`.
* Brackets on stripped code: `match_brace(code, open)`, `match_paren(code, open)` (-1 when unmatched).
* `Text(text)`: `.text`, `.code`, `.comments`, `line_of(pos)`, `line_text(line)`, `span_lines(start, end)`.
* Types: `STRUCT_RE`; `struct_defs(t) -> [TypeDef(name, start, open, close, line, end_line)]`;
  `fields(code, open, close) -> [(start, end)]`.
* Declarations: `declared_name(segment)`; `function_declarations(t) -> [Declaration(name, pos, line, start_line,
  end_line, params, params_pos, ret, ret_pos, body)]`; `STATEMENT_KEYWORDS`, `NON_DECL_HEADS`, `LINKAGE_OPEN_RE`.
* Declared names: `declared_names(clean) -> [NameDecl(name, kind, line, shape)]`; `shape_of(body)`,
  `declarator_name(region)`, `alias_names(tail)`, `NOT_NAMES`.
* Includes: `INCLUDE_RE`; `includes(text, angle=True)`; `resolve_include(name, bases)`;
  `include_closure(start, resolve, read=None, angle=False)`.
* `to_dict()` on `TypeDef`/`Declaration` gives the dict shape the tools carried (`Declaration` has `body` only
  on a definition).
* `rewrite_identifiers(text, pairs, comments=False) -> (text, counts)`: a rename's other half on one file - code tokens
  always, comment mentions with `comments`, never a literal, an `#include` line, a path-shaped or string-table-shaped
  token (`symedit.py --rewrite` walks the tree with it).

## Invariants and rules

* **One lexer.** A comment or a literal is found left to right, so a quote inside a comment and a comment opener
  inside a literal are what they look like. A literal ends at its closing quote or **before** an unescaped
  newline (an unterminated `'` in a `#error` line never swallows the next line); an escape takes the next
  character whatever it is; an unterminated block comment runs to the end.
* **The three views.** `strip`'s `code` blanks comments (newlines kept) and every literal character - an
  escaped newline inside a literal included, so offsets map exactly while line numbers come from the original
  text (`Text.line_of`); its `comments` view is the text with only the literals blanked, so an annotation
  (`/* size: 0x10 */`) is found there and the same words inside a string are not. `strip_comments` blanks both
  and keeps **every** newline, so a line count over its result is the original line (typeregistry derives
  lines that way). `remove_comments` deletes comments and keeps literals (normalised-source dedupe).
* **A cast is never a declaration.** `function_declarations` walks file, namespace and class scope with a scope
  stack; a statement inside a function body is never read, a linkage block (`extern "C" {` - `strip` blanks
  the `"C"`) is transparent, and the declarator check (`(` after the name, or `(*name)(`, then only a
  qualifier/`:`/end) separates a function header from an initializer, an expression (`static_assert`) and a
  control block.
* **`declared_name`**: the first `(` decides - `(*name)` is a function-pointer variable, `(*name(` a function
  returning one, else the last identifier before the `(`; without a `(`, the last identifier before any `=`
  with array dimensions dropped. Rule 2 and rule 11 read names through this one parser so they cannot disagree.
* **An include is a directive at a line start.** `includes` reads `#include "..."`/`<...>` only where a
  directive can be; an include *mentioned* in a comment is not one.
* `include_closure` is depth first, the start first, each file once (by normalised absolute path), and an
  unreadable file ends its branch.

## Absorbs (today's implementations)

`stylelint.strip/Source/match_brace/match_paren/_mask_preproc/struct_defs/iter_fields/_declared_name/
_declared_name_pos/_declarator_parens/_declaration_from/function_declarations/_INCLUDE_RE/_resolve_include`'s
probe, `typeregistry.strip_comments/_match_brace/_split_semicolons/shape_of/declarator_name/alias_names/
extract_decls/_includes/NOT_NAMES` and its regexes, `declclash.closure/resolve/INCLUDE_RE`, `shapes.strip_comments`.

## Lib dependencies

None (stdlib).

## Test contract

Tier: fixture (`tools/tests/lib/test_cscan.py`). The lexical rows of the stylelint selftest (block, line,
string, escaped quote, unterminated literal), typeregistry's (length and newlines kept), shapes'
(`remove_comments`), the declarator shapes, `function_declarations` refusing a call, a macro, an initializer and
a `static_assert`, includes at a line start only, and declclash's closure (depth first, cycle, missing header).

## Measured (WP2c)

On the 923 `src/`+`include/` files of the tree at `d82dbf1a2`: `strip`, `struct_defs`, `fields`,
`function_declarations`, `declared_name`, typeregistry's `extract_decls`, `shapes.strip_comments` and
declclash's/stylelint's include lists are identical old vs new; `strip` takes 0.94 s against 1.91 s.
`declclash.py --json` over every 25th source file and `typeregistry.py --json` print the same bytes before and after.

## Known gaps

* **One behaviour change**: typeregistry's include reader matched `#include "x"` anywhere, so a header that
  *mentions* an include in a comment counted as including it (3 headers on this tree: `include/ai/ainpc.h`,
  `include/ef.h`, `include/mh3_pad/vec3.h`); it now reads directives only. `typeregistry.py --json`, its text report
  and `--unit Camellia/camellia.c` are byte-identical before and after on this tree.
* Not yet here (the 3d stylelint split and 3b scoring packages): `statements` (`shapes.split_statements`, which
  skips literals on raw text), `type_definitions` with offsets and the size comment (`vtableaudit.type_definitions/
  _members`, `stylelint.struct_has_size/field_name/type_defs`), `pragmas`, `calls` (`methodize._call_sites`),
  `recordmerge.parse`, `freshguard.source_closure`, and `shapes.match_brace/_paren_match` (raw-text, literal-aware).
* stylelint's rule-13 include walk is a set-union over every `.cpp`, not `include_closure` (one closure per file
  would repeat the work); it uses `includes`/`resolve_include` only.
