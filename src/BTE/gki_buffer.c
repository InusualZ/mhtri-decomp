/*
 * BTE/gki_buffer.c - the Broadcom BTE stack's GKI buffer pools and queues (free queues, get/free buffer, mailboxes,
 *   buffer queues, pool create/delete).
 *
 * RANGE. .text 0x804772F0-0x80478790 (18 functions, 0x14A0 B); .data 0x80612D18-0x80612E60 (the error messages the
 *   slice passes to GKI_exception, read by no other code); .bss 0x8070CDE0-0x807358C0 (gki_cb and the pool memory
 *   after it). Carved from the head of the old BTE band 0x804772F0-0x804A4AC0; the remainder is BTE/gki_time.c, which
 *   starts with gki_timers_init at 0x80478790.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start from 0x804772F0 up to
 *   0x8047ABC4 is 16-aligned (79 of 79; `-O4,p` default alignment), while 488 of the 651 starts from 0x8047ABC4 on are
 *   not, which is the 4-byte packing `cflags_os` gives - the alignment scan of the map's function rows.
 * NAMES. gki_init_free_queue, GKI_getfirst (returns the queue's first word) and GKI_delete_pool are the BTE source's names read from the bodies, their order and their messages; gki_cb (.bss
 *   0x8070CDE0, read by 81 sites across the band) is a GUESS from the BTE source's control block. gki_chk_buf_damage,
 *   GKI_get_buf_start, gki_add_to_pool_list, GKI_set_pool_permission and gki_remove_from_pool_list are GUESS names from
 *   the BTE source for bodies the target only carries inlined (or, for GKI_get_buf_start, only its message). The rest are the map's names.
 * RESIDUALS. GKI_getbuf: its parameter is u16 in retail (GKI_getpoolbuf passes the pool's u16 size untruncated and the
 *   size compare is cmplw), but BTE/gki_buffer.h declares u8 because WPAD/wpad.cpp's caller relies on it for its
 *   clrlwi 24; so GKI_getbuf keeps a cmpw for cmplw and GKI_getpoolbuf an extra clrlwi before its tail call.
 *   GKI_get_buf_start is dead-stripped in the target: only its "bad addr" message survives in .data, so its body and
 *   error code are the BTE reference's, unmeasured. The object carries the out-of-line copies of the helpers
 *   (gki_chk_buf_damage, GKI_get_buf_start, gki_add_to_pool_list, GKI_set_pool_permission, gki_remove_from_pool_list,
 *   0x3F8 B with alignment), which the linker is expected to dead-strip (unmeasured; flipcheck refuses on the +0x3F8
 *   .text size); the "GKI_get_buf_start:: bad addr" string retail keeps must survive that strip. String relocations
 *   name the anonymous pool where the map has lbl_80612D18/lbl_80612D30/... labels. gki_cb is one 0x28AE0 B object
 *   where the map splits the range into four labels; three of them, lbl_8070DF3E, lbl_8072703E and lbl_80731C74, are
 *   referenced by .data R_PPC_ADDR32 relocations from mh3_pad.cpp (0x8057CDF0) and PMIC/pmic.c (0x8063871C,
 *   0x80644670), likely false pointer words, and block a flip until they are suppressed or defined.
 * SHAPES. plain C; the callees GKI_disable, GKI_enable, GKI_get_taskid, GKI_os_malloc, GKI_os_free, GKI_send_event and
 *   GKI_exception are declared in BTE/gki_time.h. gki_chk_buf_damage sits before GKI_freebuf and spells the size lookup
 *   out itself (calling GKI_get_buf_size, defined after GKI_freebuf, leaves it un-inlined there); it,
 *   gki_add_to_pool_list, GKI_set_pool_permission and gki_remove_from_pool_list are inlined by `-inline auto`.
 */

#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"

#define BUFFER_HDR_SIZE 8
#define BUFFER_PADDING_SIZE 12
#define MAGIC_NO 0xDDBADDBA
#define GKI_INVALID_TASK 0xF0
#define GKI_INVALID_POOL 0xFF
#define GKI_RESTRICTED_POOL 1
#define MAX_USER_BUF_SIZE 0xFFF3

#define BUF_STATUS_FREE 0
#define BUF_STATUS_UNLINKED 1
#define BUF_STATUS_QUEUED 2

