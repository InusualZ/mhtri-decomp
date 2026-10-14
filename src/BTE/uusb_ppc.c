/*
 * BTE/uusb_ppc.c - the USB transport of the Broadcom BTE Bluetooth stack on the Wii: opens the Bluetooth USB device,
 *   runs its HCI event and ACL receive buffers, writes outgoing messages and closes and unregisters the device.
 *
 * RANGE. .text 0x804796F0-0x8047A370 (11 functions); .bss 0x80735900-0x80737960; .sdata 0x80793D28-0x80793D38;
 *   .sbss 0x80795008-0x80795020. Strong start: 0x804796F0 is the first function to read the transport control block
 *   .bss 0x80735900, which follows BTE/hcisu_h2.c's 0x807358C0 in the .bss. Strong end: UUSB_Unregister
 *   (0x8047A320) is the last reader of 0x80735900; bte_hcisu_send (0x8047A370) reads BTE/bte_hcisu.c's .sbss
 *   0x80795020. The two 0x1000 receive buffers 0x80735960 / 0x80736960 are read by 0x80479790 / 0x80479940; the
 *   .sdata flag 0x80793D28 (written here, also read by BTA_Init) and the device name strings "oh0" / "oh1"
 *   0x80793D30 / 0x80793D34 by 0x80479CB0.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's UUSB_Close and UUSB_Unregister. GUESS, from the bodies, their callers and Broadcom's UUSB API:
 *   0x804796F0 uusb_close_cback, 0x80479790 uusb_evt_read_cback, 0x80479940 uusb_acl_read_cback, 0x80479AE0
 *   uusb_cmd_write_cback, 0x80479BD0 uusb_acl_write_cback (the IOS USB completion callbacks), 0x80479CB0 UUSB_Open,
 *   0x80479EC0 UUSB_Register, 0x8047A050 UUSB_Read, 0x8047A060 UUSB_Write; data 0x80735900 uusb_cb, 0x80735960 /
 *   0x80736960 uusb_evt_stack / uusb_acl_stack (the fiber stacks a received message is posted from),
 *   0x80793D28 uusb_open_pending, 0x80793D2C uusb_fd, 0x80795008 uusb_dev_open, 0x80795009 uusb_initialised,
 *   0x8079500C uusb_bus_index, 0x80795010 uusb_use_bus_index, 0x80795014 uusb_use_custom_ids, 0x80795018 /
 *   0x8079501C uusb_custom_vid / uusb_custom_pid (the .sbss switches nothing in the DOL sets).
 * RESIDUALS. uusb_evt_read_cback: retail tests the buffer against a pointer-typed NULL (`cmplw` on a register
 *   zero, the `(BT_HDR*)NULL` spelling); retail rematerialises that zero with `li r0,0` where ours reuses the r5 zero
 *   of the evt_read_pending store (scoped opt_common_subs/opt_propagation/global_optimizer/opt_lifetimes off and a
 *   NULL local do not split it). UUSB_Open: the vendor id lands in r0 and the bus
 *   index in r3 where ours swaps them (r4 / r0). UUSB_Write: the result, the copy target and the buffer take r28 /
 *   r27 / r26 where ours takes r26 / r28 / r27 (declaration order and an if/else chain do not move it). The
 *   control block is 0x4C bytes (UUSB_Open clears that much) while the map's object is 0x60, the gap being the
 *   32-byte alignment of the receive stacks.
 * SHAPES. plain C; the receive pipes are re-armed by the static inline helpers uusb_start_evt_read and
 *   uusb_start_acl_read, which the read callbacks and UUSB_Register inline. The read callbacks test their buffers
 *   against `(BT_HDR*)NULL`: retail's unsigned `cmplw` comes from a pointer-typed null, while types.h's int NULL
 *   gives `cmpwi`.
 */

#include "BTE/uusb_ppc.h"
#include "BTE/bte_hcisu.h"
#include "BTE/gki_ppc.h"
#include "BTE/l2c_link.h"
#include "IPC/ipcclt.h"
#include "OS/OSSwitchFiberEx.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "USB/usb.h"

