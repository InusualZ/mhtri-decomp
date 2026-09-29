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

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051B7FC - make a context for a connection: the verification option and the host name. */
s32 SSLNew(u32 verifyOption, char* host);

/* 0x8051B954 - attach the context to the connected socket. */
s32 SSLConnect(s32 ssl, s32 fd);

/* 0x8051BA1C - run one step of the handshake; 1 once it completed, a negative code to retry or fail. */
s32 SSLDoHandshake(s32 ssl);

/* 0x8051BAC8 / 0x8051BD98 - read into / write from a byte range. */
s32 SSLRead(s32 ssl, void* buf, s32 length); /* untyped: byte range */
s32 SSLWrite(s32 ssl, void* buf, s32 length); /* untyped: byte range */

/* 0x8051C058 - close the context. */
s32 SSLShutdown(s32 ssl);

/* 0x8051C104 / 0x8051C480 - the client certificate, given as bytes or as a built-in id. */
s32 SSLSetClientCert(s32 ssl, u32 cert, u32 certLength, u32 key, u32 keyLength);
s32 SSLSetBuiltinClientCert(s32 ssl, u32 id);

/* 0x8051C270 / 0x8051C3B8 - the root CA, given as bytes or as a built-in id. */
s32 SSLSetRootCA(s32 ssl, u32 ca, u32 caLength);
s32 SSLSetBuiltinRootCA(s32 ssl, u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SSL_H */
