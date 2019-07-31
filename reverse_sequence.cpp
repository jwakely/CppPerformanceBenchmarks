/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various idioms for reversing the order of a sequence.


Assumptions:
    1) std::reverse and std::reverse_copy should be well optimized.
        BidirectionalIterator
        RandomAccessIterator


NOTE - reverse_n and reverse_copy_n templates would be useful for bidirectional and radom access iterators.
        So we have a counted loop that can be unrolled/vectorized as needed.





TODO - reverse linked lists
    http://stepanovpapers.com/notes.pdf
            lecture 22
    WARNING - needs to know details of list implementation!
    forward iter ( std::forward_list::reverse )
    bidirectional iter (std::list::reverse)

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 600000;


// about 8 to 64k of data
// intended to be inside L1/L2 cache on most CPUs
const size_t SIZE = 8000;


// 64Meg - outside of cache on most CPUs
const size_t LARGE_SIZE = 64*1024L*1024L;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 31;

/******************************************************************************/
/******************************************************************************/

// simple wrapper to make a pointer act like a bidirectional iterator
template<typename T>
struct BidirectionalPointer {
    T* current;
    BidirectionalPointer() {}
    BidirectionalPointer(T* x) : current(x) {}
    T& operator*() const { return *current; }
};

template <typename T>
inline BidirectionalPointer<T>& operator++(BidirectionalPointer<T> &xx) {
    ++xx.current;
    return xx;
}

template <typename T>
inline BidirectionalPointer<T>& operator--(BidirectionalPointer<T> &xx) {
    --xx.current;
    return xx;
}

template <typename T>
inline BidirectionalPointer<T> operator++(BidirectionalPointer<T> &xx, int) {
    BidirectionalPointer<T> tmp = xx;
    ++xx;
    return tmp;
}

template <typename T>
inline BidirectionalPointer<T> operator--(BidirectionalPointer<T> &xx, int) {
    BidirectionalPointer<T> tmp = xx;
    --xx;
    return tmp;
}

// kept for test convenience, otherwise it would be a lot messier
// not used in actual reverse routines
template <typename T>
inline BidirectionalPointer<T>& operator+=(BidirectionalPointer<T> &xx, ptrdiff_t inc) {
    xx.current += inc;
    return xx;
}

template <typename T>
inline bool operator==(const BidirectionalPointer<T>& x, const BidirectionalPointer<T>& y) {
    return (x.current == y.current);
}

template <typename T>
inline bool operator!=(const BidirectionalPointer<T>& x, const BidirectionalPointer<T>& y) {
    return (x.current != y.current);
}

/******************************************************************************/

namespace std {

    // tell STL what kinds of iterators these are

    template<typename T>
    struct iterator_traits< BidirectionalPointer<T> >
    {
        typedef bidirectional_iterator_tag iterator_category;
        typedef T                         value_type;
        typedef ptrdiff_t                 difference_type;
        typedef T*                        pointer;
        typedef T&                        reference;
    };

}    // end namespace std

/******************************************************************************/
/******************************************************************************/

template <typename Iterator>
void verify_sorted(Iterator first, Iterator last, const std::string &label) {
    if (!is_sorted(first,last))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename Iterator>
void verify_sorted_reverse(Iterator first, Iterator last, const std::string &label) {
    if (!is_sorted_reverse(first,last))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/
/******************************************************************************/

template<typename BidirectionalIterator>
void specific_simple_reverse(BidirectionalIterator begin,
                            BidirectionalIterator end,
                            std::bidirectional_iterator_tag) {
    while (begin != end) {
        --end;
        if (begin == end)
            break;
        std::swap(*begin,*end);
        ++begin;
    }
}

/******************************************************************************/

// convert iterator form to deterministic loop
template <class RandomAccessIterator>
void specific_simple_reverse(RandomAccessIterator begin,
                            RandomAccessIterator end,
                            std::random_access_iterator_tag) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    if (begin == end)
        return;
    
    _Distance loop_limit = (end - begin) / 2;
    --end;    // account for STL end semantics
    
    for ( _Distance j = 0; j < loop_limit; ++j, ++begin, --end ) {
        std::swap(*begin,*end);
    }
}

/******************************************************************************/

// and this will use the correct function based on the type of iterator
template<typename Iterator>
inline void my_simple_reverse(Iterator firstBuf, Iterator lastBuf)
    {
    if (firstBuf == lastBuf)  return;
    typedef typename std::iterator_traits<Iterator>::iterator_category _iter_type;
    specific_simple_reverse( firstBuf, lastBuf, _iter_type() );
    }

/******************************************************************************/

template<typename BidirectionalIterator, typename OutputIterator>
void specific_simple_reverse_copy(BidirectionalIterator begin,
                                BidirectionalIterator end,
                                OutputIterator result,
                                std::bidirectional_iterator_tag) {
    for (; begin != end; ++result)
        *result = *--end;
}

/******************************************************************************/

// convert iterator form to deterministic loop
template <class RandomAccessIterator, typename OutputIterator>
void specific_simple_reverse_copy(RandomAccessIterator begin,
                                RandomAccessIterator end,
                                OutputIterator result,
                                std::random_access_iterator_tag) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    
    if (begin == end)
        return;
    
    _Distance loop_limit = end - begin;
    --end;    // account for STL end semantics
    
    for ( _Distance j = 0; j < loop_limit; ++j, ++result, --end ) {
        *result = *end;
    }
}

