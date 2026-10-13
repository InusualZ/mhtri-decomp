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
 * NAMES. gki_buffer_init, GKI_init_q, GKI_getbuf, GKI_getpoolbuf, GKI_freebuf, GKI_get_buf_size, GKI_send_msg,
 *   GKI_read_mbox, GKI_enqueue, GKI_enqueue_head, GKI_dequeue, GKI_remove_from_queue, GKI_getnext, GKI_queue_is_empty
 *   and GKI_create_pool are the map's; gki_init_free_queue, GKI_getfirst (returns the queue's first word) and
 *   GKI_delete_pool are the BTE source's names read from the bodies, their order and their messages; gki_cb (.bss
 *   0x8070CDE0, read by 81 sites across the band) is a GUESS from the BTE source's control block.
 * RESIDUALS. every body is unwritten: 0x804772F0-0x80478790 (18 functions).
 * SHAPES. plain C; the callees GKI_disable, GKI_enable, GKI_get_taskid, GKI_os_malloc, GKI_os_free, GKI_send_event and
 *   GKI_exception are declared in BTE/gki_time.h.
 */

#include "BTE/gki_buffer.h"
