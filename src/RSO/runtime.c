/*
 * RSO runtime (DOL side) -- Monster Hunter Tri, RMHE08.
 *
 * The DOL-side RSO loader/linker: it relocates a loaded module's symbol tables against the sections
 * that are already in memory.  `.text` 0x804D9B4C..0x804DAE40 (0x12F4), nine functions in retail
 * order: LocateObject, RSOStaticLocateObject, RSOUnLocateObject, RSOLink, RSOUnLink, fn_804DA7E4,
 * FindExportIndex, RSORelocate, RSORelocateSmallDataSection.
 *
 * Names and struct layouts come from the shared memory dump (`docs/memory-dump.md`), except
 * fn_804DA7E4 which has no name there.  A rename is always two edits - this file and
 * config/RMHE08/symbols.txt (skill: symbol-map-editing).
 *
 * Flags and compiler live in configure.py (`cflags_rso`, `mw_version: GC/3.0a3`), with the evidence in
 * the comment there; the short version is that this unit needs peephole + scheduling + level 4 where
 * Camellia needs `-opt nopeephole` (playbook 17 and 21).
 *
 * Residuals: five of the nine functions are byte-identical (RSOLink, RSOUnLink, fn_804DA7E4,
 * FindExportIndex, RSOUnLocateObject).  The other four are instruction-identical and lose only
 * register colouring (LocateObject, RSORelocate) or colouring plus one folded `addi` / a pool-base
 * swap (RSOStaticLocateObject, RSORelocateSmallDataSection); two of them also carry objdiff rows that
 * are nothing but the target's synthetic data-symbol names.  All four are allocator state, not source
 * shape - hundreds of source shapes and all 31 installed compilers give the same stream - so they are
 * recorded here rather than chased.  Numbers, sweeps and probes: `.pi/scratch/rso/`.
 *
 * Load-bearing source shapes (do not "clean up" without re-measuring): RSOLink's `idx == -1` is a value
 * merge and its `offset` a real second induction variable; RSOUnLink routes `(u32)pObject + 0x54`
 * through a `u32 buf[1]`; RSORelocate keeps `u32 value` as its third parameter and needs its case-10
 * comparison order; LocateObject needs RSOHdr's relocation-table field pointer-typed.
 *
 * TU boundary unproven: the four `RSONotify*` thunks immediately before the range may belong here, and
 * the `unlink_rso_module` group after it is a separate unit.
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef int BOOL;

#define TRUE 1
#define FALSE 0

/* 12-byte entry of the import symbol table at RSOModule::import_symbol_table_offset (real SDK
 * layout: dolphin's RSO.h `RSOImport`).  This is the array every function of this unit walks with
 * the *12 stride -- RSOLink, RSOUnLink, fn_804DA7E4, LocateObject.  RSOLink/RSOUnLink overwrite
 * code_offset with the resolved symbol address, and entry_offset chains the external relocation
 * records (RSORelocation.info >> 8) that belong to this import. */
typedef struct RSOImport {
    u32 name_offset;  /* +0x00: offset into the import symbol name table (+0x54) */
    u32 code_offset;  /* +0x04: resolved symbol address, written by RSOLink / RSOUnLink */
    u32 entry_offset; /* +0x08: offset of this import's first external relocation record */
} RSOImport;

/* 12-byte relocation record, the array RSORelocate patches.  Real SDK layout: the ELF
 * r_offset/r_info/r_addend triple, dolphin's RSO.h `RSOInternalsEntry` /
 * `RSOExternalsEntry` (struct RSORelocationTableEntry).  +0x04's low byte is the R_PPC_* type,
 * its upper 24 bits are the section index (internal) or import symbol index (external). */
typedef struct RSORelocation {
    u32 offset; /* +0x00: address of the word/halfword to patch */
    u32 info;   /* +0x04: relocation type in the low byte, index in the upper 24 bits */
    u32 addend; /* +0x08: value added to the symbol address */
} RSORelocation;

/* 8-byte entry of the section table at RSOModule::section_info_offset (real SDK layout: dolphin's
 * RSO.h `RSOSection`), indexed by the section-index bytes at +0x20..0x23.  LocateObject /
 * RSOStaticLocateObject rebase offset into a runtime address, RSOUnLocateObject subtracts the base
 * again. */
