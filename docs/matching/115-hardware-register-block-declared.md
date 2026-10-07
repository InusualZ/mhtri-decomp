---
id: 115
title: A hardware register block declared with the absolute-address declarator compiles to lis+add with no relocation
status: works
problem: indexed hardware-register access shows `lis rX,hi ; add ; lwz lo(rY)` in the target but `addis rY,rY,hi ; lwz lo(rY)` from a cast constant
tags: [data, source-shape, relocations]
applies: [Wii/1.3]
demo: 
---

# 115. A hardware register block declared with the absolute-address declarator compiles to lis+add with no relocation

**Problem.** An SDK unit indexes a register block (`EXI` channel registers at 0xCD006800, the IPC window at 0xCD000000, the low-memory probe
words at 0x800030C0). The target reads `mulli r3,chan,0x14 ; lis r0,0xcd00 ; add r3,r0,r3 ; lwz 0x6800(r3)` (or `lis r3,0xcd00 ; lwzx`), with no
relocation on the `lis`. Every cast-constant spelling (`((volatile u32(*)[5])0xCD006800)[chan][0]`, a `static const` pointer, a struct overlay,
`0xCD000000 + 0x6800 + chan * 0x14`) folds to `mulli ; addis r3,r3,-0x3300 ; lwz 0x6800(r3)`, one instruction short and different in two.

**Why it happens.** The original declared the block as a variable placed at its hardware address, so the base is a symbol (address operand
`@ha`/`@l` against an absolute symbol the linker resolves) and the compiler cannot fold it into an immediate add. dtk shows no relocation because the
symbol is absolute, so the diff looks like a plain constant.

**How to work it.** Declare the block with the absolute-address declarator and index it normally: this compiler accepts `type name[...] : 0xADDR;`
(the `@ADDR` spelling is rejected). No `.sdata`/`.sbss` object is emitted and the object carries no relocation for it.

**Result.** `EXI/EXIBios`: `__EXIRegs[3][5] : 0xCD006800` and `EXI_PROBE_TIME[2] : 0x800030C0` took the unit from 14 to 17 rows at 100 % and its
relocations from 187/196 to 195/196; `IPC/ipcMain`: `IPC_REGS[] : 0xCD000000` took `IPCReadReg` / `IPCWriteReg` from 69 % to 100 %.

**Example.**

```c
volatile u32 __EXIRegs[3][5] : 0xCD006800;       /* chan * 0x14 + lis/add, lo part is the displacement */
volatile u32 IPC_REGS[] : 0xCD000000;            /* IPC_REGS[reg] -> lis r3,0xcd00 ; lwzx */
```
