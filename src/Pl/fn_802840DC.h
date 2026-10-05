/* The player shell (gunner shot) band `Pl/fn_802840DC.cpp` (`.text` 0x802840DC-0x80288CEC).
 *
 * Declarations for the symbols this unit calls that live inside its left neighbour's range
 * (0x8027D684-0x802840DC, owned by `Pl/fn_8027D684.cpp`).  That unit has no header of its own yet,
 * so they are declared here, with the widths this unit's own call sites use, and only this unit
 * sees them (rule 2: the band header `unsplit/Pl.h` must not declare a symbol a registered
 * unit owns, and a second spelling of a name the owner defines is the `illegal function
 * overloading` class).  When `Pl/fn_8027D684.cpp` grows a header these move there - the same
 * arrangement `Pl/fn_80273B14.h` uses for the equipment resolvers that unit drives.
 */
#ifndef MHTRI_PL_FN_802840DC_H
#define MHTRI_PL_FN_802840DC_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The per-act handlers `fn_802840DC` tail-calls out of its `act_no` switch (cases 0-34; the two
 * families are the ones carrying an index argument and the ones that take only the actor).  The
 * call site's own register width is all there is to go on: the index is `li`-materialised (0-4) and
 * the actor is the same `_PLW` every other Pl entry takes. */
void fn_80282CCC(struct _PLW* self, u32 arg);
void fn_80282ED8(struct _PLW* self, u32 arg);
void fn_80283010(struct _PLW* self, u32 arg);
void fn_802831EC(struct _PLW* self, u32 arg);
void fn_802833F4(struct _PLW* self, u32 arg);
void fn_8028350C(struct _PLW* self);
void fn_802835D8(struct _PLW* self);
void fn_802836A4(struct _PLW* self, u32 arg);
void fn_80283810(struct _PLW* self, u32 arg);
void fn_802839A8(struct _PLW* self, u32 arg);
void fn_80283B88(struct _PLW* self);
void fn_80283C60(struct _PLW* self, u32 arg);
void fn_80283DB0(struct _PLW* self);
void fn_80283E80(struct _PLW* self);
void fn_80283F24(struct _PLW* self);
void fn_80283FC8(struct _PLW* self, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_802840DC_H */