typedef struct RSOSection {
    u32 offset; /* +0x00 */
    u32 size;   /* +0x04 */
} RSOSection;

/* 16-byte export symbol descriptor at RSOModule::export_symbol_table_offset (count = +0x44 / 16),
 * real SDK layout: dolphin's RSO.h `RSOExport`.  name_offset indexes the export name table at
 * +0x48, code_offset is the addend of the section offset, section_index indexes the 8-byte section
 * table at +0x0C, and hash is the ELF string hash FindExportIndex binary-searches on. */
typedef struct RSOExport {
    u32 name_offset;   /* +0x00 */
    u32 code_offset;   /* +0x04 */
    u32 section_index; /* +0x08 */
    u32 hash;          /* +0x0C */
} RSOExport;

/* RSOModule -- the SDK's 88-byte RSO module header (Ghidra MH3Shared, program
 * /DolphinDump85.raw.keep, struct RSOModule; matches the RSO file format spec and dolphin's RSO.h
 * RSOHeader).  Every offset the nine functions touch is spelled out with its real field name.  Two
 * member types deliberately differ from the annotation because this unit's codegen depends on
 * them:
 *   * section_info_offset is RSOSection* (annotation: uint) -- RSOUnLocateObject walks this table
 *     with typed indexing (`pEntry = &pObject->section_info_offset[i]`), which is load-bearing;
 *   * import_symbol_table_offset is RSOImport* (annotation: uint), for the same reason.
 * internal/external_relocation_table_offset stay u32 here (the annotation types them
 * RSORelocation*): four functions cast to RSORelocation* at the use site, and the u32 arithmetic is
 * what their matches were measured with. */
typedef struct RSOModule {
    struct RSOModule* next_module_link;   /* +0x00 */
    struct RSOModule* prev_module_link;   /* +0x04 */
    u32 section_count;                    /* +0x08: number of 8-byte section table entries */
    RSOSection* section_info_offset;      /* +0x0C: section table (pointer; see above) */
    u32 module_name_offset;               /* +0x10: rebased like a pointer, u32 (see above) */
    u32 module_name_size;                 /* +0x14 */
    u32 moduleVersion;                    /* +0x18 */
    u32 bss_size;                         /* +0x1C */
    u8 prolog_section_index;              /* +0x20: index into the section table (0 = none) */
    u8 epilog_section_index;              /* +0x21: index into the section table (0 = none) */
    u8 unresolved_section_index;          /* +0x22: index into the section table (0 = none) */
    u8 bss_section_index;                 /* +0x23: entry to clear instead of un-relocating */
    u32 prolog_function_offset;           /* +0x24 */
    u32 epilog_function_offset;           /* +0x28 */
    u32 unresolved_function_offset;       /* +0x2C: compared against import->code_offset, fn_804DA7E4 */
    u32 internal_relocation_table_offset; /* +0x30: base of a 12-byte RSORelocation table */
    u32 internal_relocation_table_size;   /* +0x34: byte size of that table */
    u32 external_relocation_table_offset; /* +0x38: base added to import->entry_offset (RSOLink) */
    u32 external_relocation_table_size;   /* +0x3C: byte size of the table at +0x38 */
    u32 export_symbol_table_offset;       /* +0x40: RSOExport table base */
    u32 export_symbol_table_size;         /* +0x44: byte size of that table */
    u32 export_symbol_names_offset;       /* +0x48: export name table base */
    RSOImport* import_symbol_table_offset;/* +0x4C: RSOImport array base (pointer; see above) */
    u32 import_symbol_table_size;         /* +0x50: byte size of the import array (count = /12) */
    u32 import_symbol_names_offset;       /* +0x54: added to import->name_offset (RSOLink) */
} RSOModule;

/* Alternate member-type view of the same 88 bytes, used for the module being linked *against*
 * (RSOLink/RSOUnLink second argument, FindExportIndex, RSORelocateSmallDataSection).  Same real
 * field names and offsets as RSOModule; the member types below are the only reason this view
 * exists: section_info_offset is a u8* (the code adds a raw byte offset to it), the relocation
 * table offsets are plain u32, and the two symbol tables are typed as their descriptor structs.
 * The 0x00..0x3F prefix is never touched through this view but is kept spelled out so the views
 * stay field-for-field comparable. */
