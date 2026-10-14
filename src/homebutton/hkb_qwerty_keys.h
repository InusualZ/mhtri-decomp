/*
 * homebutton/hkb_qwerty_keys.h - the QWERTY key maps owned by `homebutton/hkb_qwerty_keys.cpp`.
 */
#ifndef HOMEBUTTON_HKB_QWERTY_KEYS_H
#define HOMEBUTTON_HKB_QWERTY_KEYS_H

#include "types.h"

#define HKB_QWERTY_KEY_COUNT 50  /* P_key_00..49 */
#define HKB_QWERTY_NAME_COUNT 55 /* the character keys, DELETE, LF, SPACE, SHIFT and CAPS */

/* One character key: its pane name and the characters it types plain, with caps lock, shifted, and shifted
 * with caps lock. */
/* size: 0xC */
struct HkbQwertyKey {
    const char* name;        /* +0x00 */
    wchar_t plain;           /* +0x04 */
    wchar_t caps;            /* +0x06 */
    wchar_t shift;           /* +0x08 */
    wchar_t shift_caps;      /* +0x0A */
};

extern const char* hkb_qwerty_key_names[HKB_QWERTY_NAME_COUNT];
extern HkbQwertyKey hkb_qwerty_map_japanese[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_english_us[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_english_uk[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_french[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_german[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_italian[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_spanish[HKB_QWERTY_KEY_COUNT];
extern HkbQwertyKey hkb_qwerty_map_dutch[HKB_QWERTY_KEY_COUNT];

#endif
