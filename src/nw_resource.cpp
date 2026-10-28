/*
 * nw_resource.cpp - the game's resource / "work" manager: the g3d work pool and model/animation builders, the
 * effect memory manager (`memorymanagertmp.h`: nw4r::ef's MemoryManagerBase, the per-kind object pools and the
 * free-list heap), the refcounted resource-memory cache, the resource name table, the player/enemy file loaders
 * and the loader-work entry points.
 *
 * RANGE. `.text` 0x800D2FEC-0x800D6C00, `.data` 0x80595378-0x80595790 (two PTMF constants, the eight vtables, the
 *   assert strings), `.sdata` 0x80791370 (the effect pool sizes), `.sbss` 0x80794970-0x80794978, `.sdata2`
 *   0x807963C0.  Seams: `.pi/notes/ef-nwres-seam.md`; the left neighbour is `ef/fn_800CDB2C.cpp`, the right one
 *   `g3d/g3d_xsi.cpp`.
 * FLAGS. cflags_main.  `#pragma peephole off`: no record-form instruction occurs in the target and the round-up
 *   keeps `not`+`and`.  `#pragma dont_inline on`: retail calls every inline helper out of line (the memory
 *   manager's members, the round-up, the placement operators), each emitted after the first function that
 *   references it, last-referenced first; the vtables' members follow the last function.
 * NAMES. The memory manager's classes are GUESS names: `NwResMemoryManager` (nw4r's `MemoryManagerTmp` shape),
 *   `NwResObjectPool<T>`/`NwResObjectPoolLists`, `NwResHeapBlock`; `MemoryManagerBase` and its slot names are
 *   nw4r's (the assert strings name `memorymanagertmp.h`, `mHeapStartAddr`, `pp->mParticleManager`).  GUESS (from
 *   each body and its callers): `nw_res_effect_init`, `nw_res_ef_system_bind`, `nw_res_activity_release`,
 *   `nw_res_pm_release_particle`, `nw_res_heap_free_size`, `nw_res_create_heap`, `nw_res_mtx_copy`,
 *   `nw_res_ef_pool_sizes`, `nw_res_entry_clear`, `nw_resource_init`, the g3d work pool (`pull_g3d_wk*`,
 *   `push_g3d_wk_stage`/`_all`, `clear_g3d_wk`, the `_g3d_work` fields),
 *   `nw_res_scn_root_get`, `nw_res_ef_system_get`, `nw_res_ef_num_group_get`, `nw_res_ef_retire_all`,
 *   `nw_res_heap_init`/`_reset`, `nw_res_mem1_alloc`/`_free`, `res_mem_index_ref`, `res_mem_name_get`,
 *   `res_mem_index_of_data`, `res_mem_release_all`, `res_table_index_get`, `res_table_clear`, `nw_res_entry_add`,
 *   `nw_res_file_loaded`/`_load`/`_load_entry`, the player-file loaders `nw_res_pl_file_*` over the view
 *   `NwResPlLoadWork`, `nw_res_em_mdl_handle_set`/`_mot_` over the views `NwResKindWork`/`NwResKindSlots`;
 *   lane sound-mhchar's registry names `nw_scn_mdl_create`, `nw_anm_chr_blend_create`,
 *   `nw_anm_chr_res_create`; registry GUESS `ef_system_get_instance`, `ef_system_initialize`, `ef_system_retire_group`,
 *   `pl_res_buffer_get`, `pl_res_loaded_set`, `ef_particle_manager_ctor`, `ef_particle_manager_delete`,
 *   `res_file_has_anm_tex_srt`, `res_file_get_res_anm_tex_srt_num`, `res_file_get_res_anm_tex_srt`,
 *   `res_anm_tex_srt_copy`, `res_anm_tex_srt_is_valid`; GUESS `nw_res_anm_work_num_has_tex_srt`/
 *   `nw_res_anm_create_has_tex_srt` (the two member-pointer records).
 * SHAPES. The animation builders test `ResFile`'s `AnmTexSrt` category test as a member pointer, never called: retail
 *   tests a constant `{0, -1, 0x80093794}` record with `__ptmf_test` (always true).  The owner defines the test with C
 *   linkage, so the records are spelled as `__ptmf` statics.  `nw_res_anm_create` takes its rate second (every caller
 *   evaluates it before the handles; float-last was 91.30 %, float-second 100 %).  The Particle pool's
 *   `GarbageCollection` is declared, never defined (the manager collects particles itself): the declaration
 *   instantiates the pool class before the manager's, which puts the manager's vtable first in `.data`, as retail.
 * RESIDUALS. `.sdata2`: retail's 2.0f entry is 8 B (0x807963C0), ours 4 B.
 *   The pooled ef classes are views in `nw_resource.h` (`nw4r::ef::Effect`/`Emitter`/`ParticleManager`/
 *   `Particle`/`DrawInfo`); their full classes live in their own units, so the type is defined twice.
 *   The pool array constructors reach only symbols that exist: the Emitter and Particle views declare no destructor
 *   (retail passes the owners' C-linkage deleting destructors 0x800A630C/`ef_particle_delete`, ours 0: two
 *   `lis`/`addi` against one `li`), and the particle-manager array is spelled out with `__construct_new_array` over
 *   `ef_particle_manager_ctor`/`_delete`; its new-expression cleanup pad is written as a `catch (...)` that frees the
 *   block and rethrows, which adds the handler's `__end__catch` tail (6 instructions) retail's pad does not have, and
 *   the count is reloaded where retail keeps it in r30 (a cached local measured worse): 1404 B against 1384.
 *   `nw_res_effect_init` copies the pool sizes field by field; retail materialises the struct's address first
 *   (`li r3,sizes@sda21` + four `lhz`/`sth`), ours folds the first load into `lhz r0,sizes@sda21` (a pointer local
 *   and a `dst`/`src` pair: no gain; the struct assignment becomes an `__as__` call under `dont_inline`, and
 *   `dont_inline off` inlines the bind helper).
 *   `nw_res_list_load`: retail tests the entry's size word in r4, ours in r0 (a `size` local, a `for` loop: no change).
 *   `ResEntry.flag` is compared signed by `nwDelResource` (a cast; the shared header types it `u32`).
 */

#define MHTRI_NW_RESOURCE_OWNER
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/ut_list.h"
#include "nw4r/db_assert.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "MSL/new.h"           /* the placement `new`/`new[]`/`delete[]` forms */
#include "g3d/g3d_anmscn.h"     /* the placement `operator delete` (owner g3d/g3d_anmscn.cpp) */
#include "nw_resource.h" /* `ResEntry`, the pooled ef object views and the C++ entry points (this unit owns them) */
#include "ef/nw_res_manager.h"
#include "ef/pRoot.h"
#include "ef/ef_effectsystem.h"
#include "ef/ef_list_get_first.h"
#include "ef/ef_get_life_status.h"
#include "ef/ef_system_get_instance.h"
#include "ef/ef_system_initialize.h"
#include "ef/ef_system_retire_group.h"
#include "ef/ef_particle_manager_ctor.h"
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "g3d/g3d_scnobj.h" /* nw4r::g3d::ScnGroup::Clear, owner g3d/g3d_scnobj.cpp (rule 2) */
#include "draw_shape/mtx34_copy.h"
#include "camera/camera.h" /* get_current_view_mtx */
#include "g3d/g3d_scnroot.h"
#include "g3d/g3d_scnmdlsmpl.h"
#include "nw4r/g3d/scnmdl.h"
#include "g3d/g3d_anmchr.h"
#include "g3d/g3d_anmtexsrt.h"
#include "g3d/res_file_has_anm_tex_srt.h"
#include "g3d/res_anm_tex_srt_copy.h"
#include "g3d/PlayPolicy_Loop.h"
#include "Runtime.PPCEABI.H/ptmf.h"
#include "g3d/G3dInit.h"
#include "g3d/g3d_state_set_render_mode.h"
#include "camera/camera_work_init.h"
#include "mh3_pad/system_w.h"
#include "main_heap_allocator.h"
#include "nw_res_stage_free.h"
#include "ef/load_file_req.h"
#include "ef/get_move_work_adrs.h"
#include "draw_shape.h" /* res_file_ctor, res_file_assign */
#include "sound/mhchar_res_bank_set.h"
#include "Pl/fn_802693C4.h" /* Get_pl_type and the weapon-parts refresh */
#include "Pl/pl_res_buffer_get.h"
#include "Pl/pl_res_loaded_set.h"

/* The linker's start of `.bss`, where the boot render mode is kept (`main.cpp` reads it the same way). */
extern char _f_bss[];

/* The two `.sbss` words this unit defines (0x80794970-0x80794978): the resource manager every function below walks, and the
 * g3d model root (stored once, by `nw_resource_init`; read by `nwWorkInitialize`/`nwMoveStart`/`nwMoveEnd`).  Definer of both:
 * `.sbss` follows text order and the unit's first word is 0x80794970 (seam evidence in the header). */
ResManager* nw_res_manager;
s32 pRoot;

/* The whole range is peephole-clean in retail: no record-form instruction (`add.`, `and.`, `extsb.`, ...)
 * occurs anywhere in the target object, and the round-up helper encodes `~(align-1) & end` as `not`+`and`
 * rather than the peephole pass's fused `andc`.  Scoped here (one registration) instead of a lib flag. */
#pragma peephole off
/* Retail calls every inline helper out of line (the round-up, the memory manager's members, the placement
 * operators), each emitted after the first function that references it. */
