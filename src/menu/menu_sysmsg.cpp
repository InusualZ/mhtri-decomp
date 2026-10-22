/*
 * menu/menu_sysmsg.cpp - the game's software keyboard driver (the `kbd_*` hooks `system_w` carries: create the
 *   homebutton band's keyboard object, open it for a text kind, step it from the pad and the USB keyboard, close
 *   it).  The system message text before its `.data` is the data-only `menu/sysmsg_text.cpp`.
 * RANGE. .text 0x804513FC-0x804565C0 (87 functions); extab 0x8001E18C-0x8001E3B0, extabindex 0x8003EF88-0x8003F144,
 *   .ctors 0x8056F3D4, .rodata 0x80572240-0x80572418, .data 0x8060E0B8-0x8060E7FC, .bss 0x806F4658-0x806F4B48,
 *   .sdata 0x80793C80-0x80793CB8, .sbss 0x80794D64-0x80794DF8, .sdata2 0x8079C998-0x8079C9A8.  0x804513B0-0x804513FC (`kbd_modifier_bits` and the listener's two empty slots) is this
 *   TU's too (request menu-sysmsg#1).
 * FLAGS. `cflags_menu`; `#pragma dont_inline on` after the includes: retail calls the inline helpers out of line
 *   (the MEM heap wrappers, `Layout::SetAllocator`, the HKB manager accessors, the generated constructors), each
 *   copy after its first caller, while header inline bodies inline their own callees.  `no_static_dtors on`,
 *   `pool_data off`, `explicit_zero_data on` (the zero `.sdata` title).
 * NAMES. GUESS: the file name (no `__FILE__` string; request menu-sysmsg#3 proposes sys_keyboard).  The hooks are
 *   `sys_kbd_*` after the `system_w` fields `kbd_hooks_set` fills (main.cpp owns the `kbd_*` wrappers).  GUESS:
 *   every other function, class, global and view-slot name (from what each stores, tests and passes on), and the
 *   registry names `pl_gesture_req`, `vi_next_frame_buffer`, `userdata_phrase_get`, `draw_uv_rect`,
 *   `hbmPanelSetInputEnabled`, `hbmPanelClearKanaQueue`, `hbmKbdIsTextEmpty`, `hkbNodeAppend`, `hkbNodeRemove`,
 *   `kbd_modifier_bits`, `sys_font_code_table`.
 * RESIDUALS. `kbd_key_input`: retail's key switch is a `cmplwi` tree with another pivot (no case order reproduces
 *   it).  `kbd_command_step`, `kbd_step`: loop register colouring in the cursor searches.  `kbd_layout_load`: an
 *   `extsh` on the two 512 arguments.  `onKey`: r3/r4 colouring and a `kbd_request` reload.  `.sbss`: retail keeps
 *   the 8-byte `kbd_setting` 4-aligned (ours is 8-aligned, +4 B).  onConnect/onDisconnect sit in menu_plsearch's
 *   registered range (#1).  The inline band copies follow their first callers.
 * SHAPES. The keyboard object is reached through view classes with inline slots; its list is the band's
 *   `HbmAnimPanel`, whose base slot 0x40 (`getPage`) is defined inline here (its retail copy is this TU's);
 *   one-case `cmpwi` tests are one-case `switch`es; `(s8)(u8)++var` gives retail's clrlwi+extsb; the cursor searches step the globals in place.
 */

#include "types.h"
#include "nw4r/math.h"
#include "MEM/mem_expheap.h"
#include "MEM/mem_allocator.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXFrameBuf.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/string.h"
#include "MSL_C/printf.h"
#include "MSL_C/wstring.h"
#include "MSL/strlen.h"
#include "sound/fn_800E46E8.h"
#include "mh3_pad/system_w.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/control.h"
#include "main.h"
#include "sound/fn_800D7F54.h"
#include "sound/fn_800E3CBC.h"
#include "sound/set_zmode.h"
#include "ef/fn_800CDB2C.h"
#include "fn_80047398.h"
#include "fn_80056F24.h"
#include "font/flfnt.h"
#include "menu/menu_item.h"
#include "menu/get_pop_dat_ptr.h"
#include "g3d/g3d_anmscn.h"
#include "homebutton/tiHKBManager.h"
#include "homebutton/hkb_arc_resource_accessor.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hbm_anim_record.h"
#include "nw4r/RomFont.h"
#include "Network/network_pat_control.h"
#include "Network/NetworkPat.h"
#include "Network/NetworkLayerPat.h"
#include "Network/NetworkSessionManagerPat.h"
#include "Network/NetworkWiiMediator.h"
#include "Network/net_session_close.h"
#include "Pl/pl_gesture_req.h"
#include "vi_next_frame_buffer.h"
#include "userdata_arena_row_get.h"
#include "hud/draw_uv_rect.h"
#include "homebutton/hbmPanelSetInputEnabled.h"
#include "homebutton/hbmPanelClearKanaQueue.h"
#include "homebutton/hbmKbdIsTextEmpty.h"
#include "homebutton/hkbNodeAppend.h"
#include "homebutton/hkbNodeRemove.h"
#include "menu/kbd_modifier_bits.h"
#include "sys_font_code_table.h"
#include "menu/menu_sysmsg.h"

/* The sign panel's page (slot 0x40): its inline copy is this TU's (0x80454970). */
inline u8 HbmAnimPanelBase::getPage() { return mPage; }

/* ---------------------------------------------------------------------------------------------------------------- */
/* The inline helpers the driver calls out of line. */

extern "C" {
/* Creates an expandable heap with the default attributes. */
/* untyped: byte range - the heap's backing memory */
static inline MEMiHeapHead* MEMCreateExpHeap(void* start, u32 size)
{
    return (MEMiHeapHead*)MEMCreateExpHeapEx(start, size, 0);
}

/* Returns the largest block the heap hands out at the default alignment. */
static inline u32 MEMGetAllocatableSizeForExpHeap(MEMiHeapHead* heap)
{
    return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}
}

struct CNode;

/* The HKB manager's listener list head (its +0x80).  size: 0x8C (approximation: the head reached) */
struct SysKbdHkbView {
    /* +0x00 */ u8 unused_0x00[0x80];
    /* +0x80 */ u8 listeners[0xC];
};

/* Returns the HKB manager singleton. */
inline HkbManager* sysKbdHkbManager(void)
{
    return &hkbManager;
}

/* Returns the head of the manager's listener list. */
inline CNode* sysKbdHkbListeners(HkbManager* manager)
{
    return (CNode*)((SysKbdHkbView*)manager)->listeners;
}

/* The title hook the keyboard's title goes to (none in this build).  size: 0x4 (approximation) */
class SysKbdTitleHook {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void setTitle(const u16* title, s32 flags);
};

/* Returns the title hook; the build compiled its body out, so it returns whatever r3 holds. */
inline SysKbdTitleHook* sysKbdTitleHook(void) {}

/* The callees retail calls with another argument or return type than their declaration: the gesture request with an
 * unsigned byte, the screen mode read back as a byte. */
#define pl_gesture_req_c1 ((void (*)(u8))pl_gesture_req)
#define ck_WideMode_c1 ((u8 (*)(void))ck_WideMode)

#pragma dont_inline on
#pragma no_static_dtors on
#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------------------- */
/* The driver's state. */

/* One key's position on the keyboard layout (`.rodata` 0x80572240 [17][6] and 0x805723D8 [4][4]).  size: 0x4 */
struct KbdKeyPos {
    /* +0x0 */ s16 x;
    /* +0x2 */ s16 y;
};

/* A pointer position on the screen.  size: 0x10 */
struct KbdCursor {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ u8 unused_0x8[0x8];
};

/* A pointer position built for one command.  size: 0xC */
struct KbdPoint {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
};

/* The listener and event handler classes (their tables are this unit's `.data` 0x8060E700..0x8060E790). */

/* The HKB manager's listener: a node of the manager's listener list (+0x80 of `hkbManager`) whose slots receive the
 * connect, disconnect and key notifications.  size: 0x10 */
class SysKbdListenerBase {
public:
    SysKbdListenerBase() : prev(0), next(0) {}
    /* slot +0x008 */ virtual ~SysKbdListenerBase() {}
    /* slot +0x00C */ virtual void onConnect(SysKbdKeyEvent* event) {}
    /* slot +0x010 */ virtual void onDisconnect(SysKbdKeyEvent* event) {}
    /* slot +0x014 */ virtual void onKey(SysKbdKeyEvent* event) {}

    /* +0x04 */ SysKbdListenerBase* prev;
    /* +0x08 */ SysKbdListenerBase* next;
    /* +0x0C */ HkbManager* manager;
};

/* The keyboard object's event handler: the slots the keyboard calls when it plays a sound, finishes, cancels or moves
 * its cursor.  size: 0x4 */
class SysKbdHandlerBase {
public:
    /* slot +0x008 */ virtual void onInit() {}
    /* slot +0x00C */ virtual void onCalc() {}
    /* slot +0x010 */ virtual void onDraw() {}
    /* slot +0x014 */ virtual void onSound(s32 id) {}
    /* slot +0x018 */ virtual void onCancel() {}
    /* slot +0x01C */ virtual void onSubmit() {}
    /* slot +0x020 */ virtual void onClose() {}
    /* slot +0x024 */ virtual void onGesture(s32 index) {}
    /* slot +0x028 */ virtual void onGestureClose() {}
    /* slot +0x02C */ virtual void onCursorMode(s32 mode) {}
};

/* The driver's handler: plays the keyboard's sounds, ends the input and follows its cursor mode.  size: 0x4 */
class SysKbdHandler : public SysKbdHandlerBase {
public:
    /* slot +0x014 */ virtual void onSound(s32 id);
    /* slot +0x018 */ virtual void onCancel();
    /* slot +0x01C */ virtual void onSubmit();
    /* slot +0x024 */ virtual void onGesture(s32 index);
    /* slot +0x028 */ virtual void onGestureClose();
    /* slot +0x02C */ virtual void onCursorMode(s32 mode);
};

/* The driver's listener: counts the modifier keys and fills the request record from the key events.  size: 0x10 */
class SysKbdListener : public SysKbdListenerBase {
public:
    /* slot +0x008 */ virtual ~SysKbdListener();
    /* slot +0x00C */ virtual void onConnect(SysKbdKeyEvent* event);
    /* slot +0x010 */ virtual void onDisconnect(SysKbdKeyEvent* event);
    /* slot +0x014 */ virtual void onKey(SysKbdKeyEvent* event);
};

