/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018-2021 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test the performance of various idioms for finding maximum and maximum of a sequence.


Assumptions:
    1) The compiler will optimize minimum and maximum finding operations.

    2) The compiler may recognize ineffecient minimum or maximum idioms and substitute efficient methods.

    3) The compiler will unroll and vectorize as needed to get best performance.
 

Conclusions:
    1) STL really needs a find_minimum_value function instead of suggesting min_element
        ditto for maximum value

    2) STL minmax_element is really inefficient - prematurely optimized for serial scalar, not vector instructions



NOTE - several variations of unrolling have been added to show multiple issues with autovectorization in compilers

NOTE - min or max between two sequences is tested in minmax.cpp.

NOTE - pin values in sequence, see minmax.cpp

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 4000000;


// 8000 items, or between 8 and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/

long double gMinResult = 0;
long double gMaxResult = 0;

size_t gMinPosition = 0;
size_t gMaxPosition = 0;
size_t gMaxLastPosition = 0;

std::deque<std::string> gLabels;

/******************************************************************************/
/******************************************************************************/

template< typename T >
inline void check_min_result(T result, const std::string &label) {
    if (result != T(gMinResult))
        printf("test %s failed (got %g instead of %g)\n", label.c_str(), (double)result, (double)gMinResult );
}

/******************************************************************************/

template< typename T >
inline void check_max_result(T result, const std::string &label) {
    if (result != T(gMaxResult))
        printf("test %s failed (got %g instead of %g)\n", label.c_str(), (double)result, (double)gMaxResult );
}

/******************************************************************************/

inline void check_min_position(ptrdiff_t result, const std::string &label) {
    if (result != ptrdiff_t(gMinPosition))
        printf("test %s failed (got %tu instead of %zu)\n", label.c_str(), result, gMinPosition );
}

/******************************************************************************/

inline void check_max_position(ptrdiff_t result, const std::string &label) {
    if (result != ptrdiff_t(gMaxPosition))
        printf("test %s failed (got %tu instead of %zu)\n", label.c_str(), result, gMaxPosition );
}

/******************************************************************************/

inline void check_max_last_position(ptrdiff_t result, const std::string &label) {
    if (result != ptrdiff_t(gMaxLastPosition))
        printf("test %s failed (got %tu instead of %zu)\n", label.c_str(), result, gMaxLastPosition );
}

/******************************************************************************/
/******************************************************************************/

template< typename Iter, typename T >
T find_minimum( Iter first, Iter last ) {
    auto min_value = *first;
    if (first != last)
        ++first;
    while (first != last) {
        if (*first < min_value)
            min_value = *first;
        ++first;
    }
    return  min_value;
}

/******************************************************************************/

template< typename Iter, typename T >
T find_maximum( Iter first, Iter last ) {
    auto max_value = *first;
    if (first != last)
        ++first;
    while (first != last) {
        if (*first > max_value)
            max_value = *first;
        ++first;
    }
    return  max_value;
}

/******************************************************************************/

template< typename Iter, typename T >
std::pair<T,T> find_minmax( Iter first, Iter last ) {
    auto min_value = *first;
    auto max_value = *first;
    if (first != last)
        ++first;
    while (first != last) {
        if (*first < min_value)
            min_value = *first;
        if (*first > max_value)
            max_value = *first;
        ++first;
    }
    std::pair<T,T> result( min_value, max_value );
    return result;
}

/******************************************************************************/

template< typename Iter >
size_t find_minimum_position( Iter first, size_t count) {
    auto min_value = first[0];
    size_t minpos = 0;
    for (size_t k = 1; k < count; ++k) {
        if (first[k] < min_value) {
            min_value = first[k];
            minpos = k;
        }
    }
    return minpos;
}

/******************************************************************************/

template< typename Iter >
size_t find_maximum_position( Iter first, size_t count) {
    auto max_value = first[0];
    size_t maxpos = 0;
    for (size_t k = 1; k < count; ++k) {
        if (first[k] > max_value) {
            max_value = first[k];
            maxpos = k;
        }
    }
    return maxpos;
}

/******************************************************************************/

template< typename Iter >
size_t find_maximum_last_position( Iter first, size_t count) {
    auto max_value = first[0];
    size_t maxpos = 0;
    for (size_t k = 1; k < count; ++k) {
        if (first[k] >= max_value) {
            max_value = first[k];
            maxpos = k;
        }
    }
    return maxpos;
}

/******************************************************************************/

