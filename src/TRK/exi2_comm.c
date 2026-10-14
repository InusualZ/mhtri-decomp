/*
 * TRK/exi2_comm.c - the MetroTRK debugger channel over EXI2: the channel and interrupt handlers, `DBInitComm`,
 *    `DBRead`, `DBWrite`, the reserve stubs and the EXI2 register and RAM transfers.
 *
 * RANGE. .text 0x80522334-0x80522E00 (15 functions, 0xACC B); .sdata 0x80794458-0x80794460; .sbss
 *    0x80795920-0x80795938.  Cut from the old VF block between `VF/vf.cpp` (0x80522334) and `PMIC/pmic.c`
 *    (0x80522E00).
 * FLAGS. the `OS` lib's `cflags_os` (4-byte function alignment): the run's function starts are packed at 4, unlike
 *    the rest of the old VF block. Compiler: GC/3.0a3 (GC/3.0a3..3.0a5 measure the same; Wii/1.3 leaves `__DBIntrHandler`,
 *    `DBInitComm`, `DBRead` and the register-order rows open, GC/3.0a5.2 opens `DBInitInterrupts`); the registration still
 *    names the lib's Wii/1.3 until the configure.py request (`sdk-trk#1`) is applied.
 * NAMES. the `DB*` and `__DB*` names are the map's; `EXI2_Reserve` and `EXI2_Unreserve` are GUESSES (4-byte stubs
 *    called around a continue/stop); the file name `exi2_comm.c` is a GUESS.
 * EVIDENCE. this is the only 4-byte-packed run of the old block (ten of its 174 function starts are not 16-aligned
 *    and all ten are here), so it is not one of the libraries around it; its only callers are `TRK/gdev_cc.c`
 *    (`DBInitComm`, `DBQueryData`, `DBRead`, `DBWrite`, `DBInitInterrupts`, the reserve stubs); `.sdata`
 *    0x80794458 (8 B) is read by `DBWrite` and `.sbss` 0x80795920..0x80795938 by the two handlers and the DB
 *    calls; the leaf headers of the exported rows sit beside `TRK/gdev_cc.c`.
 * RESIDUALS. under GC/3.0a3 every row but `__EXI2Imm` matches; `__EXI2Imm` copies the buffer pointer for the unrolled
 *    write loop at the loop entry (target: inside the unrolled preheader, the remainder loop indexing `buffer + i`), and
 *    the index spellings that move it (`buffer[i]`, `buffer + i` temporaries) break the rest of the unroll. Under the
 *    registered Wii/1.3 `__DBIntrHandler`, `DBInitComm`, `DBRead`, `DBWrite`, `__EXI2Imm`, the register rows and
 *    `__DBEXIWriteRam` stay open (scheduling and register numbering). Flipcheck blockers: the .text above and the
 *    trailing .sdata/.sbss padding.
 * SHAPES. the register accesses are spelled as absolute volatile words; the EXI2 select/deselect are inline helpers
 *    that stage the control word in a local; the transfer status starts `FALSE` and accumulates with `|=`; the byte
 *    swaps are written low byte first; `__EXI2Imm` walks the buffer with a copied pointer on the write path and the
 *    parameter itself on the read path.
 */
#include "types.h"
#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "TRK/DBInitComm.h"
#include "TRK/DBInitInterrupts.h"
#include "TRK/DBQueryData.h"
#include "TRK/DBRead.h"
#include "TRK/DBWrite.h"
#include "TRK/EXI2_Reserve.h"
#include "TRK/EXI2_Unreserve.h"

/* The EXI channel 2 registers and the processor-interface interrupt cause register. */
#define EXI2_CSR (*(volatile u32*)0xCD006828)
#define EXI2_CR (*(volatile u32*)0xCD006834)
#define EXI2_DATA (*(volatile u32*)0xCD006838)
#define PI_INTSR (*(volatile u32*)0xCC003000)

/* Debugger channel commands and masks. */
#define DB_STATUS_READ 0x34000000
#define DB_STATUS_BUSY 0x4
#define DB_RECV_READ 0x34000200
#define DB_MAILBOX_WRITE 0xB4000100

u8 exi2_send_count = 0x80;

/* Defined in reverse address order: the compiler lays .sbss out back to front. */
s32 exi2_recv_length;
u32 exi2_recv_header;
u8 exi2_input_pending;
void (*exi2_intr_callback)(s16 interrupt, OSContext* context);
void (*exi2_mtr_callback)(s32 arg);

BOOL __EXI2Imm(u8* buffer, s32 length, s32 write);
void __DBEXIInit(void);
/* untyped: a byte range of the caller */
BOOL __DBEXIReadReg(u32 command, void* buffer, s32 size);
/* untyped: a byte range of the caller */
BOOL __DBEXIWriteReg(u32 command, void* buffer, s32 size);
/* untyped: a byte range of the caller */
BOOL __DBEXIReadRam(u32 command, void* buffer, s32 length);
/* untyped: a byte range of the caller */
BOOL __DBEXIWriteRam(u32 command, void* buffer, s32 length);