typedef struct RSORelTable {
    struct RSOModule* next_module_link;   /* +0x00 */
    struct RSOModule* prev_module_link;   /* +0x04 */
    u32 section_count;                    /* +0x08 */
    u8* section_info_offset;              /* +0x0C: raw byte pointer (see above) */
    u32 module_name_offset;               /* +0x10 */
    u32 module_name_size;                 /* +0x14 */
    u32 moduleVersion;                    /* +0x18 */
    u32 bss_size;                         /* +0x1C */
    u8 prolog_section_index;              /* +0x20 */
    u8 epilog_section_index;              /* +0x21 */
    u8 unresolved_section_index;          /* +0x22 */
    u8 bss_section_index;                 /* +0x23 */
    u32 prolog_function_offset;           /* +0x24 */
    u32 epilog_function_offset;           /* +0x28 */
    u32 unresolved_function_offset;       /* +0x2C */
    u32 internal_relocation_table_offset; /* +0x30 */
    u32 internal_relocation_table_size;   /* +0x34 */
    u32 external_relocation_table_offset; /* +0x38 */
    u32 external_relocation_table_size;   /* +0x3C */
    RSOExport* export_symbol_table_offset;/* +0x40: 16-byte export descriptor table */
    u32 export_symbol_table_size;         /* +0x44: byte size of that table */
    u32 export_symbol_names_offset;       /* +0x48: export name table base */
    RSOImport* import_symbol_table_offset;/* +0x4C */
    u32 import_symbol_table_size;         /* +0x50 */
    u32 import_symbol_names_offset;       /* +0x54 */
} RSORelTable;

/* Full link-time view of the same 88 bytes, used only by LocateObject, the
 * "rebase every offset field of the header" step.  Same real field names/offsets as RSOModule; the
 * member types are the ones that function's codegen needs:
 *   * external_relocation_table_offset MUST stay pointer-typed: with it as u32 MWCC emits
 *     `add r6,r3,r0` in the unrolled loop at +0x1F4 where retail has `add r6,r0,r3` (7
 *     instructions, 99.077 % instead of 99.392 %);
 *   * section_info_offset, internal_relocation_table_offset and import_symbol_table_offset stay
 *     u32 (the annotation types them RSOSection *, RSORelocation * and RSOImport *): this function
 *     walks those tables as raw words, and that is the shape its 99.39 % was measured with. */
typedef struct RSOHdr {
    struct RSOModule* next_module_link;   /* +0x00 */
    struct RSOModule* prev_module_link;   /* +0x04 */
    u32 section_count;                    /* +0x08: count of 8-byte fixup entries in the table below */
    u32 section_info_offset;              /* +0x0C: 8-byte fixup table base (rebase) */
    u32 module_name_offset;               /* +0x10: (rebase) */
    u32 module_name_size;                 /* +0x14 */
    u32 moduleVersion;                    /* +0x18 */
    u32 bss_size;                         /* +0x1C: size of the caller's work buffer (memset) */
    u8 prolog_section_index;              /* +0x20: index into the 8-byte table, 0 = unused */
    u8 epilog_section_index;              /* +0x21: ditto */
    u8 unresolved_section_index;          /* +0x22: ditto */
    u8 bss_section_index;                 /* +0x23: loop-1 index recorded when the buffer takes over */
    u32 prolog_function_offset;           /* +0x24: += table[prolog_section_index] */
    u32 epilog_function_offset;           /* +0x28: += table[epilog_section_index] */
    u32 unresolved_function_offset;       /* +0x2C: += table[unresolved_section_index] */
    u32 internal_relocation_table_offset; /* +0x30: 12-byte record base (rebase) */
    u32 internal_relocation_table_size;   /* +0x34: byte size of that record area */
    u8* external_relocation_table_offset; /* +0x38: 12-byte record base (rebase) -- MUST be pointer */
    u32 external_relocation_table_size;   /* +0x3C: byte size of that record area */
    u32 export_symbol_table_offset;       /* +0x40: 16-byte export descriptor table (rebase) */
    u32 export_symbol_table_size;         /* +0x44: byte size of the descriptor table */
    u32 export_symbol_names_offset;       /* +0x48: export name table base (rebase) */
    u32 import_symbol_table_offset;       /* +0x4C: RSOImport array base (rebase) */
    u32 import_symbol_table_size;         /* +0x50: byte size of the import array */
    u32 import_symbol_names_offset;       /* +0x54: (rebase) */
} RSOHdr;


