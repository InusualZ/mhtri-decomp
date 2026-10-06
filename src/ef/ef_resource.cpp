/*
 * ef/ef_resource.cpp - nw4r::ef::Resource: the resource singleton (two `nw4r::ut::List`s of loaded REFF effect
 *   projects and REFT texture projects plus their emitter/texture counters), its constructor and static initialiser,
 *   the REFF/REFT file checks and relocation, the per-project and global lookups by index and by name, the texture
 *   and child-emitter binding pass, and the project removal and teardown.
 * RANGE. .text 0x800B2878-0x800B4AC8 (31 functions, in retail order); extab 0x8000A044-0x8000A10C, extabindex
 *   0x8002340C-0x80023538, .ctors 0x8056F2E0-0x8056F2E4 (the static initialiser 0x800B4ABC), .data
 *   0x80593690-0x805939E8 (the `__FILE__` string "ef_resource.cpp" first, cited from 0x800B28FC to 0x800B4898),
 *   .bss 0x80694598-0x806945B8 (the singleton), .sdata 0x807912E8-0x80791300 ("REFF"/"REFT", read by 0x800B3670 and
 *   0x800B3E80).  Left edge: see `ef/ef_postfield.cpp`.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`.
 * NAMES. The file name is the TU's own `__FILE__` string (0x80593690).  The data names spell their contents (the
 *   assert/warning strings, the "REFF"/"REFT" tags).  Every function name is a GUESS (retail is a C++ class,
 *   nw4r::ef::Resource, whose method names the binary does not carry; the C spelling keeps the existing callers):
 *   GUESS (the singleton): `ef_resource_instance`, `ef_resource_construct`, `ef_resource_init`, `ef_resource_static_init`.
 *   GUESS (the relocation): `ef_resource_relocate_effect_project`, `ef_resource_relocate_emitter_tracks`,
 *   GUESS (the relocation): `ef_resource_relocate_texture_project`, `ef_resource_version` (the assert's `GetVersion()`).
 *   GUESS (per project): `ef_resource_project_num_emitters`, `ef_resource_project_num_textures`,
 *   GUESS (per project): `ef_resource_project_find_emitter`, `ef_resource_project_emitter_at`,
 *   GUESS (per project): `ef_resource_project_find_texture`.
 *   GUESS (the loaders): `ef_resource_add_effect_project`, `ef_resource_add_texture_project`.
 *   GUESS (the lookups): `ef_resource_find_emitter_handle`, `ef_resource_find_emitter`, `ef_resource_find_texture`,
 *   GUESS (the lookups): `ef_resource_num_emitters`, `ef_resource_num_textures`, `ef_resource_emitter_at`.
 *   GUESS (the binding, after its "Cannot resolve Texture/Child" warnings): `ef_resource_bind_track_textures`,
 *   GUESS (the binding): `ef_resource_bind_track_children`, `ef_resource_bind_references`.
 *   GUESS (the teardown): `ef_resource_remove_effect_project`, `ef_resource_remove_all_effect_projects`,
 *   GUESS (the teardown): `ef_resource_remove_texture_project`, `ef_resource_remove_all_texture_projects`.
 *   GUESS (the list accessors): `ef_resource_effect_project_at`, `ef_resource_num_texture_projects`,
 *   GUESS (the list accessors): `ef_resource_texture_project_at`.
 *   GUESS: the record names `EfResourceManager`/`EfResProject`/`EfResTrack`/`EfPtclParam` and their field names are
 *   read from the code (the track's size words are GUESSes).
 * RESIDUALS. `ef_resource_bind_references` 0x800B44F4: register assignment only - retail keeps the record in r25 and the
 *   emitter in r26, ours swaps them (declaration order, a cast copy of the parameter and hoisting the locals do not
 *   move it).  flipcheck: `.bss`, `.ctors`, `.data` and `.sdata` claimed, not emitted (declared by their map names,
 *   playbook 29).  Retail's member functions are spelled as C functions taking the record (see NAMES).
 * SHAPES. A table count read once into a local declared first (the by-name lookups); the big-endian offset read into
 *   a local before the entry pointer advances; `x + (n * 8 + 4)` for the track blocks and `&names->slot[n]` for the
 *   name list (both keep retail's temporary); the track binders count in a u32 that the caller narrows to u8.
 */

