/*
 * Network/NetworkWiiMediator.cpp - the `NetworkWiiMediator` class band
 * (`.text` 0x80413C64..0x804155D4, 79 functions / 6490 B).
 *
 * BOUNDARY.  Left seam 0x80413C64 is **strong** (two signals): `tudiscover at
 * reflectInit__18NetworkWiiMediatorFPFllllPvPv_vPv` flags the `.data` jumptable run jump
 * `jumptable_806024B8 -> jumptable_8060277C` at 0x80413C64 *and* 0x80413E18 (share 0.090/0.086),
 * so the function above it (`getLanguage`, 0x80413C40..0x80413C64) is a different TU.  Right seam
 * 0x804155D4 is **weak** (`tudiscover at agreeReflect__18NetworkWiiMediatorFv`, share 0.247): the
 * class band may continue past it, but 0x804155D4 is the largest single jump the evidence offers and
 * the next function (`fn_804155D4`, 0x9D8) opens a new shape.  Sections: `.text`
 * 0x80413C64..0x804155D4, `extab` 0x8001CA4C..0x8001CBF4, `extabindex` 0x8003D2F0..0x8003D524
 * (47 unwind records).
 *
 * RECONCILIATION.  This unit folds in the retired `Network/NetworkWiiMediator.c`, which owned the
 * 20-byte `getReflectPageBuffer` at **0x80413F3C inside this span** as a `Matching` unit.  That address is
 * this original TU's, so the class band owns it now; `getReflectPageBuffer`'s body is carried across verbatim
 * (5 instructions, 100 %).  Because the range's owner changes from a `Matching` unit (our object
 * linked) to a `NonMatching` one (retail split object linked), the link is re-verified green by the
 * batch's `ninja build/RMHE08/ok`.
 *
 * WHAT IT IS.  The mediator's account/opening/reflect query surface (the reflected class methods at
 * 0x8041416C..0x80414B88) plus the ~65 unnamed helpers around them; the jumptable-referenced
 * 0x80414BF0..0x804155D4 block is the reflect sub-machine.  C++ (the mangled member names), so the
 * class is declared in `include/Network/NetworkWiiMediator.h` and every unmangled helper is
 * `extern "C"`.
 *
 * NAMES.  The `NetworkWiiMediator::*` spellings are the runtime dump's and already in `symbols.txt`.
 * The 65 helpers answer only `zz_XXXXXXXX_` in the dump, so they were named in this batch's naming pass
 * from their own code: the accessor/updater pairs after the field they touch (`getReflectField30` /
 * `setReflectField30`, `getReflectName3C` / `setReflectName3C`, `getReflectPageRange` /
 * `setReflectPageRange`), the singleton forwarders after the 0x803FE helper they call
 * (`updatePatField854` -> fn_803FE854, `queryOpeningFlag208` -> fn_803FE208), and the packet/reflect
 * helpers after what they do (`parseReflectPacket`, `buildReflectPacket`, `validateReflectName`,
 * `getReflectModeFromLanguage`, `isShiftJisLeadByte`).  `getReflectPageBuffer` is the retired
 * `NetworkWiiMediator.c` symbol (was `fn_80413F3C`), carried across verbatim and named here.
 *
 * NAMING GUESSES (rule 6.5: a guess is stated, not hidden).  `getReflectField30/34/38` and
 * `getAccountQuery1..5` are positional - the dump has no name and no caller reveals the field's
 * meaning; `queryOpeningFlagNNN` is keyed on the 0x803FE helper's address, not a recovered API name;
 * `getMediatorFlag78B/78C/60D1` and `getMediatorField24/288` are byte/word accessors whose consumer
 * semantics are unproven.  Every name is `fn_`-free so the batch owns its symbols; the next reader
 * replaces the positional ones as the bodies come in.
 *
 * BODIES.  35 of 79 functions are reconstructed (29 at 100 %); the big sub-machine functions
 * (`dispatchReflectEvent`, `buildReflectPacket`, `parseReflectPacket`, `initializeNetworkMediator`,
 * `loadPatInterfaceBuffers`) are stubs.
 *
 * rule 7 deferred: the only `fn_` spellings left in this file are *references* to other units' unrenamed
 * symbols (`fn_804138E4`, `fn_804155D4`, `fn_8041A458`, `fn_8041A48C`, `fn_8041A540`) - the escape covers
 * references, not this unit's own definitions, which are all named above.
 */
