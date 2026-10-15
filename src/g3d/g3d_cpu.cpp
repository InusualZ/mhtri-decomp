/* g3d/g3d_cpu - nw4r g3d CPU-side display-list primitives: a 32-byte block copy and a 0.0f block fill.
 * RANGE. .text 0x8009A748-0x8009AA78 (2 functions); extab, extabindex, .data 0x80591860-0x80591900 (the file name and
 *   three assert messages), .sdata2 0x80795F50-0x80795F58 (0.0f).
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` keeps the unfused `clrlwi`+`cmpwi` alignment tests,
 *   `#pragma pool_data off` gives every string its own `lis`/`addi` as retail does.
 * NAMES. Copy32ByteBlocks and ZeroMemory32ByteBlocks are nw4r::g3d::detail's names for the two primitives.
 * RESIDUALS. .sdata2: the object emits 0x4 of the claimed 0x8.
 * SHAPES. the strings are literals in first-use order; the three asserts pass retail's line numbers explicitly.
 *   ZeroMemory32ByteBlocks stores the fill with an inline `asm` block of four `psq_st` in the loop body; MWCC
 *   unrolls the loop eight times and rebases the block's offsets (0..248), as retail does.
 */

#include "types.h"
#include "g3d/g3d_cpu.h"

#pragma peephole off
#pragma pool_data off

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9); the owner (nw4r::db) is unsplit, so the declaration lives with its consumers. */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* `NW4R_ASSERT(expr)`: the file is the TU's own name, the message is the literalised expression and the
 * line is baked into the `Panic(file, line, msg)` call - the target passes the original file's line
 * numbers (28/29/30 and 94/95), not this reconstruction's, so they are explicit. */
#define G3D_CPU_ASSERT(expr, line, msg) \
    ((expr) ? (void)0 : nw4r::db::Panic("g3d_cpu.cpp", line, msg))

/* Copies `size` bytes from `pSrc` to `pDst` as 32-byte blocks (four 8-byte doubles each - the shape
 * MWCC lowers the block move to: `lfd`/`stfd` pairs, four blocks per unrolled iteration). */
namespace nw4r {
namespace g3d {
namespace detail {

/* untyped: byte range */
void Copy32ByteBlocks(void* pDst, const void* pSrc, u32 size) {
    G3D_CPU_ASSERT(pDst && !((u32)pDst & 0x3), 28, "NW4R:Failed assertion pDst && !((u32)pDst & 0x3)");
    G3D_CPU_ASSERT(pSrc && !((u32)pSrc & 0x3), 29, "NW4R:Failed assertion pSrc && !((u32)pSrc & 0x3)");
    G3D_CPU_ASSERT(size % 32 == 0, 30, "NW4R:Failed assertion size % 32 == 0");

    f64* dst = (f64*)pDst;
    const f64* src = (const f64*)pSrc;
    size /= 32;
    while (size != 0) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        src += 4;
        dst += 4;
        size--;
    }
}

/* Fills `size` bytes at `pDst` with 0.0f, one 32-byte block (four paired-single stores) at a time. */
/* untyped: byte range */
void ZeroMemory32ByteBlocks(void* pDst, u32 size) {
    G3D_CPU_ASSERT(pDst && !((u32)pDst & 0x3), 94, "NW4R:Failed assertion pDst && !((u32)pDst & 0x3)");
    G3D_CPU_ASSERT(size % 32 == 0, 95, "NW4R:Failed assertion size % 32 == 0");

    register f32 value = 0.0f;
    register u8* dst = (u8*)pDst;
    size /= 32;
    while (size != 0) {
        asm {
            psq_st value, 0(dst), 0, 0
            psq_st value, 8(dst), 0, 0
            psq_st value, 16(dst), 0, 0
            psq_st value, 24(dst), 0, 0
        }
        dst += 32;
        size--;
    }
}

}  // namespace detail
}  // namespace g3d
}  // namespace nw4r
