/*
 * nw4r/fn_805012C4.h - one include for the eight NW4R math/ut files at 0x805012C4-0x80502828: it includes each
 *   file's own header (math_types, math_geometry, ut_list, ut_LinkList, ut_binaryFileFormat, ut_CharStrmReader,
 *   ut_TagProcessorBase with ut_Rect, ut_LockedCache), so its includers keep building unchanged.
 */
#ifndef NW4R_FN_805012C4_H
#define NW4R_FN_805012C4_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void  MEMInitList(void* list, u16 offset);   /* ut::List initialiser */

/* --------------------------------------------------------------------------------------------- */
/* Callees whose band holds no registered unit                                                    */
/* --------------------------------------------------------------------------------------------- */
/* untyped: opaque band object, typed by the callers' views */
void fn_80501A64(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501BF4(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C60(void* list, void* node);

/* untyped: opaque band object, typed by the callers' views */
void* fn_80501C9C(void* list, u16 n);            /* List_GetNth */

#ifdef __cplusplus
}
#endif

#include "nw4r/math_types.h"
#include "nw4r/math_geometry.h"
#include "nw4r/ut_list.h"
#include "nw4r/ut_LinkList.h"
#include "nw4r/ut_binaryFileFormat.h"
#include "nw4r/ut_CharStrmReader.h"
#include "nw4r/ut_TagProcessorBase.h"
#include "nw4r/ut_LockedCache.h"

#endif
