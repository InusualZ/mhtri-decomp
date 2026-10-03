/* Declarations owned by `src/Network/network_opening.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_NETWORK_OPENING_H
#define MHTRI_NETWORK_NETWORK_OPENING_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;

#ifdef __cplusplus
extern "C" {
#endif

u32 isTermsUpdateFinished(struct PatTerms* terms);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_OPENING_H */
