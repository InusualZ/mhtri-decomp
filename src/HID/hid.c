/*
 * HID/hid.c - the HID library: client registration, device attach, detach and adoption, the interface table and the
 *    request transfers (with its exported thunks at the start).
 *
 * RANGE. .text 0x805272B0-0x80528890 (44 functions, 0x15E0 B); .data 0x80649348-0x806493A0; .sdata
 *    0x807944A0-0x807944A8; .sbss 0x80795A00-0x80795A08.  Cut from the old VF block between `KPR/kpr.c`
 *    (0x805272B0) and `KBD/kbd.c` (0x80528890).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `HIDRegisterClient`, `hid_set_suspend`, `hid_client_attach`, `hid_client_detach`,
 *    `hid_device_manage_change`, `hid_device_adopt_orphans`, `hid_interface_free` and `hid_interface_alloc` are
 *    the map's names; the file name `hid.c` is a GUESS; the other 36 rows are GUESSES from what they do (the
 *    thunks are `HIDInit`, `HIDEnd`, `HIDUnregisterClient`, `HIDSetReport`, `HIDSetProtocol`, `HIDSetIdle`
 *    and `HIDRead`, the USB class requests they issue; the `hid_*_done` rows are the IOS completion callbacks).
 * EVIDENCE. the seven 4-byte thunks at 0x805272B0..0x80527320 tail-call HID functions only (the keyboard unit calls
 *    them); `.data` 0x80649348 is the build string `<< RVL_SDK - HID ... >>` read through `.sdata` 0x807944A0
 *    by 0x80528510 (reached from the first thunk); `.sbss` 0x80795A00 (8 B) is read by 29 of the range's
 *    functions; the keyboard unit's state starts at 0x80795A08, first read by 0x80528890.
 * RESIDUALS. all 44 rows have a body (39 at 100 %); `hid_interface_pool_init` 64 % (the target walks the pool with
 *    update-form stores, the unrolled copy here precomputes the addresses), `hid_request_pool_init` 92 % and
 *    `hid_set_suspend` 95 % (instruction order of the argument set-up), `hid_device_manage_change` 97 % and
 *    `hid_device_build` 97 % (register numbering); flipcheck blockers: the .text differences above and trailing
 *    .data/.sbss/.sdata padding the object does not emit.
 * SHAPES. `#pragma dont_inline on` brackets the rows from `hid_set_suspend` to `hid_end` (the target calls them
 *    where the compiler would otherwise expand them) and is reset before `hid_interface_free`, which the pool
 *    initialisers do expand; the init and end callbacks are called with (result, arg) and the error mapping is the
 *    branch-free `(status == -8) ? -5 : -4`; thunks forward with `s32` parameters (no `extsh`).
 */
#include "types.h"
#include "HID/hid.h"
#include "IPC/ipcclt.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"
#include "Runtime.PPCEABI.H/memset.h"

#define HID_DEVICES 16
#define HID_INTERFACES 32
#define HID_REQUESTS 64

/* Device slot states. */
#define HID_DEVICE_FREE 0
#define HID_DEVICE_PRESENT 1
#define HID_DEVICE_STALE 2

struct HIDInterface;
struct HIDDevice;

typedef struct HIDRequest HIDRequest;

/* size: 0x8 - the setup packet of a control request. */
typedef struct HIDControlSetup {
    /* +0x00 */ u8 requestType;
    /* +0x01 */ u8 request;
    /* +0x02 */ u16 value;
    /* +0x04 */ u16 index;
    /* +0x06 */ u16 length;
} HIDControlSetup; /* size: 0x8 */

/* size: 0x8 - the endpoint and length of an interrupt transfer. */
typedef struct HIDTransferSetup {
    /* +0x00 */ u32 endpoint;
    /* +0x04 */ u32 length;
} HIDTransferSetup; /* size: 0x8 */

/* size: 0x8 - the part of a request that depends on its kind. */
typedef union HIDRequestSetup {
    /* +0x00 */ HIDControlSetup control;
    /* +0x00 */ HIDTransferSetup transfer;
} HIDRequestSetup; /* size: 0x8 */