#include "types.h"
#include "ef.h"
#include "ef/ef_resource.h"
#include "ef/ef_list_get_first.h" /* ef_list_get_first, ef_emres_get_name (rule 2) */
#include "ef/ef_effect.h" /* ef_res_emitter_desc (rule 2) */
#include "ef/ef_list_get_last.h" /* ef_list_get_last, the particle-track accessors (rule 2) */
#include "ef/ef_emres_get_ptcl_track.h" /* the emitter-resource track accessors (rule 2) */
#include "ef/ef_emres_get_ptcl_param.h" /* ef_emres_get_ptcl_param (rule 2) */
#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::ut::List_* (rule 2) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic / Warning (rule 2) */
#include "MSL_C/alloc.h" /* strcmp (rule 2) */
#include "MSL_C/strstr.h" /* strncmp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h" /* memset (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

#pragma peephole off

/* This TU's `__FILE__`/assert strings (its claimed `.data`), its "REFF"/"REFT" tags (`.sdata`) and its singleton
 * (`.bss`), declared, never defined. */
extern char ef_resource_file_str[];           /* "ef_resource.cpp"                                .data 0x80593690 */
extern char ef_resource_project_ptr_str[];    /* "...project(=%p) is not valid pointer."          .data 0x805936A0 */
extern char ef_resource_search_name_ptr_str[]; /* "...searchName(=%p) is not valid pointer."      .data 0x805936D8 */
extern char ef_resource_data_ptr_str[];       /* "...data(=%p) is not valid pointer."             .data 0x80593714 */
extern char ef_resource_reff_signature_str[]; /* "...strncmp(fhd->signature, \"REFF\", 4) == 0"   .data 0x80593748 */
extern char ef_resource_byte_order_str[];     /* "...fhd->byteorder[0] == 0xfe && ..."            .data 0x80593788 */
extern char ef_resource_version_str[];        /* "...fhd->version == GetVersion()"                .data 0x805937D8 */
extern char ef_resource_reff_block_str[];     /* "...strncmp(bhd->type, \"REFF\", 4) == 0"        .data 0x8059380C */
extern char ef_resource_name_ptr_str[];       /* "...name(=%p) is not valid pointer."             .data 0x80593848 */
extern char ef_resource_reft_signature_str[]; /* "...strncmp(fhd->signature, \"REFT\", 4) == 0"   .data 0x8059387C */
extern char ef_resource_reft_block_str[];     /* "...strncmp(bhd->type, \"REFT\", 4) == 0"        .data 0x805938BC */
extern char ef_resource_cmd_ptr_str[];        /* "...cmdPtr(=%p) is not valid pointer."           .data 0x805938F8 */
extern char ef_resource_texture_from_str[];   /* "Cannot resolve Texture(%s from:%s)\n"           .data 0x80593930 */
extern char ef_resource_texture_str[];        /* "Cannot resolve Texture(%s)\n"                   .data 0x80593954 */
extern char ef_resource_child_from_str[];     /* "Cannot resolve Child(%s from:%s)\n"             .data 0x80593970 */
extern char ef_resource_child_str[];          /* "Cannot resolve Child(%s)\n"                     .data 0x80593994 */
extern char ef_resource_target_ptr_str[];     /* "...target(=%p) is not valid pointer."           .data 0x805939B0 */
extern char ef_resource_reff_tag[5];          /* "REFF"                                           .sdata 0x807912E8 */
extern char ef_resource_reft_tag[5];          /* "REFT"                                           .sdata 0x807912F0 */
extern EfResourceManager ef_resource_singleton;      /* the resource singleton                           .bss 0x80694598 */

/* The NW4R binary file header a loaded REFF/REFT file starts with. */
typedef struct EfResFileHeader {
    char signature[4]; /* +0x00  "REFF" / "REFT" */
    u8 byte_order[2];  /* +0x04  0xFE 0xFF */
    u16 version;       /* +0x06  checked against `ef_resource_version` */
    u32 file_size;     /* +0x08 */
    u16 header_size;   /* +0x0C  offset to the first block header */
    u16 num_blocks;    /* +0x0E */
} EfResFileHeader; /* size: 0x10 */

/* The block header in front of the project. */
typedef struct EfResBlockHeader {
    char type[4]; /* +0x00  "REFF" / "REFT" */
    u32 size;     /* +0x04 */
    u8 body[1];   /* +0x08  the project */
} EfResBlockHeader; /* size: 0x09 (approximation: the header is 0x08, the project follows) */

/* The project's name table: a count, then per entry a big-endian u16 name length, the name, a big-endian u32 offset
 * from the table to the entry's resource and a u32 size. */