template< typename Iter, typename T >
std::pair<size_t,size_t> find_minmax_position( Iter first, size_t count) {
    auto min_value = *first;
    auto max_value = *first;
    size_t minpos = 0;
    size_t maxpos = 0;
    for (size_t k = 1; k < count; ++k) {
        if (first[k] > max_value) {
            max_value = first[k];
            maxpos = k;
        }
        if (first[k] < min_value) {
            min_value = first[k];
            minpos = k;
        }
    }
    std::pair<size_t,size_t> result( minpos, maxpos );
    return result;
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct min_std_functor {
    inline T operator()(T a, T b) {
        return std::min(a, b);
    }
};

/******************************************************************************/

template <typename T>
struct min_functor1 {
    inline T operator()(T a, T b) {
        if (a < b)
            return a;
        else
            return b;
    }
};

/******************************************************************************/

template <typename T>
struct min_functor2 {
    inline T operator()(T a, T b) {
        return ( a < b ) ? a : b;
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct min_functor3 {
    inline T operator()(T a, T b) {
        return b + ((a - b) & -T(a < b));
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct min_functor4 {
    inline T operator()(T a, T b)
        {
        return b ^ ((a ^ b) & -T(a < b));
        }
};

/******************************************************************************/

// reverse of 1
template <typename T>
struct min_functor5 {
    inline T operator()(T a, T b)
        {
        if (a > b)
            return b;
        else
            return a;
        }
};

/******************************************************************************/

// reverse of 2
template <typename T>
struct min_functor6 {
    inline T operator()(T a, T b)
        {
        return ( a > b ) ? b : a;
        }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct max_std_functor {
    inline T operator()(T a, T b) {
        return std::max(a, b);
    }
};

/******************************************************************************/

template <typename T>
struct max_functor1 {
    inline T operator()(T a, T b) {
        if (a > b)
            return a;
        else
            return b;
    }
};

/******************************************************************************/

template <typename T>
struct max_functor2 {
    inline T operator()(T a, T b) {
        return ( a > b ) ? a : b;
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct max_functor3 {
    inline T operator()(T a, T b) {
        return a - ((a - b) & -T(a < b));
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct max_functor4 {
    inline T operator()(T a, T b)
        {
        return a ^ ((a ^ b) & -T(a < b));
        }
};

/******************************************************************************/

// reverse of 1
template <typename T>
struct max_functor5 {
    inline T operator()(T a, T b)
        {
        if (a < b)
            return b;
        else
            return a;
        }
};

/******************************************************************************/

// reverse pf 2
template <typename T>
struct max_functor6 {
    inline T operator()(T a, T b)
        {
        return ( a < b ) ? b : a;
        }
};

/******************************************************************************/
/******************************************************************************/

template < typename T >
void test_min_element(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T *min_value = std::min_element( first, first+count );
        check_min_result( *min_value, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_element(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T *max_value = std::max_element( first, first+count );
        check_max_result( *max_value, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_element(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T *min_value = std::min_element( first, first+count );
        T *max_value = std::max_element( first, first+count );
        check_min_result( *min_value, label );
        check_max_result( *max_value, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_element2(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto result = std::minmax_element( first, first+count );
        check_min_result( *(result.first), label );
        check_max_result( *(result.second), label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_value1(const T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T min_value = find_minimum<const T*, T>( first, first+count );
        check_min_result(min_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_value1(const T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T max_value = find_maximum<const T*, T>( first, first+count );
        check_max_result(max_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_value1(const T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto minmax = find_minmax<const T*, T>( first, first+count );
        check_min_result(minmax.first, label);
        check_max_result(minmax.second, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_value2(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T min_value = first[0];
        for (size_t k = 1; k < count; ++k) {
            if (first[k] < min_value)
                min_value = first[k];
        }
        check_min_result(min_value, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_value2(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T max_value = first[0];
        for (size_t k = 1; k < count; ++k) {
            if (first[k] > max_value)
                max_value = first[k];
        }
        check_max_result(max_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_value2(const T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T max_value = first[0];
        T min_value = first[0];
        for (size_t k = 1; k < count; ++k) {
            if (first[k] > max_value)
                max_value = first[k];
            if (first[k] < min_value)
                min_value = first[k];
        }
        check_min_result(min_value, label);
        check_max_result(max_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4 );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T min_value = first[0];
        for (size_t k = 1; k < count; ++k) {
            min_value = findMin( first[k], min_value );
        }
        
        if (isMax)
            check_max_result( min_value, label);
        else
            check_min_result( min_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll4(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4 );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        for (k = 1; k < (count-3); k += 4) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );

        min_value0 = findMin( min_value0, min_value1 );
        
        if (isMax)
            check_max_result( min_value0, label);
        else
            check_min_result( min_value0, label);

    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll4a(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4 );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T min_values[4];
        
        min_values[0] = first[0];
        min_values[1] = first[0];
        min_values[2] = first[0];
        min_values[3] = first[0];
        
        for (k = 1; k < (count-3); k += 4) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[1] = findMin( min_values[1], min_values[3] );
        min_values[0] = findMin( min_values[0], min_values[2] );

        min_values[0] = findMin( min_values[0], min_values[1] );
        
        if (isMax)
            check_max_result( min_values[0], label);
        else
            check_min_result( min_values[0], label);

    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll8(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4);
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        for (k = 1; k < (count-7); k += 8) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            T val4 = first[k+4];
            T val5 = first[k+5];
            T val6 = first[k+6];
            T val7 = first[k+7];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
            min_value0 = findMin( val4, min_value0 );
            min_value1 = findMin( val5, min_value1 );
            min_value2 = findMin( val6, min_value2 );
            min_value3 = findMin( val7, min_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );

        min_value0 = findMin( min_value0, min_value1 );

        if (isMax)
            check_max_result(min_value0, label);
        else
            check_min_result(min_value0, label);
        
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll8a(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4);
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };

        for (k = 1; k < (count-7); k += 8) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
            min_values[4] = findMin( first[k+4], min_values[4] );
            min_values[5] = findMin( first[k+5], min_values[5] );
            min_values[6] = findMin( first[k+6], min_values[6] );
            min_values[7] = findMin( first[k+7], min_values[7] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[0] = findMin( min_values[0], min_values[4] );
        min_values[1] = findMin( min_values[1], min_values[5] );
        min_values[2] = findMin( min_values[2], min_values[6] );
        min_values[3] = findMin( min_values[3], min_values[7] );
        
        min_values[0] = findMin( min_values[0], min_values[2] );
        min_values[1] = findMin( min_values[1], min_values[3] );

        min_values[0] = findMin( min_values[0], min_values[1] );

        if (isMax)
            check_max_result(min_values[0], label);
        else
            check_min_result(min_values[0], label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll16(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4);
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        for (k = 1; k < (count-15); k += 16) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            T val4 = first[k+4];
            T val5 = first[k+5];
            T val6 = first[k+6];
            T val7 = first[k+7];
            T val8 = first[k+8];
            T val9 = first[k+9];
            T valA = first[k+10];
            T valB = first[k+11];
            T valC = first[k+12];
            T valD = first[k+13];
            T valE = first[k+14];
            T valF = first[k+15];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
            min_value0 = findMin( val4, min_value0 );
            min_value1 = findMin( val5, min_value1 );
            min_value2 = findMin( val6, min_value2 );
            min_value3 = findMin( val7, min_value3 );
            min_value0 = findMin( val8, min_value0 );
            min_value1 = findMin( val9, min_value1 );
            min_value2 = findMin( valA, min_value2 );
            min_value3 = findMin( valB, min_value3 );
            min_value0 = findMin( valC, min_value0 );
            min_value1 = findMin( valD, min_value1 );
            min_value2 = findMin( valE, min_value2 );
            min_value3 = findMin( valF, min_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );

        min_value0 = findMin( min_value0, min_value1 );
        
        if (isMax)
            check_max_result( min_value0, label);
        else
            check_min_result( min_value0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor >
void test_minmax_value_unroll16a(T* first, size_t count, MinFunctor findMin, const std::string label)
{
    const bool isMax( findMin( 4, 2 ) == 4);
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T min_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };
        
        for (k = 1; k < (count-15); k += 16) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
            min_values[4] = findMin( first[k+4], min_values[4] );
            min_values[5] = findMin( first[k+5], min_values[5] );
            min_values[6] = findMin( first[k+6], min_values[6] );
            min_values[7] = findMin( first[k+7], min_values[7] );
            
            min_values[0] = findMin( first[k+8], min_values[0] );
            min_values[1] = findMin( first[k+9], min_values[1] );
            min_values[2] = findMin( first[k+10], min_values[2] );
            min_values[3] = findMin( first[k+11], min_values[3] );
            min_values[4] = findMin( first[k+12], min_values[4] );
            min_values[5] = findMin( first[k+13], min_values[5] );
            min_values[6] = findMin( first[k+14], min_values[6] );
            min_values[7] = findMin( first[k+15], min_values[7] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[0] = findMin( min_values[0], min_values[4] );
        min_values[1] = findMin( min_values[1], min_values[5] );
        min_values[2] = findMin( min_values[2], min_values[6] );
        min_values[3] = findMin( min_values[3], min_values[7] );
        
        min_values[1] = findMin( min_values[1], min_values[3] );
        min_values[0] = findMin( min_values[0], min_values[2] );

        min_values[0] = findMin( min_values[0], min_values[1] );

        if (isMax)
            check_max_result(min_values[0], label);
        else
            check_min_result(min_values[0], label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T min_value = first[0];
        T max_value = first[0];
        
        for (size_t k = 1; k < count; ++k) {
            min_value = findMin( first[k], min_value );
            max_value = findMax( first[k], max_value );
        }
        
        check_min_result( min_value, label);
        check_max_result( max_value, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll4(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        
        T max_value0 = first[0];
        T max_value1 = max_value0;
        T max_value2 = max_value0;
        T max_value3 = max_value0;
        
        for (k = 1; k < (count-3); k += 4) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
            
            max_value0 = findMax( val0, max_value0 );
            max_value1 = findMax( val1, max_value1 );
            max_value2 = findMax( val2, max_value2 );
            max_value3 = findMax( val3, max_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
            max_value0 = findMax( first[k], max_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );
        
        max_value1 = findMax( max_value1, max_value3 );
        max_value0 = findMax( max_value0, max_value2 );

        min_value0 = findMin( min_value0, min_value1 );
    
        max_value0 = findMax( max_value0, max_value1 );
        
        check_min_result( min_value0, label);
        check_max_result( max_value0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll4a(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T min_values[4] = { first[0], first[0], first[0], first[0] };
        T max_values[4] = { first[0], first[0], first[0], first[0] };
        
        for (k = 1; k < (count-3); k += 4) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
            
            max_values[0] = findMax( first[k+0], max_values[0] );
            max_values[1] = findMax( first[k+1], max_values[1] );
            max_values[2] = findMax( first[k+2], max_values[2] );
            max_values[3] = findMax( first[k+3], max_values[3] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
            max_values[0] = findMax( first[k], max_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[1] = findMin( min_values[1], min_values[3] );
        min_values[0] = findMin( min_values[0], min_values[2] );

        min_values[0] = findMin( min_values[0], min_values[1] );
        
        max_values[1] = findMax( max_values[1], max_values[3] );
        max_values[0] = findMax( max_values[0], max_values[2] );

        max_values[0] = findMax( max_values[0], max_values[1] );
        
        check_min_result( min_values[0], label);
        check_max_result( max_values[0], label);

    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll8(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        
        T max_value0 = first[0];
        T max_value1 = max_value0;
        T max_value2 = max_value0;
        T max_value3 = max_value0;
        
        for (k = 1; k < (count-7); k += 8) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            T val4 = first[k+4];
            T val5 = first[k+5];
            T val6 = first[k+6];
            T val7 = first[k+7];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
            min_value0 = findMin( val4, min_value0 );
            min_value1 = findMin( val5, min_value1 );
            min_value2 = findMin( val6, min_value2 );
            min_value3 = findMin( val7, min_value3 );
            
            max_value0 = findMax( val0, max_value0 );
            max_value1 = findMax( val1, max_value1 );
            max_value2 = findMax( val2, max_value2 );
            max_value3 = findMax( val3, max_value3 );
            max_value0 = findMax( val4, max_value0 );
            max_value1 = findMax( val5, max_value1 );
            max_value2 = findMax( val6, max_value2 );
            max_value3 = findMax( val7, max_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
            max_value0 = findMax( first[k], max_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );
        
        max_value1 = findMax( max_value1, max_value3 );
        max_value0 = findMax( max_value0, max_value2 );

        min_value0 = findMin( min_value0, min_value1 );
    
        max_value0 = findMax( max_value0, max_value1 );
        
        check_min_result( min_value0, label);
        check_max_result( max_value0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll8a(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };
        T max_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };

        for (k = 1; k < (count-7); k += 8) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
            min_values[4] = findMin( first[k+4], min_values[4] );
            min_values[5] = findMin( first[k+5], min_values[5] );
            min_values[6] = findMin( first[k+6], min_values[6] );
            min_values[7] = findMin( first[k+7], min_values[7] );

            max_values[0] = findMax( first[k+0], max_values[0] );
            max_values[1] = findMax( first[k+1], max_values[1] );
            max_values[2] = findMax( first[k+2], max_values[2] );
            max_values[3] = findMax( first[k+3], max_values[3] );
            max_values[4] = findMax( first[k+4], max_values[4] );
            max_values[5] = findMax( first[k+5], max_values[5] );
            max_values[6] = findMax( first[k+6], max_values[6] );
            max_values[7] = findMax( first[k+7], max_values[7] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
            max_values[0] = findMax( first[k], max_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[0] = findMin( min_values[0], min_values[4] );
        min_values[1] = findMin( min_values[1], min_values[5] );
        min_values[2] = findMin( min_values[2], min_values[6] );
        min_values[3] = findMin( min_values[3], min_values[7] );
        
        max_values[0] = findMax( max_values[0], max_values[4] );
        max_values[1] = findMax( max_values[1], max_values[5] );
        max_values[2] = findMax( max_values[2], max_values[6] );
        max_values[3] = findMax( max_values[3], max_values[7] );
        
        min_values[1] = findMin( min_values[1], min_values[3] );
        min_values[0] = findMin( min_values[0], min_values[2] );

        min_values[0] = findMin( min_values[0], min_values[1] );
        
        max_values[1] = findMax( max_values[1], max_values[3] );
        max_values[0] = findMax( max_values[0], max_values[2] );

        max_values[0] = findMax( max_values[0], max_values[1] );
        
        check_min_result( min_values[0], label);
        check_max_result( max_values[0], label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll16(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;

        T max_value0 = first[0];
        T max_value1 = max_value0;
        T max_value2 = max_value0;
        T max_value3 = max_value0;

        for (k = 1; k < (count-15); k += 16) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            T val4 = first[k+4];
            T val5 = first[k+5];
            T val6 = first[k+6];
            T val7 = first[k+7];
            T val8 = first[k+8];
            T val9 = first[k+9];
            T valA = first[k+10];
            T valB = first[k+11];
            T valC = first[k+12];
            T valD = first[k+13];
            T valE = first[k+14];
            T valF = first[k+15];
            min_value0 = findMin( val0, min_value0 );
            min_value1 = findMin( val1, min_value1 );
            min_value2 = findMin( val2, min_value2 );
            min_value3 = findMin( val3, min_value3 );
            min_value0 = findMin( val4, min_value0 );
            min_value1 = findMin( val5, min_value1 );
            min_value2 = findMin( val6, min_value2 );
            min_value3 = findMin( val7, min_value3 );
            min_value0 = findMin( val8, min_value0 );
            min_value1 = findMin( val9, min_value1 );
            min_value2 = findMin( valA, min_value2 );
            min_value3 = findMin( valB, min_value3 );
            min_value0 = findMin( valC, min_value0 );
            min_value1 = findMin( valD, min_value1 );
            min_value2 = findMin( valE, min_value2 );
            min_value3 = findMin( valF, min_value3 );

            max_value0 = findMax( val0, max_value0 );
            max_value1 = findMax( val1, max_value1 );
            max_value2 = findMax( val2, max_value2 );
            max_value3 = findMax( val3, max_value3 );
            max_value0 = findMax( val4, max_value0 );
            max_value1 = findMax( val5, max_value1 );
            max_value2 = findMax( val6, max_value2 );
            max_value3 = findMax( val7, max_value3 );
            max_value0 = findMax( val8, max_value0 );
            max_value1 = findMax( val9, max_value1 );
            max_value2 = findMax( valA, max_value2 );
            max_value3 = findMax( valB, max_value3 );
            max_value0 = findMax( valC, max_value0 );
            max_value1 = findMax( valD, max_value1 );
            max_value2 = findMax( valE, max_value2 );
            max_value3 = findMax( valF, max_value3 );
        }
        for (; k < count; ++k) {
            min_value0 = findMin( first[k], min_value0 );
            max_value0 = findMax( first[k], max_value0 );
        }

        // collapse individual minimums to find the global minimum
        min_value1 = findMin( min_value1, min_value3 );
        min_value0 = findMin( min_value0, min_value2 );

        max_value1 = findMax( max_value1, max_value3 );
        max_value0 = findMax( max_value0, max_value2 );

        min_value0 = findMin( min_value0, min_value1 );

        max_value0 = findMax( max_value0, max_value1 );

        check_min_result( min_value0, label);
        check_max_result( max_value0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename MinFunctor, typename MaxFunctor>
void test_minmax_value2_unroll16a(T* first, size_t count, MinFunctor findMin, MaxFunctor findMax, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };
        T max_values[8] = { first[0], first[0], first[0], first[0], first[0], first[0], first[0], first[0] };
        
        for (k = 1; k < (count-15); k += 16) {
            min_values[0] = findMin( first[k+0], min_values[0] );
            min_values[1] = findMin( first[k+1], min_values[1] );
            min_values[2] = findMin( first[k+2], min_values[2] );
            min_values[3] = findMin( first[k+3], min_values[3] );
            min_values[4] = findMin( first[k+4], min_values[4] );
            min_values[5] = findMin( first[k+5], min_values[5] );
            min_values[6] = findMin( first[k+6], min_values[6] );
            min_values[7] = findMin( first[k+7], min_values[7] );

            max_values[0] = findMax( first[k+0], max_values[0] );
            max_values[1] = findMax( first[k+1], max_values[1] );
            max_values[2] = findMax( first[k+2], max_values[2] );
            max_values[3] = findMax( first[k+3], max_values[3] );
            max_values[4] = findMax( first[k+4], max_values[4] );
            max_values[5] = findMax( first[k+5], max_values[5] );
            max_values[6] = findMax( first[k+6], max_values[6] );
            max_values[7] = findMax( first[k+7], max_values[7] );
            
            min_values[0] = findMin( first[k+8], min_values[0] );
            min_values[1] = findMin( first[k+9], min_values[1] );
            min_values[2] = findMin( first[k+10], min_values[2] );
            min_values[3] = findMin( first[k+11], min_values[3] );
            min_values[4] = findMin( first[k+12], min_values[4] );
            min_values[5] = findMin( first[k+13], min_values[5] );
            min_values[6] = findMin( first[k+14], min_values[6] );
            min_values[7] = findMin( first[k+15], min_values[7] );

            max_values[0] = findMax( first[k+8], max_values[0] );
            max_values[1] = findMax( first[k+9], max_values[1] );
            max_values[2] = findMax( first[k+10], max_values[2] );
            max_values[3] = findMax( first[k+11], max_values[3] );
            max_values[4] = findMax( first[k+12], max_values[4] );
            max_values[5] = findMax( first[k+13], max_values[5] );
            max_values[6] = findMax( first[k+14], max_values[6] );
            max_values[7] = findMax( first[k+15], max_values[7] );
        }
        for (; k < count; ++k) {
            min_values[0] = findMin( first[k], min_values[0] );
            max_values[0] = findMax( first[k], max_values[0] );
        }

        // collapse individual minimums to find the global minimum
        min_values[0] = findMin( min_values[0], min_values[4] );
        min_values[1] = findMin( min_values[1], min_values[5] );
        min_values[2] = findMin( min_values[2], min_values[6] );
        min_values[3] = findMin( min_values[3], min_values[7] );

        max_values[0] = findMax( max_values[0], max_values[4] );
        max_values[1] = findMax( max_values[1], max_values[5] );
        max_values[2] = findMax( max_values[2], max_values[6] );
        max_values[3] = findMax( max_values[3], max_values[7] );

        min_values[1] = findMin( min_values[1], min_values[3] );
        min_values[0] = findMin( min_values[0], min_values[2] );

        min_values[0] = findMin( min_values[0], min_values[1] );

        max_values[1] = findMax( max_values[1], max_values[3] );
        max_values[0] = findMax( max_values[0], max_values[2] );

        max_values[0] = findMax( max_values[0], max_values[1] );

        check_min_result( min_values[0], label);
        check_max_result( max_values[0], label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template < typename T >
void test_min_element_pos(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T *min_value = std::min_element( first, first+count );
        check_min_position( (min_value-first), label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_element_pos(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T *max_value = std::max_element( first, first+count );
        check_max_position( (max_value-first), label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// std::minmax_element differs from min_element and max_element,
// returning the LAST instance of the max value, and FIRST instance of the min value.
// This might be useful somewhere, but is annoying here.
template < typename T >
void test_minmax_element_pos(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto result = std::minmax_element( first, first+count );
        check_min_position( ((result.first)-first), label);
        check_max_last_position( ((result.second)-first), label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_element_pos2(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto minpos = std::min_element( first, first+count );
        auto maxpos = std::max_element( first, first+count );
        check_min_position( (minpos-first), label);
        check_max_position( (maxpos-first), label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_position1(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t minpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] < first[minpos]) {
                minpos = k;
            }
        }
        check_min_position(minpos, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_position2(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto min_value = first[0];
        size_t minpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] < min_value) {
                min_value = first[k];
                minpos = k;
            }
        }
        check_min_position(minpos, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_position3(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto right = first + count;
        auto left = first;
        auto minpos = left;
        if (left != right)
            ++left;
        while (left != right) {
            if (*left < *minpos)
                minpos = left;
            ++left;
        }
        check_min_position( (minpos-first), label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_position1(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t maxpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] > first[maxpos]) {
                maxpos = k;
            }
        }
        check_max_position(maxpos, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}
/******************************************************************************/

template < typename T >
void test_max_position2(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T max_value = first[0];
        size_t maxpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] > max_value) {
                max_value = first[k];
                maxpos = k;
            }
        }
        check_max_position(maxpos, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_position3(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto right = first + count;
        auto left = first;
        auto maxpos = left;
        if (left != right)
            ++left;
        while (left != right) {
            if (*left > *maxpos)
                maxpos = left;
            ++left;
        }
        check_max_position( (maxpos-first), label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_position1(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t minpos = 0;
        size_t maxpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] < first[minpos]) {
                minpos = k;
            }
            if (first[k] > first[maxpos]) {
                maxpos = k;
            }
        }
        check_min_position(minpos, label);
        check_max_position(maxpos, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_position2(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto min_value = first[0];
        auto max_value = first[0];
        size_t minpos = 0;
        size_t maxpos = 0;
        for (size_t k = 1; k < count; ++k) {
            if (first[k] < min_value) {
                min_value = first[k];
                minpos = k;
            }
            if (first[k] > max_value) {
                max_value = first[k];
                maxpos = k;
            }
        }
        check_min_position(minpos, label);
        check_max_position(maxpos, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_position3(T* first, size_t count, const std::string label)
{
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        auto right = first + count;
        auto left = first;
        auto minpos = left;
        auto maxpos = left;
        if (left != right)
            ++left;
        while (left != right) {
            if (*left < *minpos)
                minpos = left;
            if (*left > *maxpos)
                maxpos = left;
            ++left;
        }
        check_min_position( (minpos-first), label);
        check_max_position( (maxpos-first), label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_min_position_unrolled(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        size_t minpos0 = 0;
        size_t minpos1 = 0;
        size_t minpos2 = 0;
        size_t minpos3 = 0;
        for (k = 1; k < (count-3); k += 4) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            if (val0 < min_value0) {
                min_value0 = val0;
                minpos0 = k+0;
            }
            if (val1 < min_value1) {
                min_value1 = val1;
                minpos1 = k+1;
            }
            if (val2 < min_value2) {
                min_value2 = val2;
                minpos2 = k+2;
            }
            if (val3 < min_value3) {
                min_value3 = val3;
                minpos3 = k+3;
            }
        }
        for (; k < count; ++k) {
            if (first[k+0] < min_value0) {
                min_value0 = first[k+0];
                minpos0 = k+0;
            }
        }

        if (min_value1 < min_value0) {
            min_value0 = min_value1;
            minpos0 = minpos1;
        } else if (min_value1 == min_value0 && minpos1 < minpos0)
            minpos0 = minpos1;

        if (min_value2 < min_value0) {
            min_value0 = min_value2;
            minpos0 = minpos2;
        } else if (min_value2 == min_value0 && minpos2 < minpos0)
            minpos0 = minpos2;

        if (min_value3 < min_value0) {
            min_value0 = min_value3;
            minpos0 = minpos3;
        } else if (min_value3 == min_value0 && minpos3 < minpos0)
            minpos0 = minpos3;

        check_min_position(minpos0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_max_position_unrolled(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;
        T max_value0 = first[0];
        T max_value1 = max_value0;
        T max_value2 = max_value0;
        T max_value3 = max_value0;
        size_t maxpos0 = 0;
        size_t maxpos1 = 0;
        size_t maxpos2 = 0;
        size_t maxpos3 = 0;
        for (k = 1; k < (count-3); k += 4) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];
            if (val0 > max_value0) {
                max_value0 = val0;
                maxpos0 = k+0;
            }
            if (val1 > max_value1) {
                max_value1 = val1;
                maxpos1 = k+1;
            }
            if (val2 > max_value2) {
                max_value2 = val2;
                maxpos2 = k+2;
            }
            if (val3 > max_value3) {
                max_value3 = val3;
                maxpos3 = k+3;
            }
        }
        for (; k < count; ++k) {
            if (first[k+0] > max_value0) {
                max_value0 = first[k+0];
                maxpos0 = k+0;
            }
        }

        if (max_value1 > max_value0) {
            max_value0 = max_value1;
            maxpos0 = maxpos1;
        } else if (max_value1 == max_value0 && maxpos1 < maxpos0)
            maxpos0 = maxpos1;

        if (max_value2 > max_value0) {
            max_value0 = max_value2;
            maxpos0 = maxpos2;
        } else if (max_value2 == max_value0 && maxpos2 < maxpos0)
            maxpos0 = maxpos2;

        if (max_value3 > max_value0) {
            max_value0 = max_value3;
            maxpos0 = maxpos3;
        } else if (max_value3 == max_value0 && maxpos3 < maxpos0)
            maxpos0 = maxpos3;

        check_max_position(maxpos0, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T >
void test_minmax_position_unrolled(T* first, size_t count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        size_t k;

        T min_value0 = first[0];
        T min_value1 = min_value0;
        T min_value2 = min_value0;
        T min_value3 = min_value0;
        size_t minpos0 = 0;
        size_t minpos1 = 0;
        size_t minpos2 = 0;
        size_t minpos3 = 0;

        T max_value0 = first[0];
        T max_value1 = max_value0;
        T max_value2 = max_value0;
        T max_value3 = max_value0;
        size_t maxpos0 = 0;
        size_t maxpos1 = 0;
        size_t maxpos2 = 0;
        size_t maxpos3 = 0;

        for (k = 1; k < (count-3); k += 4) {
            T val0 = first[k+0];
            T val1 = first[k+1];
            T val2 = first[k+2];
            T val3 = first[k+3];

            if (val0 < min_value0) {
                min_value0 = val0;
                minpos0 = k+0;
            }
            if (val0 > max_value0) {
                max_value0 = val0;
                maxpos0 = k+0;
            }

            if (val1 < min_value1) {
                min_value1 = val1;
                minpos1 = k+1;
            }
            if (val1 > max_value1) {
                max_value1 = val1;
                maxpos1 = k+1;
            }

            if (val2 < min_value2) {
                min_value2 = val2;
                minpos2 = k+2;
            }
            if (val2 > max_value2) {
                max_value2 = val2;
                maxpos2 = k+2;
            }

            if (val3 < min_value3) {
                min_value3 = val3;
                minpos3 = k+3;
            }
            if (val3 > max_value3) {
                max_value3 = val3;
                maxpos3 = k+3;
            }
        }
        for (; k < count; ++k) {
            if (first[k+0] < min_value0) {
                min_value0 = first[k+0];
                minpos0 = k+0;
            }
            if (first[k+0] > max_value0) {
                max_value0 = first[k+0];
                maxpos0 = k+0;
            }
        }

        if (min_value1 < min_value0) {
            min_value0 = min_value1;
            minpos0 = minpos1;
        } else if (min_value1 == min_value0 && minpos1 < minpos0)
            minpos0 = minpos1;

        if (min_value2 < min_value0) {
            min_value0 = min_value2;
            minpos0 = minpos2;
        } else if (min_value2 == min_value0 && minpos2 < minpos0)
            minpos0 = minpos2;

        if (min_value3 < min_value0) {
            min_value0 = min_value3;
            minpos0 = minpos3;
        } else if (min_value3 == min_value0 && minpos3 < minpos0)
            minpos0 = minpos3;

        if (max_value1 > max_value0) {
            max_value0 = max_value1;
            maxpos0 = maxpos1;
        } else if (max_value1 == max_value0 && maxpos1 < maxpos0)
            maxpos0 = maxpos1;

        if (max_value2 > max_value0) {
            max_value0 = max_value2;
            maxpos0 = maxpos2;
        } else if (max_value2 == max_value0 && maxpos2 < maxpos0)
            maxpos0 = maxpos2;

        if (max_value3 > max_value0) {
            max_value0 = max_value3;
            maxpos0 = maxpos3;
        } else if (max_value3 == max_value0 && maxpos3 < maxpos0)
            maxpos0 = maxpos3;

        check_min_position(minpos0, label);
        check_max_position(maxpos0, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    T data[SIZE];

    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 123 );
    
    // fill one set of random numbers
    fill_random_range( data, data+SIZE, 0x00ffffff );
    
    gMinResult = find_minimum<const T*,T>( data, data+SIZE );
    gMaxResult = find_maximum<const T*,T>( data, data+SIZE );
    gMinPosition = find_minimum_position( data, SIZE );
    gMaxPosition = find_maximum_position( data, SIZE );
    gMaxLastPosition = find_maximum_last_position( data, SIZE );
    
    
    test_min_element( data, SIZE, myTypeName + " minimum value std::min_element");
    test_minmax_value( data, SIZE, min_std_functor<T>(), myTypeName + " minimum value std::min");
    test_min_value1( data, SIZE, myTypeName + " minimum value sequence1");
    test_min_value2( data, SIZE, myTypeName + " minimum value sequence2");
    test_minmax_value( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3");
    test_minmax_value( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4");
    test_minmax_value( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5");    // int only
    test_minmax_value( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6");    // int only
    test_minmax_value( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7");
    test_minmax_value( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8");
    test_minmax_value_unroll4( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled4");    // int only
    test_minmax_value_unroll4( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled4");    // int only
    test_minmax_value_unroll4( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled4");
    test_minmax_value_unroll4a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled4a");    // int only
    test_minmax_value_unroll4a( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled4a");    // int only
    test_minmax_value_unroll4a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled4a");
    
    test_minmax_value_unroll8( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled8");    // int only
    test_minmax_value_unroll8( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled8");    // int only
    test_minmax_value_unroll8( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled8");
    test_minmax_value_unroll8a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled8a");    // int only
    test_minmax_value_unroll8a( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled8a");    // int only
    test_minmax_value_unroll8a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled8a");
    
    test_minmax_value_unroll16( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled16");    // int only
    test_minmax_value_unroll16( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled16");    // int only
    test_minmax_value_unroll16( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled16");
    
    test_minmax_value_unroll16a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor3<T>(), myTypeName + " minimum value sequence5 unrolled16a");    // int only
    test_minmax_value_unroll16a( data, SIZE, min_functor4<T>(), myTypeName + " minimum value sequence6 unrolled16a");    // int only
    test_minmax_value_unroll16a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled16a");

    std::string temp4( myTypeName + " minimum value sequence" );
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_max_element( data, SIZE, myTypeName + " maximum value std::max_element");
    test_minmax_value( data, SIZE, max_std_functor<T>(), myTypeName + " maximum value std::max");
    test_max_value1( data, SIZE, myTypeName + " maximum value sequence1");
    test_max_value2( data, SIZE, myTypeName + " maximum value sequence2");
    test_minmax_value( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3");
    test_minmax_value( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4");
    test_minmax_value( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5");    // int only
    test_minmax_value( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6");    // int only
    test_minmax_value( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7");
    test_minmax_value( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8");
    test_minmax_value_unroll4( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled4");    // int only
    test_minmax_value_unroll4( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled4");    // int only
    test_minmax_value_unroll4( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled4");
    
    test_minmax_value_unroll4a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled4a");    // int only
    test_minmax_value_unroll4a( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled4a");    // int only
    test_minmax_value_unroll4a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled4a");
    
    test_minmax_value_unroll8( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled8");    // int only
    test_minmax_value_unroll8( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled8");    // int only
    test_minmax_value_unroll8( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled8");
    
    test_minmax_value_unroll8a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled8a");    // int only
    test_minmax_value_unroll8a( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled8a");    // int only
    test_minmax_value_unroll8a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled8a");
    
    test_minmax_value_unroll16( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled16");    // int only
    test_minmax_value_unroll16( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled16");    // int only
    test_minmax_value_unroll16( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled16");
    
    test_minmax_value_unroll16a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor3<T>(), myTypeName + " maximum value sequence5 unrolled16a");    // int only
    test_minmax_value_unroll16a( data, SIZE, max_functor4<T>(), myTypeName + " maximum value sequence6 unrolled16a");    // int only
    test_minmax_value_unroll16a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled16a");

    std::string temp3( myTypeName + " maximum value sequence" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    
    

    
    test_minmax_element( data, SIZE, myTypeName + " minmax value std::min and max_element");
    test_minmax_element2( data, SIZE, myTypeName + " minmax value std::minmax_element");
    test_minmax_value2( data, SIZE, min_std_functor<T>(), max_std_functor<T>(), myTypeName + " maximum value std::min and max");
    test_minmax_value1( data, SIZE, myTypeName + " minmax value sequence1");
    test_minmax_value2( data, SIZE, myTypeName + " minmax value sequence2");
    test_minmax_value2( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3");
    test_minmax_value2( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4");
    test_minmax_value2( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5");  // int only
    test_minmax_value2( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6");  // int only
    test_minmax_value2( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7");
    test_minmax_value2( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8");
    
    test_minmax_value2_unroll4( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled4");  // int only
    test_minmax_value2_unroll4( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled4");  // int only
    test_minmax_value2_unroll4( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled4");
    
    test_minmax_value2_unroll4a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled4a");  // int only
    test_minmax_value2_unroll4a( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled4a");  // int only
    test_minmax_value2_unroll4a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled4a");
    
    test_minmax_value2_unroll8( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled8");  // int only
    test_minmax_value2_unroll8( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled8");  // int only
    test_minmax_value2_unroll8( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled8");

    test_minmax_value2_unroll8a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled8a");  // int only
    test_minmax_value2_unroll8a( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled8a");  // int only
    test_minmax_value2_unroll8a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled8a");

    test_minmax_value2_unroll16( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled16");  // int only
    test_minmax_value2_unroll16( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled16");  // int only
    test_minmax_value2_unroll16( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled16");

    test_minmax_value2_unroll16a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor3<T>(), max_functor3<T>(), myTypeName + " minmax value sequence5 unrolled16a");  // int only
    test_minmax_value2_unroll16a( data, SIZE, min_functor4<T>(), max_functor4<T>(), myTypeName + " minmax value sequence6 unrolled16a");  // int only
    test_minmax_value2_unroll16a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled16a");
    
    std::string temp5( myTypeName + " minmax value sequence" );
    summarize(temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    

    // position tests are much slower, even at their best
    auto iterations_base = iterations;
    iterations = iterations / 5;

    test_min_element_pos( data, SIZE, myTypeName + " minimum position std::min_element");
    test_min_position1( data, SIZE, myTypeName + " minimum position sequence1");
    test_min_position2( data, SIZE, myTypeName + " minimum position sequence2");
    test_min_position3( data, SIZE, myTypeName + " minimum position sequence3");
    test_min_position_unrolled( data, SIZE, myTypeName + " minimum position sequence2 unrolled");
    
    std::string temp2( myTypeName + " minimum position sequence" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_max_element_pos( data, SIZE, myTypeName + " maximum position std::max_element");
    test_max_position1( data, SIZE, myTypeName + " maximum position sequence1");
    test_max_position2( data, SIZE, myTypeName + " maximum position sequence2");
    test_max_position3( data, SIZE, myTypeName + " maximum position sequence3");
    test_max_position_unrolled( data, SIZE, myTypeName + " maximum position sequence2 unrolled");
    
    std::string temp1( myTypeName + " maximum position sequence" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_minmax_element_pos( data, SIZE, myTypeName + " minmax position std::minmax_element");
    test_minmax_element_pos2( data, SIZE, myTypeName + " minmax position std::min and max_element");
    test_minmax_position1( data, SIZE, myTypeName + " minmax position sequence1");
    test_minmax_position2( data, SIZE, myTypeName + " minmax position sequence2");
    test_minmax_position2( data, SIZE, myTypeName + " minmax position sequence3");
    test_minmax_position_unrolled( data, SIZE, myTypeName + " minmax position sequence2 unrolled");

    std::string temp6( myTypeName + " minmax position sequence" );
    summarize(temp6.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = iterations_base;

}

/******************************************************************************/
/******************************************************************************/

// Because we can't branch in the template based on type just yet.
// The compiler still tries to generate unreachable code.
template<typename T>
void TestOneFloat()
{
    std::string myTypeName( getTypeName<T>() );
    
    T data[SIZE];

    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 123 );
    
    // fill one set of random numbers
    fill_random( data, data+SIZE );
    
    gMinResult = find_minimum<const T*,T>( data, data+SIZE );
    gMaxResult = find_maximum<const T*,T>( data, data+SIZE );
    gMinPosition = find_minimum_position( data, SIZE );
    gMaxPosition = find_maximum_position( data, SIZE );
    gMaxLastPosition = find_maximum_last_position( data, SIZE );
    

    test_min_element( data, SIZE, myTypeName + " minimum value std::min_element");
    test_minmax_value( data, SIZE, min_std_functor<T>(), myTypeName + " minimum value std::min");
    test_min_value1( data, SIZE, myTypeName + " minimum value sequence1");
    test_min_value2( data, SIZE, myTypeName + " minimum value sequence2");
    test_minmax_value( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3");
    test_minmax_value( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4");
    test_minmax_value( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7");
    test_minmax_value( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8");
    test_minmax_value_unroll4( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled4");
    test_minmax_value_unroll4( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled4");
    test_minmax_value_unroll4a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled4a");
    
    test_minmax_value_unroll8( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled8");
    test_minmax_value_unroll8( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled8");
    test_minmax_value_unroll8a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled8a");
    
    test_minmax_value_unroll16( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled16");
    test_minmax_value_unroll16( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled16");
    test_minmax_value_unroll16a( data, SIZE, min_functor1<T>(), myTypeName + " minimum value sequence3 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor2<T>(), myTypeName + " minimum value sequence4 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor5<T>(), myTypeName + " minimum value sequence7 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, min_functor6<T>(), myTypeName + " minimum value sequence8 unrolled16a");

    std::string temp4( myTypeName + " minimum value sequence" );
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_max_element( data, SIZE, myTypeName + " maximum value std::max_element");
    test_minmax_value( data, SIZE, max_std_functor<T>(), myTypeName + " maximum value std::max");
    test_max_value1( data, SIZE, myTypeName + " maximum value sequence1");
    test_max_value2( data, SIZE, myTypeName + " maximum value sequence2");
    test_minmax_value( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3");
    test_minmax_value( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4");
    test_minmax_value( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7");
    test_minmax_value( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8");
    test_minmax_value_unroll4( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled4");
    test_minmax_value_unroll4( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled4");
    test_minmax_value_unroll4a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled4a");
    test_minmax_value_unroll4a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled4a");
    
    test_minmax_value_unroll8( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled8");
    test_minmax_value_unroll8( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled8");
    test_minmax_value_unroll8a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled8a");
    test_minmax_value_unroll8a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled8a");
    
    test_minmax_value_unroll16( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled16");
    test_minmax_value_unroll16( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled16");
    test_minmax_value_unroll16a( data, SIZE, max_functor1<T>(), myTypeName + " maximum value sequence3 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor2<T>(), myTypeName + " maximum value sequence4 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor5<T>(), myTypeName + " maximum value sequence7 unrolled16a");
    test_minmax_value_unroll16a( data, SIZE, max_functor6<T>(), myTypeName + " maximum value sequence8 unrolled16a");

    std::string temp3( myTypeName + " maximum value sequence" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_minmax_element( data, SIZE, myTypeName + " minmax value std::min and max_element");
    test_minmax_element2( data, SIZE, myTypeName + " minmax value std::minmax_element");
    test_minmax_value2( data, SIZE, min_std_functor<T>(), max_std_functor<T>(), myTypeName + " maximum value std::min and max");
    test_minmax_value1( data, SIZE, myTypeName + " minmax value sequence1");
    test_minmax_value2( data, SIZE, myTypeName + " minmax value sequence2");
    test_minmax_value2( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3");
    test_minmax_value2( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4");
    test_minmax_value2( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7");
    test_minmax_value2( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8");

    test_minmax_value2_unroll4( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled4");
    test_minmax_value2_unroll4( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled4");

    test_minmax_value2_unroll4a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled4a");
    test_minmax_value2_unroll4a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled4a");

    test_minmax_value2_unroll8( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled8");
    test_minmax_value2_unroll8( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled8");

    test_minmax_value2_unroll8a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled8a");
    test_minmax_value2_unroll8a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled8a");

    test_minmax_value2_unroll16( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled16");
    test_minmax_value2_unroll16( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled16");

    test_minmax_value2_unroll16a( data, SIZE, min_functor1<T>(), max_functor1<T>(), myTypeName + " minmax value sequence3 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor2<T>(), max_functor2<T>(), myTypeName + " minmax value sequence4 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor5<T>(), max_functor5<T>(), myTypeName + " minmax value sequence7 unrolled16a");
    test_minmax_value2_unroll16a( data, SIZE, min_functor6<T>(), max_functor6<T>(), myTypeName + " minmax value sequence8 unrolled16a");

    std::string temp5( myTypeName + " minmax value sequence" );
    summarize(temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // position tests are much slower, even at their best
    auto iterations_base = iterations;
    iterations = iterations / 5;
    
    test_min_element_pos( data, SIZE, myTypeName + " minimum position std::min_element");
    test_min_position1( data, SIZE, myTypeName + " minimum position sequence1");
    test_min_position2( data, SIZE, myTypeName + " minimum position sequence2");
    test_min_position3( data, SIZE, myTypeName + " minimum position sequence3");
    test_min_position_unrolled( data, SIZE, myTypeName + " minimum position sequence2 unrolled");
    
    std::string temp2( myTypeName + " minimum position sequence" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_max_element_pos( data, SIZE, myTypeName + " maximum position std::max_element");
    test_max_position1( data, SIZE, myTypeName + " maximum position sequence1");
    test_max_position2( data, SIZE, myTypeName + " maximum position sequence2");
    test_max_position3( data, SIZE, myTypeName + " maximum position sequence3");
    test_max_position_unrolled( data, SIZE, myTypeName + " maximum position sequence2 unrolled");
    
    std::string temp1( myTypeName + " maximum position sequence" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_minmax_element_pos( data, SIZE, myTypeName + " minmax position std::minmax_element");
    test_minmax_element_pos2( data, SIZE, myTypeName + " minmax position std::min and max_element");
    test_minmax_position1( data, SIZE, myTypeName + " minmax position sequence1");
    test_minmax_position2( data, SIZE, myTypeName + " minmax position sequence2");
    test_minmax_position2( data, SIZE, myTypeName + " minmax position sequence3");
    test_minmax_position_unrolled( data, SIZE, myTypeName + " minmax position sequence2 unrolled");

    std::string temp6( myTypeName + " minmax position sequence" );
    summarize(temp6.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = iterations_base;
    
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


    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();

    iterations /= 2;
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    
    iterations /= 2;
    TestOneType<int64_t>();
    TestOneType<uint64_t>();

    TestOneFloat<float>();
    TestOneFloat<double>();
    TestOneFloat<long double>();
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