/* size: 0x80 - one request in flight: the library's own words, then the 0x20-byte IOS message. */
struct HIDRequest {
    /* +0x00 */ HIDDeviceInfo* info;
    /* +0x04 */ HIDResultCallback done;
    /* +0x08 */ u32 arg;
    /* +0x0C */ u8 generation;
    /* +0x0D */ u8 pad_0x0D[0x3];
    /* +0x10 */ u32 deviceId;
    /* +0x14 */ HIDRequestSetup u;
    /* +0x1C */ void* data;
    /* +0x20 */ u8 pad_0x20[0x60];
}; /* size: 0x80 */

/* size: 0x20 - the USB device descriptor the change buffer carries (the tail is the first configuration header). */
typedef struct HIDDeviceDescriptor {
    /* +0x00 */ u8 length;
    /* +0x01 */ u8 type;
    /* +0x02 */ u16 usbVersion;
    /* +0x04 */ u8 deviceClass;
    /* +0x05 */ u8 deviceSubClass;
    /* +0x06 */ u8 deviceProtocol;
    /* +0x07 */ u8 maxPacketSize;
    /* +0x08 */ u16 vendorId;
    /* +0x0A */ u16 productId;
    /* +0x0C */ u8 pad_0x0C[0x8];
    /* +0x14 */ u8 config[0xC];
} HIDDeviceDescriptor; /* size: 0x20 */

/* size: 0x28 - the head of one device entry of the change buffer; the interface descriptors follow. */
typedef struct HIDChangeEntry {
    /* +0x00 */ s32 length;
    /* +0x04 */ s32 deviceIndex;
    /* +0x08 */ HIDDeviceDescriptor descriptor;
} HIDChangeEntry; /* size: 0x28 */

/* size: 0x34 - one attached device as the change buffer described it. */
typedef struct HIDDevice {
    /* +0x00 */ s32 state;
    /* +0x04 */ s32 users;
    /* +0x08 */ u8 pad_0x08[0x8];
    /* +0x10 */ HIDDeviceDescriptor descriptor;
    /* +0x30 */ struct HIDInterface* interfaces;
} HIDDevice; /* size: 0x34 */

/* size: 0x8 - the completion callback of releasing an interface and its argument. */
typedef struct HIDReleaseRecord {
    /* +0x00 */ HIDDoneCallback done;
    /* +0x04 */ u32 arg;
} HIDReleaseRecord; /* size: 0x8 */

/* size: 0x64 - one interface of a device: its owner client and the info the client sees. */
typedef struct HIDInterface {
    /* +0x00 */ struct HIDInterface* next;
    /* +0x04 */ HIDClient* client;
    /* +0x08 */ HIDDevice* device;
    /* +0x0C */ u32 deviceId;
    /* +0x10 */ u8 suspended;
    /* +0x11 */ u8 pad_0x11[0x3];
    /* +0x14 */ HIDReleaseRecord release;
    /* +0x1C */ u32 releaseId;
    /* +0x20 */ u8 releaseInEndpoint;
    /* +0x21 */ u8 releaseOutEndpoint;
    /* +0x22 */ u8 pad_0x22[0x2];
    /* +0x24 */ HIDDeviceInfo info;
    /* +0x44 */ u8 interfaceBlock[0xC];
    /* +0x50 */ u32 endpointCount;
    /* +0x54 */ u8 inEndpoint[8];
    /* +0x5C */ u8 outEndpoint[8];
} HIDInterface; /* size: 0x64 */

/* size: 0x3AD8 - the library's working memory, handed in by the application. */
typedef struct HIDState {
    /* +0x0000 */ u8 changeBuffer[0x600];
    /* +0x0600 */ HIDRequest requests[HID_REQUESTS];
    /* +0x2600 */ HIDDevice devices[HID_DEVICES];
    /* +0x2940 */ u8 pad_0x2940[0x500];
    /* +0x2E40 */ HIDInterface interfaces[HID_INTERFACES];
    /* +0x3AC0 */ s32 fd;
    /* +0x3AC4 */ HIDClient* clients;
    /* +0x3AC8 */ HIDInterface* freeInterfaces;
    /* +0x3ACC */ HIDRequest* freeRequests;
    /* +0x3AD0 */ HIDDoneCallback initDone;
    /* +0x3AD4 */ HIDDoneCallback endDone;
} HIDState; /* size: 0x3AD8 */

