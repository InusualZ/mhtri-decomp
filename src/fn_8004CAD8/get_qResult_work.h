/*
 * Leaf header (docs/plan.md 6.5 rule 2): `get_qResult_work` (0x8004D140), defined by `src/fn_8004CAD8.cpp`.
 * `fn_8004CAD8.h` includes this one, so there is one declaration; it is separate so a lobby unit can reach
 * the accessor without the rest of that header.  C++ scope: the owner defines it without `extern "C"`, so
 * the map row is the mangling `get_qResult_work__Fv` (the mangling carries no return type).  The record is
 * `quest/quest_result_work.h`'s `Q_ResultWork`.
 */
#ifndef MHTRI_FN_8004CAD8_GET_QRESULT_WORK_H
#define MHTRI_FN_8004CAD8_GET_QRESULT_WORK_H

#ifdef __cplusplus
/* The 0x438-byte quest result work block. */
struct Q_ResultWork* get_qResult_work(void);
#endif

#endif /* MHTRI_FN_8004CAD8_GET_QRESULT_WORK_H */