#define GKI_ERROR_BUF_CORRUPTED 0xFFFF
#define GKI_ERROR_FREEBUF_BAD_QID 0xFFFD
#define GKI_ERROR_FREEBUF_BUF_LINKED 0xFFFC
#define GKI_ERROR_SEND_MSG_BAD_DEST 0xFFFB
#define GKI_ERROR_SEND_MSG_BUF_LINKED 0xFFFA
#define GKI_ERROR_ENQUEUE_BUF_LINKED 0xFFF9
#define GKI_ERROR_DELETE_POOL_BAD_QID 0xFFF8
#define GKI_ERROR_BUF_SIZE_TOOBIG 0xFFF7
#define GKI_ERROR_BUF_SIZE_ZERO 0xFFF6

tGKI_CB gki_cb;

/* 0x804772F0 (0x220): carves `p_mem` into `total` buffers of `size` bytes and links them into the free list of pool `id`. */
static void gki_init_free_queue(u8 id, u16 size, u16 total, u8* p_mem)
{
    u16 i;
    u16 act_size;
    BUFFER_HDR_T* hdr;
    BUFFER_HDR_T* hdr1 = NULL;
    u32* magic;
    s32 tempsize;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    tempsize = (size + 3) & ~3;
    act_size = (u16)(tempsize + BUFFER_PADDING_SIZE);

    p_cb->pool_start[id] = p_mem;
    p_cb->pool_end[id] = p_mem + (act_size * total);
    p_cb->pool_size[id] = act_size;

    p_cb->freeq[id].size = (u16)tempsize;
    p_cb->freeq[id].total = total;
    p_cb->freeq[id].cur_cnt = 0;
    p_cb->freeq[id].max_cnt = 0;

    hdr = (BUFFER_HDR_T*)p_mem;
    p_cb->freeq[id].p_first = hdr;
    for (i = 0; i < total; i++) {
        hdr->task_id = GKI_INVALID_TASK;
        hdr->q_id = id;
        hdr->status = BUF_STATUS_FREE;
        magic = (u32*)((u8*)hdr + BUFFER_HDR_SIZE + tempsize);
        *magic = MAGIC_NO;
        hdr1 = hdr;
        hdr = (BUFFER_HDR_T*)((u8*)hdr + act_size);
        hdr1->p_next = hdr;
    }
    hdr1->p_next = NULL;
    p_cb->freeq[id].p_last = hdr1;
}

/* 0x80477510 (0x2AC): clears the mailboxes and the pool bookkeeping, then builds the five fixed pools. */
void gki_buffer_init(void)
{
    u8 i, tt, mb;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    for (tt = 0; tt < GKI_MAX_TASKS; tt++) {
        for (mb = 0; mb < NUM_TASK_MBOX; mb++) {
            p_cb->OSTaskQFirst[tt][mb] = NULL;
            p_cb->OSTaskQLast[tt][mb] = NULL;
        }
    }

    for (tt = 0; tt < GKI_NUM_TOTAL_BUF_POOLS; tt++) {
        p_cb->pool_start[tt] = NULL;
        p_cb->pool_end[tt] = NULL;
        p_cb->pool_size[tt] = 0;

        p_cb->freeq[tt].p_first = NULL;
        p_cb->freeq[tt].p_last = NULL;
        p_cb->freeq[tt].size = 0;
        p_cb->freeq[tt].total = 0;
        p_cb->freeq[tt].cur_cnt = 0;
        p_cb->freeq[tt].max_cnt = 0;
    }

    p_cb->pool_access_mask = 0xFFF0;

    gki_init_free_queue(0, 64, 48, p_cb->bufpool0);
    gki_init_free_queue(1, 128, 26, p_cb->bufpool1);
    gki_init_free_queue(2, 660, 45, p_cb->bufpool2);
    gki_init_free_queue(3, 1800, 30, p_cb->bufpool3);
    gki_init_free_queue(4, 8192, 9, p_cb->bufpool4);

    for (i = 0; i < 5; i++) {
        p_cb->pool_list[i] = i;
    }

    p_cb->curr_total_no_of_pools = 5;
}

/* 0x804777C0 (0x14): empties a buffer queue. */
void GKI_init_q(BUFFER_Q* p_q)
{
    p_q->p_first = p_q->p_last = NULL;
    p_q->count = 0;
}

