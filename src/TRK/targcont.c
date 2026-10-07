/*
 * TRK/targcont.c - `TRKTargetContinue` and the extended-register block save/restore (`TRKSaveExtended1Block`,
 *    `TRKRestoreExtended1Block`).
 *
 * RANGE. .text 0x80469F10..0x8046A26C (3 functions in the map, 0x35C B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS; the two groups may be separate files. `TRKSaveExtended1Block` and `TRKRestoreExtended1Block` are
 *    the map's names; the block layout in `TRKCPUState` is traced from the stmw/lmw offsets.
 * EVIDENCE. adjacent dump names; the two block functions read and write the target's CPU state (`gTRKCPUState`, owned by
 *    `TRK/targimpl.c`) through r2; no data of their own.
 * RESIDUALS. none known.
 * SHAPES. both block functions are `nofralloc` assembly (segment, time base, HID, BAT and SPR registers moved through
 *    scratch registers with stmw/lmw); the restore function writes the time base back only when the restore flag is set.
 */
#include "TRK/targcont.h"
#include "TRK/dolphin_trk.h"
#include "TRK/targimpl.h"

s32 TRKTargetContinue(void)
{
    TRKTargetSetStopped(0);
    UnreserveEXI2Port();
    TRKSwapAndGo();
    ReserveEXI2Port();
    return 0;
}

asm void TRKSaveExtended1Block(void)
{
    nofralloc
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    mfsr r16, 0
    mfsr r17, 1
    mfsr r18, 2
    mfsr r19, 3
    mfsr r20, 4
    mfsr r21, 5
    mfsr r22, 6
    mfsr r23, 7
    mfsr r24, 8
    mfsr r25, 9
    mfsr r26, 10
    mfsr r27, 11
    mfsr r28, 12
    mfsr r29, 13
    mfsr r30, 14
    mfsr r31, 15
    stmw r16, 424(r2)
    mftb r27
    mftbu r28
    mfspr r29, 1008
    mfspr r30, 1009
    mfspr r31, 27
    stmw r27, 488(r2)
    mfspr r15, 287
    mfspr r16, 528
    mfspr r17, 529
    mfspr r18, 530
    mfspr r19, 531
    mfspr r20, 532
    mfspr r21, 533
    mfspr r22, 534
    mfspr r23, 535
    mfspr r24, 536
    mfspr r25, 537
    mfspr r26, 538
    mfspr r27, 539
    mfspr r28, 540
    mfspr r29, 541
    mfspr r30, 542
    mfspr r31, 543
    stmw r15, 508(r2)
    mfspr r24, 560
    mfspr r25, 561
    mfspr r26, 562
    mfspr r27, 563
    mfspr r28, 564
    mfspr r29, 565
    mfspr r30, 566
    mfspr r31, 567
    stmw r24, 576(r2)
    mfspr r22, 25
    mfspr r23, 19
    mfspr r24, 18
    mfspr r25, 272
    mfspr r26, 273
    mfspr r27, 274
    mfspr r28, 275
    mfspr r29, 22
    mfspr r30, 1010
    mfspr r31, 282
    stmw r22, 604(r2)
    mfspr r24, 1013
    mfspr r25, 953
    mfspr r26, 954
    mfspr r27, 957
    mfspr r28, 958
    mfspr r29, 955
    mfspr r30, 952
    mfspr r31, 956
    stmw r24, 644(r2)
    mfspr r29, 567
    mfspr r30, 568
    mfspr r31, 569
    stmw r29, 676(r2)
    mfspr r30, 1019
    mfspr r31, 1017
    stmw r30, 688(r2)
    mfspr r16, 26
    stw r16, 696(r2)
    mfspr r17, 570
    stw r17, 700(r2)
    mfspr r25, 936
    mfspr r26, 937
    mfspr r27, 938
    mfspr r28, 939
    mfspr r29, 940
    mfspr r30, 941
    mfspr r31, 942
    stmw r25, 704(r2)
    mfspr r25, 571
    mfspr r26, 572
    mfspr r27, 573
    mfspr r28, 574
    mfspr r29, 575
    mfspr r30, 920
    mfspr r31, 1011
    stmw r25, 732(r2)
    mfspr r20, 912
    mfspr r21, 913
    mfspr r22, 914
    mfspr r23, 915
    mfspr r24, 916
    mfspr r25, 917
    mfspr r26, 918
    mfspr r27, 919
    mfspr r28, 920
    mfspr r29, 921
    mfspr r30, 922
    mfspr r31, 923
    stmw r20, 764(r2)
    blr
}

asm void TRKRestoreExtended1Block(void)
{
    nofralloc
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    lis r5, gTRKRestoreFlags@h
    ori r5, r5, gTRKRestoreFlags@l
    lbz r3, 0(r5)
    lbz r6, 1(r5)
    li r0, 0
    stb r0, 0(r5)
    stb r0, 1(r5)
    cmpwi r3, 0
    beq L_3c
    lwz r24, 488(r2)
    lwz r25, 492(r2)
    mtspr 284, r24
    mtspr 285, r25
    L_3c:
    lmw r20, 764(r2)
    mtspr 912, r20
    mtspr 913, r21
    mtspr 914, r22
    mtspr 915, r23
    mtspr 916, r24
    mtspr 917, r25
    mtspr 918, r26
    mtspr 919, r27
    mtspr 920, r28
    mtspr 922, r30
    mtspr 923, r31
    b L_70
    L_70:
    lmw r19, 644(r2)
    mtspr 1013, r19
    mtspr 953, r20
    mtspr 954, r21
    mtspr 957, r22
    mtspr 958, r23
    mtspr 955, r24
    mtspr 952, r25
    mtspr 956, r26
    mtspr 1019, r30
    mtspr 1017, r31
    b L_a0
    L_a0:
    lmw r16, 424(r2)
    mtsr 0, r16
    mtsr 1, r17
    mtsr 2, r18
    mtsr 3, r19
    mtsr 4, r20
    mtsr 5, r21
    mtsr 6, r22
    mtsr 7, r23
    mtsr 8, r24
    mtsr 9, r25
    mtsr 10, r26
    mtsr 11, r27
    mtsr 12, r28
    mtsr 13, r29
    mtsr 14, r30
    mtsr 15, r31
    lmw r12, 496(r2)
    mtspr 1008, r12
    mtspr 1009, r13
    mtspr 27, r14
    mtspr 287, r15
    mtspr 528, r16
    mtspr 529, r17
    mtspr 530, r18
    mtspr 531, r19
    mtspr 532, r20
    mtspr 533, r21
    mtspr 534, r22
    mtspr 535, r23
    mtspr 536, r24
    mtspr 537, r25
    mtspr 538, r26
    mtspr 539, r27
    mtspr 540, r28
    mtspr 541, r29
    mtspr 542, r30
    mtspr 543, r31
    lmw r22, 604(r2)
    mtspr 25, r22
    mtspr 19, r23
    mtspr 18, r24
    mtspr 272, r25
    mtspr 273, r26
    mtspr 274, r27
    mtspr 275, r28
    mtspr 1010, r30
    mtspr 282, r31
    blr
}
