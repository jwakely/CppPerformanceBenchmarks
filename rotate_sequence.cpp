/*
    Copyright 2004-2009 Adobe Systems Incorporated
    Copyright 2018-2021 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goals: Test performance of various ways to rotate the order of a sequence.
       Test performance of std::rotate.

Minor Goals: Teach a bit about algorithm design and practicality


Assumptions:

    1) std::rotate should be well optimized for all data types and sequence sizes
        ForwardIterator            -- typically gries-mills
        BidirectionalIterator      -- typically 3 reverses
        RandomAccessIterator       -- typically gcd cycle






TODO - this could use more tuning of fallback algorithm choices on different processors
TODO - this could use more tuning of buffer sizes on various processors
TODO - figure out where copy vs copy_n are best (iterator vs. counted loops)

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
#include <forward_list>
#include <list>
#include <vector>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

using namespace std;

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 900000;


// about 4 to 32k of data
// intended to be inside L1/L2 cache on most CPUs
const size_t SIZE = 4000;


// 64Meg - outside of cache on most CPUs
const size_t LARGE_SIZE = 64*1024L*1024L;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

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
// not used in actual rotate routines
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

// simple wrapper to make a pointer act like a forward iterator
template<typename T>
struct ForwardPointer {
    T* current;
    ForwardPointer() {}
    ForwardPointer(T* x) : current(x) {}
    T& operator*() const { return *current; }
};

template <typename T>
inline ForwardPointer<T>& operator++(ForwardPointer<T> &xx) {
    ++xx.current;
    return xx;
}

template <typename T>
inline ForwardPointer<T> operator++(ForwardPointer<T> &xx, int) {
    ForwardPointer<T> tmp = xx;
    ++xx;
    return tmp;
}

// kept for test convenience, otherwise it would be a lot messier
// not used in actual rotate routines
template <typename T>
inline ForwardPointer<T>& operator+=(ForwardPointer<T> &xx, ptrdiff_t inc) {
    xx.current += inc;
    return xx;
}

template <typename T>
inline bool operator==(const ForwardPointer<T>& x, const ForwardPointer<T>& y) {
    return (x.current == y.current);
}

template <typename T>
inline bool operator!=(const ForwardPointer<T>& x, const ForwardPointer<T>& y) {
    return (x.current != y.current);
}

/******************************************************************************/
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

    template<typename T>
    struct iterator_traits< ForwardPointer<T> >
    {
        typedef forward_iterator_tag      iterator_category;
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
    if (!benchmark::is_sorted(first,last))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
T my_gcd(T aa, T bb) {
    while ( bb != 0 ) {
        T tmp = aa % bb;
        aa = bb;
        bb = tmp;
    }
    return aa;
}

/******************************************************************************/

// basic gries-mills rotate with uncountable loop
template<typename ForwardIterator>
void gries_mills_rotate(ForwardIterator first,
                        ForwardIterator middle,
                        ForwardIterator last)
{
    if (first == middle || middle == last)
        return;
    
    auto current = middle;
    while (true) {
        swap(*first, *current);
        ++first;
        ++current;
        
        if (current == last) {
            if (first == middle)
                return;
            current = middle;
        } else if (first == middle) {
            middle = current;
        }
    }
}

/******************************************************************************/

// basic 3 reverse rotate
template<typename BidirectionalIterator>
void three_reverse_rotate(  BidirectionalIterator first,
                            BidirectionalIterator middle,
                            BidirectionalIterator last )
{
    if (first == middle || middle == last)
        return;
    
    std::reverse(first, middle);
    std::reverse(middle, last);
    std::reverse(first, last);
}

/******************************************************************************/

// Simple gcd cycle with single temp value and equal length optimization
template<typename RandomAccessIterator>
void gcd_cycle_random(  RandomAccessIterator first,
                        RandomAccessIterator middle,
                        RandomAccessIterator last)
{
    if (first == middle || middle == last)
        return;
    
    auto forward  = middle - first;
    auto backward = last - middle;
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }
    
    auto new_middle = last - forward;   // == first + backward;
    auto end =  first + my_gcd(forward, backward);
    auto start = first;

    while (start < end) {
        auto value  = *start;
        auto hole = start;
        auto next = start + forward;
        
        while (next != start)  {
            *hole = *next;
            hole = next;
            next += (new_middle > hole) ? forward : -backward;
        }
        
        *hole = value;
        ++start;
    }
}

/******************************************************************************/

// simple gcd cycle with single temp value and equal length optimization
// altered for forward iterators
// VERY slow because of std::advance and std::distance
template<typename ForwardIterator>
void gcd_cycle_forward( ForwardIterator first,
                         ForwardIterator middle,
                         ForwardIterator last)
{
    if (first == middle || middle == last)
        return;
    
    auto forward = std::distance( first, middle );
    auto backward = std::distance( middle, last );
    //auto total = forward + backward;
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }
    
    //auto new_middle = last - forward;  // == first + backward;    // last = first + forward + backward;
    //size_t new_middle_index = backward;
    auto end = first;
    auto cycles = my_gcd(forward, backward);
    std::advance(end,cycles);
    
    auto start = first;
    ptrdiff_t start_index = 0;

    while (start != end) {
        auto value = *start;
        
        auto hole = start;
        ptrdiff_t hole_index = start_index;
        
        auto next = start;
        std::advance(next,forward);
        ptrdiff_t next_index = start_index + forward;
        
        while (next != start)  {
            *hole = *next;
            hole = next;
            hole_index = next_index;
            
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                if (next_index >= forward) {
                    next = middle;
                    std::advance(next,(next_index-forward));
                } else {
                    next = first;
                    std::advance(next,next_index);
                }
            }
        }
        
        *hole = value;
        ++start;
        ++start_index;
    }

}

