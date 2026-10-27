/*
 * Types of `src/Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` (MSL's Gecko C++ exception runtime): the
 * exception-table record layouts and the action records its walkers and unwinder read.
 */
#ifndef MHTRI_RUNTIME_PPCEABI_H_GECKO_EXCEPTIONPPC_H
#define MHTRI_RUNTIME_PPCEABI_H_GECKO_EXCEPTIONPPC_H

#include "types.h"

#define RETURN_ADDRESS 4


/* size: 0x10 */
typedef struct __eti_init_info {
    void* eti_start;         /* +0x00 */
    void* eti_end;           /* +0x04 */
    char* code_start;        /* +0x08 */
    unsigned long code_size; /* +0x0C */
} __eti_init_info;

/* size: 0x6 */
typedef struct ExceptionRangeSmall {
    u16 start;  /* +0x00 */
    u16 end;    /* +0x02 */
    u16 action; /* +0x04 */
} ExceptionRangeSmall;

/* size: 0x2 */
typedef struct ExceptionTableSmall {
    u16 et_field;                  /* +0x00 */
    ExceptionRangeSmall ranges[0]; /* +0x02 */
} ExceptionTableSmall;

/* size: 0x8 */
typedef struct ExceptionRangeLarge {
    u32 start;  /* +0x00 */
    u16 size;   /* +0x04 */
    u16 action; /* +0x06 */
} ExceptionRangeLarge;

/* size: 0x4 */
typedef struct ExceptionTableLarge {
    u16 et_field;                 /* +0x00 */
    u16 et_field2;                /* +0x02 */
    ExceptionRangeLarge ranges[]; /* +0x04 */
} ExceptionTableLarge;

/* size: 0xC */
typedef struct ExceptionTableIndex {
    u32 functionoffset;  /* +0x00 */
    u32 eti_field;       /* +0x04 */
    u32 exceptionoffset; /* +0x08 */
} ExceptionTableIndex;

/* size: 0x18 */
typedef struct MWExceptionInfo {
    ExceptionTableSmall* exception_record; /* +0x00 */
    char* current_function;                /* +0x04 */
    char* action_pointer;                  /* +0x08 */
    char* code_section;                    /* +0x0C */
    char* data_section;                    /* +0x10 */
    char* TOC;                             /* +0x14 */
} MWExceptionInfo;

/* size: 0x20 */
typedef struct FragmentInfo {
    ExceptionTableIndex* exception_start; /* +0x00 */
    ExceptionTableIndex* exception_end;   /* +0x04 */
    char* code_start;                     /* +0x08 */
    char* code_end;                       /* +0x0C */
    char* data_start;                     /* +0x10 */
    char* data_end;                       /* +0x14 */
    char* TOC;                            /* +0x18 */
    int active;                           /* +0x1C */
} FragmentInfo;

/* size: 0xC */
typedef struct ProcessInfo {
    __eti_init_info* exception_info; /* +0x00 */
    char* TOC;                       /* +0x04 */
    int active;                      /* +0x08 */
} ProcessInfo;

/* size: 0x24 */
typedef struct ActionIterator {
    MWExceptionInfo info; /* +0x00 */
    char* current_SP;     /* +0x18 */
    char* current_FP;     /* +0x1C */
    s32 current_R31;      /* +0x20 */
} ActionIterator;

/* et-field getters this file reads; see the note on the layout in the header. */
#define ET_GetSavedGPRs(field)   ((field) >> 11)
#define ET_GetSavedFPRs(field)   (((field) >> 6) & 0x1f)
#define ET_GetHasFramePtr(field) (((field) >> 4) & 0x1)
#define ET_IsLargeTable(field)   (((field) >> 3) & 0x1)

#define ETI_GetDirectStore(field)  ((field) >> 31)
#define ETI_GetFunctionSize(field) ((field) & 0x7fffffff)

typedef u8 exaction_type;

#define EXACTION_ENDBIT 0x80
#define EXACTION_MASK   0x7F

#define EXACTION_ENDOFLIST          0
#define EXACTION_BRANCH             1
#define EXACTION_DESTROYLOCAL       2
#define EXACTION_DESTROYLOCALCOND   3
#define EXACTION_DESTROYLOCALPOINTER 4
#define EXACTION_DESTROYLOCALARRAY  5
#define EXACTION_DESTROYBASE        6
#define EXACTION_DESTROYMEMBER      7
#define EXACTION_DESTROYMEMBERCOND  8
#define EXACTION_DESTROYMEMBERARRAY 9
#define EXACTION_DELETEPOINTER      10
#define EXACTION_DELETEPOINTERCOND  11
#define EXACTION_CATCHBLOCK         12
#define EXACTION_ACTIVECATCHBLOCK   13
#define EXACTION_TERMINATE          14
#define EXACTION_SPECIFICATION      15
#define EXACTION_CATCHBLOCK_32      16