typedef struct EfResTable {
    u32 table_size;  /* +0x00 */
    u16 num_entries; /* +0x04 */
    u16 pad_0x06;    /* +0x06 */
    u8 entries[1];   /* +0x08  the first name entry */
} EfResTable; /* size: 0x09 (approximation: the header is 0x08, the entries follow) */

/* An emitter resource: the relocation writes its name pointer. size: approximation (the emitter data follows). */
typedef struct EfEmitterRes {
    const char* name; /* +0x00 */
} EfEmitterRes; /* size: 0x04 */

/* The emitter descriptor `ef_res_emitter_desc` returns: its first word holds the bound flag. */
typedef struct EfEmitterDesc {
    u32 flags; /* +0x00  0x40000000: every reference resolved */
} EfEmitterDesc; /* size: 0x04 (approximation: the descriptor continues) */

/* A texture resource: the relocation writes its name and its image and palette pointers. */
typedef struct EfTextureRes {
    const char* name;      /* +0x00 */
    u8 pad_0x04[0x04];     /* +0x04 */
    u32 image_size;        /* +0x08  the palette follows the image */
    u8 pad_0x0C[0x10];     /* +0x0C */
    u8* image;             /* +0x1C  the data after this header */
    u8* palette;           /* +0x20 */
    u8 pad_0x24[0x1C];     /* +0x24 */
    u8 image_data[1];      /* +0x40  the image, then the palette */
} EfTextureRes; /* size: 0x41 (approximation: the header is 0x40, the image follows) */

/* The particle parameters `ef_emres_get_ptcl_param` returns: three bound textures and their names. */
typedef struct EfPtclParam {
    u8 pad_0x00[0x68];  /* +0x00 */
    void* texture[3];   /* +0x68  bound by name */
    u8 pad_0x74[0x14];  /* +0x74 */
    u8 texture_names[1]; /* +0x88  three big-endian-length-prefixed names */
} EfPtclParam; /* size: 0x89 (approximation: the names run on) */

/* A particle track: a two-byte tag (0xAB/0xAC), its kind, the sizes of its three tables and, when present, the name
 * table whose entries bind to textures (kind 4) or child emitters (kinds 2 and 5). */
typedef struct EfResTrack {
    u8 tag;              /* +0x00  0xAB or 0xAC */
    u8 pad_0x01;         /* +0x01 */
    u8 kind;             /* +0x02 */
    u8 pad_0x03[0x09];   /* +0x03 */
    u32 key_table_size;  /* +0x0C */
    u32 range_table_size; /* +0x10 */
    u32 random_table_size; /* +0x14 */
    u32 name_table_size; /* +0x18  0: no name table */
    u8 pad_0x1C[0x04];   /* +0x1C */
} EfResTrack; /* size: 0x20 */

/* The name table after a track's three tables: a count, the bound slots, then the length-prefixed names. */
typedef struct EfResNameTable {
    u16 num_names; /* +0x00 */
    u16 pad_0x02;  /* +0x02 */
    void* slot[1]; /* +0x04  one per name */
} EfResNameTable; /* size: 0x08 (approximation: one slot per name) */

/* Reads a big-endian u32 from a byte stream. */
#define EF_RES_READ_BE32(p) ((p)[3] + ((p)[2] << 8) + (((p)[0] << 24) + ((p)[1] << 16)))

/* 0x800B2878 (0xC): Returns the resource singleton. */
EfResourceManager* ef_resource_instance(void) {
    return &ef_resource_singleton;
}

void ef_resource_init(EfResourceManager* self);

/* 0x800B2884 (0x30): Constructs the singleton's record. */
EfResourceManager* ef_resource_construct(EfResourceManager* self) {
    ef_resource_init(self);
    return self;
}

/* 0x800B28B4 (0x48): Empties both project lists and clears their counters. */
void ef_resource_init(EfResourceManager* self) {
    nw4r::ut::List_Init(&self->effect_projects, 4);
    nw4r::ut::List_Init(&self->texture_projects, 4);
    self->num_emitters = 0;
    self->num_textures = 0;
}

void ef_resource_relocate_emitter_tracks(EfEmitterRes* res);

/* 0x800B28FC (0x19C): Points every emitter of an effect project at its name and relocates its track tables. */
void ef_resource_relocate_effect_project(EfResProject* project) {
    EfResTable* table;
    u8* entry;
    int i;

    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 112, ef_resource_project_ptr_str, project);
    }
    table = (EfResTable*)((u8*)project + project->header_size);
    entry = table->entries;
    for (i = 0; i < table->num_entries; i++) {
        int length = (entry[0] << 8) + entry[1];
        char* name;
        u32 offset;
        EfEmitterRes* res;

        entry += 2;
        name = (char*)entry;
        entry += length;
        offset = EF_RES_READ_BE32(entry);
        entry += 8;
        res = (EfEmitterRes*)((u8*)table + offset);
        res->name = name;
        ef_resource_relocate_emitter_tracks(res);
    }
}