/* 0x804777E0 (0x1A0): takes a buffer from the smallest public pool that fits `size` and has one free. */
BT_HDR* GKI_getbuf(u8 size)
{
    u8 i;
    FREE_QUEUE_T* Q;
    BUFFER_HDR_T* p_hdr;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (size == 0) {
        GKI_exception(GKI_ERROR_BUF_SIZE_ZERO, "getbuf: Size is zero");
        return NULL;
    }

    for (i = 0; i < p_cb->curr_total_no_of_pools; i++) {
        if (size <= p_cb->freeq[p_cb->pool_list[i]].size) {
            break;
        }
    }

    if (i == p_cb->curr_total_no_of_pools) {
        GKI_exception(GKI_ERROR_BUF_SIZE_TOOBIG, "getbuf: Size is too big");
        return NULL;
    }

    GKI_disable();

    for (; i < p_cb->curr_total_no_of_pools; i++) {
        if (((u16)1 << p_cb->pool_list[i]) & p_cb->pool_access_mask) {
            continue;
        }

        Q = &p_cb->freeq[p_cb->pool_list[i]];
        if (Q->cur_cnt < Q->total) {
            p_hdr = Q->p_first;
            Q->p_first = p_hdr->p_next;

            if (!Q->p_first) {
                Q->p_last = NULL;
            }

            if (++Q->cur_cnt > Q->max_cnt) {
                Q->max_cnt = Q->cur_cnt;
            }

            GKI_enable();

            p_hdr->task_id = GKI_get_taskid();
            p_hdr->status = BUF_STATUS_UNLINKED;
            p_hdr->p_next = NULL;
            p_hdr->Type = 0;

            return (BT_HDR*)((u8*)p_hdr + BUFFER_HDR_SIZE);
        }
    }

    GKI_enable();

    return NULL;
}