extern int strcmp(const char*, const char*);
extern void* memset(void*, int, u32);
extern void DCFlushRange(void* addr, u32 nBytes);
extern void ICInvalidateRange(void* addr, u32 nBytes);
extern void OSReport(const char* fmt, ...);

/* The four 4-byte thunks immediately before the unit are not defined here, hence the extern
 * declarations; the helpers they and the rest of the unit call are defined further down. */
extern void RSONotifyModuleLoaded(RSOHdr* p);
extern void RSONotifyModuleUnloaded(void);
extern void RSONotifyPreRSOLink(void);
extern void RSONotifyPostRSOLink(RSOModule* pObject, RSORelTable* pRel);
int FindExportIndex(RSORelTable* pRel, const char* symbol);
void RSORelocate(RSORelocation* relocation, u32 index, u32 value);
void RSORelocateSmallDataSection(RSOModule* pObject, u32 importSymbolIndex, RSORelTable* pRel);

/* The TU-wide string pool at .data:0x80629B90 (symbols.txt `@1841`): the three OSReport format
 * strings (0x80629B90, +0x28, +0x50), the "OSLink: unknown relocation type %3d\n" message
 * (0x80629C40) and the "_SDA_BASE_"/"_SDA2_BASE_"/"ERROR: incorrect R_PPC_EMB_SDA21 data."
 * strings at +0xD8/+0xE4/+0xF0. */
extern char lbl_80629B90[];
extern char lbl_80629C40[];

/* RSO section load addresses, resolved by the linker script (RSOStaticLocateObject's switch). */
extern char _f_init[];
extern char _f_text[];
extern char _f_rodata[];
extern char _f_data[];
extern char _f_bss[];
extern char _f_sdata[];
extern char _f_sdata2[];
extern char _f_sbss[];
extern char _f_sbss2[];

/* ---- retail address order --------------------------------------------- */

/* Rebases every self-relative pointer field of the module header and its relocation tables,
 * applies the section-index fixups and patches each import's relocation chain. */
BOOL LocateObject(RSOHdr* p, void* buf, int mode)
{
    u32 i;
    int n;

    p->bss_section_index = 0;
    p->section_info_offset += (u32)p;
    p->module_name_offset += (u32)p;
    p->internal_relocation_table_offset += (u32)p;
    p->external_relocation_table_offset += (u32)p;
    p->export_symbol_table_offset += (u32)p;
    p->export_symbol_names_offset += (u32)p;
    p->import_symbol_table_offset += (u32)p;
    p->import_symbol_names_offset += (u32)p;

    for (i = 1; i < p->section_count; i++) {
        u32* e = (u32*)(p->section_info_offset + i * 8);

        if (*e != 0) {
            *e += (u32)p;
        } else if (e[1] != 0) {
            p->bss_section_index = (u8)i;
            *e = (u32)buf;
        }
    }

    if (p->prolog_section_index != 0)
        p->prolog_function_offset += *(u32*)(p->section_info_offset + p->prolog_section_index * 8);
    if (p->epilog_section_index != 0)
        p->epilog_function_offset += *(u32*)(p->section_info_offset + p->epilog_section_index * 8);
    if (p->unresolved_section_index != 0)
        p->unresolved_function_offset += *(u32*)(p->section_info_offset + p->unresolved_section_index * 8);

    n = p->internal_relocation_table_size / 12;
    for (i = 0; i < n; i++) {
        u32* e = (u32*)(p->internal_relocation_table_offset + i * 12);

        *e += (u32)p;
        RSORelocate((RSORelocation*)e, 0, *(u32*)(p->section_info_offset + (e[1] >> 8) * 8));
    }

    if (mode >= 1)
        p->internal_relocation_table_size = 0;

    n = p->external_relocation_table_size / 12;
    for (i = 0; i < n; i++)
        *(u32*)(p->external_relocation_table_offset + i * 12) += (u32)p;

    n = p->import_symbol_table_size / 12;
    for (i = 0; (int)i < n; i++) {
        RSOImport* e = (RSOImport*)(p->import_symbol_table_offset + i * 12);
        RSORelocation* r;

        e->code_offset = p->unresolved_function_offset;
        r = (RSORelocation*)(p->external_relocation_table_offset + e->entry_offset);
        while ((r->info >> 8) == i) {
            RSORelocate(r, 0, e->code_offset);
            r++;
        }
    }

    if (mode <= 1)
        memset(buf, 0, p->bss_size);

    RSONotifyModuleLoaded(p);
    return TRUE;
}

