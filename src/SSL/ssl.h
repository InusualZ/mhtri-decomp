/* Declarations owned by `src/SSL/ssl.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_SSL_SSL_H
#define MHTRI_SSL_SSL_H

#include "types.h"

/* Declarations moved here from `unsplit/NCD.h, SSL.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8051C554 - fill the caller's interface-configuration block; non-zero on failure (the DWCi
 * runtime initialiser prints its own " NCDGetCurrentIfConfig failed.[%d]\n" with the answer and
 * hands it the `+0x4000` region of its runtime block).  The name is read off that call site's own
 * message: this band is unregistered, so this header is the symbol's home until `ncdsystem.c`
 * registers, and that unit's own header will be the real one then. */
s32 NCDGetCurrentIfConfig(u8* config);

/* 0x8051B7FC - make a context for a connection: the verification option and the host name. */
s32 SSLNew(u32 verifyOption, char* host);

/* 0x8051B954 - attach the context to the connected socket. */
s32 SSLConnect(s32 ssl, s32 fd);

/* 0x8051BA1C - run one step of the handshake; 1 once it completed, a negative code to retry or fail. */
s32 SSLDoHandshake(s32 ssl);

/* 0x8051BAC8 / 0x8051BD98 - read into / write from a byte range. */
s32 SSLRead(s32 ssl, void* buf, s32 length); /* untyped: byte range */

s32 SSLWrite(s32 ssl, void* buf, s32 length); /* untyped: byte range */

/* 0x8051D048 - the user-facing network error code for a failed `SOStartup` result (the SDK's NET
 * helper of that name; GUESS from its NCD-band neighbours and its one caller, which logs the negated
 * result as "Network Error Code is %d"). */
s32 NETGetStartupErrorCode(s32 result);

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

#endif /* MHTRI_SSL_SSL_H */