/******************************************************************************/

// simple gcd cycle with single temp value and equal length optimization
// altered for bidirectional iterators
// VERY slow because of std::advance and std::distance
template<typename BidirectionalIterator>
void gcd_cycle_bidirectional( BidirectionalIterator first,
                             BidirectionalIterator middle,
                             BidirectionalIterator last )
{
    if (first == middle || middle == last)
        return;
    
    auto forward = std::distance( first, middle );
    auto backward = std::distance( middle, last );
    //auto total = forward + backward;
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }
    
    //auto new_middle = last - forward;   // == first + backward;     // last = first + forward + backward;
    //size_t new_middle_index = backward;
    auto end = first;
    auto cycles = my_gcd(forward, backward);
    std::advance(end,cycles);
    
    auto start = first;
    ptrdiff_t start_index = 0;

    while (start != end) {
        auto value = *start;
        
        auto hole = start;
        ptrdiff_t hole_index = start_index;
        
        auto next = start;
        std::advance(next,forward);
        ptrdiff_t next_index = start_index + forward;
        
        while (next != start)  {
            *hole = *next;
            hole = next;
            hole_index = next_index;
            
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                std::advance(next,-backward);
            }
        }
        
        *hole = value;
        ++start;
        ++start_index;
    }

}

/******************************************************************************/
/******************************************************************************/

// Best results were from buffer sizes a bit less than half dcache, not a multiple of 64 (cache line), not a multiple of 4096 (page size).
// We want the aux buffer large, but not so large that it causes cache or TLB thrashing.
// Best values need to be studied on more processors (not just Intel i7/Xeon)
const size_t fwd_rotate_storage_bytes = 24547;
const size_t bi_rotate_storage_bytes = 24547;
const size_t rotate_storage_bytes = 24547;

const size_t rotate_small_cutoff = 30;

/******************************************************************************/

// change the gries-mills loop so it is countable
// overhead hurts on small sequences
template<typename ForwardIterator>
void gries_mills_rotate_counted(ForwardIterator first,
                                ForwardIterator middle,
                                ForwardIterator last,
                                size_t forward,
                                size_t backward)
{
    if (first == middle || middle == last)
        return;
    
    if (forward == 0) {
        forward = std::distance( first, middle );
        backward = std::distance( middle, last );
    }
    
    size_t middle_index = forward;
    auto current = middle;
    size_t current_index = forward;
    size_t first_index = 0;
    size_t last_index = forward + backward;

    while (true) {
        size_t loop_end = std::min( (last_index - current_index), (middle_index - first_index) );
        //size_t loop_end = std::min( backward, forward );

        for ( size_t i = 0; i < loop_end; ++i ) {
            swap(*first, *current);
            ++first;
            ++current;
        }
        
        first_index += loop_end;
        current_index += loop_end;
        
        // was a bit faster to use indices than compare iterators
        if (current_index == last_index) {
            if (first_index == middle_index)
                return;
            current = middle;
            current_index = middle_index;
            //forward -= backward;
        } else if (first_index == middle_index) {
            middle = current;
            middle_index = current_index;
            //backward -= forward;
        }
    }
}

/******************************************************************************/

template<typename ForwardIterator>
void gries_mills_rotate_counted_wrapper(ForwardIterator first,
                                        ForwardIterator middle,
                                        ForwardIterator last)
{
    gries_mills_rotate_counted(first,middle,last,0,0);
}

/******************************************************************************/

