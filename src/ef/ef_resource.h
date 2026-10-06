/* ef/ef_resource.h - the `ef/ef_resource.cpp` records and the symbols other units call (C linkage): the resource
 * singleton's getter, its list accessors, the REFF/REFT loaders, the reference binding pass, the emitter lookup and
 * the teardown. */
#ifndef MHTRI_EF_EF_RESOURCE_H
#define MHTRI_EF_EF_RESOURCE_H

#include "types.h"
#include "nw4r/fn_805012C4.h"

/* A loaded REFF/REFT project: the offset to its name table, then the link the resource's lists thread it on. */
typedef struct EfResProject {
    u32 header_size;          /* +0x00  offset from the project to its name table */
    nw4r::ut::Link link;      /* +0x04  cleared and appended by the loader */
} EfResProject; /* size: 0x0C (approximation: the project name follows) */

/* The resource singleton: the effect (REFF) and texture (REFT) project lists and their element counters. */
typedef struct EfResourceManager {
    nw4r::ut::List effect_projects;  /* +0x00  linked through EfResProject::link */
    u32 num_emitters;                /* +0x0C  every loaded effect project's emitters */
    nw4r::ut::List texture_projects; /* +0x10 */
    u32 num_textures;                /* +0x1C  every loaded texture project's textures */
} EfResourceManager; /* size: 0x20 */

#ifdef __cplusplus
extern "C" {
#endif

EfResourceManager* ef_resource_instance(void);
u16 ef_resource_num_texture_projects(EfResourceManager* self);
EfResProject* ef_resource_texture_project_at(EfResourceManager* self, u16 index);
EfResProject* ef_resource_effect_project_at(EfResourceManager* self, u16 index);
int ef_resource_remove_all_effect_projects(EfResourceManager* self);
int ef_resource_remove_all_texture_projects(EfResourceManager* self);
/* untyped: opaque handle - the caller's emitter handle slot */
void ef_resource_find_emitter_handle(void* out, EfResourceManager* self, const char* name, EfResProject* project);
/* untyped: opaque handle - the resource singleton as the loaders pass it through */
s32 ef_resource_bind_references(EfResourceManager* self);
/* untyped: opaque handle - the resource singleton and the loaded file */
s32 ef_resource_add_effect_project(void* work, void* data);
/* untyped: opaque handle - the resource singleton and the loaded file */
s32 ef_resource_add_texture_project(void* work, void* data);
/* untyped: opaque handle - the resource singleton and the project to unlink */
s32 ef_resource_remove_effect_project(void* work, void* target);
/* untyped: opaque handle - the resource singleton and the project to unlink */
s32 ef_resource_remove_texture_project(void* work, void* target);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_RESOURCE_H */