/* The transport states. */
#define UUSB_STATE_CLOSING 0
#define UUSB_STATE_RUNNING 2
#define UUSB_STATE_OPENED 4
#define UUSB_STATE_CLOSED 5

/* The Wii's Bluetooth module (Nintendo vendor 0x057E, product 0x0305). */
#define UUSB_WII_VID 0x057E
#define UUSB_WII_PID 0x0305

/* The endpoints of the Bluetooth module. */
#define UUSB_EP_ACL_OUT 0x02
#define UUSB_EP_ACL_IN 0x82
#define UUSB_EP_EVT_IN 0x81

/* A class request to the device, the request type of an HCI command. */
#define UUSB_HCI_CMD_REQUEST_TYPE 0x20

/* The writes a pipe keeps in flight before it queues. */
#define UUSB_MAX_PENDING_WRITES 5

#define UUSB_FIBER_STACK_SIZE 0x1000

#define GKI_RESTRICTED_POOL 1
#define GKI_INVALID_POOL 0xFF

#define BT_EVT_TO_BTU_HCI_EVT 0x1000
#define BT_EVT_TO_BTU_HCI_ACL 0x1100

/* size: 0x4C - the USB transport state: the device, its endpoints and ids, the receive pools, the callback, the
 *   state and the two write queues with their in-flight counts. */
typedef struct tUUSB_CB {
    /* +0x00 */ s32 fd;
    /* +0x04 */ u8 unused_0x04[0xC];
    /* +0x10 */ u8 acl_out_ep;
    /* +0x11 */ u8 acl_in_ep;
    /* +0x12 */ u8 evt_in_ep;
    /* +0x13 */ u8 sco_ep;
    /* +0x14 */ u32 vendor_id;
    /* +0x18 */ u32 product_id;
    /* +0x1C */ u8 evt_pool;
    /* +0x1D */ u8 acl_pool;
    /* +0x1E */ u8 pad_0x1E[2];
    /* +0x20 */ tUUSB_CBACK p_cback;
    /* +0x24 */ const tUUSB_CFG* p_cfg;
    /* +0x28 */ u8 flags;
    /* +0x29 */ u8 state;
    /* +0x2A */ u8 pad_0x2A;
    /* +0x2B */ u8 evt_read_pending;
    /* +0x2C */ BUFFER_Q acl_tx_q;
    /* +0x38 */ u8 acl_tx_pending;
    /* +0x39 */ u8 pad_0x39[3];
    /* +0x3C */ BUFFER_Q cmd_tx_q;
    /* +0x48 */ u8 cmd_tx_pending;
    /* +0x49 */ u8 pad_0x49[3];
} tUUSB_CB; /* size: 0x4C */

/* .bss 0x80735900 - the USB transport state. */
tUUSB_CB uusb_cb;

/* .bss 0x80735960 / 0x80736960 - the fiber stacks a received event or ACL message is posted to the BTU task from. */
u8 uusb_evt_stack[UUSB_FIBER_STACK_SIZE] __attribute__((aligned(32)));
u8 uusb_acl_stack[UUSB_FIBER_STACK_SIZE] __attribute__((aligned(32)));

/* .sdata 0x80793D28 - set from the start of an open until UUSB_Register has started the receive pipes. */
BOOL uusb_open_pending = TRUE;

/* .sdata 0x80793D2C - the descriptor of the open device. */
s32 uusb_fd = -1;

/* .sbss 0x80795008..0x8079501C - the transport's switches, defined in reverse because MWCC emits .sbss backwards:
 *   whether to open a device other than the Wii's module and its ids, which USB bus to open it on and whether to
 *   honour that choice, and whether the transport state and the device are set up. */
u32 uusb_custom_pid;
u32 uusb_custom_vid;
u8 uusb_use_custom_ids;
u32 uusb_use_bus_index;
u32 uusb_bus_index;
u8 uusb_initialised;
u8 uusb_dev_open;

