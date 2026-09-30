/*
 * include/Runtime.PPCEABI.H/ptmf.h - what `Runtime.PPCEABI.H/ptmf.c` owns: the pointer-to-member-function record, the
 * null record every member-function-pointer field is initialised from, and the two helpers the compiler emits calls to.
 */

#ifndef RUNTIME_PPCEABI_H_PTMF_H
#define RUNTIME_PPCEABI_H_PTMF_H

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0xC (evidence: every reader copies three words from `__ptmf_null` into a record's +0x98/+0x9C/+0xA0) */
typedef struct __ptmf {
    long this_delta;     /* +0x00 - adjustment added to `this` */
    long vtbl_offset;    /* +0x04 - offset of the vptr in the object, negative for a non-virtual target */
    long func_data;      /* +0x08 - the function address, or the vtable entry offset when virtual */
} __ptmf;

extern const __ptmf __ptmf_null;   /* 0x80572428 (.rodata) */

/* free: retail C linkage - MSL runtime symbol, unmangled in the map and called by compiler-emitted code */
long __ptmf_test(__ptmf* ptmf);    /* 1 when the record is not null */
/* untyped: opaque handle passed through - `this` of the member function, forwarded in r3 */
/* free: retail C linkage - MSL runtime symbol, unmangled in the map and called by compiler-emitted code */
long __ptmf_scall(void* self);     /* tail-calls through the record in r12 */

#ifdef __cplusplus
}
#endif

#endif /* RUNTIME_PPCEABI_H_PTMF_H */