#pragma dont_inline on

/* --- SDK symbols (unsplit band; declared here as the neighbouring system units do) --- */
extern "C" {
void* memset(void* dst, int val, u32 n);
int strcmp(const char* a, const char* b);
char* strcpy(char* dst, const char* src);
char* str_rfind(const char* str, int ch);
}

/* One entry of the resource-memory cache (`ResManager.resMem`, stride 0x4C). */
struct ResMemEntry {
    /* +0x00 */ void* data;
    /* +0x04 */ u32 size;
    /* +0x08 */ char name[64];
    /* +0x48 */ s32 refcount;
}; /* size: 0x4C */

/* The effect pool sizes the manager is built with: effects, emitters, particle managers, particles. */
struct NwResEfPoolSizes {
    /* +0x00 */ u16 numEffect;
    /* +0x02 */ u16 numEmitter;
    /* +0x04 */ u16 numParticleManager;
    /* +0x06 */ u16 numParticle;
}; /* size: 0x8 */

class NwResMemoryManager;

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
    /* +0x02C */ MEMiHeapHead* heap1;
    /* +0x030 */ MEMiHeapHead* heap0;
    /* +0x034 */ MEMiHeapHead* heap2;
    /* +0x038 */ MEMAllocator alloc1;
    /* +0x048 */ MEMAllocator alloc0;
    /* +0x058 */ MEMAllocator alloc2;
    /* +0x068 */ nw4r::g3d::ScnRoot* root;
    /* +0x06C */ u32 rootSize;
    /* +0x070 */ _g3d_work g3dWorks[1024];
    /* +0x3070 */ ResMemEntry resMem[1024];
    /* +0x16070 */ ResEntry resTable[1024];
    /* +0x29070 */ NwResMemoryManager* mEfMemoryManager;
    /* +0x29074 */ EfSys* mEfSystem;
    /* +0x29078 */ NwResEfPoolSizes mEfPoolSizes;
    /* +0x29080 */ u16 mEfNumGroup;
    /* +0x29082 */ u8 pad_0x29082[0x2];
    /* +0x29084 */ nw4r::ef::DrawInfo* mEfDrawInfo;
}; /* size: 0x29088 */

/* The C-linkage helpers: their map names are plain C names, so an `extern "C"` declaration keeps
 * the symbol; the definition below then inherits that linkage. */
extern "C" {
void nw_res_ef_retire_all(void);
void nw_res_view_mtx_apply(void);
void nw_res_heap_init(void);
void nw_res_heap_reset(void);
void* nw_res_mem1_alloc(u32 size);
void nw_res_mem1_free(void* p);
char* res_mem_name_get(s32 index);
s32 res_mem_index_ref(const char* path);
s32 res_mem_index_get(const char* path);
u32 nw_res_mem_size(s32 index);
s32 res_mem_index_of_data(void* data);
void res_mem_release_all(void);
s32 res_table_index_get(char* path);
ResEntry* nw_res_entry_get(s32 index);
void nw_res_entry_clear(s32 index);
void res_table_clear(void);
s32 nw_res_file_loaded(char* name, void* data);
void* nw_res_file_load(char* path, u32 size);
void nw_res_file_load_entry(char* path, u32* entry);
s32 nw_res_list_load(u32* table);
struct NwResKindWork;
void nw_res_em_mdl_handle_set(struct NwResKindWork* work);
void nw_res_em_mot_handle_set(struct NwResKindWork* work);
void nw_res_effect_init(void);
_g3d_work* nw_scn_mdl_create(nw4r::g3d::ResMdl mdl, u32 bufferOption, int numView);
nw4r::g3d::AnmObjChrBlend* nw_anm_chr_blend_create(nw4r::g3d::ResMdl mdl, int numChildren);
nw4r::g3d::AnmObjChrRes* nw_anm_chr_res_create(nw4r::g3d::ResAnmChr res, nw4r::g3d::ResMdl mdl, bool bHasCache);
void frame_ctrl_set_play_policy(nw4r::g3d::FrameCtrl* self, nw4r::g3d::PlayPolicyFunc policy);
}

/* The C++-linkage entry points (defined below with their real signature, never the mangled spelling -
 * the map's `RESmemAlloc__FUl` and friends are the compiler's spelling of these). */
void* RESmemAlloc(u32 size);
void RESmemFree(void* p);
s32 nwDelResource(s32 index);
void nwWorkInitialize(void);
void nwMoveStart(void);
void nwMoveEnd(void);

/* The largest block the expansion heap can hand out at 4-byte alignment. */
inline u32 nw_res_heap_free_size(MEMiHeapHead* heap) {
    return MEMGetAllocatableSizeForExpHeapEx(heap, 4);
}

/* Hands a matrix to the view-matrix copy. */
inline void nw_res_mtx_copy(nw4r::ef::DrawInfo* dst, MTX34* src) {
    mtx34_copy(&dst->mViewMtx, src);
}

/* Creates an expansion heap with the default flags. */
inline MEMiHeapHead* nw_res_create_heap(u32 startAddress, u32 size) {
    return (MEMiHeapHead*)MEMCreateExpHeapEx((void*)startAddress, size, 0);
}


/* 0x800D2FEC (0x6C): takes a free g3d work slot, cleared and marked used; NULL when all are taken. */
_g3d_work* pull_g3d_wk(void) {
    _g3d_work* work = nw_res_manager->g3dWorks;
    s32 i;
    for (i = 0; i < 1024; i++, work++) {
        if (work->used == 0) {
            memset(work, 0, sizeof(_g3d_work));
            work->used = 1;
            return work;
        }
    }
    return NULL;
}

/* 0x800D3058 (0xD4): destroys a used work's g3d object and frees the slot. */
void push_g3d_wk(_g3d_work* work) {
    if (work->used != 0) {
        if (work->obj != NULL) {
            switch (work->kind) {
            case 1:
                work->obj->Destroy();
                break;
            case 2:
                work->obj->Destroy();
                break;
            case 3:
                work->obj->Destroy();
                break;
            case 4:
                work->obj->Destroy();
                break;
            case 5:
                work->obj->Destroy();
                break;
            }
        }
        work->used = 0;
        work->kind = 0;
        work->stage = 0;
        work->obj = NULL;
        work->size = 0;
        nw_res_manager->heap0_free = nw_res_heap_free_size(nw_res_manager->heap0);
    }
}

/* 0x800D3134 (0x74): frees every used work of one lifetime stage. */
void push_g3d_wk_stage(u8 stage) {
    _g3d_work* work = nw_res_manager->g3dWorks;
    s32 i;
    for (i = 0; i < 1024; work++, i++) {
        if (work->used == 1 && work->stage == stage) {
            push_g3d_wk(work);
        }
    }
}

/* 0x800D31A8 (0x8): frees the stage-lifetime works. */
void nw_res_stage_free(void) {
    push_g3d_wk_stage(1);
}

/* 0x800D31B0 (0x5C): frees every used work. */
void push_g3d_wk_all(void) {
    _g3d_work* work = nw_res_manager->g3dWorks;
    s32 i;
    for (i = 0; i < 1024; work++, i++) {
        if (work->used == 1) {
            push_g3d_wk(work);
        }
    }
}

/* 0x800D320C (0xC0): clears every work slot. */
void clear_g3d_wk(void) {
    _g3d_work* work = nw_res_manager->g3dWorks;
    s32 i;
    for (i = 0; i < 1024; i++, work++) {
        work->used = 0;
        work->kind = 0;
        work->stage = 0;
        work->obj = NULL;
        work->size = 0;
    }
}

/* 0x800D32CC (0xC8): sets the resource manager up in the system's block: g3d, the scene root, the camera and the
 * resource heaps. */
void nw_resource_init(void) {
    ResManager* m = nw_res_manager = (ResManager*)system_w.layout_0x94c;
    memset(m, 0, sizeof(ResManager));
    nw4r::g3d::G3dInit(true);
    g3d_state_set_render_mode((const GXRenderModeObj*)_f_bss);
    nw4r::g3d::ScnRoot::Construct(NULL, &m->rootSize, 1024, 1024, 70, 40);
    m->root = nw4r::g3d::ScnRoot::Construct(&main_heap_allocator, &m->rootSize, 1024, 1024, 70, 40);
    m->root->SetCurrentCamera(0);
    pRoot = (s32)m->root;
    camera_work_init();
    system_w.field_0x21 = 1;
    nw_res_heap_init();
}

/* 0x800D3394 (0x98): builds a simple model in a new work (the size is measured first). */
_g3d_work* pull_g3d_wk_mdl_simple(nw4r::g3d::ResMdl mdl) {
    u32 size;
    _g3d_work* work = pull_g3d_wk();
    if (work == NULL) {
        return work;
    }
    nw4r::g3d::ScnMdlSimple::Construct(NULL, &size, mdl, 1024);
    nw4r::g3d::ScnMdlSimple* obj = nw4r::g3d::ScnMdlSimple::Construct(&nw_res_manager->alloc0, NULL, mdl, 1024);
    work->kind = 1;
    work->obj = obj;
    work->size = size;
    return work;
}

/* 0x800D342C (0x94): builds a model with its buffer options in a new work. */
_g3d_work* nw_scn_mdl_create(nw4r::g3d::ResMdl mdl, u32 bufferOption, int numView) {
    _g3d_work* work = pull_g3d_wk();
    if (work == NULL) {
        return work;
    }
    nw4r::g3d::ScnMdl* obj = nw4r::g3d::ScnMdl::Construct(&nw_res_manager->alloc0, NULL, mdl, bufferOption, numView);
    work->kind = 2;
    work->obj = obj;
    work->size = (u32)obj;
    return work;
}

