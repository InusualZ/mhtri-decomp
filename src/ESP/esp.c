/*
 * ESP/esp.c - the SDK ES (title/ticket) proxy library over `/dev/es`.
 *
 * RANGE. `.text` 0x804AF420-0x804AFB50 (10 functions / 0x6E4 B); `.sdata` 0x80793E20-0x80793E30.
 *   - every function reads `__esFd` (.sdata 0x80793E20) and the `/dev/es` string (.sdata 0x80793E28); `ESP_InitLib`
 *     opens the device
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `ESP_DiGetTmd` is a GUESS (the DI-side twin of the TMD getter); the unit name is a GUESS (the library's first source file); `ESP_GetTmdView` is a GUESS for the map's former
 *   placeholder (ioctls 0x14 / 0x15, the sibling of `ESP_GetTicketViews`); the record types `ESTicket` / `ESTicketView`
 *   state only their sizes; `ESIoctlvFrame` is a GUESS at the shape of the wrappers' stack frame (see SHAPES).
 * RESIDUALS. code none.  The map row of the `/dev/es` literal is `lbl_80793E28` where this object emits `@134`, so
 *   one `ESP_InitLib` relocation differs by name only (bytes equal).
 * SHAPES. every wrapper takes its vector list and buffers through local pointers (`vec`, `tid`, `num` / `len`)
 *   initialised at the declaration, which is what hoists their `addi`s to the top.  The two-mode wrappers fill
 *   `vec[0]` inside each branch and test the caller's count before copying it.  `ESP_DiGetTicketView` sets the
 *   ticket length with an `if` / `else`, not `?:`, so each arm keeps its own store.
 *   Every wrapper's frame has its buffers at +0x20 / +0x40 and its vector list at +0xF0 with a 0x120 / 0x140
 *   frame: a single 32-byte-aligned local struct whose buffer area is padded to 0xD0 bytes reproduces all of it
 *   (separate aligned locals put the vectors at +0x40).  `NULL` is redefined as `((void*)0)` after the includes
 *   (`types.h` spells it `0`): the target compares every pointer against a pointer-typed zero (`li rN,0` + `cmplw`),
 *   which an integer `0` turns into `cmpwi rN,0`.
 */

#include "types.h"
#include "ESP/esp.h"
#include "IPC/ipcclt.h"

#undef NULL
#define NULL ((void*)0)

#define ES_ERR_INVALID (-1017)

/* The ioctlv numbers of `/dev/es` the proxy issues. */
#define ES_IOCTL_LAUNCH_TITLE 8
#define ES_IOCTL_GET_NUM_TICKET_VIEWS 0x12
#define ES_IOCTL_GET_TICKET_VIEWS 0x13
#define ES_IOCTL_GET_TMD_VIEW_SIZE 0x14
#define ES_IOCTL_GET_TMD_VIEW 0x15
#define ES_IOCTL_GET_CONSUMPTION 0x16
#define ES_IOCTL_DI_GET_TICKET_VIEW 0x1B
#define ES_IOCTL_GET_DATA_DIR 0x1D
#define ES_IOCTL_GET_TITLE_ID 0x20
#define ES_IOCTL_DI_GET_TMD_SIZE 0x39
#define ES_IOCTL_DI_GET_TMD 0x3A

#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))

/* The stack frame every ioctlv wrapper builds: two 32-byte-aligned in/out buffers and the vector list, the
 * buffer area padded to 0xD0 bytes. */
typedef struct ESIoctlvFrame {
    /* +0x00 */ union {
        u64 titleId;
        u32 size;
    } first; /* first buffer */
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u32 count; /* second buffer */
    /* +0x24 */ u8 pad_0x24[0xAC];
    /* +0xD0 */ IPCIOVector vec[4];
} ESIoctlvFrame ATTRIBUTE_ALIGN(32); /* size: 0x100 */

static s32 __esFd = -1;