const char* __HIDVersion = "<< RVL_SDK - HID \trelease build: Feb 27 2009 10:06:03 (0x4302_145) >>";

HIDState* hid_state;

s32 hid_poll_device_changes(void);
s32 hid_device_change_done(s32 result, u32 arg);
s32 hid_ignore_result(s32 result, u32 arg);
s32 hid_set_suspend(HIDInterface* iface, s32 suspend);
s32 hid_set_report_done(s32 result, HIDRequest* request);
s32 hid_class_request_done(s32 result, HIDRequest* request);
s32 hid_read_done(s32 result, HIDRequest* request);
s32 hid_interface_release_done(s32 result, HIDReleaseRecord* record);
void hid_interface_release(HIDInterface* iface, HIDDoneCallback done, u32 arg);
void hid_release_client_interfaces(HIDClient* client, HIDDoneCallback done, u32 arg);
void hid_client_attach(HIDInterface* iface);
void hid_client_detach(HIDInterface* iface);
void hid_clear_devices(void);
void hid_drop_stale_devices(void);
void hid_device_build(HIDDevice* device, HIDChangeEntry* entry, u32 length);
void hid_device_manage_change(HIDChangeEntry* entry);
void hid_device_adopt_orphans(HIDClient* client);
void hid_interface_free(HIDInterface* iface);
void hid_interface_pool_init(void);
HIDInterface* hid_interface_alloc(void);
s32 hid_init_ioctl_done(s32 result, u32 arg);
s32 hid_open_done(s32 fd, u32 arg);
s32 hid_close_done(s32 result, u32 arg);
s32 hid_end_ioctl_done(s32 result, u32 arg);
void hid_request_free(HIDRequest* request);
HIDRequest* hid_request_alloc(void);
void hid_request_pool_init(void);

/* Exported entry points: thin forwards to the implementation rows below. */
/* untyped: caller-provided working memory */
s32 hid_init(void* memory, HIDDoneCallback done, u32 arg);
s32 hid_end(HIDDoneCallback done, u32 arg);
s32 hid_add_client(HIDClient* client);
s32 hid_remove_client(HIDClient* client, HIDDoneCallback done, u32 arg);
/* untyped: report payload */
s32 hid_set_report(HIDDeviceInfo* info, u8 type, u8 reportId, void* buffer, s32 length, HIDResultCallback done,
                   u32 arg);
s32 hid_set_protocol(HIDDeviceInfo* info, s32 protocol, HIDResultCallback done, u32 arg);
s32 hid_set_idle(HIDDeviceInfo* info, u8 reportId, u8 duration, HIDResultCallback done, u32 arg);
/* untyped: report payload */
s32 hid_read(HIDDeviceInfo* info, void* buffer, s32 length, HIDResultCallback done, u32 arg);

/* untyped: caller-provided working memory */
s32 HIDInit(void* memory, HIDDoneCallback done, u32 arg)
{
    return hid_init(memory, done, arg);
}

s32 HIDEnd(HIDDoneCallback done, u32 arg)
{
    return hid_end(done, arg);
}

s32 HIDRegisterClient(HIDClient* client, HIDAttachCallback attach)
{
    client->attach = attach;
    return hid_add_client(client);
}

s32 HIDUnregisterClient(HIDClient* client, HIDDoneCallback done, u32 arg)
{
    return hid_remove_client(client, done, arg);
}

/* untyped: report payload */
s32 HIDSetReport(HIDDeviceInfo* info, u8 type, u8 reportId, void* buffer, s32 length, HIDResultCallback done,
                 u32 arg)
{
    return hid_set_report(info, type, reportId, buffer, length, done, arg);
}

s32 HIDSetProtocol(HIDDeviceInfo* info, s32 protocol, HIDResultCallback done, u32 arg)
{
    return hid_set_protocol(info, protocol, done, arg);
}

s32 HIDSetIdle(HIDDeviceInfo* info, u8 reportId, u8 duration, HIDResultCallback done, u32 arg)
{
    return hid_set_idle(info, reportId, duration, done, arg);
}

/* untyped: report payload */
s32 HIDRead(HIDDeviceInfo* info, void* buffer, s32 length, HIDResultCallback done, u32 arg)
{
    return hid_read(info, buffer, length, done, arg);
}

