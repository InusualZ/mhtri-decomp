/*
 * lobby/lb_server_sel_trans.cpp - the demo/event work accessors and the server-selection transition band,
 * `.text` 0x803C3A5C..0x803CCDF8.
 *
 * Phase 4 recut of `menu/get_pop_dat_ptr`: the registered unit's range ended at 0x803C4BA0 and its
 * tail (0x803C3A5C..0x803C4B74, the demo work accessors below) joins the 0x803C4B74..0x803CCDF8 band no registered unit held
 * (33368 B, unwritten, so `.text` measures 0 % for it).  The `demo_work` block (.bss 0x806D2B20) these accessors read is
 * this unit's `.bss` claim.  Module `lobby` and the name `lb_server_sel_trans` are the candidate's (a GUESS from the band's
 * server-selection transition code, no `__FILE__` string); flags are the registered unit's `cflags_menu` (the recut keeps
 * the old row; the lobby library's own flags are untested for this band).
 *
 * UNWRITTEN: everything from 0x803C4B74 up, and the functions of 0x803C3A5C..0x803C4B74 other than the eight accessors
 * below; the pop-data/option half of the old range stays in `menu/get_pop_dat_ptr.cpp`.
 */


#include "types.h"
#include "nw4r/math.h"       /* nw4r::math::VEC3, the 3-float record VEC3_ctor takes */
#include "mh3_pad/vec3.h"    /* VEC3_ctor (owner src/mh3_pad.cpp) */
#include "fn_8004CAD8.h"     /* get_vsUser_work / _vs_user_data (owner src/fn_8004CAD8.cpp) */
#include "hud/layout.h"     /* _mh_ivec2_, which the band header's prototypes name */
#include "Runtime.PPCEABI.H/memset.h" /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "unsplit/menu.h"    /* option_w, this band's own tables, its unowned callees */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "lobby/lb_server_sel_trans.h"

/* The demo/event work block (.bss 0x806D2B20, 0x28 B) the accessors below read and write. */
DemoWork demo_work;

/* Non-zero while the demo work is playing the demo the game asks about. */
u32 event_demo_ck(void)
{
    return demo_work.state_0x00 == 4;
}

/* The demo id the demo work is playing, 0xFF when none. */
u8 get_demo_no(void)
{
    return demo_work.demo_no_0x02;
}

/* The demo work's accessors, which keep the map's plain (C-linkage) spelling. */
extern "C" {

/* The word the demo work holds at +0x04. */
u32 get_demo_data(void)
{
    return demo_work.field_0x04;
}

/* Non-zero while any of the demo work's three entry flags is set. */
u32 demo_flag_ck(void)
{
    if (demo_work.field_0x0C != 0) {
        return 1;
    }
    if (demo_work.entry_0x14.flag_0x00 != 0) {
        return 1;
    }
    if (demo_work.entry_0x14.flag_0x08 != 0) {
        return 1;
    }
    return 0;
}

/* Clears the demo work. */
void demo_work_clear(void)
{
    memset(&demo_work, 0, 36);
}

/* Marks the demo work as running. */
void demo_set_running(void)
{
    demo_work.field_0x08 = 1;
}

/* Initialises the demo work for a fresh play-through. */
void demo_work_init(void)
{
    demo_work.field_0x0D = 0;
    demo_work.field_0x0C = 0;
    demo_work.state_0x00 = 1;
    demo_work.field_0x04 = 0;
    demo_work.demo_no_0x02 = 255;
}

/* True for the five map ids 394..398. */
u32 map_id_ck(u16 map)
{
    return (u16)(map - 394) <= 4;
}

} /* extern "C" */
