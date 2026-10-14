/* homebutton/hkb_cellphone_keys.cpp - the software keyboard's cellphone-keypad key maps: the B_CPkey_* and other
 *   pane names, 19 maps of 12 keys (pane name, key label and the characters a key cycles through) and the 26 input
 *   modes that pair a map with its mode label; the static initialiser 0x8055F728 copies the pane names in.
 * RANGE. .text 0x8055F728-0x8055FB70 (1 function); .ctors 0x8056F418-0x8056F41C; .rodata 0x8057BDB0-0x8057C228;
 *   .data 0x80652530-0x80655630; .sdata 0x80794638-0x80794658 (the letter-table pair and the zero-key labels, not
 *   yet in this unit's splits.txt block: the range is unclaimed between homebutton/hbm_anim_record.cpp's .sdata and
 *   homebutton/hkb_sign_keys.cpp's, and only this unit's .data and homebutton/hkb_cellphone.cpp read it).
 *   Edges: the .rodata modes run 0x8057BDB0..0x8057C228 in 0x2C steps; the last, 0x8057C1FC, points at the
 *   last key map .data 0x806553C0 and is read by homebutton/hkb_cellphone.cpp (0x8053E968) through the
 *   same mode table (.rodata 0x8057AFA8) that names 0x8057C14C..0x8057C1D0, so both runs end with this unit's
 *   claim, before homebutton/hbm_kb_event.cpp's .rodata 0x8057C228 and homebutton/hkb_sign_keys.cpp's .data 0x80655630.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: the file name, from the B_CPkey_* strings; GUESS hkb_cellphone_key_names, HkbCellphoneKey,
 *   HkbCellphoneMode, the letter tables and every map and mode name, from their contents (the map languages
 *   follow the Wii's language order and the accented letters; maps 16-17 carry the e-mail address symbols);
 *   GUESS case_mode for the mode's +0x28 halfword (1 on the "Abc" modes, 2 on three "abc" modes, otherwise 0);
 *   GUESS hkb_cellphone_letter_tables, hkb_cellphone_zero_label and hkb_cellphone_predict_label (the E057 glyph and
 *   the zero the European keypad shows on its zero key); the static initialiser is the compiler's
 *   __sinit_\hkb_cellphone_keys_cpp.
 * RESIDUALS. The static initialiser: retail stores the tenth key's name into the last seven maps first and sets up
 *   the epilogue's r11 early (scheduling); the stores and their order of maps are otherwise the same.
 * RESIDUALS. .sdata 0x80794638-0x80794658 needs the claim (request hkb-c#4); until then the object's .sdata is extra.
 * SHAPES. The maps copy their pane names from hkb_cellphone_key_names at run time (a non-const array read in an
 *   aggregate initialiser); the Latin maps' last key is the B_othersBT_EU pane, the kana and numeric maps' the
 *   B_CPkey_11 pane.
 */
#include "types.h"
#include "homebutton/hkb_cellphone_keys.h"

/* The lower-case and upper-case letters the keypad maps cover, in matching order. */
wchar_t hkb_cellphone_lower_letters[] = L"abcdefghijklmnopqrstuvwxyz\x00E0\x00E1\x00E2\x00E4\x00E5\x00E6\x00E3\x00E8\x00E9\x00EA\x00EB\x00EC\x00ED\x00EE\x00EF\x00F2\x00F3\x00F4\x00F6\x00F8\x00F5\x0153\x00F9\x00FA\x00FB\x00FC\x00E7\x00F1\x00FD\x00FF";
wchar_t hkb_cellphone_upper_letters[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ\x00C0\x00C1\x00C2\x00C4\x00C5\x00C6\x00C3\x00C8\x00C9\x00CA\x00CB\x00CC\x00CD\x00CE\x00CF\x00D2\x00D3\x00D4\x00D6\x00D8\x00D5\x0152\x00D9\x00DA\x00DB\x00DC\x00C7\x00D1\x00DD\x0178";

/* The letter tables, lower case then upper case. */
wchar_t* hkb_cellphone_letter_tables[2] = {hkb_cellphone_lower_letters, hkb_cellphone_upper_letters};

/* The zero key's label on a European keypad: the marker glyph and the digit, and the glyph alone while
 * predicting. */
const wchar_t* hkb_cellphone_zero_label = L"\xE057" L"0";
const wchar_t* hkb_cellphone_predict_label = L"\xE057";

/* The keypad's pane names: the 12 keys, line feed and delete, then the mode, tag and sign buttons. */
const char* hkb_cellphone_key_names[HKB_CELLPHONE_NAME_COUNT] = {
    "B_CPkey_00", "B_CPkey_01", "B_CPkey_02", "B_CPkey_03", "B_CPkey_04", "B_CPkey_05",
    "B_CPkey_06", "B_CPkey_07", "B_CPkey_08", "B_CPkey_09", "B_CPkey_10", "B_CPkey_11",
    "B_CPkey_LF", "B_CPkey_DELETE", "B_othersBT_JP", "B_othersBT_EU",
    "B_ChngTag_00", "B_ChngTag_01", "B_ChngTag_02", "B_ChngTag_03",
    "B_SGNkey_close", "B_SGNkey_prev", "B_SGNkey_next", "B_CPkey_Prdc_JP", "B_prdcModeBT_EU",
};

HkbCellphoneKey hkb_cellphone_map_us_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!-':@/$#&1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"More", L""},
};

