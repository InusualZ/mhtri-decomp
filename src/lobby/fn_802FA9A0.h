/* Declarations of `lobby/fn_802FA9A0.cpp`'s symbols and its own views of the `lobby_w`/`lobby_world_block` blocks it
 * reads (one view per unit, like `lobby/fn_8021E1EC.h`). */
#ifndef MHTRI_LOBBY_FN_802FA9A0_H
#define MHTRI_LOBBY_FN_802FA9A0_H

#include "types.h"

/* The lobby work block (`lobby_w`, .bss 0x806AAB44, 0x17C B in the map) as THIS range sees it: the
 * three flag bytes below are the only fields it touches - the rest of the block is filler because the
 * range never reaches into it (rule 5's padding exception).  The offsets come from the range's own
 * code (`fn_802FB5F0` reads +0x003, `LbCheckKujiraEvent` +0x15F, `fn_802FB9F8` +0x161).
 * size: 0x17C */
typedef struct LbLobbyView {
    /* +0x000 */ u8 unused_0x000[0x3];
    /* +0x003 */ u8 field_0x003;   /* 0 while the lobby act layer is idle */
    /* +0x004 */ u8 unused_0x004[0x15B];
    /* +0x15F */ u8 kujira_0x15F;  /* == 1 while the whale event is up (`LbCheckKujiraEvent`) */
    /* +0x160 */ u8 unused_0x160[0x1];
    /* +0x161 */ u8 field_0x161;   /* the second event gate `fn_802FB9F8` reports */
    /* +0x162 */ u8 unused_0x162[0x1A];
} LbLobbyView; /* size: 0x17C */

/* The sub-record at +0x4832 of the block `lobby_world_block` points at, passed to its own helpers by
 * address (`fn_802FF248` hands `&block->work_0x4832` to `fn_802FF478`, which writes its +0x04).
 * size: 0x0C (the extent this range reads; the parent block's filler covers the gap to its next
 * field) */
typedef struct LbKujiraWork {
    /* +0x00 */ u8 flag_0x00;   /* mirrored from +0x01 by `fn_802FF248` */
    /* +0x01 */ u8 bits_0x01;   /* one bit per unlocked kind (`fn_802FF29C` sets, `kujira_event_over_ck`
                                * reports the top one) */
    /* +0x02 */ s8 count_0x02;  /* `fn_802FF248` seeds it to -4 */
    /* +0x03 */ u8 unused_0x03[0x1];
    /* +0x04 */ u16 timer_0x04; /* `fn_802FF478` loads it with a `ran_suu(1)` draw */
    /* +0x06 */ u8 unused_0x06[0x6];
} LbKujiraWork; /* size: 0x0C */

/* The block the `.sbss` pointer `lobby_world_block` (0x80794880) points at, as this range sees it: the
 * mode byte at +0x3E01 (`fn_802FB5E4`), the per-kind state bytes at +0x4654 (`fn_802FB8EC`) and the
 * flag bytes at +0x4832..+0x4834 (`fn_802FF248`/`kujira_event_over_ck`/`fn_802FF29C`).  Everything between them
 * is filler.  size: 0x6010 (the neighbour view's extent - `lobby/fn_8021E1EC.h`; this range
 * reads nothing above +0x4834) */
typedef struct LbBlockView {
    /* +0x0000 */ u8 unused_0x0000[0x3E01];
    /* +0x3E01 */ u8 mode_0x3E01;   /* the scene byte the area gate compares against 4 */
    /* +0x3E02 */ u8 unused_0x3E02[0x4654 - 0x3E02];
    /* +0x4654 */ u8 states_0x4654[0x4832 - 0x4654]; /* per-kind state bytes, `kind`-indexed */
    /* +0x4832 */ LbKujiraWork work_0x4832;
    /* +0x483E */ u8 unused_0x483E[0x6010 - 0x483E];
} LbBlockView; /* size: 0x6010 */

/* The record `fn_802FF90C` indexes: the per-slot byte array starts at +0x74, so only the head is
 * reachable from this range.  size: >= 0x75 (only the byte array is read) */
typedef struct LbEventSlot {
    /* +0x00 */ u8 unused_0x00[0x74];
    /* +0x74 */ u8 kind_0x74[];
} LbEventSlot;

/* The block the range's own `fn_802FDB90`/`fn_802FDF9C`/`fn_802FBE44` receive: the whale-event work
 * record (`LbCheckKujiraEvent`'s band).  Only the bytes below are touched.  size: 0x32E (approximate:
 * the extent is the largest offset this range reads) */
typedef struct LbKujiraEventWork {
    /* +0x000 */ u8 unused_0x000[0x1E6];
    /* +0x1E6 */ u8 done_0x1E6;   /* non-zero skips the step the two wrappers forward to */
    /* +0x1E7 */ u8 unused_0x1E7[0x328 - 0x1E7];
    /* +0x328 */ u8 field_0x328;
    /* +0x329 */ u8 field_0x329;
    /* +0x32A */ u8 field_0x32A;
    /* +0x32B */ u8 field_0x32B;
    /* +0x32C */ u8 field_0x32C;
    /* +0x32D */ u8 unused_0x32D[0x1];
} LbKujiraEventWork; /* size: 0x32E */

/* The caller's record, only ever handled by address here (`struct _ENEMY_WORK*` parameters). */
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

extern LbLobbyView lobby_w;      /* .bss 0x806AAB44 */
extern LbBlockView* lobby_world_block; /* .sbss 0x80794880 */

/* The two id tables the predicates walk, in this unit's `.data` claim: 28 entries (ids 0..27) and 12 (ids 28..39),
 * each pointing at a `u16` array terminated by 0xFFFF. */
extern u16* lbl_805D8BA0[28];
extern u16* lbl_805D8C10[12];

/* The `.sdata` byte table at 0x80792A38 this range indexes by a work byte (`fn_802FF90C`). */
extern u8 lbl_80792A38[];

/* `fn_802FB5E4` (0x802FB5E4) is defined by this range and read by `fn_802FB97C` below. */
u8 fn_802FB5E4(void);

/* `fn_802FDB04`/`fn_802FDDCC` (0x802FDB04, 0x802FDDCC) are this range's own event steps, forwarded to
 * by `fn_802FDB90`/`fn_802FDF9C`. */
void fn_802FDB04(LbKujiraEventWork* work);
void fn_802FDDCC(LbKujiraEventWork* work);

/* `fn_802FF29C`/`fn_802FF478` (0x802FF29C, 0x802FF478) are this range's own whale-event work helpers,
 * called by `fn_802FF248` above them. */
void fn_802FF29C(u32 raw);
void fn_802FF478(LbKujiraWork* work);

/* The four unwritten per-action step machines `enemy/em_sub_state_prog.cpp`'s `fn_802FA964` tail-calls; the
 * parameter is the caller's `_ENEMY_WORK`, the only type the call sites set. */
void fn_802FA9A0(struct _ENEMY_WORK* work);
void fn_802FAB98(struct _ENEMY_WORK* work);
void fn_802FAFB4(struct _ENEMY_WORK* work);
void fn_802FAFC4(struct _ENEMY_WORK* work);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x802FB9DC - whether the whale event is up; the `ef` band calls it.  C++ linkage: the map spells it mangled
 * (rule 9). */
u32 LbCheckKujiraEvent(void);
#endif

#endif /* MHTRI_LOBBY_FN_802FA9A0_H */
