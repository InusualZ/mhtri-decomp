/*
 * DWCi/hashtable.c - the GameSpy hash table: a table of darray buckets with create, free, enter, remove, lookup and
 *   the two map walks.
 *
 * RANGE. `.text` 0x8050B460..0x8050B880 (7 functions); no data of its own. The left edge follows the dynamic array
 *   (`DWCi/dwc_nasfunc.c`), the right edge is the platform layer (`DWCi/gsPlatform.c`). Its own TU: every darray call
 *   stays a `bl`, even `ArrayNth` (0x30 B), which `-inline auto` folds into any later caller in the same TU.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names (`TableNew2`, `TableFree`, `TableEnter`, `TableRemove`, `TableLookup`, `TableMapSafe`,
 *   `TableMapSafe2`). The file name is the GameSpy SDK's `hashtable.c` (no `__FILE__` string).
 * RESIDUALS. none measured (every row matches).
 */
#include "DWCi/hashtable.h"
#include "DWCi/dwc_nasfunc.h"

/* 0x8050B460 (0xA4): a table of `nBuckets` arrays of `elemSize`-byte elements. */
HashTable TableNew2(int elemSize, int nBuckets, int nChains, TableHashFn hashFn, TableCompareFn compFn,
                    TableElementFreeFn freeFn) {
    HashTable table;
    int i;

    table = (HashTable)gsimalloc(sizeof(struct HashImplementation));
    table->buckets = (DArray*)gsimalloc(nBuckets * sizeof(DArray));
    for (i = 0; i < nBuckets; i++) {
        table->buckets[i] = ArrayNew(elemSize, nChains, freeFn);
    }
    table->nbuckets = nBuckets;
    table->freefn = freeFn;
    table->compfn = compFn;
    table->hashfn = hashFn;
    return table;
}

/* 0x8050B510 (0x7C): frees every bucket and the table. */
void TableFree(HashTable table) {
    int i;
    if (table != NULL) {
        for (i = 0; i < table->nbuckets; i++) {
            ArrayFree(table->buckets[i]);
        }
        gsifree(table->buckets);
        gsifree(table);
    }
}

/* 0x8050B590 (0xA8): adds a copy of `newElem`, replacing an equal element. */
/* untyped: caller-owned payload - the element copied in */
void TableEnter(HashTable table, const void* newElem) {
    int hash;
    int itempos;
    if (table != NULL) {
        hash = table->hashfn(newElem, table->nbuckets);
        itempos = ArraySearch(table->buckets[hash], newElem, table->compfn, 0, 0);
        if (itempos == -1) {
            ArrayAppend(table->buckets[hash], newElem);
        } else {
            ArrayReplaceAt(table->buckets[hash], newElem, itempos);
        }
    }
}

/* 0x8050B640 (0xA4): removes the element equal to `delElem`; 1 when one was found. */
/* untyped: caller-owned payload - the key */
int TableRemove(HashTable table, const void* delElem) {
    int hash;
    int itempos;
    if (table == NULL) {
        return 0;
    }
    hash = table->hashfn(delElem, table->nbuckets);
    itempos = ArraySearch(table->buckets[hash], delElem, table->compfn, 0, 0);
    if (itempos == -1) {
        return 0;
    }
    ArrayDeleteAt(table->buckets[hash], itempos);
    return 1;
}

/* 0x8050B6F0 (0xA0): the element equal to `elemKey`, or NULL. */
/* untyped: caller-owned payload - the key and the element returned */
void* TableLookup(HashTable table, const void* elemKey) {
    int hash;
    int itempos;
    if (table == NULL) {
        return NULL;
    }
    hash = table->hashfn(elemKey, table->nbuckets);
    itempos = ArraySearch(table->buckets[hash], elemKey, table->compfn, 0, 0);
    if (itempos == -1) {
        return NULL;
    }
    return ArrayNth(table->buckets[hash], itempos);
}

/* 0x8050B790 (0x6C): calls `fn` on every element (each bucket last first). */
/* untyped: caller-owned payload - the client data */
void TableMapSafe(HashTable table, TableMapFn fn, void* clientData) {
    int i;
    for (i = 0; i < table->nbuckets; i++) {
        ArrayMapBackwards(table->buckets[i], fn, clientData);
    }
}

/* 0x8050B800 (0x7C): calls `fn` on every element and returns the first one it answers 0 for. */
/* untyped: caller-owned payload - the client data and the element returned */
void* TableMapSafe2(HashTable table, TableMapFn2 fn, void* clientData) {
    int i;
    void* elem;
    for (i = 0; i < table->nbuckets; i++) {
        elem = ArrayMapBackwards2(table->buckets[i], fn, clientData);
        if (elem != NULL) {
            return elem;
        }
    }
    return NULL;
}
