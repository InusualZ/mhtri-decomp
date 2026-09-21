/*
 * MSL C runtime: memcpy and the __fill_mem helper that memset (the next .init range) calls.
 *
 * .init 0x80004000-0x80004350 - memcpy (0x29C, 668 B) then __fill_mem (0xB4, 180 B), in that
 * order; both leaf functions, and __fill_mem's only caller in the whole DOL is memset
 * (R_PPC_REL24 at memset+0x14). The retail member is the SDK's __mem.o =
 * {memcpy, __fill_mem, memset}; this unit is the memcpy/__fill_mem half. Registered NonMatching
 * under the Runtime.PPCEABI.H lib (Wii/1.3, cflags_runtime), mirroring memset.c.
 *
 * memcpy is a hand-written assembly function in the original source (an `asm` block, not C): the
 * retail body is the three-tier 8-byte/4-byte/byte copy with the dcbt prefetch, and no C shape
 * reproduces it. Recovered from the Revolution SDK's __mem.c - the same file the xenoblade and
 * Petari decomps carry - and __fill_mem is that file's C function.
 *
 * Load-bearing source shapes:
 *   - both definitions are forced into .init with `#pragma section code_type ".init"` as in the
 *     original file; compiled as plain .text objdiff pairs nothing.
 *   - `#pragma function_align 4` stands in for a per-library flag: -O4,p implies `-func_align 16`,
 *     which pads __fill_mem to 0x2A0 and moves its tail loop head to 0x348, so MWCC inserts a nop
 *     (0xB8 vs 0xB4) and __fill_mem reads 97.8 %. Every object of this library has 2**2 section
 *     alignment, i.e. the retail build used function alignment 4; a `-func_align 4` in this lib's
 *     cflags is the faithful fix and would retire this pragma. Evidence: with -func_align 4 our
 *     .init is byte-identical, with -O3 it is not.
 *   - __fill_mem's unrolled 8-word loop stores idest[1] first and decrements its counter between
 *     store 1 and store 2 (`idest[1] = cval; --r0; ...`) - that placement is what puts the
 *     `addic.` there - and the block ends with `*(idest += 8) = cval` (stwu).
 * Residual: none. .init is byte-identical to the target object (0x350 B, no relocations) and both
 * symbols measure 100 %.
 */

#pragma section code_type ".init"
#pragma function_align 4

