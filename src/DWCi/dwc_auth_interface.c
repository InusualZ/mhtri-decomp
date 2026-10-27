/*
 * DWCi/dwc_auth_interface.c - the DWC auth interface: the auth state machine over the NAND auth-data file, the
 * NAS request builder and the request's completion callback.
 *
 * RANGE. `.text` 0x80508750..0x8050A710 (14 functions, in address order); `.data` 0x80630020..0x806307F0;
 *   `.bss` 0x80760F00..0x807613B8; `.sdata` 0x80794200..0x80794330; `.sbss` 0x807957E0..0x807957F8. The left
 *   edges: every function addresses its `.data` off `DWCi_authDataPath` (0x80630020), which MWCC does only for
 *   the object that starts the TU's `.data` (`DWCi/dwc_nonport.c` owns what lies before). The right edges are
 *   the TU's own: the `.data` run 0x806302C8..0x806307F0 is the request and callback strings, and `.sdata`
 *   0x807942FC..0x80794330 is referenced only by the callback (`callers.py`).
 * FLAGS. `cflags_base` (the retail functions start 16-byte aligned, `gap_*` words between them; the lib's
 *   `cflags_dwc` packs on 4); measurement in docs/network.md.
 * NAMES. `DWCi_Auth_*` follow the SDK's own `DWCi_Auth_EndProcess` string (`dwc_nasfunc.c`'s `.data`); the rest
 *   are GUESSes from the bodies (`DWCi_authDataTask`, `DWCi_npStart`, `DWCi_initRuntime`, `DWCi_npSetup`, the
 *   status accessors, the `.bss`/`.sbss` object names, `DWCi_Auth_RequestCallback`). The file name is the SDK's
 *   `dwc_auth_interface.c` (GUESS: no `__FILE__` string).
 * SHAPES. Each POST field of `DWCi_Auth_StartRequest` is `DWCI_AUTH_ADD_POST` (the next-field pointer is stepped
 *   before the add call). `DWCi_authDataTask`'s idle states 0/24/25/26 `return`, which keeps the 27-entry table.
 *   `.bss` objects are emitted in first-use order, the host name (a static of `DWCi_Auth_StartRequest`) last; the
 *   auth-data image is 32-byte aligned (NAND I/O), which leaves the service-locator result's 0x1C-byte tail gap.
 *   The `.sbss` objects are defined last first. `NULL` is the SDK's `((void*)0)` (playbook 125: the callback's
 *   token and response tests compare against a register zero). The callback reaches the state block as an object
 *   (`DWCI_AUTH_STATE`), the service-locator result and the profile record as their own globals, so MWCC
 *   addresses them off the one `.bss` base the way retail does; its return-code test is `if/else` with the final
 *   status store after it, and `separators` holds the `strtok` set.
 * RESIDUALS. None in the code (14/14 rows at 100 %, `flipcheck` READY, a trial flip links the retail DOL).
 *   Relocation names that differ at equal addresses: ours names the section symbols `...data.0`/`...bss.0` and the
 *   `.sdata` pool `@NNN` where retail names `DWCi_authDataPath`, `DWCi_stateBlock`, `DWCi_npEmptyFriendCode`,
 *   `DWCi_npEmptyPlayerName`, `DWCi_authDataStateTable` and `DWCi_urlSchemeSeparator`. `.data` 6 B and `.sdata`
 *   3 B short of the claim (the target's alignment fill).
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"    /* memset: the runtime unit's header (rule 2) */
#include "NHTTP/d_nhttp.h"               /* NHTTPi_RegisterCallbacks: its owner's header (rule 2) */
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers, and - through it - */
                                         /* the two owners' headers for the data they now own       */
#include "NCD/ncdsystem.h"               /* NCDGetCurrentIfConfig: its owner's header (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h"   /* strncpy / wcsncpy */
#include "NAND/nand.h"                   /* the NAND file calls of the auth task */
#include "unsplit/OS.h"                  /* OSGetTime, OS_BUS_CLOCK */
#include "SC/sc.h"                       /* SCGetLanguage, SCGetProductArea */
#include "MSL_C/alloc.h"                 /* sprintf, strcpy */
#include "MSL_C/strstr.h"                /* strstr, wcslen */
#include "DWCi/dwc_base64.h"              /* DWCi_Base64Encode/Decode */
#include "DWCi/dwc_nonport.h"             /* DWCi_GetConsoleFriendCode */

/* The SDK's own `NULL` (playbook 125): a pointer test against it compares two registers. */
#undef NULL
#define NULL ((void*)0)








/* This unit's own entry points, in address order. */
u32 DWCi_npStart(u32 arg);
u32 DWCi_npSetup(char* tag, DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback);
void DWCi_npSetValue(u32 value);
void DWCi_npSetValueEx(u32 value);
void DWCi_SetResult(u32 arg);
u32 DWCi_initRuntime(char* playerName, char* friendCode, u64 storedUserId, DWCiAllocCallback allocCallback,
                     DWCiCommandCallback commandCallback);