#include "types.h"
#include "Network/NetworkWiiMediator.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The mediator's field layout, traced from the disassembly: every offset below is one an instruction
 * in this unit addresses.  Named `flag_*` where the code only sets/clears a byte, `field_*` where the
 * meaning is not yet evidenced (a reconnaissance registration: naming them from context is the body
 * pass).  `pad_*` is a gap the range never touches. */
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
    /* +0x03C */ u8  pad_03C[0x2F];
    /* +0x06B */ u8  field_6B;
    /* +0x06C */ u8  pad_06C[0x21C];
    /* +0x288 */ u32 field_288;
    /* +0x28C */ u8  pad_28C[0x3FE];
    /* +0x68A */ u8  field_68A;
    /* +0x68B */ u8  field_68B;
    /* +0x68C */ u8  pad_68C[0xFF];
    /* +0x78B */ u8  field_78B;
    /* +0x78C */ u8  field_78C;
    /* +0x78D */ u8  pad_78D[0x3283];
    /* +0x3A10 */ u32 field_3A10;
    /* +0x3A14 */ u32 field_3A14;
    /* +0x3A18 */ u8  pad_3A18[0x26B9];
    /* +0x60D1 */ u8  field_60D1;
} NetworkWiiMediatorFields;  /* size: 0x60D2 */

extern "C" {
/* the retired NetworkWiiMediator.c symbol, carried across verbatim */
void getReflectPageBuffer(char* self, char** subobject, unsigned int* limit);
void setMediatorBufferA(void* self, void* src);
void getMediatorBufferA(void* self, void* dst);
void setMediatorBufferB(void* self, void* src);
void getMediatorBufferB(void* self, void* dst);
void getMediatorNameBuffer(NetworkWiiMediatorFields* self, void* out1, void* out2);
void setMediatorFlag78B(NetworkWiiMediatorFields* self, u8 value);
void getMediatorField288(NetworkWiiMediatorFields* self, void* out);
void setMediatorFlag78C(NetworkWiiMediatorFields* self, u8 value);
void getMediatorFlag78C(NetworkWiiMediatorFields* self, void* out);
void resetMediatorState(void* self);
void initializeNetworkMediator(void* self, u32 value);
void* getInstance_(void);
void* fn_804138E4(void);
void  fn_8041A458(void* self);
void  fn_8041A48C(void* self);
void  fn_8041A540(void* self);
s32   isOpeningMaintenanceTerms(void* self);
s32   isOpeningMaintenanceServer(void* self);
s32   isOpeningAnnounce(void* self);
u32   getMediatorField24(NetworkWiiMediatorFields* self);
u8    getMediatorFlag6B(NetworkWiiMediatorFields* self);
u8    getMediatorFlag60D1(NetworkWiiMediatorFields* self);
void  setMediatorTimestamp(NetworkWiiMediatorFields* self, u32 unused, u32 a, u32 b);
void  getMediatorState68A(NetworkWiiMediatorFields* self, void* out);
void  fn_803FE180(void* self, u32 a, u32 b, u32 c);
void  setTermVersion(void* self, u32 value);
void  updatePatInterface180(void* self, u32 a, u32 b, u32 c);
void  updateTermVersion(void* self, u32 value);
s32   getWarningUInt2(void* self);
void  resetMediatorFlags(NetworkWiiMediatorFields* self);
void  resetMediatorFlag1D(NetworkWiiMediatorFields* self);
u64   getMediatorTimestamp(NetworkWiiMediatorFields* self);
s32   getAccountQuery1(void* self);
s32   getAccountQuery2(void* self);
s32   getAccountQuery3(void* self);
s32   getAccountQuery4(void* self);
s32   getAccountQuery5(void* self);
void  getReflectPageRange(NetworkWiiMediatorFields* self, void* out1, void* out2);
void  getReflectField30(NetworkWiiMediatorFields* self, void* out);
void  getReflectField34(NetworkWiiMediatorFields* self, void* out);
void  getReflectField38(NetworkWiiMediatorFields* self, void* out);
s32   isShiftJisLeadByte(void* self, u8 value);
s32   getWarningUInt(void* self);
void dispatchReflectEvent();
void getReflectModeFromLanguage();
void loadPatInterfaceBuffers();
void updateOpeningState();
void updateServerTime();
void setServerTimeResult();
void setMediatorState68A();
void queryOpeningFlag208();
void queryOpeningFlag250();
void queryOpeningFlag278();
void queryOpeningFlag290();
void queryOpeningFlag2A8();
void queryOpeningFlag2C0();
void getReflectPageText();
void getAccountName();
void getReflectName3C();
void getMediaVersionString();
void getStr1String();
void getReflectName5C();
void setReflectPageRange();
void setReflectField30();
void setReflectField34();
void setReflectField38();
void setReflectName3C();
void setReflectName5C();
void updatePatField854();
void updatePatField860();
void isNameSymbolChar();
void parseReflectLines();
void parseReflectPacket();
void validateReflectName();
void buildReflectPacket();

} /* extern "C" */