// change the gries-mills loop so it is countable
// apply other optimizations using known distances
template<typename ForwardIterator>
void gries_mills_rotate_iterative(ForwardIterator first,
                                    ForwardIterator middle,
                                    ForwardIterator last,
                                    size_t forward,
                                    size_t backward)
{
    typedef typename std::iterator_traits<ForwardIterator>::value_type _ValueType;
    
    if (first == middle || middle == last)
        return;

    if (forward == 0) {
        forward = std::distance( first, middle );
        backward = std::distance( middle, last );
    }
    
    // iteratively position the output values by swapping ranges
    while (true) {

#if 0
        {  // debugging
            assert( forward > 0 );
            assert( backward > 0 );
            auto temp_fwd = std::distance( first, middle );
            auto temp_back = std::distance( middle, last );
            assert( temp_fwd == forward );
            assert( temp_back == backward );
        }
#endif

        if (forward == backward) {
            std::swap_ranges(first, middle, middle);
            return;
        }
        
        if ( (forward+backward) < 20 ) {        // for small sequences, just use a simple loop (reduced overhead)
            gries_mills_rotate(first,middle,last);
            return;
        }
        
        size_t loop_end = std::min( backward, forward );
        
        const size_t aux_storage_count = fwd_rotate_storage_bytes / sizeof(_ValueType);
        
        if (loop_end <= aux_storage_count) {
            _ValueType values[ aux_storage_count ];
        
            auto new_middle = first;
            if (backward >= forward) {
                new_middle = middle;
                std::advance(new_middle,(backward-forward));
            } else
                std::advance(new_middle,backward);
            
            // use the aux storage and copy for small rotates
            if (forward <= aux_storage_count) {
                std::copy(first, middle, values);               // copy first few into aux
                std::copy(middle, last, first);                 // move bulk of array (down in array)
                std::copy_n(values, forward, new_middle );      // copy aux to end
                return;
            }
            
            if (backward <= aux_storage_count) {
                std::copy(middle, last, values);              // copy last few into aux
                if (backward > forward) {
                    std::copy_n(first, forward, new_middle);      // move bulk of array up, without overlap
                    std::copy_n(values, backward, first);         // copy aux to beginning
                } else {
// TODO - can this be done with larger chunk size?
// works, because dist == increment for forward movement
// but possibly copying small blocks at a time
                    auto total = forward+backward;  // move forward section up, in chunks, using swapping with aux
                    auto dist = backward;
                    while (total > 0) {
                        for (size_t i = 0; i < dist; ++i)
                            swap( *first++, values[i] );
                        total -= dist;
                        if (dist > total)
                            dist = total;
                    }
                }
                return;
            }
        
        }   // can use local buffer to speed things up
        
        
        auto current = middle;

        for ( size_t i = 0; i < loop_end; ++i ) {
            swap(*first, *current);
            ++first;
            ++current;
        }
        
        // new first += min(forward,backward)
        // new current = middle + min(forward,backward)

        if (backward < forward) {
            forward -= backward;
        } else {
            middle = current;
            backward -= forward;
        }
    
    }   // while true

}

/******************************************************************************/

template<typename ForwardIterator>
void gries_mills_rotate_iterative_wrapper(ForwardIterator first,
                                            ForwardIterator middle,
                                            ForwardIterator last)
{
    gries_mills_rotate_iterative(first,middle,last,0,0);
}

/******************************************************************************/

// gcd cycle with temporary buffer
// algorithm by Mat Marcus and Chris Cox  ~2004
// adapted for forward iterators
// This is not very practical, but was useful for experimentation
template<typename ForwardIterator>
void gcd_cycle_buffered_forward(  ForwardIterator first,
                                    ForwardIterator middle,
                                    ForwardIterator last)
{
    typedef typename std::iterator_traits<ForwardIterator>::value_type _ValueType;
    
    if (first == middle || middle == last)
        return;
    
    auto forward = std::distance( first, middle );
    auto backward = std::distance( middle, last );
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }

    const size_t aux_storage_count = fwd_rotate_storage_bytes / sizeof(_ValueType);

    if ( (aux_storage_count < 1) || ((forward+backward) < rotate_small_cutoff) ) {
        gries_mills_rotate(first,middle,last,forward,backward);
        return;
    }
    
    _ValueType values[ aux_storage_count ];

    auto new_middle = first;
    if (backward >= forward) {
        new_middle = middle;
        std::advance(new_middle,(backward-forward));
    } else
        std::advance(new_middle,backward);
    // middle_index = forward;
    // new_middle_index = backward;
    
    // use the aux storage and copy for small rotates
    if (forward <= aux_storage_count) {
        std::copy(first, middle, values);             // copy first few into aux
        std::copy(middle, last, first);               // move bulk of array (down in array)
        std::copy_n(values, forward, new_middle );    // copy aux to end
        return;
    }
    
    if (backward <= aux_storage_count) {
        std::copy(middle, last, values);              // copy last few into aux
        if (backward > forward) {
            std::copy_n(first, forward, new_middle);      // move bulk of array (up in array, not overlap)
            std::copy_n(values, backward, first);         // copy aux to beginning
        } else {
            auto total = forward+backward;  // copy forward section up, in chunks, using swapping with buffer
            auto dist = backward;
            while (total > 0) {
                for (size_t i = 0; i < dist; ++i)
                    swap( *first++, values[i]);
                total -= dist;
                if (dist > total)
                    dist = total;
            }
        }
        return;
    }

    // on large sequences the gcd is frequently 1, or very small
    size_t num_cycles = my_gcd(forward, backward);

    if (num_cycles < 50) {
        gries_mills_rotate_iterative(first,middle,last,forward,backward);
        return;
    }
    
    size_t cycles_remaining = num_cycles;
    size_t chunk_size = cycles_remaining;
    auto begin = first; // because we will increment first
    size_t first_offset = 0;
    
    if ( chunk_size > aux_storage_count )
        chunk_size = aux_storage_count;
    
    while ( cycles_remaining > 1 ) {
        
        std::copy_n(first, chunk_size, values);
        
        auto hole = first;          // first is incremented in this loop!
        size_t hole_index = 0;
        
        auto next = first;          // first is incremented in this loop!
        std::advance(next,forward);
        size_t next_index = first_offset + forward;

        while (next != first) {
            std::copy_n(next, chunk_size, hole);
            hole = next;
            hole_index = next_index;
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                // try to use the iterator closest to the next point, so we do fewer advances
                if (forward > backward) {
                    if (next_index >= forward) {
                        next = middle;
                        std::advance(next,(next_index-forward));
                    } else if (next_index >= backward) {
                        next = new_middle;
                        std::advance(next,(next_index-backward));
                    } else if (next_index >= first_offset) {
                        next = first;
                        std::advance(next,(next_index-first_offset));
                    } else {
                        next = begin;
                        std::advance(next,next_index);
                    }
                } else {
                    if (next_index >= backward) {
                        next = new_middle;
                        std::advance(next,(next_index-backward));
                    } else if (next_index >= forward) {
                        next = middle;
                        std::advance(next,(next_index-forward));
                    }  else if (next_index >= first_offset) {
                        next = first;
                        std::advance(next,(next_index-first_offset));
                    } else {
                        next = begin;
                        std::advance(next,next_index);
                    }
                }
            }
        }
        
        std::copy_n(values, chunk_size, hole);
        cycles_remaining -= chunk_size;
        std::advance(first,chunk_size);
        first_offset += chunk_size;
        
        if ( chunk_size > cycles_remaining )
            chunk_size = cycles_remaining;
    }   // while ( cycles_remaining > 1 )

    if ( cycles_remaining == 1 ) {
        _ValueType value = *first;
        
        auto hole = first;
        size_t hole_index = 0;
        
        auto next = first;
        std::advance(next,forward);
        size_t next_index = first_offset + forward;
        
        while (next != first) {
            *hole = *next;
            hole = next;
            hole_index = next_index;
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                // try to use the iterator closest to the next point, so we do fewer advances
                if (forward > backward) {
                    if (next_index >= forward) {
                        next = middle;
                        std::advance(next,(next_index-forward));
                    } else if (next_index >= backward) {
                        next = new_middle;
                        std::advance(next,(next_index-backward));
                    } else if (next_index >= first_offset) {
                        next = first;
                        std::advance(next,(next_index-first_offset));
                    } else {
                        next = begin;
                        std::advance(next,next_index);
                    }
                } else {
                    if (next_index >= backward) {
                        next = new_middle;
                        std::advance(next,(next_index-backward));
                    } else if (next_index >= forward) {
                        next = middle;
                        std::advance(next,(next_index-forward));
                    }  else if (next_index >= first_offset) {
                        next = first;
                        std::advance(next,(next_index-first_offset));
                    } else {
                        next = begin;
                        std::advance(next,next_index);
                    }
                }
            }
        }
        
        *hole = value;
    }   // cycles == 1

}

