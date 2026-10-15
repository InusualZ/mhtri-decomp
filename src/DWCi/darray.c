/*
 * DWCi/darray.c - the GameSpy dynamic array: create, free, length, element access, append, sorted insert, remove,
 *   delete, replace, search and the two backward map walks.
 *
 * RANGE. `.text` 0x8050AAF0..0x8050B460 (12 functions); no data of its own. The left edge follows the service
 *   locator (`DWCi/dwc_nasfunc.c`; its `.sbss` word `DWCi_svlResult` is padded to 8 bytes, an object end), the right
 *   edge is the hash table (`DWCi/hashtable.c`, which calls every array function out of line).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names (`Array*`). The file name is the GameSpy SDK's `darray.c` (no `__FILE__` string).
 * RESIDUALS. `ArraySearch` saves from r22 where retail saves from r23 (`_savegpr_22`/`_restgpr_22`);
 *   `ArrayInsertSorted` differs in register colouring.
 * SHAPES. The bounds-checked element address and the insert are static inlines (retail expands them in every
 *   caller).
 */
#include "DWCi/darray.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "unsplit/Runtime.PPCEABI.H.h"

/* The address of element `n`, or NULL outside the array. */
/* untyped: caller-owned payload - the element */
static inline void* ArrayNthInline(DArray array, int n) {
    if (!(n >= 0 && n < array->count)) {
        return NULL;
    }
    return (char*)array->list + array->elemsize * n;
}

/* 0x8050AAF0 (0x9C): a new array of `elemSize`-byte elements growing by `numElemsToAllocate` (8 for 0). */
DArray ArrayNew(int elemSize, int numElemsToAllocate, ArrayElementFreeFn elemFreeFn) {
    DArray array = (DArray)gsimalloc(sizeof(struct DArrayImplementation));
    if (numElemsToAllocate == 0) {
        numElemsToAllocate = 8;
    }
    array->count = 0;
    array->capacity = numElemsToAllocate;
    array->elemsize = elemSize;
    array->growby = numElemsToAllocate;
    array->elemfreefn = elemFreeFn;
    if (array->capacity != 0) {
        array->list = gsimalloc(array->capacity * array->elemsize);
    } else {
        array->list = NULL;
    }
    return array;
}

/* 0x8050AB90 (0x94): frees every element through the array's free function, then the array. */
void ArrayFree(DArray array) {
    int i;
    for (i = 0; i < array->count; i++) {
        if (array->elemfreefn != NULL) {
            array->elemfreefn(ArrayNthInline(array, i));
        }
    }
    gsifree(array->list);
    gsifree(array);
}

/* 0x8050AC30 (0x8): the element count. */
int ArrayLength(DArray array) {
    return array->count;
}

/* 0x8050AC40 (0x30): the address of element `n`, or NULL outside the array. */
/* untyped: caller-owned payload - the element */
void* ArrayNth(DArray array, int n) {
    if (n < 0 || n >= array->count) {
        return NULL;
    }
    return (char*)array->list + array->elemsize * n;
}

/* Inserts a copy of `newElem` before element `n`, growing the array when it is full. */
/* untyped: caller-owned payload - the element copied in */
static inline void ArrayInsertAt(DArray array, const void* newElem, int n) {
    if (array->count == array->capacity) {
        array->capacity += array->growby;
        array->list = gsirealloc(array->list, array->capacity * array->elemsize);
    }
    array->count++;
    if (n < array->count - 1) {
        memmove(ArrayNthInline(array, n + 1), ArrayNthInline(array, n), (array->count - 1 - n) * array->elemsize);
    }
    memcpy(ArrayNthInline(array, n), newElem, array->elemsize);
}

/* 0x8050AC70 (0x124): appends a copy of `newElem`. */
/* untyped: caller-owned payload - the element copied in */
void ArrayAppend(DArray array, const void* newElem) {
    if (array != NULL) {
        ArrayInsertAt(array, newElem, array->count);
    }
}