/* 0x80477980 (0xE8): takes a buffer from pool `pool_id`, falling back to the public pools when it is empty. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getpoolbuf(u8 pool_id)
{
    FREE_QUEUE_T* Q;
    BUFFER_HDR_T* p_hdr;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS) {
        return NULL;
    }

    GKI_disable();

    Q = &p_cb->freeq[pool_id];
    if (Q->cur_cnt < Q->total) {
        p_hdr = Q->p_first;
        Q->p_first = p_hdr->p_next;

        if (!Q->p_first) {
            Q->p_last = NULL;
        }

        if (++Q->cur_cnt > Q->max_cnt) {
            Q->max_cnt = Q->cur_cnt;
        }

        GKI_enable();

        p_hdr->task_id = GKI_get_taskid();
        p_hdr->status = BUF_STATUS_UNLINKED;
        p_hdr->p_next = NULL;
        p_hdr->Type = 0;

        return (u8*)p_hdr + BUFFER_HDR_SIZE;
    }

    GKI_enable();

    return GKI_getbuf(p_cb->freeq[pool_id].size);
}

/* Returns whether the buffer's trailing magic word sits at an odd address or is overwritten. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
u8 gki_chk_buf_damage(void* p_buf)
{
    BUFFER_HDR_T* p_hdr;
    u16 size;
    u32* magic;

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);
    if ((u32)p_hdr & 1) {
        size = 0;
    } else if (p_hdr->q_id < GKI_NUM_TOTAL_BUF_POOLS) {
        size = gki_cb.com.freeq[p_hdr->q_id].size;
    } else {
        size = 0;
    }

    magic = (u32*)((u8*)p_buf + size);

    if ((u32)magic & 1) {
        return 1;
    }

    if (*magic == MAGIC_NO) {
        return 0;
    }

    return 1;
}

/* 0x80477A70 (0x170): checks a buffer and puts it back on its pool's free list. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_freebuf(void* p_buf)
{
    FREE_QUEUE_T* Q;
    BUFFER_HDR_T* p_hdr;

    if (!p_buf || gki_chk_buf_damage(p_buf)) {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Free - Buf Corrupted");
        return;
    }

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED) {
        GKI_exception(GKI_ERROR_FREEBUF_BUF_LINKED, "Freeing Linked Buf");
        return;
    }

    if (p_hdr->q_id >= GKI_NUM_TOTAL_BUF_POOLS) {
        GKI_exception(GKI_ERROR_FREEBUF_BAD_QID, "Bad Buf QId");
        return;
    }

    GKI_disable();

    Q = &gki_cb.com.freeq[p_hdr->q_id];
    if (Q->p_last) {
        Q->p_last->p_next = p_hdr;
    } else {
        Q->p_first = p_hdr;
    }

    Q->p_last = p_hdr;
    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_FREE;
    p_hdr->task_id = GKI_INVALID_TASK;
    if (Q->cur_cnt > 0) {
        Q->cur_cnt--;
    }

    GKI_enable();
}

/* 0x80477BE0 (0x44): returns the usable size of a buffer from its pool, 0 for an odd address or a bad pool id. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
u16 GKI_get_buf_size(void* p_buf)
{
    BUFFER_HDR_T* p_hdr;

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);

    if ((u32)p_hdr & 1) {
        return 0;
    }

    if (p_hdr->q_id < GKI_NUM_TOTAL_BUF_POOLS) {
        return gki_cb.com.freeq[p_hdr->q_id].size;
    }

    return 0;
}

/* 0x80477C30 (0x1BC): queues a buffer in mailbox `mbox` of task `task_id` and sends the task the mailbox's event. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_send_msg(u8 task_id, u8 mbox, void* msg)
{
    BUFFER_HDR_T* p_hdr;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (task_id >= GKI_MAX_TASKS || mbox >= NUM_TASK_MBOX || p_cb->OSRdyTbl[task_id] == 0) {
        GKI_exception(GKI_ERROR_SEND_MSG_BAD_DEST, "Sending to unknown dest");
        GKI_freebuf(msg);
        return;
    }

    if (gki_chk_buf_damage(msg)) {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Send - Buffer corrupted");
        return;
    }

    p_hdr = (BUFFER_HDR_T*)((u8*)msg - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED) {
        GKI_exception(GKI_ERROR_SEND_MSG_BUF_LINKED, "Send - buffer linked");
        return;
    }

    GKI_disable();

    if (p_cb->OSTaskQFirst[task_id][mbox]) {
        p_cb->OSTaskQLast[task_id][mbox]->p_next = p_hdr;
    } else {
        p_cb->OSTaskQFirst[task_id][mbox] = p_hdr;
    }

    p_cb->OSTaskQLast[task_id][mbox] = p_hdr;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;
    p_hdr->task_id = task_id;

    GKI_enable();

    GKI_send_event(task_id, (u16)(1 << mbox));
}

/* 0x80477DF0 (0xB0): unlinks the first buffer of mailbox `mbox` of the calling task, NULL when it is empty. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_read_mbox(u8 mbox)
{
    u8 task_id = GKI_get_taskid();
    u8* p_buf = NULL;
    BUFFER_HDR_T* p_hdr;

    if (task_id >= GKI_MAX_TASKS || mbox >= NUM_TASK_MBOX) {
        return NULL;
    }

    GKI_disable();

    if (gki_cb.com.OSTaskQFirst[task_id][mbox]) {
        p_hdr = gki_cb.com.OSTaskQFirst[task_id][mbox];
        gki_cb.com.OSTaskQFirst[task_id][mbox] = p_hdr->p_next;

        p_hdr->p_next = NULL;
        p_hdr->status = BUF_STATUS_UNLINKED;

        p_buf = (u8*)p_hdr + BUFFER_HDR_SIZE;
    }

    GKI_enable();

    return p_buf;
}

/* 0x80477EA0 (0x13C): appends an unlinked buffer to a queue. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_enqueue(BUFFER_Q* p_q, void* p_buf)
{
    BUFFER_HDR_T* p_hdr;

    if (gki_chk_buf_damage(p_buf)) {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Enqueue - Buffer corrupted");
        return;
    }

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED) {
        GKI_exception(GKI_ERROR_ENQUEUE_BUF_LINKED, "Eneueue - buf already linked");
        return;
    }

    GKI_disable();

    if (p_q->p_first) {
        BUFFER_HDR_T* p_last_hdr = (BUFFER_HDR_T*)(p_q->p_last - BUFFER_HDR_SIZE);
        p_last_hdr->p_next = p_hdr;
    } else {
        p_q->p_first = p_buf;
    }

    p_q->p_last = p_buf;
    p_q->count++;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_QUEUED;

    GKI_enable();
}

/* 0x80477FE0 (0x140): puts an unlinked buffer at the head of a queue. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_enqueue_head(BUFFER_Q* p_q, void* p_buf)
{
    BUFFER_HDR_T* p_hdr;

    if (gki_chk_buf_damage(p_buf)) {
        GKI_exception(GKI_ERROR_BUF_CORRUPTED, "Enqueue - Buffer corrupted");
        return;
    }

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED) {
        GKI_exception(GKI_ERROR_ENQUEUE_BUF_LINKED, "Eneueue head - buf already linked");
        return;
    }

    GKI_disable();

    if (p_q->p_first) {
        p_hdr->p_next = (BUFFER_HDR_T*)(p_q->p_first - BUFFER_HDR_SIZE);
        p_q->p_first = p_buf;
    } else {
        p_q->p_first = p_buf;
        p_q->p_last = p_buf;
        p_hdr->p_next = NULL;
    }

    p_q->count++;

    p_hdr->status = BUF_STATUS_QUEUED;

    GKI_enable();
}

/* 0x80478120 (0xA0): unlinks the first buffer of a queue, NULL when the queue is missing or empty. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_dequeue(BUFFER_Q* p_q)
{
    BUFFER_HDR_T* p_hdr;

    GKI_disable();

    if (!p_q || !p_q->count) {
        GKI_enable();
        return NULL;
    }

    p_hdr = (BUFFER_HDR_T*)(p_q->p_first - BUFFER_HDR_SIZE);

    if (p_hdr->p_next) {
        p_q->p_first = (u8*)p_hdr->p_next + BUFFER_HDR_SIZE;
    } else {
        p_q->p_first = NULL;
        p_q->p_last = NULL;
    }

    p_q->count--;

    p_hdr->p_next = NULL;
    p_hdr->status = BUF_STATUS_UNLINKED;

    GKI_enable();

    return (u8*)p_hdr + BUFFER_HDR_SIZE;
}

/* 0x804781C0 (0xC4): unlinks `p_buf` from anywhere in a queue; returns it, or NULL when it is not queued there. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_remove_from_queue(BUFFER_Q* p_q, void* p_buf)
{
    BUFFER_HDR_T* p_prev;
    BUFFER_HDR_T* p_buf_hdr;

    GKI_disable();

    if (p_buf == p_q->p_first) {
        GKI_enable();
        return GKI_dequeue(p_q);
    }

    p_buf_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);
    p_prev = (BUFFER_HDR_T*)(p_q->p_first - BUFFER_HDR_SIZE);

    for (; p_prev; p_prev = p_prev->p_next) {
        if (p_prev->p_next == p_buf_hdr) {
            p_prev->p_next = p_buf_hdr->p_next;

            if (p_buf == p_q->p_last) {
                p_q->p_last = (u8*)p_prev + BUFFER_HDR_SIZE;
            }

            p_q->count--;

            p_buf_hdr->p_next = NULL;
            p_buf_hdr->status = BUF_STATUS_UNLINKED;

            GKI_enable();
            return p_buf;
        }
    }

    GKI_enable();
    return NULL;
}

/* 0x80478290 (0x8): returns the first buffer of a queue. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getfirst(BUFFER_Q* p_q)
{
    return p_q->p_first;
}

/* 0x804782A0 (0x1C): returns the buffer queued after `p_buf`, or NULL. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getnext(void* p_buf)
{
    BUFFER_HDR_T* p_hdr;

    p_hdr = (BUFFER_HDR_T*)((u8*)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->p_next) {
        return (u8*)p_hdr->p_next + BUFFER_HDR_SIZE;
    } else {
        return NULL;
    }
}

/* 0x804782C0 (0x10): returns whether a queue is empty. */
u8 GKI_queue_is_empty(BUFFER_Q* p_q)
{
    return (u8)(p_q->count == 0);
}

