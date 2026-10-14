/* BTE/gki_buffer.h - the stack buffer header, the GKI buffer pool control block and the GKI buffer and queue entry points
 *   `BTE/gki_buffer.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GKI_BUFFER_H
#define MHTRI_BTE_GKI_BUFFER_H

#include "types.h"

/* size: 0x08 - the header of a Bluetooth stack buffer; `offset` bytes of room follow it before the payload. */
typedef struct BT_HDR {
    /* +0x00 */ u16 event;
    /* +0x02 */ u16 len;
    /* +0x04 */ u16 offset;
    /* +0x06 */ u16 layer_specific;
} BT_HDR; /* size: 0x08 */

/* size: 0x0A - a stack buffer with the first payload byte; the payload starts `offset` bytes into `data`. */
typedef struct BT_BUF {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 data[1];
} BT_BUF; /* size: 0x0A */

/* size: 0x08 - the GKI header in front of every pool buffer; the caller's pointer is the byte after it. */
typedef struct BUFFER_HDR_T {
    /* +0x00 */ struct BUFFER_HDR_T* p_next;
    /* +0x04 */ u8 q_id;
    /* +0x05 */ u8 task_id;
    /* +0x06 */ u8 status;
    /* +0x07 */ u8 Type;
} BUFFER_HDR_T; /* size: 0x08 */

/* size: 0x10 - the free list of one buffer pool. */
typedef struct FREE_QUEUE_T {
    /* +0x00 */ BUFFER_HDR_T* p_first;
    /* +0x04 */ BUFFER_HDR_T* p_last;
    /* +0x08 */ u16 size;
    /* +0x0A */ u16 total;
    /* +0x0C */ u16 cur_cnt;
    /* +0x0E */ u16 max_cnt;
} FREE_QUEUE_T; /* size: 0x10 */

/* size: 0x0C - a caller's buffer queue; `p_first`/`p_last` hold the caller's pointers (the bytes after the headers). */
typedef struct BUFFER_Q {
    /* +0x00 */ u8* p_first;
    /* +0x04 */ u8* p_last;
    /* +0x08 */ u16 count;
} BUFFER_Q; /* size: 0x0C */

#define GKI_MAX_TASKS 8
#define NUM_TASK_MBOX 4
#define GKI_NUM_TOTAL_BUF_POOLS 9

/* size: 0x28A8C - the buffer pools, the task mailboxes and the pool bookkeeping. */
typedef struct tGKI_COM_CB {
    /* +0x00000 */ u8 bufpool0[48 * (64 + 12)];
    /* +0x00E40 */ u8 bufpool1[26 * (128 + 12)];
    /* +0x01C78 */ u8 bufpool2[45 * (660 + 12)];
    /* +0x09298 */ u8 bufpool3[30 * (1800 + 12)];
    /* +0x166F0 */ u8 bufpool4[9 * (8192 + 12)];
    /* +0x2875C */ u8* OSStack[GKI_MAX_TASKS];
    /* +0x2877C */ u16 OSStackSize[GKI_MAX_TASKS];
    /* +0x2878C */ s8* OSTName[GKI_MAX_TASKS];
    /* +0x287AC */ u8 OSRdyTbl[GKI_MAX_TASKS];
    /* +0x287B4 */ u16 OSWaitEvt[GKI_MAX_TASKS];
    /* +0x287C4 */ u16 OSWaitForEvt[GKI_MAX_TASKS];
    /* +0x287D4 */ u32 OSTicks;
    /* +0x287D8 */ u32 OSIdleCnt;
    /* +0x287DC */ s16 OSDisableNesting;
    /* +0x287DE */ s16 OSLockNesting;
    /* +0x287E0 */ s16 OSIntNesting;
    /* +0x287E2 */ u8 pad_0x287E2[2];
    /* +0x287E4 */ s32 OSTicksTilExp;
    /* +0x287E8 */ s32 OSNumOrigTicks;
    /* +0x287EC */ s32 OSWaitTmr[GKI_MAX_TASKS];
    /* +0x2880C */ s32 OSTaskTmr0[GKI_MAX_TASKS];
    /* +0x2882C */ s32 OSTaskTmr0R[GKI_MAX_TASKS];
    /* +0x2884C */ s32 OSTaskTmr1[GKI_MAX_TASKS];
    /* +0x2886C */ s32 OSTaskTmr1R[GKI_MAX_TASKS];
    /* +0x2888C */ BUFFER_HDR_T* OSTaskQFirst[GKI_MAX_TASKS][NUM_TASK_MBOX];
    /* +0x2890C */ BUFFER_HDR_T* OSTaskQLast[GKI_MAX_TASKS][NUM_TASK_MBOX];
    /* +0x2898C */ FREE_QUEUE_T freeq[GKI_NUM_TOTAL_BUF_POOLS];
    /* +0x28A1C */ u8* pool_start[GKI_NUM_TOTAL_BUF_POOLS];
    /* +0x28A40 */ u8* pool_end[GKI_NUM_TOTAL_BUF_POOLS];
    /* +0x28A64 */ u16 pool_size[GKI_NUM_TOTAL_BUF_POOLS];
    /* +0x28A78 */ u8* p_user_mempool;
    /* +0x28A7C */ u16 pool_access_mask;
    /* +0x28A7E */ u8 pool_list[GKI_NUM_TOTAL_BUF_POOLS];
    /* +0x28A87 */ u8 curr_total_no_of_pools;
    /* +0x28A88 */ u8 pad_0x28A88[4];
} tGKI_COM_CB; /* size: 0x28A8C */