/* Fills each section-table entry with the address of the linker section it names (or zeroes it),
 * reports unknown sections, then rebases the external relocation record table. */
BOOL RSOStaticLocateObject(RSOModule* p)
{
    char* msg = lbl_80629B90;
    u32 off;
    u32 i;
    u32 n;

    p->bss_section_index = 0;
    p->section_info_offset = (RSOSection*)((u32)p->section_info_offset + (u32)p);
    p->module_name_offset = p->module_name_offset + (u32)p;
    p->internal_relocation_table_offset = p->internal_relocation_table_offset + (u32)p;
    p->external_relocation_table_offset = p->external_relocation_table_offset + (u32)p;
    p->export_symbol_table_offset = p->export_symbol_table_offset + (u32)p;
    p->export_symbol_names_offset = p->export_symbol_names_offset + (u32)p;
    p->import_symbol_table_offset = (RSOImport*)((u32)p->import_symbol_table_offset + (u32)p);
    p->import_symbol_names_offset = p->import_symbol_names_offset + (u32)p;

    for (i = 1, off = sizeof(RSOSection); i < p->section_count; off += sizeof(RSOSection), i++) {
        RSOSection* pSec = (RSOSection*)((u32)p->section_info_offset + off);

        switch (i) {
        case 1:
            pSec->offset = (u32)_f_init;
            break;
        case 2:
            pSec->offset = (u32)_f_text;
            break;
        case 3:
            if (pSec->size != 0)
                OSReport(msg, i, pSec->size);
            pSec->offset = 0;
            break;
        case 4:
            if (pSec->size != 0)
                OSReport(msg + 40, i, pSec->size);
            pSec->offset = 0;
            break;
        case 5:
            pSec->offset = (u32)_f_rodata;
            break;
        case 6:
            pSec->offset = (u32)_f_data;
            break;
        case 7:
            pSec->offset = (u32)_f_bss;
            break;
        case 8:
            pSec->offset = (u32)_f_sdata;
            break;
        case 11:
            pSec->offset = (u32)_f_sbss;
            break;
        case 9:
            pSec->offset = (u32)_f_sdata2;
            break;
        case 12:
            pSec->offset = (u32)_f_sbss2;
            break;
        case 10:
            pSec->offset = 0;
            break;
        case 13:
            pSec->offset = 0;
            break;
        default:
            if (pSec->size != 0)
                OSReport(msg + 80, i, pSec->size);
            pSec->offset = 0;
            break;
        }
    }

    n = p->external_relocation_table_size / 12;
    for (i = 0; i < n; i++)
        ((u32*)p->external_relocation_table_offset)[i * 3] = (u32)p + ((u32*)p->external_relocation_table_offset)[i * 3];

    return TRUE;
}

/* Undoes the locate step: turns every address stored in the object back into an offset relative
 * to the object, so the module can be moved. */
