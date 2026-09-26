/*
 * fn_80423E74.h - views for the 0x80423E74 band (network work record + PatCamellia crypto).
 *
 * The band is the TU that owns the arena/base vectors of the `net_ctrl_wk` record and the PatCamellia
 * wrapper over the retail Camellia cipher.  `NetCtrlWk` itself lives in `fn_80429B94.h` (one definition,
 * docs/plan.md 6.5 rule 1) - this header only adds what this unit needs on top.
 *
 * A symbol with no registered owner is declared here rather than in the .cpp (docs/plan.md 6.5 rule 2's
 * unsplit gap; `stylelint` only checks `src/`).
 */
#ifndef MHTRI_FN_80423E74_H
#define MHTRI_FN_80423E74_H

#include "types.h"
#include "fn_80429B94.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Camellia's key schedule (owner: src/Camellia/camellia.c).  Declared here because the vendor header
 * sits beside its source and is not on the include path; a second consumer should promote it into
 * `include/` (rule 2).  `PatCamelliaKey` is the vendor's `KEY_TABLE_TYPE`. */
typedef unsigned int PatCamelliaKey[68];

void Camellia_Ekeygen(int keyBitLength, const unsigned char* rawKey, PatCamelliaKey keyTable);
void Camellia_EncryptBlock(int keyBitLength, const unsigned char* plaintext,
                           const PatCamelliaKey keyTable, unsigned char* cipherText);
void Camellia_DecryptBlock(int keyBitLength, const unsigned char* cipherText,
                           const PatCamelliaKey keyTable, unsigned char* plaintext);

/* The unit's Camellia key schedule (.bss 0x806D3670, 0x110 B) and the 64 x 0x400 arena base
 * (.sbss 0x80794CEC); neither has a registered owner. */
extern PatCamelliaKey lbl_806D3670;
extern u8* lbl_80794CEC;

/* Unsplit game callees. */
u8* fn_800404BC(u32 size);
void fn_8042ED64(int code);
s32 fn_8042CC38(void);
s32 fn_804C2380(u32 id);

/* The layer facade both network units drive (mirrors `getNetworkSessionManagerPat` in fn_80429B94.h:
 * `getPatsObject()` is the owner, `index` the layer). */
void* getNetworkLayerPat(void* pats, int index);

/* The slot mode-word source value (.sdata2 0x8079C888). */
extern f32 lbl_8079C888;

/* MSL primitives. */
void* memcpy(void* dst, const void* src, u32 size);
void* memset(void* dst, int value, u32 size);
char* strcpy(char* dst, const char* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80423E74_H */
