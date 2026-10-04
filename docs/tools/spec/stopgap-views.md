# `STOPGAP-VIEW` - a proposal: stubbing a missing field or virtual slot of another lane's class (design only)

Status: **proposal, not implemented** (tooling batch B, 2026-10-04). Nothing in `tools/` reads the marker yet; the owner
decides whether it is built.

## Problem

A STOPGAP block (`/* STOPGAP-BEGIN(<id>) */ ... /* STOPGAP-END(<id>) */`, `lib/requests.py`) can only hold declarations of
**free** symbols: a prototype or an `extern` the integrator later moves to the owner's header. It cannot express a
**member** the lane needs on a class another lane owns - a field at `+0xA0`, or a virtual slot at vtable `+0x4C` - because a
member lives inside the owner's class body, which only the owner's header may define (rule 1), and reaching it by offset
is pointer arithmetic (rule 6). In the network pilot's round 2 this blocked about 9 KB of L3 bodies: the session-close
roster/profile senders `0x80435740..0x80435E34`, `flushRosterSync`, `refreshRosterCache`, `startRosterFetch`,
`fn_80435F48` and `fn_80436220` need `NetworkCommunityPat` slots `+0x48/+0x4C/+0x50/+0x60` and `NetworkLayerPat` field
`+0xA0` (requests `net2-l3#23`/`#24`), and the pat-control message-pool helpers need layer slots `+0x5C/+0x60`. The lane
filed `field` requests and left the bodies unwritten until the integrator ran.

## What has to hold

* **Codegen-identical to the final form.** The body the lane measures must compile to the same instructions as the body
  after the owner's class gains the member; otherwise the lane's score is a fiction and the integrator re-measures from
  zero.
* **No edit of the owner's files.** Two lanes writing one class header is the conflict this campaign pays for most (the
  L3 merge silently shifted `NetCtrlWk` by `0x140` when an embedded record changed size).
* **Mechanically removable.** `integrate.py` applies the `field` request and deletes the stopgap without judgement, the way
  it deletes a STOPGAP block today.
* **Visible debt.** Every rule the stub bends is counted until the request is applied, exactly as a STOPGAP block's
  declarations still count for rules 2 and 7.

## The smallest mechanism

A **view**: a local, layout-only struct the lane casts the owner's object to, wrapped in a block that names the request.

```cpp
/* STOPGAP-VIEW-BEGIN(net2-l3#24) */
/* view of NetworkLayerPat (owner src/Network/NetworkLayerPat.cpp) - fields this lane needs until #24 is applied.
 * size: partial (a view to +0xA4; the class is larger) */
struct NetworkLayerPat_view {
    /* +0x00 */ const struct NetworkLayerPat_view_slots* slots;   /* the class's vtable, read-only (rule 10 Case 2) */
    /* +0x04 */ u8 pad_0x04[0x9C];
    /* +0xA0 */ NetworkSessionState* session_state;               /* request #24: the proposed name and type */
};
/* the slots this lane calls, at their vtable offsets; the first argument is the object itself */
struct NetworkLayerPat_view_slots {
    /* +0x00 */ u8 pad_0x00[0x5C];
    /* +0x5C */ s32 (*acquireMessage)(struct NetworkLayerPat_view* self, u32 kind);   /* request #25 */
    /* +0x60 */ void (*releaseMessage)(struct NetworkLayerPat_view* self, NetworkMessage* message);
};
#define STOPGAP_VIEW(id, View, expr) ((View*)(expr))
/* STOPGAP-VIEW-END(net2-l3#24) */

    state = STOPGAP_VIEW(net2-l3#24, NetworkLayerPat_view, layer)->session_state;
    STOPGAP_VIEW(net2-l3#25, NetworkLayerPat_view, layer)->slots->releaseMessage(
        STOPGAP_VIEW(net2-l3#25, NetworkLayerPat_view, layer), message);
```

(The `id` argument of `STOPGAP_VIEW` is unused by the compiler - the macro must stay a plain cast - and is how the tools
find every use; a request id is not a C token, so the real spelling would carry it in a comment, `/* view:<id> */((View*)
(expr))`, or as a string in an unused macro parameter. The block above shows the shape, not the final lexical form.)

* **The block** holds only the view types (and at most the macro); it lives in the consuming `.cpp`, never in a header,
  so no other unit can grow a dependency on it.
* **A field** is a member of the view at its owner offset, named and typed as the `field` request proposes; leading bytes
  are one `pad_0xNN` array. A load or store through the view is the same `lwz`/`stw rX, 0xA0(rY)` the owner's member gives.
* **A virtual slot** is read through a struct of typed function pointers at `+0x00` - rule 10 Case 2's sanctioned shape for
  calling a slot "without dragging a class into the TU" - and called with the object as its first argument. For a class
  with single inheritance and no `this` adjustment this is the same `lwz r12,0(r3); lwz r12,0x5C(r12); mtctr r12; bctrl` a
  `virtual` call compiles to. The view **never** declares `virtual` (MWCC would then own a hand-modelled class and may
  emit a table) and never writes `+0x00`.
