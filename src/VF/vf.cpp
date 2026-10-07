/*
 * VF/vf.cpp - the VF file system layer over PRFILE2: disk manager, FAT and system bring-up, hash and drive tables,
 *    the init and shutdown pair.
 *
 * RANGE. .text 0x80521340-0x80522334 (25 functions, 0xFF4 B); .rodata 0x80574E10-0x80579088; .bss
 *    0x80766C68-0x8078FA38; .sbss 0x807958F0-0x80795920.  Cut from the SO/soi.cpp end (0x80521340); the right
 *    edge 0x80522334 is the first function of the debugger channel (4-byte packed).  COARSE: the prfile2 source
 *    files (`VFipdm_*`, `VFiPF*`, `VFSys*`, `dHash_*`, `dCommon_*`) are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `VF*`, `dHash_*` and `dCommon_*` names are the map's or GUESSES in the prfile2 scheme (declared in
 *    `VF/vf.h`: `VFipf2Init`, `VFipf2Shutdown`, `VFipf2IsInitialized`); the file name is a GUESS from the library
 *    prefix.
 * EVIDENCE. `.rodata` 0x80574E10 (0x4278 B, a character code conversion table read by 0x80521730 and 0x805218F0) ends
 *    exactly where the KPR unit's table begins; `.bss` 0x80766C68..0x8078FA38 are the disk, volume and hash
 *    work areas read by `VFipdm_init_diskmanager`, 0x805215A0, `VFipf2Init`, `VFSysInit`,
 *    `dHash_InitHashTable` and `dCommon_initDriveInfo`; `.sbss` 0x807958F0..0x80795920 is the init state.
 * RESIDUALS. no bodies yet: all 25 functions are unwritten (largest fn_805218F0, 0x258 B).
 * SHAPES. none yet: no body is written, so nothing is measured.
 */

#include "VF/vf.h"