/* 0x800D34C0 (0x3C): builds a character-animation blend over the model. */
nw4r::g3d::AnmObjChrBlend* nw_anm_chr_blend_create(nw4r::g3d::ResMdl mdl, int numChildren) {
    return nw4r::g3d::AnmObjChrBlend::Construct(&nw_res_manager->alloc2, NULL, mdl, numChildren);
}

/* 0x800D34FC (0x48): builds a character animation over the model. */
nw4r::g3d::AnmObjChrRes* nw_anm_chr_res_create(nw4r::g3d::ResAnmChr res, nw4r::g3d::ResMdl mdl, bool bHasCache) {
    return nw4r::g3d::AnmObjChrRes::Construct(&nw_res_manager->alloc2, NULL, res, mdl, bHasCache);
}

/* 0x800D3544 (0x9C): builds a texture-SRT animation in a new work. */
_g3d_work* pull_g3d_wk_anm_tex_srt(nw4r::g3d::ResAnmTexSrt res, nw4r::g3d::ResMdl mdl, bool bHasCache) {
    _g3d_work* work = pull_g3d_wk();
    if (work == NULL) {
        return work;
    }
    nw4r::g3d::AnmObjTexSrtRes* obj =
        nw4r::g3d::AnmObjTexSrtRes::Construct(&nw_res_manager->alloc0, NULL, res, mdl, bHasCache);
    work->kind = 3;
    work->obj = obj;
    work->size = (u32)obj;
    return work;
}

/* 0x800D35E0 (0x48): builds a texture-SRT animation work of the stage lifetime. */
_g3d_work* pull_g3d_wk_anm_tex_srt_stage(nw4r::g3d::ResAnmTexSrt res, nw4r::g3d::ResMdl mdl, bool bHasCache) {
    _g3d_work* work = pull_g3d_wk_anm_tex_srt(res, mdl, bHasCache);
    if (work != NULL) {
        work->stage = 1;
    }
    return work;
}

/* 0x800D3628 (0x50): builds a texture-SRT animation work of the given lifetime. */
_g3d_work* pull_g3d_wk_anm_tex_srt_at(nw4r::g3d::ResAnmTexSrt res, nw4r::g3d::ResMdl mdl, bool bHasCache, u8 stage) {
    _g3d_work* work = pull_g3d_wk_anm_tex_srt(res, mdl, bHasCache);
    if (work != NULL) {
        work->stage = stage;
    }
    return work;
}

/* 0x800D3678 (0x84): builds a texture-SRT override in a new work. */
_g3d_work* pull_g3d_wk_anm_tex_srt_override(nw4r::g3d::ResMdl mdl, int numChildren) {
    _g3d_work* work = pull_g3d_wk();
    if (work == NULL) {
        return work;
    }
    nw4r::g3d::AnmObjTexSrtOverride* obj =
        nw4r::g3d::AnmObjTexSrtOverride::Construct(&nw_res_manager->alloc0, NULL, mdl, numChildren);
    work->kind = 4;
    work->obj = obj;
    work->size = (u32)obj;
    return work;
}

/* 0x800D36FC (0x3C): builds a texture-SRT override work of the stage lifetime. */
_g3d_work* pull_g3d_wk_anm_tex_srt_override_stage(nw4r::g3d::ResMdl mdl, int numChildren) {
    _g3d_work* work = pull_g3d_wk_anm_tex_srt_override(mdl, numChildren);
    if (work != NULL) {
        work->stage = 1;
    }
    return work;
}

/* 0x800D3738 (0x44): builds a texture-SRT override work of the given lifetime. */
_g3d_work* pull_g3d_wk_anm_tex_srt_override_at(nw4r::g3d::ResMdl mdl, int numChildren, u8 stage) {
    _g3d_work* work = pull_g3d_wk_anm_tex_srt_override(mdl, numChildren);
    if (work != NULL) {
        work->stage = stage;
    }
    return work;
}

/* 0x800D377C (0x78): builds a scene animation in a new work. */
_g3d_work* pull_g3d_wk_anm_scn(ResHandle res, bool bHasCache) {
    _g3d_work* work = pull_g3d_wk();
    nw4r::g3d::AnmScnRes* obj = nw4r::g3d::AnmScnRes::Construct(&nw_res_manager->alloc0, NULL, res, bHasCache);
    work->kind = 5;
    work->obj = obj;
    work->size = (u32)obj;
    return work;
}

/* 0x800D37F4 (0x3C): builds a scene animation work of the stage lifetime. */
_g3d_work* pull_g3d_wk_anm_scn_stage(ResHandle res, bool bHasCache) {
    _g3d_work* work = pull_g3d_wk_anm_scn(res, bHasCache);
    if (work != NULL) {
        work->stage = 1;
    }
    return work;
}

/* 0x800D3830 (0x44): builds a scene animation work of the given lifetime. */
_g3d_work* pull_g3d_wk_anm_scn_at(ResHandle res, bool bHasCache, u8 stage) {
    _g3d_work* work = pull_g3d_wk_anm_scn(res, bHasCache);
    if (work != NULL) {
        work->stage = stage;
    }
    return work;
}

/* The member-function pointer records to the file's `AnmTexSrt` category test that the two animation builders test
 * (a constant record, so the test always passes); one record per builder, as retail. */
static __ptmf nw_res_anm_work_num_has_tex_srt = {0, -1, (long)res_file_has_anm_tex_srt};
static __ptmf nw_res_anm_create_has_tex_srt = {0, -1, (long)res_file_has_anm_tex_srt};

/* 0x800D3874 (0x84): builds the texture-SRT override work sized by the file's texture-SRT animations; returns
 * their count. */
s32 nw_res_anm_work_num(EftResFileHandle res, EftResFileHandle mdl, s32 kind, _g3d_work** list) {
    _g3d_work* work = NULL;
    s32 num;
    if (__ptmf_test(&nw_res_anm_work_num_has_tex_srt)) {
        num = res_file_get_res_anm_tex_srt_num(&res);
        work = pull_g3d_wk_anm_tex_srt_override_at(reinterpret_cast<nw4r::g3d::ResMdl&>(mdl), num, kind);
    }
    *list = work;
    return num;
}

/* 0x800D38F8 (0x1CC): builds a looping texture-SRT animation work per animation of the file, attaches each to the
 * override in `first` and sets the override on the model; returns the override, or NULL. */
/* untyped: opaque handle - the override the callers hand on, as `nw_resource.h` declares it */
void* nw_res_anm_create(EftResFileHandle res, f32 strength, EftResFileHandle mdl, nw4r::g3d::ScnMdlSimple* scn, s32 kind,
                        _g3d_work* first, _g3d_work** rest) {
    if (first == NULL) {
        return NULL;
    }
    if (__ptmf_test(&nw_res_anm_create_has_tex_srt)) {
        u32 i;
        u32 num = res_file_get_res_anm_tex_srt_num(&res);
        bool bHasCache;
        nw4r::g3d::AnmObjTexSrtOverride* override = static_cast<nw4r::g3d::AnmObjTexSrtOverride*>(first->obj);
        bHasCache = (u8)kind == 5;
        for (i = 0; i < num; i++) {
            nw4r::g3d::ResAnmTexSrt anm;
            void* data = res_file_get_res_anm_tex_srt(&res, i);
            res_anm_tex_srt_copy(&anm, &data);
            if (override != NULL && res_anm_tex_srt_is_valid(&anm)) {
                _g3d_work* work = pull_g3d_wk_anm_tex_srt_at(anm, reinterpret_cast<nw4r::g3d::ResMdl&>(mdl),
                                                             bHasCache, kind);
                if (rest != NULL) {
                    *rest++ = work;
                }
                if (work != NULL) {
                    nw4r::g3d::AnmObjTexSrtRes* obj = static_cast<nw4r::g3d::AnmObjTexSrtRes*>(work->obj);
                    obj->Bind(reinterpret_cast<nw4r::g3d::ResMdl&>(mdl));
                    obj->SetUpdateRate(2.0f * strength);
                    obj->SetPlayPolicy(nw4r::g3d::PlayPolicy_Loop);
                    override->Attach(i, obj);
                }
            }
        }
        scn->SetAnmObj(override, nw4r::g3d::ScnMdlSimple::ANMOBJTYPE_NOT_SPECIFIED);
        return override;
    }
    return NULL;
}

/* 0x800D3AC4 (0x8): sets the play policy of the animation's frame counter. */
inline void nw4r::g3d::AnmObjTexSrtRes::SetPlayPolicy(PlayPolicyFunc policy) {
    FrameCtrl& frame = *this;
    frame_ctrl_set_play_policy(&frame, policy);
}

/* The RVL address windows nw4r's pointer assert accepts. */
#define NW_RES_VALID_PTR(p)                                                                                         \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||                              \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||                              \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||                              \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

/* 0x800D3ACC (0x124): asserts the play policy is a valid pointer and stores it in the frame counter. */
inline void frame_ctrl_set_play_policy(nw4r::g3d::FrameCtrl* self, nw4r::g3d::PlayPolicyFunc policy) {
    NW_RES_VALID_PTR(policy) ? (void)0
                             : nw4r::db::Panic("g3d_anmobj.h", 95, "NW4R:Pointer Error\nfunc(=%p) is not valid pointer.",
                                               policy);
    self->mpPlayPolicy = policy;
}