int RSOUnLocateObject(RSOModule* pObject)
{
    u32 base = (u32)pObject;
    int i;
    int n;

    RSONotifyModuleUnloaded();

    n = pObject->external_relocation_table_size / 12;
    for (i = 0; i < n; i++) {
        ((RSORelocation*)pObject->external_relocation_table_offset)[i].offset -= base;
    }

    n = pObject->internal_relocation_table_size / 12;
    for (i = 0; i < n; i++) {
        ((RSORelocation*)pObject->internal_relocation_table_offset)[i].offset -= base;
    }

    if (pObject->prolog_section_index != 0)
        pObject->prolog_function_offset -= pObject->section_info_offset[pObject->prolog_section_index].offset;
    if (pObject->epilog_section_index != 0)
        pObject->epilog_function_offset -= pObject->section_info_offset[pObject->epilog_section_index].offset;
    if (pObject->unresolved_section_index != 0)
        pObject->unresolved_function_offset -= pObject->section_info_offset[pObject->unresolved_section_index].offset;

    for (i = 1; i < pObject->section_count; i++) {
        RSOSection* pEntry = &pObject->section_info_offset[i];

        if (i == pObject->bss_section_index) {
            pObject->bss_section_index = 0;
            pEntry->offset = 0;
        } else if (pEntry->offset != 0) {
            pEntry->offset -= base;
        }
    }

    pObject->section_info_offset = (RSOSection*)((u32)pObject->section_info_offset - base);
    pObject->module_name_offset -= base;
    pObject->internal_relocation_table_offset -= base;
    pObject->external_relocation_table_offset -= base;
    pObject->export_symbol_table_offset -= base;
    pObject->export_symbol_names_offset -= base;
    pObject->import_symbol_table_offset = (RSOImport*)((u32)pObject->import_symbol_table_offset - base);
    pObject->import_symbol_names_offset -= base;

    return TRUE;
}

/* Resolves the module's imports through the target module's export tables and patches their
 * external relocation chains, returning the number of relocations applied. */
int RSOLink(RSOModule* pObject, RSORelTable* pRel)
{
    u32 offset;
    int i;
    int relocated = 0;
    int count = pObject->import_symbol_table_size / 12;
    RSOImport* entry = pObject->import_symbol_table_offset;
    u32* p;
    RSOImport* e;

    RSONotifyPreRSOLink();
    for (i = 0, offset = 0; i < count; i++, entry++) {
        u32 addr = entry->name_offset + pObject->import_symbol_names_offset;

        if (addr != 0) {
            int idx;
            u32 value;

            RSORelocateSmallDataSection(pObject, i, pRel);
            idx = FindExportIndex(pRel, (const char*)addr);
            if (idx == -1)
                value = 0;
            else {
                RSOExport* desc = &pRel->export_symbol_table_offset[idx];
                u32 sym = *(u32*)(pRel->section_info_offset + desc->section_index * 8);
                value = sym + desc->code_offset;
            }
            if (value != 0) {
                e = (RSOImport*)((u8*)pObject->import_symbol_table_offset + offset);
                e->code_offset = value;
                for (p = (u32*)(pObject->external_relocation_table_offset + e->entry_offset); (p[1] >> 8) == (u32)i; p += 3)
                    RSORelocate((RSORelocation*)p, 0, e->code_offset);
                relocated++;
            }
        }
        offset += 12;
    }
    RSONotifyPostRSOLink(pObject, pRel);
    return relocated;
}

/* Resolves the module's imports against the already-linked module, recording the
 * unresolved-function stub address instead of the computed symbol value. */
void RSOUnLink(RSOModule* pObject, RSORelTable* pRel)
{
    int i;
    int count = pObject->import_symbol_table_size / 12;

    RSONotifyPreRSOLink();

    for (i = 0; i < count; i++) {
        u32 buf[1];
        u32* p;
        RSOImport* e;
        u32 addr;
        int idx;
        u32 name;
        u32 ent;

        *(u32*)buf = (u32)pObject + 0x54;
        ent = pObject->import_symbol_table_offset[(u32)i].name_offset;

        name = ent + *(u32*)((u32*)buf)[0];
        if (name == 0)
            continue;

        idx = FindExportIndex(pRel, (const char*)name);
        if (idx == -1)
            addr = 0;
        else
            addr = *(u32*)(pRel->section_info_offset + (pRel->export_symbol_table_offset[idx].section_index << 3)) + pRel->export_symbol_table_offset[idx].code_offset;

        if (addr == 0)
            continue;

        e = &pObject->import_symbol_table_offset[i];
        e->code_offset = pObject->unresolved_function_offset;
        p = (u32*)(pObject->external_relocation_table_offset + e->entry_offset);

        while ((p[1] >> 8) == (u32)i) {
            RSORelocate((RSORelocation*)p, 0, e->code_offset);
            p += 3;
        }
    }

    RSONotifyPostRSOLink(pObject, pRel);
}