/* 0x800B2A98 (0x120): Fills an emitter's particle- and emitter-track tables with the tracks' addresses. */
void ef_resource_relocate_emitter_tracks(EfEmitterRes* res) {
    u8* ptcl_block;
    u8* track;
    u8** ptcl_table;
    u8** emit_table;
    u32* ptcl_size;
    u32* emit_size;
    int i;

    ptcl_block = ef_emres_get_ptcl_track(res);
    track = ptcl_block + (ef_emres_num_ptcl_track(res) * 8 + 4);
    track = track + (ef_emres_num_emit_track(res) * 8 + 4);
    ptcl_table = ef_emres_get_ptcl_track_tbl(res);
    emit_table = ef_emres_get_emit_track_tbl(res);
    ptcl_size = (u32*)(ef_emres_get_ptcl_track_tbl(res) + ef_emres_num_ptcl_track(res));
    emit_size = (u32*)(ef_emres_get_emit_track_tbl(res) + ef_emres_num_emit_track(res));
    for (i = 0; i < ef_emres_num_ptcl_track(res); i++) {
        *ptcl_table = track;
        track += *ptcl_size;
        ptcl_table++;
        ptcl_size++;
    }
    for (i = 0; i < ef_emres_num_emit_track(res); i++) {
        *emit_table = track;
        track += *emit_size;
        emit_table++;
        emit_size++;
    }
}

/* 0x800B2BB8 (0x120): Returns the number of emitters an effect project holds. */
u16 ef_resource_project_num_emitters(EfResProject* project) {
    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 151, ef_resource_project_ptr_str, project);
    }
    return ((EfResTable*)((u8*)project + project->header_size))->num_entries;
}

/* 0x800B2CD8 (0x120): Returns the number of textures a texture project holds. */
u16 ef_resource_project_num_textures(EfResProject* project) {
    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 165, ef_resource_project_ptr_str, project);
    }
    return ((EfResTable*)((u8*)project + project->header_size))->num_entries;
}

/* 0x800B2DF8 (0x294): Returns the effect project's emitter named `name`, or null. */
EfEmitterRes* ef_resource_project_find_emitter(EfResProject* project, const char* name) {
    u16 count;
    EfResTable* table;
    u8* entry;
    int i;

    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 179, ef_resource_project_ptr_str, project);
    }
    if (!IsValidPointer((u32)name)) {
        nw4r::db::Panic(ef_resource_file_str, 180, ef_resource_search_name_ptr_str, name);
    }
    table = (EfResTable*)((u8*)project + project->header_size);
    entry = table->entries;
    i = 0;
    count = table->num_entries;
    for (; i < count; i++) {
        int length = (entry[0] << 8) + entry[1];
        char* entry_name;
        u32 offset;

        entry += 2;
        entry_name = (char*)entry;
        entry += length;
        offset = EF_RES_READ_BE32(entry);
        entry += 8;
        if (strcmp(name, entry_name) == 0) {
            return (EfEmitterRes*)((u8*)table + offset);
        }
    }
    return 0;
}

/* 0x800B308C (0x1B4): Returns the effect project's emitter number `index`, or null. */
EfEmitterRes* ef_resource_project_emitter_at(EfResProject* project, u16 index) {
    EfResTable* table;
    u8* entry;
    int i;

    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 222, ef_resource_project_ptr_str, project);
    }
    table = (EfResTable*)((u8*)project + project->header_size);
    if (index >= table->num_entries) {
        return 0;
    }
    entry = table->entries;
    for (i = 0; i < table->num_entries; i++) {
        int length = (entry[0] << 8) + entry[1];
        u32 offset;

        entry += 2;
        entry += length;
        offset = EF_RES_READ_BE32(entry);
        entry += 8;
        if (i == index) {
            return (EfEmitterRes*)((u8*)table + offset);
        }
    }
    return 0;
}

