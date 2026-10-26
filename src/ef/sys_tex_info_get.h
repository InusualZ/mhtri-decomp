/* `sys_tex_info_get` (0x800D05A4), defined by `ef/system_core.cpp` (leaf header, rule 2): `system_w`'s texture slot
 * `index` (`SysTask`, `mh3_pad/system_work.h`), or NULL when it is out of range or not live. */
#ifndef MHTRI_EF_SYS_TEX_INFO_GET_H
#define MHTRI_EF_SYS_TEX_INFO_GET_H

#include "types.h"

struct _tex_info;

#ifdef __cplusplus
extern "C" {
#endif

struct _tex_info* sys_tex_info_get(s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYS_TEX_INFO_GET_H */