/* Queues the next device-change query on the HID device. */
s32 hid_poll_device_changes(void)
{
    return IOS_IoctlAsync(hid_state->fd, 0, NULL, 0, hid_state->changeBuffer, 0x600, hid_device_change_done, NULL);
}

/* Completion of a device-change query: applies the changes and asks again. */
s32 hid_device_change_done(s32 result, u32 arg)
{
    hid_device_manage_change((HIDChangeEntry*)hid_state->changeBuffer);
    if (result == 0) {
        return IOS_IoctlAsync(hid_state->fd, 0, NULL, 0, hid_state->changeBuffer, 0x600, hid_device_change_done,
                              NULL);
    }
    return 0;
}

/* Completion callback of requests whose result nobody looks at. */
s32 hid_ignore_result(s32 result, u32 arg)
{
    return 0;
}

#pragma dont_inline on
/* Raises or lowers the device's use count and tells IOS when the first user comes or the last one goes. */
s32 hid_set_suspend(HIDInterface* iface, s32 suspend)
{
    HIDDevice* device = iface->device;

    if (suspend != 0) {
        if (++device->users == 1) {
            iface->suspended = 1;
            IOS_IoctlAsync(hid_state->fd, 1, &iface->deviceId, 8, NULL, 0, hid_ignore_result, NULL);
        }
    } else {
        if (--device->users == 0) {
            iface->suspended = 0;
            IOS_IoctlAsync(hid_state->fd, 1, &iface->deviceId, 8, NULL, 0, hid_ignore_result, NULL);
        }
    }
    return 0;
}

/* Completion of SET_REPORT: reports the outcome to the caller unless the interface was re-attached meanwhile. */
s32 hid_set_report_done(s32 result, HIDRequest* request)
{
    HIDDeviceInfo* info = request->info;
    s32 status;
    s32 length;

    if (request->generation == info->generation) {
        if (result >= 0) {
            length = result - 8;
            status = 0;
        } else {
            status = -4;
            length = 0;
        }
        if (request->done != NULL) {
            request->done(info, status, request->data, length, request->arg);
        }
    }
    hid_request_free(request);
    return 0;
}

/* Starts a SET_REPORT class request on the interface. */
/* untyped: report payload */
s32 hid_set_report(HIDDeviceInfo* info, u8 type, u8 reportId, void* buffer, s32 length, HIDResultCallback done,
                   u32 arg)
{
    HIDRequest* request = hid_request_alloc();
    s32 status;

    if (request != NULL) {
        request->info = info;
        request->done = done;
        request->arg = arg;
        request->generation = info->generation;
        request->deviceId = info->deviceId;
        request->u.control.requestType = 0x21;
        request->u.control.request = 9;
        request->u.control.value = (type << 8) | reportId;
        request->u.control.index = 0;
        request->u.control.length = length;
        request->data = buffer;
        DCFlushRange(buffer, length);
        status = IOS_IoctlAsync(hid_state->fd, 2, request, 0x20, NULL, 0, hid_set_report_done, request);
        switch (status) {
        case 0:
            return 0;
        case -8:
            return -5;
        default:
            return -4;
        }
    }
    return -5;
}

/* Completion of the parameterless class requests (SET_PROTOCOL, SET_IDLE). */
s32 hid_class_request_done(s32 result, HIDRequest* request)
{
    HIDDeviceInfo* info = request->info;
    s32 status;
    s32 length;

    if (request->generation == info->generation) {
        if (result == 8) {
            length = result - 8;
            status = 0;
        } else {
            status = -4;
            length = 0;
        }
        if (request->done != NULL) {
            request->done(info, status, request->data, length, request->arg);
        }
    }
    hid_request_free(request);
    return 0;
}