/* 0x800D3BF0 (0x1C): the scene root, or NULL before the manager exists. */
nw4r::g3d::ScnRoot* nw_res_scn_root_get(void) {
    if (nw_res_manager == NULL) {
        return NULL;
    }
    return nw_res_manager->root;
}

/* 0x800D3C0C (0x20): the effect system, or NULL before the manager exists. */
EfSys* nw_res_ef_system_get(void) {
    if (nw_res_manager == NULL) {
        return NULL;
    }
    return nw_res_manager->mEfSystem;
}

/* 0x800D3C2C (0x20): the effect group count, or 0 before the manager exists. */
u16 nw_res_ef_num_group_get(void) {
    if (nw_res_manager == NULL) {
        return 0;
    }
    return nw_res_manager->mEfNumGroup;
}

/* ------------------------------------------------------------------------------------------------------------
 * The effect memory manager (`memorymanagertmp.h`): nw4r::ef's abstract `MemoryManagerBase`, the per-kind object
 * pool and the game's manager, which carves the four pools and its own free-list heap out of one block.  Every
 * member is inline, so each is emitted after the first function that references it.
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ef {

/* The effect system's allocator interface: one Alloc/Free/count quintet per pooled kind, then the heap. */
class MemoryManagerBase {
public:
    virtual ~MemoryManagerBase() {}
    virtual void GarbageCollection() = 0;
    virtual Effect* AllocEffect() = 0;
    /* untyped: caller-owned payload - the effect being returned to its pool */
    virtual void FreeEffect(void* object) = 0;
    virtual u32 GetNumAllocEffect() const = 0;
    virtual u32 GetNumActiveEffect() const = 0;
    virtual u32 GetNumFreeEffect() const = 0;
    virtual Emitter* AllocEmitter() = 0;
    /* untyped: caller-owned payload - the emitter being returned to its pool */
    virtual void FreeEmitter(void* object) = 0;
    virtual u32 GetNumAllocEmitter() const = 0;
    virtual u32 GetNumActiveEmitter() const = 0;
    virtual u32 GetNumFreeEmitter() const = 0;
    virtual ParticleManager* AllocParticleManager() = 0;
    /* untyped: caller-owned payload - the particle manager being returned to its pool */
    virtual void FreeParticleManager(void* object) = 0;
    virtual u32 GetNumAllocParticleManager() const = 0;
    virtual u32 GetNumActiveParticleManager() const = 0;
    virtual u32 GetNumFreeParticleManager() const = 0;
    virtual Particle* AllocParticle() = 0;
    /* untyped: caller-owned payload - the particle being returned to its pool */
    virtual void FreeParticle(void* object) = 0;
    virtual u32 GetNumAllocParticle() const = 0;
    virtual u32 GetNumActiveParticle() const = 0;
    virtual u32 GetNumFreeParticle() const = 0;
    /* untyped: byte range - a raw block of the manager's heap */
    virtual void* AllocHeap(u32 size) = 0;
    /* untyped: byte range - a block AllocHeap returned */
    virtual void FreeHeap(void* block) = 0;
}; /* size: 0x04 */

}  // namespace ef
}  // namespace nw4r

/* Removes a retired object from its owner's retire list. */
/* untyped: caller-owned payload - the list holds objects of any pooled kind */
inline void nw_res_activity_release(EfSysActivityList* list, void* object) {
    nw4r::ut::List_Remove(reinterpret_cast<nw4r::ut::List*>(&list->mRetireList), object);
}

/* Removes a retired particle from its particle manager's retire list. */
inline void nw_res_pm_release_particle(nw4r::ef::ParticleManager* pm, nw4r::ef::Particle* particle) {
    nw_res_activity_release(&pm->mParticles, particle);
}

/* The pool's lists (GUESS name): the objects' array, the free and the active lists and the element size.  The
 * pool's vtable pointer follows them, which is where MWCC puts it behind a non-polymorphic base. */
struct NwResObjectPoolLists {
    /* untyped: the objects' array, of the pool's kind */
    /* +0x00 */ void* mObjects;
    /* +0x04 */ nw4r::ut::List mFreeList;
    /* +0x10 */ nw4r::ut::List mActiveList;
    /* +0x1C */ u32 mObjectSize;
}; /* size: 0x20 */

/* One kind's object pool (GUESS name): the lists above and a vtable whose one slot stamps an object as it is
 * handed out. */
template <class T>
class NwResObjectPool : public NwResObjectPoolLists {
public:
    NwResObjectPool(u32 num, T* objects) {
        u32 i;
        nw4r::ut::List_Init(&mFreeList, 0);
        nw4r::ut::List_Init(&mActiveList, 0);
        mObjectSize = sizeof(T);
        mObjects = objects;
        for (i = 0; i < num; i++) {
            objects->mPoolId = i;
            nw4r::ut::List_Append(&mFreeList, objects);
            objects++;
        }
    }

    virtual void InitObject(T* object) {
        object->mPoolId += 0x10000;
    }

    T* Alloc() {
        T* object = (T*)ef_list_get_first(&mFreeList);
        if (object == NULL) {
            return NULL;
        }
        InitObject(object);
        nw4r::ut::List_Remove(&mFreeList, object);
        nw4r::ut::List_Append(&mActiveList, object);
        return object;
    }

    void Free(T* object) {
        nw4r::ut::List_Remove(&mActiveList, object);
        nw4r::ut::List_Append(&mFreeList, object);
    }

    u32 GetNumAlloc() const {
        return GetNumActive() + GetNumFree();
    }
    u32 GetNumFree() const {
        return mFreeList.numObjects;
    }
    u32 GetNumActive() const {
        return mActiveList.numObjects;
    }

    void GarbageCollection();

    /* +0x20: the vtable pointer */
}; /* size: 0x24 */

/* The retired, unreferenced effects go back to the pool (and leave their group's retire list). */
template <>
inline void NwResObjectPool<nw4r::ef::Effect>::GarbageCollection() {
    nw4r::ef::Effect* object = (nw4r::ef::Effect*)ef_list_get_first(&mActiveList);
    if (object != NULL) {
        nw4r::ef::Effect* next = (nw4r::ef::Effect*)nw4r::ut::List_GetNext(&mActiveList, object);
        while (object != NULL) {
            if (ef_get_life_status(object) == 3 && object->GetRefCount() == 0) {
                nw_res_activity_release(&object->mManagerES->mActivityList[object->mGroupID], object);
                nw4r::ut::List_Remove(&mActiveList, object);
                nw4r::ut::List_Append(&mFreeList, object);
            }
            object = next;
            next = (nw4r::ef::Effect*)nw4r::ut::List_GetNext(&mActiveList, next);
        }
    }
}

/* The retired, unreferenced emitters go back to the pool (and leave their effect's retire list). */
template <>
inline void NwResObjectPool<nw4r::ef::Emitter>::GarbageCollection() {
    nw4r::ef::Emitter* object = (nw4r::ef::Emitter*)ef_list_get_first(&mActiveList);
    if (object != NULL) {
        nw4r::ef::Emitter* next = (nw4r::ef::Emitter*)nw4r::ut::List_GetNext(&mActiveList, object);
        while (object != NULL) {
            if (ef_get_life_status(object) == 3 && object->GetRefCount() == 0) {
                nw_res_activity_release(&object->mEffect->mEmitters, object);
                nw4r::ut::List_Remove(&mActiveList, object);
                nw4r::ut::List_Append(&mFreeList, object);
            }
            object = next;
            next = (nw4r::ef::Emitter*)nw4r::ut::List_GetNext(&mActiveList, next);
        }
    }
}

/* The retired, unreferenced particle managers go back to the pool (and leave their emitter's retire list). */
template <>
inline void NwResObjectPool<nw4r::ef::ParticleManager>::GarbageCollection() {
    nw4r::ef::ParticleManager* object = (nw4r::ef::ParticleManager*)ef_list_get_first(&mActiveList);
    if (object != NULL) {
        nw4r::ef::ParticleManager* next = (nw4r::ef::ParticleManager*)nw4r::ut::List_GetNext(&mActiveList, object);
        while (object != NULL) {
            if (ef_get_life_status(object) == 3 && object->GetRefCount() == 0) {
                nw_res_activity_release(&object->mEmitter->mParticleManagers, object);
                nw4r::ut::List_Remove(&mActiveList, object);
                nw4r::ut::List_Append(&mFreeList, object);
            }
            object = next;
            next = (nw4r::ef::ParticleManager*)nw4r::ut::List_GetNext(&mActiveList, next);
        }
    }
}

/* The particle pool is collected by the manager itself (its particles leave their manager's list first). */
template <>
void NwResObjectPool<nw4r::ef::Particle>::GarbageCollection();

/* One block of the manager's heap: the free/used list links, the address-order neighbours, the payload size and
 * the in-use flag; the payload starts one rounded header past it. */
struct NwResHeapBlock {
    /* +0x00 */ NwResHeapBlock* prev;
    /* +0x04 */ NwResHeapBlock* next;
    /* +0x08 */ NwResHeapBlock* physPrev;
    /* +0x0C */ NwResHeapBlock* physNext;
    /* +0x10 */ u32 size;
    /* +0x14 */ u8 used;
}; /* size: 0x18 */