/* 0x8050ADA0 (0x180): inserts a copy of `newElem` at its sorted place (binary search with `comparator`). */
/* untyped: caller-owned payload - the element copied in */
void ArrayInsertSorted(DArray array, const void* newElem, ArrayCompareFn comparator) {
    char* base = (char*)array->list;
    int elemsize = array->elemsize;
    int low = 0;
    int high = array->count - 1;
    int mid;
    int result;
    int n;

    while (low <= high) {
        mid = (low + high) >> 1;
        result = comparator(base + mid * elemsize, newElem);
        if (result < 0) {
            low = mid + 1;
        }
        if (result >= 0) {
            high = mid - 1;
        }
    }
    n = (base + low * elemsize - (char*)array->list) / array->elemsize;
    ArrayInsertAt(array, newElem, n);
}

/* 0x8050AF20 (0xAC): removes element `n` without freeing it. */
void ArrayRemoveAt(DArray array, int n) {
    if (n < array->count - 1) {
        memmove(ArrayNthInline(array, n), ArrayNthInline(array, n + 1), (array->count - 1 - n) * array->elemsize);
    }
    array->count--;
}

/* 0x8050AFD0 (0xF0): frees element `n` through the array's free function and removes it. */
void ArrayDeleteAt(DArray array, int n) {
    if (array->elemfreefn != NULL) {
        array->elemfreefn(ArrayNthInline(array, n));
    }
    ArrayRemoveAt(array, n);
}

/* 0x8050B0C0 (0xB8): frees element `n` and overwrites it with a copy of `newElem`. */
/* untyped: caller-owned payload - the element copied in */
void ArrayReplaceAt(DArray array, const void* newElem, int n) {
    if (array->elemfreefn != NULL) {
        array->elemfreefn(ArrayNthInline(array, n));
    }
    memcpy(ArrayNthInline(array, n), newElem, array->elemsize);
}

/* 0x8050B180 (0x198): the index of the element matching `key` from `fromIndex` on (binary search when
 * `isSorted`), or -1. */
/* untyped: caller-owned payload - the key */
int ArraySearch(DArray array, const void* key, ArrayCompareFn comparator, int fromIndex, int isSorted) {
    char* res;
    int found = 1;
    int count;

    if (array == NULL || array->count == 0) {
        return -1;
    }
    if (isSorted) {
        char* base = (char*)ArrayNthInline(array, fromIndex);
        int elemsize = array->elemsize;
        int low = 0;
        int high = array->count - fromIndex - 1;
        int mid;
        int result;

        found = 0;
        while (low <= high) {
            mid = (low + high) >> 1;
            result = comparator(base + mid * elemsize, key);
            if (result == 0) {
                found = 1;
            }
            if (result < 0) {
                low = mid + 1;
            }
            if (result >= 0) {
                high = mid - 1;
            }
        }
        res = base + low * elemsize;
    } else {
        char* base = (char*)ArrayNthInline(array, fromIndex);
        int elemsize = array->elemsize;
        int i;

        count = array->count - fromIndex;
        res = NULL;
        for (i = 0; i < count; i++) {
            if (comparator(key, base + elemsize * i) == 0) {
                res = base + elemsize * i;
                break;
            }
        }
    }
    if (res != NULL && found) {
        return (res - (char*)array->list) / array->elemsize;
    }
    return -1;
}

/* 0x8050B320 (0x94): calls `fn` on every element, last first. */
/* untyped: caller-owned payload - the client data */
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void* clientData) {
    int i;
    for (i = array->count - 1; i >= 0; i--) {
        fn(ArrayNthInline(array, i), clientData);
    }
}

/* 0x8050B3C0 (0x9C): calls `fn` on every element, last first, and returns the first one it answers 0 for. */
/* untyped: caller-owned payload - the client data and the element returned */
void* ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void* clientData) {
    int i;
    void* elem;
    for (i = array->count - 1; i >= 0; i--) {
        elem = ArrayNthInline(array, i);
        if (!fn(elem, clientData)) {
            return elem;
        }
    }
    return NULL;
}