/* Starts a SET_PROTOCOL class request on the interface. */
s32 hid_set_protocol(HIDDeviceInfo* info, s32 protocol, HIDResultCallback done, u32 arg)
{
    HIDRequest* request = hid_request_alloc();
    s32 status;

    if (request != NULL) {
        request->info = info;
        request->done = done;
        request->arg = arg;
        request->generation = info->generation;
        request->deviceId = info->deviceId;
        request->u.control.requestType = 0x21;
        request->u.control.request = 0xB;
        request->u.control.value = protocol;
        request->u.control.index = info->interfaceNumber;
        request->u.control.length = 0;
        request->data = info;
        status = IOS_IoctlAsync(hid_state->fd, 2, request, 0x20, NULL, 0, hid_class_request_done, request);
        switch (status) {
        case 0:
            return 0;
        case -8:
            return -5;
        default:
            return -4;
        }
    }
    return -5;
}

/* Starts a SET_IDLE class request on the interface. */
s32 hid_set_idle(HIDDeviceInfo* info, u8 reportId, u8 duration, HIDResultCallback done, u32 arg)
{
    HIDRequest* request = hid_request_alloc();
    s32 status;

    if (request != NULL) {
        request->info = info;
        request->done = done;
        request->arg = arg;
        request->generation = info->generation;
        request->deviceId = info->deviceId;
        request->u.control.requestType = 0x21;
        request->u.control.request = 0xA;
        request->u.control.value = (duration << 8) | reportId;
        request->u.control.index = info->interfaceNumber;
        request->u.control.length = 0;
        request->data = info;
        status = IOS_IoctlAsync(hid_state->fd, 2, request, 0x20, NULL, 0, hid_class_request_done, request);
        switch (status) {
        case 0:
            return 0;
        case -8:
            return -5;
        default:
            return -4;
        }
    }
    return -5;
}

/* Completion of an interrupt-endpoint read. */
s32 hid_read_done(s32 result, HIDRequest* request)
{
    HIDDeviceInfo* info = request->info;
    s32 status;
    s32 length;

    if (request->generation == info->generation) {
        if (result >= 0) {
            length = result;
            status = 0;
        } else if (result == -0x1B6E) {
            status = -6;
            length = 0;
        } else {
            status = -4;
            length = 0;
        }
        if (request->done != NULL) {
            request->done(info, status, request->data, length, request->arg);
        }
    }
    hid_request_free(request);
    return 0;
}

/* Starts a read of `length` bytes from the interface's interrupt endpoint. */
/* untyped: report payload */
s32 hid_read(HIDDeviceInfo* info, void* buffer, s32 length, HIDResultCallback done, u32 arg)
{
    HIDRequest* request = hid_request_alloc();
    s32 status;

    if (request != NULL) {
        request->info = info;
        request->done = done;
        request->arg = arg;
        request->generation = info->generation;
        request->deviceId = info->deviceId;
        request->u.transfer.endpoint = info->inEndpoint;
        request->u.transfer.length = length;
        request->data = buffer;
        DCInvalidateRange(buffer, length);
        status = IOS_IoctlAsync(hid_state->fd, 3, request, 0x20, NULL, 0, hid_read_done, request);
        switch (status) {
        case 0:
            return 0;
        case -8:
            return -5;
        default:
            return -4;
        }
    }
    return -5;
}

/* Completion of closing an interface: forwards to the caller's callback. */
s32 hid_interface_release_done(s32 result, HIDReleaseRecord* record)
{
    if (record->done != NULL) {
        record->done(result, record->arg);
    }
    return 0;
}

/* Tells IOS the interface is no longer used and remembers the caller's completion callback. */
void hid_interface_release(HIDInterface* iface, HIDDoneCallback done, u32 arg)
{
    iface->release.done = done;
    iface->release.arg = arg;
    iface->releaseId = iface->info.deviceId;
    iface->releaseInEndpoint = iface->info.inEndpoint;
    iface->releaseOutEndpoint = iface->info.outEndpoint;
    IOS_IoctlAsync(hid_state->fd, 8, &iface->releaseId, 8, NULL, 0, hid_interface_release_done,
                   &iface->release);
}

/* Forgets every registered client. */
void hid_clear_clients(void)
{
    hid_state->clients = NULL;
}

/* Links a client in front of the list and offers it the interfaces nobody owns yet. */
s32 hid_add_client(HIDClient* client)
{
    client->next = hid_state->clients;
    hid_state->clients = client;
    hid_device_adopt_orphans(client);
    return 0;
}