* **Each use** is the marked cast, so integrate can rewrite it.

### What `integrate.py` would do on apply

1. Apply the `field` request to the owner: a field replaces (or splits) the `pad_0xNN`/`unused_0xNN` that covers its
   offset, refusing when a named member of another type already sits there; a slot becomes `virtual R name(args);` at the
   declaration position that yields its vtable offset - checked against the DOL with `vtableaudit --at <vtable>` /
   `vtslot.py`, because inserting a virtual shifts every later slot (this half is `semi` at best: the position is read, not
   guessed, and a mismatch refuses).
2. Rewrite every marked use: `STOPGAP_VIEW(id, X_view, e)->field` -> `e->field`;
   `STOPGAP_VIEW(id, X_view, e)->slots->name(STOPGAP_VIEW(id, X_view, e), args)` -> `e->name(args)`. The owner's header
   is already included (the lane needs it for the type `e` has); a use whose `e` is not a `X*` refuses.
3. Delete the block, rebuild, and re-measure the consumer **and the owner and its other consumers** (playbook 60: a
   header's declaration set is a codegen input). Accept only when every row is equal or higher; the lane's measured rows
   are the expectation.

## Rule interactions (`docs/plan.md` 6.5)

| rule | interaction | while the request is open |
| --- | --- | --- |
| 1 one header per shared type | the view is a second definition of the owner's layout | **counted**: one rule-1 finding per view type (it shadows `X`) |
| 2 extern with the owner | no symbol is declared; a view slot is a member pointer, not a prototype | unchanged |
| 3 size stated | a view is partial by construction | **counted** unless marked `size: partial (view to +0xNN)` - the "approximation, marked" case |
| 4 offsets | every view member carries `/* +0xNN */`, ascending; integrate maps members to owner offsets with them | enforced (a view without offsets is unapplyable) |
| 5 context names | members take the request's `proposed_name`; leading bytes are `pad_0xNN` | enforced |
| 6 no pointer arithmetic | the view exists so the lane never writes `*(T*)((u8*)p + 0xA0)`; a cast to a declared type is not arithmetic | the cast is legal **only** through a `STOPGAP_VIEW` use; a bare cast to `*_view` elsewhere is a finding |
| 7 no generated names | a member named `slot_4C`/`unk_A0` is still a finding; name it | counted as usual |
| 8 no goto | none | - |
| 9 mangled names via owner | a view slot is called by its member name; after apply the call is `e->name()` and the mangling is the owner's | - |
| 10 vtable is compiler output | reading slots through a typed function-pointer struct is Case 2 (legal); the view must never declare `virtual` nor assign `+0x00` | `vtableaudit` sees no write; a `virtual` in a view is a finding |
| 11 no `void *` | the slot's `self` is `X_view*`, not `void*` | enforced |
| 12 unclaimed data | none | - |
| 13 a method is a member | the view has no functions; a free `X_view_name(X_view*)` helper is the defect rule 13 names | enforced |

Stylelint would treat a `STOPGAP-VIEW` block like a STOPGAP block: a block whose id is no open request is a finding, an
unpaired marker is a finding, and the rule-1/3 findings above are reported with the block's id so the debt is attributable.
The gate stays add-only: a lane adding a view adds findings exactly as a STOPGAP block's foreign declarations do, so the
pilot's lane exception (the orchestrator's call, not a lane's) governs both alike.

## Alternatives considered

* **Let the lane add the member to the owner's header inside a STOPGAP block.** Smallest diff, but it is two lanes
  editing one class: the owner lane's layout changes under it, and the merge is the integrator's worst case (the
  `NetCtrlWk` shift). Rejected.
* **A STOPGAP accessor function** (`u32 getSessionState(NetworkLayerPat*)` declared as a free symbol): it compiles to a
  `bl`, not a load - wrong codegen, and a fake symbol in the map. Rejected.
* **Raw offsets** (`*(T*)((u8*)p + 0xA0)`): rule 6 forbids it and nothing would mark it for removal. Rejected.
* **Wait for the integrator** (today): costs a round trip per missing member; the 9 KB stayed unwritten for a whole round.

## Recommendation

Build the view only if the next pilot round shows the same blockage again: the integrator now runs after every lane
report (22-37 min per lane), which already shortens the wait, and a `field`/slot request applied promptly costs less than
a new marker, two stylelint rules and an integrate rewrite step. If it is built, build it in this order: the stylelint
half first (block pairing, open id, the rule-1/3 counts, the cast-only-through-the-marker check for rule 6, `virtual` in a
view as a finding), then integrate's field application (mechanical: pad split at a stated offset), and the virtual-slot
application last, as `semi`, refusing on any slot-position disagreement with the DOL. The measurement rule is the one
that makes it worth having: a lane's body written against a view must re-measure equal after apply, or the view was
not codegen-identical and the stub is reverted.
