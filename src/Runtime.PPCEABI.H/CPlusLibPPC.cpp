/* Runtime.PPCEABI.H/CPlusLibPPC - the MSL C++ support runtime: terminate/unexpected handlers, the catch-type
 *   matcher and the new[]/array construction and destruction helpers (MSL's NMWException.cp).
 * RANGE. .text 0x80456704-0x80456C88 (9 functions); extab 0x8001E3B0-0x8001E3F0; extabindex 0x8003F144-0x8003F174;
 *   .sdata 0x80793CC0-0x80793CC8 (`thandler`, `uhandler`).
 * FLAGS. the `Runtime.PPCEABI.H` lib in configure.py: Wii/1.3, `cflags_ppceabi` (`cflags_runtime` plus
 *   `-func_align 4`); `#pragma exceptions on` here, because the target carries extab/extabindex records for
 *   `__construct_new_array`, the partial-array destructor, `__construct_array` and `__destroy_arr`.
 * NAMES. every symbol is the compiler's spelling of MSL NMWException.cp (the function set and its order are that
 *   file's); the registered file name CPlusLibPPC.cpp is a GUESS (the MSL library's support-file name), the
 *   source is NMWException.cp. `__dt__26__partial_array_destructorFv` is the inline class destructor emitted out
 *   of line (weak); `dthandler`/`duhandler`/`thandler`/`uhandler` are file-local statics in namespace std.
 * RESIDUALS. none in the bytes. flipcheck row 36 names `__dt__26__partial_array_destructorFv` (force-active in the
 *   target's .comment, not in ours); the extab records reference it, so the link keeps it: with the object linked as
 *   Matching, main.dol hashes OK.
 * SHAPES. `__construct_array` keeps the header's `void* ctor, void* dtor` and casts at each call: a typed local
 *   copy changes the r28-r31 allocation. The partial-array guard's counter `i` is the callers' loop counter
 *   (`pad.i`), so an exception mid-construction destroys exactly the elements built so far. The guard is a `struct`
 *   (public members, no access-specifier lines): a `class` with none makes its destructor private (MWCC error 10381).
 */
#pragma exceptions on

#include "types.h"

#include "Runtime.PPCEABI.H/CPlusLibPPC.h"
#include "MSL_C/abort_exit.h"

/* untyped: raw element storage of a caller-owned array */
typedef void (*ConstructorDestructor)(void* obj, s16 flag);

/* Bytes in front of a new[] array: the element size at +0x0, the element count at +0x4. */
#define ARRAY_HEADER_SIZE 16

namespace std {
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();

/* 0x80456704 (0x4): the default terminate handler; aborts. */
static void dthandler() { abort(); }
static terminate_handler thandler = dthandler;

/* 0x80456708 (0x4): the default unexpected handler; terminates. */
static void duhandler() { terminate(); }
static unexpected_handler uhandler = duhandler;

/* 0x8045670C (0xC): calls the installed terminate handler. */
void terminate() { (*thandler)(); }

/* 0x80456718 (0xC): calls the installed unexpected handler. */
void unexpected() { (*uhandler)(); }
}

/* 0x80456724 (0x234): matches a thrown type string against a catch type string; writes the base class offset
 * of a class match to `offset_result`. */
