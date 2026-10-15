/*
 * nw4r/fn_80502828.h - one include for the four NW4R font files at 0x80502828-0x80504A3C: it includes each file's
 *   own header (ut_Font, ut_ResFontBase, ut_ResFont, ut_CharWriter, with ut_Color), so its includers keep building
 *   unchanged.
 */
#ifndef NW4R_FN_80502828_H
#define NW4R_FN_80502828_H

#include "types.h"
#include "nw4r/fn_805012C4.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------------------------- */
/* Callees (their bodies live in their own units)                                                 */
/* --------------------------------------------------------------------------------------------- */
/* untyped: opaque band object, typed by the callers' views */
u32 fn_805041F4(void* self, int arg);

#ifdef __cplusplus
}
#endif

#include "nw4r/ut_Color.h"
#include "nw4r/ut_Font.h"
#include "nw4r/ut_ResFontBase.h"
#include "nw4r/ut_ResFont.h"
#include "nw4r/ut_CharWriter.h"

#endif