void uusb_evt_read_cback(s32 result, BT_HDR* p_msg);
void uusb_acl_read_cback(s32 result, BT_HDR* p_msg);

/* Takes a buffer from the event pool and starts the next HCI event read into it. */
static inline void uusb_start_evt_read(void) {
    BT_HDR* p_buf;
    u8* p_data;

    while ((p_buf = (BT_HDR*)GKI_getpoolbuf(uusb_cb.evt_pool)) == NULL) {
    }

    p_buf->event = BT_EVT_TO_BTU_HCI_EVT;
    p_buf->len = 0;
    p_data = (u8*)(((u32)p_buf + sizeof(BT_HDR) + 31) & ~31);
    p_buf->offset = p_data - ((BT_BUF*)p_buf)->data;

    if (IUSB_ReadIntrMsgAsync(uusb_cb.fd, uusb_cb.evt_in_ep, GKI_get_buf_size(p_buf) - 40 - p_buf->offset, p_data,
                              (USBCallback)uusb_evt_read_cback, p_buf) != 0) {
        GKI_freebuf(p_buf);
    }
    uusb_cb.evt_read_pending = TRUE;
}

/* Takes a buffer from the ACL pool and starts the next ACL read into it. */
static inline void uusb_start_acl_read(void) {
    BT_HDR* p_buf;
    u8* p_data;

    while ((p_buf = (BT_HDR*)GKI_getpoolbuf(uusb_cb.acl_pool)) == NULL) {
    }

    p_buf->event = BT_EVT_TO_BTU_HCI_ACL;
    p_buf->len = 0;
    p_data = (u8*)(((u32)p_buf + sizeof(BT_HDR) + 31) & ~31);
    p_buf->offset = p_data - ((BT_BUF*)p_buf)->data;

    if (IUSB_ReadBlkMsgAsync(uusb_cb.fd, uusb_cb.acl_in_ep, GKI_get_buf_size(p_buf) - 40 - p_buf->offset, p_data,
                             (USBCallback)uusb_acl_read_cback, p_buf) != 0) {
        GKI_freebuf(p_buf);
    }
}

/* 0x804796F0 (0x94): Forgets the closed device, closes the IOS USB library and reports the close. */
void uusb_close_cback(s32 result, void* arg) { /* untyped: caller-owned payload - unused */
    uusb_cb.fd = 0;
    uusb_cb.acl_out_ep = 0;
    uusb_cb.acl_in_ep = 0;
    uusb_cb.evt_in_ep = 0;
    uusb_cb.sco_ep = 0;
    uusb_dev_open = FALSE;
    IUSB_CloseLib();

    GKI_disable();
    uusb_cb.state = UUSB_STATE_CLOSED;
    uusb_cb.flags = 0;
    uusb_initialised = FALSE;
    GKI_enable();

    if (uusb_cb.p_cback != NULL) {
        uusb_cb.p_cback(UUSB_EVT_CLOSED, (s8)result);
    }
}

/* 0x80479790 (0x1A8): Posts a received HCI event to the BTU task and starts the next event read. */
void uusb_evt_read_cback(s32 result, BT_HDR* p_msg) {
    BT_HDR* p_new;

    uusb_cb.evt_read_pending = FALSE;

    if (uusb_cb.state != UUSB_STATE_RUNNING) {
        GKI_freebuf(p_msg);
        GKI_delete_pool(uusb_cb.evt_pool);
        uusb_cb.evt_pool = GKI_INVALID_POOL;
        return;
    }

    if (p_msg != (BT_HDR*)NULL) {
        if (result <= 0) {
            GKI_freebuf(p_msg);
        } else {
            p_new = (BT_HDR*)GKI_getpoolbuf(2);
            if (p_new == NULL) {
                GKI_freebuf(p_msg);
            } else {
                p_msg->event = BT_EVT_TO_BTU_HCI_EVT;
                p_msg->len = result;
                memcpy(p_new, p_msg, ((p_msg->len + p_msg->offset + sizeof(BT_HDR)) & ~3) + 4);
                OSSwitchFiberEx((u32)p_new, 0, 0, 0, (void (*)(void))bte_hcisu_rcv_msg,
                                uusb_evt_stack + UUSB_FIBER_STACK_SIZE);
                GKI_freebuf(p_msg);
            }
        }
    }

    uusb_start_evt_read();
}