/******************************************************************************/

// gcd cycle with temporary buffer
// algorithm by Mat Marcus and Chris Cox  ~2004
// adapted for bidirectional iterators
// This is not very practical, but was useful for experimentation
template<typename BidirectionalIterator>
void gcd_cycle_buffered_bidirectional(  BidirectionalIterator first,
                                        BidirectionalIterator middle,
                                        BidirectionalIterator last )
{
    typedef typename std::iterator_traits<BidirectionalIterator>::value_type _ValueType;
    
    if (first == middle || middle == last)
        return;
    
    auto forward = std::distance( first, middle );
    auto backward = std::distance( middle, last );
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }

    const size_t aux_storage_count = bi_rotate_storage_bytes / sizeof(_ValueType);
    
    if ( aux_storage_count <= 1 || (forward+backward) < rotate_small_cutoff) {
        // 3 reverses is best performer overall for small containers
        three_reverse_rotate( first, middle, last );
        return;
    }


    _ValueType values[ aux_storage_count ];

    auto new_middle = first;
    if (backward >= forward) {
        new_middle = middle;
        std::advance(new_middle,(backward-forward));
    } else
        std::advance(new_middle,backward);
    // new_middle_index = backward;


    // use the aux storage and copy for small rotates
    if (forward <= aux_storage_count) {
        std::copy(first, middle, values);             // copy first few into aux
        std::copy(middle, last, first);               // move bulk of array (down in array)
        std::copy_n(values, forward, new_middle );    // copy aux to end
        return;
    }
    
   if ( backward <= aux_storage_count) {
        std::copy(middle, last, values);                 // copy last few into aux
        if (forward >= backward)
            std::copy_backward(first, middle, last);     // move bulk of array (up in array, overlaps - must copy backwards)
        else
            std::copy(first, middle, new_middle);        // move bulk of array (up in array, not overlap)
        std::copy_n(values, backward, first);            // copy aux to beginning
        return;
    }
   
    size_t num_cycles = my_gcd(forward, backward);

    if (num_cycles < 40) {
        // 3 reverses is best performer overall for small containers
        three_reverse_rotate( first, middle, last);
        return;
    }

    size_t cycles_remaining = num_cycles;
    size_t chunk_size = cycles_remaining;
    
    if ( chunk_size > aux_storage_count )
        chunk_size = aux_storage_count;
    
    while ( cycles_remaining > 1 ) {
        
        std::copy_n(first, chunk_size, values);
        
        auto hole = first;
        size_t hole_index = 0;
        
        auto next = first;
        std::advance(next,forward);
        size_t next_index = forward;

        while (next != first) {
            std::copy_n(next, chunk_size, hole);
            hole = next;
            hole_index = next_index;
            // DEFERRED - use middle and new_middle positions to speed up iterator movement
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                std::advance(next,-backward);
            }
        }
        
        std::copy_n(values, chunk_size, hole);
        cycles_remaining -= chunk_size;
        std::advance(first,chunk_size);
        
        if ( chunk_size > cycles_remaining )
            chunk_size = cycles_remaining;
    }   //  while ( cycles_remaining > 1 )

    if ( cycles_remaining == 1 ) {
        _ValueType value = *first;
        
        auto hole = first;
        size_t hole_index = 0;
        
        auto next = first;
        std::advance(next,forward);
        size_t next_index = forward;
        
        while (next != first) {
            *hole = *next;
            hole = next;
            hole_index = next_index;
            if (backward > hole_index) {
                next_index += forward;
                std::advance(next,forward);
            } else {
                next_index -= backward;
                std::advance(next,-backward);
            }
        }
        
        *hole = value;
    }   //  if ( cycles_remaining == 1 )

}