/******************************************************************************/

// and this will use the correct function based on the type of iterator
template<typename Iterator, typename OutputIterator>
inline void my_simple_reverse_copy(Iterator firstBuf, Iterator lastBuf, OutputIterator result)
    {
    if (firstBuf == lastBuf)  return;
    typedef typename std::iterator_traits<Iterator>::iterator_category _iter_type;
    specific_simple_reverse_copy( firstBuf, lastBuf, result, _iter_type() );
    }

/******************************************************************************/
/******************************************************************************/

// Can't improve on this much with bidirectional iterators.
// If we knew the count to start with, we could use a counted loop like random access does and run faster.
// Getting the initial count by iterating would vary with type of iterator, and hurt if it causes cache thrashing (linked list).
template<typename BidirectionalIterator>
void specific_fast_reverse(BidirectionalIterator begin,
                            BidirectionalIterator end,
                            std::bidirectional_iterator_tag) {
    while (begin != end) {
        --end;
        if (begin == end)
            break;
        std::swap(*begin,*end);
        ++begin;
    }
}

/******************************************************************************/

#if WOULD_BE_NICE

// If we knew the count to start with, we could use a counted loop and run faster.
//     This should be an optional version available at the STL level (since most of the time the caller does know the count).
template<typename BidirectionalIterator>
void specific_reverse_n(BidirectionalIterator begin,
                        BidirectionalIterator end,
                        size_t count,
                        std::bidirectional_iterator_tag) {
    if (begin == end || count == 0)
        return;
    
    --end;    // account for STL end semantics

    // NOTE - this could also be unrolled as needed
    for ( size_t j = 0; j < count; ++j ) {
        std::swap(*begin,*end);
        ++begin; --end;
    }
}

template <class BidirectionalIterator, typename OutputIterator>
void specific_reverse_copy_n(BidirectionalIterator begin,
                            BidirectionalIterator end,
                            OutputIterator result,
                            size_t count,
                            std::bidirectional_iterator_tag) {
    if (begin == end || count == 0)
        return;
    
    --end;    // account for STL end semantics

    // NOTE - this could also be unrolled as needed
    for ( size_t j = 0; j < count; ++j) {
        *result = *end;
        ++result; --end;
    }
}

#endif  // WOULD_BE_NICE

/******************************************************************************/

// A simple, countable loop, unrolled 4X
template <class RandomAccessIterator>
void specific_fast_reverse(RandomAccessIterator begin,
                            RandomAccessIterator end,
                            std::random_access_iterator_tag) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = (end - begin) / 2;
    --end;    // account for STL end semantics

    j = 0;

    for ( ; j < (loop_limit - 3); j += 4, begin += 4, end -= 4 ) {
        std::swap(*(begin+0),*(end-0));
        std::swap(*(begin+1),*(end-1));
        std::swap(*(begin+2),*(end-2));
        std::swap(*(begin+3),*(end-3));
    }

    for ( ; j < loop_limit; ++j, ++begin, --end ) {
        std::swap(*begin,*end);
    }
}

/******************************************************************************/

// and this will use the correct function based on the type of iterator
template<typename Iterator>
inline void my_fast_reverse(Iterator begin, Iterator end)
    {
    if (begin == end)  return;
    typedef typename std::iterator_traits<Iterator>::iterator_category _iter_type;
    specific_fast_reverse( begin, end, _iter_type() );
    }

/******************************************************************************/
/******************************************************************************/

static inline
uint32_t reverse32_bytes(uint32_t input) {
        return (  ((input & 0xFF000000) >> 24)
                ^ ((input & 0x000000FF) << 24)
                ^ ((input & 0x00FF0000) >> 8)
                ^ ((input & 0x0000FF00) << 8) );
}

/******************************************************************************/

static inline
uint32_t reverse32_short(uint32_t input) {
        return (  ((input & 0xFFFF0000) >> 16)
                ^ ((input & 0x0000FFFF) << 16) );
}

