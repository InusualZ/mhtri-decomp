/* nw4r/db_assert.h - the cross-unit declarations of `nw4r/db_assert.cpp`: nw4r::db's assert reporters.
 *   C++ callers name the owner (`nw4r::db::Panic`), and the front-end emits the map's mangling
 *   (`Panic__Q24nw4r2dbFPCciPCce`); a C caller has no namespaces, so its declaration spells that mangling. */
#ifndef MHTRI_NW4R_DB_ASSERT_H
#define MHTRI_NW4R_DB_ASSERT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace db {

/* 0x80500AB0 - the assert failure handler: reports file, line and the formatted message, then halts. */
void Panic(const char* file, int line, const char* fmt, ...);

/* 0x80500B44 - the warning reporter: reports the formatted message and keeps running. */
void Warning(const char* file, int line, const char* fmt, ...);

}  // namespace db
}  // namespace nw4r
#else
void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);
#endif

#endif /* MHTRI_NW4R_DB_ASSERT_H */
