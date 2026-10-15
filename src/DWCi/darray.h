/*
 * DWCi/darray.h - the declarations `src/DWCi/darray.c` owns (docs/plan.md 6.5 rule 2): the GameSpy dynamic array,
 *   its record and element callbacks.
 */
#ifndef MHTRI_DWCI_DARRAY_H
#define MHTRI_DWCI_DARRAY_H

#include "types.h"

/* The GameSpy dynamic array: count, capacity, element size, growth step, element free function and the element
 * storage. */
/* untyped: caller-owned payload - the element */
typedef void (*ArrayElementFreeFn)(void* elem);
/* untyped: caller-owned payload - the two elements compared */
typedef int (*ArrayCompareFn)(const void* elem1, const void* elem2);
/* untyped: caller-owned payload - the element and the client data */
typedef void (*ArrayMapFn)(void* elem, void* clientData);
/* untyped: caller-owned payload - the element and the client data */
typedef int (*ArrayMapFn2)(void* elem, void* clientData);
struct DArrayImplementation {
    /* +0x00 */ int count;
    /* +0x04 */ int capacity;
    /* +0x08 */ int elemsize;
    /* +0x0C */ int growby;
    /* +0x10 */ ArrayElementFreeFn elemfreefn;
    /* +0x14 */ void* list;
}; /* size: 0x18 */
typedef struct DArrayImplementation* DArray;

#ifdef __cplusplus
extern "C" {
#endif

DArray ArrayNew(int elemSize, int numElemsToAllocate, ArrayElementFreeFn elemFreeFn);
void ArrayFree(DArray array);
int ArrayLength(DArray array);
/* untyped: caller-owned payload - the element */
void* ArrayNth(DArray array, int n);
/* untyped: caller-owned payload - the element copied in */
void ArrayAppend(DArray array, const void* newElem);
/* untyped: caller-owned payload - the element copied in */
void ArrayInsertSorted(DArray array, const void* newElem, ArrayCompareFn comparator);
void ArrayRemoveAt(DArray array, int n);
void ArrayDeleteAt(DArray array, int n);
/* untyped: caller-owned payload - the element copied in */
void ArrayReplaceAt(DArray array, const void* newElem, int n);
/* untyped: caller-owned payload - the key */
int ArraySearch(DArray array, const void* key, ArrayCompareFn comparator, int fromIndex, int isSorted);
/* untyped: caller-owned payload - the client data */
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void* clientData);
/* untyped: caller-owned payload - the client data and the element returned */
void* ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void* clientData);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DARRAY_H */