/* Detaches every interface a client owns and runs `done` when it owned none. */
void hid_release_client_interfaces(HIDClient* client, HIDDoneCallback done, u32 arg)
{
    int i;
    s32 released = 0;
    HIDDevice* device = hid_state->devices;
    BOOL enabled = OSDisableInterrupts();
    HIDInterface* iface;

    for (i = 0; i < HID_DEVICES; i++, device++) {
        if (device->state == HID_DEVICE_PRESENT) {
            for (iface = device->interfaces; iface != NULL; iface = iface->next) {
                if (iface->client == client) {
                    released++;
                    if (client->attach != NULL) {
                        u8 generation = iface->info.generation++;

                        client->attach(client, &iface->info, 0);
                        hid_interface_release(iface, done, arg);
                        hid_set_suspend(iface, 0);
                        iface->client = NULL;
                    }
                }
            }
        }
    }
    OSRestoreInterrupts(enabled);
    if (released == 0 && done != NULL) {
        done(0, arg);
    }
}

/* Unlinks a client and releases its interfaces. */
s32 hid_remove_client(HIDClient* client, HIDDoneCallback done, u32 arg)
{
    HIDClient* walk = hid_state->clients;

    if (walk == client) {
        hid_state->clients = client->next;
    } else {
        while (walk != NULL) {
            HIDClient* next = walk->next;

            if (next == client) {
                walk->next = client->next;
            } else {
                walk = next;
                continue;
            }
            break;
        }
    }
    hid_release_client_interfaces(client, done, arg);
    return 0;
}

/* Offers a device's interfaces to the registered clients, in registration order. */
void hid_client_attach(HIDInterface* iface)
{
    HIDClient* client;

    while (iface != NULL) {
        client = hid_state->clients;
        iface->info.generation++;
        while (client != NULL) {
            if (client->attach != NULL && client->attach(client, &iface->info, 1) != 0) {
                hid_set_suspend(iface, 1);
                iface->client = client;
                break;
            }
            client = client->next;
        }
        iface = iface->next;
    }
}

/* Tells the owning clients their interfaces have gone. */
void hid_client_detach(HIDInterface* iface)
{
    while (iface != NULL) {
        if (iface->client != NULL) {
            hid_set_suspend(iface, 0);
            if (iface->client->attach != NULL) {
                iface->info.generation++;
                iface->client->attach(iface->client, &iface->info, 0);
            }
        }
        iface = iface->next;
    }
}

/* Marks every device slot free. */
void hid_clear_devices(void)
{
    int i;
    HIDDevice* device = hid_state->devices;

    for (i = 0; i < HID_DEVICES; i++) {
        device[i].state = HID_DEVICE_FREE;
    }
}

/* Detaches and frees every device the last change message no longer listed. */
void hid_drop_stale_devices(void)
{
    HIDDevice* device = hid_state->devices;
    int i;
    HIDInterface* iface;
    HIDInterface* next;

    for (i = 0; i < HID_DEVICES; i++, device++) {
        if (device->state == HID_DEVICE_STALE) {
            iface = device->interfaces;
            device->state = HID_DEVICE_FREE;
            hid_client_detach(device->interfaces);
            while (iface != NULL) {
                u8 generation = iface->info.generation;

                next = iface->next;
                iface->info.generation = generation + 1;
                hid_interface_free(iface);
                iface = next;
            }
        }
    }
}