/* Deselects the EXI2 device, keeping the interrupt mask bits. */
static inline void exi2_deselect(void)
{
    u32 csr = EXI2_CSR;

    csr &= 0x405;
    EXI2_CSR = csr;
}

/* Selects EXI2 device 0 at the debugger clock, keeping the interrupt mask bits. */
static inline void exi2_select(void)
{
    u32 csr = EXI2_CSR;

    csr &= 0x405;
    csr |= 0xC0;
    EXI2_CSR = csr;
}

/* Debugger-channel monitor interrupt: flags pending input and forwards to the registered callback. */
void __DBMtrHandler(s16 interrupt, OSContext* context)
{
    exi2_input_pending = 1;
    if (exi2_mtr_callback != NULL) {
        exi2_mtr_callback(0);
    }
}

/* Debugger-channel interrupt: acknowledges the interrupt and forwards to the registered callback. */
void __DBIntrHandler(s16 interrupt, OSContext* context)
{
    PI_INTSR = 0x1000;
    if (exi2_intr_callback != NULL) {
        exi2_intr_callback(interrupt, context);
    }
}

/* Registers the callback, hands out the pending-input flag and brings the EXI2 channel up. */
void DBInitComm(u32* inputPendingPtrRef, void (*handler)(void))
{
    BOOL enabled = OSDisableInterrupts();

    *inputPendingPtrRef = (u32)&exi2_input_pending;
    exi2_mtr_callback = (void (*)(s32))handler;
    __DBEXIInit();
    OSRestoreInterrupts(enabled);
}

/* Masks and installs the debugger-channel interrupt handlers. */
void DBInitInterrupts(void)
{
    __OSMaskInterrupts(0x18000);
    __OSMaskInterrupts(0x40);
    exi2_intr_callback = (void (*)(s16, OSContext*))__DBMtrHandler;
    __OSSetInterruptHandler(0x19, __DBIntrHandler);
    __OSUnmaskInterrupts(0x40);
}

/* Returns the number of bytes waiting on the channel, reading the mailbox when none is known. */
s32 DBQueryData(void)
{
    u8 status;
    u32 header;
    BOOL enabled;

    exi2_input_pending = 0;
    if (exi2_recv_length == 0) {
        enabled = OSDisableInterrupts();
        __DBEXIReadReg(DB_STATUS_READ, &status, 1);
        if (!(status & 8)) {
            __DBEXIReadReg(DB_RECV_READ, &header, 4);
            if ((header & 0x1F000000) == 0x1F000000) {
                exi2_recv_header = header;
                exi2_recv_length = header & 0x1FFF;
                exi2_input_pending = 1;
            }
        }
        OSRestoreInterrupts(enabled);
    }
    return exi2_recv_length;
}

/* Reads `length` bytes of the pending message from the channel's RAM window. */
s32 DBRead(u8* destination, s32 length)
{
    BOOL enabled = OSDisableInterrupts();
    u32 offset = ((exi2_recv_header >> 16) & 1) ? 0x800 : 0;

    __DBEXIReadRam(((offset + 0xD11000) << 6) & 0x3FFFFF00, destination, (length + 3) & ~3);
    exi2_recv_length = 0;
    exi2_input_pending = 0;
    OSRestoreInterrupts(enabled);
    return 0;
}

/* Writes `length` bytes to the channel's RAM window and signals the host. */
s32 DBWrite(const u8* source, s32 length)
{
    u32 mailbox;
    u8 statusBefore;
    u8 statusAfterRam;
    u8 statusAfterMailbox;
    u32 ramCommand;
    u32 rounded;
    u32 header;
    BOOL enabled = OSDisableInterrupts();

    do {
        __DBEXIReadReg(DB_STATUS_READ, &statusBefore, 1);
    } while (statusBefore & DB_STATUS_BUSY);
    exi2_send_count++;
    ramCommand = ((((exi2_send_count & 1) ? 0x800 : 0) + 0xD10000) << 6) & 0x3FFFFF00 | 0x80000000;
    rounded = (length + 3) & ~3;
    do {
    } while (__DBEXIWriteRam(ramCommand, (void*)source, rounded) == 0);
    do {
        __DBEXIReadReg(DB_STATUS_READ, &statusAfterRam, 1);
    } while (statusAfterRam & DB_STATUS_BUSY);
    header = ((exi2_send_count << 16) & 0xFF0000) | (((length & 0x1FFF) | 0x1F000000) & ~0xFF0000);
    do {
        mailbox = header;
    } while (__DBEXIWriteReg(DB_MAILBOX_WRITE, &mailbox, 4) == 0);
    do {
        __DBEXIReadReg(DB_STATUS_READ, &statusAfterMailbox, 1);
    } while (statusAfterMailbox & DB_STATUS_BUSY);
    OSRestoreInterrupts(enabled);
    return 0;
}

void EXI2_Reserve(void)
{
}

