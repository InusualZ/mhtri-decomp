/* stdarg.h - the CodeWarrior PowerPC `va_list` and the variadic intrinsics (the pinned toolchain ships no MSL
 *   headers).  `va_list` is a one-element array, so passing it hands the callee a pointer to the caller's state. */
#ifndef MHTRI_STDARG_H
#define MHTRI_STDARG_H

/* size: 0xC */
typedef struct __va_list_struct {
    /* +0x0 */ char gpr;
    /* +0x1 */ char fpr;
    /* +0x2 */ char reserved[2];
    /* +0x4 */ char* input_arg_area;
    /* +0x8 */ char* reg_save_area;
} va_list[1];

#define va_start(ap, last) ((void)(last), __builtin_va_info(&(ap)))
#define va_end(ap) ((void)0)

#endif /* MHTRI_STDARG_H */
