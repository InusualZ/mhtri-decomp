/*
 * homebutton/hkb_cellphone_keys.h - the cellphone-keypad key maps and input modes owned by
 * `homebutton/hkb_cellphone_keys.cpp`.
 */
#ifndef HOMEBUTTON_HKB_CELLPHONE_KEYS_H
#define HOMEBUTTON_HKB_CELLPHONE_KEYS_H

#include "types.h"

#define HKB_CELLPHONE_KEY_COUNT 12      /* B_CPkey_00..11 */
#define HKB_CELLPHONE_NAME_COUNT 25     /* the entries of hkb_cellphone_key_names */
#define HKB_CELLPHONE_NAME_OTHERS_EU 15 /* "B_othersBT_EU", the Latin maps' last key */

/* One keypad key: its pane name, the label it shows and the characters a press cycles through. */
/* size: 0x34 */
struct HkbCellphoneKey {
    const char* name;        /* +0x00 */
    wchar_t label[8];        /* +0x04 */
    wchar_t chars[16];       /* +0x14 */
};

/* One input mode: its key map, the label of the mode button, the key count and the case behaviour. */
/* size: 0x2C */
struct HkbCellphoneMode {
    HkbCellphoneKey* keys;   /* +0x00 */
    wchar_t label[16];       /* +0x04 */
    u32 key_count;           /* +0x24 */
    u16 case_mode;           /* +0x28 */
};

extern wchar_t hkb_cellphone_lower_letters[];
extern wchar_t hkb_cellphone_upper_letters[];
extern const char* hkb_cellphone_key_names[HKB_CELLPHONE_NAME_COUNT];

/* The letter tables, lower case then upper case. */
extern wchar_t* hkb_cellphone_letter_tables[2];

/* The zero key's label on a European keypad, without and with prediction. */
extern const wchar_t* hkb_cellphone_zero_label;
extern const wchar_t* hkb_cellphone_predict_label;

extern HkbCellphoneKey hkb_cellphone_map_us_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_us_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_uk_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_uk_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_german_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_german_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_french_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_french_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_italian_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_italian_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_spanish_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_spanish_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_dutch_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_dutch_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_hiragana[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_katakana[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_address_upper[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_address_lower[HKB_CELLPHONE_KEY_COUNT];
extern HkbCellphoneKey hkb_cellphone_map_numeric[HKB_CELLPHONE_KEY_COUNT];


extern const HkbCellphoneMode hkb_cellphone_mode_us_title;
extern const HkbCellphoneMode hkb_cellphone_mode_us_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_us_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_uk_title;
extern const HkbCellphoneMode hkb_cellphone_mode_uk_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_uk_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_german_title;
extern const HkbCellphoneMode hkb_cellphone_mode_german_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_german_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_french_title;
extern const HkbCellphoneMode hkb_cellphone_mode_french_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_french_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_italian_title;
extern const HkbCellphoneMode hkb_cellphone_mode_italian_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_italian_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_spanish_title;
extern const HkbCellphoneMode hkb_cellphone_mode_spanish_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_spanish_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_dutch_title;
extern const HkbCellphoneMode hkb_cellphone_mode_dutch_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_dutch_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_hiragana;
extern const HkbCellphoneMode hkb_cellphone_mode_katakana;
extern const HkbCellphoneMode hkb_cellphone_mode_address_upper;
extern const HkbCellphoneMode hkb_cellphone_mode_address_lower;
extern const HkbCellphoneMode hkb_cellphone_mode_numeric;

#endif