void EXI2_Unreserve(void)
{
}

/* Runs one immediate EXI2 transfer of up to four bytes, packing the bytes big-endian into the data register. */
BOOL __EXI2Imm(u8* buffer, s32 length, s32 write)
{
    u32 data;
    u8* p;
    s32 i;

    if (write != 0) {
        p = buffer;
        data = 0;
        for (i = 0; i < length; i++) {
            data |= *p++ << ((3 - i) * 8);
        }
        EXI2_DATA = data;
    }
    EXI2_CR = (write << 2) | 1 | ((length - 1) << 4);
    while (EXI2_CR & 1) {
    }
    if (write == 0) {
        data = EXI2_DATA;
        for (i = 0; i < length; i++) {
            *buffer++ = data >> ((3 - i) * 8);
        }
    }
    return TRUE;
}

/* Selects EXI2 device 0 and sends the two attention words that open the debugger channel. */
void __DBEXIInit(void)
{
    u32 attention1;
    u32 attention2;

    __OSMaskInterrupts(0x18000);
    while ((EXI2_CR & 1) == 1) {
    }
    EXI2_CSR = 0;
    attention1 = 0xB4000000;
    attention2 = 0xD4000000;
    exi2_select();
    __EXI2Imm((u8*)&attention1, 4, 1);
    while (EXI2_CR & 1) {
    }
    __EXI2Imm((u8*)&attention2, 4, 1);
    while (EXI2_CR & 1) {
    }
    exi2_deselect();
}

/* Reads a debugger register of 1, 2 or 4 bytes; returns nonzero when the transfer succeeded. */
/* untyped: a byte range of the caller */
BOOL __DBEXIReadReg(u32 command, void* buffer, s32 size)
{
    BOOL failed = FALSE;
    u32 value = 0;
    u32 request = command;

    exi2_select();
    failed |= __EXI2Imm((u8*)&request, 4, 1) == 0;
    while (EXI2_CR & 1) {
    }
    failed |= __EXI2Imm((u8*)&value, 4, 0) == 0;
    while (EXI2_CR & 1) {
    }
    exi2_deselect();
    switch (size) {
    case 1:
        *(u8*)buffer = value >> 24;
        break;
    case 2:
        *(u16*)buffer = (value >> 24) | ((value >> 8) & 0xFF00);
        break;
    default:
        *(u32*)buffer = (value >> 24) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | (value << 24);
        break;
    }
    return failed == 0;
}

/* Writes a debugger register of 1, 2 or 4 bytes; returns nonzero when the transfer succeeded. */
/* untyped: a byte range of the caller */
BOOL __DBEXIWriteReg(u32 command, void* buffer, s32 size)
{
    u32 value;
    u32 request = command;
    BOOL failed = FALSE;

    switch (size) {
    case 1:
        value = *(u8*)buffer << 24;
        break;
    case 2: {
        u16 half = *(u16*)buffer;

        value = (half << 24) | ((half << 8) & 0xFF0000);
        break;
    }
    default: {
        u32 word = *(u32*)buffer;

        value = (word >> 24) | ((word >> 8) & 0xFF00) | ((word << 8) & 0xFF0000) | (word << 24);
        break;
    }
    }
    exi2_select();
    failed |= __EXI2Imm((u8*)&request, 4, 1) == 0;
    while (EXI2_CR & 1) {
    }
    failed |= __EXI2Imm((u8*)&value, 4, 1) == 0;
    while (EXI2_CR & 1) {
    }
    exi2_deselect();
    return failed == 0;
}

/* Reads `length` bytes of the channel's RAM window; returns nonzero when every transfer succeeded. */
/* untyped: a byte range of the caller */
BOOL __DBEXIReadRam(u32 command, void* buffer, s32 length)
{
    u32 word;
    u32 request = command;
    BOOL failed = FALSE;
    u32* out = buffer;

    exi2_select();
    failed |= __EXI2Imm((u8*)&request, 4, 1) == 0;
    while (EXI2_CR & 1) {
    }
    while (length > 0) {
        failed |= __EXI2Imm((u8*)&word, 4, 0) == 0;
        while (EXI2_CR & 1) {
        }
        length -= 4;
        *out++ = word;
    }
    exi2_deselect();
    return failed == 0;
}

/* Writes `length` bytes of the channel's RAM window; returns nonzero when every transfer succeeded. */
/* untyped: a byte range of the caller */
BOOL __DBEXIWriteRam(u32 command, void* buffer, s32 length)
{
    u32 word;
    u32 request = command;
    BOOL failed = FALSE;
    u32* in = buffer;

    exi2_select();
    failed |= __EXI2Imm((u8*)&request, 4, 1) == 0;
    while (EXI2_CR & 1) {
    }
    while (length > 0) {
        word = *in;
        in++;
        failed |= __EXI2Imm((u8*)&word, 4, 1) == 0;
        while (EXI2_CR & 1) {
        }
        length -= 4;
    }
    exi2_deselect();
    return failed == 0;
}