/* 0x800B3240 (0x19C): Points every texture of a texture project at its name, image and palette. */
void ef_resource_relocate_texture_project(EfResProject* project) {
    EfResTable* table;
    u8* entry;
    int i;

    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 310, ef_resource_project_ptr_str, project);
    }
    table = (EfResTable*)((u8*)project + project->header_size);
    entry = table->entries;
    for (i = 0; i < table->num_entries; i++) {
        int length = (entry[0] << 8) + entry[1];
        char* name;
        u32 offset;
        u8* image;
        EfTextureRes* res;

        entry += 2;
        name = (char*)entry;
        entry += length;
        offset = EF_RES_READ_BE32(entry);
        entry += 8;
        res = (EfTextureRes*)((u8*)table + offset);
        res->name = name;
        image = res->image_data;
        res->image = image;
        res->palette = image + res->image_size;
    }
}

/* 0x800B33DC (0x294): Returns the texture project's texture named `name`, or null. */
EfTextureRes* ef_resource_project_find_texture(EfResProject* project, const char* name) {
    u16 count;
    EfResTable* table;
    u8* entry;
    int i;

    if (!IsValidPointer((u32)project)) {
        nw4r::db::Panic(ef_resource_file_str, 350, ef_resource_project_ptr_str, project);
    }
    if (!IsValidPointer((u32)name)) {
        nw4r::db::Panic(ef_resource_file_str, 351, ef_resource_search_name_ptr_str, name);
    }
    table = (EfResTable*)((u8*)project + project->header_size);
    entry = table->entries;
    i = 0;
    count = table->num_entries;
    for (; i < count; i++) {
        int length = (entry[0] << 8) + entry[1];
        char* entry_name;
        u32 offset;

        entry += 2;
        entry_name = (char*)entry;
        entry += length;
        offset = EF_RES_READ_BE32(entry);
        entry += 8;
        if (strcmp(name, entry_name) == 0) {
            return (EfTextureRes*)((u8*)table + offset);
        }
    }
    return 0;
}

u16 ef_resource_version(void);

/* 0x800B3670 (0x248): Checks a loaded REFF file, files its effect project and relocates it; returns the project. */
s32 ef_resource_add_effect_project(void* work, void* data) { /* untyped: opaque handle - the singleton and the loaded file as the loader passes them */
    EfResourceManager* self = (EfResourceManager*)work;
    EfResFileHeader* fhd = (EfResFileHeader*)data;
    EfResBlockHeader* bhd;
    EfResProject* project;
    bool ok;

    if (!IsValidPointer((u32)fhd)) {
        nw4r::db::Panic(ef_resource_file_str, 392, ef_resource_data_ptr_str, fhd);
    }
    if (strncmp(fhd->signature, ef_resource_reff_tag, 4) != 0) {
        nw4r::db::Panic(ef_resource_file_str, 400, ef_resource_reff_signature_str);
    }
    ok = fhd->byte_order[0] == 0xFE && fhd->byte_order[1] == 0xFF;
    if (!ok) {
        nw4r::db::Panic(ef_resource_file_str, 401, ef_resource_byte_order_str);
    }
    if (fhd->version != ef_resource_version()) {
        nw4r::db::Panic(ef_resource_file_str, 402, ef_resource_version_str);
    }
    bhd = (EfResBlockHeader*)((u8*)fhd + fhd->header_size);
    if (strncmp(bhd->type, ef_resource_reff_tag, 4) != 0) {
        nw4r::db::Panic(ef_resource_file_str, 410, ef_resource_reff_block_str);
    }
    project = (EfResProject*)bhd->body;
    memset(&project->link, 0, sizeof(project->link));
    nw4r::ut::List_Append(&self->effect_projects, project);
    self->num_emitters += ef_resource_project_num_emitters(project);
    ef_resource_relocate_effect_project(project);
    return (s32)project;
}

/* 0x800B38B8 (0x8): Returns the REFF/REFT file version this loader accepts. */
u16 ef_resource_version(void) {
    return 11;
}

EfEmitterRes* ef_resource_find_emitter(EfResourceManager* self, const char* name, EfResProject* project);

/* 0x800B38C0 (0x154): Stores the emitter named `name` into the caller's handle. */
void ef_resource_find_emitter_handle(void* out, EfResourceManager* self, const char* name, EfResProject* project) { /* untyped: opaque handle - the caller's emitter handle slot */
    if (!IsValidPointer((u32)name)) {
        nw4r::db::Panic(ef_resource_file_str, 463, ef_resource_name_ptr_str, name);
    }
    ef_store_word(out, (s32)ef_resource_find_emitter(self, name, project));
}

