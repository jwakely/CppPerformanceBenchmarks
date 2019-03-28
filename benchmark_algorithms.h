/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )
    
    Shared source file for algorithms used in multiple benchmark files

*/

#include <iterator>
#include <algorithm>

namespace benchmark {
    
/******************************************************************************/

template<typename T>
bool isSigned() { return (T(-1) < T(1)); }

template<typename T>
bool isFloat() { return (T(2.1) > T(2)); }

template<typename T>
bool isInteger() { return (T(2.1) == T(2)); }

/******************************************************************************/

template <typename ForwardIterator>
bool is_sorted(ForwardIterator first, ForwardIterator last) {
    auto prev = first;
    first++;
    while (first != last) {
        if ( *first++ < *prev++)
            return false;
    }
    return true;
}

/******************************************************************************/

template <typename ForwardIterator>
bool is_sorted_reverse(ForwardIterator first, ForwardIterator last) {
    auto prev = first;
    first++;
    while (first != last) {
        if ( *prev++ < *first++ )
            return false;
    }
    return true;
}

/******************************************************************************/

template <typename ForwardIterator>
bool is_sorted_reverse(ForwardIterator first, size_t count) {
    auto prev = first;
    first++;
    count--;
    while (count > 0) {
        if ( *prev++ < *first++ )
            return false;
        count--;
    }
    return true;
}

/******************************************************************************/

template <typename ForwardIterator, typename T>
void fill(ForwardIterator first, ForwardIterator last, T value) {
    while (first != last) *first++ = T(value);
}

/******************************************************************************/

template <typename ForwardIterator>
void fill_random(ForwardIterator first, ForwardIterator last) {
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    while (first != last) {
        *first++ = static_cast<T>( rand() >> 3 );
    }
}

/******************************************************************************/

template <class ForwardIterator>
void fill_ascending(ForwardIterator first, ForwardIterator last) {
    size_t i = 0;
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    while (first != last) {
        *(first++) = static_cast<T>( i++ );
    }
}

/******************************************************************************/

template <class ForwardIterator>
void fill_descending(ForwardIterator first, ForwardIterator last, size_t count) {
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    while (first != last) {
        *(first++) = static_cast<T>( --count );
    }
}

/******************************************************************************/

template <class ForwardIterator>
void fill_alternating(ForwardIterator first, ForwardIterator last, size_t count) {
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    for (size_t i=0; first != last; ++i, ++first){
        if (i & 0x01)
            *first = static_cast<T>( i );
        else
            *first = static_cast<T>( count-i );
    }
}

/******************************************************************************/

template <class ForwardIterator>
void fill_steps(ForwardIterator first, ForwardIterator last, size_t count, size_t steps) {
    using T = typename std::iterator_traits<ForwardIterator>::value_type;
    size_t run_length = count / steps;
    if (run_length < 1) run_length = 1; // happens when count < steps
    for (size_t i=0; first != last; ++i) {
        *first++ = T( i / run_length );
    }
}

/******************************************************************************/

template <typename ForwardIterator, typename ForwardIterator2>
void copy(ForwardIterator firstSource, ForwardIterator lastSource, ForwardIterator2 firstDest) {
    while (firstSource != lastSource) *(firstDest++) = *(firstSource++);
}

/******************************************************************************/

template <class BidirectionalIterator, class Swapper>
void reverse(BidirectionalIterator begin, BidirectionalIterator end, Swapper doswap)
{
    while (begin != end)
    {
        --end;
        if (begin == end)
            break;
        doswap(begin, end);
        ++begin;
    }
}

/******************************************************************************/

// our accumulator function template, using iterators or pointers
template <typename ForwardIterator, typename Number>
Number accumulate(ForwardIterator first, ForwardIterator last, Number result) {
    while (first != last) result =  result + *first++;
    return result;
}

/******************************************************************************/

// https://en.wikipedia.org/wiki/Insertion_sort

template <typename BidirectionalIterator>
void insertionSort( BidirectionalIterator begin, BidirectionalIterator end )
{
    if (begin == end)
        return;
    auto p = begin;
    ++p;

    while ( p != end ) {
        auto tmp = *p;
        auto j = p;
        auto prev = j;

        for (  ; (j != begin) && (tmp < *--prev); --j ) {
            *j = *prev;
        }

        *j = tmp;
        ++p;
    }
}

/******************************************************************************/

// https://en.wikipedia.org/wiki/Quicksort

// Very simple implementation. Also very slow in many cases.
template<typename RandomAccessIterator>
void quicksort( RandomAccessIterator begin, RandomAccessIterator end )
{
    if ( (end - begin) > 1 ) {

        auto middleValue = *begin;
        auto left = begin;
        auto right = end;

        for(;;) {

            while ( middleValue < *(--right) );
            if ( !(left < right ) ) break;
            
            while ( *(left) < middleValue )
                ++left;
            if ( !(left < right ) ) break;

            // swap
            auto temp = *right;
            *right = *left;
            *left = temp;
        }

        quicksort( begin, right + 1 );
        quicksort( right + 1, end );
    }
}

/******************************************************************************/

// https://en.wikipedia.org/wiki/Quicksort

// Very simple implementation. Also very slow in many cases.
template<typename RandomAccessIterator, class Swapper>
void quicksort( RandomAccessIterator begin, RandomAccessIterator end, Swapper doswap )
{
    if ( (end - begin) > 1 ) {

        auto middleValue = *begin;
        auto left = begin;
        auto right = end;

        for(;;) {

            while ( middleValue < *(--right) );
            if ( !(left < right ) ) break;
            
            while ( *(left) < middleValue )
                ++left;
            if ( !(left < right ) ) break;

            // swap
            doswap( right, left );
        }

        quicksort<RandomAccessIterator,Swapper>( begin, right + 1, doswap );
        quicksort<RandomAccessIterator,Swapper>( right + 1, end, doswap );
    }
}

/******************************************************************************/

// https://en.wikipedia.org/wiki/Bogosort
// Just to see if you're paying attention...

template <class RandomAccessIterator>
void bogosort( RandomAccessIterator begin, RandomAccessIterator end )
{
    do {
        std::random_shuffle(begin,end);
    } while (!is_sorted(begin,end));
}

/******************************************************************************/

template<typename RandomAccessIterator, typename T>
void __sift_in(ptrdiff_t count, RandomAccessIterator begin, ptrdiff_t free_in, T next)
{
    ptrdiff_t i;
    ptrdiff_t free = free_in;

    // sift up the free node 
    for ( i = 2*(free+1); i < count; i += i) {
        if ( *(begin+(i-1)) < *(begin+i))
            i++;
        *(begin + free) = *(begin+(i-1));
        free = i-1;
    }

    // special case in sift up if the last inner node has only 1 child
    if (i == count) {
        *(begin + free) = *(begin+(i-1));
        free = i-1;
    }

    // sift down the new item next
    i = (free-1)/2;
    while( (free > free_in) && *(begin+i) < next) {
        *(begin + free) = *(begin+i);
        free = i;
        i = (free-1)/2;
    }

    *(begin + free) = next;
}

// https://en.wikipedia.org/wiki/Heapsort

template<typename RandomAccessIterator>
void heapsort( RandomAccessIterator begin, RandomAccessIterator end )
{
    ptrdiff_t count = end - begin;

    // build the heap structure 
    for(ptrdiff_t j = (count / 2) - 1; j >= 0; --j) {
        auto next = *(begin+j);
        __sift_in(count, begin, j, next);
    }

    // put each max element in place
    for(ptrdiff_t j = count - 1; j >= 1; --j) {
        auto next = *(begin+j);
        *(begin+j) = *(begin);
        __sift_in(j, begin, 0, next);
    }
}

/******************************************************************************/

}    // end namespace benchmark

using namespace benchmark;

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