asm void* memcpy(void* dest, const void* src, unsigned int size) {
    // Return if size is 0
    cmplwi cr1, r5, 0
    beqlr cr1

    cmplw cr1, r4, r3
    blt cr1, reverse
    beqlr cr1

    li r6, 0x80
    cmplw cr5, r5, r6
    blt cr5, test_word_alignment

    clrlwi r9, r4, 0x1d
    clrlwi r10, r3, 0x1d
    subf r8, r10, r3

    // Request a data cache block fetch
    dcbt 0, r4

    xor. r11, r10, r9
    bne byte_setup
    andi. r10, r10, 7
    beq+ double_copy_setup
    li r6, 8
    subf r9, r9, r6
    addi r8, r3, 0
    mtctr r9
    subf r5, r9, r5

byte_loop_double_align:
    lbz r9, 0(r4)
    addi r4, r4, 1
    stb r9, 0(r8)
    addi r8, r8, 1
    bdnz byte_loop_double_align

double_copy_setup:
    srwi r6, r5, 5
    mtctr r6

double_loop:
    lfd f1, 0(r4)
    lfd f2, 8(r4)
    lfd f3, 0x10(r4)
    lfd f4, 0x18(r4)
    addi r4, r4, 0x20
    stfd f1, 0(r8)
    stfd f2, 8(r8)
    stfd f3, 0x10(r8)
    stfd f4, 0x18(r8)

    addi r8, r8, 0x20
    bdnz double_loop
    andi. r6, r5, 0x1f
    beqlr
    addi r4, r4, -1
    mtctr r6
    addi r8, r8, -1

byte_loop_1:
    lbzu r9, 1(r4)
    stbu r9, 1(r8)
    bdnz byte_loop_1
    blr

test_word_alignment:
    li r6, 0x14
    cmplw cr5, r5, r6
    ble cr5, byte_setup
    clrlwi r9, r4, 0x1e
    clrlwi r10, r3, 0x1e
    xor. r11, r10, r9
    bne byte_setup
    li r6, 4
    subf r9, r9, r6
    addi r8, r3, 0
    subf r5, r9, r5
    mtctr r9

byte_loop_word_align:
    lbz r9, 0(r4)
    addi r4, r4, 1
    stb r9, 0(r8)
    addi r8, r8, 1
    bdnz byte_loop_word_align

word_copy_setup:
    srwi r6, r5, 4
    mtctr r6

word_loop:
    lwz r9, 0(r4)
    lwz r10, 4(r4)
    lwz r11, 8(r4)
    lwz r12, 0xc(r4)
    addi r4, r4, 0x10
    stw r9, 0(r8)
    stw r10, 4(r8)
    stw r11, 8(r8)
    stw r12, 0xc(r8)
    addi r8, r8, 0x10
    bdnz word_loop

    andi. r6, r5, 0xf
    beqlr
    addi r4, r4, -1
    mtctr r6
    addi r8, r8, -1

byte_loop_2:
    lbzu r9, 1(r4)
    stbu r9, 1(r8)
    bdnz byte_loop_2
    blr

byte_setup:
    addi r7, r4, -1
    addi r8, r3, -1
    mtctr r5

byte_loop_3:
    lbzu r9, 1(r7)
    stbu r9, 1(r8)
    bdnz byte_loop_3
    blr

reverse:
    add r4, r4, r5
    add r12, r3, r5
    li r6, 0x80
    cmplw cr5, r5, r6
    blt cr5, reverse_test_word_alignment
    clrlwi r9, r4, 0x1d
    clrlwi r10, r12, 0x1d
    xor. r11, r10, r9
    bne reverse_byte_setup
    andi. r10, r10, 7
    beq+ reverse_double_copy_setup
    mtctr r10

reverse_byte_loop_double_align:
    lbzu r9, -1(r4)
    stbu r9, -1(r12)
    bdnz reverse_byte_loop_double_align

reverse_double_copy_setup:
    subf r5, r10, r5
    srwi r6, r5, 5
    mtctr r6

reverse_double_loop:
    lfd f1, -8(r4)
    lfd f2, -0x10(r4)
    lfd f3, -0x18(r4)
    lfd f4, -0x20(r4)
    addi r4, r4, -32
    stfd f1, -8(r12)
    stfd f2, -0x10(r12)
    stfd f3, -0x18(r12)
    stfdu f4, -0x20(r12)

    bdnz reverse_double_loop

    andi. r6, r5, 0x1f
    beqlr
    mtctr r6

reverse_byte_loop_1:
    lbzu r9, -1(r4)
    stbu r9, -1(r12)
    bdnz reverse_byte_loop_1
    blr

reverse_test_word_alignment:
    li r6, 0x14
    cmplw cr5, r5, r6
    ble cr5, reverse_byte_setup
    clrlwi r9, r4, 0x1e
    clrlwi r10, r12, 0x1e
    xor. r11, r10, r9
    bne reverse_byte_setup
    andi. r10, r10, 7
    beq+ reverse_word_loop_setup
    mtctr r10

reverse_byte_loop_word_align:
    lbzu r9, -1(r4)
    stbu r9, -1(r12)
    bdnz reverse_byte_loop_word_align

reverse_word_loop_setup:
    subf r5, r10, r5
    srwi r6, r5, 4
    mtctr r6

reverse_word_loop:
    lwz r9, -4(r4)
    lwz r10, -8(r4)
    lwz r11, -0xc(r4)
    lwz r8, -0x10(r4)
    addi r4, r4, -16
    stw r9, -4(r12)
    stw r10, -8(r12)
    stw r11, -0xc(r12)
    stwu r8, -0x10(r12)
    bdnz reverse_word_loop

    andi. r6, r5, 0xf
    beqlr
    mtctr r6

reverse_byte_loop_2:
    lbzu r9, -1(r4)
    stbu r9, -1(r12)
    bdnz reverse_byte_loop_2
    blr

reverse_byte_setup:
    mtctr r5

reverse_byte_loop_3:
    lbzu r9, -1(r4)
    stbu r9, -1(r12)
    bdnz reverse_byte_loop_3
    blr
}

void __fill_mem(void* dest, int val, unsigned int count)
{
    char* cdest = (char*)dest;
    int cval = (unsigned char)val;
    int* idest = (int*)dest;
    int r0;

    cdest--;

    if (count >= 0x20)
    {
        r0 = ~(int)cdest & 3;

        if (r0)
        {
            count -= r0;

            do
            {
                *++cdest = cval;
            } while (--r0);
        }

        if (cval)
        {
            cval = (cval << 0x18) | (cval << 0x10) | (cval << 0x8) | cval;
        }

        r0 = count >> 5;
        idest = (int*)(cdest - 3);

        if (r0)
        {
            do
            {
                idest[1] = cval;
                --r0;
                idest[2] = cval;
                idest[3] = cval;
                idest[4] = cval;
                idest[5] = cval;
                idest[6] = cval;
                idest[7] = cval;
                *(idest += 8) = cval;
            } while (r0);
        }

        r0 = (count >> 2) & 7;

        if (r0)
        {
            do
            {
                *++idest = cval;
            } while (--r0);
        }

        cdest = (char*)idest + 3;
        count &= 3;
    }

    if (count)
    {
        do
        {
            *++cdest = cval;
        } while (--count);
    }
}

#pragma section code_type