/* retired Network/NetworkWiiMediator.c, carried across verbatim (100 %) */
void getReflectPageBuffer(char *self, char **subobject, unsigned int *limit)
{
    *subobject = self + 0x78D;
    *limit = 0x400;
}

void NetworkWiiMediator::reflectInit(NetworkWiiMediatorReflectFn callback, void* arg) { (void)callback; (void)arg; }
void NetworkWiiMediator::reflectStart()
{
    if (fn_804138E4() != NULL) {
        fn_8041A458(fn_804138E4());
    }
}
void NetworkWiiMediator::reflectStop()
{
    if (fn_804138E4() != NULL) {
        fn_8041A48C(fn_804138E4());
    }
}
void NetworkWiiMediator::reflectFinal() {}
s32  NetworkWiiMediator::getOpeningProgress() { return 0; }
s32  NetworkWiiMediator::getOpeningTermsVersion() { return 0; }
s32  NetworkWiiMediator::isOpeningMaintenanceTerms()
{
    if (getInstance_() != NULL) {
        return ::isOpeningMaintenanceTerms(getInstance_());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningMaintenanceServer()
{
    if (getInstance_() != NULL) {
        return ::isOpeningMaintenanceServer(getInstance_());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningAnnounce()
{
    if (getInstance_() != NULL) {
        return ::isOpeningAnnounce(getInstance_());
    }
    return 0;
}
void NetworkWiiMediator::getAccountBan(char* out, u32 size) { (void)out; (void)size; }
void NetworkWiiMediator::getAccountWarning(char* out, u32 size) { (void)out; (void)size; }
void NetworkWiiMediator::getAccountWaitQueue(char* out, u32 size) { (void)out; (void)size; }
void NetworkWiiMediator::getReflectPage(u8 page) { (void)page; }
void NetworkWiiMediator::agreeReflect()
{
    if (fn_804138E4() != NULL) {
        fn_8041A540(fn_804138E4());
    }
}

/* the unmangled helpers */
void dispatchReflectEvent() {}
void getReflectModeFromLanguage() {}
void setMediatorBufferA(void* self, void* src)
{
    if (src == NULL) {
        return;
    }
    memcpy((u8*)self + 124, src, 262);
}
void getMediatorBufferA(void* self, void* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, (u8*)self + 124, 262);
}
void setMediatorBufferB(void* self, void* src)
{
    if (src == NULL) {
        return;
    }
    memcpy((u8*)self + 386, src, 262);
}
void getMediatorBufferB(void* self, void* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, (u8*)self + 386, 262);
}
void getMediatorNameBuffer(NetworkWiiMediatorFields* self, void* out1, void* out2)
{
    *(u32*)out1 = (u32)&self->field_68B;
    *(u8*)out2 = self->field_78B;
}
void setMediatorFlag78B(NetworkWiiMediatorFields* self, u8 value)
{
    self->field_78B = value;
}
void getMediatorField288(NetworkWiiMediatorFields* self, void* out)
{
    *(u32*)out = (u32)&self->field_288;
}
void setMediatorFlag78C(NetworkWiiMediatorFields* self, u8 value)
{
    self->field_78C = value;
}
void getMediatorFlag78C(NetworkWiiMediatorFields* self, void* out)
{
    *(u8*)out = self->field_78C;
}
void resetMediatorState(void* self)
{
    initializeNetworkMediator(self, 0);
}
void initializeNetworkMediator(void* self, u32 value) { (void)self; (void)value; }
void resetMediatorFlags(NetworkWiiMediatorFields* self)
{
    self->flag_1C = 1;
    self->flag_1E = 0;
    self->flag_1F = 0;
    self->field_68A = 0;
    self->flag_21 = 0;
}
void resetMediatorFlag1D(NetworkWiiMediatorFields* self)
{
    self->flag_1D = 1;
    self->flag_1C = 0;
}
void loadPatInterfaceBuffers() {}
u32 getMediatorField24(NetworkWiiMediatorFields* self) { return self->field_24; }
u8 getMediatorFlag6B(NetworkWiiMediatorFields* self) { return self->field_6B; }
u8 getMediatorFlag60D1(NetworkWiiMediatorFields* self) { return self->field_60D1; }
void updatePatInterface180(void* self, u32 a, u32 b, u32 c)
{
    (void)self;
    if (getInstance_() != NULL) {
        fn_803FE180(getInstance_(), a, b, c);
    }
}
void updateTermVersion(void* self, u32 value)
{
    (void)self;
    if (getInstance_() != NULL) {
        setTermVersion(getInstance_(), value);
    }
}
void updateOpeningState() {}
void updateServerTime() {}
void setServerTimeResult() {}
u64 getMediatorTimestamp(NetworkWiiMediatorFields* self)
{
    return *(u64*)&self->field_3A10;
}
void setMediatorState68A() {}
void setMediatorTimestamp(NetworkWiiMediatorFields* self, u32 unused, u32 a, u32 b)
{
    (void)unused;
    self->field_3A14 = b;
    self->field_3A10 = a;
}
void getMediatorState68A(NetworkWiiMediatorFields* self, void* out)
{
    *(u8*)out = self->field_68A;
}
void queryOpeningFlag208() {}
void queryOpeningFlag250() {}
void queryOpeningFlag278() {}
void queryOpeningFlag290() {}
void queryOpeningFlag2A8() {}
void queryOpeningFlag2C0() {}
s32 getAccountQuery1(void* self) { (void)self; return 0; }
s32 getAccountQuery2(void* self) { (void)self; return 0; }
s32 getAccountQuery3(void* self) { (void)self; return 0; }
s32 getAccountQuery4(void* self) { (void)self; return 0; }
s32 getAccountQuery5(void* self) { (void)self; return 0; }
void getReflectPageText() {}
void getAccountName() {}
s32 getWarningUInt(void* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return getWarningUInt2(getInstance_());
    }
    return 0;
}
void getReflectPageRange(NetworkWiiMediatorFields* self, void* out1, void* out2)
{
    *(u32*)out1 = self->field_28;
    *(u32*)out2 = self->field_2C;
}
void getReflectField30(NetworkWiiMediatorFields* self, void* out) { *(u32*)out = self->field_30; }
void getReflectField34(NetworkWiiMediatorFields* self, void* out) { *(u32*)out = self->field_34; }
void getReflectField38(NetworkWiiMediatorFields* self, void* out) { *(u32*)out = self->field_38; }
void getReflectName3C() {}
void getMediaVersionString() {}
void getStr1String() {}
void getReflectName5C() {}
void setReflectPageRange() {}
void setReflectField30() {}
void setReflectField34() {}
void setReflectField38() {}
void setReflectName3C() {}
void setReflectName5C() {}
void updatePatField854() {}
void updatePatField860() {}
s32 isShiftJisLeadByte(void* self, u8 value)
{
    (void)self;
    if ((value >= 129 && value <= 159) || (value >= 224 && value <= 239)) {
        return 1;
    }
    return 0;
}
void isNameSymbolChar() {}
void parseReflectLines() {}
void parseReflectPacket() {}
void validateReflectName() {}
void buildReflectPacket() {}