/* Returns the start of the buffer holding `p_user_area`, or NULL for an address outside every pool. */
u8* GKI_get_buf_start(u8* p_user_area)
{
    u16 xx, size;
    u32 yy;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++) {
        if (p_user_area > p_cb->pool_start[xx] && p_user_area < p_cb->pool_end[xx]) {
            yy = (u32)(p_user_area - p_cb->pool_start[xx]);
            size = p_cb->pool_size[xx];
            yy = (yy / size) * size;
            return p_cb->pool_start[xx] + yy + BUFFER_HDR_SIZE;
        }
    }

    GKI_exception(GKI_ERROR_BUF_SIZE_ZERO, "GKI_get_buf_start:: bad addr");
    return NULL;
}

/* Inserts pool `pool_id` into the pool list, which is kept sorted by buffer size. */
static void gki_add_to_pool_list(u8 pool_id)
{
    s32 i, j;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    for (i = 0; i < p_cb->curr_total_no_of_pools; i++) {
        if (p_cb->freeq[pool_id].size <= p_cb->freeq[p_cb->pool_list[i]].size) {
            break;
        }
    }

    for (j = p_cb->curr_total_no_of_pools; j > i; j--) {
        p_cb->pool_list[j] = p_cb->pool_list[j - 1];
    }

    p_cb->pool_list[i] = pool_id;
}