void DWCi_Auth_StartRequest(s32 mode, u16* playerName, char* friendCode, u64 userId);

/* The unit's data, in address order: MWCC places an object whose size is a multiple of 8 on 8 and every other
 * on 4, which reproduces the retail offsets; `DWCi_authDataPath` states its own `aligned(8)`. */


/* 0x80630020 (0x16): the save file's path.  `DWCi_authDataTask` hands the `.sdata` word that
 * points here to NANDPrivateOpenAsync / NANDPrivateDeleteAsync, and DWCi_initRuntime's report
 * lines are addressed as displacements off it.  `aligned(8)` is the target's layout, not
 * decoration (see the block comment above). */
__attribute__((aligned(8))) char DWCi_authDataPath[] = "/shared2/DWC_AUTHDATA";

/* 0x80794200: the path pointer the auth task hands the NAND calls (the unit's first `.sdata` object). */
char* DWCi_authDataPathPtr = DWCi_authDataPath;

/* The unit's `.bss` run 0x80760F00..0x807613B8 (MWCC emits the objects in the order the code first uses them;
 * the request's host name is a static of `DWCi_Auth_StartRequest` and comes last). */
DWCiAuthProfile DWCi_authProfile;
__attribute__((aligned(32))) DWCiAuthSaveData DWCi_authSaveData;
DWCSvlResult DWCi_authSvlResult;
u32 DWCi_stateBlock[0x1D0 / 4];

/* The unit's `.sbss` run 0x807957D0..0x807957F8, defined in reverse: MWCC emits `.sbss` objects last first. */
volatile s32 DWCi_state;
DWCiRuntime* DWCi_runtime;
u32 DWCi_initArgument;
u32 DWCi_useStoredConfig;
DWCiFreeList DWCi_freeList;

/* 0x80630038..0x806300AC: the account-creation URL per server environment, then the three-entry
 * table the environment index selects from (test / production / development, in that order). */
char DWCi_acUrlTest[] = "https://naswii.test.nintendowifi.net/ac";
char DWCi_acUrlProd[] = "https://naswii.nintendowifi.net/ac";
char DWCi_acUrlDev[] = "https://naswii.dev.nintendowifi.net/ac";
char* DWCi_acUrlTable[3] = { DWCi_acUrlTest, DWCi_acUrlProd, DWCi_acUrlDev };

/* 0x806300B8..0x8063012C: the profile ("pr") URLs and their table, the same shape. */
char DWCi_prUrlTest[] = "https://naswii.test.nintendowifi.net/pr";
char DWCi_prUrlProd[] = "https://naswii.nintendowifi.net/pr";
char DWCi_prUrlDev[] = "https://naswii.dev.nintendowifi.net/pr";
char* DWCi_prUrlTable[3] = { DWCi_prUrlTest, DWCi_prUrlProd, DWCi_prUrlDev };

/* 0x80630138..0x8063025C: the state machine's report lines, in the order its arms reach them
 * (the four DWCi_initRuntime prints first, then the auth-data task's own). */
char DWCi_reportAuthProcessing[] = " auth is processing\n";
char DWCi_reportMemoryShortage[] = " memory shortage\n";
char DWCi_reportIfConfigQueryFailed[] = " NCDGetCurrentIfConfig failed.[%d]\n";
char DWCi_reportHttpStartFailed[] = " failed to start NHTTP\n";
char DWCi_reportHttpDestroy[] = "NHTTPDestroyResponse()\n";
char DWCi_reportUserIdRead[] = " read userid = %llu\n";
char DWCi_reportUserIdReadSize[] = " illegal size userid read = %d\n";
char DWCi_reportUserIdDelete[] = " delete illegal userid.\n";
char DWCi_reportAccountCreateTimeout[] = " acctcreate timeout.\n";
char DWCi_reportUserIdWriteSize[] = " illegal size userid write = %d\n";
char DWCi_reportLoginTimeout[] = " login timeout.\n";




/* 0x80508750 (0x50): drain the request list, calling DWC_Free(0xC, node, 0) per node. */
u32 DWCi_FreeList(void) {
    DWCiListNode* node = DWCi_freeList.head;
    while (node != 0) {
        DWCiListNode* next = node->next;
        DWC_Free(0xC, node, 0);
        DWCi_freeList.head = next;
        node = next;
    }
    return 0;
}

/* 0x805087A0 (0x88): clear the whole DWCi state block and its trailing regions, drop the runtime
 * block, remember the argument and return 1.  The reset every session begins with. */

u32 DWCi_npStart(u32 arg) {
    memset(DWCi_stateBlock, 0, 0x1D0);
    memset(&DWCi_authSvlResult, 0, sizeof(DWCSvlResult));
    memset(&DWCi_authSaveData, 0, 0x20);
    DWCi_runtime = 0;
    DWCi_state = 0;
    DWCi_initArgument = arg;
    ((DWCiStateBlock*)DWCi_stateBlock)->loginKind = 0;
    DWCi_useStoredConfig = 0;
    return 1;
}

