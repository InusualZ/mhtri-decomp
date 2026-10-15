/*
 * DWCi/hashtable.h - the declarations `src/DWCi/hashtable.c` owns (docs/plan.md 6.5 rule 2): the GameSpy hash table
 *   over darray buckets, its record and callbacks.
 */
#ifndef MHTRI_DWCI_HASHTABLE_H
#define MHTRI_DWCI_HASHTABLE_H

#include "types.h"
#include "DWCi/darray.h"

/* The GameSpy hash table: an array of darray buckets and the element callbacks. */
/* untyped: caller-owned payload - the element hashed */
typedef int (*TableHashFn)(const void* elem, int numBuckets);
typedef ArrayCompareFn TableCompareFn;
typedef ArrayElementFreeFn TableElementFreeFn;
typedef ArrayMapFn TableMapFn;
typedef ArrayMapFn2 TableMapFn2;
struct HashImplementation {
    /* +0x00 */ DArray* buckets;
    /* +0x04 */ int nbuckets;
    /* +0x08 */ TableElementFreeFn freefn;
    /* +0x0C */ TableHashFn hashfn;
    /* +0x10 */ TableCompareFn compfn;
}; /* size: 0x14 */
typedef struct HashImplementation* HashTable;

#ifdef __cplusplus
extern "C" {
#endif

HashTable TableNew2(int elemSize, int nBuckets, int nChains, TableHashFn hashFn, TableCompareFn compFn,
                    TableElementFreeFn freeFn);
void TableFree(HashTable table);
/* untyped: caller-owned payload - the element copied in */
void TableEnter(HashTable table, const void* newElem);
/* untyped: caller-owned payload - the key */
int TableRemove(HashTable table, const void* delElem);
/* untyped: caller-owned payload - the key and the element returned */
void* TableLookup(HashTable table, const void* elemKey);
/* untyped: caller-owned payload - the client data */
void TableMapSafe(HashTable table, TableMapFn fn, void* clientData);
/* untyped: caller-owned payload - the client data and the element returned */
void* TableMapSafe2(HashTable table, TableMapFn2 fn, void* clientData);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_HASHTABLE_H */
