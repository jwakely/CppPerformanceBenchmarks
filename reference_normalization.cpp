/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine any change in performance when using different methods of expressing array references.
        This is sometimes called array access normalization or index normalization,
        and usually related to induction variable and loop normalization.


Assumptions:

    1) The compiler will normalize array/pointer references whenever possible.

    2) The compiler will normalize array/pointer reference increments whenever possible.

    3) The compiler will not penalize use of std::vector or std::array iterators compared to pointers.
        (because the iterators should always be simple pointers in release builds)
        See also stepanov_vector.cpp, stepanov_array.cpp

    4) The compiler will normalize multidimensional references whenever possible.
 

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
#include <deque>
#include <array>
#include <vector>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 16000000;


// 2000 items, or between 2KB and 16KB of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 2000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

// between 16 and 128MB of data, outside the L2 cache of most common CPUs
const int sizeX = 4000;
const int sizeY = 4000;

// between 16 and 128MB of data, outside the L2 cache of most common CPUs
const int sizeA = 254;
const int sizeB = 255;
const int sizeC = 257;

// between 16 and 128MB of data, outside the L2 cache of most common CPUs
const int sizeD = 63;
const int sizeE = 65;
const int sizeF = 63;
const int sizeG = 65;

/******************************************************************************/
/******************************************************************************/

