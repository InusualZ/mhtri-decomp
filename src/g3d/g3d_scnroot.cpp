/*
 * nw4r g3d: g3d_scnroot.cpp - the ScnRoot scene-graph root (camera, fog, light, the root's draw
 * buffers and the scnMdl list).
 * `.text` 0x800827E4-0x8008452C (45 functions, 7496 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `8007C540` - this
 * unit is one of the four original translation units that proposal's range turned out to span, and it
 * is the last: the proposal's cap 0x8008452C is this unit's right edge.
 *
 * Name - evidence class 1, a `__FILE__` string: the unit's `.data` fragment starts at 0x8058F530 with
 * the bare source-file name `g3d_scnroot.cpp` (lbl_8058F530, referenced by fn_800827E4, by
 * SetCurrentCamera__Q34nw4r3g3d7ScnRootFi and by fn_80083598/fn_8008446C), and it carries the
 * class's own asserts (`((u32)buf & 0x3) == 0`, `0 <= camID && camID < NUM_CAMERA`,
 * `NW4R:Pointer must not be NULL (bufOpa/bufXlu)`).  Module `g3d`, language C++ - the map carries two
 * real C++ manglings for this range, `GetCamera__Q34nw4r3g3d7ScnRootFi` (0x80082B70) and
 * `SetCurrentCamera__Q34nw4r3g3d7ScnRootFi` (0x80082C08), i.e. `nw4r::g3d::ScnRoot::GetCamera(int)` and
 * `ScnRoot::SetCurrentCamera(int)` (rule 9: called through the class, never by the mangled spelling).
 *
 * Seams.  Left: 0x800827E4 - tudiscover's weak `codegen fingerprint change` cut, the extent of this
 * unit's proven match set (45 functions, "certainly one TU", anchored by `g3d_scnroot.cpp`) and the
 * `.data` fragment boundary.  Right: 0x8008452C, the proposal cap.
 *
 * RESIDUAL on the right edge: the unit's own name-record reader for ScnRoot,
 * `return *fn_800638B8(&local, lbl_8056F6D0)` with lbl_8056F6D0 = "ScnRoot", is NOT inside this
 * range - the two functions that read that record are fn_8008452C (the very first function after the
 * cap) and fn_80084600.  Every other class in this cluster has its reader inside the allocated range,
 * so the real g3d_scnroot.cpp almost certainly ends at 0x8008455C or later and the proposal's
 * `--max-bytes` cap cut the file's tail off.  The range is registered at the cap for measurement
 * (never widened by guess); the tail is recorded in the outbox for the batch re-split, together with
 * the observation that fn_8008452C/fn_80084600/fn_80084630 read the ScnRoot record and the shared
 * float table lbl_8056F6E0 and so belong beside this unit.
 *
 * Sections claimed: `.text` 0x800827E4-0x8008452C, `extab` 0x800088F4-0x80008ADC,
 * `extabindex` 0x800212C4-0x8002148C.  The `.data` fragment 0x8058F530-... is not claimed
 * (docs/plan.md 8.4); it is recorded for the data pass.
 *
 * Reconstruction status: registration only - the bodies are the next pass's work.  Per-unit scores
 * are in the outbox.
 */

#include "types.h"
