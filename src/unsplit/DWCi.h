/*
 * unsplit/DWCi.h - the DWCi band's include hub: it declares nothing of its own (docs/plan.md 6.5 rule 2).
 *
 * Every symbol the DWCi units call is owned now: the 0x8050A..-0x8050E.. socket / list / address layer and the
 * GT2 API by `DWCi/dwc_nasfunc.cpp` (`DWCi/dwc_nasfunc.h`, with the `DWCiHostEntry`/`DWCiHostAddr` records),
 * `DWC_Free` by `DWCi/dwc_error.cpp` (`DWCi/dwc_error.h`), the NATNEG half's data by `DWCi/DWCi_NatNeg.c`
 * (`.sdata` 0x80794368..0x807943A0, `.sbss` 0x80795828..0x80795878, `.bss` 0x807614D8..0x80762A20) and the Np
 * unit's by `DWCi/DWCi_Np_CPUCopyFast.c` (`.sdata` 0x80794200..0x80794210, `.sbss` 0x807957D0..0x807957F8).
 * `struct DWCiConn` / `struct DWCiReq` / `struct DWCiAddrKey` / `struct DWCiCType` are completed by
 * `src/DWCi/fn_805113B0.c`.
 */
#ifndef MHTRI_UNSPLIT_DWCI_H
#define MHTRI_UNSPLIT_DWCI_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"

#endif /* MHTRI_UNSPLIT_DWCI_H */