/* Opens `/dev/es` once. */
s32 ESP_InitLib(void)
{
    s32 rc = 0;

    if (__esFd < 0) {
        __esFd = IOS_Open("/dev/es", 0);
        if (__esFd < 0) {
            rc = __esFd;
        }
    }
    return rc;
}

/* Closes `/dev/es`. */
s32 ESP_CloseLib(void)
{
    s32 rc = 0;

    if (__esFd >= 0) {
        rc = IOS_Close(__esFd);
        if (rc == 0) {
            __esFd = -1;
        }
    }
    return rc;
}

/* Reboots into a title with the given ticket view. */
s32 ESP_LaunchTitle(u64 titleId, ESTicketView* ticketView)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u64* tid = &f.first.titleId;

    if (__esFd < 0) {
        return ES_ERR_INVALID;
    }
    if ((u32)ticketView & 0x1F) {
        return ES_ERR_INVALID;
    }

    *tid = titleId;
    vec[0].base = tid;
    vec[0].length = sizeof(*tid);
    vec[1].base = ticketView;
    vec[1].length = sizeof(ESTicketView);

    rc = IOS_IoctlvReboot(__esFd, ES_IOCTL_LAUNCH_TITLE, 2, 0, vec);
    __esFd = -1;
    return rc;
}

/* Reads the ticket views of a title, or their count when `views` is NULL. */
s32 ESP_GetTicketViews(u64 titleId, ESTicketView* views, u32* count)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u64* tid = &f.first.titleId;
    u32* num = &f.count;

    if (__esFd < 0 || count == NULL) {
        return ES_ERR_INVALID;
    }
    if ((u32)views & 0x1F) {
        return ES_ERR_INVALID;
    }

    *tid = titleId;

    if (views == NULL) {
        vec[0].base = tid;
        vec[0].length = sizeof(*tid);
        vec[1].base = num;
        vec[1].length = sizeof(*num);
        rc = IOS_Ioctlv(__esFd, ES_IOCTL_GET_NUM_TICKET_VIEWS, 1, 1, vec);
        if (rc == 0) {
            *count = *num;
        }
        return rc;
    }

    if (*count == 0) {
        return ES_ERR_INVALID;
    }
    *num = *count;
    vec[0].base = tid;
    vec[0].length = sizeof(*tid);
    vec[1].base = num;
    vec[1].length = sizeof(*num);
    vec[2].base = views;
    vec[2].length = *count * sizeof(ESTicketView);
    return IOS_Ioctlv(__esFd, ES_IOCTL_GET_TICKET_VIEWS, 2, 1, vec);
}

/* Derives the ticket view of a disc ticket. */
s32 ESP_DiGetTicketView(const ESTicket* ticket, ESTicketView* view)
{
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;

    if (__esFd < 0 || view == NULL) {
        return ES_ERR_INVALID;
    }
    if (((u32)ticket & 0x1F) || ((u32)view & 0x1F)) {
        return ES_ERR_INVALID;
    }

    vec[0].base = (void*)ticket;
    if (ticket == NULL) {
        vec[0].length = 0;
    } else {
        vec[0].length = sizeof(ESTicket);
    }
    vec[1].base = view;
    vec[1].length = sizeof(ESTicketView);
    return IOS_Ioctlv(__esFd, ES_IOCTL_DI_GET_TICKET_VIEW, 1, 1, vec);
}

/* Reads the disc's title metadata, or its size when `tmd` is NULL. */
/* untyped: the title metadata blob */
s32 ESP_DiGetTmd(void* tmd, u32* size)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u32* len = &f.first.size;

    if (__esFd < 0 || size == NULL) {
        return ES_ERR_INVALID;
    }
    if ((u32)tmd & 0x1F) {
        return ES_ERR_INVALID;
    }

    if (tmd == NULL) {
        vec[0].base = len;
        vec[0].length = sizeof(*len);
        rc = IOS_Ioctlv(__esFd, ES_IOCTL_DI_GET_TMD_SIZE, 0, 1, vec);
        if (rc == 0) {
            *size = *len;
        }
        return rc;
    }

    if (*size == 0) {
        return ES_ERR_INVALID;
    }
    *len = *size;
    vec[0].base = len;
    vec[0].length = sizeof(*len);
    vec[1].base = tmd;
    vec[1].length = *size;
    return IOS_Ioctlv(__esFd, ES_IOCTL_DI_GET_TMD, 1, 1, vec);
}

