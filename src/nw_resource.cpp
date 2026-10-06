/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `nm build/RMHE08/main.elf`
 * and `python tools/symbols/dumpmap.py lookup`, which give a `zz_` placeholder for every fn_ address).
 *
 * The game's resource / "work" manager: the refcounted resource-memory cache (`RESmemAlloc`,
 * `RESmemFree`, `pull_res_mem`, `push_res_mem`, `getResMemAdrs`), the name table (`ckResourceName`,
 * `nwAddResource`, `nwDelResource`) and the loader-work entry points (`nwWorkInitialize`, `nwMoveStart`,
 * `nwMoveEnd`).  `.text` 0x800D2FEC-0x800D6C00 (recut 2026-09-30: it absorbs the memory-manager/g3d-work group 0x800D2FEC..0x800D45AC
 * that follows `ef/system_core.cpp`, and hands 0x800D6C00..0x800D77B0 to `g3d/g3d_xsi.cpp`).
 *
 * Naming (brief section 2, class 3 - what the code does plus the siblings' scheme): the range carries no
 * `__FILE__` string of its own.  The only file strings it references are the header `memorymanagertmp.h`
 * (the asserts inside `fn_800D45AC`/`fn_800D5F9C`) and the bare source name `g3d_xsi.cpp` (the asserts
 * inside `fn_800D74E8`, .data 0x80595790, lines 404-406); the latter argues that the tail of the range is
 * a separate original TU whose seam is not pinned.  The file name is therefore descriptive, at the root
 * beside `sys_mem.cpp`/`main.cpp`, and the remaining `fn_XXXXXXXX` names are the map's.
 *
 * Seams (evidence: `.pi/notes/ef-nwres-seam.md`).  Left edge 0x800D2FEC: the previous function is this TU's neighbour's
 * `__sinit` element constructor (`.ctors` 0x8056F2EC).  Right edge 0x800D6C00: `g3d/g3d_xsi.cpp` starts there (the `g3d_xsi.cpp`
 * file string `.data` 0x80595790, one `.sdata2` pool 0x807963C8).  The vtable group `.data` 0x80595378..0x80595428 is one TU's
 * and its readers straddle the former 0x800D45AC edge, which is therefore not a seam.
 *
 * Result (measured against the target object with `recompile.py --measure`, official report metric):
 * 35 of 111 symbols at >= 80 %, 18 of them byte-identical (100.00).  The two tables (`resMem` at
 * +0x3070 and `resTable` at +0x16070), the five heap/allocator entry points, the name lookup / refcount
 * cluster (`pull_res_mem`, `push_res_mem`, `ckResourceName`, `fn_800D4CE4`/`fn_800D4DD8`, `nwDelResource`),
 * the retable reset pair (`fn_800D490C`/`fn_800D4A7C`) and the work entry points (`nwWorkInitialize`,
 * `nwMoveStart`, `nwMoveEnd`, `fn_800D5A04`/`fn_800D5A08`/`fn_800D5C4C`) are covered.
 *
 * Residuals / not reconstructed:
 *   - `fn_800D45AC` (448 B, the unit's namesake): the embedded free-list pool allocator over a struct
 *     whose +0x2C/+0x30 are the list head/tail (node = {next, prev, +0x08, +0x0C, size@+0x10,
 *     used@+0x14}); 0 % (no body written).  Its `memorymanagertmp.h` assert is line 580.
 *   - `nwAddResource` (76.29 %, just under the bar): 420 B, logic reproduced (basename, live-entry
 *     bump via `ckResourceName`, free-slot scan over `resTable`, then `strcpy`/data/index/flag) but the
 *     register-save idiom differs - the target keeps 4 callee-saved registers as four `stw`s, ours via
 *     `_savegpr_27` (5), which shifts every register by one.  Probed declaration order and a table-base
 *     local; the allocation is the compiler's.
 *   - the 0x800D5D60-0x800D74E8 tail (76 functions, incl. the `fn_800D5F9C` particle-manager walkers
 *     and `fn_800D74E8`, which carries the `g3d_xsi.cpp` asserts at .data 0x80595790): not written.
 *   - `#pragma peephole off` is a per-unit deviation: no record-form instruction occurs anywhere in the
 *     target object, and `fn_800D476C` encodes `~(a-1) & (s+a-1)` as `not`+`and` rather than the
 *     peephole pass's fused `andc` (with `align` a *signed* type).  If the sibling `800CDB2C` range needs
 *     the same, this belongs in that lib's cflags instead.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `_savegpr_27`, `_restgpr_27`.
 *   flipcheck: `.sdata` claimed, not emitted.
 *   flipcheck: force-active in retail .comment, not in ours: `fn_800D4C14`, `fn_800D4CA4`, `fn_800D516C`,
 *     `fn_800D5250`, `fn_800D5360`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "nw_resource.h" /* `ResEntry` and the C++ entry points (this unit owns them) */
#include "ef/nw_res_manager.h"
#include "ef/pRoot.h"

/* The two `.sbss` words this unit defines (0x80794970-0x80794978): the resource manager every function below walks, and the
 * g3d model root (stored once, by `fn_800D32CC`; read by `nwWorkInitialize`/`nwMoveStart`/`nwMoveEnd`).  Definer of both:
 * `.sbss` follows text order and the unit's first word is 0x80794970 (seam evidence in the header). */
ResManager* nw_res_manager;
s32 pRoot;

/* The whole range is peephole-clean in retail: no record-form instruction (`add.`, `and.`, `extsb.`, ...)
 * occurs anywhere in the target object, and `fn_800D476C` encodes `~(align-1) & end` as `not`+`and`
 * rather than the peephole pass's fused `andc`.  Scoped here (one registration) instead of a lib flag. */
#pragma peephole off

/* --- SDK symbols (unsplit band; declared here as the neighbouring system units do) --- */
extern "C" {
void* MEMCreateExpHeapEx(void* startAddress, u32 size, u16 option);
void MEMInitAllocatorForExpHeap(void* allocator, void* heap, s32 align);
void* MEMAllocFromAllocator(void* allocator, u32 size);
void MEMFreeToAllocator(void* allocator, void* block);
void MEMDestroyExpHeap(void* heap);
void* memset(void* dst, int val, u32 n);
int strcmp(const char* a, const char* b);
char* strcpy(char* dst, const char* src);
char* fn_80041404(const char* str, int ch);
}

/* The Dolphin `MEMAllocator`: a function pointer and its heap/parameter words. */
struct MEMAllocator {
    /* +0x00 */ u32 func;
    /* +0x04 */ void* heap;
    /* +0x08 */ u32 param1;
    /* +0x0C */ u32 param2;
}; /* size: 0x10 */

/* One entry of the resource-memory cache (`ResManager.resMem`, stride 0x4C). */
struct ResMemEntry {
    /* +0x00 */ void* data;
    /* +0x04 */ u32 size;
    /* +0x08 */ char name[64];
    /* +0x48 */ s32 refcount;
}; /* size: 0x4C */

/* The resource/"work" manager block, pointed to by the `.sbss` global 0x80794970. */
struct ResManager {
    /* +0x000 */ u32 heap0_start;
    /* +0x004 */ u32 heap0_size;
    /* +0x008 */ u32 heap0_free;
    /* +0x00C */ u32 heap1_start;
    /* +0x010 */ u32 heap1_size;
    /* +0x014 */ u32 heap1_free;
    /* +0x018 */ u32 heap2_start;
    /* +0x01C */ u32 heap2_size;
    /* +0x020 */ u32 heap2_free;
    /* +0x024 */ u32 heap3_start;
    /* +0x028 */ u32 heap3_size;
    /* +0x02C */ void* heap1;
    /* +0x030 */ void* heap0;
    /* +0x034 */ void* heap2;
    /* +0x038 */ MEMAllocator alloc1;
    /* +0x048 */ MEMAllocator alloc0;
    /* +0x058 */ MEMAllocator alloc2;
    /* +0x068 */ u8 pad_0x068[0x3070 - 0x068];
    /* +0x3070 */ ResMemEntry resMem[1024];
    /* +0x16070 */ ResEntry resTable[1024];
    /* +0x29070 */ u8 pad_0x29070[0x4];
    /* +0x29074 */ void* ptr_0x29074;
    /* +0x29078 */ u8 pad_0x29078[0x8];
    /* +0x29080 */ u16 count_0x29080;
    /* +0x29082 */ u8 pad_0x29082[0x2];
    /* +0x29084 */ void* ptr_0x29084;
}; /* size: 0x29088 */

extern u8 lbl_80595428[];

/* The C-linkage helpers: their map names are plain `fn_XXXXXXXX`, so an `extern "C"` declaration keeps
 * the symbol; the definition below then inherits that linkage. */
extern "C" {
void mtx34_copy(void* a, void* b);
u32 fn_800D312C(void* heap);
u32 fn_800D476C(u32 size, s32 align);
void fn_800D4784(void* self);
u32 fn_800D4794(u16 a, u16 b, u16 c, u16 d, u32 e);
void fn_800D4848(void);
void fn_800D48AC(void);
void fn_800D4908(void* a, void* b);
void fn_800D490C(void);
void* fn_800D4A74(void* startAddress, u32 size);
void fn_800D4A7C(void);
void* fn_800D4C14(u32 size);
void fn_800D4CA4(void* p);
char* fn_800D4D9C(s32 index);
s32 fn_800D4CE4(const char* path);
s32 fn_800D4DD8(const char* path);
u32 fn_800D5138(s32 index);
s32 fn_800D516C(void* data);
void fn_800D5250(void);
s32 fn_800D5360(char* path);
ResEntry* fn_800D5418(s32 index);
void fn_800D58B0(s32 index);
void fn_800D58DC(void);
s32 fn_800D5A04(char* name, void* data);
void* fn_800D5A08(char* path, u32 size);
void fn_800D5C4C(char* path, u32* entry);
s32 fn_800D5CAC(void* table);
void fn_800D5F00(void);
void fn_800D5F10(void);
void fn_800D5F20(void);
void fn_800D5F30(void);
void fn_800A5D8C(void* p, u32 index);
void fn_800D31B0(void);
void fn_800D320C(void);
void fn_800D3C4C(void);
void fn_80082668(void* root);
void fn_80083290(void* root);
void fn_800832DC(void* root);
}

/* The C++-linkage entry points (defined below with their real signature, never the mangled spelling -
 * the map's `RESmemAlloc__FUl` and friends are the compiler's spelling of these). */
void* RESmemAlloc(u32 size);
void RESmemFree(void* p);
s32 nwDelResource(s32 index);
void nwWorkInitialize(void);
void nwMoveStart(void);
void nwMoveEnd(void);
MTX34 get_current_view_mtx(void);
void load_file_req(char* path, u32 a, s32 b, u32 c, s32 d, u32* e);


/* 0x800D476C - round `size` up to an `align` boundary. */
u32 fn_800D476C(u32 size, s32 align) {
    return (size + align - 1) & ~(align - 1);
}

/* 0x800D4784 - install the vtable/data word at the head of an object. */
void fn_800D4784(void* self) {
    *(void**)self = lbl_80595428;
}

/* 0x800D4794 - the combined allocation size for a five-axis record. */
u32 fn_800D4794(u16 a, u16 b, u16 c, u16 d, u32 e) {
    return fn_800D476C(b * 340 + 32, 32) + fn_800D476C(a * 164 + 32, 32) +
           fn_800D476C(c * 188 + 32, 32) + fn_800D476C(d * 232 + 32, 32) +
           fn_800D476C(e * 28 + 32, 32) + 544;
}

/* 0x800D4908 - hand a matrix to the view-matrix update (a tail call). */
void fn_800D4908(void* dst, void* src) {
    mtx34_copy(dst, src);
}

/* 0x800D4A74 - create an expansion heap with the default flags. */
void* fn_800D4A74(void* startAddress, u32 size) {
    return MEMCreateExpHeapEx(startAddress, size, 0);
}

/* 0x800D4BC4 - allocate from the resource-memory allocator and cache the free total. */
void* RESmemAlloc(u32 size) {
    ResManager* m = nw_res_manager;
    void* p = MEMAllocFromAllocator(&m->alloc1, size);
    m->heap1_free = fn_800D312C(m->heap1);
    return p;
}

/* 0x800D4C14 - allocate from the second resource allocator and cache the free total. */
void* fn_800D4C14(u32 size) {
    ResManager* m = nw_res_manager;
    void* p = MEMAllocFromAllocator(&m->alloc0, size);
    m->heap0_free = fn_800D312C(m->heap0);
    return p;
}

/* 0x800D4C64 - free into the resource-memory allocator and cache the free total. */
void RESmemFree(void* p) {
    ResManager* m = nw_res_manager;
    MEMFreeToAllocator(&m->alloc1, p);
    m->heap1_free = fn_800D312C(m->heap1);
}

/* 0x800D4CA4 - free into the second resource allocator and cache the free total. */
void fn_800D4CA4(void* p) {
    ResManager* m = nw_res_manager;
    MEMFreeToAllocator(&m->alloc0, p);
    m->heap0_free = fn_800D312C(m->heap0);
}

/* 0x800D4D9C - the name of a live resource-memory slot, or NULL. */
char* fn_800D4D9C(s32 index) {
    ResManager* m = nw_res_manager;
    if (m->resMem[index].data == 0) {
        return 0;
    }
    if (m->resMem[index].size == 0) {
        return 0;
    }
    return m->resMem[index].name;
}

/* 0x800D5104 - the data pointer of a resource-memory slot, or NULL. */
void* getResMemAdrs(s32 index) {
    if (index < 0) {
        return 0;
    }
    if (index >= 1024) {
        return 0;
    }
    return nw_res_manager->resMem[index].data;
}

/* 0x800D5138 - the size of a resource-memory slot, or 0. */
u32 fn_800D5138(s32 index) {
    if (index < 0) {
        return 0;
    }
    if (index >= 1024) {
        return 0;
    }
    return nw_res_manager->resMem[index].size;
}

/* 0x800D516C - the first resource-memory slot holding `data`, or -1. */
s32 fn_800D516C(void* data) {
    ResManager* m = nw_res_manager;
    s32 i;
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data == data) {
            return i;
        }
    }
    return -1;
}