/* 0x80479940 (0x19C): Posts a received ACL packet to the BTU task once L2CAP has it whole, and starts the next ACL
 * read. */
void uusb_acl_read_cback(s32 result, BT_HDR* p_msg) {
    BT_HDR* p_new;

    if (uusb_cb.state != UUSB_STATE_RUNNING) {
        GKI_freebuf(p_msg);
        GKI_delete_pool(uusb_cb.acl_pool);
        uusb_cb.acl_pool = GKI_INVALID_POOL;
        return;
    }

    if (result <= 0) {
        GKI_freebuf(p_msg);
    } else {
        p_msg->len = result;
        p_new = (BT_HDR*)GKI_getpoolbuf(3);
        if (p_new == NULL) {
            GKI_freebuf(p_msg);
        } else {
            memcpy(p_new, p_msg, ((p_msg->len + p_msg->offset + sizeof(BT_HDR)) & ~3) + 4);
            p_new = l2cap_link_chk_pkt_start(p_new);
            if (p_new != (BT_HDR*)NULL && l2cap_link_chk_pkt_end()) {
                OSSwitchFiberEx((u32)p_new, 0, 0, 0, (void (*)(void))bte_hcisu_rcv_msg,
                                uusb_acl_stack + UUSB_FIBER_STACK_SIZE);
            }
            GKI_freebuf(p_msg);
        }
    }

    uusb_start_acl_read();
}

/* 0x80479AE0 (0xEC): Frees a written HCI command and sends the next queued one. */
void uusb_cmd_write_cback(s32 result, void* arg) { /* untyped: caller-owned payload - the buffer written */
    BT_HDR* p_msg;

    if (arg != NULL) {
        GKI_freebuf(arg);
        GKI_disable();
        uusb_cb.cmd_tx_pending--;
        GKI_enable();
    }

    if (uusb_cb.cmd_tx_pending < UUSB_MAX_PENDING_WRITES && uusb_cb.cmd_tx_q.count != 0) {
        p_msg = (BT_HDR*)GKI_dequeue(&uusb_cb.cmd_tx_q);
        if (IUSB_WriteCtrlMsgAsync(uusb_cb.fd, UUSB_HCI_CMD_REQUEST_TYPE, 0, 0, 0, p_msg->len,
                                   ((BT_BUF*)p_msg)->data + p_msg->offset, uusb_cmd_write_cback, p_msg) < 0) {
            GKI_enqueue_head(&uusb_cb.cmd_tx_q, p_msg);
        } else {
            GKI_disable();
            uusb_cb.cmd_tx_pending++;
            GKI_enable();
        }
    }
}

/* 0x80479BD0 (0xE0): Frees a written ACL packet and sends the next queued one. */
void uusb_acl_write_cback(s32 result, void* arg) { /* untyped: caller-owned payload - the buffer written */
    BT_HDR* p_msg;

    if (arg != NULL) {
        GKI_freebuf(arg);
        GKI_disable();
        uusb_cb.acl_tx_pending--;
        GKI_enable();
    }

    if (uusb_cb.acl_tx_pending < UUSB_MAX_PENDING_WRITES && uusb_cb.acl_tx_q.count != 0) {
        p_msg = (BT_HDR*)GKI_dequeue(&uusb_cb.acl_tx_q);
        if (IUSB_WriteBlkMsgAsync(uusb_cb.fd, uusb_cb.acl_out_ep, p_msg->len, ((BT_BUF*)p_msg)->data + p_msg->offset,
                                  uusb_acl_write_cback, p_msg) < 0) {
            GKI_enqueue_head(&uusb_cb.acl_tx_q, p_msg);
        } else {
            GKI_disable();
            uusb_cb.acl_tx_pending++;
            GKI_enable();
        }
    }
}