HkbCellphoneKey hkb_cellphone_map_us_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!-':@/$#&1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc2"},
    {hkb_cellphone_key_names[2], L"def", L"def3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"More", L""},
};

HkbCellphoneKey hkb_cellphone_map_uk_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!-':@/\x00A3" L"1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_uk_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!-':@/\x00A3" L"1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc2"},
    {hkb_cellphone_key_names[2], L"def", L"def3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_german_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"'@_", L".,!?:;-()'@_/&1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC\x00C4" L"2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO\x00D6" L"6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV\x00DC" L"8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_german_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"'@_", L".,!?:;-()'@_/&1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc\x00E4" L"2"},
    {hkb_cellphone_key_names[2], L"def", L"def3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno\x00F6" L"6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs\x00DF" L"7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv\x00FC" L"8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_french_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!:;'\"-()@/_1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC\x00C0\x00C1\x00C4\x00C2\x00C7" L"2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF\x00C8\x00C9\x00CB\x00CA" L"3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI\x00CC\x00CD\x00CF\x00CE" L"4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO\x00D2\x00D3\x00D6\x00D4\x00D1\x0152" L"6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV\x00D9\x00DA\x00DC\x00DB" L"8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_french_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,?!:;'\"-()@/_1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc\x00E0\x00E1\x00E4\x00E2\x00E7" L"2"},
    {hkb_cellphone_key_names[2], L"def", L"def\x00E8\x00E9\x00EB\x00EA" L"3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi\x00EC\x00ED\x00EF\x00EE" L"4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno\x00F2\x00F3\x00F6\x00F4\x00F1\x0153" L"6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs\x00DF" L"7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv\x00F9\x00FA\x00FC\x00FB" L"8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_italian_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,:", L".,-?!'@:;/()1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC\x00C4\x00C1\x00C0\x00C2\x00C7" L"2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF\x00CB\x00C9\x00C8\x00CA" L"3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI\x00CF\x00CD\x00CC\x00CE" L"4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO\x00D6\x00D3\x00D2\x00D4" L"6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV\x00DC\x00DA\x00D9\x00DB" L"8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_italian_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,:", L".,-?!'@:;/()1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc\x00E4\x00E1\x00E0\x00E2\x00E7" L"2"},
    {hkb_cellphone_key_names[2], L"def", L"def\x00EB\x00E9\x00E8\x00EA" L"3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi\x00EF\x00ED\x00EC\x00EE" L"4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno\x00F6\x00F3\x00F2\x00F4" L"6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv\x00FC\x00FA\x00F9\x00FB" L"8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E9\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_spanish_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,:;'\"\x00A1!\x00BF?@/-_1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC\x00C7\x00C1\x00C0\x00C4\x00C2" L"2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF\x00C9\x00C8\x00CB\x00CA" L"3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI\x00CD\x00CC\x00CF\x00CE" L"4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MN\x00D1O", L"MN\x00D1O\x00D3\x00D2\x00D6\x00D4\x0152" L"6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV\x00DA\x00D9\x00DC\x00DB" L"8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_spanish_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".,?@", L".,:;'\"\x00A1!\x00BF?@/-_1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc\x00E7\x00E1\x00E0\x00E4\x00E2" L"2"},
    {hkb_cellphone_key_names[2], L"def", L"def\x00E9\x00E8\x00EB\x00EA" L"3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi\x00ED\x00EC\x00EF\x00EE" L"4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mn\x00F1o", L"mn\x00F1o\x00F3\x00F2\x00F6\x00F4\x0153" L"6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs\x00DF" L"7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv\x00FA\x00F9\x00FC\x00FB" L"8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_dutch_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".!@", L".,'!?@-_:/\"1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABC\x00C0\x00C2\x00C4\x00C1\x00C7" L"2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEF\x00C9\x00C8\x00CA\x00CB" L"3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHI\x00CE\x00CF\x00CD\x00CC" L"4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKL5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNO\x00D1\x00D4\x0152\x00D6\x00D3\x00D2" L"6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRS7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUV\x00D9\x00DB\x00DC\x00DA" L"8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_dutch_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L".!@", L".,'!?@-_:/\"1"},
    {hkb_cellphone_key_names[1], L"abc", L"abc\x00E0\x00E2\x00E4\x00E1\x00E7" L"2"},
    {hkb_cellphone_key_names[2], L"def", L"def\x00E9\x00E8\x00EA\x00EB" L"3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghi\x00EE\x00EF\x00ED\x00EC" L"4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jkl5"},
    {hkb_cellphone_key_names[5], L"mno", L"mno\x00F1\x00F4\x0153\x00F6\x00F3\x00F2" L"6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrs\x00DF" L"7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuv\x00F9\x00FB\x00FC\x00FA" L"8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[HKB_CELLPHONE_NAME_OTHERS_EU], L"\x20AC\x00E5\x03B1", L""},
};

HkbCellphoneKey hkb_cellphone_map_hiragana[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"\x3042", L"\x3042\x3044\x3046\x3048\x304A\x3041\x3043\x3045\x3047\x3049"},
    {hkb_cellphone_key_names[1], L"\x304B", L"\x304B\x304D\x304F\x3051\x3053"},
    {hkb_cellphone_key_names[2], L"\x3055", L"\x3055\x3057\x3059\x305B\x305D"},
    {hkb_cellphone_key_names[3], L"\x305F", L"\x305F\x3061\x3064\x3066\x3068\x3063"},
    {hkb_cellphone_key_names[4], L"\x306A", L"\x306A\x306B\x306C\x306D\x306E"},
    {hkb_cellphone_key_names[5], L"\x306F", L"\x306F\x3072\x3075\x3078\x307B"},
    {hkb_cellphone_key_names[6], L"\x307E", L"\x307E\x307F\x3080\x3081\x3082"},
    {hkb_cellphone_key_names[7], L"\x3084", L"\x3084\x3086\x3088\x3083\x3085\x3087"},
    {hkb_cellphone_key_names[8], L"\x3089", L"\x3089\x308A\x308B\x308C\x308D"},
    {hkb_cellphone_key_names[9], L"", L"\x309B\x309C"},
    {hkb_cellphone_key_names[10], L"\x308F\x3092\x3093", L"\x308F\x3092\x3093\x308E\x30FC"},
    {hkb_cellphone_key_names[11], L"\x3001\x3002\xFF01\xFF1F", L"\x3001\x3002\xFF01\xFF1F "},
};