/* The game's effect memory manager (GUESS name; nw4r's `MemoryManagerTmp` shape): the four pools and a first-fit
 * free-list heap over the block it is given. */
class NwResMemoryManager : public nw4r::ef::MemoryManagerBase {
public:
    static u32 RoundUp(u32 size, s32 align) {
        return (align + size - 1) & ~(align - 1);
    }

    /* The heap the manager needs for the given pool sizes and effect groups. */
    static u32 GetRequiredHeapSize(u16 numEffect, u16 numEmitter, u16 numParticleManager, u16 numParticle,
                                   u32 numGroup) {
        return 544 + RoundUp(numEffect * sizeof(nw4r::ef::Effect) + 32, 32) +
               RoundUp(numEmitter * sizeof(nw4r::ef::Emitter) + 32, 32) +
               RoundUp(numParticleManager * sizeof(nw4r::ef::ParticleManager) + 32, 32) +
               RoundUp(numParticle * sizeof(nw4r::ef::Particle) + 32, 32) +
               RoundUp(numGroup * sizeof(EfSysActivityList) + 32, 32);
    }

    NwResMemoryManager(u32 startAddress, u32 size, s32 numEffect, s32 numEmitter, s32 numParticleManager,
                       s32 numParticle) {
        if (startAddress & 31) {
            nw4r::db::Panic("memorymanagertmp.h", 267,
                            "NW4R:Alignment Error(0x%x)\nstartAddress must be aligned to 32 bytes boundary.",
                            startAddress);
        }
        u32 headerSize = RoundUp(sizeof(NwResHeapBlock), 32);
        mNumEffect = numEffect;
        mNumEmitter = numEmitter;
        mNumParticleManager = numParticleManager;
        mNumParticle = numParticle;
        mEffectPool = NULL;
        mEmitterPool = NULL;
        mParticleManagerPool = NULL;
        mParticlePool = NULL;
        mUsedList = NULL;
        mFreeList = NULL;
        mField_0x34 = NULL;
        mFirstBlock = NULL;
        mHeapEndAddr = startAddress + size;
        mHeapStartAddr = RoundUp(startAddress, 32);
        NW4R_ASSERT(mHeapStartAddr < mHeapEndAddr && mHeapEndAddr - mHeapStartAddr > headerSize,
                    "memorymanagertmp.h", 291,
                    "NW4R:Failed assertion mHeapStartAddr < mHeapEndAddr && reinterpret_cast<u8*>(mHeapEndAddr) - "
                    "reinterpret_cast<u8*>(mHeapStartAddr) > headerSize");
        mFreeList = (NwResHeapBlock*)mHeapStartAddr;
        mFreeList->prev = NULL;
        mFreeList->next = NULL;
        mFreeList->physPrev = NULL;
        mFreeList->physNext = NULL;
        mFreeList->used = 0;
        mFreeList->size = mHeapEndAddr - mHeapStartAddr - headerSize;
        mFirstBlock = mFreeList;
        mUsedList = NULL;

        nw4r::ef::Effect* effects = NULL;
        if (mNumEffect > 0) {
            effects = new (AllocHeap(mNumEffect * sizeof(nw4r::ef::Effect) + 32)) nw4r::ef::Effect[mNumEffect];
        }
        mEffectPool = new (AllocHeap(sizeof(NwResObjectPool<nw4r::ef::Effect>)))
            NwResObjectPool<nw4r::ef::Effect>(mNumEffect, effects);

        nw4r::ef::Emitter* emitters = NULL;
        if (mNumEmitter > 0) {
            emitters = new (AllocHeap(mNumEmitter * sizeof(nw4r::ef::Emitter) + 32)) nw4r::ef::Emitter[mNumEmitter];
        }
        mEmitterPool = new (AllocHeap(sizeof(NwResObjectPool<nw4r::ef::Emitter>)))
            NwResObjectPool<nw4r::ef::Emitter>(mNumEmitter, emitters);

        /* the owner's constructor and destructor are C functions, so the new[] expression is spelled out */
        nw4r::ef::ParticleManager* managers = NULL;
        if (mNumParticleManager > 0) {
            void* block = AllocHeap(mNumParticleManager * sizeof(nw4r::ef::ParticleManager) + 32);
            managers = (nw4r::ef::ParticleManager*)operator new[](
                mNumParticleManager * sizeof(nw4r::ef::ParticleManager) + 16, block);
            if (managers != NULL) {
                try {
                    managers = (nw4r::ef::ParticleManager*)__construct_new_array(
                        managers, (ConstructorDestructor)ef_particle_manager_ctor,
                        (ConstructorDestructor)ef_particle_manager_delete, sizeof(nw4r::ef::ParticleManager),
                        mNumParticleManager);
                } catch (...) {
                    operator delete[](managers, block);
                    throw;
                }
            }
        }
        mParticleManagerPool = new (AllocHeap(sizeof(NwResObjectPool<nw4r::ef::ParticleManager>)))
            NwResObjectPool<nw4r::ef::ParticleManager>(mNumParticleManager, managers);

        nw4r::ef::Particle* particles = NULL;
        if (mNumParticle > 0) {
            particles = new (AllocHeap(mNumParticle * sizeof(nw4r::ef::Particle) + 32))
                nw4r::ef::Particle[mNumParticle];
        }
        mParticlePool = new (AllocHeap(sizeof(NwResObjectPool<nw4r::ef::Particle>)))
            NwResObjectPool<nw4r::ef::Particle>(mNumParticle, particles);
    }

    virtual ~NwResMemoryManager() {}

    virtual void GarbageCollection() {
        nw4r::ef::ParticleManager* pm;
        nw4r::ef::ParticleManager* next;
        for (pm = (nw4r::ef::ParticleManager*)ef_list_get_first(&mParticleManagerPool->mActiveList);
             pm != NULL; pm = next) {
            next = (nw4r::ef::ParticleManager*)nw4r::ut::List_GetNext(&mParticleManagerPool->mActiveList, pm);
            nw4r::ef::Particle* pp;
            nw4r::ef::Particle* nextParticle;
            for (pp = (nw4r::ef::Particle*)pm->mParticles.mRetireList.head; pp != NULL; pp = nextParticle) {
                /* the next element through the list's link offset */
                nextParticle = (nw4r::ef::Particle*)reinterpret_cast<nw4r::ut::Link*>(
                                   (u8*)pp + pm->mParticles.mRetireList.linkOffset)->nextObject;
                NW4R_ASSERT(pp->GetRefCount() == 0, "memorymanagertmp.h", 378,
                            "NW4R:Failed assertion pp->GetRefCount() == 0");
                NW4R_ASSERT(ef_get_life_status(pp->mParticleManager) == 1 ||
                                ef_get_life_status(pp->mParticleManager) == 2 ||
                                ef_get_life_status(pp->mParticleManager) == 3,
                            "memorymanagertmp.h", 381,
                            "NW4R:Failed assertion pp->mParticleManager->GetLifeStatus() == "
                            "::nw4r::ef::ReferencedObject::NW4R_EF_LS_ACTIVE || "
                            "pp->mParticleManager->GetLifeStatus() == "
                            "::nw4r::ef::ReferencedObject::NW4R_EF_LS_WAIT || "
                            "pp->mParticleManager->GetLifeStatus() == "
                            "::nw4r::ef::ReferencedObject::NW4R_EF_LS_CLOSING");
                nw_res_pm_release_particle(pp->mParticleManager, pp);
                nw4r::ut::List_Remove(&mParticlePool->mActiveList, pp);
                nw4r::ut::List_Append(&mParticlePool->mFreeList, pp);
            }
        }
        if (mParticleManagerPool != NULL) {
            mParticleManagerPool->GarbageCollection();
        }
        if (mEmitterPool != NULL) {
            mEmitterPool->GarbageCollection();
        }
        if (mEffectPool != NULL) {
            mEffectPool->GarbageCollection();
        }
    }

    virtual nw4r::ef::Effect* AllocEffect() {
        if (mEffectPool == NULL) {
            return NULL;
        }
        return mEffectPool->Alloc();
    }
    /* untyped: caller-owned payload - the effect being returned to its pool */
    virtual void FreeEffect(void* object) {
        if (mEffectPool != NULL) {
            mEffectPool->Free((nw4r::ef::Effect*)object);
        }
    }
    virtual u32 GetNumAllocEffect() const {
        if (mEffectPool == NULL) {
            return 0;
        }
        return mEffectPool->GetNumAlloc();
    }
    virtual u32 GetNumActiveEffect() const {
        if (mEffectPool == NULL) {
            return 0;
        }
        return mEffectPool->GetNumActive();
    }
    virtual u32 GetNumFreeEffect() const {
        if (mEffectPool == NULL) {
            return 0;
        }
        return mEffectPool->GetNumFree();
    }

    virtual nw4r::ef::Emitter* AllocEmitter() {
        if (mEmitterPool == NULL) {
            return NULL;
        }
        return mEmitterPool->Alloc();
    }
    /* untyped: caller-owned payload - the emitter being returned to its pool */
    virtual void FreeEmitter(void* object) {
        if (mEmitterPool != NULL) {
            mEmitterPool->Free((nw4r::ef::Emitter*)object);
        }
    }
    virtual u32 GetNumAllocEmitter() const {
        if (mEmitterPool == NULL) {
            return 0;
        }
        return mEmitterPool->GetNumAlloc();
    }
    virtual u32 GetNumActiveEmitter() const {
        if (mEmitterPool == NULL) {
            return 0;
        }
        return mEmitterPool->GetNumActive();
    }
    virtual u32 GetNumFreeEmitter() const {
        if (mEmitterPool == NULL) {
            return 0;
        }
        return mEmitterPool->GetNumFree();
    }

