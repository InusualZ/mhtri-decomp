/*
 * Runtime.PPCEABI.H/__va_arg.cpp - the PPC EABI variadic-argument fetch helper of the MSL runtime.
 *
 * RANGE. .text 0x804565DC..0x804566A4 (`__va_arg`, 200 B); no data sections; the right edge is the start of
 *   `global_destructor_chain.c`.
 * FLAGS. the `Runtime.PPCEABI.H` lib: Wii/1.3, `cflags_ppceabi` (`cflags_runtime` plus `-func_align 4`).
 * NAMES. `__va_arg` is the map's name and the MSL runtime's own; its declaration is `__va_arg.h`.
 * RESIDUALS. none.
 * SHAPES. `type` classes: 0 a word passed by reference, 1 a GPR word, 2 a doubleword in an even GPR pair,
 *   3 an FPR double; past the register save area the slot comes from the overflow area, aligned to its size.
 */

#include "Runtime.PPCEABI.H/__va_arg.h"

/* 0x804565DC (0xC8): Returns the address of the next variadic argument of class `type` and advances the list. */
u32* __va_arg(struct __va_list_struct* v_list, s32 type)
{
    char* addr;
    char* reg = &(v_list->gpr);
    s32 g_reg = v_list->gpr;
    s32 maxsize = 8;
    s32 size = 4;
    s32 increment = 1;
    s32 even = 0;
    s32 fpr_offset = 0;
    s32 regsize = 4;

    if (type == 3) {
        reg = &(v_list->fpr);
        g_reg = v_list->fpr;
        size = 8;
        fpr_offset = 32;
        regsize = 8;
    }
    if (type == 2) {
        size = 8;
        maxsize--;
        if (g_reg & 1) {
            even = 1;
        }
        increment = 2;
    }
    if (g_reg < maxsize) {
        g_reg += even;
        addr = v_list->reg_save_area + fpr_offset + (g_reg * regsize);
        *reg = g_reg + increment;
    } else {
        *reg = 8;
        addr = v_list->input_arg_area;
        addr = (char*)(((u32)(addr) + ((size) - 1)) & ~((size) - 1));
        v_list->input_arg_area = addr + size;
    }
    if (type == 0) {
        addr = *((char**)addr);
    }
    return (u32*)addr;
}
