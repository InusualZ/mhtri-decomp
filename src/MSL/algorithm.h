/* MSL/algorithm.h - the slice of MSL C++'s <algorithm> and <iterator> the g3d units instantiate: std::find and
 *   std::distance over a pointer range (ScnGroup::Remove(ScnObj*) walks its child array with them; the weak
 *   instances 0x8007B42C/0x8007B450/0x8007B47C sit in `g3d/g3d_scninline.cpp`'s range).  Only the pointer case is
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

/* The larger of two values (`g3d/g3d_light.cpp`'s LightSetting::ApplyViewMtx instantiates it over u32). */
template <class T>
const T& max(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

/* The sort family `g3d/g3d_draw.cpp` instantiates over its depth-sort records (0x80079604..0x80079F10): a
 * median-of-three quicksort with a jittered pivot that hands ranges of 20 or fewer to a selection sort.  Retail calls
 * every helper out of line, so none of them is declared inline.  The spelling is a GUESS rebuilt from those
 * instances. */

/* Hands its argument back (the swap's value-transfer step). */
template <class T>
T& move(T& x)
{
    return x;
}

/* Exchanges two values through a temporary. */
template <class T>
void swap(T& a, T& b)
{
    T tmp(move(a));
    a = move(b);
    b = move(tmp);
}

/* The first smallest element of [first, last) under `comp`. */
template <class ForwardIterator, class Compare>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last, Compare comp)
{
    ForwardIterator result = first;
    if (first != last) {
        for (++first; first != last; ++first) {
            if (comp(*first, *result)) {
                result = first;
            }
        }
    }
    return result;
}

/* Sorts a short range by repeatedly swapping the smallest remaining element forward. */
template <class RandomAccessIterator, class Compare>
void __selection_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    if (first == last) {
        return;
    }
    for (RandomAccessIterator back = last - 1; first != back; ++first) {
        RandomAccessIterator i = min_element<RandomAccessIterator, Compare>(first, last, comp);
        if (i != first) {
            swap(*i, *first);
        }
    }
}

/* Moves the median of three elements into the third. */
template <class RandomAccessIterator, class Compare>
void __median3(RandomAccessIterator a, RandomAccessIterator b, RandomAccessIterator c, Compare comp)
{
    bool cNotLessA = !comp(*c, *a);
    bool bNotLessC = !comp(*b, *c);
    if (cNotLessA && bNotLessC) {
        return;
    }
    if (!cNotLessA && !bNotLessC) {
        swap(*a, *b);
        return;
    }
    if (comp(*b, *a)) {
        swap(*a, *b);
    }
    if (cNotLessA) {
        swap(*b, *c);
    } else {
        swap(*a, *c);
    }
}

/* The quicksort loop: partitions around a jittered median of three, recurses into the shorter side. */
template <class RandomAccessIterator, class Compare>
void __sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    static int jitter = -4;
    for (;;) {
        long n = last - first;
        if (n <= 1) {
            return;
        }
        if (n <= 20) {
            __selection_sort<RandomAccessIterator, Compare>(first, last, comp);
            return;
        }
        RandomAccessIterator m1 = first + (n / 4 + jitter % 5);
        if (++jitter >= 5) {
            jitter = -4;
        }
        RandomAccessIterator m2 = first + 3 * n / 4 + jitter % 5;
        if (++jitter >= 5) {
            jitter = -4;
        }
        RandomAccessIterator j;
        RandomAccessIterator pivot = last - 1;
        __median3<RandomAccessIterator, Compare>(m1, m2, pivot, comp);
        RandomAccessIterator i = first;
        j = pivot;
        while (comp(*i, *pivot)) {
            ++i;
        }
        while (i != --j && !comp(*j, *pivot)) {
        }
        if (i < j) {
            swap(*i, *j);
            ++i;
            for (;;) {
                while (comp(*i, *pivot)) {
                    ++i;
                }
                while (!comp(*--j, *pivot)) {
                }
                if (i >= j) {
                    break;
                }
                swap(*i, *j);
                ++i;
            }
        }
        if (i == first) {
            swap(*i, *pivot);
            ++i;
            j = last - 1;
            if (!comp(*first, *j)) {
                while (i != last && !comp(*first, *i)) {
                    ++i;
                }
                if (i < j) {
                    swap(*i, *j);
                }
            }
            if (i < j) {
                for (;;) {
                    while (!comp(*first, *i)) {
                        ++i;
                    }
                    while (comp(*first, *--j)) {
                    }
                    if (i >= j) {
                        break;
                    }
                    swap(*i, *j);
                    ++i;
                }
            }
            first = i;
        } else if (i - first < last - i) {
            __sort<RandomAccessIterator, Compare>(first, i, comp);
            first = i;
        } else {
            __sort<RandomAccessIterator, Compare>(i, last, comp);
            last = i;
        }
    }
}

/* Sorts [first, last) under `comp`: the quicksort loop with the comparator held by reference. */
template <class RandomAccessIterator, class Compare>
void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
    static int jitter = -4;
    for (;;) {
        long n = last - first;
        if (n <= 1) {
            return;
        }
        if (n <= 20) {
            __selection_sort<RandomAccessIterator, Compare&>(first, last, comp);
            return;
        }
        RandomAccessIterator m1 = first + (n / 4 + jitter % 5);
        if (++jitter >= 5) {
            jitter = -4;
        }
        RandomAccessIterator m2 = first + 3 * n / 4 + jitter % 5;
        if (++jitter >= 5) {
            jitter = -4;
        }
        RandomAccessIterator j;
        RandomAccessIterator pivot = last - 1;
        __median3<RandomAccessIterator, Compare&>(m1, m2, pivot, comp);
        RandomAccessIterator i = first;
        j = pivot;
        while (comp(*i, *pivot)) {
            ++i;
        }
        while (i != --j && !comp(*j, *pivot)) {
        }
        if (i < j) {
            swap(*i, *j);
            ++i;
            for (;;) {
                while (comp(*i, *pivot)) {
                    ++i;
                }
                while (!comp(*--j, *pivot)) {
                }
                if (i >= j) {
                    break;
                }
                swap(*i, *j);
                ++i;
            }
        }
        if (i == first) {
            swap(*i, *pivot);
            ++i;
            j = last - 1;
            if (!comp(*first, *j)) {
                while (i != last && !comp(*first, *i)) {
                    ++i;
                }
                if (i < j) {
                    swap(*i, *j);
                }
            }
            if (i < j) {
                for (;;) {
                    while (!comp(*first, *i)) {
                        ++i;
                    }
                    while (comp(*first, *--j)) {
                    }
                    if (i >= j) {
                        break;
                    }
                    swap(*i, *j);
                    ++i;
                }
            }
            first = i;
        } else if (i - first < last - i) {
            __sort<RandomAccessIterator, Compare&>(first, i, comp);
            first = i;
        } else {
            __sort<RandomAccessIterator, Compare&>(i, last, comp);
            last = i;
        }
    }
}

}  // namespace std

#endif /* MHTRI_MSL_ALGORITHM_H */