    virtual nw4r::ef::ParticleManager* AllocParticleManager() {
        if (mParticleManagerPool == NULL) {
            return NULL;
        }
        return mParticleManagerPool->Alloc();
    }
    /* untyped: caller-owned payload - the particle manager being returned to its pool */
    virtual void FreeParticleManager(void* object) {
        if (mParticleManagerPool != NULL) {
            mParticleManagerPool->Free((nw4r::ef::ParticleManager*)object);
        }
    }
    virtual u32 GetNumAllocParticleManager() const {
        if (mParticleManagerPool == NULL) {
            return 0;
        }
        return mParticleManagerPool->GetNumAlloc();
    }
    virtual u32 GetNumActiveParticleManager() const {
        if (mParticleManagerPool == NULL) {
            return 0;
        }
        return mParticleManagerPool->GetNumActive();
    }
    virtual u32 GetNumFreeParticleManager() const {
        if (mParticleManagerPool == NULL) {
            return 0;
        }
        return mParticleManagerPool->GetNumFree();
    }

    virtual nw4r::ef::Particle* AllocParticle() {
        if (mParticlePool == NULL) {
            return NULL;
        }
        return mParticlePool->Alloc();
    }
    /* untyped: caller-owned payload - the particle being returned to its pool */
    virtual void FreeParticle(void* object) {
        if (mParticlePool != NULL) {
            mParticlePool->Free((nw4r::ef::Particle*)object);
        }
    }
    virtual u32 GetNumAllocParticle() const {
        if (mParticlePool == NULL) {
            return 0;
        }
        return mParticlePool->GetNumAlloc();
    }
    virtual u32 GetNumActiveParticle() const {
        if (mParticlePool == NULL) {
            return 0;
        }
        return mParticlePool->GetNumActive();
    }
    virtual u32 GetNumFreeParticle() const {
        if (mParticlePool == NULL) {
            return 0;
        }
        return mParticlePool->GetNumFree();
    }

    /* First fit: a block too small to split is taken whole, otherwise its tail stays free. */
    /* untyped: byte range - a raw block of the manager's heap */
    virtual void* AllocHeap(u32 size) {
        u32 headerSize = RoundUp(sizeof(NwResHeapBlock), 32);
        size = RoundUp(size, 32);
        for (NwResHeapBlock* block = mFreeList; block != NULL; block = block->next) {
            if (block->size >= size) {
                if (block->size < size + headerSize + 32) {
                    if (block->prev != NULL) {
                        block->prev->next = block->next;
                    } else {
                        mFreeList = block->next;
                    }
                    if (block->next != NULL) {
                        block->next->prev = block->prev;
                    }
                    if (mUsedList != NULL) {
                        mUsedList->prev = block;
                    }
                    block->next = mUsedList;
                    block->prev = NULL;
                    block->used = 1;
                    mUsedList = block;
                } else {
                    NwResHeapBlock* rest = (NwResHeapBlock*)((u8*)block + headerSize + size);
                    rest->prev = block->prev;
                    rest->next = block->next;
                    rest->size = block->size - size - headerSize;
                    rest->used = 0;
                    rest->physNext = block->physNext;
                    if (block->physNext != NULL) {
                        block->physNext->physPrev = rest;
                    }
                    rest->physPrev = block;
                    block->physNext = rest;
                    if (block->prev != NULL) {
                        block->prev->next = rest;
                    } else {
                        mFreeList = rest;
                    }
                    if (block->next != NULL) {
                        rest->next->prev = rest;
                    }
                    if (mUsedList != NULL) {
                        mUsedList->prev = block;
                    }
                    block->next = mUsedList;
                    block->prev = NULL;
                    block->size = size;
                    block->used = 1;
                    mUsedList = block;
                }
                return (u8*)block + headerSize;
            }
        }
        NW4R_ASSERT(false, "memorymanagertmp.h", 580, "NW4R:Failed assertion false");
        return NULL;
    }

    /* Returns a block to the free list, merging it with free address-order neighbours. */
    /* untyped: byte range - a block AllocHeap returned */
    virtual void FreeHeap(void* p) {
        u32 headerSize = RoundUp(sizeof(NwResHeapBlock), 32);
        NwResHeapBlock* block = (NwResHeapBlock*)((u8*)p - headerSize);
        if (block->prev != NULL) {
            block->prev->next = block->next;
        } else {
            mUsedList = block->next;
        }
        if (block->next != NULL) {
            block->next->prev = block->prev;
        }
        NwResHeapBlock* before = NULL;
        if (block->physPrev != NULL && block->physPrev->used == 0) {
            before = block->physPrev;
        }
        NwResHeapBlock* after = NULL;
        if (block->physNext != NULL && block->physNext->used == 0) {
            after = block->physNext;
        }
        if (before != NULL) {
            if (after != NULL) {
                if (after->prev != NULL) {
                    after->prev->next = after->next;
                } else {
                    mFreeList = after->next;
                }
                if (after->next != NULL) {
                    after->next->prev = after->prev;
                }
                before->size += headerSize + block->size + headerSize + after->size;
                before->physNext = after->physNext;
                if (after->physNext != NULL) {
                    after->physNext->physPrev = before;
                }
            } else {
                before->size += headerSize + block->size;
                before->physNext = block->physNext;
                if (block->physNext != NULL) {
                    block->physNext->physPrev = before;
                }
            }
        } else if (after != NULL) {
            if (after->prev != NULL) {
                after->prev->next = block;
            } else {
                mFreeList = block;
            }
            if (after->next != NULL) {
                after->next->prev = block;
            }
            block->prev = after->prev;
            block->next = after->next;
            block->size += headerSize + after->size;
            block->used = 0;
            block->physNext = after->physNext;
            if (after->physNext != NULL) {
                after->physNext->physPrev = before;
            }
        } else {
            if (mFreeList != NULL) {
                mFreeList->prev = block;
            }
            block->next = mFreeList;
            block->prev = NULL;
            block->used = 0;
            mFreeList = block;
        }
    }

    /* +0x04 */ s32 mNumEffect;
    /* +0x08 */ NwResObjectPool<nw4r::ef::Effect>* mEffectPool;
    /* +0x0C */ s32 mNumEmitter;
    /* +0x10 */ NwResObjectPool<nw4r::ef::Emitter>* mEmitterPool;
    /* +0x14 */ s32 mNumParticleManager;
    /* +0x18 */ NwResObjectPool<nw4r::ef::ParticleManager>* mParticleManagerPool;
    /* +0x1C */ s32 mNumParticle;
    /* +0x20 */ NwResObjectPool<nw4r::ef::Particle>* mParticlePool;
    /* +0x24 */ u32 mHeapStartAddr;
    /* +0x28 */ u32 mHeapEndAddr;
    /* +0x2C */ NwResHeapBlock* mUsedList;
    /* +0x30 */ NwResHeapBlock* mFreeList;
    /* +0x34 */ NwResHeapBlock* mField_0x34; /* cleared by the constructor, never read here */
    /* +0x38 */ NwResHeapBlock* mFirstBlock;
}; /* size: 0x3C */

/* Binds the effect system to its memory manager and sizes its group table (nw4r's inline `EffectSystem`
 * initialiser); without a manager the group table is cleared. */
inline void nw_res_ef_system_bind(EfSys* system, NwResMemoryManager* manager, u32 numGroup) {
    system->mMemoryManager = reinterpret_cast<EfSysMemoryManager*>(manager);
    if (manager != NULL) {
        ef_system_initialize(system, numGroup);
    } else {
        system->mMaxGroupID = 0;
        system->mActivityList = NULL;
    }
}


/* The effect pool sizes the manager is built with (`.sdata` 0x80791370). */
NwResEfPoolSizes nw_res_ef_pool_sizes = {0x100, 0x200, 0x200, 0x400};

/* 0x800D3C4C (0xFC): builds the effect memory manager over the fourth heap, binds the effect system to it and
 * allocates the draw state. */
void nw_res_effect_init(void) {
    ResManager* m = nw_res_manager;
    m->mEfPoolSizes.numEffect = nw_res_ef_pool_sizes.numEffect;
    m->mEfPoolSizes.numEmitter = nw_res_ef_pool_sizes.numEmitter;
    m->mEfPoolSizes.numParticleManager = nw_res_ef_pool_sizes.numParticleManager;
    m->mEfPoolSizes.numParticle = nw_res_ef_pool_sizes.numParticle;
    m->mEfNumGroup = 2;
    u32 size = NwResMemoryManager::GetRequiredHeapSize(m->mEfPoolSizes.numEffect, m->mEfPoolSizes.numEmitter,
                                                       m->mEfPoolSizes.numParticleManager,
                                                       m->mEfPoolSizes.numParticle, m->mEfNumGroup);
    m->mEfMemoryManager = new NwResMemoryManager(m->heap3_start, size, m->mEfPoolSizes.numEffect,
                                                 m->mEfPoolSizes.numEmitter, m->mEfPoolSizes.numParticleManager,
                                                 m->mEfPoolSizes.numParticle);
    m->mEfSystem = ef_system_get_instance();
    nw_res_ef_system_bind(m->mEfSystem, m->mEfMemoryManager, m->mEfNumGroup);
    m->mEfDrawInfo = new nw4r::ef::DrawInfo;
}