/* 0x80508830 (0x1B8): bring a DWC session's runtime block up.  The host's allocator hands over the
 * 0x5B30-byte block, the interface configuration is read into its +0x4000 region, the HTTP layer is
 * started with the caller's two command callbacks and the player name / friend code are copied in -
 * or the block goes back through the callback and 0 is answered. */
u32 DWCi_initRuntime(char* playerName, char* friendCode, u64 storedUserId, DWCiAllocCallback allocCallback,
                     DWCiCommandCallback commandCallback) {
    DWCiRuntime* runtime;
    s32 error;

    if (DWCi_state != 0 && DWCi_state != 0x1A) {
        DWC_Printf(0x1000000, DWCi_reportAuthProcessing);
        return 0;
    }
    DWCi_runtime = allocCallback(0, 0x5B30);
    if (DWCi_runtime == 0) {
        DWC_Printf(0x1000000, DWCi_reportMemoryShortage);
        return 0;
    }
    memset(DWCi_runtime, 0, 0x5B30);
    DWCi_runtime->allocCallback = allocCallback;
    DWCi_runtime->commandCallback = commandCallback;
    error = NCDGetCurrentIfConfig(&DWCi_runtime->ifSelected);
    if (error != 0) {
        DWC_Printf(0x1000000, DWCi_reportIfConfigQueryFailed, error);
    } else if (NHTTPi_RegisterCallbacks(DWCi_npSetValue, DWCi_npSetValueEx, 17) < 0) {
        DWC_Printf(0x1000000, DWCi_reportHttpStartFailed);
    } else {
        wcsncpy(DWCi_runtime->playerName_0x415E, (const u16*)playerName, 26);
        strncpy(DWCi_runtime->friendCode_0x4192, friendCode, 12);
        DWCi_runtime->mode_0x59C8 = 1;
        memset(DWCi_stateBlock, 0, 0x1D0);
        ((DWCiStateBlock*)DWCi_stateBlock)->loginKind = 0;
        runtime = DWCi_runtime;
        runtime->storedUserId = storedUserId;
        DWCi_state = 1;
        return 1;
    }
    DWCi_runtime->commandCallback(0, (u32)DWCi_runtime, 0);
    return 0;
}

/* 0x805089F0 (0x7C): open a session: bring the runtime block up under the band's two empty-name
 * defaults and the caller's two host callbacks, clear the work buffer, copy the 5-byte tag into the
 * runtime block and set its mode. */
u32 DWCi_npSetup(char* tag, DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback) {
    DWCi_initRuntime((char*)L"", "", 0, allocCallback, commandCallback);
    memset(&DWCi_authSvlResult, 0, sizeof(DWCSvlResult));
    strncpy(DWCi_runtime->tag_0x419E, tag, 5);
    DWCi_runtime->mode_0x59C8 = 2;
    return 1;
}

/* 0x80508A70 (0x708): one step of the auth state machine: the saved user id is read from (or written to) the
 * NAND auth-data file, the login request is started and its answer or timeout awaited, and the HTTP layer is
 * torn down at the end. */