/* 0x800B3A14 (0x190): Finds the emitter named `name` in `project`, else in the newest project that has it. */
EfEmitterRes* ef_resource_find_emitter(EfResourceManager* self, const char* name, EfResProject* project) {
    EfResProject* node;

    if (!IsValidPointer((u32)name)) {
        nw4r::db::Panic(ef_resource_file_str, 472, ef_resource_name_ptr_str, name);
    }
    if (project != 0) {
        EfEmitterRes* res = ef_resource_project_find_emitter(project, name);
        if (res != 0) {
            return res;
        }
    }
    for (node = (EfResProject*)ef_list_get_last(&self->effect_projects); node != 0;
         node = (EfResProject*)nw4r::ut::List_GetPrev(&self->effect_projects, node)) {
        EfEmitterRes* res = ef_resource_project_find_emitter(node, name);
        if (res != 0) {
            return res;
        }
    }
    return 0;
}

/* 0x800B3BA4 (0x1A8): Finds the texture named `name` in `project`, else in the newest project that has it. */
EfTextureRes* ef_resource_find_texture(EfResourceManager* self, const char* name, EfResProject* project) {
    EfResProject* node;

    if (!IsValidPointer((u32)name)) {
        nw4r::db::Panic(ef_resource_file_str, 514, ef_resource_name_ptr_str, name);
    }
    if (*name == 0) {
        return 0;
    }
    if (project != 0) {
        EfTextureRes* res = ef_resource_project_find_texture(project, name);
        if (res != 0) {
            return res;
        }
    }
    for (node = (EfResProject*)ef_list_get_last(&self->texture_projects); node != 0;
         node = (EfResProject*)nw4r::ut::List_GetPrev(&self->texture_projects, node)) {
        EfTextureRes* res = ef_resource_project_find_texture(node, name);
        if (res != 0) {
            return res;
        }
    }
    return 0;
}

/* 0x800B3D4C (0x38): Returns `project`'s emitter count, or every loaded project's when it is null. */
u32 ef_resource_num_emitters(EfResourceManager* self, EfResProject* project) {
    if (project != 0) {
        return (u16)ef_resource_project_num_emitters(project);
    }
    return self->num_emitters;
}

/* 0x800B3D84 (0x38): Returns `project`'s texture count, or every loaded project's when it is null. */
u32 ef_resource_num_textures(EfResourceManager* self, EfResProject* project) {
    if (project != 0) {
        return (u16)ef_resource_project_num_textures(project);
    }
    return self->num_textures;
}

/* 0x800B3DBC (0xC4): Returns emitter number `index` of `project`, or of all projects in load order when it is null. */
EfEmitterRes* ef_resource_emitter_at(EfResourceManager* self, u32 index, EfResProject* project) {
    if (project != 0) {
        u32 n = (u16)ef_resource_project_num_emitters(project);
        if (index < n) {
            return ef_resource_project_emitter_at(project, (u16)index);
        }
        return 0;
    }
    EfResProject* node = (EfResProject*)ef_list_get_first(&self->effect_projects);
    while (node != 0) {
        u32 n = (u16)ef_resource_project_num_emitters(node);
        if (index < n) {
            return ef_resource_project_emitter_at(node, (u16)index);
        }
        index -= n;
        node = (EfResProject*)nw4r::ut::List_GetNext(&self->effect_projects, node);
    }
    return 0;
}

/* 0x800B3E80 (0x248): Checks a loaded REFT file, files its texture project and relocates it; returns the project. */
s32 ef_resource_add_texture_project(void* work, void* data) { /* untyped: opaque handle - the singleton and the loaded file as the loader passes them */
    EfResourceManager* self = (EfResourceManager*)work;
    EfResFileHeader* fhd = (EfResFileHeader*)data;
    EfResBlockHeader* bhd;
    EfResProject* project;
    bool ok;

    if (!IsValidPointer((u32)fhd)) {
        nw4r::db::Panic(ef_resource_file_str, 647, ef_resource_data_ptr_str, fhd);
    }
    if (strncmp(fhd->signature, ef_resource_reft_tag, 4) != 0) {
        nw4r::db::Panic(ef_resource_file_str, 655, ef_resource_reft_signature_str);
    }
    ok = fhd->byte_order[0] == 0xFE && fhd->byte_order[1] == 0xFF;
    if (!ok) {
        nw4r::db::Panic(ef_resource_file_str, 656, ef_resource_byte_order_str);
    }
    if (fhd->version != ef_resource_version()) {
        nw4r::db::Panic(ef_resource_file_str, 657, ef_resource_version_str);
    }
    bhd = (EfResBlockHeader*)((u8*)fhd + fhd->header_size);
    if (strncmp(bhd->type, ef_resource_reft_tag, 4) != 0) {
        nw4r::db::Panic(ef_resource_file_str, 665, ef_resource_reft_block_str);
    }
    project = (EfResProject*)bhd->body;
    memset(&project->link, 0, sizeof(project->link));
    nw4r::ut::List_Append(&self->texture_projects, project);
    self->num_textures += ef_resource_project_num_textures(project);
    ef_resource_relocate_texture_project(project);
    return (s32)project;
}