HkbCellphoneKey hkb_cellphone_map_katakana[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"\x30A2", L"\x30A2\x30A4\x30A6\x30A8\x30AA\x30A1\x30A3\x30A5\x30A7\x30A9"},
    {hkb_cellphone_key_names[1], L"\x30AB", L"\x30AB\x30AD\x30AF\x30B1\x30B3"},
    {hkb_cellphone_key_names[2], L"\x30B5", L"\x30B5\x30B7\x30B9\x30BB\x30BD"},
    {hkb_cellphone_key_names[3], L"\x30BF", L"\x30BF\x30C1\x30C4\x30C6\x30C8\x30C3"},
    {hkb_cellphone_key_names[4], L"\x30CA", L"\x30CA\x30CB\x30CC\x30CD\x30CE"},
    {hkb_cellphone_key_names[5], L"\x30CF", L"\x30CF\x30D2\x30D5\x30D8\x30DB"},
    {hkb_cellphone_key_names[6], L"\x30DE", L"\x30DE\x30DF\x30E0\x30E1\x30E2"},
    {hkb_cellphone_key_names[7], L"\x30E4", L"\x30E4\x30E6\x30E8\x30E3\x30E5\x30E7"},
    {hkb_cellphone_key_names[8], L"\x30E9", L"\x30E9\x30EA\x30EB\x30EC\x30ED"},
    {hkb_cellphone_key_names[9], L"", L"\x309B\x309C"},
    {hkb_cellphone_key_names[10], L"\x30EF\x30F2\x30F3", L"\x30EF\x30F2\x30F3\x30EE\x30FC"},
    {hkb_cellphone_key_names[11], L"\x3001\x3002\xFF01\xFF1F", L"\x3001\x3002\xFF01\xFF1F "},
};

