/* homebutton/hkb_sign_keys.cpp - the software keyboard's sign (quick-chat) key tables: the B_SGNkey_* and T_SGNkey_*
 *   pane names, the default phrase and gesture texts, and the 20 sign key records whose name pointers the static
 *   initialiser 0x8055FB70 copies in.
 * RANGE. .text 0x8055FB70-0x8055FD58 (1 function); .ctors 0x8056F41C-0x8056F420; .data 0x80655630-0x80655BD8;
 *   .sdata 0x80794658-0x80794668 (L"Bow" and L"*", the two wide literals of 8 bytes or less).
 *   Edges: the left edge follows homebutton/hkb_cellphone_keys.cpp's static initialiser 0x8055F728; the right edge is
 *   this unit's static initialiser (.ctors word 0x8056F41C).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: the file name, from the B_SGNkey_* strings; GUESS hkb_sign_key_names, hkb_sign_phrase_defaults,
 *   hkb_sign_gesture_defaults, hkb_sign_keys and HkbSignKey, from their contents (the phrase and gesture texts are
 *   MH3's quick-chat signs); the static initialiser is the compiler's __sinit_\hkb_sign_keys_cpp.
 * SHAPES. The key records copy their pane names from hkb_sign_key_names at run time (a non-const array read in an
 *   aggregate initialiser), while the text-buffer pointers are address constants and stay static .data.
 */
#include "types.h"
#include "homebutton/hbm_anim_record.h"
#include "homebutton/hkb_sign_keys.h"

/* The pane names: B_SGNkey_* buttons in key order, then the T_SGNkey_* text boxes. */
const char* hkb_sign_key_names[2][HKB_SIGN_KEY_COUNT] = {
    {
        "B_SGNkey_12", "B_SGNkey_01", "B_SGNkey_02", "B_SGNkey_03", "B_SGNkey_04",
        "B_SGNkey_05", "B_SGNkey_06", "B_SGNkey_07", "B_SGNkey_08", "B_SGNkey_09",
        "B_SGNkey_10", "B_SGNkey_11", "B_SGNkey_00", "B_SGNkey_13", "B_SGNkey_14",
        "B_SGNkey_15", "B_SGNkey_16", "B_SGNkey_17", "B_SGNkey_18", "B_SGNkey_19",
    },
    {
        "T_SGNkey_20", "T_SGNkey_21", "T_SGNkey_22", "T_SGNkey_23", "T_SGNkey_24",
        "T_SGNkey_25", "T_SGNkey_26", "T_SGNkey_27", "T_SGNkey_28", "T_SGNkey_29",
        "T_SGNkey_30", "T_SGNkey_31", "T_SGNkey_32", "T_SGNkey_33", "T_SGNkey_34",
        "T_SGNkey_35", "T_SGNkey_36", "T_SGNkey_37", "T_SGNkey_38", "T_SGNkey_39",
    },
};

/* The default texts of the editable phrase signs. */
const wchar_t* hkb_sign_phrase_defaults[HKB_SIGN_EDIT_COUNT] = {
    L"Greetings", L"Farewell", L"Thanks", L"LFM:", L"LFG:", L"Meet:",
    L"Give me:", L"Give you:", L"Question:", L"Item:", L"Monster:", L"Found:",
};

/* The default texts of the gesture signs. */
const wchar_t* hkb_sign_gesture_defaults[HKB_SIGN_EDIT_COUNT] = {
    L"Bow", L"Greet", L"Wave", L"Call", L"Nod Head", L"Shake Head",
    L"Cheer", L"Despair", L"Frustrated", L"Praise", L"Footwork", L"Provoke",
};

HkbSignKey hkb_sign_keys[HKB_SIGN_KEY_COUNT] = {
    {hkb_sign_key_names[0][0], hkb_sign_key_names[1][0], hkb_sign_phrase_text[0], hkb_sign_gesture_text[0]},
    {hkb_sign_key_names[0][1], hkb_sign_key_names[1][1], hkb_sign_phrase_text[1], hkb_sign_gesture_text[1]},
    {hkb_sign_key_names[0][2], hkb_sign_key_names[1][2], hkb_sign_phrase_text[2], hkb_sign_gesture_text[2]},
    {hkb_sign_key_names[0][3], hkb_sign_key_names[1][3], hkb_sign_phrase_text[3], hkb_sign_gesture_text[3]},
    {hkb_sign_key_names[0][4], hkb_sign_key_names[1][4], hkb_sign_phrase_text[4], hkb_sign_gesture_text[4]},
    {hkb_sign_key_names[0][5], hkb_sign_key_names[1][5], hkb_sign_phrase_text[5], hkb_sign_gesture_text[5]},
    {hkb_sign_key_names[0][6], hkb_sign_key_names[1][6], hkb_sign_phrase_text[6], hkb_sign_gesture_text[6]},
    {hkb_sign_key_names[0][7], hkb_sign_key_names[1][7], hkb_sign_phrase_text[7], hkb_sign_gesture_text[7]},
    {hkb_sign_key_names[0][8], hkb_sign_key_names[1][8], hkb_sign_phrase_text[8], hkb_sign_gesture_text[8]},
    {hkb_sign_key_names[0][9], hkb_sign_key_names[1][9], hkb_sign_phrase_text[9], hkb_sign_gesture_text[9]},
    {hkb_sign_key_names[0][10], hkb_sign_key_names[1][10], hkb_sign_phrase_text[10], hkb_sign_gesture_text[10]},
    {hkb_sign_key_names[0][11], hkb_sign_key_names[1][11], hkb_sign_phrase_text[11], hkb_sign_gesture_text[11]},
    {hkb_sign_key_names[0][12], hkb_sign_key_names[1][12], L"*", L"*"},
    {hkb_sign_key_names[0][13], hkb_sign_key_names[1][13], L"*", L"*"},
    {hkb_sign_key_names[0][14], hkb_sign_key_names[1][14], L"*", L"*"},
    {hkb_sign_key_names[0][15], hkb_sign_key_names[1][15], L"*", L"*"},
    {hkb_sign_key_names[0][16], hkb_sign_key_names[1][16], L"*", L"*"},
    {hkb_sign_key_names[0][17], hkb_sign_key_names[1][17], L"*", L"*"},
    {hkb_sign_key_names[0][18], hkb_sign_key_names[1][18], L"*", L"*"},
    {hkb_sign_key_names[0][19], hkb_sign_key_names[1][19], L"*", L"*"},
};
