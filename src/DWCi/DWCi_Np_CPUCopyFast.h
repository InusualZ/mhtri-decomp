/*
 * DWCi/DWCi_Np_CPUCopyFast.h - the declarations `src/DWCi/DWCi_Np_CPUCopyFast.c` (the DWC auth interface) owns
 * that other units use (docs/plan.md 6.5 rule 2): its data runs and its entry points.
 */
#ifndef MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H
#define MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

struct DWCiRuntime;
struct DWCiListNode;

#ifdef __cplusplus
extern "C" {
#endif

/* The two host callbacks `DWCi_initRuntime` stores in the runtime block: the allocator it is handed the block by,
 * and the command/free callback it publishes results through. */
typedef struct DWCiRuntime* (*DWCiAllocCallback)(u32 command, u32 size);
typedef void (*DWCiCommandCallback)(u32 command, u32 arg1, u32 arg2);

/* The service-locator result `DWC_SVLGetTokenAsync` fills (the SDK's `DWCSvlResult`: a status, the host and the
 * token).  size: 0x174 - the mediator memsets 0x174 bytes at its +0x2BA8 before handing it over, and
 * `getNASToken` returns +0x2BA8 + 0x45 (`svltoken`). */
typedef struct DWCSvlResult {
    /* +0x000 */ s32  status;
    /* +0x004 */ char svlhost[65];
    /* +0x045 */ char svltoken[301];
} DWCSvlResult; /* size: 0x174 (two bytes of tail padding) */

/* The DWCi runtime block `DWCi_runtime` (this unit's own object, declared in
 * `DWCi/DWCi_Np_CPUCopyFast.h`) points at.  Only the fields this unit's bodies read or
 * write are named; every other offset is an unmodelled run of the original's own layout.
 * size: 0x5B30 - the size DWCi_initRuntime asks the host allocator for, which is also where its
 * last named field ends. */
typedef struct DWCiRuntime {
    /* +0x0000 */ u8 pad_0x0000[0x4000];
    /* +0x4000 */ u8 ifSelected;              /* the head of the 0x15E-byte interface configuration */
    /* +0x4001 */ u8 pad_0x4001[0x03];          /*    `NCDGetCurrentIfConfig` fills: the selected flag, */
    /* +0x4004 */ u8 ifConfMethod;              /*    the configuration method the login reports, */
    /* +0x4005 */ u8 ifConfigRest[0x159];       /*    and the rest of the record */
    /* +0x415E */ u16 playerName_0x415E[26];  /* 52 B: the wide player name `DWCi_initRuntime` copies */
    /* +0x4192 */ char friendCode_0x4192[12];
    /* +0x419E */ char tag_0x419E[5];   /* the 5-byte tag DWCi_npSetup copies; no terminator is written */
    /* +0x41A3 */ u8 pad_0x41A3[0x01];
    /* +0x41A4 */ char* words;          /* the profile words the mode-3 request sends */
    /* +0x41A8 */ u32 wordsLength;
    /* +0x41AC */ s32 wordsHasUserId;   /* 1: the mode-3 request also sends the user id */
    /* +0x41B0 */ char region[2];       /* the "wregion" value */
    /* +0x41B2 */ char responseBuffer[0x1000]; /* the NHTTP response buffer of the request */
    /* +0x51B2 */ char postBuffer[0x800];      /* the base64 POST field values, back to back */
    /* +0x59B2 */ u8 pad_0x59B2[0x02];
    /* +0x59B4 */ s32 retryCount;       /* the NAND busy / HTTP failure retries of the auth task */
    /* +0x59B8 */ u32 resultFlag;       /* DWCi_SetResult publishes a pending result here */
    /* +0x59BC */ u32 resultValue;      /* the NAND byte count or result code the callback delivered */
    /* +0x59C0 */ u8 pad_0x59C0[0x04];
    /* +0x59C4 */ s32 httpRequestId;    /* the NHTTP request the auth task cancels on a timeout */
    /* +0x59C8 */ s32 mode_0x59C8;      /* 1 from DWCi_initRuntime, 2 while DWCi_npSetup's session is open */
    /* +0x59CC */ u8 nandCommandBlock[0xBC]; /* the `NANDCommandBlock` of the auth-data file I/O */
    /* +0x5A88 */ u8 nandFileInfo[0x8C];     /* the `NANDFileInfo` of the open auth-data file */
    /* +0x5B14 */ DWCiAllocCallback allocCallback;
    /* +0x5B18 */ DWCiCommandCallback commandCallback; /* its second argument is the runtime block on
                                                     * DWCi_initRuntime's failure path and a command
                                                     * argument in DWCi_npSetValueEx */
    /* +0x5B1C */ u8 pad_0x5B1C[0x04];
    /* +0x5B20 */ s64 requestStartTime; /* `OSGetTime` when the auth task started its HTTP request */
    /* +0x5B28 */ u64 storedUserId;     /* DWCi_initRuntime's user id, sent instead of the saved one while
                                         * DWCi_useStoredConfig is set */
} DWCiRuntime; /* size: 0x5B30 */

/* A node of the DWCi list `DWCi_FreeList` drains; the link sits at +0x18. size: 0x1C */
typedef struct DWCiListNode {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ struct DWCiListNode* next;
} DWCiListNode;

/* 0x80761260 - the auth-data file image the auth task reads and writes (0x20 bytes of I/O): the saved user id
 * first.  GUESS on the name; the rest of the record is not read by the written bodies. */
typedef struct DWCiAuthSaveData {
    /* +0x00 */ u64 userId;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ char profileWords[0x34]; /* the "prwords" answer */
    /* +0x54 */ s32 requestFailed;       /* set by the request callback on an NHTTP error */
} DWCiAuthSaveData; /* size: 0x58 */

/* The DWCi state block: the request status and the NAS answer fields (token, challenge, locator, user id, the
 * login kind, the server time offset, the response), then the work buffer behind it (+0x1D0, the
 * service-locator result) and the auth-data image (+0x360) - one run of the unit's `.bss`. */
typedef struct DWCiStateBlock {
    /* +0x000 */ s32 status;
    /* +0x004 */ char token[0x12D];
    /* +0x131 */ char challenge[0x4A];
    /* +0x17B */ char locator[0x35];
    /* +0x1B0 */ u64 userId;
    /* +0x1B8 */ u32 loginKind;          /* 1, or 2 when the server created the account (returncd 40) */
    /* +0x1BC */ u8 pad_0x1BC[0x04];
    /* +0x1C0 */ s64 serverTimeOffset;   /* the server's datetime minus OSGetTime */
    /* +0x1C8 */ NHTTPResponse* response; /* the answer the callback keeps and the auth task destroys */
    /* +0x1CC */ u8 pad_0x1CC[0x04];
    /* +0x1D0 */ DWCSvlResult svl;
    /* +0x344 */ u8 pad_0x344[0x1C];
    /* +0x360 */ DWCiAuthSaveData saveImage;
} DWCiStateBlock; /* size: 0x3B8 */

/* The request list head; the second word is never touched by the written bodies (the target object's row is 8
 * bytes and the next object starts 8 bytes on - an approximation of the original's record). */
typedef struct DWCiFreeList {
    /* +0x00 */ struct DWCiListNode* head;
    /* +0x04 */ u8 unused_0x04[4];
} DWCiFreeList; /* size: 0x08 */

/* The `.sbss` run 0x807957D0..0x807957F8: the one-shot flag and value of the console friend code, the request
 * list, the flag that sends the runtime block's stored user id instead of the saved one, the argument
 * `DWCi_npStart` remembers (the server environment index), the runtime block and the auth state (retail reloads
 * it, hence volatile). */
extern s32 DWCi_friendCodeReady;
extern u64 DWCi_consoleFriendCode;
extern DWCiFreeList DWCi_freeList;
extern u32 DWCi_useStoredConfig;
extern u32 DWCi_initArgument;
extern struct DWCiRuntime* DWCi_runtime;
extern volatile s32 DWCi_state;

/* 0x80794200 - the pointer to the auth-data file path `DWCi_authDataTask` opens, creates and deletes. */
extern char* DWCi_authDataPathPtr;


/* The `.bss` run 0x80760F00..0x807613B8: the state block (0x1D0 B, first word the request status), the 0x190-byte
 * work buffer, the auth-data image and the host name the request copies out of its URL. GUESS on the names. */
extern u32 DWCi_stateBlock[0x1D0 / 4];
extern u8 DWCi_workBuffer[0x190];
extern DWCiAuthSaveData DWCi_authSaveData;
extern char DWCi_authHostName[0x100];

/* 0x80508830 - brings the runtime block up for a NAS request (player name, friend code, stored user id, the two
 * host callbacks); 0x805089F0 - the service-locator form (the service tag). Both answer 1 on success. */
u32 DWCi_initRuntime(char* playerName, char* friendCode, u64 storedUserId, DWCiAllocCallback allocCallback,
                     DWCiCommandCallback commandCallback);
u32 DWCi_npSetup(char* tag, DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback);
/* 0x80509180 - non-zero once the request finished; 0x805091D0 - it finished successfully; 0x805091F0 - its status
 * word; 0x80509200 - the work buffer the answer is parsed into. */
u32 DWCi_AdvanceStatus(void);
u32 DWCi_IsStatusReady(void);
u32 DWCi_GetStatus(void);
u8* DWCi_GetWorkBuffer(void);

/* 0x80507C40 - copies `len` bytes by the widest unit both pointers allow. */
void DWCi_Np_CPUCopyFast(u8* dst, const u8* src, u32 len);
/* 0x80508A70 - one step of the auth state machine (called by `DWC_NASLoginProcess`/`DWC_SVLProcess`). */
void DWCi_authDataTask(void);
/* 0x80508630 - the console friend code, fetched once. */
u64 DWCi_GetConsoleFriendCode(void);
/* 0x80508750 - drains the request list; 0x805087A0 - resets the state block and starts the auth layer
 * (`DWC_Init` treats 0 as failure). */
u32 DWCi_FreeList(void);
u32 DWCi_npStart(u32 arg);

/* The `.data` run 0x8062FF90..0x806302C8, defined by the unit in address order. */
extern char DWCi_reportFriendCodeFormat[];
extern char DWCi_reportFriendCodeFailedFormat[];
extern char DWCi_authDataPath[];
extern char DWCi_acUrlTest[];
extern char DWCi_acUrlProd[];
extern char DWCi_acUrlDev[];
extern char* DWCi_acUrlTable[3];
extern char DWCi_prUrlTest[];
extern char DWCi_prUrlProd[];
extern char DWCi_prUrlDev[];
extern char* DWCi_prUrlTable[3];
extern char DWCi_reportAuthProcessing[];
extern char DWCi_reportMemoryShortage[];
extern char DWCi_reportIfConfigQueryFailed[];
extern char DWCi_reportHttpStartFailed[];
extern char DWCi_reportHttpDestroy[];
extern char DWCi_reportUserIdRead[];
extern char DWCi_reportUserIdReadSize[];
extern char DWCi_reportUserIdDelete[];
extern char DWCi_reportAccountCreateTimeout[];
extern char DWCi_reportUserIdWriteSize[];
extern char DWCi_reportLoginTimeout[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H */
