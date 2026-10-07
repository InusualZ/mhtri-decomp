/*
 * MSL_C/alloc.cpp - the MSL block allocator: sub-block link and merge helpers, the fixed-pool free path and the
 *    free/clear entry.
 *
 * RANGE. .text 0x80458C94..0x804591A8 (4 functions in the map, 0x514 B); .rodata 0x80572528..0x80572540; .bss
 *    0x806F4CC8..0x806F4D00; .sbss 0x80794E00..0x80794E08.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name kept from the registration (a `.cpp` name for a C file); `fixed_pool_sizes` (0x80572528),
 *    `protopool` (0x806F4CC8) and `initialized` (0x80794E00) name the three data rows and the type and field names
 *    are the MSL allocator's scheme: all GUESSes taken from the access patterns.
 * EVIDENCE. `.rodata` 0x80572528 (fixed pool size table) is read only by the free path; `.bss` 0x806F4CC8 and
 *    `.sbss` 0x80794E00 are read only by its last function; every callee is inside the unit or `__sys_free` (the
 *    unit before it); `__close_all` (next unit) calls its free entry.
 * RESIDUALS. `free` 88.8: the target keeps the block pointer and the argument in one saved register (r31) and the pool
 *    address in r30 where ours takes three, and orders the fixed-pool path first; `SubBlock_merge_next` 93.5: the
 *    target builds the flag word before the merged size; `.bss` ours 52 B vs 56 B and `.sbss` 1 B vs 8 B (map rows pad).
 * SHAPES. a sub-block header is `{size|flags, owner, prev, next}` with the size repeated in the last word of a free
 *    block; bit 1 of the size word marks a block in use, bit 2 a free predecessor.
 */
#include "MSL_C/GCN_Mem_Alloc.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memset.h"

struct SubBlock;
struct Block;

/* A free or used run of a block. */
typedef struct SubBlock {
    /* +0x00 */ u32 size;               /* byte count with the flags in the low three bits */
    /* +0x04 */ u32 owner;              /* the owning Block, or a fixed-pool block with bit 0 clear */
    /* +0x08 */ struct SubBlock* prev;  /* free-list neighbours */
    /* +0x0C */ struct SubBlock* next;
} SubBlock; /* size: 0x10 */

/* A block obtained from the system: a header followed by its sub-blocks; the free-list head is the last word. */
typedef struct Block {
    /* +0x00 */ struct Block* prev;
    /* +0x04 */ struct Block* next;
    /* +0x08 */ u32 max_size;           /* largest free run seen */
    /* +0x0C */ u32 size;               /* byte count of the whole block */
} Block; /* size: 0x10 */

struct FixedSizeBlock;

/* One chunk of a fixed-size block: the owner pointer, then the client bytes (or the free-chunk link). */
typedef struct FixedChunk {
    /* +0x00 */ struct FixedSizeBlock* block;
    /* +0x04 */ struct FixedChunk* next;
} FixedChunk; /* size: 0x8 */

/* A sub-block carved into equal chunks. */
typedef struct FixedSizeBlock {
    /* +0x00 */ struct FixedSizeBlock* prev;
    /* +0x04 */ struct FixedSizeBlock* next;
    /* +0x08 */ u32 client_size;        /* chunk payload size */
    /* +0x0C */ FixedChunk* free_chunks;
    /* +0x10 */ u32 n_allocated;
} FixedSizeBlock; /* size: 0x14 */

typedef struct FixedPool {
    /* +0x00 */ FixedSizeBlock* rear;
    /* +0x04 */ FixedSizeBlock* front;  /* blocks with a free chunk sit at the front */
} FixedPool; /* size: 0x8 */

typedef struct PoolObj {
    /* +0x00 */ Block* start;           /* circular list of blocks */
    /* +0x04 */ FixedPool fixed_pools[6];
} PoolObj; /* size: 0x34 */

static const u32 fixed_pool_sizes[6] = { 4, 12, 20, 36, 52, 68 };

static PoolObj protopool;
static u8 initialized;

#define SubBlock_size(sb) ((sb)->size & ~7)
#define Block_start(b) (*(SubBlock**)((char*)(b) + ((b)->size & ~7) - 4))

