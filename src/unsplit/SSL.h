/*
 * SSL declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The SSL library sits at 0x8051B7FC and up, right behind the NHTTP band: `SSLNew` is the first
 * function past `NHTTPi_InitConnectionList`.  No registered unit covers it, so the calls the NHTTP
 * core (`NHTTP/d_nhttp.c`) makes into it are declared here.  A context is the small integer handle
 * `SSLNew` returns; every other call takes it first and answers 0 (or a byte count) on success and a
 * negative code on failure.
 */
#ifndef MHTRI_UNSPLIT_SSL_H
#define MHTRI_UNSPLIT_SSL_H

#include "types.h"
#include "SSL/ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SSL_H */
