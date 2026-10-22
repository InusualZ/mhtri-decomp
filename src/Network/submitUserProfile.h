/* Leaf header (docs/plan.md 6.5 rule 2) for `submitUserProfile` (0x80427630), the `Network/network_pat_control.cpp`
 * profile submit stub (a bare `blr`).  C linkage (the map row is unmangled).  The lobby's profile submit passes the
 * profile it just built in r3, so the parameter is declared here; the owner's definition still spells `(void)`. */
#ifndef MHTRI_NETWORK_SUBMITUSERPROFILE_H
#define MHTRI_NETWORK_SUBMITUSERPROFILE_H

#include "types.h"

#ifdef __cplusplus
struct NetUserProfile;
extern "C" {
/* 0x80427630 - submits `profile` to the match server (an empty body in this build). */
void submitUserProfile(NetUserProfile* profile);
}
#endif

#endif /* MHTRI_NETWORK_SUBMITUSERPROFILE_H */
