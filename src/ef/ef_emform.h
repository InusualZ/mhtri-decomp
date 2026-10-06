/*
 * ef/ef_emform.h - the emitter-form classes: the abstract `nw4r::ef::EmitterForm` (its table and its implicit
 * constructor come out of `ef/ef_emform.cpp`), the seven shapes deriving from it (each shape's `Emission` is the
 * key function, so each shape unit emits its own table), and `EmitterFormBuilder`, whose `Create` maps a shape id
 * to its registered instance (`ef/ef_emform.cpp`; its constructor is `ef/ef_effectsystem.cpp`'s).
 */
#ifndef MHTRI_EF_EF_EMFORM_H
#define MHTRI_EF_EF_EMFORM_H

#include "ef.h"                   /* EfWork, Vec */
#include "ef/ef_particlemanager.h" /* ParticleManager */

namespace nw4r {
namespace ef {

/* The abstract emitter form: a table pointer and nothing else.  `CalcVelocity`/`CalcLife` are the shared
 * helpers `ef/ef_emitterform.cpp` defines. */
class EmitterForm {
public:
    /* +0x00: the vtable pointer */
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space) = 0;

    void CalcVelocity(Vec* out, EfWork* em, Vec* pos, Vec* normal, Vec* fromOrigin, Vec* fromYAxis);
    u16 CalcLife(u16 life, f32 lifeRnd, EfWork* em);
}; /* size: 0x4 */

/* The disc shape (`ef/ef_disc.cpp`). */
class EmitterFormDisc : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The line shape (`ef/ef_line.cpp`). */
class EmitterFormLine : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The cylinder shape (`ef/ef_cylinder.cpp`). */
class EmitterFormCylinder : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The sphere shape (`ef/ef_sphere.cpp`). */
class EmitterFormSphere : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The torus shape (`ef/ef_torus.cpp`). */
class EmitterFormTorus : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The cube shape (`ef/ef_cube.cpp`). */
class EmitterFormCube : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* The point shape (`ef/ef_point.cpp`). */
class EmitterFormPoint : public EmitterForm {
public:
    virtual void Emission(EfWork* em, ParticleManager* pm, int count, u32 flags, f32* params, u16 life,
                          f32 lifeRnd, const MTX34* space);
}; /* size: 0x4 */

/* Maps a shape id (0 disc, 1 line, 5 cube, 7 cylinder, 8 sphere, 9 point, 10 torus) to its registered
 * instance. */
class EmitterFormBuilder {
public:
    EmitterFormBuilder();
    /* +0x00: the vtable pointer */
    virtual EmitterForm* Create(int id);
}; /* size: 0x4 */

} // namespace ef
} // namespace nw4r

/* `EmitterFormBuilder`'s table, under the map's name: `ef/ef_effectsystem.cpp`'s static initializer still builds
 * its builder instance by storing it (the instance becomes a real static once that unit's statics are classes). */
extern void* lbl_80594EC4[];

#endif /* MHTRI_EF_EF_EMFORM_H */
