/*
 * ESP/esp.h - the ES (title/ticket) proxy's entry points and record types.
 */
#ifndef ESP_ESP_H
#define ESP_ESP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One consumption record of a ticket: what a limit of the same index has used up. size: 0x8 */
typedef struct ESConsumption {
    /* +0x0 */ u32 kind;
    /* +0x4 */ u32 used;
} ESConsumption;

/* A ticket as the ES keeps it. size: 0x2A4 (layout not reconstructed) */
typedef struct ESTicket {
    /* +0x000 */ u8 pad_0x00[0x2A4];
} ESTicket;

typedef struct ESTicketLimit {
    /* +0x0 */ u32 kind;  /* 0 unused, 1 play-time seconds, 4 launch count, anything else is unsupported */
    /* +0x4 */ u32 value;
} ESTicketLimit; /* size: 0x8 */

/* The part of a ticket a title may inspect. size: 0xD8 (only the fields the OS reads are named) */
typedef struct ESTicketView {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ u32 ticketIdHi;
    /* +0x08 */ u32 ticketIdLo;
    /* +0x0C */ u8 pad_0x0C[0xC];
    /* +0x18 */ u8 accessMaskLow;  /* access bits 0-7 (one per content the ticket grants) */
    /* +0x19 */ u8 accessMaskHigh; /* access bits 8-15 */
    /* +0x1A */ u8 pad_0x1A[0x7E];
    /* +0x98 */ ESTicketLimit limits[8];
} ESTicketView;

s32 ESP_InitLib(void);
s32 ESP_CloseLib(void);
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* ticketView);
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* count);
s32 ESP_DiGetTicketView(const ESTicket* ticket, ESTicketView* view);
/* untyped: the title metadata blob */
s32 ESP_DiGetTmd(void* tmd, u32* size);
/* untyped: the title metadata view blob */
s32 ESP_GetTmdView(u64 titleId, void* tmdView, u32* size);
s32 ESP_GetDataDir(u64 titleId, char* path);
s32 ESP_GetTitleId(u64* titleId);
s32 ESP_GetConsumption(u64 ticketId, ESConsumption* consumptions, u32* count);

#ifdef __cplusplus
}
#endif

#endif