HkbCellphoneKey hkb_cellphone_map_address_upper[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"@1_", L"@./_-1"},
    {hkb_cellphone_key_names[1], L"ABC", L"ABCabc2"},
    {hkb_cellphone_key_names[2], L"DEF", L"DEFdef3"},
    {hkb_cellphone_key_names[3], L"GHI", L"GHIghi4"},
    {hkb_cellphone_key_names[4], L"JKL", L"JKLjkl5"},
    {hkb_cellphone_key_names[5], L"MNO", L"MNOmno6"},
    {hkb_cellphone_key_names[6], L"PQRS", L"PQRSpqrs7"},
    {hkb_cellphone_key_names[7], L"TUV", L"TUVtuv8"},
    {hkb_cellphone_key_names[8], L"WXYZ", L"WXYZwxyz9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[11], L",.!?", L",.!?: "},
};

HkbCellphoneKey hkb_cellphone_map_address_lower[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"@1_", L"@./_-1"},
    {hkb_cellphone_key_names[1], L"abc", L"abcABC2"},
    {hkb_cellphone_key_names[2], L"def", L"defDEF3"},
    {hkb_cellphone_key_names[3], L"ghi", L"ghiGHI4"},
    {hkb_cellphone_key_names[4], L"jkl", L"jklJKL5"},
    {hkb_cellphone_key_names[5], L"mno", L"mnoMNO6"},
    {hkb_cellphone_key_names[6], L"pqrs", L"pqrsPQRS7"},
    {hkb_cellphone_key_names[7], L"tuv", L"tuvTUV8"},
    {hkb_cellphone_key_names[8], L"wxyz", L"wxyzWXYZ9"},
    {hkb_cellphone_key_names[9], L"", L""},
    {hkb_cellphone_key_names[10], L"\xE057" L"0", L" 0"},
    {hkb_cellphone_key_names[11], L",.!?", L",.!?: "},
};

HkbCellphoneKey hkb_cellphone_map_numeric[HKB_CELLPHONE_KEY_COUNT] = {
    {hkb_cellphone_key_names[0], L"1", L"1"},
    {hkb_cellphone_key_names[1], L"2", L"2"},
    {hkb_cellphone_key_names[2], L"3", L"3"},
    {hkb_cellphone_key_names[3], L"4", L"4"},
    {hkb_cellphone_key_names[4], L"5", L"5"},
    {hkb_cellphone_key_names[5], L"6", L"6"},
    {hkb_cellphone_key_names[6], L"7", L"7"},
    {hkb_cellphone_key_names[7], L"8", L"8"},
    {hkb_cellphone_key_names[8], L"9", L"9"},
    {hkb_cellphone_key_names[9], L",", L","},
    {hkb_cellphone_key_names[10], L"0", L"0"},
    {hkb_cellphone_key_names[11], L"*", L"*"},
};

const HkbCellphoneMode hkb_cellphone_mode_us_title = {hkb_cellphone_map_us_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_us_upper = {hkb_cellphone_map_us_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_us_lower = {hkb_cellphone_map_us_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 2};
const HkbCellphoneMode hkb_cellphone_mode_uk_title = {hkb_cellphone_map_uk_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_uk_upper = {hkb_cellphone_map_uk_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_uk_lower = {hkb_cellphone_map_uk_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 2};
const HkbCellphoneMode hkb_cellphone_mode_german_title = {hkb_cellphone_map_german_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_german_upper = {hkb_cellphone_map_german_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_german_lower = {hkb_cellphone_map_german_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_french_title = {hkb_cellphone_map_french_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_french_upper = {hkb_cellphone_map_french_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_french_lower = {hkb_cellphone_map_french_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_italian_title = {hkb_cellphone_map_italian_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_italian_upper = {hkb_cellphone_map_italian_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_italian_lower = {hkb_cellphone_map_italian_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 2};
const HkbCellphoneMode hkb_cellphone_mode_spanish_title = {hkb_cellphone_map_spanish_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_spanish_upper = {hkb_cellphone_map_spanish_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_spanish_lower = {hkb_cellphone_map_spanish_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_dutch_title = {hkb_cellphone_map_dutch_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 1};
const HkbCellphoneMode hkb_cellphone_mode_dutch_upper = {hkb_cellphone_map_dutch_upper, L"ABC", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_dutch_lower = {hkb_cellphone_map_dutch_lower, L"abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_hiragana = {hkb_cellphone_map_hiragana, L"\x3042\x3044\x3046", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_katakana = {hkb_cellphone_map_katakana, L"\x30A2\x30A4\x30A6", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_address_upper = {hkb_cellphone_map_address_upper, L"Abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_address_lower = {hkb_cellphone_map_address_lower, L"Abc", HKB_CELLPHONE_KEY_COUNT, 0};
const HkbCellphoneMode hkb_cellphone_mode_numeric = {hkb_cellphone_map_numeric, L"123", HKB_CELLPHONE_KEY_COUNT, 0};