extern "C" char __throw_catch_compare(const char* throwtype, const char* catchtype, s32* offset_result)
{
    const char* cptr1;
    const char* cptr2;

    *offset_result = 0;
    if ((cptr2 = catchtype) == 0) {
        return 1;
    }
    cptr1 = throwtype;
    if (*cptr2 == 'P') {
        cptr2++;
        if (*cptr2 == 'C') cptr2++;
        if (*cptr2 == 'V') cptr2++;
        if (*cptr2 == 'v') {
            if (*cptr1 == 'P' || *cptr1 == '*') {
                return 1;
            }
        }
        cptr2 = catchtype;
    }
    switch (*cptr1) {
    case '*':
    case '!':
        if (*cptr1++ != *cptr2++) {
            return 0;
        }
        for (;;) {
            if (*cptr1 == *cptr2++) {
                if (*cptr1++ == '!') {
                    s32 offset;
                    for (offset = 0; *cptr1 != '!';) {
                        offset = offset * 10 + *cptr1++ - '0';
                    }
                    *offset_result = offset;
                    return 1;
                }
            } else {
                while (*cptr1++ != '!') {
                }
                while (*cptr1++ != '!') {
                }
                if (*cptr1 == 0) {
                    return 0;
                }
                cptr2 = catchtype + 1;
            }
        }
        return 0;
    }
    while ((*cptr1 == 'P' || *cptr1 == 'R') && *cptr1 == *cptr2) {
        cptr1++;
        cptr2++;
        if (*cptr2 == 'C') {
            if (*cptr1 == 'C') cptr1++;
            cptr2++;
        }
        if (*cptr1 == 'C') return 0;
        if (*cptr2 == 'V') {
            if (*cptr1 == 'V') cptr1++;
            cptr2++;
        }
        if (*cptr1 == 'V') return 0;
    }
    for (; *cptr1 == *cptr2; cptr1++, cptr2++) {
        if (*cptr1 == 0) return 1;
    }
    return 0;
}

/* A stack guard over an array under construction: destroys the elements built so far when unwound early. */
/* size: 0x14 */
struct __partial_array_destructor {
    /* +0x00 */ void* p;
    /* +0x04 */ u32 size;
    /* +0x08 */ u32 n;
    /* +0x0C */ ConstructorDestructor dtor;
    /* +0x10 */ u32 i;

    /* untyped: raw element storage of a caller-owned array */
    __partial_array_destructor(void* array, u32 elementsize, u32 nelements, ConstructorDestructor destructor)
    {
        p = array;
        size = elementsize;
        n = nelements;
        dtor = destructor;
        i = n;
    }

    /* 0x80456A5C (0xBC): destroys the constructed elements in reverse when construction stopped early. */
    ~__partial_array_destructor()
    {
        char* ptr;
        if (i < n && dtor) {
            for (ptr = (char*)p + size * i; i > 0; i--) {
                ptr -= size;
                dtor(ptr, -1);
            }
        }
    }
};

/* 0x80456958 (0x104): writes the new[] header into `block` and constructs `n` elements of `size` bytes after
 * it; returns the first element. */
/* untyped: raw element storage of a caller-owned array */
extern "C" void* __construct_new_array(void* block, ConstructorDestructor ctor, ConstructorDestructor dtor, u32 size, u32 n)
{
    char* ptr;
    if ((ptr = (char*)block) != 0) {
        u32* header = (u32*)ptr;
        header[0] = size;
        header[1] = n;
        ptr += ARRAY_HEADER_SIZE;
        if (ctor) {
            __partial_array_destructor pad(ptr, size, n, dtor);
            char* q;
            for (pad.i = 0, q = ptr; pad.i < n; pad.i++, q += size) {
                ctor(q, 1);
            }
        }
    }
    return ptr;
}

/* 0x80456B18 (0xF8): constructs `n` elements of `size` bytes at `ptr`, destroying the built ones on unwind. */
/* untyped: opaque code address passed through by compiler-emitted calls */
extern "C" void __construct_array(void* ptr, void* ctor, void* dtor, u32 size, u32 n)
{
    __partial_array_destructor pad(ptr, size, n, (ConstructorDestructor)dtor);
    char* p;
    for (pad.i = 0, p = (char*)ptr; pad.i < n; pad.i++, p += size) {
        ((ConstructorDestructor)ctor)(p, 1);
    }
}

/* 0x80456C10 (0x78): destroys `n` elements of `size` bytes at `block`, last element first. */
/* untyped: raw element storage of a caller-owned array */
extern "C" void __destroy_arr(void* block, ConstructorDestructor dtor, u32 size, u32 n)
{
    char* p;
    for (p = (char*)block + size * n; n > 0; n--) {
        p -= size;
        dtor(p, -1);
    }
}