extern "C" {

/* free: retail C linkage, the unmangled map name */
static void SubBlock_merge_next(SubBlock* ths, SubBlock** start);

/* 0x80458C94 (0x150): puts a freed sub-block on the block's free list and coalesces it with its neighbours. */
/* free: retail C linkage, the unmangled map name */
static void Block_link(Block* ths, SubBlock* sb)
{
    SubBlock** startp;
    SubBlock* merged;
    SubBlock* next_sb;
    u32 sz = SubBlock_size(sb);

    sb->size &= ~2;
    next_sb = (SubBlock*)((char*)sb + sz);
    next_sb->size &= ~4;
    ((u32*)next_sb)[-1] = sz;

    startp = &Block_start(ths);
    if (*startp != NULL) {
        sb->prev = (*startp)->prev;
        sb->prev->next = sb;
        sb->next = *startp;
        (*startp)->prev = sb;
        *startp = sb;
        merged = sb;
        if (!(sb->size & 4)) {
            u32 prev_size = ((u32*)sb)[-1];
            if (!(prev_size & 2)) {
                merged = (SubBlock*)((char*)sb - prev_size);
                merged->size &= 7;
                merged->size = (merged->size & 7) | ((prev_size + SubBlock_size(sb)) & ~7);
                if (!(merged->size & 2)) {
                    u32 total = prev_size + SubBlock_size(sb);
                    ((u32*)((char*)merged + total))[-1] = total;
                }
                if (*startp == sb) {
                    *startp = sb->next;
                }
                sb->next->prev = sb->prev;
                sb->prev->next = sb->next;
            }
        }
        *startp = merged;
        SubBlock_merge_next(merged, startp);
    } else {
        *startp = sb;
        sb->prev = sb;
        sb->next = sb;
    }

    {
        u32 largest = SubBlock_size(*startp);
        if (ths->max_size < largest) {
            ths->max_size = largest;
        }
    }
}

/* 0x80458DE4 (0xA8): absorbs the free sub-block that follows `ths` into it. */
/* free: retail C linkage, the unmangled map name */
static void SubBlock_merge_next(SubBlock* ths, SubBlock** start)
{
    u32 word = ths->size;
    u32 size = word & ~7;
    SubBlock* next_sb = (SubBlock*)((char*)ths + size);
    u32 merged_size;

    if (!(next_sb->size & 2)) {
        merged_size = size + SubBlock_size(next_sb);
        ths->size = (word & 7) | (merged_size & ~7);
        if (!(ths->size & 2)) {
            ((u32*)((char*)ths + merged_size))[-1] = merged_size;
        }
        if (!(ths->size & 2)) {
            ((SubBlock*)((char*)ths + merged_size))->size &= ~4;
        } else {
            ((SubBlock*)((char*)ths + merged_size))->size |= 4;
        }
        if (*start == next_sb) {
            *start = next_sb->next;
        }
        if (*start == next_sb) {
            *start = NULL;
        }
        next_sb->next->prev = next_sb->prev;
        next_sb->prev->next = next_sb->next;
    }
}

/* 0x80458E8C (0x1EC): returns a chunk to its fixed-size block and gives the block back when it is empty. */
static void deallocate_from_fixed_pools(PoolObj* pool, FixedChunk** client, u32 size)
{
    u32 i = 0;
    const u32* sizes = fixed_pool_sizes;
    FixedChunk* chunk;
    FixedSizeBlock* fsb;
    FixedPool* fp;
    Block* block;
    SubBlock* sb;
    int empty;

    while (size > *sizes) {
        sizes++;
        i++;
    }

    chunk = (FixedChunk*)(client - 1);
    fsb = chunk->block;
    fp = &pool->fixed_pools[i];

    if (fsb->free_chunks == NULL) {
        if (fp->front != fsb) {
            if (fp->rear == fsb) {
                fp->front = fp->front->prev;
                fp->rear = fp->rear->prev;
            } else {
                fsb->prev->next = fsb->next;
                fsb->next->prev = fsb->prev;
                fsb->next = fp->front;
                fsb->prev = fp->front->prev;
                fsb->prev->next = fsb;
                fsb->next->prev = fsb;
                fp->front = fsb;
            }
        }
    }

    chunk->next = fsb->free_chunks;
    fsb->free_chunks = chunk;
    if (--fsb->n_allocated == 0) {
        if (fp->front == fsb) {
            fp->front = fsb->next;
        }
        if (fp->rear == fsb) {
            fp->rear = fsb->prev;
        }
        fsb->prev->next = fsb->next;
        fsb->next->prev = fsb->prev;
        if (fp->front == fsb) {
            fp->front = NULL;
        }
        if (fp->rear == fsb) {
            fp->rear = NULL;
        }

        sb = (SubBlock*)((SubBlock**)fsb - 2);
        block = (Block*)(sb->owner & ~1);
        Block_link(block, sb);

        {
            SubBlock* first = (SubBlock*)(block + 1);
            empty = 0;
            if (!(first->size & 2) && SubBlock_size(first) == (block->size & ~7) - 0x18) {
                empty = 1;
            }
        }
        if (empty) {
            Block* next = block->next;
            if (next == block) {
                next = NULL;
            }
            if (pool->start == block) {
                pool->start = next;
            }
            if (next != NULL) {
                next->prev = block->prev;
                block->prev->next = next;
            }
            block->next = NULL;
            block->prev = NULL;
            __sys_free(block);
        }
    }
}

/* 0x80459078 (0x130): releases a block of the MSL heap. */
/* untyped: caller-owned heap block */
void free(void* ptr)
{
    PoolObj* pool;
    u32 size;
    Block* block;
    SubBlock* sb;
    int empty;

    if (!initialized) {
        memset(&protopool, 0, sizeof(PoolObj));
        initialized = 1;
    }

    pool = &protopool;
    if (ptr != NULL) {
        u32 owner = ((u32*)ptr)[-1];
        if (owner & 1) {
            size = SubBlock_size((SubBlock*)((SubBlock**)ptr - 2)) - 8;
        } else {
            size = ((FixedSizeBlock*)owner)->client_size;
        }
        if (size <= 68) {
            deallocate_from_fixed_pools(pool, (FixedChunk**)ptr, size);
            return;
        }

        sb = (SubBlock*)((SubBlock**)ptr - 2);
        block = (Block*)(sb->owner & ~1);
        Block_link(block, sb);

        {
            SubBlock* first = (SubBlock*)(block + 1);
            empty = 0;
            if (!(first->size & 2) && SubBlock_size(first) == (block->size & ~7) - 0x18) {
                empty = 1;
            }
        }
        if (empty) {
            Block* next = block->next;
            if (next == block) {
                next = NULL;
            }
            if (pool->start == block) {
                pool->start = next;
            }
            if (next != NULL) {
                next->prev = block->prev;
                block->prev->next = next;
            }
            block->next = NULL;
            block->prev = NULL;
            __sys_free(block);
        }
    }
}

} // extern "C"