/* 0x800B40C8 (0x21C): Binds a texture track's names to loaded textures; returns how many stayed unresolved. */
u32 ef_resource_bind_track_textures(EfResourceManager* self, EfResTrack* track, EfEmitterRes* emitter) {
    void** slot;
    EfResNameTable* names;
    u8* name;
    int i;
    u8* tables;
    u32 failed;

    if (!IsValidPointer((u32)track)) {
        nw4r::db::Panic(ef_resource_file_str, 685, ef_resource_cmd_ptr_str, track);
    }
    failed = 0;
    tables = (u8*)track + track->key_table_size;
    tables += track->range_table_size;
    tables += track->random_table_size;
    names = (EfResNameTable*)(tables + sizeof(EfResTrack));
    if (track->name_table_size == 0) {
        return 0;
    }
    name = (u8*)&names->slot[names->num_names];
    for (i = 0, slot = (void**)names; i < names->num_names; i++) {
        u16 length = *(u16*)name;

        name += 2;
        slot[1] = ef_resource_find_texture(self, (char*)name, 0);
        if (*name != 0 && slot[1] == 0) {
            failed++;
            if (emitter != 0) {
                nw4r::db::Warning(ef_resource_file_str, 712, ef_resource_texture_from_str, name,
                                  ef_emres_get_name(emitter));
            } else {
                nw4r::db::Warning(ef_resource_file_str, 716, ef_resource_texture_str, name);
            }
        }
        name += length;
        slot++;
    }
    return failed;
}

/* 0x800B42E4 (0x210): Binds a child track's names to loaded emitters; returns how many stayed unresolved. */
u32 ef_resource_bind_track_children(EfResourceManager* self, EfResTrack* track, EfEmitterRes* emitter) {
    void** slot;
    EfResNameTable* names;
    u8* name;
    int i;
    u8* tables;
    u32 failed;

    if (!IsValidPointer((u32)track)) {
        nw4r::db::Panic(ef_resource_file_str, 729, ef_resource_cmd_ptr_str, track);
    }
    failed = 0;
    tables = (u8*)track + track->key_table_size;
    tables += track->range_table_size;
    tables += track->random_table_size;
    names = (EfResNameTable*)(tables + sizeof(EfResTrack));
    if (track->name_table_size == 0) {
        return 0;
    }
    name = (u8*)&names->slot[names->num_names];
    for (i = 0, slot = (void**)names; i < names->num_names; i++) {
        u16 length = *(u16*)name;

        name += 2;
        slot[1] = ef_resource_find_emitter(self, (char*)name, 0);
        if (slot[1] == 0) {
            failed++;
            if (emitter != 0) {
                nw4r::db::Warning(ef_resource_file_str, 756, ef_resource_child_from_str, name,
                                  ef_emres_get_name(emitter));
            } else {
                nw4r::db::Warning(ef_resource_file_str, 760, ef_resource_child_str, name);
            }
        }
        name += length;
        slot++;
    }
    return failed;
}

/* 0x800B44F4 (0x1CC): Binds every loaded emitter's textures and child emitters; returns how many stayed unresolved. */
s32 ef_resource_bind_references(EfResourceManager* self) {
    u32 total = 0;
    u32 index;

    for (index = 0; index < ef_resource_num_emitters(self, 0); index++) {
        u8 failed = 0;
        EfEmitterRes* emitter = ef_resource_emitter_at(self, index, 0);

        if (emitter != 0) {
            void** slot;
            u8* name;
            int i;
            u16 track_index;
            EfEmitterDesc* desc;
            EfPtclParam* param = ef_emres_get_ptcl_param(emitter);

            name = param->texture_names;
            i = 0;
            slot = param->texture;

            do {
                u16 length = *(u16*)name;

                name += 2;
                *slot = ef_resource_find_texture(self, (char*)name, 0);
                if (*name != 0 && *slot == 0) {
                    failed++;
                    nw4r::db::Warning(ef_resource_file_str, 800, ef_resource_texture_from_str, name,
                                      ef_emres_get_name(emitter));
                }
                name += length;
                slot++;
                i++;
            } while (i < 3);
            for (track_index = 0; track_index < ef_emres_num_ptcl_track(emitter); track_index++) {
                EfResTrack* track = ef_emres_get_ptcl_track_at(emitter, track_index);

                if ((u8)(track->tag + 0x55) <= 1) {
                    switch (track->kind) {
                    case 4:
                        failed += ef_resource_bind_track_textures(self, track, emitter);
                        break;
                    case 5:
                    case 2:
                        failed += ef_resource_bind_track_children(self, track, emitter);
                        break;
                    }
                }
            }
            desc = ef_res_emitter_desc(emitter);
            if (failed != 0) {
                desc->flags &= ~0x40000000;
            } else {
                desc->flags |= 0x40000000;
            }
            total += failed;
        }
    }
    return total;
}