/******************************************************************************/

// gcd cycle with temporary buffer
// algorithm by Mat Marcus and Chris Cox  ~2004
template<typename RandomAccessIterator>
void gcd_cycle_buffered_random(  RandomAccessIterator first,
                        RandomAccessIterator middle,
                        RandomAccessIterator last)
{
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type _ValueType;
    
    if (first == middle || middle == last)
        return;

    auto forward  = middle - first;
    auto backward = last - middle;
    
    if (forward == backward) {
        std::swap_ranges(first, middle, middle);
        return;
    }
    
    auto total = last - first;    // types are known, so this is automatically divided by the value size
    
    if (total < rotate_small_cutoff) {
        // 3 reverses is best performer overall for small containers
        three_reverse_rotate( first, middle, last);
        return;
    }
    
    RandomAccessIterator new_middle = last - forward;       // == first + backward;
    //assert( new_middle == first+backward );
    //assert( middle == first+forward );
    //assert( last == middle+backward );

    const size_t aux_storage_count = rotate_storage_bytes / sizeof(_ValueType);
    
    if (aux_storage_count <= 1) {
        // basic gcd used when we can't hold multiple items in aux storage
        
        auto end =  first + my_gcd(forward, backward);
        auto start = first;

        while (start < end) {
            auto value  = *start;
            auto hole = start;
            auto next = start + forward;
            
            while (next != start) {
                *hole = *next;
                hole = next;
                next += (new_middle > hole) ? forward : -backward;
            }
            
            *hole = value;
            ++start;
        }
        return;
    }
    
    _ValueType values[ aux_storage_count ];

    // use the aux storage and copy for small rotates
    if (forward <= aux_storage_count) {
        std::copy(first, middle, values);              // copy first few into aux
        std::copy(middle, last, first);                // move bulk of array (down in array)
        std::copy(values, values+forward, new_middle);        // copy aux to end
        return;
    }
    
    if ( backward <= aux_storage_count) {
        std::copy(middle, last, values);                // copy last few into aux
        if (forward >= backward)
            std::copy_backward(first, middle, last);    // move bulk of array (up in array, overlaps - must copy backwards)
        else
            std::copy(first, middle, new_middle);       // move bulk of array (up in array, not overlap)
        std::copy(values, values+backward, first);      // copy aux to beginning
        return;
    }

    
    auto num_cycles = my_gcd(forward, backward);

    if (num_cycles < 40) {
        // 3 reverses is best performer overall for small containers
        three_reverse_rotate( first, middle, last);
        return;
    }

    auto cycles_remaining = num_cycles;
    auto chunk_size = cycles_remaining;
    
    if ( chunk_size > aux_storage_count )
        chunk_size = aux_storage_count;
    
    while ( cycles_remaining > 1 ) {
        
        std::copy(first, first+chunk_size, values);
        
        auto hole = first;
        auto next = first + forward;

        while (next != first) {
            std::copy(next, next+chunk_size, hole);
            hole = next;
            next += (new_middle > hole) ? forward : -backward;
        }
        
        std::copy(values, values+chunk_size, hole);
        cycles_remaining -= chunk_size;
        first += chunk_size;
        
        if ( chunk_size > cycles_remaining )
            chunk_size = cycles_remaining;
        
    }

    if ( cycles_remaining == 1 ) {
        auto value = *first;
        auto hole = first;
        auto next = first + forward;
        
        while (next != first) {
            *hole = *next;
            hole = next;
            next += (new_middle > hole) ? forward : -backward;
        }
        
        *hole = value;
    }

}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

