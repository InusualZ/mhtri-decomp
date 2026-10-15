/*
 * nw4r/ut_list.cpp - nw4r::ut's offset-based intrusive list (List_Init .. List_GetNth).
 * RANGE. .text 0x80501A4C-0x80501CE8 (7 functions); no data.
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_list.cpp. GUESS from the nw4r source, read off the bodies: List_Init (the map's
 *   MEMInitList twin) .. List_GetNth (fn_80501C9C).
 * SHAPES. GetLink is a macro: as an inline function its argument copies swap List_Remove's r5/r6.
 */

#include "nw4r/ut_list.h"

namespace nw4r {
namespace ut {

/* The link record inside `object`, `list->offset` bytes in. */
#define GetLink(list, object) reinterpret_cast<Link*>(reinterpret_cast<u32>(object) + (list)->offset)

/* Makes `object` the only element of an empty list. */
/* untyped: caller-owned payload - the list holds elements of any type */
static inline void SetFirstObject(List* list, void* object) {
    Link* link = GetLink(list, object);

    link->nextObject = NULL;
    link->prevObject = NULL;
    list->headObject = object;
    list->tailObject = object;
    list->numObjects++;
}

/* 0x80501A4C (0x18): empties a list whose elements keep their link `offset` bytes in. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...) */
void List_Init(List* list, u16 offset) {
    list->headObject = NULL;
    list->tailObject = NULL;
    list->numObjects = 0;
    list->offset = offset;
}

/* 0x80501A64 (0x70): appends `object` at the tail. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Append(List* list, void* object) {
    if (list->headObject == NULL) {
        SetFirstObject(list, object);
    } else {
        Link* link = GetLink(list, object);

        link->prevObject = list->tailObject;
        link->nextObject = NULL;
        GetLink(list, list->tailObject)->nextObject = object;
        list->tailObject = object;
        list->numObjects++;
    }
}

/* Prepends `object` at the head. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
static inline void List_Prepend(List* list, void* object) {
    if (list->headObject == NULL) {
        SetFirstObject(list, object);
    } else {
        Link* link = GetLink(list, object);

        link->prevObject = NULL;
        link->nextObject = list->headObject;
        GetLink(list, list->headObject)->prevObject = object;
        list->headObject = object;
        list->numObjects++;
    }
}

/* 0x80501AD4 (0x120): links `object` in front of `target`, or at the tail when `target` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Insert(List* list, void* target, void* object) {
    if (target == NULL) {
        List_Append(list, object);
    } else if (target == list->headObject) {
        List_Prepend(list, object);
    } else {
        Link* link = GetLink(list, object);
        void* prev = GetLink(list, target)->prevObject;
        Link* prevLink = GetLink(list, prev);

        link->prevObject = prev;
        link->nextObject = target;
        prevLink->nextObject = object;
        GetLink(list, target)->prevObject = object;
        list->numObjects++;
    }
}

/* 0x80501BF4 (0x6C): unlinks `object`. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Remove(List* list, void* object) {
    Link* link = GetLink(list, object);

    if (link->prevObject == NULL) {
        list->headObject = link->nextObject;
    } else {
        GetLink(list, link->prevObject)->nextObject = link->nextObject;
    }
    if (link->nextObject == NULL) {
        list->tailObject = link->prevObject;
    } else {
        GetLink(list, link->nextObject)->prevObject = link->prevObject;
    }
    link->prevObject = NULL;
    link->nextObject = NULL;
    list->numObjects--;
}

/* 0x80501C60 (0x20): the element after `object`, or the head when `object` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNext(const List* list, const void* object) {
    if (object == NULL) {
        return list->headObject;
    }
    return GetLink(list, object)->nextObject;
}

/* 0x80501C80 (0x1C): the element before `object`, or the tail when `object` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetPrev(const List* list, const void* object) {
    if (object == NULL) {
        return list->tailObject;
    }
    return GetLink(list, object)->prevObject;
}

/* 0x80501C9C (0x4C): the element at `index`, or NULL past the end. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNth(const List* list, u16 index) {
    int count = 0;
    void* object = NULL;

    while ((object = List_GetNext(list, object)) != NULL) {
        if (index == count) {
            return object;
        }
        count++;
    }
    return NULL;
}

}  // namespace ut
}  // namespace nw4r