/******************************************************************************/

static inline
uint64_t reverse64_bytes(uint64_t input) {
        return (  ((input >> 56) & 0x00000000000000FFULL)
                ^ ((input >> 40) & 0x000000000000FF00ULL)
                ^ ((input >> 24) & 0x0000000000FF0000ULL)
                ^ ((input >>  8) & 0x00000000FF000000ULL)
                ^ ((input << 56) & 0xFF00000000000000ULL)
                ^ ((input << 40) & 0x00FF000000000000ULL)
                ^ ((input << 24) & 0x0000FF0000000000ULL)
                ^ ((input <<  8) & 0x000000FF00000000ULL)
                );
}

/******************************************************************************/

static inline
uint64_t reverse64_short(uint64_t input) {
        return (  ((input >> 48) & 0x000000000000FFFFULL)
                ^ ((input >> 16) & 0x00000000FFFF0000ULL)
                ^ ((input << 48) & 0xFFFF000000000000ULL)
                ^ ((input << 16) & 0x0000FFFF00000000ULL)
                );
}

/******************************************************************************/

static inline
uint64_t reverse64_long(uint64_t input) {
        return (  ((input >> 32) & 0x00000000FFFFFFFFULL)
                ^ ((input << 32) & 0xFFFFFFFF00000000ULL)
                );
}

/******************************************************************************/

// A simple, countable loop, unrolled 4X
// use 32 bit load/store and shift values inside 32 bit word
template <class RandomAccessIterator>
void fast_reverse2(RandomAccessIterator begin,
                    RandomAccessIterator end) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = (end - begin) / 2;
    --end;    // account for STL end semantics

    j = 0;

    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 3); j += 4, begin += 4, end -= 4 ) {
            uint32_t left0 = *((uint32_t *)(begin+0));
            uint32_t right0 = *((uint32_t *)(end-3));
            
            left0 = reverse32_bytes(left0);
            right0 = reverse32_bytes(right0);
            
            *((uint32_t *)(begin+0)) = right0;
            *((uint32_t *)(end-3)) = left0;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 3); j += 4, begin += 4, end -= 4 ) {
            uint32_t left0 = *((uint32_t *)(begin+0));
            uint32_t left1 = *((uint32_t *)(begin+2));
            uint32_t right0 = *((uint32_t *)(end-1));
            uint32_t right1 = *((uint32_t *)(end-3));
            
            left0 = reverse32_short(left0);
            left1 = reverse32_short(left1);
            right0 = reverse32_short(right0);
            right1 = reverse32_short(right1);
            
            *((uint32_t *)(begin+0)) = right0;
            *((uint32_t *)(begin+2)) = right1;
            *((uint32_t *)(end-1)) = left0;
            *((uint32_t *)(end-3)) = left1;
        }
    else
        for ( ; j < (loop_limit - 3); j += 4, begin += 4, end -= 4 ) {
            std::swap(*(begin+0),*(end-0));
            std::swap(*(begin+1),*(end-1));
            std::swap(*(begin+2),*(end-2));
            std::swap(*(begin+3),*(end-3));
        }

    for ( ; j < loop_limit; ++j, ++begin, --end ) {
        std::swap(*begin,*end);
    }
}

/******************************************************************************/