/* 0x80479CB0 (0x208): Opens the IOS USB library and the Bluetooth device and creates the receive pools. */
void UUSB_Open(const tUUSB_CFG* p_cfg) {
    s32 fd;
    u32 vendor_id;
    u32 product_id;

    memset(&uusb_cb, 0, sizeof(uusb_cb));

    GKI_disable();
    uusb_cb.state = UUSB_STATE_CLOSED;
    GKI_enable();

    if (!uusb_initialised) {
        uusb_cb.flags = 0;
        uusb_initialised = TRUE;
    }

    if (IPCCltInit() != 0) {
        return;
    }
    if (IUSB_OpenLib() != 0) {
        return;
    }

    uusb_cb.p_cfg = p_cfg;
    if (uusb_use_custom_ids == TRUE) {
        uusb_cb.vendor_id = uusb_custom_vid;
        uusb_cb.product_id = uusb_custom_pid;
    } else {
        uusb_cb.vendor_id = UUSB_WII_VID;
        uusb_cb.product_id = UUSB_WII_PID;
    }

    uusb_cb.acl_out_ep = 0;
    uusb_cb.acl_in_ep = 0;
    uusb_cb.evt_in_ep = 0;
    uusb_cb.sco_ep = 0;

    product_id = uusb_cb.product_id;
    vendor_id = uusb_cb.vendor_id;
    if (uusb_use_bus_index == TRUE) {
        if (uusb_bus_index == 0) {
            fd = IUSB_OpenDeviceIds("oh0", (u16)vendor_id, (u16)product_id, &uusb_cb.fd);
        } else if (uusb_bus_index == 1) {
            fd = IUSB_OpenDeviceIds("oh1", (u16)vendor_id, (u16)product_id, &uusb_cb.fd);
        }
    } else {
        fd = IUSB_OpenDeviceIds("oh1", (u16)vendor_id, (u16)product_id, &uusb_cb.fd);
    }

    if (fd >= 0) {
        uusb_fd = fd;
    }
    if (fd < 0) {
        return;
    }

    uusb_cb.acl_out_ep = UUSB_EP_ACL_OUT;
    uusb_cb.acl_in_ep = UUSB_EP_ACL_IN;
    uusb_cb.evt_in_ep = UUSB_EP_EVT_IN;
    uusb_cb.sco_ep = 0;

    GKI_init_q(&uusb_cb.acl_tx_q);
    uusb_cb.acl_tx_pending = 0;
    GKI_init_q(&uusb_cb.cmd_tx_q);
    uusb_cb.cmd_tx_pending = 0;

    uusb_cb.evt_pool = GKI_create_pool(660, 45, GKI_RESTRICTED_POOL, NULL);
    uusb_cb.acl_pool = GKI_create_pool(1800, 30, GKI_RESTRICTED_POOL, NULL);
    if (uusb_cb.evt_pool == GKI_INVALID_POOL || uusb_cb.acl_pool == GKI_INVALID_POOL) {
        return;
    }

    GKI_disable();
    uusb_cb.state = UUSB_STATE_OPENED;
    GKI_enable();
    uusb_open_pending = TRUE;
}

/* 0x80479EC0 (0x18C): Installs the event callback and starts the HCI event and ACL receive pipes. */
void UUSB_Register(const tUUSB_CFG* p_cfg, tUUSB_CBACK p_cback) {
    if (uusb_cb.state != UUSB_STATE_OPENED) {
        return;
    }
    if (uusb_cb.evt_pool == GKI_INVALID_POOL || uusb_cb.acl_pool == GKI_INVALID_POOL) {
        return;
    }

    GKI_disable();
    uusb_cb.p_cback = p_cback;
    uusb_cb.state = UUSB_STATE_RUNNING;
    GKI_enable();

    uusb_start_evt_read();
    uusb_start_acl_read();
    uusb_open_pending = FALSE;
}

/* 0x8047A050 (0x8): Reads nothing; the receive pipes post whole messages instead. */
u16 UUSB_Read(u8 port, u8* p_data, u16 len) {
    return 0;
}