void DWCi_authDataTask(void) {
    DWCiStateBlock* state;

    switch (DWCi_state) {
    case 1:
        if (DWCi_runtime->mode_0x59C8 == 3) {
            DWCi_state = 19;
        } else if (DWCi_authSaveData.userId != 0) {
            DWCi_state = 19;
        } else {
            NANDPrivateOpenAsync(DWCi_authDataPathPtr, (NANDFileInfo*)DWCi_runtime->nandFileInfo, 1,
                                 (NANDAsyncCallback)DWCi_SetResult,
                                 (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
            DWCi_state = 2;
        }
        break;
    case 2:
        if (DWCi_runtime->resultFlag != 0) {
            DWCi_Auth_CheckNandResult(3, 1, 9, 7);
        }
        break;
    case 3:
        NANDReadAsync((NANDFileInfo*)DWCi_runtime->nandFileInfo, &DWCi_authSaveData, 0x20,
                      (NANDAsyncCallback)DWCi_SetResult, (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 4;
        break;
    case 4:
        if (DWCi_runtime->resultFlag != 0) {
            if (DWCi_runtime->resultValue == 0x20) {
                DWC_Printf(0x1000000, DWCi_reportUserIdRead, DWCi_authSaveData.userId);
                DWCi_runtime->resultValue = 0;
            } else if ((s32)DWCi_runtime->resultValue >= 0) {
                DWC_Printf(0x1000000, DWCi_reportUserIdReadSize, DWCi_runtime->resultValue);
                DWCi_runtime->resultValue = (u32)-1;
            }
            DWCi_Auth_CheckNandResult(17, 3, 27, 5);
        }
        break;
    case 5:
        NANDCloseAsync((NANDFileInfo*)DWCi_runtime->nandFileInfo, (NANDAsyncCallback)DWCi_SetResult,
                       (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 6;
        break;
    case 6:
        if (DWCi_runtime->resultFlag != 0) {
            DWCi_Auth_CheckNandResult(7, 5, 27, 27);
        }
        break;
    case 7:
        NANDPrivateDeleteAsync(DWCi_authDataPathPtr, (NANDAsyncCallback)DWCi_SetResult,
                               (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 8;
        break;
    case 8:
        if (DWCi_runtime->resultFlag != 0) {
            if (DWCi_runtime->resultValue == 0) {
                DWC_Printf(0x1000000, DWCi_reportUserIdDelete);
            }
            DWCi_Auth_CheckNandResult(9, 7, 9, 27);
        }
        break;
    case 9:
        DWCi_Auth_StartRequest(0, NULL, NULL, 0);
        DWCi_runtime->requestStartTime = OSGetTime();
        DWCi_state = 10;
        break;
    case 10:
        if (*(volatile s32*)DWCi_stateBlock == 1) {
            DWCi_state = 11;
        } else if (*(volatile s32*)DWCi_stateBlock >= -20999 && *(volatile s32*)DWCi_stateBlock <= -20102) {
            DWCi_state = 23;
        } else if (*(volatile s32*)DWCi_stateBlock < 0) {
            DWCi_runtime->retryCount++;
            if (DWCi_runtime->retryCount < 3) {
                DWCi_state = 9;
            } else {
                DWCi_state = 23;
            }
        } else if ((OSGetTime() - DWCi_runtime->requestStartTime) / (OS_BUS_CLOCK / 4 / 1000) > 30000) {
            DWC_Printf(0x1000000, DWCi_reportAccountCreateTimeout);
            DWCi_state = 21;
        }
        break;
    case 11:
        NANDPrivateCreateAsync(DWCi_authDataPathPtr, 0x3F, 0, (NANDAsyncCallback)DWCi_SetResult,
                               (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 12;
        break;
    case 12:
        if (DWCi_runtime->resultFlag != 0) {
            DWCi_Auth_CheckNandResult(13, 11, 27, 27);
        }
        break;
    case 13:
        NANDPrivateOpenAsync(DWCi_authDataPathPtr, (NANDFileInfo*)DWCi_runtime->nandFileInfo, 2,
                             (NANDAsyncCallback)DWCi_SetResult, (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 14;
        break;
    case 14:
        if (DWCi_runtime->resultFlag != 0) {
            DWCi_Auth_CheckNandResult(15, 13, 27, 27);
        }
        break;
    case 15:
        NANDWriteAsync((NANDFileInfo*)DWCi_runtime->nandFileInfo, &DWCi_authSaveData, 0x20,
                       (NANDAsyncCallback)DWCi_SetResult, (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 16;
        break;
    case 16:
        if (DWCi_runtime->resultFlag != 0) {
            if (DWCi_runtime->resultValue == 0x20) {
                DWCi_runtime->resultValue = 0;
            } else {
                DWC_Printf(0x1000000, DWCi_reportUserIdWriteSize, DWCi_runtime->resultValue);
                DWCi_runtime->resultValue = (u32)-1;
            }
            DWCi_Auth_CheckNandResult(17, 15, 27, 27);
        }
        break;
    case 17:
        NANDCloseAsync((NANDFileInfo*)DWCi_runtime->nandFileInfo, (NANDAsyncCallback)DWCi_SetResult,
                       (NANDCommandBlock*)DWCi_runtime->nandCommandBlock);
        DWCi_state = 18;
        break;
    case 18:
        if (DWCi_runtime->resultFlag != 0) {
            DWCi_Auth_CheckNandResult(19, 17, 27, 27);
        }
        break;
    case 19:
        if (DWCi_useStoredConfig != 0) {
            DWCi_Auth_StartRequest(DWCi_runtime->mode_0x59C8, DWCi_runtime->playerName_0x415E,
                                   DWCi_runtime->friendCode_0x4192, DWCi_runtime->storedUserId);
        } else {
            DWCi_Auth_StartRequest(DWCi_runtime->mode_0x59C8, DWCi_runtime->playerName_0x415E,
                                   DWCi_runtime->friendCode_0x4192, DWCi_authSaveData.userId);
        }
        DWCi_runtime->requestStartTime = OSGetTime();
        DWCi_state = 20;
        break;
    case 20:
        if (*(volatile s32*)DWCi_stateBlock == 1) {
            DWCi_state = 23;
        } else if (*(volatile s32*)DWCi_stateBlock >= -20999 && *(volatile s32*)DWCi_stateBlock <= -20102) {
            DWCi_state = 23;
        } else if (*(volatile s32*)DWCi_stateBlock < 0) {
            DWCi_runtime->retryCount++;
            if (DWCi_runtime->retryCount < 3) {
                DWCi_state = 19;
            } else {
                DWCi_state = 23;
            }
        } else if ((OSGetTime() - DWCi_runtime->requestStartTime) / (OS_BUS_CLOCK / 4 / 1000) > 30000) {
            DWC_Printf(0x1000000, DWCi_reportLoginTimeout);
            DWCi_state = 21;
        }
        break;
    case 21:
        DWCi_state = 22;
        NHTTPCancelRequest(DWCi_runtime->httpRequestId);
        break;
    case 22:
        if (*(volatile s32*)DWCi_stateBlock != 0) {
            *(volatile s32*)DWCi_stateBlock = -20100;
            DWCi_state = 23;
        }
        break;
    case 23:
        state = (DWCiStateBlock*)DWCi_stateBlock;
        DWCi_state = 24;
        if (state->response != NULL) {
            DWC_Printf(0x1000000, DWCi_reportHttpDestroy);
            NHTTPDestroyResponse(state->response);
            state->response = NULL;
        }
        NHTTPDestroy(DWCi_Auth_EndProcess);
        break;
    case 0:
    case 24:
    case 25:
    case 26:
        return;
    }
}

/* 0x80509180 (0x44): the state ladder 0x19 -> 0x1A; 1 while the state is 0, 0x1A or just advanced. */
u32 DWCi_AdvanceStatus(void) {
    if (DWCi_state == 0x19) {
        DWCi_state = 0x1A;
        return 1;
    }
    if (DWCi_state == 0 || DWCi_state == 0x1A) {
        return 1;
    }
    return 0;
}

/* 0x805091D0 (0x18): `state == 1`. */
u32 DWCi_IsStatusReady(void) {
    return DWCi_stateBlock[0] == 1;
}

/* 0x805091F0 (0xC): the DWCi state word. */
u32 DWCi_GetStatus(void) {
    return DWCi_stateBlock[0];
}

/* 0x80509200 (0xC): returns the service-locator result the request callback fills. */
DWCSvlResult* DWCi_GetWorkBuffer(void) {
    return &DWCi_authSvlResult;
}

/* 0x80509210 (0x18): issue the host callback's command 13 with `value`. */
void DWCi_npSetValue(u32 value) {
    DWCi_runtime->allocCallback(13, value);
}

/* 0x80509230 (0x1C): the three-argument form of that same command. */
void DWCi_npSetValueEx(u32 value) {
    DWCi_runtime->commandCallback(13, value, 0);
}

/* 0x80509250 (0x18): publish a flag + the argument into the runtime block. */
void DWCi_SetResult(u32 arg) {
    DWCi_runtime->resultFlag = 1;
    DWCi_runtime->resultValue = arg;
}

/* Base64-encodes `length` bytes of `value` into the POST buffer, terminates it, adds it to the request under
 * `label` and moves past it. */
#define DWCI_AUTH_ADD_POST(label, value, length)                \
    encoded = DWCi_Base64Encode((value), (length), post, 0x800);     \
    post[encoded] = 0;                                         \
    field = post;                                              \
    post += encoded + 1;                                       \
    NHTTPAddPostDataAscii(request, (label), field)

/* 0x80509270 (0xB40): builds and sends the NAS request of `mode` (0 account creation, 1 login, 2 service
 * locator, 3 profile words): the URL for the configured server, the headers, and every POST field base64
 * encoded into the runtime block's buffer. */
void DWCi_Auth_StartRequest(s32 mode, u16* playerName, char* friendCode, u64 userId) {
    static char hostName[0x100];
    char text[0x100];
    char homeDir[0x40];
    OSCalendarTime calendar;
    u8 macAddress[8];
    NANDStatus status;
    u32 serial;
    struct NHTTPRequest* request;
    char* post = DWCi_runtime->postBuffer;
    char* url;
    char* host;
    const char* value;
    const char* code;
    s32 error;
    s32 encoded;
    char* field;

    if (DWCi_runtime->mode_0x59C8 != 3) {
        url = DWCi_acUrlTable[DWCi_initArgument];
    } else {
        url = DWCi_prUrlTable[DWCi_initArgument];
    }
    DWC_Printf(0x1000000, " url = %s\n", url);
    request = NHTTPCreateRequest(url, 1, (u32)DWCi_runtime->responseBuffer, 0x1000,
                                 (NHTTPCompleteCallback)DWCi_Auth_RequestCallback, 0);
    if (NHTTPClearRootCA(request) != 0) {
        OSPanic("dwc_auth_interface.c", 1033, "\tDWC Auth: failed to set RootCA\n");
    }
    if (NHTTPClearClientCert(request) != 0) {
        OSPanic("dwc_auth_interface.c", 1037, "\tDWC Auth: failed to set ClientCert\n");
    }
    NHTTPSetSystemProxy(request);
    NHTTPSetVerifyOption(request, 2);
    NHTTPAddHeaderField(request, "User-Agent", "RVL SDK/1.0");
    if (DWCi_runtime->mode_0x59C8 != 3) {
        host = strstr(DWCi_acUrlTable[DWCi_initArgument], "//");
    } else {
        host = strstr(DWCi_prUrlTable[DWCi_initArgument], "//");
    }
    strcpy(hostName, host + 2);
    host = strstr(hostName, "/");
    *host = 0;
    NHTTPAddHeaderField(request, "Host", hostName);
    NHTTPAddHeaderField(request, "HTTP_X_GAMECD", OSGetAppGamename());
    DWC_Printf(0x1000000, " HTTP_X_GAMECD = %s\n", OSGetAppGamename());

    switch (mode) {
    case 0:
        value = "acctcreate";
        DWCI_AUTH_ADD_POST("action", value, strlen(value));
        DWC_Printf(0x1000000, " action = acctcreate\n");
        break;
    case 1:
        value = "login";
        DWCI_AUTH_ADD_POST("action", value, strlen(value));
        DWCI_AUTH_ADD_POST("gsbrcd", friendCode, strlen(friendCode));
        DWC_Printf(0x1000000, " action = login\n");
        DWC_Printf(0x1000000, " gsbrcd = %s\n", friendCode);
        sprintf(text, "%013llu", userId);
        DWCI_AUTH_ADD_POST("userid", text, strlen(text));
        DWC_Printf(0x1000000, " userid = 0x%016llx\n", userId);
        DWCI_AUTH_ADD_POST("ingamesn", (const char*)playerName, wcslen(playerName) * 2);
        break;
    case 2:
        value = "svcloc";
        DWCI_AUTH_ADD_POST("action", value, strlen(value));
        DWC_Printf(0x1000000, " action = svcloc\n");
        value = DWCi_runtime->tag_0x419E;
        DWCI_AUTH_ADD_POST("svc", value, strlen(value));
        DWC_Printf(0x1000000, " svc = %s\n", DWCi_runtime->tag_0x419E);
        sprintf(text, "%013llu", userId);
        DWCI_AUTH_ADD_POST("userid", text, strlen(text));
        DWC_Printf(0x1000000, " userid = 0x%016llx\n", userId);
        break;
    case 3:
        if (DWCi_runtime->wordsHasUserId == 1) {
            sprintf(text, "%013llu", userId);
            DWCI_AUTH_ADD_POST("userid", text, strlen(text));
            DWC_Printf(0x1000000, " userid = 0x%016llx\n", userId);
        }
        value = DWCi_runtime->region;
        DWCI_AUTH_ADD_POST("wregion", value, strlen(value));
        DWC_Printf(0x1000000, " wregion = %s\n", DWCi_runtime->region);
        NHTTPAddPostDataAscii(request, "wtype", "");
        DWC_Printf(0x1000000, " wtype = \n");
        value = "UTF-16BE";
        DWCI_AUTH_ADD_POST("wenc", value, strlen(value));
        DWC_Printf(0x1000000, " wenc = UTF-16BE\n");
        DWCI_AUTH_ADD_POST("words", DWCi_runtime->words, DWCi_runtime->wordsLength);
        break;
    }

    value = "001000";
    DWCI_AUTH_ADD_POST("sdkver", value, strlen(value));
    value = OSGetAppGamename();
    DWCI_AUTH_ADD_POST("gamecd", value, strlen(value));

    error = NANDGetHomeDir(homeDir);
    if (error == 0) {
        error = NANDGetStatus(homeDir, &status);
        if (error == 0) {
            if (status.groupId == 2) {
                strncpy(text, "02", 3);
            } else {
                sprintf(text, "%c%c", status.groupId >> 8, status.groupId & 0xFF);
            }
        } else {
            DWC_Printf(0x1000000, " NANDGetStatus failed.[%d]\n", error);
            strncpy(text, "00", 3);
        }
    } else {
        DWC_Printf(0x1000000, " NANDGetHomeDir failed.[%d]\n", error);
        strncpy(text, "00", 3);
    }
    DWCI_AUTH_ADD_POST("makercd", text, strlen(text));
    DWC_Printf(0x1000000, " makercd = %s\n", text);

    value = "1";
    DWCI_AUTH_ADD_POST("unitcd", value, strlen(value));

    NCDGetWirelessMacAddress(macAddress);
    sprintf(text, "%02x%02x%02x%02x%02x%02x", macAddress[0], macAddress[1], macAddress[2], macAddress[3],
            macAddress[4], macAddress[5]);
    DWCI_AUTH_ADD_POST("macadr", text, strlen(text));
    DWC_Printf(0x1000000, " macadr = %s\n", text);

    sprintf(text, "%02d", SCGetLanguage());
    DWC_Printf(0x1000000, " lang = %s\n", text);
    DWCI_AUTH_ADD_POST("lang", text, strlen(text));

    if (DWCi_runtime->mode_0x59C8 != 3) {
        OSTicksToCalendarTime(OSGetTime(), &calendar);
        sprintf(text, "%02d%02d%02d%02d%02d%02d", calendar.year % 100, calendar.mon + 1, calendar.mday,
                calendar.hour, calendar.min, calendar.sec);
        DWCI_AUTH_ADD_POST("devtime", text, strlen(text));
        DWC_Printf(0x1000000, " CalendarTime = %s\n", text);
    }
    if (DWCi_runtime->ifSelected == 1 && DWCi_runtime->mode_0x59C8 != 3) {
        sprintf(text, "%02d", DWCi_runtime->ifConfMethod);
        DWC_Printf(0x1000000, " confmethod = %s\n", text);
        DWCI_AUTH_ADD_POST("confmethod", text, strlen(text));
    }
    code = SCGetProductCode();
    if (code != NULL && SCGetProductSN(&serial) != 0) {
        sprintf(text, "%s%09d", code, serial);
        DWC_Printf(0x1000000, " csnum = %s\n", text);
        DWCI_AUTH_ADD_POST("csnum", text, strlen(text));
    }
    sprintf(text, "%016lld", DWCi_GetConsoleFriendCode());
    DWC_Printf(0x1000000, " cfc = %s\n", text);
    DWCI_AUTH_ADD_POST("cfc", text, strlen(text));
    if (DWCi_runtime->mode_0x59C8 != 3) {
        sprintf(text, "%02d", SCGetProductArea());
        DWC_Printf(0x1000000, " region = %s\n", text);
        DWCI_AUTH_ADD_POST("region", text, strlen(text));
    }
    DWCi_runtime->httpRequestId = NHTTPSendRequestAsync(request);
    *(volatile s32*)DWCi_stateBlock = 0;
}

/* Base64-decodes the value after `key` in `token` into `value` (NUL-terminated); answers the decoded length. */
#define DWCI_AUTH_DECODE(key)                                                                     \
    (decoded = DWCi_Base64Decode(&token[strlen(key)], strlen(token) - strlen(key), value, 0x100), \
     value[decoded] = 0, decoded)

/* The state block as the object it is: the callback addresses its fields as a global's (no pointer local). */
#define DWCI_AUTH_STATE (*(DWCiStateBlock*)DWCi_stateBlock)

/* 0x80509DB0 (0x960): the completion callback of the NAS request: keeps the response, then parses the
 * "key=value" lines (each value base64) into the state block and sets the request status. */
void DWCi_Auth_RequestCallback(s32 result, struct NHTTPResponse* response, s32 size) {
    char value[0x100];
    OSCalendarTime calendar;
    char* token;
    char* errorBody;
    char* body;
    char returnCode[4];
    const char* separators;
    s64 serverTime;
    s32 decoded;
    s32 haveReturnCode;
    s32 code;

    if (DWCI_AUTH_STATE.response != NULL) {
        DWC_Printf(0x1000000, DWCi_reportHttpDestroy);
        NHTTPDestroyResponse(DWCI_AUTH_STATE.response);
    }
    DWCI_AUTH_STATE.response = response;
    DWC_Printf(0x1000000, " request_callback = %d\n", result);
    if (result == 8) {
        DWC_Printf(0x1000000, " nhttp canceled(%d)\n", result);
        DWCI_AUTH_STATE.status = 2;
        return;
    }
    if (result != 0) {
        if (result == 14) {
            DWC_Printf(0x1000000, " ssl error(%d)\n", NHTTPGetSSLError());
        }
        DWC_Printf(0x1000000, " nhttp error(%d)\n", result);
        DWCI_AUTH_STATE.status = -20100;
        DWCi_authProfile.requestFailed = 1;
        return;
    }
    code = NHTTPGetResultCode(response);
    if (code != 200) {
        DWC_Printf(0x1000000, " status code is not 200, but %d\n", code);
        DWCI_AUTH_STATE.status = -23000 - code;
        if (NHTTPGetBodyAll(response, (u32*)&errorBody) > 0) {
            DWC_Printf(0x1000000, "%s\n", errorBody);
        } else {
            DWC_Printf(0x1000000, "no body\n", errorBody);
        }
        return;
    }
    separators = "&\r\n";
    haveReturnCode = 0;
    if (NHTTPGetBodyAll(response, (u32*)&body) > 0) {
        DWC_Printf(0x1000000, " %s\n", body);
        for (token = strtok(body, separators); token != NULL; token = strtok(NULL, separators)) {
            if (strncmp(token, "retry=", strlen("retry=")) == 0) {
                DWC_Printf(0x1000000, " (%d) retry=%s\n", DWCI_AUTH_DECODE("retry="), value);
            } else if (strncmp(token, "returncd=", strlen("returncd=")) == 0) {
                decoded = DWCI_AUTH_DECODE("returncd=");
                strcpy(returnCode, value);
                DWC_Printf(0x1000000, " (%d) returncd=%s\n", decoded, value);
                haveReturnCode = 1;
            } else if (strncmp(token, "datetime=", strlen("datetime=")) == 0) {
                decoded = DWCI_AUTH_DECODE("datetime=");
                if (sscanf(value, "%04d%02d%02d%02d%02d%02d", &calendar.year, &calendar.mon, &calendar.mday,
                           &calendar.hour, &calendar.min, &calendar.sec) != 6) {
                    DWC_Printf(0x1000000, " cannot parse datetime: %s\n", value);
                    serverTime = 0;
                } else {
                    calendar.mon -= 1;
                    calendar.wday = 0;
                    calendar.yday = 0;
                    calendar.msec = 0;
                    calendar.usec = 0;
                    serverTime = OSCalendarTimeToTicks(&calendar);
                }
                DWCI_AUTH_STATE.serverTimeOffset = serverTime - OSGetTime();
                DWC_Printf(0x1000000, " (%d) datetime=%s\n", decoded, value);
            } else if (strncmp(token, "locator=", strlen("locator=")) == 0) {
                decoded = DWCI_AUTH_DECODE("locator=");
                strcpy(DWCI_AUTH_STATE.locator, value);
                DWC_Printf(0x1000000, " (%d) locator=%s\n", decoded, value);
            } else if (strncmp(token, "token=", strlen("token=")) == 0) {
                decoded = DWCI_AUTH_DECODE("token=");
                strcpy(DWCI_AUTH_STATE.token, value);
                DWC_Printf(0x1000000, " (%d) token=%s\n", decoded, value);
            } else if (strncmp(token, "challenge=", strlen("challenge=")) == 0) {
                decoded = DWCI_AUTH_DECODE("challenge=");
                strcpy(DWCI_AUTH_STATE.challenge, value);
                DWC_Printf(0x1000000, " (%d) challenge=%s\n", decoded, value);
            } else if (strncmp(token, "userid=", strlen("userid=")) == 0) {
                decoded = DWCI_AUTH_DECODE("userid=");
                sscanf(value, "%llu", &DWCI_AUTH_STATE.userId);
                DWC_Printf(0x1000000, " (%d) userid=%llu\n", decoded, DWCI_AUTH_STATE.userId);
                DWCi_authSaveData.userId = DWCI_AUTH_STATE.userId;
            } else if (strncmp(token, "svchost=", strlen("svchost=")) == 0) {
                decoded = DWCI_AUTH_DECODE("svchost=");
                strcpy(DWCi_authSvlResult.svlhost, value);
                DWC_Printf(0x1000000, " (%d) svlhost=%s\n", decoded, value);
            } else if (strncmp(token, "servicetoken=", strlen("servicetoken=")) == 0) {
                decoded = DWCI_AUTH_DECODE("servicetoken=");
                strcpy(DWCi_authSvlResult.svltoken, value);
                DWC_Printf(0x1000000, " (%d) servicetoken=%s\n", decoded, value);
            } else if (strncmp(token, "statusdata=", strlen("statusdata=")) == 0) {
                decoded = DWCI_AUTH_DECODE("statusdata=");
                if (value[0] == 'Y') {
                    DWCi_authSvlResult.status = 1;
                } else {
                    DWCi_authSvlResult.status = 0;
                }
                DWC_Printf(0x1000000, " (%d) statusdata=%s\n", decoded, value);
            } else if (strncmp(token, "prwords=", strlen("prwords=")) == 0) {
                DWCI_AUTH_DECODE("prwords=");
                strcpy(DWCi_authProfile.profileWords, value);
            } else {
                DWC_Printf(0x1000000, " unknown token : %s\n", token);
            }
        }
    }
    if (haveReturnCode) {
        code = strtol(returnCode, NULL, 10);
        if (code == 0 && DWCi_runtime->mode_0x59C8 != 3) {
            DWC_Printf(0x1000000, " cannot parse returncd(%s)\n", returnCode);
            DWCI_AUTH_STATE.status = -20101;
            return;
        }
        if (code == 1 && DWCi_runtime->mode_0x59C8 == 3) {
            DWC_Printf(0x1000000, " prof server maintenance(%s)\n", returnCode);
            DWCI_AUTH_STATE.status = -33001;
            return;
        }
        if (code >= 100) {
            if (DWCi_runtime->mode_0x59C8 == 3 && code == 100) {
                DWC_Printf(0x1000000, " prof server retruns error (%d) but ignored by DWC library\n", code);
            } else {
                DWC_Printf(0x1000000, " server retruns error (%d)\n", code);
                DWCI_AUTH_STATE.status = -20000 - code;
                return;
            }
        }
        if (code == 40) {
            DWCI_AUTH_STATE.loginKind = 2;
        } else {
            DWCI_AUTH_STATE.loginKind = 1;
        }
    } else {
        DWC_Printf(0x1000000, " no return code.\n");
        DWCI_AUTH_STATE.status = -20101;
        return;
    }
    DWCI_AUTH_STATE.status = 1;
}