// A simple, countable loop, unrolled 8X
// use 32 bit load/store and shift values inside 32 bit word
template <class RandomAccessIterator>
void fast_reverse3(RandomAccessIterator begin,
                    RandomAccessIterator end) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = (end - begin) / 2;
    --end;    // account for STL end semantics

    j = 0;

    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            uint32_t left0 = *((uint32_t *)(begin+0));
            uint32_t left1 = *((uint32_t *)(begin+4));
            
            uint32_t right0 = *((uint32_t *)(end-3));
            uint32_t right1 = *((uint32_t *)(end-7));
            
            right0 = reverse32_bytes(right0);
            right1 = reverse32_bytes(right1);
            
            left0 = reverse32_bytes(left0);
            left1 = reverse32_bytes(left1);
            
            *((uint32_t *)(begin+0)) = right0;
            *((uint32_t *)(begin+4)) = right1;
            *((uint32_t *)(end-3)) = left0;
            *((uint32_t *)(end-7)) = left1;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            uint32_t left0 = *((uint32_t *)(begin+0));
            uint32_t left1 = *((uint32_t *)(begin+2));
            uint32_t left2 = *((uint32_t *)(begin+4));
            uint32_t left3 = *((uint32_t *)(begin+6));
            
            uint32_t right0 = *((uint32_t *)(end-1));
            uint32_t right1 = *((uint32_t *)(end-3));
            uint32_t right2 = *((uint32_t *)(end-5));
            uint32_t right3 = *((uint32_t *)(end-7));
            
            right0 = reverse32_short(right0);
            right1 = reverse32_short(right1);
            right2 = reverse32_short(right2);
            right3 = reverse32_short(right3);
            
            left0 = reverse32_short(left0);
            left1 = reverse32_short(left1);
            left2 = reverse32_short(left2);
            left3 = reverse32_short(left3);
            
            *((uint32_t *)(begin+0)) = right0;
            *((uint32_t *)(begin+2)) = right1;
            *((uint32_t *)(begin+4)) = right2;
            *((uint32_t *)(begin+6)) = right3;
            *((uint32_t *)(end-1)) = left0;
            *((uint32_t *)(end-3)) = left1;
            *((uint32_t *)(end-5)) = left2;
            *((uint32_t *)(end-7)) = left3;
        }
    else
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            std::swap(*(begin+0),*(end-0));
            std::swap(*(begin+1),*(end-1));
            std::swap(*(begin+2),*(end-2));
            std::swap(*(begin+3),*(end-3));
            std::swap(*(begin+4),*(end-4));
            std::swap(*(begin+5),*(end-5));
            std::swap(*(begin+6),*(end-6));
            std::swap(*(begin+7),*(end-7));
        }

    for ( ; j < loop_limit; ++j, ++begin, --end ) {
        std::swap(*begin,*end);
    }
}

/******************************************************************************/

// A simple, countable loop, unrolled 8X
// use 64 bit load/store and shift values inside 64 bit word
template <class RandomAccessIterator>
void fast_reverse4(RandomAccessIterator begin,
                    RandomAccessIterator end) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = (end - begin) / 2;
    --end;    // account for STL end semantics

    j = 0;

    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            uint64_t left = *((uint64_t *)(begin+0));
            uint64_t right = *((uint64_t *)(end-7));
            
            left = reverse64_bytes(left);
            right = reverse64_bytes(right);
            
            *((uint64_t *)(begin+0)) = right;
            *((uint64_t *)(end-7)) = left;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            uint64_t left0 = *((uint64_t *)(begin+0));
            uint64_t left1 = *((uint64_t *)(begin+4));
            uint64_t right0 = *((uint64_t *)(end-3));
            uint64_t right1 = *((uint64_t *)(end-7));
            
            right0 = reverse64_short(right0);
            right1 = reverse64_short(right1);
            
            left0 = reverse64_short(left0);
            left1 = reverse64_short(left1);
            
            *((uint64_t *)(begin+0)) = right0;
            *((uint64_t *)(begin+4)) = right1;
            
            *((uint64_t *)(end-3)) = left0;
            *((uint64_t *)(end-7)) = left1;
        }
    else if (sizeof(T) == 4)
        for ( ; j < (loop_limit - 7); j += 8, begin += 8, end -= 8 ) {
            uint64_t left0 = *((uint64_t *)(begin+0));
            uint64_t left1 = *((uint64_t *)(begin+2));
            uint64_t left2 = *((uint64_t *)(begin+4));
            uint64_t left3 = *((uint64_t *)(begin+6));
            uint64_t right0 = *((uint64_t *)(end-1));
            uint64_t right1 = *((uint64_t *)(end-3));
            uint64_t right2 = *((uint64_t *)(end-5));
            uint64_t right3 = *((uint64_t *)(end-7));
            
            right0 = reverse64_long(right0);
            right1 = reverse64_long(right1);
            right2 = reverse64_long(right2);
            right3 = reverse64_long(right3);
            
            left0 = reverse64_long(left0);
            left1 = reverse64_long(left1);
            left2 = reverse64_long(left2);
            left3 = reverse64_long(left3);
            
            *((uint64_t *)(begin+0)) = right0;
            *((uint64_t *)(begin+2)) = right1;
            *((uint64_t *)(begin+4)) = right2;
            *((uint64_t *)(begin+6)) = right3;
            
            *((uint64_t *)(end-1)) = left0;
            *((uint64_t *)(end-3)) = left1;
            *((uint64_t *)(end-5)) = left2;
            *((uint64_t *)(end-7)) = left3;
        }
    else
        for ( ; j < (loop_limit - 3); j += 4, begin += 4, end -= 4 ) {
            std::swap(*(begin+0),*(end-0));
            std::swap(*(begin+1),*(end-1));
            std::swap(*(begin+2),*(end-2));
            std::swap(*(begin+3),*(end-3));
        }

    for ( ; j < loop_limit; ++j, ++begin, --end ) {
        std::swap(*begin,*end);
    }
}

/******************************************************************************/
/******************************************************************************/