/* Reports whether every import has been located: returns FALSE as soon as an import still holds
 * the unresolved-function stub address. */
BOOL fn_804DA7E4(RSOModule* pObject)
{
    int count = pObject->import_symbol_table_size / 12;
    u32 offset = 0;
    RSOImport* pEntry;

    while (count-- > 0) {
        pEntry = (RSOImport*)((u8*)pObject->import_symbol_table_offset + offset);
        if (pEntry->code_offset == pObject->unresolved_function_offset)
            return FALSE;
        offset += 12;
    }
    return TRUE;
}

/* Looks a symbol name up in the module's hash-sorted export table: ELF string hash, then a binary
 * search and strcmp scan over the equal-hash neighbours. */
int FindExportIndex(RSORelTable* pRel, const char* symbol)
{
    u32 hash = 0;
    const char* p = symbol;
    u32 g;
    RSOExport* tbl;
    int n;
    int lo, hi, mid, found;
    int i;

    while (*p) {
        hash = (hash << 4) + *p++;
        if ((g = hash & 0xF0000000) != 0)
            hash ^= g >> 24;
        hash &= ~g;
    }

    n = pRel->export_symbol_table_size >> 4;
    tbl = pRel->export_symbol_table_offset;
    lo = 0;
    hi = n - 1;
    found = -1;

    if (n <= 0)
        return -1;

    while (found == -1) {
        mid = (lo + hi) >> 1;
        if (hash > tbl[mid].hash) {
            if (lo == mid)
                found = hi;
            else
                lo = mid;
        } else if (hash < tbl[mid].hash) {
            if (lo == mid)
                found = lo;
            else
                hi = mid;
        } else {
            found = mid;
        }
    }

    if (tbl[found].hash != hash)
        return -1;
    {
        u32 no = tbl[found].name_offset;
        if (strcmp(symbol, (const char*)(no + pRel->export_symbol_names_offset)) == 0)
            return found;
    }

    for (i = found + 1; i <= hi; i++) {
        if (tbl[i].hash == hash) {
            u32 no = ((RSOExport*)pRel->export_symbol_table_offset)[i].name_offset;
            if (strcmp(symbol, (const char*)(no + pRel->export_symbol_names_offset)) == 0)
                return i;
        } else {
            i = hi + 1;
        }
    }

    for (i = found - 1; i >= lo; i--) {
        RSOExport* p2 = &tbl[i];
        u32 off = (u32)i << 4;
        if (hash == p2->hash) {
            u32 no = *(u32*)((u8*)pRel->export_symbol_table_offset + off);
            if (strcmp(symbol, (const char*)(no + pRel->export_symbol_names_offset)) == 0)
                return i;
        } else {
            return -1;
        }
    }
    return -1;
}

/* Applies one R_PPC_* relocation to the word it points at, then flushes both caches over the
 * patched word. */