extern "C" {
u8 kbd_unused_flag;                 /* never read or written by this unit's code */
u8 kbd_backspace_repeat;            /* frames to the next backspace repeat */
u8 kbd_last_usage;                  /* the HID usage of the last key pressed while the keyboard was closed */
u8 kbd_caps_lock;                   /* the caps flag of the keyboard's settings */
s8 kbd_unlock_delay;                /* frames before the text panel takes input again */
s8 kbd_close_delay;                 /* frames the closing keyboard keeps stepping */
u8 kbd_submit;                      /* 1: the text was submitted (the step finishes the input) */
u8 kbd_key_forwarded;               /* 1: a key was forwarded to the keyboard this frame */
SysKbdView* kbd_object;             /* the keyboard object (null while it is not created) */
SysKbdSetting kbd_setting;          /* the keyboard's settings */
s32 kbd_created;                    /* 1: `kbd_init` created the keyboard object */
s32 kbd_opening;                    /* 1 while `kbd_open_text` sets the keyboard up */
s32 kbd_closing;                    /* 1 once `kbd_close` started closing the keyboard */
s32 kbd_wait;                       /* frames the step waits before it reads input */
s32 kbd_gesture_index;              /* the gesture the gesture list picked */
s32 kbd_cursor_mode;                /* 0: off, 1: the keyboard cursor, 2: the gesture list cursor */
s32 kbd_cursor_y;                   /* the keyboard cursor's row */
u8 kbd_wide_mode;                   /* the `ck_WideMode` the panel was laid out for */
s8 kbd_result;                      /* -1: the input was cancelled */
s32 kbd_heap_size;                  /* the keyboard heap's size */
u32 kbd_heap_base;                  /* the keyboard heap's base address */
char* kbd_dest_text;                /* the caller's text buffer (`set_kbd_param`) */
u8 kbd_hkb_ready;                   /* 1: the HKB manager is initialised and the listener registered */
u8 kbd_target_slot;                 /* the session slot the whisper goes to */
SysKbdRequest* kbd_request;         /* the request record (`kbd_request_buf`) */
s8 kbd_gesture_hidden;              /* 1: the gesture list was closed by the cursor mode */
s32 kbd_step_state;                      /* 0: input, 1: closing */
s32 kbd_active;                     /* 1: the keyboard is open (2 after its first step) */
s32 kbd_open_arg;                   /* `kbd_open_text`'s last argument */
s32 kbd_open_arg_copy;              /* the same, kept */
s32 kbd_shown;                      /* 1: the keyboard was opened and not closed */
s32 kbd_pad_skip;                   /* 1: the pad input of this frame was consumed */
s32 kbd_cursor_jump;                /* 1/2: the cursor jumps to the OK/the mode key */
s8 kbd_ctrl_count;                  /* the Ctrl keys held */
s8 kbd_shift_count;                 /* the Shift keys held */
s8 kbd_alt_count;                   /* the Alt keys held */
s8 kbd_list_flag;                   /* cleared when the gesture list closes */
s8 kbd_se_enabled;                  /* 1: the keyboard's sound requests are played */
s32 kbd_panel_animating;            /* the text panel's animation state last frame */
u8 kbd_panel_idle_count;            /* frames the panel has been idle */
s8 kbd_panel_idle;                  /* 1: the panel has been idle for three frames */
u8 kbd_target_cycle;                /* 0: chat to all, 1: to the party, 2..: whisper to a session slot */
KbdCursor kbd_cursor_pos;           /* the pad cursor's screen position */
u16 kbd_text[0x200];                /* the text the keyboard edits (UTF-16) */
char kbd_utf8_text[0x48];           /* the chat text posted (UTF-8) */
u16 kbd_title[0x10];                /* the chat target title */
char kbd_target_id[0x10];           /* the whisper target's net id */
SysKbdRequest kbd_request_buf;      /* the request record `kbd_request` points at */
s32 kbd_pad[4];                     /* the pad's held, pressed, released and repeated keyboard bits */
KbdPoint kbd_list_cursor;           /* the gesture list cursor's screen position */
char kbd_ascii_text[0x14];          /* the text the OK key compares with */
SysKbdListener kbd_listener;        /* the listener `kbd_init` registers with the HKB manager */
MEMAllocator kbd_allocator;         /* the keyboard heap's allocator */
SysKbdHandler kbd_handler;          /* the handler the keyboard object is created with */
nw4r::lyt::MultiArcResourceAccessor* kbd_resources; /* the keyboard layout's archive accessor */
nw4r::ut::RomFont* kbd_font;        /* the system font the layout draws with */
MEMiHeapHead* kbd_heap;             /* the keyboard heap */
s32 kbd_key_count;                  /* the soft keyboard's mode last frame */
s32 kbd_key_change_delay;           /* frames to the text panel refresh after a mode change */
s32 kbd_pointer_channel;            /* the pointer channel handed to the step */

/* The phrase buffers the keyboard's gesture list shows (the player's chat phrases, converted when it opens) and the
 * gesture name buffers, with their tables. */
u16 kbd_phrase_buf_00[46] = {
    0xFF11, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_01[46] = {
    0xFF12, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_02[46] = {
    0xFF13, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_03[46] = {
    0xFF14, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_04[46] = {
    0xFF15, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_05[46] = {
    0xFF16, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_06[46] = {
    0xFF17, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_07[46] = {
    0xFF18, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_08[46] = {
    0xFF19, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_09[46] = {
    0xFF10, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_10[46] = {
    0xFF21, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16 kbd_phrase_buf_11[46] = {
    0xFF22, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x3000, 0x3000, 0x3000, 0x0000
};
u16* kbd_phrase_names[12] = {
    kbd_phrase_buf_00, kbd_phrase_buf_01, kbd_phrase_buf_02, kbd_phrase_buf_03, kbd_phrase_buf_04,
    kbd_phrase_buf_05, kbd_phrase_buf_06, kbd_phrase_buf_07, kbd_phrase_buf_08, kbd_phrase_buf_09,
    kbd_phrase_buf_10, kbd_phrase_buf_11
};
u16 kbd_gesture_label_buf_00[11] = {
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_01[11] = {
    0x0031, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_02[11] = {
    0x0032, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_03[11] = {
    0x0033, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_04[11] = {
    0x0034, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_05[11] = {
    0x0035, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_06[11] = {
    0x0036, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_07[11] = {
    0x0037, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_08[11] = {
    0x0038, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_09[11] = {
    0x0039, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_10[11] = {
    0x0061, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16 kbd_gesture_label_buf_11[11] = {
    0x0062, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0000
};
u16* kbd_gesture_labels[12] = {
    kbd_gesture_label_buf_00, kbd_gesture_label_buf_01, kbd_gesture_label_buf_02, kbd_gesture_label_buf_03,
    kbd_gesture_label_buf_04, kbd_gesture_label_buf_05, kbd_gesture_label_buf_06, kbd_gesture_label_buf_07,
    kbd_gesture_label_buf_08, kbd_gesture_label_buf_09, kbd_gesture_label_buf_10, kbd_gesture_label_buf_11
};

u8 kbd_gesture_table[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; /* the gesture each phrase slot plays */

u8 kbd_text_kind = 0xFF;            /* the kana flag of the open text kind (0xFF: closed) */
s32 kbd_cursor_x = 2;               /* the keyboard cursor's column */
s32 kbd_list_x = 3;                 /* the gesture list cursor's column */
s32 kbd_list_y = 3;                 /* the gesture list cursor's row */
s32 kbd_prev_x = -1;                /* the cursor's column before its last move */
s32 kbd_prev_y = -1;                /* the cursor's row before its last move */
s16 kbd_offset_y = -20;             /* the keyboard's vertical offset */
s32 kbd_max_length = 0x400;         /* the caller's text length */
char* kbd_target_formats[3] = {"", "/quest_member:", "/%s:"}; /* the chat target title formats */
u8 kbd_setup_type = 0xFF;           /* the heap half `kbd_init` set up (0xFF: none) */
u8 kbd_text_mode = 0xFF;            /* the open text kind (0: chat) */
char kbd_arc_root[] = "arc";        /* the layout archive's resource root */
#pragma explicit_zero_data on
u16 kbd_empty_title[4] = {0};       /* the empty title and text (zero, kept in .sdata) */
#pragma explicit_zero_data off


/* The keyboard's key positions [column][row] and the gesture list's [column][row]. */
const KbdKeyPos kbd_key_pos[17][6] = {
    {{-187, -140}, {-187, -122}, {-187, -104}, {-187, -86}, {0, 0}, {0, 0}},
    {{-165, -140}, {-165, -122}, {-165, -104}, {-165, -86}, {0, 0}, {0, 0}},
    {{-140, -140}, {-140, -122}, {-140, -104}, {-140, -86}, {-123, -68}, {0, -170}},
    {{-118, -140}, {-118, -122}, {-118, -104}, {-118, -86}, {-123, -68}, {0, -170}},
    {{-96, -140}, {-96, -122}, {-96, -104}, {-96, -86}, {-123, -68}, {0, -170}},
    {{-74, -140}, {-74, -122}, {-74, -104}, {-74, -86}, {-68, -68}, {0, -170}},
    {{-52, -140}, {-52, -122}, {-52, -104}, {-52, -86}, {-68, -68}, {0, -170}},
    {{-26, -140}, {-26, -122}, {-26, -104}, {-26, -86}, {-11, -68}, {0, -170}},
    {{-4, -140}, {-4, -122}, {-4, -104}, {-4, -86}, {-11, -68}, {0, -170}},
    {{16, -140}, {16, -122}, {16, -104}, {16, -86}, {-11, -68}, {0, -170}},
    {{38, -140}, {38, -122}, {38, -104}, {38, -86}, {45, -68}, {0, -170}},
    {{60, -140}, {60, -122}, {60, -104}, {60, -86}, {45, -68}, {0, -170}},
    {{85, -140}, {85, -122}, {85, -104}, {85, -86}, {102, -68}, {0, -170}},
    {{107, -140}, {107, -122}, {107, -104}, {107, -86}, {102, -68}, {0, -170}},
    {{129, -140}, {129, -122}, {129, -104}, {129, -86}, {102, -68}, {0, -170}},
    {{150, -140}, {150, -122}, {150, -104}, {150, -86}, {157, -68}, {0, -170}},
    {{172, -140}, {172, -122}, {172, -104}, {172, -86}, {157, -68}, {0, -170}},
};
const KbdKeyPos kbd_list_pos[4][4] = {
    {{-160, -130}, {-160, -95}, {-160, -65}, {0, 0}},
    {{-65, -130}, {-65, -95}, {-65, -65}, {0, 0}},
    {{30, -130}, {30, -95}, {30, -65}, {0, 0}},
    {{120, -130}, {120, -95}, {120, -65}, {120, -30}},
};
}

/* Ignores a keyboard connection (retail's copy is the row 0x804513F4, in the range registered to menu/menu_plsearch). */
void SysKbdListener::onConnect(SysKbdKeyEvent* event) {}

/* Ignores a keyboard disconnection (retail's copy is the row 0x804513F8). */
void SysKbdListener::onDisconnect(SysKbdKeyEvent* event) {}

/* Forward declarations of the driver's functions (in address order below). */
void kbd_screen_mode_update(void);
int sys_kbd_close(void);
void kbd_pad_dispatch(s32* trigger, s32* held, s32* release);
int kbd_step(s32 channel, u16 trigger, u16 held, u16 release);
void kbd_command_step(void);
int kbd_close_step(void);
void kbd_target_cycle_step(void);
void kbd_text_clear(void);
void kbd_phrase_insert(u32 index);
void kbd_draw(void);

/* ---------------------------------------------------------------------------------------------------------------- */
/* The HKB keyboard listener. */
/* 0x804513FC (0x434): Counts the modifier keys, then turns the key into the request's key, modifier and command
 * bytes. */
void SysKbdListener::onKey(SysKbdKeyEvent* event)
{
    resetIdleFrames();
    if (event->type == 1) {
        kbd_last_usage = event->usage;
    }
    if (event->type & 1) {
        switch (event->usage) {
        case 0xE0:
        case 0xE4:
            kbd_ctrl_count++;
            break;
        case 0xE1:
        case 0xE5:
            kbd_shift_count++;
            break;
        case 0xE2:
        case 0xE6:
            kbd_alt_count++;
            break;
        }
    } else if (!(event->type & 1)) {
        switch (event->usage) {
        case 0xE0:
        case 0xE4:
            kbd_ctrl_count--;
            break;
        case 0xE1:
        case 0xE5:
            kbd_shift_count--;
            break;
        case 0xE2:
        case 0xE6:
            kbd_alt_count--;
            break;
        }
    }
    if (kbd_opening != 0) {
        return;
    }
    if (kbd_active == 0) {
        if (system_w.menu_closed_0x8c0 == 0) {
            kbd_request_buf.key = 0;
            kbd_request_buf.modifiers = 0;
        } else if (event->type == 1) {
            kbd_request_buf.key = event->usage;
            kbd_request_buf.modifiers = event->modifiers;
            if (kbd_request_buf.flags & 1) {
                kbd_request_buf.flags = (kbd_request_buf.flags | 2) & 0xFE;
            }
        }
        return;
    }
    if (event->type == 1) {
        u8 key = event->usage;
        kbd_request_buf.key = key;
        kbd_request_buf.modifiers = event->modifiers;
        switch ((u32)kbd_request->mode) {
        case 0:
            if ((kbd_request_buf.modifiers & 0x2F) == 2 && key >= 0x3A && key <= 0x45) {
                pl_gesture_req_c1(kbd_gesture_table[(u8)(key - 0x3A)] + 1);
                sysSE_req(0x37);
            }
        case 4:
            if (kbd_request_buf.modifiers == 0) {
                switch (kbd_request_buf.key) {
                case 43:
                    kbd_request->command = 43;
                    break;
                }
            }
            break;
        }
        SysKbdRequest* req = kbd_request;
        if (req->mode <= 1 || req->mode == 4) {
            if ((kbd_request_buf.modifiers & 0x2F) == 1 && kbd_request->key >= 58 && kbd_request->key <= 69) {
                req->command = kbd_request->key;
                kbd_request->command_kind = 1;
            }
            req = kbd_request;
            if (req->modifiers == 0) {
                key = req->key;
                switch (key) {
                case 61:
                    if (kbd_cursor_mode != 2) {
                        if (!kbd_object->getPanel()->getModeView()->isHalfWidth()) {
                            kbd_request->gesture_key = 61;
                        }
                    } else {
                        req->command = 61;
                    }
                    kbd_request->gesture_open = 0;
                    break;
                case 79:
                case 80:
                case 81:
                case 82:
                    req->command = key;
                    break;
                case 88:
                case 40:
                    req->command = 40;
                    break;
                }
            }
        }
        switch (kbd_request_buf.key) {
        case 42:
            kbd_request->command = 42;
            kbd_backspace_repeat = 30;
            break;
        }
        switch (kbd_request->key) {
        case 41:
            kbd_request->command = 41;
            kbd_request->command_kind = 0;
            break;
        }
        return;
    }
    kbd_request_buf.key = event->usage;
    kbd_request_buf.modifiers = event->modifiers;
    if ((u32)(kbd_cursor_mode - 1) <= 1 && kbd_request->modifiers == 0) {
        switch (kbd_request->key) {
        case 42:
            if ((u8)--kbd_backspace_repeat == 0) {
                kbd_request->command = 42;
                kbd_backspace_repeat = 10;
            }
            break;
        }
    }
}

/* Destroys the listener. */
SysKbdListener::~SysKbdListener() {}

/* ---------------------------------------------------------------------------------------------------------------- */
/* Creating and destroying the keyboard. */

/* 0x804518E8 (0x358): Loads the keyboard layout archive into the keyboard heap's top half (`slot` 0) or the lower
 * one, and attaches it and the system font to the layout resource accessor. */
void kbd_layout_load(u8 slot)
{
    if (slot == 0) {
        load_file("00/sofkeybd_eur.arc", 0x90648000, 0xA9F60);
        nw4r::lyt::ArcResourceLink* link = new ((void*)0x906F6760) nw4r::lyt::ArcResourceLink;
        link->Set((void*)0x90648000, kbd_arc_root);
        kbd_resources = new ((void*)0x906F6820) nw4r::lyt::MultiArcResourceAccessor;
        kbd_resources->Attach(link);
        kbd_font = new ((void*)0x906F68C0) nw4r::ut::RomFont;
        kbd_font->Load(sys_font_code_table, (void*)system_w.tex_0x96c[1].field_0x24, 512, 512);
        nw4r::lyt::FontRefLink* font = new ((void*)0x906F6940) nw4r::lyt::FontRefLink;
        font->Set("WiiBitmapFontType1.brfnt", kbd_font);
        kbd_resources->RegistFont(font);
    } else {
        load_file("00/sofkeybd_eur.arc", 0x90348000, 0xA9F60);
        nw4r::lyt::ArcResourceLink* link = new ((void*)0x903F6760) nw4r::lyt::ArcResourceLink;
        link->Set((void*)0x90348000, kbd_arc_root);
        kbd_resources = new ((void*)0x903F6820) nw4r::lyt::MultiArcResourceAccessor;
        kbd_resources->Attach(link);
        kbd_font = new ((void*)0x903F68C0) nw4r::ut::RomFont;
        kbd_font->Load(sys_font_code_table, (void*)system_w.tex_0x96c[1].field_0x24, 512, 512);
        nw4r::lyt::FontRefLink* font = new ((void*)0x903F6940) nw4r::lyt::FontRefLink;
        font->Set("WiiBitmapFontType1.brfnt", kbd_font);
        kbd_resources->RegistFont(font);
    }
}

/* 0x80451CB0 (0x68): Initialises the HKB manager over `work` and registers the key listener, once. */
void kbd_hkb_init(u8* work)
{
    if (kbd_hkb_ready == 0) {
        HkbManager* manager = sysKbdHkbManager();
        hkbNodeAppend(sysKbdHkbListeners(manager), (CNode*)&kbd_listener);
        hkbManagerInit(manager, work);
        kbd_hkb_ready = 1;
    }
}

/* 0x80451D2C (0x88): Fills the keyboard settings with the driver's defaults; `keep_caps` keeps the caps flag. */
void kbd_setting_init(SysKbdSetting* setting, s8 keep_caps)
{
    setting->version = 0x11;
    setting->flags.id = 0xFA;
    setting->flags.field_0x1 = 0;
    setting->flags.field_0x2 = 0;
    setting->flags.enabled = 1;
    setting->flags.layout = 2;
    setting->flags.kana = 0;
    setting->flags.shift = 0;
    setting->style.panel = 1;
    setting->style.style = 0;
    if (keep_caps != 0) {
        setting->flags.caps = kbd_caps_lock;
    }
    setting->style.field_0x1 = 0;
    setting->field_0x7 = 0;
}

/* 0x80451DB4 (0x7C): Plays the text panel's title animation and hands the title to the title hook. */
void kbd_title_show(const u16* title)
{
    kbd_object->getPanel()->mTitle.playAnim("T_title_text");
    SysKbdTitleHook* hook = sysKbdTitleHook();
    if (hook != 0) {
        hook->setTitle(title, 0);
    }
}

/* 0x80451E34 (0x8): Returns the heap half `kbd_init` set the keyboard up in (0xFF: none). */
u8 sys_get_kbd_setup_type(void)
{
    return kbd_setup_type;
}

/* 0x80451E3C (0x24): Clears the request record's key, command and modifier bytes. */
void sys_kbd_reset(void)
{
    kbd_request_buf.key = 0;
    kbd_request_buf.key_held = 0;
    kbd_request_buf.modifiers = 0;
    kbd_request_buf.command = 0;
    kbd_request_buf.command_kind = 0;
}

/* 0x80451E60 (0x7C): Closes the keyboard, shuts the HKB manager down and forgets the keyboard object. */
void sys_kbd_exit(void)
{
    sys_kbd_close();
    if (kbd_hkb_ready != 0) {
        HkbManager* manager = sysKbdHkbManager();
        hkbNodeRemove((HkbNode*)sysKbdHkbListeners(manager), (HkbNode*)&kbd_listener);
        hkbManagerShutdown(manager);
        kbd_hkb_ready = 0;
    }
    if (kbd_object != 0) {
        kbd_object = 0;
        kbd_created = 0;
    }
    kbd_setup_type = 0xFF;
}

/* 0x80451EDC (0x310): Creates the keyboard heap, the HKB manager, the layout resources and the keyboard object in the
 * heap half `slot` names (nothing when it is already set up there). */
void sys_kbd_init(u8 slot)
{
    kbd_se_enabled = 0;
    memset(&kbd_request_buf, 0, sizeof(SysKbdRequest));
    kbd_request = &kbd_request_buf;
    if (kbd_setup_type != slot) {
        if (kbd_object != 0) {
            kbd_object = 0;
            kbd_created = 0;
        }
        kbd_setup_type = slot;
        if (kbd_heap != 0) {
            MEMDestroyExpHeap(kbd_heap);
            kbd_heap = 0;
        }
        kbd_heap = MEMCreateExpHeap((void*)0x92AA1C00, 0xD6400);
        MEMInitAllocatorForExpHeap(&kbd_allocator, kbd_heap, 8);
        OSReport("LYTmemAlloc %x\n", MEMGetAllocatableSizeForExpHeap(kbd_heap));
        kbd_heap_base = (slot == 0) ? 0x906F6760 : 0x903F6760;
        kbd_heap_size = 0xD18A0;
        kbd_setting_init(&kbd_setting, 1);
        if (slot == 0) {
            kbd_hkb_init((u8*)0x906F1F60);
        } else {
            kbd_hkb_init((u8*)0x903F1F60);
        }
        kbd_layout_load(slot);
        nw4r::lyt::Layout::SetAllocator(&kbd_allocator);
        OSReport("LYTmemAlloc %x\n", MEMGetAllocatableSizeForExpHeap(kbd_heap));
        void* place = (slot == 0) ? (void*)0x906F6840 : (void*)0x903F6840;
        kbd_object = new (place) SysKbdView(&kbd_allocator, kbd_resources, &kbd_handler);
        kbd_object->setInputStyle(1);
        switch ((u8)system_w.field_0x09) {
        case 3:
            kbd_object->setLanguage(4);
            break;
        case 2:
            kbd_object->setLanguage(3);
            break;
        case 5:
            kbd_object->setLanguage(6);
            break;
        case 4:
            kbd_object->setLanguage(5);
            break;
        default:
            kbd_object->setLanguage(1);
            break;
        }
        kbd_object->setVisible(0);
        OSReport("LYTmemAlloc %x\n", MEMGetAllocatableSizeForExpHeap(kbd_heap));
        kbd_created = 1;
    }
}

/* 0x8045227C (0x64): Lays the keyboard out for the current screen mode. */
void kbd_screen_mode_update(void)
{
    kbd_wide_mode = ck_WideMode_c1();
    if (kbd_wide_mode == 0) {
        kbd_object->setStandardScreen(1);
    } else {
        kbd_object->setStandardScreen(0);
    }
}

/* 0x804522E0 (0x668): Opens the keyboard for input mode `mode` and text kind `kind`: resets the request, sets the
 * keyboard up for the language, fills the phrase and gesture lists and the text, and starts the step. */
void kbd_open_text(s32 mode, s32 kind, s32 arg)
{
    char label[0x80];
    s32 dstLength;
    s32 srcLength;
    s32 length;
    char* phrase;
    s32 i;
    s32 j;

    kbd_opening = 1;
    kbd_result = 0;
    kbd_se_enabled = 1;
    if (kind == 6) {
        mode = 3;
    }
    kbd_open_arg = arg;
    kbd_open_arg_copy = arg;
    memset(kbd_ascii_text, 0, sizeof(kbd_ascii_text));
    kbd_request->mode = mode;
    kbd_request->key = 0;
    kbd_request->modifiers = 0;
    kbd_request->kind = kind;
    kbd_pad_skip = 0;
    kbd_request->command_kind = 0;
    kbd_request->command = 0;
    kbd_request->gesture_key = 0;
    kbd_gesture_hidden = 0;
    kbd_object->setInputStyle(1);
    switch (system_w.field_0x09) {
    case 3:
        kbd_object->setLanguage(4);
        break;
    case 2:
        kbd_object->setLanguage(3);
        break;
    case 5:
        kbd_object->setLanguage(6);
        break;
    case 4:
        kbd_object->setLanguage(5);
        break;
    default:
        kbd_object->setLanguage(1);
        break;
    }
    if (mode != 3) {
        if (kbd_request->mode == 2) {
            kbd_object->setNameEntry(1);
        } else {
            kbd_object->setNameEntry(0);
        }
    } else if (kind == 6) {
        kbd_object->setGestureEntry(1);
    } else {
        kbd_object->setGestureEntry(0);
    }
    kbd_object->setOffsetY(kbd_offset_y);
    kbd_setting_init(&kbd_setting, 0);
    kbd_setting.style.style = 6;
    if (kbd_text_kind == 0) {
        kbd_setting.flags.kana = 0;
    } else {
        kbd_setting.flags.kana = 1;
    }
    SysKbdSetting setting = kbd_setting;
    kbd_object->setSetting(&setting);
    kbd_object->open();
    kbd_object->setActive(1);
    kbd_object->setCandidateMode(false);
    if (mode != 3) {
        for (i = 0; i < 12; i++) {
            phrase = userdata_arena_row_get(i);
            dstLength = 45;
            srcLength = strlen(phrase);
            memset(kbd_phrase_names[i], 0, 90);
            netMessageUtf8ToUtf16(kbd_phrase_names[i], &dstLength, (const u8*)phrase, &srcLength);
        }
        kbd_object->getList()->setPhraseTexts((const wchar_t**)kbd_phrase_names);
        for (j = 0; j < 12; j++) {
            strcpy(label, (char*)get_menu_msg(0x2C, (u16)j));
            char* end = flKnjMsgNumPtr(label, 10);
            if (end != 0) {
                *end = 0;
            }
            memset(kbd_gesture_labels[j], 0, 20);
            length = 20;
            netMessageUtf8ToUtf16(kbd_gesture_labels[j], &length, (const u8*)label, 0);
        }
        kbd_object->getList()->setGestureTexts((const wchar_t**)kbd_gesture_labels);
    }
    kbd_object->setLineCount(10);
    kbd_cursor_pos.x = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].x;
    kbd_cursor_pos.y = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].y - kbd_offset_y;
    kbd_closing = 0;
    kbd_opening = 0;
    kbd_submit = 0;
    kbd_title_show(kbd_empty_title);
    kbd_object->setTitle(kbd_empty_title);
    kbd_target_cycle = 0;
    if (mode == 0) {
        kbd_dest_text = 0;
        memset(kbd_text, 0, sizeof(kbd_text));
        kbd_max_length = 60;
        kbd_object->setMaxLength(60);
        kbd_object->setMaxBytes(60);
        kbd_object->setText(kbd_text);
    } else if (kbd_dest_text != 0) {
        s32 dstLength = kbd_max_length;
        memset(kbd_text, 0, sizeof(kbd_text));
        netMessageUtf8ToUtf16(kbd_text, &dstLength, (const u8*)kbd_dest_text, 0);
        kbd_object->setMaxLength(kbd_max_length);
        kbd_object->setMaxBytes(kbd_max_length);
        kbd_object->setText(kbd_text);
    }
    kbd_active = 1;
    kbd_shown = 1;
    kbd_request->gesture_open = 0;
    if (kbd_last_usage != 0) {
        HkbManager* manager = sysKbdHkbManager();
        hkbManagerUpdate(manager);
        kbd_object->getInput()->inputKey(manager, kbd_last_usage, 0);
        kbd_last_usage = 0;
    }
    kbd_unlock_delay = 1;
    kbd_screen_mode_update();
}

/* 0x804529E0 (0x228): Draws the keyboard over the frame (after copying the frame buffer) and its pad cursor. */
void kbd_draw(void)
{
    s16 x;
    s16 y;

    GXSetColorUpdate(0);
    GXSetAlphaUpdate(0);
    GXSetZMode(1, 7, 1);
    GXCopyDisp(vi_next_frame_buffer(), 1);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    set_zmode(0, 0, 0);
    kbd_object->draw();
    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    if (kbd_active != 0 && kbd_step_state == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0) {
        switch (kbd_cursor_mode) {
        case 1:
            x = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].x;
            y = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].y - kbd_offset_y;
            break;
        case 2:
            x = kbd_list_pos[kbd_list_x][kbd_list_y].x;
            y = kbd_list_pos[kbd_list_x][kbd_list_y].y - kbd_offset_y;
            break;
        default:
            return;
        }
        if (ck_WideMode() != 0) {
            x = x + (s32)(0.5f * kbd_cursor_x) + 418;
        } else {
            x = x + (s32)(f32)kbd_cursor_x + 304;
        }
        draw_uv_rect(x, y + 228, 32, 32, 0xFFFFFFF0, 0);
    }
}

/* Returns whether the soft keyboard shows the half-width set. */
s32 HkbKeyboardBase::isHalfWidth() { return mKeyState.isHalfWidth(); }

/* 0x80452C28 (0xB8): Takes the caller's text buffer and its length and shows the text in the keyboard. */
void sys_set_kbd_param(char* text, u32 length)
{
    s32 dstLength;

    kbd_dest_text = text;
    kbd_max_length = length;
    dstLength = length;
    memset(kbd_text, 0, sizeof(kbd_text));
    netMessageUtf8ToUtf16(kbd_text, &dstLength, (const u8*)text, 0);
    kbd_object->setMaxLength(length);
    kbd_object->setMaxBytes(length);
    kbd_object->setText(kbd_text);
}

/* 0x80452CE0 (0x158): Opens the keyboard for the text kind in `type` (bit 7: the kana flag); returns 1 once it is
 * open, 0 while it waits. */
int sys_kbd_open(u8 type)
{
    u8 mode;
    s32 kind;
    s32 length;

    if (kbd_created == 0) {
        return 0;
    }
    if (kbd_wait > 0) {
        kbd_wait--;
        return 0;
    }
    if (kbd_key_forwarded == 1) {
        kbd_key_forwarded = 0;
    }
    if (kbd_text_kind != 0xFF && kbd_closing == 0) {
        return 1;
    }
    kbd_text_kind = (type & 0x80) != 0;
    u8 text = type & 0x7F;
    kbd_gesture_index = 0;
    kbd_cursor_mode = 0;
    kbd_cursor_x = 2;
    kbd_cursor_y = 0;
    kbd_list_x = 3;
    kbd_list_y = 3;
    kbd_prev_x = -1;
    kbd_prev_y = -1;
    kbd_wide_mode = 0;
    kbd_offset_y = -20;
    kbd_result = 0;
    kbd_text_mode = text;
    length = kbd_max_length;
    switch (text) {
    case 0:
        mode = 2;
        kind = 16;
        break;
    case 1:
        mode = 0;
        kind = 14;
        break;
    case 2:
        mode = 0;
        kind = 6;
        break;
    case 3:
        mode = 3;
        kind = 15;
        break;
    case 4:
        mode = 3;
        kind = 6;
        break;
    }
    kbd_open_text(mode, kind, length);
    kbd_wait = 15;
    return 1;
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* Stepping the keyboard. */

/* 0x80452E38 (0x3E0): Reads pad 0's four button words (the Classic Controller's or the Wii Remote's) into the
 * keyboard's button bits. */
void kbd_pad_read(void)
{
    u16 buttons[4];
    s32 bits[4];
    u16* button;
    s32* bit;
    s32 i;

    memset(bits, 0, sizeof(bits));
    if ((u8)get_ControlType(0) == 2) {
        buttons[0] = Psw[0].control_0x0D6;
        buttons[1] = Psw[0].field_0xda;
        buttons[2] = Psw[0].cc_word2_0x0DC;
        buttons[3] = Psw[0].field_0xf8;
        button = buttons;
        bit = bits;
        for (i = 4; i != 0; i--) {
            if (*button & 0x2000) {
                *bit |= 1;
            }
            if (*button & 0x1000) {
                *bit |= 2;
            }
            if (*button & 0x800) {
                *bit |= 4;
            }
            if (*button & 0x400) {
                *bit |= 8;
            }
            if (*button & 0x20) {
                *bit |= 0x10;
            }
            if (*button & 0x40) {
                *bit |= 0x20;
            }
            if (*button & 0x8000) {
                *bit |= 0x40;
            }
            if (*button & 0x4000) {
                *bit |= 0x80;
            }
            if (*button & 0x100) {
                *bit |= 0x800;
            }
            if (*button & 0x200) {
                *bit |= 0x1000;
            }
            if (*button & 0x80) {
                *bit |= 0x4000;
            }
            if (*button & 8) {
                *bit |= 0x8000;
            }
            if (*button & 0x10) {
                *bit |= 0x10000;
            }
            if (*button & 4) {
                *bit |= 0x20000;
            }
            button++;
            bit++;
        }
    } else if ((u8)get_ControlType(0) == 1) {
        buttons[0] = Psw[0].control_0x0FC;
        buttons[1] = Psw[0].field_0x100;
        buttons[2] = Psw[0].wpad_word2_0x102;
        buttons[3] = Psw[0].field_0x11e;
        button = buttons;
        bit = bits;
        for (i = 4; i != 0; i--) {
            if (*button & 0x2000) {
                *bit |= 1;
            }
            if (*button & 0x1000) {
                *bit |= 2;
            }
            if (*button & 0x800) {
                *bit |= 4;
            }
            if (*button & 0x400) {
                *bit |= 8;
            }
            if (*button & 0x200) {
                *bit |= 0x10;
            }
            if (*button & 0x100) {
                *bit |= 0x20;
            }
            if (*button & 0x40) {
                *bit |= 0x1000;
            }
            if (*button & 0x80) {
                *bit |= 0x800;
            }
            if (*button & 0x20) {
                *bit |= 0x10000;
            }
            if (*button & 0x10) {
                *bit |= 0x80;
            }
            if (*button & 4) {
                *bit |= 0x4000;
            }
            if (*button & 8) {
                *bit |= 0x400;
            }
            button++;
            bit++;
        }
    }
    kbd_pad[0] = bits[0];
    kbd_pad[1] = bits[1];
    kbd_pad[2] = bits[2];
    kbd_pad[3] = bits[3];
}

/* 0x80453218 (0x27C): The per-frame hook: steps the keyboard from the pad and the USB keyboard while it is open;
 * returns 1 when the input ended, -1 while the keyboard is closed. */
int sys_kbd_move(void)
{
    s32 result;

    if (kbd_active == 0) {
        return -1;
    }
    if (kbd_wait > 0) {
        kbd_wait--;
    }
    kbd_pad_read();
    switch (kbd_step_state) {
    case 0:
        OSDisableInterrupts();
        if (kbd_pad_skip == 0) {
            kbd_pad_dispatch(&kbd_pad[1], &kbd_pad[0], &kbd_pad[2]);
            if (kbd_key_count != kbd_object->getSoftKeyboard()->getMode()) {
                kbd_key_change_delay = 2;
            }
            kbd_key_count = kbd_object->getSoftKeyboard()->getMode();
        }
        OSEnableInterrupts();
        kbd_pad_skip = 0;
        result = kbd_step(kbd_pointer_channel, kbd_pad[1], kbd_pad[0], kbd_pad[2]);
        if (result != 0) {
            kbd_step_state = 1;
            kbd_close_delay = 3;
        }
        if (result == 0) {
            OSDisableInterrupts();
            kbd_command_step();
            OSEnableInterrupts();
            kbd_request->command = 0;
            kbd_request->command_kind = 0;
            if (kbd_key_change_delay > 0) {
                if (--kbd_key_change_delay == 0) {
                    hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 1);
                }
            }
            kbd_panel_idle = 0;
            if (kbd_object->getPanel()->isAnimating() == 0) {
                if (kbd_panel_animating == 1) {
                    kbd_panel_idle_count = 3;
                }
                if ((s8)(u8)--kbd_panel_idle_count <= 0) {
                    kbd_panel_idle_count = 0;
                    kbd_panel_idle = 1;
                }
            }
            kbd_panel_animating = kbd_object->getPanel()->isAnimating();
        }
        break;
    case 1:
        kbd_step(kbd_pointer_channel, 0, 0, 0);
        if (kbd_close_step() != 0) {
            kbd_step_state = 0;
            kbd_shown = 0;
            return 1;
        }
        break;
    }
    if (kbd_active == 1) {
        kbd_active++;
    }
    return 0;
}

/* Returns the soft keyboard's input mode. */
s32 HkbKeyboardBase::getMode() { return mKeyState.getMode(); }

/* 0x804534B8 (0xEC): Turns the small roman numerals (U+2170..U+2179) of `text` into the capital ones. */
void kbd_text_fix_numerals(u16* text)
{
    while (*text != 0) {
        if (*text == 0x2170) {
            *text = 0x2160;
        }
        if (*text == 0x2171) {
            *text = 0x2161;
        }
        if (*text == 0x2172) {
            *text = 0x2162;
        }
        if (*text == 0x2173) {
            *text = 0x2163;
        }
        if (*text == 0x2174) {
            *text = 0x2164;
        }
        if (*text == 0x2175) {
            *text = 0x2165;
        }
        if (*text == 0x2176) {
            *text = 0x2166;
        }
        if (*text == 0x2177) {
            *text = 0x2167;
        }
        if (*text == 0x2178) {
            *text = 0x2168;
        }
        if (*text == 0x2179) {
            *text = 0x2169;
        }
        text++;
    }
}

/* 0x804535A4 (0xDE0): Steps the keyboard one frame: the USB keyboard, the pad cursor over the keys or the gesture
 * list, the pointer, then the end of the input (posting the text to the chat or the caller's buffer); returns 1
 * when the input ended, -1 when it ended empty, 0 while it goes on. */
int kbd_step(s32 channel, u16 trigger, u16 held, u16 release)
{
    s32 shown;
    s32 result = 0;
    s32 held_bits;
    s32 trigger_bits;
    s32 release_bits;
    s32 move_bits;
    u16* text = kbd_text;
    HkbManager* manager;
    u16 buttons[3];

    kbd_request->modifiers = 0;
    kbd_request->key_held = 0;
    kbd_request->key = 0;
    held_bits = kbd_pad[0];
    trigger_bits = kbd_pad[1];
    release_bits = kbd_pad[2];
    move_bits = kbd_pad[3];
    move_bits |= trigger_bits;
    kbd_request->input_mode = kbd_object->getPanel()->getModeView()->getInputMode();
    manager = sysKbdHkbManager();
    shown = kbd_object->getSoftKeyboard()->isHalfWidth();
    if (kbd_closing == 0 && kbd_submit == 0) {
        hkbManagerUpdate(manager);
        HkbKeyIter iter = hkbKeyIterBeginWord1(manager);
        u32 modifiers = kbd_modifier_bits();
        kbd_object->getPanel()->setModifiers(modifiers);
    }
    kbd_object->calc();
    if (kbd_unlock_delay != 0 && kbd_object->getPanel()->isAnimating() == 0) {
        if (kbd_unlock_delay > 1) {
            kbd_unlock_delay--;
        } else {
            hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 1);
            kbd_unlock_delay = 0;
        }
    }
    switch (kbd_cursor_mode) {
    case 1:
        if (!kbd_object->getPanel()->getModeView()->isHalfWidth()) {
            s32 dir = trigger_bits & 0xF;
            if (dir != 0 || (move_bits & 0xF) != 0) {
                if (dir != 0 && kbd_object->getSoftKeyboard()->isHalfWidth() != 0) {
                    kbd_pad_skip = 1;
                    move_bits = 0;
                    trigger_bits = 0;
                    kbd_request->gesture_open = 0;
                    kbd_object->getSoftKeyboard()->setHalfWidth(0);
                    sysSE_req(0x3F);
                    kbd_object->getSoftKeyboard()->getKanaMode();
                }
                s32 cur_x;
                s32 cur_y;
                s32 start_x;
                s32 start_y;
                const KbdKeyPos* pos;
                start_x = kbd_cursor_x;
                cur_x = start_x;
                kbd_prev_x = start_x;
                start_y = kbd_cursor_y;
                cur_y = start_y;
                kbd_prev_y = start_y;
                do {
                    if (move_bits & 4) {
                        if (cur_x <= 0) {
                            kbd_cursor_x = 16;
                        } else {
                            kbd_cursor_x--;
                        }
                    } else if (move_bits & 8) {
                        kbd_cursor_x++;
                        if (kbd_cursor_x > 16) {
                            kbd_cursor_x = 0;
                        }
                    } else if (move_bits & 1) {
                        if (cur_y <= 0) {
                            kbd_cursor_y = 5;
                        } else {
                            kbd_cursor_y--;
                        }
                    } else if (move_bits & 2) {
                        kbd_cursor_y++;
                        if (kbd_cursor_y > 5) {
                            kbd_cursor_y = 0;
                        }
                    }
                    cur_x = kbd_cursor_x;
                    cur_y = kbd_cursor_y;
                    pos = &kbd_key_pos[kbd_cursor_x][kbd_cursor_y];
                } while (((pos->x == 0 && pos->y == 0) ||
                          (kbd_cursor_pos.x == pos->x && kbd_cursor_pos.y == pos->y - kbd_offset_y)) &&
                         (start_x != kbd_cursor_x || start_y != kbd_cursor_y));
            }
            kbd_cursor_pos.x = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].x;
            kbd_cursor_pos.y = kbd_key_pos[kbd_cursor_x][kbd_cursor_y].y - kbd_offset_y;
        }
        break;
    case 2: {
        s32 dir = trigger_bits & 0xF;
        if (dir != 0 || (move_bits & 0xF) != 0) {
            if (dir != 0 && kbd_object->getSoftKeyboard()->isHalfWidth() != 0) {
                kbd_object->getSoftKeyboard()->setHalfWidth(0);
            }
            s32 start_x;
            s32 start_y;
            s32 cur_x;
            s32 cur_y;
            const KbdKeyPos* pos;
            start_x = kbd_list_x;
            cur_x = start_x;
            kbd_prev_x = start_x;
            start_y = kbd_list_y;
            cur_y = start_y;
            kbd_prev_y = start_y;
            do {
                if (move_bits & 4) {
                    if (cur_x <= 0) {
                        kbd_list_x = 3;
                    } else {
                        kbd_list_x--;
                    }
                } else if (move_bits & 8) {
                    kbd_list_x++;
                    if (kbd_list_x > 3) {
                        kbd_list_x = 0;
                    }
                } else if (move_bits & 1) {
                    if (cur_y <= 0) {
                        kbd_list_y = 3;
                    } else {
                        kbd_list_y--;
                    }
                } else if (move_bits & 2) {
                    kbd_list_y++;
                    if (kbd_list_y > 3) {
                        kbd_list_y = 0;
                    }
                }
                cur_x = kbd_list_x;
                cur_y = kbd_list_y;
                pos = &kbd_list_pos[kbd_list_x][kbd_list_y];
            } while (((pos->x == 0 && pos->y == 0) ||
                      (kbd_cursor_pos.x == pos->x && kbd_cursor_pos.y == pos->y - kbd_offset_y)) &&
                     (start_x != kbd_list_x || start_y != kbd_list_y));
        }
        kbd_cursor_pos.x = kbd_list_pos[kbd_list_x][kbd_list_y].x;
        kbd_cursor_pos.y = kbd_list_pos[kbd_list_x][kbd_list_y].y - kbd_offset_y;
        break;
    }
    }
    {
        s32 words[3];
        s32 i;
        s32* word;
        u16* button;

        buttons[0] = 0;
        buttons[1] = 0;
        buttons[2] = 0;
        words[0] = trigger_bits;
        words[1] = held_bits;
        words[2] = release_bits;
        word = words;
        button = buttons;
        for (i = 3; i != 0; i--) {
            if (*word & 1) {
                *button |= 1;
            }
            if (*word & 2) {
                *button |= 0x4000;
            }
            if (*word & 4) {
                *button |= 2;
            }
            if (*word & 8) {
                *button |= 0x8000;
            }
            if (*word & 0x10) {
                *button |= 0x10;
            }
            if (*word & 0x20) {
                *button |= 0x40;
            }
            if (*word & 0x40) {
                *button |= 0x400;
            }
            if (*word & 0x80) {
                *button |= 0x1000;
            }
            word++;
            button++;
        }
    }
    if (kbd_closing == 0 && kbd_submit == 0) {
        if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0) {
            kbd_object->inputPointer(0, kbd_cursor_pos.x, -kbd_cursor_pos.y, buttons[0], buttons[1], buttons[2]);
        } else if (kbd_cursor_mode == 2) {
            kbd_object->inputPointer(0, kbd_cursor_pos.x, -kbd_cursor_pos.y, buttons[0], buttons[1], buttons[2]);
        } else {
            kbd_object->inputPointer(0, kbd_cursor_pos.x, -kbd_cursor_pos.y, 0, 0, 0);
        }
    }
    if (kbd_closing == 0 && kbd_submit == 0) {
        kbd_object->inputKeyboard(manager);
    }
    kbd_title_show(kbd_empty_title);
    if (shown != kbd_object->getSoftKeyboard()->isHalfWidth() && kbd_object->getSoftKeyboard()->isHalfWidth() != 0) {
        trigger_bits = 0;
    }
    if (kbd_object->getStateView()->getState() == 2 && kbd_submit != 0) {
        kbd_setting = kbd_object->getSetting();
        kbd_caps_lock = kbd_setting.flags.caps;
        kbd_object->setActive(1);
        kbd_submit = 0;
        kbd_text_kind = 0xFF;
        if (kbd_result != -1) {
            if (hbmKbdIsTextEmpty(kbd_object) == 0) {
                wcscpy(text, kbd_object->getText());
                kbd_text_fix_numerals(text);
                if (kbd_dest_text != 0 && kbd_max_length > 0) {
                    s32 length;
                    memset(kbd_dest_text, 0, kbd_max_length);
                    length = kbd_max_length;
                    netUtf16ToUtf8((u8*)kbd_dest_text, &length, text, 0);
                    kbd_dest_text[kbd_max_length] = 0;
                    if (kbd_text_mode == 0) {
                        postMediatorRecord(getInstance(), (u8*)kbd_dest_text);
                    }
                }
                if (kbd_dest_text == 0) {
                    s32 length = kbd_max_length;
                    netUtf16ToUtf8((u8*)kbd_utf8_text, &length, text, 0);
                    kbd_utf8_text[length] = 0;
                }
                if (kbd_request->mode == 0) {
                    if ((s8)kbd_target_cycle >= 2) {
                        NetCtrlWk::postEntryWithText(kbd_target_cycle, kbd_utf8_text, 0xFFFF50FF, kbd_target_slot,
                                                     kbd_target_id);
                    } else if (isServerSelectState() == 0) {
                        if ((s8)kbd_target_cycle == 0) {
                            NetCtrlWk::postEntryWithText(kbd_target_cycle, kbd_utf8_text, 0xF0F0F0FF, 0, 0);
                        } else if ((u32)hasJoinedServer() == 1) {
                            NetCtrlWk::postEntry(1, kbd_utf8_text, 0xFFA461FF, 0);
                        }
                    } else {
                        NetCtrlWk::postEntry(0, kbd_utf8_text, 0xF0F0F0FF, 0);
                    }
                }
            } else {
                if (kbd_dest_text != 0) {
                    memset(kbd_dest_text, 0, kbd_max_length + 1);
                }
                if (kbd_request->mode == 0) {
                    sysSE_req(2);
                }
            }
            system_w.menu_closed_0x8c0 = 0;
            kbd_request_buf.key = 0;
            kbd_request_buf.modifiers = 0;
            return 1;
        }
        system_w.menu_closed_0x8c0 = 0;
        kbd_request_buf.key = 0;
        kbd_request_buf.modifiers = 0;
        return -1;
    }
    if (kbd_object->getStateView()->getState() == 2 && hbmKbdIsTextEmpty(kbd_object) != 0 && (trigger_bits & 0x20) &&
        kbd_cursor_mode == 1 && kbd_wait == 0) {
        kbd_object->getCanceller()->cancel();
        kbd_text_kind = 0xFF;
        kbd_result = -1;
        result = 0;
    } else {
        if ((trigger_bits & 0xF) && kbd_object->getStateView()->getState() != 0 &&
            kbd_object->getSoftKeyboard()->isHalfWidth() != 0) {
            kbd_object->getSoftKeyboard()->setHalfWidth(0);
        }
        if (kbd_wide_mode != ck_WideMode_c1()) {
            kbd_screen_mode_update();
        }
    }
    kbd_font->ClearGlyphCache();
    subTransSetPrio(9, (u32)kbd_draw, 0, 0);
    return result;
}

/* Returns the panel's candidate list (inline in the band's header). */

/* 0x804543F8 (0x578): Turns the pad buttons into the keyboard's commands: the cursor jumps, the cancel and the
 * gesture list keys, and the USB keyboard's command keys. */
void kbd_pad_dispatch(s32* trigger, s32* held, s32* release)
{
    KbdPoint point = {0.0f, 0.0f, 0.0f};

    kbd_cursor_jump = 0;
    switch (kbd_cursor_mode) {
    case 0:
        *trigger &= ~0x10;
        break;
    case 1:
        if ((*trigger & 0x840) && kbd_wait == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0) {
            u32 half = kbd_object->getPanel()->getModeView()->isHalfWidth();
            kbd_cursor_jump = (half == 1) ? 1 : 2;
            *trigger = 0;
            hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 0);
            return;
        }
        if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (*trigger & 0x10) &&
            kbd_object->getPanel()->getModeView()->isHalfWidth() == 1) {
            kbd_cursor_jump = 1;
            *trigger &= ~0x10;
        }
        if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (*trigger & 0x80) && kbd_wait == 0) {
            kbd_object->getCanceller()->cancel();
            kbd_text_kind = 0xFF;
            kbd_result = -1;
            *held = 0;
            *trigger = 0;
            return;
        }
        break;
    case 2:
        if (kbd_object->getList()->getPage() == 0) {
            if (kbd_request->gesture_open == 1) {
                if ((*trigger & 0x10) && (kbd_list_x != 3 || kbd_list_y != 3)) {
                    kbd_text_clear();
                }
                if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (*trigger & 0x80) && kbd_wait == 0) {
                    kbd_object->getList()->refresh();
                    kbd_object->getCanceller()->cancel();
                    kbd_text_kind = 0xFF;
                    kbd_result = -1;
                }
                break;
            }
            if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (*trigger & 0x80) && kbd_wait == 0) {
                kbd_object->getList()->refresh();
                kbd_object->getCanceller()->cancel();
                kbd_text_kind = 0xFF;
                kbd_result = -1;
                *held = 0;
                *trigger = 0;
                return;
            }
            break;
        }
        if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (*trigger & 0x80) && kbd_wait == 0) {
            kbd_object->getList()->refresh();
            kbd_object->getCanceller()->cancel();
            kbd_text_kind = 0xFF;
            kbd_result = -1;
            *held = 0;
            *trigger = 0;
            return;
        }
        break;
    }
    u8 command = kbd_request->command;
    if (command != 0 && (u32)(kbd_request->command_kind - 1) > 1) {
        switch (command) {
        case 40:
            if (kbd_cursor_mode == 2 && kbd_request->gesture_open == 1) {
                kbd_text_clear();
                return;
            }
            break;
        case 42:
            if (wcslen(kbd_object->getText()) == 0 &&
                kbd_object->getPanel()->getModeView()->getInputMode() == 1 && kbd_wait == 0) {
                sys_kbd_close();
            }
            break;
        }
    } else {
        return;
    }
}

/* Points the keyboard's pointer at the key at `pos` with the A button down. */
#define KBD_POINT_AT(pos, button)                                                                        \
    point.x = (pos).x;                                                                                   \
    point.y = (pos).y - kbd_offset_y;                                                                    \
    kbd_object->inputPointer(0, point.x, -point.y, button, 0, 0)

/* 0x80454978 (0xF94): Acts on the frame's command: the function keys (phrases), the tab and escape keys, the pad's
 * jumps to the OK and mode keys, the gesture list and the target cycling. */
void kbd_command_step(void)
{
    KbdPoint point = {0.0f, 0.0f, 0.0f};
    s32 button = 0x10;
    s32 chat;
    s32 command;
    u8 mode;
    SysKbdRequest* req;

    if (kbd_submit == 1) {
        return;
    }
    chat = 0;
    if (kbd_request->mode <= 2 || kbd_request->mode == 4) {
        chat = 1;
    }
    command = kbd_request->command;
    if (command != 0 && kbd_cursor_mode != 0) {
        switch (kbd_request->command_kind) {
        case 1:
            if (command >= 58 && command <= 69 &&
                !kbd_object->getPanel()->getModeView()->isHalfWidth()) {
                s32 index = kbd_request->command - 58;
                if (index < 12) {
                    kbd_list_x = index % 4;
                    kbd_list_y = index / 4;
                    kbd_phrase_insert(index);
                    kbd_request->gesture_open = 1;
                    if (kbd_object->getList()->getPage() == 1) {
                        kbd_object->getList()->prevPageWrap();
                    }
                    sysSE_req(0x37);
                    kbd_unlock_delay = 2;
                }
            }
            break;
        default:
            switch (command) {
            case 41:
                if (kbd_wait == 0) {
                    sys_kbd_close();
                }
                break;
            case 43:
                kbd_target_cycle_step();
                break;
            }
            break;
        }
    }
    switch (kbd_cursor_mode) {
    case 0:
        return;
    case 1:
        if (kbd_object->getSoftKeyboard()->isHalfWidth() == 0) {
            if (kbd_cursor_jump == 1) {
                if (kbd_object->getPanel()->getModeView()->isHalfWidth() == 1) {
                    KBD_POINT_AT(kbd_key_pos[9][4], button);
                    kbd_cursor_jump = 0;
                }
            } else if (kbd_cursor_jump == 2) {
                KBD_POINT_AT(kbd_key_pos[15][4], 0x10);
            }
        }
        if (kbd_pad_skip == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (kbd_pad[1] & 0x20) &&
            kbd_wait == 0) {
            if (kbd_object->getPanel()->getModeView()->getInputMode() == 1) {
                u32 length = strlen(kbd_ascii_text);
                if (wcslen(kbd_object->getText()) <= length) {
                    sys_kbd_close();
                }
            }
            KBD_POINT_AT(kbd_key_pos[12][4], 0x10);
        }
        if (kbd_pad_skip == 0 && kbd_panel_idle == 1) {
            kbd_object->getSoftKeyboard()->isHalfWidth();
        }
        if (kbd_pad_skip == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (kbd_pad[1] & 0x4000)) {
            if (kbd_object->getPanel()->getModeView()->isHalfWidth() == 1) {
                button = 0x4000;
                kbd_object->inputPointer(0, 0.0f, 0.0f, 0x4000, 0, 0);
            } else if (kbd_request->kind == 14 && kbd_wait <= 10) {
                KBD_POINT_AT(kbd_key_pos[5][4], 0x10);
                kbd_phrase_insert(0);
                kbd_request->gesture_open = 1;
                kbd_list_x = 0;
                kbd_list_y = 0;
                kbd_unlock_delay = 2;
            }
        }
        if (kbd_pad_skip == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (kbd_pad[1] & 0x20000) &&
            chat == 1) {
            u8 layout = (kbd_object->getSoftKeyboard()->getKanaMode() & 3) + 1;
            if (layout >= 4) {
                layout = 1;
            }
            switch (layout) {
            case 0:
                point.x = kbd_key_pos[0][0].x;
                point.y = kbd_key_pos[0][0].y - kbd_offset_y;
                break;
            case 1:
                point.x = kbd_key_pos[0][1].x;
                point.y = kbd_key_pos[0][1].y - kbd_offset_y;
                break;
            case 2:
                point.x = kbd_key_pos[0][2].x;
                point.y = kbd_key_pos[0][2].y - kbd_offset_y;
                break;
            case 3:
                point.x = kbd_key_pos[0][3].x;
                point.y = kbd_key_pos[0][3].y - kbd_offset_y;
                break;
            }
            kbd_object->inputPointer(0, point.x, -point.y, button, 0, 0);
            hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 1);
        }
        if (kbd_pad_skip == 0 && kbd_object->getSoftKeyboard()->isHalfWidth() == 0 && (kbd_pad[1] & 0x10000)) {
            mode = kbd_request->mode;
            if (mode == 0 || mode == 4) {
                kbd_target_cycle_step();
            }
        }
        break;
    case 2:
        if (kbd_request->gesture_open == 1 && (kbd_pad[1] & 0x10)) {
            KBD_POINT_AT(kbd_list_pos[3][3], 0x10);
            kbd_request->gesture_open = 0;
            return;
        }
        if (kbd_object->getList()->getPage() == 0) {
            if ((kbd_pad[1] & 0x4000) && kbd_panel_idle == 1) {
                if (kbd_request->gesture_open == 0) {
                    kbd_list_x = -1;
                    kbd_list_y = 0;
                }
                kbd_list_x++;
                if (kbd_list_x > 3) {
                    kbd_list_x = 0;
                    kbd_list_y++;
                    if (kbd_list_y >= 3) {
                        kbd_list_x = 0;
                        kbd_list_y = 0;
                    }
                }
                u32 index = kbd_list_x + kbd_list_y * 4;
                if (kbd_request->gesture_open == 0) {
                    sysSE_req(0x3F);
                }
                kbd_request->gesture_open = 1;
                kbd_phrase_insert(index);
                return;
            }
        } else if ((kbd_pad[1] & 0x840) && kbd_panel_idle == 1) {
            kbd_object->getList()->prevPageWrap();
            kbd_phrase_insert(0);
            kbd_request->gesture_open = 1;
            kbd_list_x = 0;
            kbd_list_y = 0;
            return;
        }
        if ((kbd_pad[1] & 0x20) && kbd_wait == 0) {
            if (hbmKbdIsTextEmpty(kbd_object) != 0) {
                kbd_object->getList()->refresh();
                sys_kbd_close();
                kbd_request->gesture_open = 0;
                return;
            }
            kbd_list_flag = 0;
            kbd_object->getList()->refresh();
            kbd_request->gesture_open = 0;
            sysSE_req(0x3B);
        }
        if ((kbd_pad[1] & 0x840) && kbd_request->command != 61 && kbd_wait == 0) {
            kbd_object->getList()->refresh();
            kbd_submit = 1;
            kbd_request->gesture_open = 0;
        }
        req = kbd_request;
        command = req->command;
        if (command != 0) {
            if ((u32)(command - 79) > 3) {
                switch (command) {
                case 41:
                    if (kbd_wait == 0) {
                        kbd_object->getList()->refresh();
                        sys_kbd_close();
                    }
                    break;
                case 42:
                    kbd_list_flag = 0;
                    kbd_object->getList()->refresh();
                    if (wcslen(kbd_object->getText()) == 0 && kbd_wait == 0) {
                        sys_kbd_close();
                    } else {
                        sysSE_req(0x39);
                    }
                    break;
                case 40:
                    KBD_POINT_AT(kbd_list_pos[kbd_list_x][kbd_list_y], 0x10);
                    break;
                case 61:
                    if (req->command_kind == 0) {
                        kbd_object->getList()->prevPageWrap();
                        kbd_list_x = 0;
                        kbd_list_y = 0;
                    }
                    break;
                }
            } else if (kbd_cursor_mode == 2 && req->gesture_open == 0) {
                s32 start_x;
                s32 start_y;
                s32 cur_x;
                s32 cur_y;
                const KbdKeyPos* pos;
                start_x = kbd_list_x;
                cur_x = start_x;
                kbd_prev_x = start_x;
                start_y = kbd_list_y;
                cur_y = start_y;
                kbd_prev_y = start_y;
                do {
                    if (req->command == 80) {
                        if (cur_x <= 0) {
                            kbd_list_x = 3;
                        } else {
                            kbd_list_x--;
                        }
                    } else if (req->command == 79) {
                        kbd_list_x++;
                        if (kbd_list_x > 3) {
                            kbd_list_x = 0;
                        }
                    } else if (req->command == 82) {
                        if (cur_y <= 0) {
                            kbd_list_y = 3;
                        } else {
                            kbd_list_y--;
                        }
                    } else if (req->command == 81) {
                        kbd_list_y++;
                        if (kbd_list_y > 3) {
                            kbd_list_y = 0;
                        }
                    }
                    cur_x = kbd_list_x;
                    cur_y = kbd_list_y;
                    pos = &kbd_list_pos[kbd_list_x][kbd_list_y];
                } while (((pos->x == 0.0f && pos->y == 0.0f) ||
                          (kbd_list_cursor.x == pos->x && kbd_list_cursor.y == pos->y - kbd_offset_y)) &&
                         (start_x != kbd_list_x || start_y != kbd_list_y));
                kbd_list_cursor.x = kbd_list_pos[kbd_list_x][kbd_list_y].x;
                kbd_list_cursor.y = kbd_list_pos[kbd_list_x][kbd_list_y].y - kbd_offset_y;
            }
        }
        if (kbd_pad[1] & 0x10000) {
            mode = kbd_request->mode;
            if (mode == 0 || mode == 4) {
                kbd_target_cycle_step();
            }
        }
        break;
    }
}

/* 0x8045590C (0xD4): Steps the closing keyboard: counts the close delay down (cursor mode off) or closes the text
 * panel once the wait ran out; returns 1 when the keyboard is closed. */
int kbd_close_step(void)
{
    if (kbd_cursor_mode == 0) {
        if (kbd_close_delay < 0) {
            kbd_closing = 0;
            sys_kbd_close();
            return 1;
        }
        kbd_close_delay--;
        return 0;
    }
    if (kbd_closing != 0) {
        if (--kbd_wait < 0) {
            hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 0);
            kbd_object->getPanel()->close();
            kbd_active = 0;
            kbd_se_enabled = 0;
            return 1;
        }
    }
    return 0;
}

/* 0x80455A48 (0x17C): Closes the keyboard: keeps its settings, deactivates it and the request, and closes the text
 * panel. */
int sys_kbd_close(void)
{
    if (kbd_setup_type == 0xFF) {
        return 1;
    }
    if (kbd_active != 0) {
        if (kbd_closing == 0) {
            if (kbd_cursor_mode != 1) {
                kbd_object->getList()->refresh();
            }
            kbd_setting = kbd_object->getSetting();
            kbd_caps_lock = kbd_setting.flags.caps;
            if (kbd_object->getStateView()->getState() != 0) {
                kbd_object->setActive(1);
            }
        }
        system_w.menu_closed_0x8c0 = 0;
        kbd_request_buf.key = 0;
        kbd_request_buf.modifiers = 0;
        kbd_submit = 0;
        kbd_active = 0;
        kbd_closing = 1;
        kbd_wait = 0;
        kbd_se_enabled = 0;
        hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 0);
        kbd_object->getPanel()->close();
        kbd_step_state = 0;
    }
    return 1;
}

/* 0x80455BC4 (0x310): Cycles the chat target (all, party, then each connected session slot) and shows it as the
 * keyboard's title. */
void kbd_target_cycle_step(void)
{
    char title[0x10];
    char id[0x10];
    u32 length;
    s32 found;
    s32 slot;

    if ((s8)kbd_target_cycle == 10) {
        kbd_target_cycle = 0;
        kbd_object->setTitle(kbd_empty_title);
        return;
    }
    if (isLayerReady() == 0) {
        kbd_target_cycle = 0;
        kbd_object->setTitle(kbd_empty_title);
        return;
    }
    if ((s8)(u8)++kbd_target_cycle == 1 && (isServerSelectState() == 1 || net_ctrl_wk->profile_index_0x170 < 0)) {
        kbd_target_cycle++;
    }
    memset(kbd_title, 0, sizeof(kbd_title));
    if ((s8)kbd_target_cycle >= 2) {
        kbd_target_id[0] = 0;
        NetCtrlWk* work = net_ctrl_wk;
        if (work->layer_state_0x064 != 2) {
            kbd_target_cycle = 0;
            sprintf(title, kbd_target_formats[0]);
        } else {
            found = 0;
            if (getNetworkLayerPat(getPatsObject(), 0) != 0) {
                for (slot = 0; slot < 10; slot++) {
                    if (getNetworkLayerPat(getPatsObject(), 0)->friends_3568.entries_0x004[slot].valid_0x35 != 0) {
                        formatNetId(id, &getNetworkLayerPat(getPatsObject(), 0)->friends_3568.entries_0x004[slot].id_0x00);
                        if (strcmp(id, work->name_0x7368) != 0) {
                            if (found != (s8)kbd_target_cycle - 2) {
                                found++;
                            } else {
                                kbd_target_slot = slot;
                                strcpy(kbd_target_id, id);
                                if (get_option_cfg(0xC) == 0) {
                                    sprintf(title, kbd_target_formats[2],
                                            getNetworkLayerPat(getPatsObject(), 0)->friends_3568.entries_0x004[slot].name_0x20);
                                } else {
                                    sprintf(title, kbd_target_formats[2], id);
                                }
                                break;
                            }
                        }
                    }
                }
                if (slot >= 10) {
                    kbd_target_cycle = 0;
                    sprintf(title, kbd_target_formats[0]);
                    sysSE_req(0x37);
                }
            } else {
                kbd_target_cycle = 0;
                sprintf(title, kbd_target_formats[0]);
            }
        }
    } else {
        strcpy(title, kbd_target_formats[(s8)kbd_target_cycle]);
    }
    if ((s8)kbd_target_cycle != 0) {
        sysSE_req(0x37);
    }
    length = strlen(title);
    netMessageUtf8ToUtf16(kbd_title, (s32*)&length, (const u8*)title, 0);
    kbd_object->setTitle(kbd_title);
}

/* 0x80455ED4 (0x18): Empties the keyboard's text. */
void kbd_text_clear(void)
{
    kbd_object->setText(kbd_empty_title);
}

/* 0x80455EEC (0x80): Puts the phrase `index` into the keyboard's text. */
void kbd_phrase_insert(u32 index)
{
    hbmPanelClearKanaQueue((HbmTextPanel*)kbd_object->getPanel());
    kbd_object->setText(kbd_phrase_names[index]);
    hbmPanelSetInputEnabled((HbmTextPanel*)kbd_object->getPanel(), 1);
}

/* 0x80455F6C (0x1E4): Handles the USB keyboard while the keyboard is closed in the lobby: Shift+F1..F12 play a
 * gesture, a printable key opens the keyboard with it; returns 1 when the key opened the keyboard. */
int kbd_key_input(void)
{
    if (isNetCityMode() == 0 && isServerSelectState() == 0) {
        return 0;
    }
    if (get_fade_stat(0) == 2) {
        return 0;
    }
    if (isServerSelectState() == 0 && NetCtrlWk::getLobbyReadyByte() != 0) {
        return 0;
    }
    if (NetCtrlWk::hasSubError() != 0) {
        return 0;
    }
    kbd_request->flags |= 1;
    u8 modifiers = kbd_request->modifiers;
    if (modifiers == 2) {
        u8 key = kbd_request->key;
        if (key >= 0x3A && key <= 0x45) {
            pl_gesture_req_c1(kbd_gesture_table[(u8)(key - 0x3A)] + 1);
            sysSE_req(0x37);
            kbd_request->key = 0;
            return 0;
        }
    }
    if (modifiers & 5) {
        return 1;
    }
    switch (kbd_request->key) {
    case 0:
        return 0;
    case 41:
        return 0;
    case 1:
    case 2:
    case 3:
        return 0;
    case 83:
        return 0;
    case 70:
    case 71:
    case 72:
    default:
        return 0;
    /* the printable, editing, cursor and keypad keys open the keyboard with the key */
    case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16:
    case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28:
    case 29: case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39: case 40:
    case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50: case 51: case 52: case 53:
    case 54: case 55: case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63: case 64: case 65:
    case 66: case 67: case 68: case 69:
    case 73: case 74: case 75: case 76: case 77: case 78: case 79: case 80: case 81: case 82:
    case 84: case 85: case 86: case 87: case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95:
    case 96: case 97: case 98: case 99: case 100:
        HkbManager* manager = sysKbdHkbManager();
        hkbManagerUpdate(manager);
        kbd_object->getInput()->inputKey(manager, kbd_last_usage, 1);
        kbd_last_usage = 0;
        kbd_key_forwarded = 1;
        return 1;
    }
}

/* 0x80456150 (0x5C): The per-frame keyboard poll: handles the USB keyboard in the lobby and clears its key. */
int sys_kbd_input(void)
{
    if (isNetCityMode() == 0 && isServerSelectState() == 0) {
        return 0;
    }
    int result = kbd_key_input();
    kbd_request->modifiers = 0;
    kbd_request->key_held = 0;
    kbd_request->key = 0;
    return result;
}

/* 0x804561AC (0x80): Stores the driver's hooks into `system_w`. */
void kbd_hooks_set(void)
{
    system_w.kbd_hooks_set_0x8b4 = 1;
    system_w.kbd_init = sys_kbd_init;
    system_w.kbd_open = sys_kbd_open;
    system_w.kbd_move = sys_kbd_move;
    system_w.set_kbd_param = sys_set_kbd_param;
    system_w.get_kbd_setup_type = sys_get_kbd_setup_type;
    system_w.kbd_reset = sys_kbd_reset;
    system_w.kbd_close = sys_kbd_close;
    system_w.kbd_exit = sys_kbd_exit;
    system_w.kbd_input = sys_kbd_input;
}

/* ---------------------------------------------------------------------------------------------------------------- */
/* The keyboard's event handler. */

/* 0x80456324 (0x18): Ends the input when the keyboard is cancelled with a cursor mode set. */
inline void SysKbdHandler::onCancel()
{
    if (kbd_cursor_mode != 0) {
        kbd_submit = 1;
    }
}

/* 0x8045633C (0xC): Ends the input when the keyboard submits its text. */
inline void SysKbdHandler::onSubmit()
{
    kbd_submit = 1;
}

/* 0x80456348 (0x100): Plays the keyboard's sound `id` (while the sounds are enabled); id 29 also starts closing. */
inline void SysKbdHandler::onSound(s32 id)
{
    if (kbd_se_enabled == 0) {
        return;
    }
    switch (id) {
    case 3:
        sysSE_req(0x35);
        break;
    case 4:
        sysSE_req(0x36);
        break;
    case 5:
        sysSE_req(0x37);
        break;
    case 6:
        sysSE_req(0x38);
        break;
    case 7:
        sysSE_req(0x39);
        break;
    case 8:
        sysSE_req(0x3A);
        break;
    case 9:
        sysSE_req(0x3B);
        break;
    case 10:
        sysSE_req(0x3C);
        break;
    case 11:
        sysSE_req(0x3D);
        break;
    case 13:
        sysSE_req(0x3F);
        break;
    case 14:
        sysSE_req(0x40);
        break;
    case 15:
        sysSE_req(0x3F);
        break;
    case 16:
        sysSE_req(0x42);
        break;
    case 17:
        sysSE_req(0x43);
        break;
    case 18:
        sysSE_req(0x44);
        break;
    case 19:
        sysSE_req(0x45);
        break;
    case 20:
        sysSE_req(0x46);
        break;
    case 21:
        sysSE_req(0x47);
        break;
    case 22:
        sysSE_req(0x48);
        break;
    case 23:
        sysSE_req(0x49);
        break;
    case 27:
        sysSE_req(0x4D);
        break;
    case 28:
        sysSE_req(0x4E);
        break;
    case 29:
        kbd_closing = 1;
        kbd_wait = 2;
        sysSE_req(0x4F);
        break;
    }
}

/* 0x80456448 (0x4C): Plays the gesture the gesture list picked. */
inline void SysKbdHandler::onGesture(s32 index)
{
    kbd_gesture_index = index;
    if (index >= 12) {
        index -= 12;
    }
    pl_gesture_req_c1(kbd_gesture_table[index] + 1);
    sysSE_req(0x37);
}

/* 0x80456494 (0x60): Closes the gesture list (ending the input when it was open from a function key). */
inline void SysKbdHandler::onGestureClose()
{
    if (kbd_request->gesture_open == 1) {
        kbd_object->getList()->refresh();
        kbd_submit = 1;
    }
    kbd_gesture_hidden = 1;
}

/* 0x804564F4 (0xBC): Follows the keyboard's cursor mode. */
inline void SysKbdHandler::onCursorMode(s32 mode)
{
    kbd_cursor_mode = mode;
    switch (mode) {
    case 0:
        kbd_list_x = 0;
        kbd_list_y = 0;
        break;
    case 1:
        if (kbd_gesture_hidden == 1) {
            if (kbd_request->gesture_open == 1) {
                kbd_submit = 1;
            }
            kbd_gesture_hidden = 0;
        }
        kbd_request->gesture_open = 0;
        break;
    case 2:
        kbd_object->getSoftKeyboard()->isHalfWidth();
        kbd_request->gesture_key = 0;
        break;
    }
}