/* The cleanup records `ExPPC_NextAction` steps over; only their sizes are used, each is the number of bytes
 * the action pointer advances by.  The sizes are the `addi r0,r4,N` immediates of that function's switch,
 * in source-case order: 8, 12, 8, 12, 12, 16, 20, 8, 12, 12, 16, 4, and 12 + specs * 4. */
/* size: 0x4 */
typedef struct ex_branch {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    u16 target;           /* +0x02 */
} ex_branch;

/* size: 0x8 */
typedef struct ex_destroylocal {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    s16 local;            /* +0x02 */
    void* dtor;           /* +0x04 */
} ex_destroylocal;

/* size: 0xC */
typedef struct ex_destroylocalcond {
    exaction_type action; /* +0x00 */
    u8 dlc_field;         /* +0x01 */
    s16 cond;             /* +0x02 */
    s16 local;            /* +0x04 */
    void* dtor;           /* +0x08 */
} ex_destroylocalcond;

/* size: 0x8 */
typedef struct ex_destroylocalpointer {
    exaction_type action; /* +0x00 */
    u8 dlp_field;         /* +0x01 */
    s16 pointer;          /* +0x02 */
    void* dtor;           /* +0x04 */
} ex_destroylocalpointer;

/* size: 0xC */
typedef struct ex_destroylocalarray {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    s16 localarray;       /* +0x02 */
    u16 elements;         /* +0x04 */
    u16 element_size;     /* +0x06 */
    void* dtor;           /* +0x08 */
} ex_destroylocalarray;

/* size: 0xC */
typedef struct ex_destroymember {
    exaction_type action; /* +0x00 */
    u8 dm_field;          /* +0x01 */
    s16 objectptr;        /* +0x02 */
    s32 offset;           /* +0x04 */
    void* dtor;           /* +0x08 */
} ex_destroymember;

/* size: 0x10 */
typedef struct ex_destroymembercond {
    exaction_type action; /* +0x00 */
    u8 dmc_field;         /* +0x01 */
    s16 cond;             /* +0x02 */
    s16 objectptr;        /* +0x04 */
    s32 offset;           /* +0x08 */
    void* dtor;           /* +0x0C */
} ex_destroymembercond;

/* size: 0x14 */
typedef struct ex_destroymemberarray {
    exaction_type action; /* +0x00 */
    u8 dma_field;         /* +0x01 */
    s16 objectptr;        /* +0x02 */
    s32 offset;           /* +0x04 */
    s32 elements;         /* +0x08 */
    s32 element_size;     /* +0x0C */
    void* dtor;           /* +0x10 */
} ex_destroymemberarray;

/* size: 0x8 */
typedef struct ex_deletepointer {
    exaction_type action; /* +0x00 */
    u8 dp_field;          /* +0x01 */
    s16 objectptr;        /* +0x02 */
    void* deletefunc;     /* +0x04 */
} ex_deletepointer;

/* size: 0xC */
typedef struct ex_deletepointercond {
    exaction_type action; /* +0x00 */
    u8 dpc_field;         /* +0x01 */
    s16 cond;             /* +0x02 */
    s16 objectptr;        /* +0x04 */
    void* deletefunc;     /* +0x08 */
} ex_deletepointercond;

/* size: 0xC */
typedef struct ex_catchblock {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    char* catch_type;     /* +0x04 */
    u16 catch_pcoffset;   /* +0x08 */
    s16 cinfo_ref;        /* +0x0A */
} ex_catchblock;

/* size: 0x4 */
typedef struct ex_activecatchblock {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    s16 cinfo_ref;        /* +0x02 */
} ex_activecatchblock;

/* size: 0x10 */
typedef struct ex_catchblock_32 {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    char* catch_type;     /* +0x04 */
    s32 catch_pcoffset;   /* +0x08 */
    s32 cinfo_ref;        /* +0x0C */
} ex_catchblock_32;

/* size: 0xC */
typedef struct ex_specification {
    exaction_type action; /* +0x00 */
    u8 unused;            /* +0x01 */
    u16 specs;            /* +0x02 */
    s32 pcoffset;         /* +0x04 */
    s32 cinfo_ref;        /* +0x08 */
    char* spec[];         /* +0x0C */
} ex_specification;

#endif /* MHTRI_RUNTIME_PPCEABI_H_GECKO_EXCEPTIONPPC_H */