/* 0x800D4848 (0x64): walks the effect groups the effect system was sized for. */
void nw_res_ef_retire_all(void) {
    ResManager* m = nw_res_manager;
    u32 i;
    if (m->mEfSystem != 0) {
        for (i = 0; i < m->mEfNumGroup; i++) {
            ef_system_retire_group(m->mEfSystem, i);
        }
    }
}

/* 0x800D48AC (0x5C): hands the current view matrix to the effect draw state. */
void nw_res_view_mtx_apply(void) {
    ResManager* m = nw_res_manager;
    if (m->mEfSystem != 0 && m->mEfDrawInfo != 0) {
        nw_res_mtx_copy(m->mEfDrawInfo, &get_current_view_mtx());
    }
}

/* 0x800D490C (0x168): sizes the four resource heaps and resets the two tables. */
void nw_res_heap_init(void) {
    ResManager* m = nw_res_manager;
    s32 i;
    m->heap0_start = 0x81276600;
    m->heap0_size = 0x00280000;
    m->heap2_start = 0x80FF6600;
    m->heap2_size = 0x00280000;
    m->heap3_start = 0x814F6600;
    m->heap3_size = 0x00140000;
    m->heap1_start = 0x90F48000;
    m->heap1_size = 0x01B59C00;
    m->heap0 = nw_res_create_heap(m->heap0_start, m->heap0_size);
    MEMInitAllocatorForExpHeap(&m->alloc0, m->heap0, 32);
    m->heap1 = nw_res_create_heap(m->heap1_start, m->heap1_size);
    MEMInitAllocatorForExpHeap(&m->alloc1, m->heap1, 32);
    m->heap2 = nw_res_create_heap(m->heap2_start, m->heap2_size);
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
    m->heap1_free = nw_res_heap_free_size(m->heap1);
    m->heap0_free = nw_res_heap_free_size(m->heap0);
    m->heap2_free = nw_res_heap_free_size(m->heap2);
    clear_g3d_wk();
    nw_res_effect_init();
}

/* 0x800D4A7C (0x148): rebuilds the resource heaps and resets the two tables. */
void nw_res_heap_reset(void) {
    ResManager* m = nw_res_manager;
    s32 i;
    if (m->heap0 != 0) {
        MEMDestroyExpHeap(m->heap0);
    }
    if (m->heap1 != 0) {
        MEMDestroyExpHeap(m->heap1);
    }
    if (m->heap2 != 0) {
        MEMDestroyExpHeap(m->heap2);
    }
    m->heap0 = nw_res_create_heap(m->heap0_start, m->heap0_size);
    m->heap1 = nw_res_create_heap(m->heap1_start, m->heap1_size);
    m->heap2 = nw_res_create_heap(m->heap2_start, m->heap2_size);
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
    m->heap1_free = nw_res_heap_free_size(m->heap1);
    m->heap0_free = nw_res_heap_free_size(m->heap0);
    m->heap2_free = nw_res_heap_free_size(m->heap2);
    clear_g3d_wk();
    nw_res_ef_retire_all();
}

/* 0x800D4BC4 (0x50): allocates from the resource-memory allocator and caches the free total. */
void* RESmemAlloc(u32 size) {
    ResManager* m = nw_res_manager;
    void* p = MEMAllocFromAllocator(&m->alloc1, size);
    m->heap1_free = nw_res_heap_free_size(m->heap1);
    return p;
}

/* 0x800D4C14 (0x50): allocates from the second resource allocator and caches the free total. */
void* nw_res_mem1_alloc(u32 size) {
    ResManager* m = nw_res_manager;
    void* p = MEMAllocFromAllocator(&m->alloc0, size);
    m->heap0_free = nw_res_heap_free_size(m->heap0);
    return p;
}

/* 0x800D4C64 (0x40): frees into the resource-memory allocator and caches the free total. */
void RESmemFree(void* p) {
    ResManager* m = nw_res_manager;
    MEMFreeToAllocator(&m->alloc1, p);
    m->heap1_free = nw_res_heap_free_size(m->heap1);
}

/* 0x800D4CA4 (0x40): frees into the second resource allocator and caches the free total. */
void nw_res_mem1_free(void* p) {
    ResManager* m = nw_res_manager;
    MEMFreeToAllocator(&m->alloc0, p);
    m->heap0_free = nw_res_heap_free_size(m->heap0);
}

/* 0x800D4CE4 (0xB8): finds a resource-memory slot by basename and takes a reference. */
s32 res_mem_index_ref(const char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    char* p = str_rfind(path, 47);
    path = p == 0 ? path : p + 1;
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data != 0 && m->resMem[i].size != 0 && strcmp(m->resMem[i].name, path) == 0) {
            m->resMem[i].refcount++;
            return i;
        }
    }
    return -1;
}

/* 0x800D4D9C (0x3C): the name of a live resource-memory slot, or NULL. */
char* res_mem_name_get(s32 index) {
    ResMemEntry* e = &nw_res_manager->resMem[index];
    if (e->data == 0) {
        return 0;
    }
    if (e->size == 0) {
        return 0;
    }
    return e->name;
}