/* size: 0x54 - the OS glue's part of the GKI control block: the nesting depth of GKI_disable and the interrupt level
 *   each depth saved (the array length is an approximation: the 0x50 bytes up to the common part are its bound). */
typedef struct tGKI_OS {
    /* +0x00 */ u8 int_nest;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ BOOL int_level[20];
} tGKI_OS; /* size: 0x54 */

/* size: 0x28AE0 - the GKI control block: the OS glue's part, then the common part. */
typedef struct tGKI_CB {
    /* +0x00 */ tGKI_OS os;
    /* +0x54 */ tGKI_COM_CB com;
} tGKI_CB; /* size: 0x28AE0 */

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8070CDE0 - the GKI control block. */
extern tGKI_CB gki_cb;

/* 0x80477510 - clears the mailboxes and pool bookkeeping and builds the five fixed pools. */
void gki_buffer_init(void);

/* 0x804777C0 - empties a buffer queue. */
void GKI_init_q(BUFFER_Q* p_q);

/* 0x804777E0 - takes a buffer of at least `size` bytes from the stack's pools. */
BT_HDR* GKI_getbuf(u16 size);

/* 0x80477980 - takes a buffer from pool `pool_id`, or from the public pools when it is empty. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getpoolbuf(u8 pool_id);

/* 0x80477A70 - returns a buffer to its pool. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_freebuf(void* p_buf);

/* 0x80477BE0 - the usable size of a buffer, 0 for a bad one. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
u16 GKI_get_buf_size(void* p_buf);

/* 0x80477C30 - posts a buffer into mailbox `mbox` of task `task_id` and signals the task. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_send_msg(u8 task_id, u8 mbox, void* msg);

/* 0x80477DF0 - takes the first buffer from mailbox `mbox` of the calling task. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_read_mbox(u8 mbox);

/* 0x80477EA0 / 0x80477FE0 - append a buffer to a queue, or put it at the head. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_enqueue(BUFFER_Q* p_q, void* p_buf);
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void GKI_enqueue_head(BUFFER_Q* p_q, void* p_buf);

/* 0x80478120 - takes the first buffer from a queue. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_dequeue(BUFFER_Q* p_q);

/* 0x804781C0 - unlinks `p_buf` from a queue. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_remove_from_queue(BUFFER_Q* p_q, void* p_buf);

/* 0x80478290 / 0x804782A0 - the first buffer of a queue, and the buffer after `p_buf`. */
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getfirst(BUFFER_Q* p_q);
/* untyped: a caller-owned payload - a GKI buffer carries any message */
void* GKI_getnext(void* p_buf);

/* 0x804782C0 - whether a queue holds no buffer. */
u8 GKI_queue_is_empty(BUFFER_Q* p_q);

/* 0x804782D0 - creates a pool of `count` buffers of `size` bytes in `p_mem_pool` (allocated when NULL); returns its id. */
u8 GKI_create_pool(u16 size, u16 count, u8 permission, u8* p_mem_pool);

/* 0x80478620 - frees an empty pool. */
void GKI_delete_pool(u8 pool_id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GKI_BUFFER_H */