// Can't improve on this much with bidirectional iterators.
// If we knew the count to start with, we could use a counted loop like random access does and run faster.
// Getting the initial count by iterating would vary with type of iterator, and hurt if it causes cache thrashing (linked list).
template<typename BidirectionalIterator, typename OutputIterator>
void specific_fast_reverse_copy(BidirectionalIterator begin,
                                BidirectionalIterator end,
                                OutputIterator result,
                                std::bidirectional_iterator_tag) {
    for (; begin != end; ++result)
        *result = *--end;
}

/******************************************************************************/

// A simple, countable loop, unrolled 4X
template <class RandomAccessIterator, typename OutputIterator>
void specific_fast_reverse_copy(RandomAccessIterator begin,
                                RandomAccessIterator end,
                                OutputIterator result,
                                std::random_access_iterator_tag) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = end - begin;
    --end;    // account for STL end semantics

    j = 0;

    for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
        *(result+0) = *(end-0);
        *(result+1) = *(end-1);
        *(result+2) = *(end-2);
        *(result+3) = *(end-3);
    }

    for ( ; j < loop_limit; ++j, --end, ++result ) {
        *result = *end;
    }
}

/******************************************************************************/

// and this will use the correct function based on the type of iterator
template<typename Iterator, typename OutputIterator>
inline void my_fast_reverse_copy(Iterator begin, Iterator end, OutputIterator result)
    {
    if (begin == end)  return;
    typedef typename std::iterator_traits<Iterator>::iterator_category _iter_type;
    specific_fast_reverse_copy( begin, end, result, _iter_type() );
    }

/******************************************************************************/

// A simple, countable loop, unrolled 4X
// use 32 bit load/store and shift values inside 32 bit word
template <class RandomAccessIterator, typename OutputIterator>
void fast_reverse_copy2(RandomAccessIterator begin,
                        RandomAccessIterator end,
                        OutputIterator result) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = end - begin;
    --end;    // account for STL end semantics

    j = 0;

    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
            uint32_t right0 = *((uint32_t *)(end-3));
            
            right0 = reverse32_bytes(right0);
            
            *((uint32_t *)(result+0)) = right0;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
            uint32_t right0 = *((uint32_t *)(end-1));
            uint32_t right1 = *((uint32_t *)(end-3));
            
            right0 = reverse32_short(right0);
            right1 = reverse32_short(right1);
            
            *((uint32_t *)(result+0)) = right0;
            *((uint32_t *)(result+2)) = right1;
        }
    else
        for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
            *(result+0) = *(end-0);
            *(result+1) = *(end-1);
            *(result+2) = *(end-2);
            *(result+3) = *(end-3);
        }

    for ( ; j < loop_limit; ++j, --end, ++result ) {
        *result = *end;
    }
}

/******************************************************************************/

// A simple, countable loop, unrolled 8X
// use 32 bit load/store and shift values inside 32 bit word
template <class RandomAccessIterator, typename OutputIterator>
void fast_reverse_copy3(RandomAccessIterator begin,
                        RandomAccessIterator end,
                        OutputIterator result) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = end - begin;
    --end;    // account for STL end semantics

    j = 0;

    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 7); j += 8, result += 8, end -= 8 ) {
            uint32_t right0 = *((uint32_t *)(end-3));
            uint32_t right1 = *((uint32_t *)(end-7));
            
            right0 = reverse32_bytes(right0);
            right1 = reverse32_bytes(right1);
            
            *((uint32_t *)(result+0)) = right0;
            *((uint32_t *)(result+4)) = right1;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 7); j += 8, result += 8, end -= 8 ) {
            uint32_t right0 = *((uint32_t *)(end-1));
            uint32_t right1 = *((uint32_t *)(end-3));
            uint32_t right2 = *((uint32_t *)(end-5));
            uint32_t right3 = *((uint32_t *)(end-7));
            
            right0 = reverse32_short(right0);
            right1 = reverse32_short(right1);
            right2 = reverse32_short(right2);
            right3 = reverse32_short(right3);
            
            *((uint32_t *)(result+0)) = right0;
            *((uint32_t *)(result+2)) = right1;
            *((uint32_t *)(result+4)) = right2;
            *((uint32_t *)(result+6)) = right3;
        }
    else
        for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
            *(result+0) = *(end-0);
            *(result+1) = *(end-1);
            *(result+2) = *(end-2);
            *(result+3) = *(end-3);
        }

    for ( ; j < loop_limit; ++j, --end, ++result ) {
        *result = *end;
    }
}

/******************************************************************************/