/* 0x800B46C0 (0x17C): Unlinks an effect project and drops its emitters from the count; returns whether it was loaded. */
s32 ef_resource_remove_effect_project(void* work, void* target) { /* untyped: opaque handle - the singleton and the project to unlink */
    EfResourceManager* self = (EfResourceManager*)work;
    EfResProject* node;

    if (!IsValidPointer((u32)target)) {
        nw4r::db::Panic(ef_resource_file_str, 845, ef_resource_target_ptr_str, target);
    }
    for (node = (EfResProject*)ef_list_get_first(&self->effect_projects); node != 0;
         node = (EfResProject*)nw4r::ut::List_GetNext(&self->effect_projects, node)) {
        if (node == target) {
            nw4r::ut::List_Remove(&self->effect_projects, target);
            self->num_emitters -= ef_resource_num_emitters(self, (EfResProject*)target);
            return 1;
        }
    }
    return 0;
}

/* 0x800B483C (0x5C): Unlinks every effect project and clears the emitter count. */
int ef_resource_remove_all_effect_projects(EfResourceManager* self) {
    while (ef_list_get_first(&self->effect_projects) != 0) {
        nw4r::ut::List_Remove(&self->effect_projects, ef_list_get_first(&self->effect_projects));
    }
    self->num_emitters = 0;
    return 1;
}

/* 0x800B4898 (0x17C): Unlinks a texture project and drops its textures from the count; returns whether it was loaded. */
s32 ef_resource_remove_texture_project(void* work, void* target) { /* untyped: opaque handle - the singleton and the project to unlink */
    EfResourceManager* self = (EfResourceManager*)work;
    EfResProject* node;

    if (!IsValidPointer((u32)target)) {
        nw4r::db::Panic(ef_resource_file_str, 881, ef_resource_target_ptr_str, target);
    }
    for (node = (EfResProject*)ef_list_get_first(&self->texture_projects); node != 0;
         node = (EfResProject*)nw4r::ut::List_GetNext(&self->texture_projects, node)) {
        if (node == target) {
            nw4r::ut::List_Remove(&self->texture_projects, target);
            self->num_textures -= ef_resource_num_textures(self, (EfResProject*)target);
            return 1;
        }
    }
    return 0;
}

/* 0x800B4A14 (0x5C): Unlinks every texture project and clears the texture count. */
int ef_resource_remove_all_texture_projects(EfResourceManager* self) {
    while (ef_list_get_first(&self->texture_projects) != 0) {
        nw4r::ut::List_Remove(&self->texture_projects, ef_list_get_first(&self->texture_projects));
    }
    self->num_textures = 0;
    return 1;
}

/* 0x800B4A70 (0x20): Returns effect project number `index`, or null. */
EfResProject* ef_resource_effect_project_at(EfResourceManager* self, u16 index) {
    if ((u32)index >= self->effect_projects.numObjects) {
        return 0;
    }
    return (EfResProject*)nw4r::ut::List_GetNth(&self->effect_projects, index);
}

/* 0x800B4A90 (0x8): Returns the number of loaded texture projects. */
u16 ef_resource_num_texture_projects(EfResourceManager* self) {
    return self->texture_projects.numObjects;
}

/* 0x800B4A98 (0x24): Returns texture project number `index`, or null. */
EfResProject* ef_resource_texture_project_at(EfResourceManager* self, u16 index) {
    if ((u32)index >= self->texture_projects.numObjects) {
        return 0;
    }
    return (EfResProject*)nw4r::ut::List_GetNth(&self->texture_projects, index);
}

/* 0x800B4ABC (0xC): Constructs the singleton at start-up (the unit's `.ctors` entry). */
void ef_resource_static_init(void) {
    ef_resource_construct(&ef_resource_singleton);
}

#ifdef __cplusplus
}
#endif