/* 0x800D4DD8 (0xB0): finds a resource-memory slot by basename, without taking a reference. */
s32 res_mem_index_get(const char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    char* p = str_rfind(path, 47);
    path = p == 0 ? path : p + 1;
    for (i = 0; i < 1024; i++) {
        if (m->resMem[i].data != 0 && m->resMem[i].size != 0 && strcmp(m->resMem[i].name, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* 0x800D4E88 (0x1E4): the refcounted resource-memory pull: reuses a live slot, else allocates one. */
s32 pull_res_mem(char* path, u32 size, s32 mode) {
    ResManager* m = nw_res_manager;
    u32 asize;
    s32 i;
    char* name;
    if (size == 0) {
        return -1;
    }
    if (path != 0) {
        char* p = str_rfind(path, 47);
        name = p == 0 ? path : p + 1;
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
    asize = (size + 31) & ~31;
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

/* 0x800D506C (0x98): drops one reference on a resource-memory slot; frees it at zero. */
s32 push_res_mem(s32 index) {
    ResManager* m = nw_res_manager;
    if (m->resMem[index].data == 0) {
        return -1;
    }
    m->resMem[index].refcount = m->resMem[index].refcount - 1;
    if (m->resMem[index].refcount <= 0) {
        RESmemFree(m->resMem[index].data);
        m->resMem[index].data = 0;
        m->resMem[index].size = 0;
        m->resMem[index].name[0] = 0;
        m->resMem[index].refcount = 0;
        return 0;
    }
    return -1;
}

/* 0x800D5104 (0x34): the data pointer of a resource-memory slot, or NULL. */
void* getResMemAdrs(s32 index) {
    if (index < 0) {
        return 0;
    }
    if (index >= 1024) {
        return 0;
    }
    return nw_res_manager->resMem[index].data;
}

/* 0x800D5138 (0x34): the size of a resource-memory slot, or 0. */
u32 nw_res_mem_size(s32 index) {
    if (index < 0) {
        return 0;
    }
    if (index >= 1024) {
        return 0;
    }
    return nw_res_manager->resMem[index].size;
}

/* 0x800D516C (0xE4): the first resource-memory slot holding `data`, or -1. */
s32 res_mem_index_of_data(void* data) {
    s32 i;
    ResMemEntry* e = nw_res_manager->resMem;
    for (i = 0; i < 1024; e++, i++) {
        if (e->data == data) {
            return i;
        }
    }
    return -1;
}

/* 0x800D5250 (0x58): releases every live resource-memory slot. */
void res_mem_release_all(void) {
    ResManager* m;
    s32 i = 0;
    m = nw_res_manager;
    for (; i < 1024; i++) {
        if (m->resMem[i].data != 0) {
            push_res_mem(i);
        }
    }
}

/* 0x800D52A8 (0xB8): finds a resource-table entry by basename; NULL when absent. */
ResEntry* ckResourceName(char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    ResEntry* e = m->resTable;
    char* p = str_rfind(path, 47);
    path = p == 0 ? path : p + 1;
    for (i = 0; i < 1024; i++, e++) {
        if (e->index != -1 && e->name[0] != 0 && e->data != 0 && strcmp(e->name, path) == 0) {
            return e;
        }
    }
    return 0;
}

/* 0x800D5360 (0xB8): the table index of a resource-table entry by basename, or -1. */
s32 res_table_index_get(char* path) {
    ResManager* m = nw_res_manager;
    s32 i;
    ResEntry* e = m->resTable;
    char* p = str_rfind(path, 47);
    path = p == 0 ? path : p + 1;
    for (i = 0; i < 1024; i++, e++) {
        if (e->index != -1 && e->name[0] != 0 && e->data != 0 && strcmp(e->name, path) == 0) {
            return i;
        }
    }
    return -1;
}

/* 0x800D5418 (0x138): the resource-table entry whose index equals `index`, or NULL. */
ResEntry* nw_res_entry_get(s32 index) {
    ResManager* m = nw_res_manager;
    s32 i;
    ResEntry* e = m->resTable;
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

/* 0x800D5550 (0x1A4): registers a resource-table entry (or bumps an existing one's reference). */
s32 nwAddResource(char* name, void* data) {
    ResManager* m = nw_res_manager;
    ResEntry* e;
    s32 i;
    if (name == 0) {
        return -1;
    }
    if (name[0] == 0) {
        return -1;
    }
    char* p = str_rfind(name, 47);
    name = p == 0 ? name : p + 1;
    ResEntry* found = ckResourceName(name);
    if (found != 0) {
        found->flag++;
        return found->index;
    }
    if (data == 0) {
        return -1;
    }
    e = m->resTable;
    for (i = 0; i < 1024; i++, e++) {
        if (e->index == -1) {
            break;
        }
    }
    if (i >= 1024) {
        return -1;
    }
    strcpy(e->name, name);
    e->data = data;
    e->index = i;
    e->flag = 1;
    return i;
}

/* 0x800D56F4 (0x170): registers a resource-table entry for a new file without the name lookup. */
/* untyped: byte range - the loaded file image `data` registers under `name` */
s32 nw_res_entry_add(char* name, void* data) {
    ResManager* m = nw_res_manager;
    ResEntry* e;
    s32 i;
    if (name[0] == 0) {
        return -1;
    }
    if (data == 0) {
        return -1;
    }
    char* p = str_rfind(name, 47);
    name = p == 0 ? name : p + 1;
    e = m->resTable;
    for (i = 0; i < 1024; i++, e++) {
        if (e->index == -1) {
            break;
        }
    }
    if (i >= 1024) {
        return -1;
    }
    strcpy(e->name, name);
    e->data = data;
    e->index = i;
    e->flag = 1;
    return i;
}

/* 0x800D5864 (0x4C): drops a resource-table entry's registration. */
s32 nwDelResource(s32 index) {
    ResEntry* e = &nw_res_manager->resTable[index];
    if ((s32)e->flag > 1) {
        e->flag--;
        return -1;
    }
    e->name[0] = 0;
    e->index = -1;
    e->data = 0;
    e->flag = 0;
    return 0;
}

/* 0x800D58B0 (0x2C): clears one resource-table entry outright. */
void nw_res_entry_clear(s32 index) {
    ResEntry* e = &nw_res_manager->resTable[index];
    e->name[0] = 0;
    e->index = -1;
    e->data = 0;
    e->flag = 0;
}

/* 0x800D58DC (0x128): clears every resource-table entry. */
void res_table_clear(void) {
    ResEntry* e = nw_res_manager->resTable;
    u32 i;
    for (i = 0; i < 1024; i++, e++) {
        e->name[0] = 0;
        e->index = -1;
        e->data = 0;
        e->flag = 0;
    }
}

/* 0x800D5A04 (0x4): adds a resource-table entry (the load callback's spelling of `nwAddResource`). */
s32 nw_res_file_loaded(char* name, void* data) {
    return nwAddResource(name, data);
}

/* 0x800D5A08 (0x80): pulls a resource-memory slot and queues its file read. */
void* nw_res_file_load(char* path, u32 size) {
    s32 index = pull_res_mem(path, size, 0);
    if (index < 0) {
        return 0;
    }
    void* p = getResMemAdrs(index);
    load_file_req(path, (u32)p, (s32)size, (u32)nw_res_file_loaded, 0, 0);
    return p;
}

/* The player record the player-file loader works on (GUESS view): the loaded flag, the player's weapon type, the
 * player index and the two equipment records. */
struct NwResPlLoadWork {
    /* +0x000 */ u8 loaded;
    /* +0x001 */ u8 pad_0x001;
    /* +0x002 */ u8 plType;
    /* +0x003 */ u8 pad_0x003[0x5];
    /* +0x008 */ u8 player;
    /* +0x009 */ u8 pad_0x009[0x1C7];
    /* +0x1D0 */ u8 equipA[0x18]; /* an `_EQUIP` record */
    /* +0x1E8 */ u8 equipB[0x18]; /* an `_EQUIP` record */
}; /* size: 0x200 (approximation - the highest field read ends at +0x200) */

/* 0x800D5A88 (0xD0): the player-file load callback: registers the file, binds it to the player's character bank and
 * refreshes the weapon parts. */
/* untyped: byte range - the file image the read delivered */
void nw_res_pl_file_loaded(char* name, void* data, s32 size, NwResPlLoadWork** ctx) {
    u32 file;
    res_file_ctor(&file, 0);
    NwResPlLoadWork* pl = *ctx;
    nwAddResource(name, data);
    if (pl == NULL) {
        return;
    }
    ResEntry* e = ckResourceName(name);
    if (e != NULL) {
        u32 tmp;
        res_file_assign(&file, res_file_ctor(&tmp, (u32)e->data));
        mhchar_res_bank_set(1, pl->player, &file);
        pl->loaded = 1;
        pl->plType = Get_pl_type((_EQUIP*)pl->equipA, (_EQUIP*)pl->equipB);
        pl_weapon_parts_refresh(pl->player, (_EQUIP*)pl->equipA, (_EQUIP*)pl->equipB);
    }
    pl_res_loaded_set(pl->player);
}

/* 0x800D5B58 (0x8C): queues a player file's read into the player's buffer; 0 when the player has none. */
u32 nw_res_pl_file_load(NwResPlLoadWork* pl, char* path, s32 size) {
    u32 buffer = pl_res_buffer_get(pl->player);
    if (buffer == 0) {
        return 0;
    }
    NwResPlLoadWork* ctx = pl;
    load_file_req(path, buffer, size, (u32)nw_res_pl_file_loaded, 1, (u32*)&ctx);
    return buffer;
}

/* 0x800D5BE4 (0x68): queues every entry of a (size, name) table for the player; returns the count. */
s32 nw_res_pl_file_list_load(NwResPlLoadWork* pl, u32* table) {
    s32 count = 0;
    while (table[0] != 0) {
        nw_res_pl_file_load(pl, (char*)table[1], table[0]);
        count++;
        table += 2;
    }
    return count;
}

/* 0x800D5C4C (0x28): queues one table entry's file read. */
void nw_res_file_load_entry(char* path, u32* entry) {
    load_file_req((char*)entry[1], (u32)path, (s32)entry[0], (u32)nw_res_file_loaded, 0, 0);
}

/* 0x800D5C74 (0x38): brings up the work system. */
void nwWorkInitialize(void) {
    if (pRoot == 0) {
        reinterpret_cast<nw4r::g3d::ScnGroup*>(pRoot)->Clear();
    }
    res_table_clear();
    push_g3d_wk_all();
    nw_res_heap_reset();
}

/* 0x800D5CAC (0x6C): pulls every entry of a (name, size) request table. */
s32 nw_res_list_load(u32* table) {
    s32 count = 0;
    while (table[0] != 0) {
        if (res_mem_index_ref((const char*)table[1]) < 0) {
            nw_res_file_load((char*)table[1], table[0]);
            count++;
        }
        table += 2;
    }
    return count;
}

/* 0x800D5D18 (0x14): starts the move phase. */
void nwMoveStart(void) {
    if (pRoot != 0) {
        reinterpret_cast<nw4r::g3d::ScnGroup*>(pRoot)->Clear();
    }
}

/* 0x800D5D2C (0x34): ends the move phase. */
void nwMoveEnd(void) {
    if (pRoot != 0) {
        scn_root_calc_world(reinterpret_cast<nw4r::g3d::ScnRoot*>(pRoot));
        scn_root_calc_material(reinterpret_cast<nw4r::g3d::ScnRoot*>(pRoot));
    }
}

/* The enemy-kind record the enemy file loaders fill (GUESS view of `enemy/em_kind.cpp`'s work): its kind and the
 * resource-table handles of its model file and its motion file. */
struct NwResKindWork {
    /* +0x00 */ u8 pad_0x00[0x2];
    /* +0x02 */ u8 kind;
    /* +0x03 */ u8 pad_0x03[0x5];
    /* +0x08 */ s32 mdlHandle;
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ s32 motHandle;
}; /* size: 0x14 (approximation - the highest field read ends at +0x14) */

/* The six enemy-kind slots of move work 0 (GUESS view): each slot's kind and its model and motion handles. */
struct NwResKindSlots {
    /* +0x0000 */ u8 pad_0x0000[0x2164];
    /* +0x2164 */ u16 kinds[6];
    /* +0x2170 */ u8 pad_0x2170[0x24];
    /* +0x2194 */ s32 mdlHandles[6];
    /* +0x21AC */ u8 pad_0x21AC[0x48];
    /* +0x21F4 */ s32 motHandles[6];
}; /* size: 0x220C (approximation - the highest field written ends at +0x220C) */

/* 0x800D5D60 (0xD0): records an enemy kind's model handle in the slot of its kind. */
void nw_res_em_mdl_handle_set(NwResKindWork* work) {
    NwResKindSlots* slots = (NwResKindSlots*)get_move_work_adrs(0);
    s32 i;
    if (slots != NULL) {
        for (i = 0; i < 6; i++) {
            if (work->kind == (u8)slots->kinds[i]) {
                break;
            }
        }
        if (i < 6) {
            slots->mdlHandles[i] = work->mdlHandle;
        }
    }
}

/* 0x800D5E30 (0xD0): records an enemy kind's motion handle in the slot of its kind. */
void nw_res_em_mot_handle_set(NwResKindWork* work) {
    NwResKindSlots* slots = (NwResKindSlots*)get_move_work_adrs(0);
    s32 i;
    if (slots != NULL) {
        for (i = 0; i < 6; i++) {
            if (work->kind == (u8)slots->kinds[i]) {
                break;
            }
        }
        if (i < 6) {
            slots->motHandles[i] = work->motHandle;
        }
    }
}