// A simple, countable loop, unrolled 8X
// use 64 bit load/store and shift values inside 64 bit word
template <class RandomAccessIterator, typename OutputIterator>
void fast_reverse_copy4(RandomAccessIterator begin,
                        RandomAccessIterator end,
                        OutputIterator result) {
    typedef typename std::iterator_traits<RandomAccessIterator>::difference_type _Distance;
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
    
    if (begin == end)
        return;
    
    _Distance j;
    _Distance loop_limit = end - begin;
    --end;    // account for STL end semantics

    j = 0;


    if (sizeof(T) == 1)
        for ( ; j < (loop_limit - 7); j += 8, result += 8, end -= 8 ) {
            uint64_t right = *((uint64_t *)(end-7));
            
            right = reverse64_bytes(right);
            
            *((uint64_t *)(result+0)) = right;
        }
    else if (sizeof(T) == 2)
        for ( ; j < (loop_limit - 7); j += 8, result += 8, end -= 8 ) {
            uint64_t right0 = *((uint64_t *)(end-3));
            uint64_t right1 = *((uint64_t *)(end-7));
            
            right0 = reverse64_short(right0);
            right1 = reverse64_short(right1);
            
            *((uint64_t *)(result+0)) = right0;
            *((uint64_t *)(result+4)) = right1;
        }
    else if (sizeof(T) == 4)
        for ( ; j < (loop_limit - 7); j += 8, result += 8, end -= 8 ) {
            uint64_t right0 = *((uint64_t *)(end-1));
            uint64_t right1 = *((uint64_t *)(end-3));
            uint64_t right2 = *((uint64_t *)(end-5));
            uint64_t right3 = *((uint64_t *)(end-7));
            
            right0 = reverse64_long(right0);
            right1 = reverse64_long(right1);
            right2 = reverse64_long(right2);
            right3 = reverse64_long(right3);
            
            *((uint64_t *)(result+0)) = right0;
            *((uint64_t *)(result+2)) = right1;
            *((uint64_t *)(result+4)) = right2;
            *((uint64_t *)(result+6)) = right3;
        }
    else
        for ( ; j < (loop_limit - 3); j += 4, result += 4, end -= 4 ) {
            *(result+0) = *(end-0);
            *(result+1) = *(end-1);
            *(result+2) = *(end-2);
            *(result+3) = *(end-3);
        }

    for ( ; j < loop_limit; ++j, --end, ++result ) {
        *result = *end;
    }
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

// test with a simple template
template <class Iterator, typename RR>
void test_reverse(Iterator begin, Iterator end, RR func, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        func( begin, end );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    verify_sorted( begin, end, label );
}

/******************************************************************************/

// test with a simple template
template <class Iterator, typename RR>
void test_reverse_copy(Iterator begin, Iterator end, Iterator result, Iterator resultEnd, RR func, const std::string label) {

    std::fill( result, resultEnd, 99 );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        func( begin, end, result );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    verify_sorted_reverse( result, resultEnd, label );
}

/******************************************************************************/
/******************************************************************************/

#define OUTPUT_CSV  0

// test rotate functions on different sized arrays
template <class Iterator, typename RR>
void test_reverse_sizes(Iterator firstDest, const int max_count, RR func, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;

#if OUTPUT_CSV
    printf("description, seconds, \"ops per sec.\"\n");
    
    for( i = 30, j = 0; i <= max_count; i+=(i>>1), ++j ) {
//    for( i = 1, j = 0; i <= 60; ++i, ++j ) {               // profile small buffer performance
//    for( i = 1, j = 0; i <= 100; ++i, ++j ) {            // profile cutoffs between algorithms
#else
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");
    
    for( i = 4, j = 0; i <= max_count; i *= 2, ++j ) {
#endif
        int64_t temp1 = max_count / i;
        int64_t temp2 = saved_iterations * temp1;

#if OUTPUT_CSV
    // keep the counts small while profiling
        if (temp2 > 0x700000)
            temp2 = 0x700000;
#else
        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
#endif
        if (temp2 < 4)
            temp2 = 4;
        
        temp2 = (temp2+1) & ~1; // make sure iteration count is even
        
        iterations = temp2;
    
        Iterator lastTemp = firstDest;
        lastTemp += ptrdiff_t(i);    // we can do this only because we defined our own iterators
        
        test_reverse(firstDest, lastTemp, func, label);
        
        // items * iterations != actual swaps, but still a good measure
        double millions = ((double)(i) * iterations)/1000000.0;
        
#if OUTPUT_CSV
        printf("\"%s %d items\", %5.2f, %5.2f\n",
                label.c_str(),
                i,
                results[0].time,
                millions/results[0].time );
#else
        printf("%2i \"%s %d items\"  %5.2f sec   %5.2f M\n",
                j,
                label.c_str(),
                i,
                results[0].time,
                millions/results[0].time );
#endif
    
        // reset the test counter
        current_test = 0;
    }

    iterations = saved_iterations;
}

/******************************************************************************/

// test rotate functions on different sized arrays
template <class Iterator, typename RR>
void test_reverse_copy_sizes(Iterator firstDest, Iterator firstResult, const int max_count, RR func, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;

#if OUTPUT_CSV
    printf("description, seconds, \"ops per sec.\"\n");
    
    for( i = 30, j = 0; i <= max_count; i+=(i>>1), ++j ) {
//    for( i = 1, j = 0; i <= 60; ++i, ++j ) {               // profile small buffer performance
//    for( i = 1, j = 0; i <= 100; ++i, ++j ) {            // profile cutoffs between algorithms
#else
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");
    
    for( i = 4, j = 0; i <= max_count; i *= 2, ++j ) {
#endif
        int64_t temp1 = max_count / i;
        int64_t temp2 = saved_iterations * temp1;

#if OUTPUT_CSV
    // keep the counts small while profiling
        if (temp2 > 0x700000)
            temp2 = 0x700000;
#else
        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
#endif
        if (temp2 < 4)
            temp2 = 4;
        
        temp2 = (temp2+1) & ~1; // make sure iteration count is even
        
        iterations = temp2;
    
        Iterator lastTemp = firstDest;
        lastTemp += ptrdiff_t(i);    // we can do this only because we defined our own iterators
        
        Iterator lastTempResult = firstResult;
        lastTempResult += ptrdiff_t(i);    // we can do this only because we defined our own iterators
        
        test_reverse_copy(firstDest, lastTemp, firstResult, lastTempResult, func, label);
        
        // items * iterations != actual swaps, but still a good measure
        double millions = ((double)(i) * iterations)/1000000.0;
        
#if OUTPUT_CSV
        printf("\"%s %d items\", %5.2f, %5.2f\n",
                label.c_str(),
                i,
                results[0].time,
                millions/results[0].time );
#else
        printf("%2i \"%s %d items\"  %5.2f sec   %5.2f M\n",
                j,
                label.c_str(),
                i,
                results[0].time,
                millions/results[0].time );
#endif
    
        // reset the test counter
        current_test = 0;
    }

    iterations = saved_iterations;
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void TestOneType()
{
    auto base_iterations = iterations;
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    const size_t large_count = LARGE_SIZE / sizeof(T);
    const size_t item_count = std::max( SIZE, large_count );
    
    std::unique_ptr<T> storage(new T[ item_count ]);
    T *data = storage.get();
    
    std::unique_ptr<T> storageResult(new T[ item_count ]);
    T *dataResult = storageResult.get();
    
    
    fill_descending( data, data+item_count, item_count+init_value );
    // sort, to account for aliasing of values in some types
    std::sort( data, data+item_count );
    
    
    auto dataBB = BidirectionalPointer<T>(data);
    auto dataBE = BidirectionalPointer<T>(data+SIZE);
    using BPT = BidirectionalPointer<T>;
    

    // test basics, in cache
    test_reverse( dataBB, dataBE, std::reverse<BPT>, myTypeName + " std::reverse bidirectional");
    test_reverse( dataBB, dataBE, my_simple_reverse<BPT>, myTypeName + " simple_reverse bidirectional");
    test_reverse( dataBB, dataBE, my_fast_reverse<BPT>, myTypeName + " fast_reverse bidirectional");
    
    test_reverse( data, data+SIZE, std::reverse<T*>, myTypeName + " std::reverse random access");
    test_reverse( data, data+SIZE, my_simple_reverse<T*>, myTypeName + " simple_reverse random access");
    test_reverse( data, data+SIZE, my_fast_reverse<T*>, myTypeName + " fast_reverse random access");
    test_reverse( data, data+SIZE, fast_reverse2<T*>, myTypeName + " fast_reverse2 random access");
    test_reverse( data, data+SIZE, fast_reverse3<T*>, myTypeName + " fast_reverse3 random access");
    test_reverse( data, data+SIZE, fast_reverse4<T*>, myTypeName + " fast_reverse4 random access");
    
    std::string temp1( myTypeName + " reverse");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // test different sizes, in and out of cache
    iterations = base_iterations / (16*1024);

    test_reverse_sizes( dataBB, large_count, std::reverse<BPT>, myTypeName + " std::reverse bidirectional" );
    test_reverse_sizes( dataBB, large_count, my_simple_reverse<BPT>, myTypeName + " simple_reverse bidirectional" );
    test_reverse_sizes( dataBB, large_count, my_fast_reverse<BPT>, myTypeName + " fast_reverse bidirectional" );

    test_reverse_sizes( data, large_count, std::reverse<T*>, myTypeName + " std::reverse random access" );
    test_reverse_sizes( data, large_count, my_simple_reverse<T*>, myTypeName + " simple_reverse random access" );
    test_reverse_sizes( data, large_count, my_fast_reverse<T*>, myTypeName + " fast_reverse random access" );
    test_reverse_sizes( data, large_count, fast_reverse2<T*>, myTypeName + " fast_reverse2 random access" );
    test_reverse_sizes( data, large_count, fast_reverse3<T*>, myTypeName + " fast_reverse3 random access" );
    test_reverse_sizes( data, large_count, fast_reverse4<T*>, myTypeName + " fast_reverse4 random access" );



    iterations = base_iterations;

    // test basics, in cache
    test_reverse_copy( dataBB, dataBE, BPT(dataResult), BPT(dataResult+SIZE),
                        std::reverse_copy<BPT,BPT>, myTypeName + " std::reverse_copy bidirectional");
    test_reverse_copy( dataBB, dataBE, BPT(dataResult), BPT(dataResult+SIZE),
                        my_simple_reverse_copy<BPT,BPT>, myTypeName + " simple_reverse_copy bidirectional");
    test_reverse_copy( dataBB, dataBE, BPT(dataResult), BPT(dataResult+SIZE),
                        my_fast_reverse_copy<BPT,BPT>, myTypeName + " fast_reverse_copy bidirectional");
    
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, std::reverse_copy<T*,T*>, myTypeName + " std::reverse_copy random access");
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, my_simple_reverse_copy<T*,T*>, myTypeName + " simple_reverse_copy random access");
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, my_fast_reverse_copy<T*,T*>, myTypeName + " fast_reverse_copy random access");
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, fast_reverse_copy2<T*,T*>, myTypeName + " fast_reverse_copy2 random access");
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, fast_reverse_copy3<T*,T*>, myTypeName + " fast_reverse_copy3 random access");
    test_reverse_copy( data, data+SIZE, dataResult, dataResult+SIZE, fast_reverse_copy4<T*,T*>, myTypeName + " fast_reverse_copy4 random access");
    
    std::string temp2( myTypeName + " reverse_copy");
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // test different sizes, in and out of cache
    iterations = base_iterations / (16*1024);

    test_reverse_copy_sizes( dataBB, BPT(dataResult), large_count,
                            std::reverse_copy<BPT, BPT>, myTypeName + " std::reverse_copy bidirectional" );
    test_reverse_copy_sizes( dataBB, BPT(dataResult), large_count,
                            my_simple_reverse_copy<BPT, BPT>, myTypeName + " simple_reverse_copy bidirectional" );
    test_reverse_copy_sizes( dataBB, BPT(dataResult), large_count,
                            my_fast_reverse_copy<BPT, BPT>, myTypeName + " fast_reverse_copy bidirectional" );
    
    test_reverse_copy_sizes( data, dataResult, large_count, std::reverse_copy<T*,T*>, myTypeName + " std::reverse_copy random access" );
    test_reverse_copy_sizes( data, dataResult, large_count, my_simple_reverse_copy<T*,T*>, myTypeName + " simple_reverse_copy random access" );
    test_reverse_copy_sizes( data, dataResult, large_count, my_fast_reverse_copy<T*,T*>, myTypeName + " fast_reverse_copy random access" );
    test_reverse_copy_sizes( data, dataResult, large_count, fast_reverse_copy2<T*,T*>, myTypeName + " fast_reverse_copy2 random access" );
    test_reverse_copy_sizes( data, dataResult, large_count, fast_reverse_copy3<T*,T*>, myTypeName + " fast_reverse_copy3 random access" );
    test_reverse_copy_sizes( data, dataResult, large_count, fast_reverse_copy4<T*,T*>, myTypeName + " fast_reverse_copy4 random access" );


    
    iterations = base_iterations;
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int32_t) atoi(argv[2]);
    
    // make sure we have an even number of iterations
    iterations = (iterations+1) & ~1;
    
    
    // only size of the type should matter here, but compiler bugs still happen

    TestOneType<uint8_t>();
    
    TestOneType<int16_t>();
    
    TestOneType<int32_t>();
    
    TestOneType<float>();
    
    TestOneType<uint64_t>();
    
    TestOneType<double>();


#if THESE_WORK_BUT_TAKE_A_WHILE_TO_RUN
    TestOneType<int8_t>();
    TestOneType<uint16_t>();
    TestOneType<uint32_t>();
    TestOneType<int64_t>();
    TestOneType<long double>();
#endif


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
