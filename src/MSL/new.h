/*
 * MSL/new.h - MSL's `<new>` placement array forms.  Both are inline, so every unit that uses them emits its own
 *   weak copy beside the first function that calls it (`ef/ef_effectsystem.cpp` keeps 0x800A5908/0x800A5940).
 */
#ifndef MHTRI_MSL_NEW_H
#define MHTRI_MSL_NEW_H

/* Returns the caller's block unchanged. */
/* untyped: placement allocation hands back the caller-owned byte range */
inline void* operator new[](unsigned long size, void* block) {
    (void)size;
    return block;
}

/* Releases nothing: the caller owns the block. */
/* untyped: placement release of a caller-owned byte range */
inline void operator delete[](void* array, void* block) {
    (void)array;
    (void)block;
}

#endif
