/*
 * homebutton/hkb_keyboard.h - declarations of the symbols owned by `homebutton/hkb_keyboard.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HKB_KEYBOARD_H
#define HOMEBUTTON_HKB_KEYBOARD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The 7 base destructors the range's deleting destructors dispatch into (each in the unsplit band). */
/* untyped: opaque band object, typed by the callers' views */
void fn_805385D0(void* self, s32 flag);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h"

namespace nw4r {
namespace lyt {
namespace detail {

/* Returns whether two material names agree in their first 20 characters. */
bool EqualsMaterialName(const char* name1, const char* name2);
/* Returns whether a binary resource header carries `signature` and the big-endian byte-order mark. */
bool TestFileHeader(const ut::BinaryFileHeader& fileHeader, u32 signature);

}  // namespace detail
}  // namespace lyt
}  // namespace nw4r
#endif

#endif