/* 0x800D5250 - release every live resource-memory slot. */
void fn_800D5250(void) {
    ResManager* m = nw_res_manager;
    s32 i;
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data != 0) {
            push_res_mem(i);
        }
    }
}

/* 0x800D4CE4 - find a resource-memory slot by basename and take a reference. */
s32 fn_800D4CE4(const char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    char* p = fn_80041404(path, 47);
    if (p != 0) {
        path = p + 1;
    }
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data != 0 && m->resMem[i].size != 0 && strcmp(m->resMem[i].name, path) == 0) {
            m->resMem[i].refcount++;
            return i;
        }
    }
    return -1;
}

/* 0x800D4DD8 - find a resource-memory slot by basename, without taking a reference. */
s32 fn_800D4DD8(const char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    char* p = fn_80041404(path, 47);
    if (p != 0) {
        path = p + 1;
    }
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data != 0 && m->resMem[i].size != 0 && strcmp(m->resMem[i].name, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* 0x800D52A8 - find a resource-table entry by basename; NULL when absent. */
ResEntry* ckResourceName(char* path) {
    ResManager* m = nw_res_manager;
    ResEntry* e = m->resTable;
    s32 i;
    char* p = fn_80041404(path, 47);
    if (p != 0) {
        path = p + 1;
    }
    for (i = 0; i < 1024; i++, e++) {
        if (e->index != -1 && e->name[0] != 0 && e->data != 0 && strcmp(e->name, path) == 0) {
            return e;
        }
    }
    return 0;
}

/* 0x800D5360 - the table index of a resource-table entry by basename, or -1. */
s32 fn_800D5360(char* path) {
    ResManager* m = nw_res_manager;
    ResEntry* e = m->resTable;
    s32 i;
    char* p = fn_80041404(path, 47);
    if (p != 0) {
        path = p + 1;
    }
    for (i = 0; i < 1024; i++, e++) {
        if (e->index != -1 && e->name[0] != 0 && e->data != 0 && strcmp(e->name, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* 0x800D5418 - the resource-table entry whose index equals `index`, or NULL. */
ResEntry* fn_800D5418(s32 index) {
    ResManager* m = nw_res_manager;
    ResEntry* e = m->resTable;
    s32 i;
    if (index < 0) {
        return 0;
    }
    if (index >= 1024) {
        return 0;
    }
    for (i = 0; i < 1024; i++, e++) {
        if (e->index != -1 && e->name[0] != 0 && e->data != 0 && e->index == index) {
            return e;
        }
    }
    return 0;
}

/* 0x800D506C - drop one reference on a resource-memory slot; free it at zero. */
s32 push_res_mem(s32 index) {
    ResManager* m = nw_res_manager;
    if (m->resMem[index].data == 0) {
        return -1;
    }
    m->resMem[index].refcount = m->resMem[index].refcount - 1;
    if (m->resMem[index].refcount > 0) {
        return -1;
    }
    RESmemFree(m->resMem[index].data);
    m->resMem[index].data = 0;
    m->resMem[index].size = 0;
    m->resMem[index].name[0] = 0;
    m->resMem[index].refcount = 0;
    return 0;
}

/* 0x800D4848 - walk the auxiliary table the work system publishes. */
void fn_800D4848(void) {
    ResManager* m = nw_res_manager;
    u32 i;
    if (m->ptr_0x29074 != 0) {
        for (i = 0; i < m->count_0x29080; i++) {
            fn_800A5D8C(m->ptr_0x29074, i);
        }
    }
}

/* 0x800D48AC - hand the current view matrix to the work system. */
void fn_800D48AC(void) {
    ResManager* m = nw_res_manager;
    if (m->ptr_0x29074 != 0 && m->ptr_0x29084 != 0) {
        fn_800D4908(m->ptr_0x29084, &get_current_view_mtx());
    }
}

/* 0x800D490C - size the four resource heaps and reset the two tables. */
void fn_800D490C(void) {
    ResManager* m = nw_res_manager;
    u32 i;
    m->heap0_start = 0x81276600;
    m->heap0_size = 0x00280000;
    m->heap2_start = 0x80FF6600;
    m->heap2_size = 0x00280000;
    m->heap3_start = 0x814F6600;
    m->heap3_size = 0x00140000;
    m->heap1_start = 0x90F58000;
    m->heap1_size = 0x01B60000;
    m->heap0 = fn_800D4A74((void*)m->heap0_start, m->heap0_size);
    MEMInitAllocatorForExpHeap(&m->alloc0, m->heap0, 32);
    m->heap1 = fn_800D4A74((void*)m->heap1_start, m->heap1_size);
    MEMInitAllocatorForExpHeap(&m->alloc1, m->heap1, 32);
    m->heap2 = fn_800D4A74((void*)m->heap2_start, m->heap2_size);
    MEMInitAllocatorForExpHeap(&m->alloc2, m->heap2, 32);
    for (i = 0; i < 1024; i++) {
        m->resMem[i].data = 0;
        m->resMem[i].size = 0;
        m->resMem[i].name[0] = 0;
        m->resMem[i].refcount = 0;
        memset(m->resTable[i].name, 0, 64);
        m->resTable[i].index = -1;
        m->resTable[i].data = 0;
        m->resTable[i].flag = 0;
    }
    m->heap1_free = fn_800D312C(m->heap1);
    m->heap0_free = fn_800D312C(m->heap0);
    m->heap2_free = fn_800D312C(m->heap2);
    fn_800D320C();
    fn_800D3C4C();
}

/* 0x800D4A7C - rebuild the resource heaps and reset the two tables. */
void fn_800D4A7C(void) {
    ResManager* m = nw_res_manager;
    u32 i;
    if (m->heap0 != 0) {
        MEMDestroyExpHeap(m->heap0);
    }
    if (m->heap1 != 0) {
        MEMDestroyExpHeap(m->heap1);
    }
    if (m->heap2 != 0) {
        MEMDestroyExpHeap(m->heap2);
    }
    m->heap0 = fn_800D4A74((void*)m->heap0_start, m->heap0_size);
    m->heap1 = fn_800D4A74((void*)m->heap1_start, m->heap1_size);
    m->heap2 = fn_800D4A74((void*)m->heap2_start, m->heap2_size);
    MEMInitAllocatorForExpHeap(&m->alloc0, m->heap0, 32);
    MEMInitAllocatorForExpHeap(&m->alloc1, m->heap1, 32);
    MEMInitAllocatorForExpHeap(&m->alloc2, m->heap2, 32);
    for (i = 0; i < 1024; i++) {
        m->resMem[i].data = 0;
        m->resMem[i].size = 0;
        m->resMem[i].name[0] = 0;
        m->resMem[i].refcount = 0;
        memset(m->resTable[i].name, 0, 64);
        m->resTable[i].index = -1;
        m->resTable[i].data = 0;
        m->resTable[i].flag = 0;
    }
    m->heap1_free = fn_800D312C(m->heap1);
    m->heap0_free = fn_800D312C(m->heap0);
    m->heap2_free = fn_800D312C(m->heap2);
    fn_800D320C();
    fn_800D4848();
}

/* 0x800D5864 - drop a resource-table entry's registration. */
s32 nwDelResource(s32 index) {
    ResManager* m = nw_res_manager;
    if (m->resTable[index].flag > 1) {
        m->resTable[index].flag--;
        return -1;
    }
    m->resTable[index].name[0] = 0;
    m->resTable[index].index = -1;
    m->resTable[index].data = 0;
    m->resTable[index].flag = 0;
    return 0;
}

/* 0x800D58B0 - clear one resource-table entry outright. */
void fn_800D58B0(s32 index) {
    ResEntry* e = &nw_res_manager->resTable[index];
    e->name[0] = 0;
    e->index = -1;
    e->data = 0;
    e->flag = 0;
}

/* 0x800D58DC - clear every resource-table entry. */
void fn_800D58DC(void) {
    ResManager* m = nw_res_manager;
    u32 i;
    for (i = 0; i < 1024; i++) {
        m->resTable[i].name[0] = 0;
        m->resTable[i].index = -1;
        m->resTable[i].data = 0;
        m->resTable[i].flag = 0;
    }
}

/* 0x800D5A04 - add a resource-table entry (the load callback's spelling of `nwAddResource`). */
s32 fn_800D5A04(char* name, void* data) {
    return nwAddResource(name, data);
}

/* 0x800D5A08 - pull a resource-memory slot and queue its file read. */
void* fn_800D5A08(char* path, u32 size) {
    s32 index = pull_res_mem(path, size, 0);
    if (index < 0) {
        return 0;
    }
    void* p = getResMemAdrs(index);
    load_file_req(path, (u32)p, (s32)size, (u32)fn_800D5A04, 0, 0);
    return p;
}

/* 0x800D5C4C - queue one table entry's file read. */
void fn_800D5C4C(char* path, u32* entry) {
    load_file_req((char*)entry[1], (u32)path, (s32)entry[0], (u32)fn_800D5A04, 0, 0);
}

/* 0x800D5CAC - pull every entry of a (name, size) request table. */
s32 fn_800D5CAC(void* table) {
    u32* e = (u32*)table;
    s32 count = 0;
    while (e[0] != 0) {
        if (fn_800D4CE4((const char*)e[1]) < 0) {
            fn_800D5A08((char*)e[1], e[0]);
            count++;
        }
        e += 2;
    }
    return count;
}

/* 0x800D5C74 - bring up the work system. */
void nwWorkInitialize(void) {
    if (pRoot == 0) {
        fn_80082668((void*)pRoot);
    }
    fn_800D58DC();
    fn_800D31B0();
    fn_800D4A7C();
}

/* 0x800D5D18 - start the move phase. */
void nwMoveStart(void) {
    if (pRoot != 0) {
        fn_80082668((void*)pRoot);
    }
}

/* 0x800D5D2C - end the move phase. */
void nwMoveEnd(void) {
    if (pRoot != 0) {
        fn_80083290((void*)pRoot);
        fn_800832DC((void*)pRoot);
    }
}

/* 0x800D4E88 - the refcounted resource-memory pull: reuse a live slot, else allocate one. */
s32 pull_res_mem(char* path, u32 size, s32 mode) {
    ResManager* m = nw_res_manager;
    char* name;
    s32 i;
    if (size == 0) {
        return -1;
    }
    if (path != 0) {
        char* p = fn_80041404(path, 47);
        name = p != 0 ? p + 1 : path;
        if (mode != 1) {
            for (i = 0; i < 1024; i++) {
                if (m->resMem[i].data != 0 && m->resMem[i].size != 0 &&
                    strcmp(m->resMem[i].name, name) == 0) {
                    m->resMem[i].refcount++;
                    return i;
                }
            }
        }
    }
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data == 0) {
            break;
        }
    }
    if (i >= 1024) {
        return -1;
    }
    u32 asize = (size + 31) & ~31;
    void* p = RESmemAlloc(asize);
    if (p == 0) {
        return -1;
    }
    m->resMem[i].data = p;
    m->resMem[i].size = asize;
    m->resMem[i].refcount = 1;
    if (path != 0) {
        strcpy(m->resMem[i].name, name);
    } else {
        m->resMem[i].name[0] = 0;
    }
    return i;
}

/* 0x800D5550 - register a resource-table entry (or bump an existing one's reference). */
s32 nwAddResource(char* name, void* data) {
    ResEntry* table = nw_res_manager->resTable;
    ResEntry* e;
    s32 i;
    if (name == 0) {
        return -1;
    }
    if (name[0] == 0) {
        return -1;
    }
    char* p = fn_80041404(name, 47);
    if (p != 0) {
        name = p + 1;
    }
    e = (ResEntry*)ckResourceName(name);
    if (e != 0) {
        e->flag++;
        return e->index;
    }
    if (data == 0) {
        return -1;
    }
    for (i = 0; i < 1024; i++) {
        if (table[i].index == -1) {
            break;
        }
    }
    if (i >= 1024) {
        return -1;
    }
    strcpy(table[i].name, name);
    table[i].data = data;
    table[i].index = i;
    table[i].flag = 1;
    return i;
}