void RSORelocate(RSORelocation* relocation, u32 index, u32 value)
{
    u32* addr = (u32*)relocation[index].offset;

    switch ((u8)relocation->info) {
    case 0: /* R_PPC_NONE */
        break;
    case 1: /* R_PPC_ADDR32 */
    {
        *addr = value + relocation->addend;
        break;
    }
    case 2: /* R_PPC_ADDR24 */
    {
        *addr = (*addr & 0xFC000003) | ((value + relocation->addend) & 0x03FFFFFC);
        break;
    }
    case 3: /* R_PPC_ADDR16 */
    {
        *(u16*)addr = value + relocation->addend;
        break;
    }
    case 4: /* R_PPC_ADDR16_LO */
    {
        *(u16*)addr = value + relocation->addend;
        break;
    }
    case 5: /* R_PPC_ADDR16_HI */
    {
        *(u16*)addr = (value + relocation->addend) >> 16;
        break;
    }
    case 6: /* R_PPC_ADDR16_HA: ((x + 0x8000) >> 16) == (x >> 16) + bit15(x) */
    {
        u32 x = value + relocation->addend;
        *(u16*)addr = (x >> 16) + ((x >> 15) & 1);
        break;
    }
    case 7: /* R_PPC_ADDR14 */
    case 8: /* R_PPC_ADDR14_BRTAKEN */
    case 9: /* R_PPC_ADDR14_BRNTAKEN */
    {
        *addr = (*addr & 0xFFFF0003) | ((value + relocation->addend) & 0xFFFC);
        break;
    }
    case 10: /* R_PPC_REL24, with an out-of-range trap */
    {
        u32 target = value + relocation->addend;
        u32 low = (u32)addr & 0x03FFFFFC;
        u32 insn = *addr;
        u32 page = (u32)addr & 0xFC000003;
        u32 newInsn = (insn & 0xFC000003) | ((target - (u32)addr) & 0x03FFFFFC);
        u32 reached;

        *addr = newInsn;
        reached = page | ((low + (newInsn & 0x03FFFFFC)) & 0x03FFFFFC);
        if (value + relocation->addend != 0 && value + relocation->addend != reached)
            *addr = (newInsn & 0xFC000003) | 0x03FFFFFC;
        break;
    }
    case 11: /* R_PPC_REL14 */
    case 12: /* R_PPC_REL14_BRTAKEN */
    case 13: /* R_PPC_REL14_BRNTAKEN */
    {
        *addr = (*addr & 0xFFFF0003) | (((value + relocation->addend) - (u32)addr) & 0xFFFC);
        break;
    }
    case 109: /* R_PPC_EMB_SDA21 -- already resolved elsewhere */
        break;
    default:
        OSReport(lbl_80629C40, (u8)relocation->info);
        break;
    }

    DCFlushRange(addr, 32);
    ICInvalidateRange(addr, 32);
}

/* Patches every embedded SDA21 relocation belonging to one import symbol, resolving the
 * _SDA_BASE_/_SDA2_BASE_ symbols the patched instruction references. */
void RSORelocateSmallDataSection(RSOModule* pObject, u32 importSymbolIndex, RSORelTable* pRel)
{
    char* base = lbl_80629B90;
    u32 e0 = pObject->import_symbol_table_offset[importSymbolIndex].name_offset;
    u32 name = e0 + pObject->import_symbol_names_offset;
    u8* p;
    u32 value;
    RSORelocation* reloc = (RSORelocation*)(pObject->external_relocation_table_offset +
                                            pObject->import_symbol_table_offset[importSymbolIndex].entry_offset);

    while ((reloc->info >> 8) == importSymbolIndex) {
        if ((u8)reloc->info == 109) {
            RSOExport* desc;
            RSOExport* sym;
            int idx;

            p = (u8*)reloc->offset;
            switch (reloc->offset & 3) {
            case 0:
                p += 1;
                break;
            case 2:
                p -= 1;
                break;
            case 3:
                p -= 2;
                break;
            }

            idx = FindExportIndex(pRel, (const char*)name);
            desc = idx == -1 ? 0 : &pRel->export_symbol_table_offset[idx];
            if (desc != 0) {
                switch (desc->section_index) {
                case 8:
                case 11:
                    *p = (u8)((*p & 0xFFFFFFE0) | 13);
                    idx = FindExportIndex(pRel, base + 0xD8);
                    sym = idx == -1 ? 0 : &pRel->export_symbol_table_offset[idx];
                    if (sym == 0) {
                        reloc++;
                        continue;
                    }
                    value = sym->code_offset;
                    break;
                case 9:
                case 12:
                    *p = (u8)((*p & 0xFFFFFFE0) | 2);
                    idx = FindExportIndex(pRel, base + 0xE4);
                    sym = idx == -1 ? 0 : &pRel->export_symbol_table_offset[idx];
                    if (sym == 0) {
                        reloc++;
                        continue;
                    }
                    value = sym->code_offset;
                    break;
                case 10:
                case 13:
                    *p = (u8)(*p & 0xE0);
                    value = 0;
                    break;
                case 241:
                    OSReport(base + 0xF0);
                    break;
                default:
                    OSReport(base + 0xF0);
                    break;
                }

                *(u16*)(p + 1) = reloc->addend + desc->code_offset + *(u32*)(pRel->section_info_offset + desc->section_index * 8) - value;
                DCFlushRange(p, 32);
                ICInvalidateRange(p, 32);
            }
        }
        reloc++;
    }
}