/* Builds the interface records of a newly seen device from its change message and offers them to the clients. */
void hid_device_build(HIDDevice* device, HIDChangeEntry* entry, u32 length)
{
    u8* cursor = (u8*)entry + sizeof(HIDChangeEntry);
    HIDInterface* iface;
    u8* endpoints;
    s32 remaining;

    device->users = 0;
    memcpy(&device->descriptor, &entry->descriptor, 0x20);
    device->interfaces = NULL;
    remaining = length - 0x28;
    while (remaining != 0) {
        iface = hid_interface_alloc();
        iface->next = device->interfaces;
        device->interfaces = iface;
        memcpy(iface->interfaceBlock, cursor, 0xC);
        iface->client = NULL;
        endpoints = cursor + 0xC;
        iface->device = device;
        iface->deviceId = entry->deviceIndex;
        iface->info.deviceId = entry->deviceIndex;
        iface->info.vendorId = device->descriptor.vendorId;
        iface->info.productId = device->descriptor.productId;
        iface->info.interfaceNumber = cursor[2];
        iface->info.generation++;
        iface->info.deviceDescriptor = (u8*)&device->descriptor;
        iface->info.configDescriptor = device->descriptor.config;
        iface->info.interfaceDescriptor = iface->interfaceBlock;
        iface->endpointCount = cursor[4];
        memcpy(iface->inEndpoint, endpoints, 8);
        memcpy(iface->outEndpoint, endpoints, 8);
        iface->info.inEndpoint = iface->inEndpoint[2];
        cursor += 0x14;
        remaining -= 0x14;
        iface->info.inEndpointDescriptor = iface->inEndpoint;
        iface->info.outEndpoint = iface->inEndpoint[2];
        iface->info.outEndpointDescriptor = iface->inEndpoint;
        if (iface->interfaceBlock[4] == 2) {
            iface->info.outEndpoint = iface->outEndpoint[2];
            cursor += 8;
            iface->info.outEndpointDescriptor = iface->outEndpoint;
            if (endpoints[10] & 0x80) {
                memcpy(iface->outEndpoint, endpoints + 8, 8);
                iface->info.inEndpoint = iface->inEndpoint[2];
                iface->info.inEndpointDescriptor = iface->inEndpoint;
            } else {
                memcpy(iface->outEndpoint, endpoints + 8, 8);
                iface->info.outEndpoint = iface->outEndpoint[2];
                iface->info.outEndpointDescriptor = iface->outEndpoint;
            }
            remaining -= 8;
        } else {
            iface->info.outEndpoint = 0xFF;
            iface->info.outEndpointDescriptor = NULL;
        }
    }
    hid_client_attach(device->interfaces);
}

/* Applies a device-change message: marks the known devices stale, then builds or revives each listed one. */
void hid_device_manage_change(HIDChangeEntry* entry)
{
    HIDDevice* devices = hid_state->devices;
    s32 length = entry->length;
    HIDDevice* device;
    int i;

    for (i = 0; i < HID_DEVICES; i++) {
        if (devices[i].state == HID_DEVICE_PRESENT) {
            devices[i].state = HID_DEVICE_STALE;
        }
    }
    while (length != -1) {
        device = &devices[entry->deviceIndex];
        switch (device->state) {
        case HID_DEVICE_FREE:
            hid_device_build(device, entry, entry->length);
            device->state = HID_DEVICE_PRESENT;
            break;
        case HID_DEVICE_STALE:
            device->state = HID_DEVICE_PRESENT;
            break;
        }
        entry = (HIDChangeEntry*)((u8*)entry + length);
        length = entry->length;
    }
    hid_drop_stale_devices();
}

/* Offers the interfaces nobody owns to a newly registered client. */
void hid_device_adopt_orphans(HIDClient* client)
{
    int i;
    HIDDevice* device = hid_state->devices;
    BOOL enabled = OSDisableInterrupts();
    HIDInterface* iface;

    for (i = 0; i < HID_DEVICES; i++, device++) {
        if (device->state == HID_DEVICE_PRESENT) {
            for (iface = device->interfaces; iface != NULL; iface = iface->next) {
                if (iface->client == NULL) {
                    if (client->attach != NULL && client->attach(client, &iface->info, 1) != 0) {
                        hid_set_suspend(iface, 1);
                        iface->client = client;
                    }
                }
            }
        }
    }
    OSRestoreInterrupts(enabled);
}

#pragma dont_inline reset

/* Returns an interface record to the free list. */
void hid_interface_free(HIDInterface* iface)
{
    iface->next = hid_state->freeInterfaces;
    hid_state->freeInterfaces = iface;
}

/* Builds the free list of interface records. */
void hid_interface_pool_init(void)
{
    int i;
    HIDInterface* iface = hid_state->interfaces;

    hid_state->freeInterfaces = NULL;
    for (i = 0; i < HID_INTERFACES; i++) {
        HIDInterface* next = iface;

        iface++;
        hid_interface_free(next);
    }
}

/* Takes an interface record off the free list, or returns null. */
HIDInterface* hid_interface_alloc(void)
{
    HIDInterface* iface = hid_state->freeInterfaces;

    if (iface != NULL) {
        hid_state->freeInterfaces = iface->next;
        iface->next = NULL;
    }
    return iface;
}

