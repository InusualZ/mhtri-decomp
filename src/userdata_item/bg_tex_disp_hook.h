/* userdata_item/bg_tex_disp_hook.h - `BgTexDispHook`, the type of `userdata_item.cpp`'s hook `bg_tex_disp_func`.  Split
 * out of `userdata_item.h` (like `mh3_pad/task.h`) so the unit itself can include it (rule 1). */
#ifndef MHTRI_USERDATA_ITEM_BG_TEX_DISP_HOOK_H
#define MHTRI_USERDATA_ITEM_BG_TEX_DISP_HOOK_H

#include "types.h"

/* The hook `bg_tex_disp_func` holds: `mh3_pad.cpp` stores the function, the frame draw calls it.  The map gives the
 * object 8 bytes; only the first word is referenced (GUESS: the second is the hook's argument).  size: 0x8 */
struct BgTexDispHook {
    /* +0x00 */ void (*func)(void);
    /* +0x04 */ u32 unused_0x04;
};

#endif /* MHTRI_USERDATA_ITEM_BG_TEX_DISP_HOOK_H */
