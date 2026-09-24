/*
 * Symbols whose module is UNDECIDED (docs/plan.md 6.5 rule 2).
 *
 * The registered unit bands interleave across modules, so an address whose nearest registered unit below
 * and nearest above name different modules (or, as here, sit in a section with no registered range at all)
 * has no sound `<module>.h` to move to.  Those sites are documented here rather than guessed into a wrong
 * module - a wrong module is worse than a documented unknown.
 *
 * `system_w` (.bss 0x806585E0, 0xA5C bytes, scope global) is the game's system state block.  It is read by
 * `main.cpp` (root), `fn_80040598.cpp` (root) and `enemy/fn_8014A1BC.c`, so the type they each used to
 * define locally lives here once (rule 1).  No `.bss` range is registered in splits.txt, so the address
 * band gives no module for it.
 */
#ifndef MHTRI_UNSPLIT_UNKNOWN_H
#define MHTRI_UNSPLIT_UNKNOWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The game/system state block (`system_w`, 0xA5C B in retail).  Union of the three private copies:
 *   - `main.cpp`: the largest view, naming the bytes it polls at +0x01, +0x08 and +0x863..+0x931;
 *   - `fn_80040598.cpp`: the keyboard entry points at +0x8E8..+0x908;
 *   - `enemy/fn_8014A1BC.c`: the +0x0C word.
 *
 * Disagreement found: the copies disagree on the extent, not on any field's offset.  `fn_80040598.cpp`'s
 * copy stops at +0x90C and `enemy/fn_8014A1BC.c`'s at +0x10; the retail object is 0xA5C and `main.cpp`
 * zeroes all 0xA5C, so the size here is 0xA5C and the trailing bytes are padding.  Field offsets are
 * identical in every copy; the union below reproduces them.
 * size: 0xA5C
 */
typedef struct SystemWork {
    /* +0x000 */ u8 pad_0x00[0x1];
    /* +0x001 */ u8 unk1;
    /* +0x002 */ u8 pad_0x02[0x6];
    /* +0x008 */ u8 unk8;
    /* +0x009 */ u8 pad_0x09[0x3];
    /* +0x00C */ u32 field_0x0c;
    /* +0x010 */ u8 pad_0x10[0x853];
    /* +0x863 */ u8 unk2147;
    /* +0x864 */ u8 unk2148;
    /* +0x865 */ u8 unk2149;
    /* +0x866 */ u8 pad_0x866[0x8];
    /* +0x86E */ u8 unk2158;
    /* +0x86F */ u8 unk2159;
    /* +0x870 */ u8 pad_0x870[0x1];
    /* +0x871 */ u8 unk2161;
    /* +0x872 */ u8 pad_0x872[0x66];
    /* +0x8D8 */ u32 (*unk2264)(void);
    /* +0x8DC */ u8 pad_0x8DC[0x4];
    /* +0x8E0 */ void (*unk2272)(void);
    /* +0x8E4 */ void (*unk2276)(void);
    /* +0x8E8 */ void (*kbd_init)(u8);
    /* +0x8EC */ int (*kbd_open)(u8);
    /* +0x8F0 */ int (*kbd_move)(void);
    /* +0x8F4 */ void (*set_kbd_param)(char*, u32);
    /* +0x8F8 */ u8 (*get_kbd_setup_type)(void);
    /* +0x8FC */ void (*kbd_reset)(void);
    /* +0x900 */ int (*kbd_close)(void);
    /* +0x904 */ void (*kbd_exit)(void);
    /* +0x908 */ int (*kbd_input)(void);
    /* +0x90C */ u8 pad_0x90C[0x25];
    /* +0x931 */ u8 unk2353;
    /* +0x932 */ u8 pad_0x932[0x12A];
} SystemWork;

extern SystemWork system_w;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_UNKNOWN_H */