#pragma dont_inline on
/* Completion of the version query: checks the protocol version and starts the device-change polling. */
s32 hid_init_ioctl_done(s32 result, u32 arg)
{
    if ((result & 0xFFFF0000) != 0x40000) {
        hid_state->initDone(-3, arg);
    } else if ((result & 0xFFFF) < 1) {
        hid_state->initDone(-3, arg);
    } else {
        hid_request_pool_init();
        hid_interface_pool_init();
        hid_clear_devices();
        hid_clear_clients();
        if (hid_poll_device_changes() != 0) {
            hid_state->initDone(-4, arg);
        } else {
            hid_state->initDone(0, arg);
        }
    }
    return 0;
}

/* Completion of opening the HID device: asks IOS for the protocol version. */
s32 hid_open_done(s32 fd, u32 arg)
{
    if (fd >= 0) {
        hid_state->fd = fd;
        if (IOS_IoctlAsync(hid_state->fd, 6, NULL, 0, hid_state, 0x20, hid_init_ioctl_done, (void*)arg) != 0) {
            hid_state->initDone(-4, arg);
        }
    } else {
        hid_state->initDone(-3, arg);
    }
    return 0;
}

/* Brings the library up in `memory` and opens the HID device; `done` receives the outcome. */
/* untyped: caller-provided working memory */
s32 hid_init(void* memory, HIDDoneCallback done, u32 arg)
{
    s32 status;

    if (hid_state != NULL) {
        return -2;
    }
    OSRegisterVersion(__HIDVersion);
    hid_state = memory;
    memset(memory, 0, sizeof(HIDState));
    hid_state->initDone = done;
    status = IOS_OpenAsync("/dev/usb/hid", 0, hid_open_done, (void*)arg);
    if (status == 0) {
        return 0;
    }
    return (status == -8) ? -5 : -4;
}

/* Completion of closing the HID device: reports the outcome and drops the state. */
s32 hid_close_done(s32 result, u32 arg)
{
    if (result == 0) {
        hid_state->endDone(0, arg);
    } else {
        hid_state->endDone(-4, arg);
    }
    hid_state = NULL;
    return 0;
}

/* Completion of the shutdown ioctl: closes the HID device. */
s32 hid_end_ioctl_done(s32 result, u32 arg)
{
    s32 status;

    if (result == 0) {
        status = IOS_CloseAsync(hid_state->fd, hid_close_done, (void*)arg);
    } else {
        status = hid_state->endDone(-4, arg);
    }
    if (status != 0) {
        hid_state->endDone(-5, arg);
    }
    return 0;
}

/* Unregisters every client and tells IOS to shut the HID session down. */
s32 hid_end(HIDDoneCallback done, u32 arg)
{
    s32 status;

    if (hid_state == NULL) {
        return -1;
    }
    while (hid_state->clients != NULL) {
        hid_remove_client(hid_state->clients, NULL, 0);
    }
    hid_state->endDone = done;
    status = IOS_IoctlAsync(hid_state->fd, 7, NULL, 0, NULL, 0, hid_end_ioctl_done, (void*)arg);
    if (status == 0) {
        return 0;
    }
    return (status == -8) ? -5 : -4;
}

#pragma dont_inline reset

/* Returns a request record to the free list. */
void hid_request_free(HIDRequest* request)
{
    BOOL enabled = OSDisableInterrupts();

    request->info = (HIDDeviceInfo*)hid_state->freeRequests;
    hid_state->freeRequests = request;
    OSRestoreInterrupts(enabled);
}

/* Takes a request record off the free list, or returns null. */
HIDRequest* hid_request_alloc(void)
{
    BOOL enabled = OSDisableInterrupts();
    HIDRequest* request = hid_state->freeRequests;

    if (request != NULL) {
        hid_state->freeRequests = (HIDRequest*)request->info;
    }
    OSRestoreInterrupts(enabled);
    return request;
}

/* Builds the free list of request records. */
void hid_request_pool_init(void)
{
    int i;
    HIDRequest* request = hid_state->requests;

    for (i = 0; i < HID_REQUESTS; i++) {
        HIDRequest* next = request;

        request++;
        hid_request_free(next);
    }
}
