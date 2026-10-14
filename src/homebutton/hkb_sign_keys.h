/*
 * homebutton/hkb_sign_keys.h - the sign (quick-chat) key tables owned by `homebutton/hkb_sign_keys.cpp`.
 */
#ifndef HOMEBUTTON_HKB_SIGN_KEYS_H
#define HOMEBUTTON_HKB_SIGN_KEYS_H

#include "types.h"

#define HKB_SIGN_KEY_COUNT 20  /* B_SGNkey_00..19 */
#define HKB_SIGN_EDIT_COUNT 12 /* the sign keys with an editable phrase and gesture */

/* One sign key: its button and text-box pane names and the phrase/gesture texts it shows. */
/* size: 0x10 */
struct HkbSignKey {
    const char* button_name;     /* +0x00 */
    const char* textbox_name;    /* +0x04 */
    const wchar_t* phrase;       /* +0x08 */
    const wchar_t* gesture;      /* +0x0C */
};

extern const char* hkb_sign_key_names[2][HKB_SIGN_KEY_COUNT];
extern const wchar_t* hkb_sign_phrase_defaults[HKB_SIGN_EDIT_COUNT];
extern const wchar_t* hkb_sign_gesture_defaults[HKB_SIGN_EDIT_COUNT];
extern HkbSignKey hkb_sign_keys[HKB_SIGN_KEY_COUNT];

#endif
