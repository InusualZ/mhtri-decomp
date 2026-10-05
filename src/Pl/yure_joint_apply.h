/* Leaf header for `yure_joint_apply` (0x802AD738), owned by `Pl/pl_yure.cpp` whose full header is C++-only
 * (it defines the sway record); `enemy/fn_80138074.c` calls this from C, so the declaration is
 * C-compatible (rule 2).
 */
struct YureRec;
struct MtxHolder;

#ifndef MHTRI_PL_YURE_JOINT_APPLY_H
#define MHTRI_PL_YURE_JOINT_APPLY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Rotates the joint matrix `joint` holds by the sway record's two angle words (a no-op until the record
 * has seated its chain, i.e. while its state byte is 0). */
void yure_joint_apply(struct YureRec* rec, struct MtxHolder* joint);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_YURE_JOINT_APPLY_H */