/* Reads the title metadata view of a title, or its size when `tmdView` is NULL. */
/* untyped: the title metadata view blob */
s32 ESP_GetTmdView(u64 titleId, void* tmdView, u32* size)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u64* tid = &f.first.titleId;
    u32* len = &f.count;

    if (__esFd < 0 || size == NULL) {
        return ES_ERR_INVALID;
    }
    if ((u32)tmdView & 0x1F) {
        return ES_ERR_INVALID;
    }

    *tid = titleId;

    if (tmdView == NULL) {
        vec[0].base = tid;
        vec[0].length = sizeof(*tid);
        vec[1].base = len;
        vec[1].length = sizeof(*len);
        rc = IOS_Ioctlv(__esFd, ES_IOCTL_GET_TMD_VIEW_SIZE, 1, 1, vec);
        if (rc == 0) {
            *size = *len;
        }
        return rc;
    }

    if (*size == 0) {
        return ES_ERR_INVALID;
    }
    *len = *size;
    vec[0].base = tid;
    vec[0].length = sizeof(*tid);
    vec[1].base = len;
    vec[1].length = sizeof(*len);
    vec[2].base = tmdView;
    vec[2].length = *size;
    return IOS_Ioctlv(__esFd, ES_IOCTL_GET_TMD_VIEW, 2, 1, vec);
}

/* Reads the data directory path of a title. */
s32 ESP_GetDataDir(u64 titleId, char* path)
{
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u64* tid = &f.first.titleId;

    if (__esFd < 0 || path == NULL) {
        return ES_ERR_INVALID;
    }
    if ((u32)path & 0x1F) {
        return ES_ERR_INVALID;
    }

    *tid = titleId;
    vec[0].base = tid;
    vec[0].length = sizeof(*tid);
    vec[1].base = path;
    vec[1].length = 30;
    return IOS_Ioctlv(__esFd, ES_IOCTL_GET_DATA_DIR, 1, 1, vec);
}

/* Reads the id of the running title. */
s32 ESP_GetTitleId(u64* titleId)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;

    if (__esFd < 0 || titleId == NULL) {
        return ES_ERR_INVALID;
    }

    vec[0].base = &f.first.titleId;
    vec[0].length = sizeof(f.first.titleId);
    rc = IOS_Ioctlv(__esFd, ES_IOCTL_GET_TITLE_ID, 0, 1, vec);
    if (rc == 0) {
        *titleId = f.first.titleId;
    }
    return rc;
}

/* Reads the consumption records of a ticket. */
s32 ESP_GetConsumption(u64 ticketId, ESConsumption* consumptions, u32* count)
{
    s32 rc;
    ESIoctlvFrame f;
    IPCIOVector* vec = f.vec;
    u64* tid = &f.first.titleId;
    u32* num = &f.count;

    if (__esFd < 0) {
        return ES_ERR_INVALID;
    }
    if ((u32)consumptions & 0x1F) {
        return ES_ERR_INVALID;
    }

    *tid = ticketId;
    vec[0].base = tid;
    vec[0].length = sizeof(*tid);
    if (consumptions == NULL) {
        vec[1].base = NULL;
        vec[1].length = 0;
    } else {
        vec[1].base = consumptions;
        *num = *count;
        vec[1].length = *num * sizeof(ESConsumption);
    }
    vec[2].base = num;
    vec[2].length = sizeof(*num);
    rc = IOS_Ioctlv(__esFd, ES_IOCTL_GET_CONSUMPTION, 1, 2, vec);
    *count = *num;
    return rc;
}