// test various rotate functions
template <class Iterator, typename Rotate>
void test_rotate(Iterator firstDest, Iterator lastDest, int count, Rotate func, const std::string label) {
    int i;
    int dist = 0;
    int newBegin = 0;
    Iterator middle = firstDest;

    start_timer();
    
    // spread out the increments when testing larger containers, so we don't get all small movement numbers
    int dist_increment = (count-1+iterations-1) / iterations;
    if (dist_increment < 1)
        dist_increment = 1;

    // test different rotation points by incrementing (modulo length) each iteration
    for( i = 0; i < iterations; ++i ) {
        if (dist >= count) {        // modulo, but avoid a slow divide
            dist -= count;
            middle = firstDest;
            std::advance( middle, dist );
        }
        
        func( firstDest, middle, lastDest );
        
        newBegin -= dist;
        if (newBegin < 0)        // modulo, but avoid a slow divide
            newBegin += count;
        if (newBegin >= count)
            newBegin -= count;
        
        dist += dist_increment;
        
        // avoid overrunning the end of the list
        if (dist < count) {
            std::advance( middle, dist_increment );
        }
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    
    // rotate back into original order to validate
    middle = firstDest;
    std::advance( middle, newBegin );
    
    std::rotate( firstDest, middle, lastDest );
    verify_sorted( firstDest, lastDest, label );
}

/******************************************************************************/

// an option for debugging and performance tuning
#define OUTPUT_CSV  0

// test rotate functions on different sized arrays
template <class Iterator, typename Rotator>
void test_rotate_sizes(Iterator firstDest, const int max_count, Rotator func, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;

#if OUTPUT_CSV
    printf("\n\n%ld byte buffer\n", rotate_storage_bytes );
    printf("description, seconds, \"ops per sec.\"\n");
    
//    for( i = 30, j = 0; i <= max_count; i+=(i>>1), ++j ) {
//    for( i = 1, j = 0; i <= 60; ++i, ++j ) {               // profile small buffer performance
    for( i = 1, j = 0; i <= 100; ++i, ++j ) {            // profile cutoffs between algorithms
#else
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");
    
    for( i = 4, j = 0; i <= max_count; i *= 2, ++j ) {
#endif
        int64_t temp1 = max_count / i;
        int64_t temp2 = saved_iterations * temp1;

#if OUTPUT_CSV
    // keep the counts smaller while profiling
        if (temp2 > 0x700000)
            temp2 = 0x700000;
#else
        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
#endif
        if (temp2 < 8)
            temp2 = 8;
        
        temp2 = (temp2+1) & ~1; // make sure iteration count is even
        
        iterations = temp2;
//iterations = 129;

        Iterator lastTemp = firstDest;
        std::advance(lastTemp, i);
        //lastTemp += ptrdiff_t(i);    // we can do this only because we defined our own iterators
        
        test_rotate(firstDest, lastTemp, i, func, label);
        
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


// Theory is great for debugging algorithms, but fails to account for the real world cost of operations
#define THEORY      false

// Practice shows how things behave in reality
#define PRACTICE    true


template <typename T>
void TestOneType()
{
    const int base_iterations = iterations;
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
// NOTE - this could call TestOneContainer like sort_sequence.cpp
// But that makes the results more difficult to read, or we have to go completely to test by size like sort did

    const size_t large_count = LARGE_SIZE / sizeof(T);
    const size_t item_count = std::max( SIZE, large_count );

    std::unique_ptr<T> storage(new T[ item_count ]);
    T *data = storage.get();
    
    
    fill_descending( data, data+item_count, item_count+init_value );
    // sort, to account for aliasing of values in some types
    std::sort( data, data+item_count );

    std::forward_list<T> fListStorage( SIZE );
    fill_descending( fListStorage.begin(), fListStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    fListStorage.sort();
    
    std::list<T> listStorage( SIZE );
    fill_descending( listStorage.begin(), listStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    listStorage.sort();
    
    std::vector<T> vecStorage( SIZE );
    fill_descending( vecStorage.begin(), vecStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    std::sort( vecStorage.begin(), vecStorage.end() );
    

    // test basics, in cache

#if THEORY
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, std::rotate<ForwardPointer<T> >, myTypeName + " std::rotate forward");
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, gries_mills_rotate<ForwardPointer<T> >, myTypeName + " gries_mills forward");
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, gries_mills_rotate_counted_wrapper<ForwardPointer<T> >, myTypeName + " gries_mills_counted forward");
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, gries_mills_rotate_iterative_wrapper<ForwardPointer<T> >, myTypeName + " gries_mills_iterative forward");
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, gcd_cycle_forward<ForwardPointer<T> >, myTypeName + " gcd_cycle forward");
    test_rotate( ForwardPointer<T>(data), ForwardPointer<T>(data+SIZE), SIZE, gcd_cycle_buffered_forward<ForwardPointer<T> >, myTypeName + " gcd_cycle_buffered forward");
    
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, std::rotate<BidirectionalPointer<T> >, myTypeName + " std::rotate bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, gries_mills_rotate<BidirectionalPointer<T> >, myTypeName + " gries_mills bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, gries_mills_rotate_counted_wrapper<BidirectionalPointer<T> >, myTypeName + " gries_mills_counted bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, gries_mills_rotate_iterative_wrapper<BidirectionalPointer<T> >, myTypeName + " gries_mills_iterative bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, three_reverse_rotate<BidirectionalPointer<T> >, myTypeName + " three_reverses bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, gcd_cycle_bidirectional<BidirectionalPointer<T> >, myTypeName + " gcd_cycle bidirectional");
    test_rotate( BidirectionalPointer<T>(data), BidirectionalPointer<T>(data+SIZE), SIZE, gcd_cycle_buffered_bidirectional<BidirectionalPointer<T> >, myTypeName + " gcd_cycle_buffered bidirectional");
    
    test_rotate( data, data+SIZE, SIZE, std::rotate<T*>, myTypeName + " std::rotate random_access");
    test_rotate( data, data+SIZE, SIZE, gries_mills_rotate<T*>, myTypeName + " gries_mills random_access");
    test_rotate( data, data+SIZE, SIZE, gries_mills_rotate_counted_wrapper<T*>, myTypeName + " gries_mills_counted random_access");
    test_rotate( data, data+SIZE, SIZE, gries_mills_rotate_iterative_wrapper<T*>, myTypeName + " gries_mills_iterative random_access");
    test_rotate( data, data+SIZE, SIZE, three_reverse_rotate<T*>, myTypeName + " three_reverses random_access");
    test_rotate( data, data+SIZE, SIZE, gcd_cycle_random<T*>, myTypeName + " gcd_cycle random_access");
    test_rotate( data, data+SIZE, SIZE, gcd_cycle_buffered_random<T*>, myTypeName + " gcd_cycle_buffered random_access");
#endif

#if PRACTICE
    test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, std::rotate< typename std::forward_list<T>::iterator >, myTypeName + " std::rotate std::forward_list");
    test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, gries_mills_rotate< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills std::forward_list");
    test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, gries_mills_rotate_counted_wrapper< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills_counted std::forward_list");
    test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, gries_mills_rotate_iterative_wrapper< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills_iterative std::forward_list");
    //test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, gcd_cycle_forward< typename std::forward_list<T>::iterator >, myTypeName + " gcd_cycle std::forward_list");  // WAY too slow
    //test_rotate( fListStorage.begin(), fListStorage.end(), SIZE, gcd_cycle_buffered_forward< typename std::forward_list<T>::iterator >, myTypeName + " gcd_cycle_buffered std::forward_list"); // too slow

    test_rotate( listStorage.begin(), listStorage.end(), SIZE, std::rotate< typename std::list<T>::iterator >, myTypeName + " std::rotate std::list");
    test_rotate( listStorage.begin(), listStorage.end(), SIZE, gries_mills_rotate< typename std::list<T>::iterator >, myTypeName + " gries_mills std::list");
    test_rotate( listStorage.begin(), listStorage.end(), SIZE, gries_mills_rotate_counted_wrapper< typename std::list<T>::iterator >, myTypeName + " gries_mills_counted std::list");
    test_rotate( listStorage.begin(), listStorage.end(), SIZE, gries_mills_rotate_iterative_wrapper< typename std::list<T>::iterator >, myTypeName + " gries_mills_iterative std::list");
    test_rotate( listStorage.begin(), listStorage.end(), SIZE, three_reverse_rotate< typename std::list<T>::iterator >, myTypeName + " three_reverses std::list");
    //test_rotate( listStorage.begin(), listStorage.end(), SIZE, gcd_cycle_bidirectional< typename std::list<T>::iterator >, myTypeName + " gcd_cycle std::list");  // WAY too slow
    //test_rotate( listStorage.begin(), listStorage.end(), SIZE, gcd_cycle_buffered_bidirectional< typename std::list<T>::iterator >, myTypeName + " gcd_cycle_buffered std::list");    // too slow

    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, std::rotate< typename std::vector<T>::iterator >, myTypeName + " std::rotate std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, gries_mills_rotate< typename std::vector<T>::iterator >, myTypeName + " gries_mills std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, gries_mills_rotate_counted_wrapper< typename std::vector<T>::iterator >, myTypeName + " gries_mills_counted std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, gries_mills_rotate_iterative_wrapper< typename std::vector<T>::iterator >, myTypeName + " gries_mills_iterative std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, three_reverse_rotate< typename std::vector<T>::iterator >, myTypeName + " three_reverses std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, gcd_cycle_random< typename std::vector<T>::iterator >, myTypeName + " gcd_cycle std::vector");
    test_rotate( vecStorage.begin(), vecStorage.end(), SIZE, gcd_cycle_buffered_random< typename std::vector<T>::iterator >, myTypeName + " gcd_cycle_buffered std::vector");
#endif

    std::string temp1( myTypeName + " rotate");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    iterations = base_iterations / (8*1024);
    
    // test different sizes, in and out of cache

#if THEORY
    test_rotate_sizes( ForwardPointer<T>(data), large_count, std::rotate<ForwardPointer<T> >, myTypeName + " std::rotate forward" );
    test_rotate_sizes( ForwardPointer<T>(data), large_count, gries_mills_rotate<ForwardPointer<T> >, myTypeName + " gries_mills forward" );
    test_rotate_sizes( ForwardPointer<T>(data), large_count, gries_mills_rotate_counted_wrapper<ForwardPointer<T> >, myTypeName + " gries_mills_counted forward");
    test_rotate_sizes( ForwardPointer<T>(data), large_count, gries_mills_rotate_iterative_wrapper<ForwardPointer<T> >, myTypeName + " gries_mills_iterative forward");
    test_rotate_sizes( ForwardPointer<T>(data), large_count, gcd_cycle_forward<ForwardPointer<T> >, myTypeName + " gcd_cycle forward" );
    test_rotate_sizes( ForwardPointer<T>(data), large_count, gcd_cycle_buffered_forward<ForwardPointer<T> >, myTypeName + " gcd_cycle_buffered forward" );
    
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, std::rotate<BidirectionalPointer<T> >, myTypeName + " std::rotate bidirectional" );
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, gries_mills_rotate<BidirectionalPointer<T> >, myTypeName + " gries_mills bidirectional" );
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, gries_mills_rotate_counted_wrapper<BidirectionalPointer<T> >, myTypeName + " gries_mills_counted bidirectional");
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, gries_mills_rotate_iterative_wrapper<BidirectionalPointer<T> >, myTypeName + " gries_mills_iterative bidirectional");
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, three_reverse_rotate<BidirectionalPointer<T> >, myTypeName + " three_reverses bidirectional" );
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, gcd_cycle_bidirectional<BidirectionalPointer<T> >, myTypeName + " gcd_cycle bidirectional" );
    test_rotate_sizes( BidirectionalPointer<T>(data), large_count, gcd_cycle_buffered_bidirectional<BidirectionalPointer<T> >, myTypeName + " gcd_cycle_buffered bidirectional" );
    
    test_rotate_sizes( data, large_count, std::rotate<T*>, myTypeName + " std::rotate random_access" );
    test_rotate_sizes( data, large_count, gries_mills_rotate< T*>, myTypeName + " gries_mills random_access" );
    test_rotate_sizes( data, large_count, gries_mills_rotate_counted_wrapper< T* >, myTypeName + " gries_mills_counted random_access");
    test_rotate_sizes( data, large_count, gries_mills_rotate_iterative_wrapper< T* >, myTypeName + " gries_mills_iterative random_access");
    test_rotate_sizes( data, large_count, three_reverse_rotate<T*>, myTypeName + " three_reverses random_access" );
    test_rotate_sizes( data, large_count, gcd_cycle_random<T*>, myTypeName + " gcd_cycle random_access" );
    test_rotate_sizes( data, large_count, gcd_cycle_buffered_random<T*>, myTypeName + " gcd_cycle_buffered random_access" );
