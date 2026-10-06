/* MSL/algorithm.h - the slice of MSL C++'s <algorithm> and <iterator> the g3d units instantiate: std::find and
 *   std::distance over a pointer range (ScnGroup::Remove(ScnObj*) walks its child array with them; the weak
 *   instances 0x8007B42C/0x8007B450/0x8007B47C sit in `g3d/fn_80075DCC.cpp`'s range).  Only the pointer case is
 *   spelled: the iterator category is always random access. */
#ifndef MHTRI_MSL_ALGORITHM_H
#define MHTRI_MSL_ALGORITHM_H

namespace std {

/* The iterator category tags; only the random-access one is dispatched on.  size: 0x1 each */
struct input_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

/* The first element of [first, last) equal to `value`, or `last`. */
template <class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& value)
{
    while (first != last && !(*first == value)) {
        ++first;
    }
    return first;
}

/* The element count of a random-access range. */
template <class RandomAccessIterator>
long __distance(RandomAccessIterator first, RandomAccessIterator last, random_access_iterator_tag)
{
    return last - first;
}

/* The element count of [first, last). */
template <class RandomAccessIterator>
long distance(RandomAccessIterator first, RandomAccessIterator last)
{
    return __distance(first, last, random_access_iterator_tag());
}

}  // namespace std

#endif /* MHTRI_MSL_ALGORITHM_H */
