/*
 * include/Network/NetworkWiiMediator.h - the class the 0x80413C64..0x804155D4 Network band owns.
 *
 * Reconstructed from the range's own disassembly and the runtime dump's map.  The dump spells the
 * member functions `NetworkWiiMediator::<method>`; the project's `symbols.txt` carries the same names
 * already mangled (`reflectInit__18NetworkWiiMediatorPFllllPvPv_vPv`, `getAccountBan__18NetworkWiiMediatorFPcUl`),
 * so the declarations below are chosen so MWCC mangles them back to those exact spellings.
 *
 * Only the methods the range defines are declared.  They are declared **non-virtual** on purpose:
 * the range's object stores no `.data` vtable (the class's vtable lives in another translation unit),
 * and a class whose virtuals are all defined in its own TU gains a 20-byte `.data` vtable (the same
 * finding the GameSpy interface unit's header records).  `reflectInit`'s first parameter is the callback
 * the retail body stores at +0x78C - the `PFllllPvPv_v` half of the mangled name.
 */
#ifndef NETWORK_WII_MEDIATOR_H
#define NETWORK_WII_MEDIATOR_H

#include "types.h"
#include "Network/NetworkReflectService.h"   /* NetworkWiiMediatorReflectFn - the reflect service's callback type */

/* The mediator's field layout, traced from the disassembly: every offset below is one an instruction
 * in this unit addresses.  `buffer_A`/`buffer_B` are the two 0x106-byte blocks the accessors copy,
 * `reflect_page` the 0x400-byte page `getReflectPageBuffer` hands out, `line_table_a`/`line_table_b`
 * the two 1024-entry pointer arrays `parseReflectLines` clears and fills, and `line_table_c` the
 * one-entry array of its "single line" mode.  `pat_*` are the four words `initializeNetworkMediator`
 * and `updateOpeningState` keep the two `setPatRange` slots' address/length pairs in, and `stamp_*`
 * the 64-bit server timestamp.  `pad_*` is a gap the range never touches. */
typedef struct NetworkWiiMediatorFields {
    /* +0x000 */ u8  pad_000[0x1C];
    /* +0x01C */ u8  flag_1C;
    /* +0x01D */ u8  flag_1D;
    /* +0x01E */ u8  flag_1E;
    /* +0x01F */ u8  flag_1F;
    /* +0x020 */ u8  pad_020;
    /* +0x021 */ u8  flag_21;
    /* +0x022 */ u8  pad_022[0x02];
    /* +0x024 */ u32 field_24;
    /* +0x028 */ u32 field_28;
    /* +0x02C */ u32 field_2C;
    /* +0x030 */ u32 field_30;
    /* +0x034 */ u32 field_34;
    /* +0x038 */ u32 field_38;
    /* +0x03C */ u8  reflect_name_3C[0x20];
    /* +0x05C */ u8  reflect_name_5C[0x0F];
    /* +0x06B */ u8  field_6B;
    /* +0x06C */ u8  pad_06C[0x10];
    /* +0x07C */ u8  buffer_A[0x106];
    /* +0x182 */ u8  buffer_B[0x106];
    /* +0x288 */ u32 field_288;
    /* +0x28C */ u8  pad_28C[0x3FE];
    /* +0x68A */ u8  flag_68A;
    /* +0x68B */ u8  name_buffer_68B;
    /* +0x68C */ u8  pad_68C[0xFF];
    /* +0x78B */ u8  flag_78B;
    /* +0x78C */ u8  flag_78C;
    /* +0x78D */ u8  reflect_page[0x400];
    /* +0xB8D */ u8  pad_B8D[0x03];
    /* +0xB90 */ u32 pat_terms_ptr;
    /* +0xB94 */ u32 pat_maintenance_ptr;
    /* +0xB98 */ u32 pat_terms_size;
    /* +0xB9C */ u32 pat_maintenance_size;
    /* +0xBA0 */ u8  pad_BA0[0x02];
    /* +0xBA2 */ u8  single_line_buffer[0x02];
    /* +0xBA4 */ char* line_table_a[0x400];
    /* +0x1BA4 */ char* line_table_b[0x400];
    /* +0x2BA4 */ char* line_table_c[0x01];
    /* +0x2BA8 */ u8  pad_2BA8[0xE68];
    /* +0x3A10 */ u64 stamp_3A10;
    /* +0x3A18 */ u8  pad_3A18[0x26B9];
    /* +0x60D1 */ u8  flag_60D1;
} NetworkWiiMediatorFields;  /* size: 0x60D2 */

/* The mediator singleton: its record is the field layout above (the entry points below take it as that
 * record); the class only adds the member functions this band defines. */
class NetworkWiiMediator : public NetworkWiiMediatorFields {
public:

    void reflectInit(NetworkWiiMediatorReflectFn callback, void* arg);
    void reflectStart();
    void reflectStop();
    void reflectFinal();
    s32  getOpeningProgress();
    s32  getOpeningTermsVersion();
    s32  isOpeningMaintenanceTerms();
    s32  isOpeningMaintenanceServer();
    s32  isOpeningAnnounce();
    char* getAccountBan(char* out, u32 size);
    char* getAccountWarning(char* out, u32 size);
    char* getAccountWaitQueue(char* out, u32 size);
    void getReflectPage(u8 page);
    void agreeReflect();
};


#ifdef __cplusplus
extern "C" {
#endif

/* The mediator's plain-named entry points (each takes the singleton `getInstance` returns, as its field
 * record); the parameter lists are this unit's own definitions'. */
void resetMediatorState(NetworkWiiMediatorFields* self);
void resetMediatorFlags(NetworkWiiMediatorFields* self);
void resetMediatorFlag1D(NetworkWiiMediatorFields* self);
void updateOpeningState(NetworkWiiMediatorFields* self, u32 slot, u32 address, u32 size);
void updateTermVersion(NetworkWiiMediatorFields* self, u32 value);
void updatePatInterface180(NetworkWiiMediatorFields* self, u32 a, u32 b, u32 c);
void loadPatInterfaceBuffers();
s32 queryOpeningFlag278(NetworkWiiMediatorFields* self);
s32 queryOpeningFlag2A8(NetworkWiiMediatorFields* self);
s32 queryOpeningFlag2C0(NetworkWiiMediatorFields* self);
void setReflectPageRange(NetworkWiiMediatorFields* self, u32 address, u32 size);
void setReflectField30(NetworkWiiMediatorFields* self, u32 value);
void setReflectField34(NetworkWiiMediatorFields* self, u32 value);
void setReflectField38(NetworkWiiMediatorFields* self, u32 value);
void setReflectName3C(NetworkWiiMediatorFields* self, char* name);
void setReflectName5C(NetworkWiiMediatorFields* self, char* name);
s32 dispatchReflectEvent();
s32 getWarningUInt(NetworkWiiMediatorFields* self);
char* getAccountName(NetworkWiiMediatorFields* self, char* out, u32 size);
/* the mediator state byte at +0x68A (the session state machine sets and reads it) */
void setMediatorState68A(NetworkWiiMediatorFields* self, u8 value);
void getMediatorState68A(NetworkWiiMediatorFields* self, u8* out);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_WII_MEDIATOR_H */