/* Marks pool `pool_id` restricted or public in the access mask; returns 0, or 0xFF for a bad pool id. */
u8 GKI_set_pool_permission(u8 pool_id, u8 permission)
{
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (pool_id < GKI_NUM_TOTAL_BUF_POOLS) {
        if (permission == GKI_RESTRICTED_POOL) {
            p_cb->pool_access_mask = (u16)(p_cb->pool_access_mask | (1 << pool_id));
        } else {
            p_cb->pool_access_mask = (u16)(p_cb->pool_access_mask & ~(1 << pool_id));
        }
        return 0;
    }

    return GKI_INVALID_POOL;
}

/* 0x804782D0 (0x348): creates a pool in the first free slot; returns its id, or 0xFF on failure. */
u8 GKI_create_pool(u16 size, u16 count, u8 permission, u8* p_mem_pool)
{
    u8 xx;
    u32 mem_needed;
    s32 tempsize;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (size > MAX_USER_BUF_SIZE) {
        return GKI_INVALID_POOL;
    }

    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++) {
        if (!p_cb->pool_start[xx]) {
            break;
        }
    }

    if (xx == GKI_NUM_TOTAL_BUF_POOLS) {
        return GKI_INVALID_POOL;
    }

    tempsize = (size + 3) & ~3;
    mem_needed = (tempsize + BUFFER_PADDING_SIZE) * count;

    if (!p_mem_pool) {
        p_mem_pool = GKI_os_malloc(mem_needed);
    }

    if (p_mem_pool) {
        gki_init_free_queue(xx, size, count, p_mem_pool);
        gki_add_to_pool_list(xx);
        GKI_set_pool_permission(xx, permission);
        p_cb->curr_total_no_of_pools++;
        return xx;
    } else {
        return GKI_INVALID_POOL;
    }
}

/* Removes pool `pool_id` from the sorted pool list. */
static void gki_remove_from_pool_list(u8 pool_id)
{
    tGKI_COM_CB* p_cb = &gki_cb.com;
    u8 i;

    for (i = 0; i < p_cb->curr_total_no_of_pools; i++) {
        if (pool_id == p_cb->pool_list[i]) {
            break;
        }
    }

    while (i < p_cb->curr_total_no_of_pools - 1) {
        p_cb->pool_list[i] = p_cb->pool_list[i + 1];
        i++;
    }
}

/* 0x80478620 (0x168): frees a pool none of whose buffers is taken, and drops it from the pool list. */
void GKI_delete_pool(u8 pool_id)
{
    FREE_QUEUE_T* Q;
    tGKI_COM_CB* p_cb = &gki_cb.com;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS || !p_cb->pool_start[pool_id]) {
        return;
    }

    GKI_disable();
    Q = &p_cb->freeq[pool_id];

    if (!Q->cur_cnt) {
        Q->size = 0;
        Q->total = 0;
        Q->cur_cnt = 0;
        Q->max_cnt = 0;
        Q->p_first = NULL;
        Q->p_last = NULL;

        GKI_os_free(p_cb->pool_start[pool_id]);

        p_cb->pool_start[pool_id] = NULL;
        p_cb->pool_end[pool_id] = NULL;
        p_cb->pool_size[pool_id] = 0;

        gki_remove_from_pool_list(pool_id);
        p_cb->curr_total_no_of_pools--;
    } else {
        GKI_exception(GKI_ERROR_DELETE_POOL_BAD_QID, "Deleting bad pool");
    }

    GKI_enable();
}
