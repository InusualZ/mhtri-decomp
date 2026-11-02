---
id: 290
title: One inlined by-value wrapper inside a dont_inline TU: open the window for that function, close it after a declaration, never_inline the record ctors
status: works
problem: Two by-value argument copies sit in the wrong frame slots (ours interleaves each copy with the record it copies, retail puts both copies below both records) in a TU compiled under `#pragma dont_inline on`
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
related: [32, 61, 285]
---

# 290. One inlined by-value wrapper inside a dont_inline TU: open the window for that function, close it after a declaration, never_inline the record ctors

**Problem.** `stw r3,0x14(r1)` / `addi r4,r1,0x14` where ours has `0x20`, and the other case the other way round: a big
switch posts an error twice through a by-value inline wrapper (`postError(NetworkPostedError info) { postError(&info); }`).
The TU calls every inline member out of line (`#pragma dont_inline on`), so the copy was spelled as a named local
(`NetworkPostedError posted(error)`) - and a named local takes the next frame slot after the record it copies, while
retail's copies are the inlined wrapper's argument temporaries, laid out below every named local (idea 285). A temporary
bound to a reference gets the slots right but holds its address in a saved register across the `getInstance_` call
(`addi r28,r1,0x14` ... `mr r4,r28`, +8 B).

**Why it happens.** Two facts measured on this unit. (1) MWCC decides inlining when it **compiles** the caller, with the
pragma state it has read by then - and it reads one declaration ahead: a `#pragma dont_inline on` written right after the
function's closing brace is already in force when the function is compiled, so the wrapper stays a `bl`. A declaration
between the brace and the pragma (here the record's own inline ctor/dtor definitions, which retail emits right after
this function anyway) makes the window real. (2) With inlining open for the function, every inline member it calls is
inlined - the implicit and the `inline`-defined constructors/destructors of its block records included - and the object
loses their out-of-line copies while the call sites turn into the member ctors' calls (same bytes, other relocations:
`relocdiff --by-owner` is what shows it, the row stays at 100 %).

**How to work it.** Call the wrapper by value, bracket only that function with `#pragma dont_inline off` ... a declaration
... `#pragma dont_inline on`, and declare the records' constructors and destructors explicitly with the trailing
attribute form `NetworkCommunityData() __attribute__((never_inline));` (the leading form is a syntax error on Wii/1.3),
defined `inline` where the others are. The attribute keeps them out of line inside the window; `inline` keeps retail's
deferred emission after the first caller. Check `relocdiff --by-owner` and `flipcheck` after, not only the row.

**Result.** `Network/NetworkCommunityPat` `onPatEvent` 99.99 -> 100 % (the 7 frame-slot instructions), 87/87 rows,
flipcheck READY, trial flip `main.dol: OK`.

**Example.**

```
#pragma dont_inline off
void NetworkCommunityPat::onPatEvent(...)
{
    ...
        if (count < 0) {
            NetworkErrorInfo error;            /* 0x2c / 0x20: the named records */
            ...
            getInstance_()->postError(*(NetworkPostedError*)&error);   /* the copy: 0x14 / 0x08, below both */
            break;
        }
    ...
}
inline NetworkCommunityData::~NetworkCommunityData() {}   /* the declaration that ends the window */
inline NetworkCommunityData::NetworkCommunityData() {}
#pragma dont_inline on
```