template<typename Number>
inline void check_sum(Number result, const std::string &label) {
    if (result != Number(SIZE * init_value) )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template<typename Number>
inline void check_sum2D(Number result, const std::string &label) {
    if (result != Number(sizeX*sizeY * Number(init_value)) )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template<typename Number>
inline void check_sum3D(Number result, const std::string &label) {
    if (result != Number(sizeA*sizeB*sizeC * Number(init_value)) )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template<typename Number>
inline void check_sum4D(Number result, const std::string &label) {
    if (result != Number(sizeD*sizeE*sizeF*sizeG * Number(init_value)) )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename Iterator>
void verify_sorted(Iterator first, Iterator last, const std::string &label) {
    if (!::is_sorted(first,last))
        printf("sort test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename Number>
Number accumulate_array(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += first[i];
    return result;
}

/******************************************************************************/

// technically loop normalization here, reducing induction variables
template <typename Number>
Number accumulate_array2(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += first[k];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    first += 3;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += first[k - 3];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    first -= 80;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += first[k + 80];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += *first++;
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr2(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i ) {
        result += *first;
        first = (const Number *) (((int8_t *)first) + sizeof(Number));
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr3(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i ) {
        result += *first;
        first = (const Number *) (((uintptr_t)first) + sizeof(Number));
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr4(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i ) {
        result += *first;
        first = (const Number *) (((uint8_t *)first) + sizeof(Number));
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr5(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += *(first + i);
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr6(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += *(first + k);
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr7(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += *( (const Number *)( (uint8_t *)first + k*sizeof(Number) ) );
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr8(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0, k = 0; i < count; ++i, ++k )
        result += *( (const Number *)( (uintptr_t)first + k*sizeof(Number) ) );
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_ptr9(const Number *first, const Number *last, Number result) {
    ptrdiff_t count = last - first;
    first += 1;
    for ( ptrdiff_t i = 0; i < count; ++i) {
        result += *( first - 1 );
        first++;
    }
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator(Iter first, Iter last, Number result) {
    while (first != last)
        result += *first++;
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator2(Iter first, Iter last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += first[i];
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator3(Iter first, Iter last, Number result) {
    ptrdiff_t count = last - first;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += *(first + i);
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator4(Iter first, Iter last, Number result) {
    ptrdiff_t count = last - first;
    first += 4;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += *(first + i - 4);
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator5(Iter first, Iter last, Number result) {
    ptrdiff_t count = last - first;
    first -= 100;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += *(first + 100 + i);
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator6(Iter first, Iter last, Number result) {
    ptrdiff_t count = last - first;
    first -= 200;
    for ( ptrdiff_t i = 0; i < count; ++i )
        result += first[i + 200];
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator7(Iter first, Iter last, Number result) {
    while (first != last) {
        result += *first;
        first++;
    }
    return result;
}

/******************************************************************************/

template <typename Iter, typename Number>
Number accumulate_iterator8(Iter first, Iter last, Number result) {
    if (first == last)
        return result;
    do {
        result += *first;
        first++;
    } while (first != last);
    return result;
}

/******************************************************************************/
/******************************************************************************/

// contiguous 2D array, so we can treat it as a 1D array for summation
template <typename Number>
Number accumulate_array2D1(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    size_t count = firstDim * secondDim;
    for ( size_t x = 0; x < count; ++x )
        result += first[x];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D2(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    for ( size_t y = 0; y < firstDim; ++y )
        for ( size_t x = 0; x < secondDim; ++x )
            result += first[ (y * secondDim) + x ];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D3(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    for ( size_t y = 0; y < firstDim; ++y ) {
        const Number *thisRow = first + y * secondDim;
        for ( size_t x = 0; x < secondDim; ++x )
            result += thisRow[ x ];
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D4(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    const Number *thisRow = first;
    for ( size_t y = 0; y < firstDim; ++y ) {
        for ( size_t x = 0; x < secondDim; ++x )
            result += thisRow[ x ];
        thisRow += secondDim;
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D5(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    const Number *thisRow = first;
    for ( size_t y = 0; y < firstDim; ++y ) {
        for ( size_t x = 0; x < secondDim; ++x )
            result += *thisRow++;
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D6(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    const Number *thisRow = first;
    for ( size_t y = 0; y < firstDim; ++y ) {
        for ( size_t x = 0; x < secondDim; ++x ) {
            result += *thisRow;
            thisRow = (const Number *) (((uintptr_t)thisRow) + sizeof(Number));
        }
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D7(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    const Number *thisRow = first;
    for ( size_t y = 0; y < firstDim; ++y ) {
        for ( size_t x = 0; x < secondDim; ++x )
            result += *( (const Number *)( (uintptr_t)thisRow + x*sizeof(Number) ) );
        thisRow += secondDim;
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D8(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    for ( size_t y = 0, i = 0; y < firstDim; ++i, ++y ) {
        const Number *thisRow = first + i * secondDim;
        for ( size_t x = 0, j = 0; x < secondDim; ++j, ++x )
            result += thisRow[ j ];
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array2D9(const Number *first, size_t firstDim, size_t secondDim, Number result) {
    for ( size_t y = 0, i = 0; y < firstDim; ++i, ++y )
        for ( size_t x = 0, j = 0; x < secondDim; ++j, ++x )
            result += first[ (i * secondDim) + j ];
    return result;
}

/******************************************************************************/
/******************************************************************************/

// contiguous 3D array, so we can treat it as a 1D array for summation
template <typename Number>
Number accumulate_array3D1(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    size_t count = firstDim * secondDim * thirdDim;
    for ( size_t x = 0; x < count; ++x )
        result += first[x];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D2(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x )
                result += first[ (z*secondDim*thirdDim) + (y * thirdDim) + x ];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D3(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisPlane = first + z*(secondDim*thirdDim);
        for ( size_t y = 0; y < secondDim; ++y ) {
            const Number *thisRow = thisPlane + y * thirdDim;
            for ( size_t x = 0; x < thirdDim; ++x )
                result += thisRow[ x ];
        }
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D4(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    const Number *thisPlane = first;
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisRow = thisPlane;
        for ( size_t y = 0; y < secondDim; ++y ) {
            for ( size_t x = 0; x < thirdDim; ++x )
                result += thisRow[ x ];
            thisRow += thirdDim;
        }
        thisPlane += (secondDim*thirdDim);
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D5(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    const Number *thisRow = first;
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x )
                result += *thisRow++;
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D6(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    const Number *thisRow = first;
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x ) {
                result += *thisRow;
                thisRow = (const Number *) (((uintptr_t)thisRow) + sizeof(Number));
            }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D7(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    const Number *thisPlane = first;
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisRow = thisPlane;
        for ( size_t y = 0; y < secondDim; ++y ) {
            for ( size_t x = 0; x < thirdDim; ++x )
                result += *( (const Number *)( (uintptr_t)thisRow + x*sizeof(Number) ) );
            thisRow += thirdDim;
        }
        thisPlane += (secondDim*thirdDim);
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D8(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    for ( size_t z = 0, i = 0; z < firstDim; ++i, ++z ) {
        const Number *thisPlane = first + i*(secondDim*thirdDim);
        for ( size_t y = 0, j = 0; y < secondDim; ++j, ++y ) {
            const Number *thisRow = thisPlane + j * thirdDim;
            for ( size_t x = 0, k = 0; x < thirdDim; ++k, ++x )
                result += thisRow[ k ];
        }
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array3D9(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, Number result) {
    for ( size_t z = 0, i = 0; z < firstDim; ++i, ++z )
        for ( size_t y = 0, j = 0; y < secondDim; ++j, ++y )
            for ( size_t x = 0, k = 0; x < thirdDim; ++k, ++x )
                result += first[ (i*secondDim*thirdDim) + (j * thirdDim) + k ];
    return result;
}

/******************************************************************************/
/******************************************************************************/

// contiguous 4D array, so we can treat it as a 1D array for summation
template <typename Number>
Number accumulate_array4D1(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    size_t count = firstDim * secondDim * thirdDim * fourthDim;
    for ( size_t x = 0; x < count; ++x )
        result += first[x];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D2(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x )
                for ( size_t v = 0; v < fourthDim; ++v )
                    result += first[ (z*secondDim*thirdDim*fourthDim) + (y*thirdDim*fourthDim) + (x * fourthDim) + v ];
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D3(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisSpace = first + z*(secondDim*thirdDim*fourthDim);
        for ( size_t y = 0; y < secondDim; ++y ) {
            const Number *thisPlane = thisSpace + y*(thirdDim*fourthDim);
            for ( size_t x = 0; x < thirdDim; ++x ) {
                const Number *thisRow = thisPlane + x * fourthDim;
                for ( size_t v = 0; v < fourthDim; ++v )
                    result += thisRow[ v ];
            }
        }
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D4(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    const Number *thisSpace = first;
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisPlane = thisSpace;
        for ( size_t y = 0; y < secondDim; ++y ) {
            const Number *thisRow = thisPlane;
            for ( size_t x = 0; x < thirdDim; ++x ) {
                for ( size_t v = 0; v < fourthDim; ++v )
                    result += thisRow[ v ];
                thisRow += fourthDim;
            }
            thisPlane += (thirdDim*fourthDim);
        }
        thisSpace += (secondDim*thirdDim*fourthDim);
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D5(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    const Number *thisRow = first;
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x )
                for ( size_t v = 0; v < fourthDim; ++v )
                    result += *thisRow++;
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D6(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    const Number *thisRow = first;
    for ( size_t z = 0; z < firstDim; ++z )
        for ( size_t y = 0; y < secondDim; ++y )
            for ( size_t x = 0; x < thirdDim; ++x )
                for ( size_t v = 0; v < fourthDim; ++v ) {
                    result += *thisRow;
                    thisRow = (const Number *) (((uintptr_t)thisRow) + sizeof(Number));
                }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D7(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    const Number *thisSpace = first;
    for ( size_t z = 0; z < firstDim; ++z ) {
        const Number *thisPlane = thisSpace;
        for ( size_t y = 0; y < secondDim; ++y ) {
            const Number *thisRow = thisPlane;
            for ( size_t x = 0; x < thirdDim; ++x ) {
                for ( size_t v = 0; v < fourthDim; ++v )
                    result += *( (const Number *)( (uintptr_t)thisRow + v*sizeof(Number) ) );
                thisRow += fourthDim;
            }
            thisPlane += (thirdDim*fourthDim);
        }
        thisSpace += (secondDim*thirdDim*fourthDim);
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D8(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    for ( size_t z = 0, i = 0; z < firstDim; ++i, ++z ) {
        const Number *thisSpace = first + i*(secondDim*thirdDim*fourthDim);
        for ( size_t y = 0, j = 0; y < secondDim; ++j, ++y ) {
            const Number *thisPlane = thisSpace + j*(thirdDim*fourthDim);
            for ( size_t x = 0, k = 0; x < thirdDim; ++k, ++x ) {
                const Number *thisRow = thisPlane + k * fourthDim;
                for ( size_t v = 0, m = 0; v < fourthDim; ++m, ++v )
                    result += thisRow[ m ];
            }
        }
    }
    return result;
}

/******************************************************************************/

template <typename Number>
Number accumulate_array4D9(const Number *first, size_t firstDim, size_t secondDim, size_t thirdDim, size_t fourthDim, Number result) {
    for ( size_t z = 0, i = 0; z < firstDim; ++i, ++z )
        for ( size_t y = 0, j = 0; y < secondDim; ++j, ++y )
            for ( size_t x = 0, k = 0; x < thirdDim; ++k, ++x )
                for ( size_t v = 0, m = 0; v < fourthDim; ++m, ++v )
                    result += first[ (i*secondDim*thirdDim*fourthDim) + (j*thirdDim*fourthDim) + (k * fourthDim) + m ];
    return result;
}

/******************************************************************************/
/******************************************************************************/

std::deque<std::string> gLabels;

template <typename T, typename Iter, typename Accumulator>
void test_accumulate(const Iter first, const Iter last, const T zero, Accumulator _accum, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i)
        check_sum( _accum(first, last, zero), label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Iter, typename Accumulator>
void test_accumulate2D(const Iter first, const size_t firstDim, const size_t secondDim,
    const T zero, Accumulator _accum, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i)
        check_sum2D( _accum(first, firstDim, secondDim, zero), label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Iter, typename Accumulator>
void test_accumulate3D(const Iter first, const size_t firstDim, const size_t secondDim, const size_t thirdDim,
    const T zero, Accumulator _accum, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i)
        check_sum3D( _accum(first, firstDim, secondDim, thirdDim, zero), label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Iter, typename Accumulator>
void test_accumulate4D(const Iter first, const size_t firstDim, const size_t secondDim, const size_t thirdDim, const size_t fourthDim,
    const T zero, Accumulator _accum, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i)
        check_sum4D( _accum(first, firstDim, secondDim, thirdDim, fourthDim, zero), label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_insertion_sort(const Iterator firstSource, const Iterator lastSource, Iterator firstDest,
                        Iterator lastDest, T zero, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        insertionSort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_quicksort(const Iterator firstSource, const Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        quicksort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_heap_sort(const Iterator firstSource, const Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        heapsort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template<typename Number>
void testOneType()
{
    typedef std::array<Number,SIZE> dataArrayType;
    dataArrayType dataArray;
    
    typedef std::vector<Number> dataVectorType;
    dataVectorType dataVector;
    dataVector.resize(SIZE);
    
    Number data[SIZE];
    Number zero(0);
    
    gLabels.clear();
    
    std::string myTypeName( getTypeName<Number>() );


    ::fill(data, data+SIZE, Number(init_value));
    ::fill(dataArray.begin(), dataArray.end(), Number(init_value));
    ::fill(dataVector.begin(), dataVector.end(), Number(init_value));
    
    test_accumulate(data, data+SIZE, zero, accumulate_array<Number>, myTypeName + " accum array");
    test_accumulate(data, data+SIZE, zero, accumulate_array2<Number>, myTypeName + " accum array2");
    test_accumulate(data, data+SIZE, zero, accumulate_array3<Number>, myTypeName + " accum array3");
    test_accumulate(data, data+SIZE, zero, accumulate_array4<Number>, myTypeName + " accum array4");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr<Number>, myTypeName + " accum pointer");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr2<Number>, myTypeName + " accum pointer2");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr3<Number>, myTypeName + " accum pointer3");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr4<Number>, myTypeName + " accum pointer4");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr5<Number>, myTypeName + " accum pointer5");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr6<Number>, myTypeName + " accum pointer6");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr7<Number>, myTypeName + " accum pointer7");
    test_accumulate(data, data+SIZE, zero, accumulate_ptr8<Number>, myTypeName + " accum pointer8");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator<Number*,Number>, myTypeName + " accum iterator");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator2<Number*,Number>, myTypeName + " accum iterator2");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator2< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator2");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator2< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator2");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator3<Number*,Number>, myTypeName + " accum iterator3");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator3< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator3");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator3< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator3");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator4<Number*,Number>, myTypeName + " accum iterator4");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator4< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator4");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator4< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator4");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator5<Number*,Number>, myTypeName + " accum iterator5");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator5< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator5");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator5< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator5");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator6<Number*,Number>, myTypeName + " accum iterator6");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator6< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator6");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator6< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator6");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator7<Number*,Number>, myTypeName + " accum iterator7");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator7< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator7");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator7< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator7");
    test_accumulate(data, data+SIZE, zero, accumulate_iterator8<Number*,Number>, myTypeName + " accum iterator8");
    test_accumulate(dataArray.begin(), dataArray.end(), zero,
            accumulate_iterator8< typename dataArrayType::iterator, Number>, myTypeName + " accum std::array iterator8");
    test_accumulate(dataVector.begin(), dataVector.end(), zero,
            accumulate_iterator8< typename dataVectorType::iterator, Number>, myTypeName + " accum std::vector iterator8");

    std::string temp4( myTypeName + " reference_normalize accumulate" );
    summarize( temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );




    int base_iterations = iterations;
    iterations /= 28000;
    
    auto data2 = new Number[sizeY*sizeX];
    ::fill(data2, data2+sizeY*sizeX, Number(init_value));
    
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D1<Number>, myTypeName + " accum array2D1");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D2<Number>, myTypeName + " accum array2D2");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D3<Number>, myTypeName + " accum array2D3");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D4<Number>, myTypeName + " accum array2D4");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D5<Number>, myTypeName + " accum array2D5");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D6<Number>, myTypeName + " accum array2D6");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D7<Number>, myTypeName + " accum array2D7");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D8<Number>, myTypeName + " accum array2D8");
    test_accumulate2D(data2, sizeY, sizeX, zero, accumulate_array2D9<Number>, myTypeName + " accum array2D9");

    std::string temp5( myTypeName + " reference_normalize accumulate2D" );
    summarize( temp5.c_str(), sizeY*sizeX, iterations, kDontShowGMeans, kDontShowPenalty );
    
    delete[] data2;
    
    
    auto data3 = new Number[sizeA*sizeB*sizeC];
    ::fill(data3, data3+sizeA*sizeB*sizeC, Number(init_value));
    
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D1<Number>, myTypeName + " accum array3D1");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D2<Number>, myTypeName + " accum array3D2");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D3<Number>, myTypeName + " accum array3D3");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D4<Number>, myTypeName + " accum array3D4");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D5<Number>, myTypeName + " accum array3D5");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D6<Number>, myTypeName + " accum array3D6");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D7<Number>, myTypeName + " accum array3D7");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D8<Number>, myTypeName + " accum array3D8");
    test_accumulate3D(data3, sizeA, sizeB, sizeC, zero, accumulate_array3D9<Number>, myTypeName + " accum array3D9");

    std::string temp6( myTypeName + " reference_normalize accumulate3D" );
    summarize( temp6.c_str(), sizeA*sizeB*sizeC, iterations, kDontShowGMeans, kDontShowPenalty );
    
    delete[] data3;
    
    
    auto data4 = new Number[sizeD*sizeE*sizeF*sizeG];
    ::fill(data4, data4+sizeD*sizeE*sizeF*sizeG, Number(init_value));
    
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D1<Number>, myTypeName + " accum array4D1");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D2<Number>, myTypeName + " accum array4D2");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D3<Number>, myTypeName + " accum array4D3");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D4<Number>, myTypeName + " accum array4D4");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D5<Number>, myTypeName + " accum array4D5");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D6<Number>, myTypeName + " accum array4D6");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D7<Number>, myTypeName + " accum array4D7");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D8<Number>, myTypeName + " accum array4D8");
    test_accumulate4D(data4, sizeD, sizeE, sizeF, sizeG, zero, accumulate_array4D9<Number>, myTypeName + " accum array4D9");

    std::string temp7( myTypeName + " reference_normalize accumulate4D" );
    summarize( temp7.c_str(), sizeD*sizeE*sizeF*sizeG, iterations, kDontShowGMeans, kDontShowPenalty );
    
    delete[] data4;
    iterations = base_iterations;


    

    dataArrayType dataArray2;
    Number data22[SIZE];
    dataVectorType dataVector2;
    dataVector2.resize(SIZE);
    
    iterations /= 533;
    
    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 12345 );

    // fill one set of random numbers
    fill_random( data, data+SIZE );
    // copy to the other sets, so we have the same numbers
    ::copy( data, data+SIZE, dataArray.begin() );
    ::copy( data, data+SIZE, dataVector.begin() );
    
    test_insertion_sort( data, data+SIZE, data22, data22+SIZE, zero, myTypeName + " insertion_sort pointer");
    test_insertion_sort( dataArray.begin(), dataArray.end(), dataArray2.begin(), dataArray2.end(),
            zero, myTypeName + " insertion_sort std::array iterator");
    test_insertion_sort( dataVector.begin(), dataVector.end(), dataVector2.begin(), dataVector2.end(),
            zero, myTypeName + " insertion_sort std::vector iterator");


    // these are slightly faster - O(NLog2(N))
    test_quicksort( data, data+SIZE, data22, data22+SIZE, zero, myTypeName + " quick_sort pointer");
    test_quicksort( dataArray.begin(), dataArray.end(), dataArray2.begin(), dataArray2.end(),
            zero, myTypeName + " quick_sort std::array iterator");
    test_quicksort( dataVector.begin(), dataVector.end(), dataVector2.begin(), dataVector2.end(),
            zero, myTypeName + " quick_sort std::vector iterator");
    
    test_heap_sort( data, data+SIZE, data22, data22+SIZE, zero, myTypeName + " heap_sort pointer");
    test_heap_sort( dataArray.begin(), dataArray.end(), dataArray2.begin(), dataArray2.end(),
            zero, myTypeName + " heap_sort std::array iterator");
    test_heap_sort( dataVector.begin(), dataVector.end(), dataVector2.begin(), dataVector2.end(),
            zero, myTypeName + " heap_sort std::vector iterator");

    std::string temp3( myTypeName + " reference_normalize sort" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
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
    if (argc > 2) init_value = (double) atof(argv[2]);


    // Most types give the same results - so we only need to test a subset
 
    //testOneType<uint8_t>();
    //testOneType<int8_t>();
    testOneType<uint16_t>();
    testOneType<int16_t>();
    //testOneType<uint32_t>();
    //testOneType<int32_t>();
    //testOneType<uint64_t>();
    //testOneType<int64_t>();
    //testOneType<float>();       // not enough precision for large arrays
    testOneType<double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