#endif

#if PRACTICE
    fListStorage.resize( item_count );
    fill_descending( fListStorage.begin(), fListStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    fListStorage.sort();
    
    listStorage.resize( item_count );
    fill_descending( listStorage.begin(), listStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    listStorage.sort();
    
    vecStorage.resize( item_count );
    fill_descending( vecStorage.begin(), vecStorage.end(), item_count+init_value );
    // sort, to account for aliasing of values in some types
    std::sort( vecStorage.begin(), vecStorage.end() );
    
    
    test_rotate_sizes( fListStorage.begin(), large_count, std::rotate< typename std::forward_list<T>::iterator >, myTypeName + " std::rotate std::forward_list" );
    test_rotate_sizes( fListStorage.begin(), large_count, gries_mills_rotate< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills std::forward_list" );
    test_rotate_sizes( fListStorage.begin(), large_count, gries_mills_rotate_counted_wrapper< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills_counted std::forward_list" );
    test_rotate_sizes( fListStorage.begin(), large_count, gries_mills_rotate_iterative_wrapper< typename std::forward_list<T>::iterator >, myTypeName + " gries_mills_iterative std::forward_list" );
    //test_rotate_sizes( fListStorage.begin(), large_count, gcd_cycle_forward< typename std::forward_list<T>::iterator >, myTypeName + " gcd_cycle std::forward_list" );        // WAY too slow
    //test_rotate_sizes( fListStorage.begin(), large_count, gcd_cycle_buffered_forward< typename std::forward_list<T>::iterator >, myTypeName + " gcd_cycle_buffered std::forward_list" ); // too slow

    test_rotate_sizes( listStorage.begin(), large_count, std::rotate< typename std::list<T>::iterator >, myTypeName + " std::rotate std::list");
    test_rotate_sizes( listStorage.begin(), large_count, gries_mills_rotate< typename std::list<T>::iterator >, myTypeName + " gries_mills std::list");
    test_rotate_sizes( listStorage.begin(), large_count, gries_mills_rotate_counted_wrapper< typename std::list<T>::iterator >, myTypeName + " gries_mills_counted std::list");
    test_rotate_sizes( listStorage.begin(), large_count, gries_mills_rotate_iterative_wrapper< typename std::list<T>::iterator >, myTypeName + " gries_mills_iterative std::list");
    test_rotate_sizes( listStorage.begin(), large_count, three_reverse_rotate< typename std::list<T>::iterator >, myTypeName + " three_reverses std::list");
    //test_rotate_sizes( listStorage.begin(), large_count, gcd_cycle_bidirectional< typename std::list<T>::iterator >, myTypeName + " gcd_cycle std::list");  // WAY too slow
    //test_rotate_sizes( listStorage.begin(), large_count, gcd_cycle_buffered_bidirectional< typename std::list<T>::iterator >, myTypeName + " gcd_cycle_buffered std::list");  // too slow

    test_rotate_sizes( vecStorage.begin(), large_count, std::rotate< typename std::vector<T>::iterator >, myTypeName + " std::rotate std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, gries_mills_rotate< typename std::vector<T>::iterator >, myTypeName + " gries_mills std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, gries_mills_rotate_counted_wrapper< typename std::vector<T>::iterator >, myTypeName + " gries_mills_counted std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, gries_mills_rotate_iterative_wrapper< typename std::vector<T>::iterator >, myTypeName + " gries_mills_iterative std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, three_reverse_rotate< typename std::vector<T>::iterator >, myTypeName + " three_reverses std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, gcd_cycle_random< typename std::vector<T>::iterator >, myTypeName + " gcd_cycle std::vector");
    test_rotate_sizes( vecStorage.begin(), large_count, gcd_cycle_buffered_random< typename std::vector<T>::iterator >, myTypeName + " gcd_cycle_buffered std::vector");
#endif

    
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
    
    
    // So far, patterns are the same for all types
    TestOneType<double>();


#if THESE_WORK_BUT_TAKE_FOREVER_TO_RUN
    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    TestOneType<int64_t>();
    TestOneType<uint64_t>();
    TestOneType<float>();
    
    TestOneType<long double>();
#endif


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
