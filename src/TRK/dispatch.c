/*
 * TRK/dispatch.c - the MetroTRK command dispatcher (`TRKDispatchMessage`, one switch).
 *
 * RANGE. .text 0x80468C2C..0x80468D4C (1 function in the map, 0x120 B); .data 0x8060F538..0x8060F5A8.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `dispatch`); the command enum names are GUESS from the handler each value reaches.
 * EVIDENCE. `.data` 0x8060F538 (the 0x6C B jump table, 27 slots) is read only by it; the table maps 1, 2, 3, 7, 16..19,
 *    23..26 to the twelve handlers, every other value to the unknown-command result.
 * RESIDUALS. none known.
 * SHAPES. the result starts as 0x500 (unknown command) and is replaced by the handler's return value.
 */
#include "TRK/dispatch.h"
#include "TRK/msghndlr.h"

#define TRK_ERR_UNKNOWN_COMMAND 0x500

typedef enum TRKCommand {
    TRK_CMD_CONNECT = 1,
    TRK_CMD_DISCONNECT = 2,
    TRK_CMD_RESET = 3,
    TRK_CMD_OVERRIDE = 7,
    TRK_CMD_READ_MEMORY = 16,
    TRK_CMD_WRITE_MEMORY = 17,
    TRK_CMD_READ_REGISTERS = 18,
    TRK_CMD_WRITE_REGISTERS = 19,
    TRK_CMD_SET_OPTION = 23,
    TRK_CMD_CONTINUE = 24,
    TRK_CMD_STEP = 25,
    TRK_CMD_STOP = 26
} TRKCommand;

s32 TRKDispatchMessage(TRKBuffer* message)
{
    s32 result = TRK_ERR_UNKNOWN_COMMAND;

    TRK_SetBufferPosition(message, 0);
    switch (message->data[4]) {
    case TRK_CMD_CONNECT:
        result = TRK_DoConnect(message);
        break;
    case TRK_CMD_DISCONNECT:
        result = TRKDoDisconnect(message);
        break;
    case TRK_CMD_RESET:
        result = TRKDoReset(message);
        break;
    case TRK_CMD_OVERRIDE:
        result = TRKDoOverride(message);
        break;
    case TRK_CMD_READ_MEMORY:
        result = TRKDoReadMemory(message);
        break;
    case TRK_CMD_WRITE_MEMORY:
        result = TRKDoWriteMemory(message);
        break;
    case TRK_CMD_READ_REGISTERS:
        result = TRKDoReadRegisters(message);
        break;
    case TRK_CMD_WRITE_REGISTERS:
        result = TRKDoWriteRegisters(message);
        break;
    case TRK_CMD_CONTINUE:
        result = TRKDoContinue(message);
        break;
    case TRK_CMD_STEP:
        result = TRKDoStep(message);
        break;
    case TRK_CMD_STOP:
        result = TRKDoStop(message);
        break;
    case TRK_CMD_SET_OPTION:
        result = TRKDoSetOption(message);
        break;
    }
    return result;
}