/* 0x8047A060 (0x20C): Copies `len` bytes at `p_data` into a transport buffer and writes it to the command or ACL
 * pipe, or queues it while the pipe has too many writes in flight; returns the IOS USB result of the write. */
u16 UUSB_Write(u8 port, const u8* p_data, u16 len, BT_HDR* p_msg) {
    BT_HDR* p_buf;
    u8* p_dst;
    s32 result = 0;

    if (uusb_cb.state != UUSB_STATE_RUNNING) {
        return 0;
    }

    switch (port) {
    case UUSB_PORT_CMD:
        p_buf = (BT_HDR*)GKI_getpoolbuf(uusb_cb.evt_pool);
        if (p_buf == NULL) {
            return 0;
        }

        p_buf->len = len;
        p_dst = (u8*)(((u32)p_buf + sizeof(BT_HDR) + 31) & ~31);
        p_buf->offset = p_dst - ((BT_BUF*)p_buf)->data;
        memcpy(p_dst, p_data, len);

        if (uusb_cb.cmd_tx_pending < UUSB_MAX_PENDING_WRITES && uusb_cb.cmd_tx_q.count == 0) {
            result = IUSB_WriteCtrlMsgAsync(uusb_cb.fd, UUSB_HCI_CMD_REQUEST_TYPE, 0, 0, 0, len, p_dst,
                                            uusb_cmd_write_cback, p_buf);
        } else {
            GKI_enqueue(&uusb_cb.cmd_tx_q, p_buf);
            return 0;
        }

        if (result != 0) {
            GKI_freebuf(p_buf);
        } else {
            GKI_disable();
            uusb_cb.cmd_tx_pending++;
            GKI_enable();
        }
        break;

    case UUSB_PORT_ACL:
        p_buf = (BT_HDR*)GKI_getpoolbuf(uusb_cb.acl_pool);
        if (p_buf == NULL) {
            return 0;
        }

        p_buf->len = len;
        p_dst = (u8*)(((u32)p_buf + sizeof(BT_HDR) + 31) & ~31);
        p_buf->offset = p_dst - ((BT_BUF*)p_buf)->data;
        memcpy(p_dst, p_data, len);

        if (uusb_cb.acl_tx_pending < UUSB_MAX_PENDING_WRITES && uusb_cb.acl_tx_q.count == 0) {
            result = IUSB_WriteBlkMsgAsync(uusb_cb.fd, uusb_cb.acl_out_ep, len, p_dst, uusb_acl_write_cback, p_buf);
        } else {
            GKI_enqueue(&uusb_cb.acl_tx_q, p_buf);
            return 0;
        }

        if (result != 0) {
            GKI_freebuf(p_buf);
        } else {
            GKI_disable();
            uusb_cb.acl_tx_pending++;
            GKI_enable();
        }
        break;
    }

    return result;
}

/* 0x8047A270 (0xAC): Stops the pipes, frees every queued write and closes the device. */
void UUSB_Close(void) {
    GKI_disable();
    uusb_cb.state = UUSB_STATE_CLOSING;
    GKI_enable();

    if (uusb_cb.acl_tx_q.count != 0) {
        while (uusb_cb.acl_tx_q.count != 0) {
            GKI_freebuf(GKI_dequeue(&uusb_cb.acl_tx_q));
        }
    }
    if (uusb_cb.cmd_tx_q.count != 0) {
        while (uusb_cb.cmd_tx_q.count != 0) {
            GKI_freebuf(GKI_dequeue(&uusb_cb.cmd_tx_q));
        }
    }

    IUSB_CloseDeviceAsync(uusb_cb.fd, uusb_close_cback, NULL);
}

/* 0x8047A320 (0x44): Closes the IOS USB library and marks the transport closed. */
void UUSB_Unregister(void) {
    IUSB_CloseLib();

    GKI_disable();
    uusb_cb.state = UUSB_STATE_CLOSED;
    uusb_cb.flags = 0;
    uusb_initialised = FALSE;
    GKI_enable();
}
