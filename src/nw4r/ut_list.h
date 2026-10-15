/*
 * nw4r/ut_list.h - nw4r::ut's offset-based intrusive list, whose functions `nw4r/ut_list.cpp` defines.
 */
#ifndef NW4R_UT_LIST_H
#define NW4R_UT_LIST_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* The link record an element of an offset-based List carries. size: 0x8 */
struct Link {
    /* untyped: the neighbouring elements, whatever type the list holds */
    /* +0x0 */ void* prevObject;
    /* +0x4 */ void* nextObject;
};

/* An intrusive doubly linked list of elements whose Link sits `offset` bytes in. size: 0xC */
struct List {
    /* untyped: the end elements, whatever type the list holds */
    /* +0x0 */ void* headObject;
    /* +0x4 */ void* tailObject;
    /* +0x8 */ u16 numObjects;
    /* +0xA */ u16 offset;
};

/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...) */
void List_Init(List* list, u16 offset);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Append(List* list, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Insert(List* list, void* target, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Remove(List* list, void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNext(const List* list, const void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetPrev(const List* list, const void* object);
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNth(const List* list, u16 index);

}  // namespace ut
}  // namespace nw4r
#endif

#endif
