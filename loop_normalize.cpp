/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine changes in performance with different loop types and termination styles.
        Related to loop normalization or loop canonization, and somewhat to induction variable elimination.

Assumptions:
    1) The compiler will normalize all loop types and optimize all equally.
    
    2) The compiler will normalize different loop termination styles.
    
    3) The compiler will recognize pointless loops and induction variables,
        and optimize them away.



NOTE - All count_half cases here (except the labeled unoptimizable cases) are
the same loop, just expressed in slightly different ways.  This problem was
found when looking at bad machine code generated by std::reverse templates.

*/

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 800000;


// 4000 items, or about 4k - 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 4000;


// initial value for filling our arrays, may be changed from the command line
int init_value = 3;

/******************************************************************************/
/******************************************************************************/

// An unoptimizable case (because it depends on finding a NULL in the data)
template <typename T>
size_t count_half_for_uncountable( T* begin, const int count ) {
    
    size_t result = 0;
    for ( T *i = begin; *i != 0; ++i ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

// Removing the useless loop gets a fully optimized version.
// Several different optimizations could lead to this, after the loop and termination conditions are normalized.
template <typename T>
size_t count_half_optA( T* /* begin */, const int count ) {
    const int loop_limit = count / 2;
    size_t result = loop_limit;
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_opt( T* /* begin */, const int count ) {
    
    const int loop_limit = count / 2;
    size_t result = 0;
    for ( int k = 0; k < loop_limit; ++k ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer1( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    for ( i = begin, j = end-1; i < j; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer2( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    for ( i = begin, j = end-1; j > i; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer3( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    for ( i = begin, j = end-1; (j - i) > 0; ++i, --j ) {    // ((j - i) > 0) ==> (j > i)
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer4( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    for ( i = begin, j = end; i < --j; ++i ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer5( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    for ( i = begin, j = end; i != j; ++i ) {
        if (i == --j)
            break;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_pointer6( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    for ( i = begin, j = end-1; i < j; ++i, --j ) {
    }
    return size_t( i - begin );
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator1( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    size_t result = 0;
    for ( i = begin, j = end-1; i < j; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator2( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    size_t result = 0;
    for ( i = begin, j = end-1; j > i; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator3( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    size_t result = 0;
    for ( i = begin, j = end-1; (j - i) > 0; ++i, --j ) {    // ((j - i) > 0) ==> (j > i)
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator4( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    size_t result = 0;
    for ( i = begin, j = end; i < --j; ++i ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator5( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    size_t result = 0;
    for ( i = begin, j = end; i != j; ++i ) {
        if (i == --j)
            break;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_iterator6( T begin, const int count ) {
    
    T end = begin + count;
    T i, j;
    for ( i = begin, j = end-1; i < j; ++i, --j ) {
    }
    return size_t( i - begin );
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index1( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count-1; i < j; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index2( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count-1; j > i; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index3( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count-1; (j - i) > 0; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index4( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count-1; (i - j) < 0; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index5( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count-1; (j - i) > 0; ++i, --j ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index6( T* /* begin */, const int count ) {

    int i, j;
    size_t result = 0;
    for ( i = 0, j = count; i < --j; ++i ) {
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index7( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    for ( i = 0, j = count; i != j; ++i ) {
        if (i == --j)
            break;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_for_index8( T* /* begin */, const int count ) {
    
    int i, j;
    for ( i = 0, j = count-1; i < j; ++i, --j ) {
    }
    return size_t( i );
}

/******************************************************************************/
/******************************************************************************/

// An unoptimizable case (because it depends on finding a NULL in the data)
template <typename T>
size_t count_half_while_uncountable( T* begin, const int count ) {
    
    size_t result = 0;
    T *i = begin;
    while ( *i != 0 ) {
        ++result;
        ++i;
    }
    return result;
}

/******************************************************************************/

// use an explicit iteration counter variable
template <typename T>
size_t count_half_while_opt( T* /* begin */, const int count ) {
    
    const int loop_limit = count / 2;
    size_t result = 0;
    int k = 0;
    while ( k < loop_limit ) {
        ++result;
        ++k;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer1( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    while ( i < j ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer2( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    while ( j > i ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer3( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    while ( (j - i) > 0 ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer4( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end;
    while ( i < --j ) {
        ++result;
        ++i;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer5( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    i = begin;
    j = end;
    size_t result = 0;
    while ( i != j ) {
        if (i == --j)
            break;
        ++i;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_pointer6( T* begin, const int count ) {
    
    T *end = begin + count;
    T *i, *j;
    i = begin;
    j = end-1;
    while ( i < j ) {
        ++i;
        --j;
    }
    return size_t( i - begin );
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index1( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    while ( i < j ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index2( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    while ( j > i ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index3( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    while ( (j - i) > 0 ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index4( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    while ( (i - j) < 0 ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index5( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    while ( (j - i) > 0 ) {
        ++result;
        ++i;
        --j;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index6( T* /* begin */, const int count ) {

    int i, j;
    size_t result = 0;
    i = 0;
    j = count;
    while ( i < --j ) {
        ++i;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index7( T* /* begin */, const int count ) {
    
    int i, j;
    size_t result = 0;
    i = 0;
    j = count;
    while ( i != j ) {
        if (i == --j)
            break;
        ++i;
        ++result;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_while_index8( T* /* begin */, const int count ) {
    
    int i, j;
    i = 0;
    j = count-1;
    while( i < j ) {
        ++i;
        --j;
    }
    return size_t( i );
}

/******************************************************************************/
/******************************************************************************/

// An unoptimizable case (because it depends on finding a NULL in the data)
template <typename T>
size_t count_half_do_uncountable( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    size_t result = 0;
    T *i = begin;
    do {
        ++result;
        ++i;
    } while ( *i != 0 );
    return result;
}

/******************************************************************************/

// use an explicit iteration counter variable
template <typename T>
size_t count_half_do_opt( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    const int loop_limit = count / 2;
    size_t result = 0;
    int k = 0;
    do {
        ++result;
        ++k;
    } while ( k < loop_limit );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_pointer1( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( i < j );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_pointer2( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( j > i );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_pointer3( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( (j - i) > 0 );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_index1( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( i < j );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_index2( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( j > i );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_index3( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( (j - i) > 0 );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_index4( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( (i - j) < 0 );
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_do_index5( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    do {
        ++result;
        ++i;
        --j;
    } while ( (j - i) > 0 );
    return result;
}

/******************************************************************************/
/******************************************************************************/

// An unoptimizable case (because it depends on finding a NULL in the data)
template <typename T>
size_t count_half_goto_uncountable( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    size_t result = 0;
    T *i = begin;
    {
loop_start:
        ++result;
        ++i;
        if ( *i != 0 )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

// use an explicit iteration counter variable
template <typename T>
size_t count_half_goto_opt( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    const int loop_limit = count / 2;
    size_t result = 0;
    int k = 0;

    {
loop_start:
        ++result;
        ++k;
        if ( k < loop_limit )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_pointer1( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( i < j )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_pointer2( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( j > i )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_pointer3( T* begin, const int count ) {
    
    if (count <= 0)
        return 0;
    T *end = begin + count;
    T *i, *j;
    size_t result = 0;
    i = begin;
    j = end-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( (j - i) > 0 )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_index1( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( i < j )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_index2( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( j > i )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_index3( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( (j - i) > 0 )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_index4( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( (i - j) < 0  )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T>
size_t count_half_goto_index5( T* /* begin */, const int count ) {
    
    if (count <= 0)
        return 0;
    int i, j;
    size_t result = 0;
    i = 0;
    j = count-1;
    {
loop_start:
        ++result;
        ++i;
        --j;
        if ( (j - i) > 0 )
            goto loop_start;
    }
    return result;
}

/******************************************************************************/
/******************************************************************************/

inline void check_half(size_t result, const int count, const std::string &label) {
    if ( result != (count/2) )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

std::deque<std::string> gLabels;

template <typename Iterator, typename Counter>
void test_count_half(Iterator first, const int count, Counter count_func, const std::string &label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        size_t half = count_func( first, count );
        check_half( half, count, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void TestLoops()
{
    typedef std::vector<T> intVector;

    T myValues[ SIZE ];
    intVector myVectorValues( SIZE );
    
    std::string myTypeName( getTypeName<T>() );
    std::string myTypeNameP( getTypeName<T *>() );
    
    gLabels.clear();
    
    std::fill( myValues, myValues+SIZE, T(init_value) );
    std::fill( myVectorValues.begin(), myVectorValues.end(), T(init_value) );
    myValues[ SIZE/2 ] = 0;    // marker for our unoptimizable case


    test_count_half( &myValues[0], SIZE, count_half_for_opt<T>, myTypeName + " for count_half opt" );
    test_count_half( &myValues[0], SIZE, count_half_optA<T>, myTypeName + " no_loop count_half opt" );
    test_count_half( &myValues[0], SIZE, count_half_for_uncountable<T>, myTypeName + " for count_half no_opt" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer1<T>, myTypeName + " for count_half pointer1" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer2<T>, myTypeName + " for count_half pointer2" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer3<T>, myTypeName + " for count_half pointer3" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer4<T>, myTypeName + " for count_half pointer4" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer5<T>, myTypeName + " for count_half pointer5" );
    test_count_half( &myValues[0], SIZE, count_half_for_pointer6<T>, myTypeName + " for count_half pointer6" );
    test_count_half( &myValues[0], SIZE, count_half_for_index1<T>, myTypeName + " for count_half index1" );
    test_count_half( &myValues[0], SIZE, count_half_for_index2<T>, myTypeName + " for count_half index2" );
    test_count_half( &myValues[0], SIZE, count_half_for_index3<T>, myTypeName + " for count_half index3" );
    test_count_half( &myValues[0], SIZE, count_half_for_index4<T>, myTypeName + " for count_half index4" );
    test_count_half( &myValues[0], SIZE, count_half_for_index5<T>, myTypeName + " for count_half index5" );
    test_count_half( &myValues[0], SIZE, count_half_for_index6<T>, myTypeName + " for count_half index6" );
    test_count_half( &myValues[0], SIZE, count_half_for_index7<T>, myTypeName + " for count_half index7" );
    test_count_half( &myValues[0], SIZE, count_half_for_index8<T>, myTypeName + " for count_half index8" );
    test_count_half( myValues, SIZE, count_half_for_iterator1<T *>, myTypeNameP + " for count_half iterator1" );
    test_count_half( myValues, SIZE, count_half_for_iterator2<T *>, myTypeNameP + " for count_half iterator2" );
    test_count_half( myValues, SIZE, count_half_for_iterator3<T *>, myTypeNameP + " for count_half iterator3" );
    test_count_half( myValues, SIZE, count_half_for_iterator4<T *>, myTypeNameP + " for count_half iterator4" );
    test_count_half( myValues, SIZE, count_half_for_iterator5<T *>, myTypeNameP + " for count_half iterator5" );
    test_count_half( myValues, SIZE, count_half_for_iterator6<T *>, myTypeNameP + " for count_half iterator6" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator1< typename intVector::iterator>, myTypeName + " vector for count_half iterator1" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator2< typename intVector::iterator>, myTypeName + " vector for count_half iterator2" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator3< typename intVector::iterator>, myTypeName + " vector for count_half iterator3" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator4< typename intVector::iterator>, myTypeName + " vector for count_half iterator4" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator5< typename intVector::iterator>, myTypeName + " vector for count_half iterator5" );
    test_count_half( myVectorValues.begin(), myVectorValues.size(),
        count_half_for_iterator6< typename intVector::iterator>, myTypeName + " vector for count_half iterator6" );
    
    std::string temp1( myTypeName + " for loop normalize count half" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_count_half( &myValues[0], SIZE, count_half_while_opt<T>, myTypeName + " while count_half opt" );
    test_count_half( &myValues[0], SIZE, count_half_while_uncountable<T>, myTypeName + " while count_half no_opt" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer1<T>, myTypeName + " while count_half pointer1" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer2<T>, myTypeName + " while count_half pointer2" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer3<T>, myTypeName + " while count_half pointer3" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer4<T>, myTypeName + " while count_half pointer4" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer5<T>, myTypeName + " while count_half pointer5" );
    test_count_half( &myValues[0], SIZE, count_half_while_pointer6<T>, myTypeName + " while count_half pointer6" );
    test_count_half( &myValues[0], SIZE, count_half_while_index1<T>, myTypeName + " while count_half index1" );
    test_count_half( &myValues[0], SIZE, count_half_while_index2<T>, myTypeName + " while count_half index2" );
    test_count_half( &myValues[0], SIZE, count_half_while_index3<T>, myTypeName + " while count_half index3" );
    test_count_half( &myValues[0], SIZE, count_half_while_index4<T>, myTypeName + " while count_half index4" );
    test_count_half( &myValues[0], SIZE, count_half_while_index5<T>, myTypeName + " while count_half index5" );
    test_count_half( &myValues[0], SIZE, count_half_while_index6<T>, myTypeName + " while count_half index6" );
    test_count_half( &myValues[0], SIZE, count_half_while_index7<T>, myTypeName + " while count_half index7" );
    test_count_half( &myValues[0], SIZE, count_half_while_index8<T>, myTypeName + " while count_half index8" );
    
    std::string temp2( myTypeName + " while loop normalize count half" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_count_half( &myValues[0], SIZE, count_half_do_opt<T>, myTypeName + " do count_half opt" );
    test_count_half( &myValues[0], SIZE, count_half_do_uncountable<T>, myTypeName + " do count_half no_opt" );
    test_count_half( &myValues[0], SIZE, count_half_do_pointer1<T>, myTypeName + " do count_half pointer1" );
    test_count_half( &myValues[0], SIZE, count_half_do_pointer2<T>, myTypeName + " do count_half pointer2" );
    test_count_half( &myValues[0], SIZE, count_half_do_pointer3<T>, myTypeName + " do count_half pointer3" );
    test_count_half( &myValues[0], SIZE, count_half_do_index1<T>, myTypeName + " do count_half index1" );
    test_count_half( &myValues[0], SIZE, count_half_do_index2<T>, myTypeName + " do count_half index2" );
    test_count_half( &myValues[0], SIZE, count_half_do_index3<T>, myTypeName + " do count_half index3" );
    test_count_half( &myValues[0], SIZE, count_half_do_index4<T>, myTypeName + " do count_half index4" );
    test_count_half( &myValues[0], SIZE, count_half_do_index5<T>, myTypeName + " do count_half index5" );
    
    std::string temp3( myTypeName + " do loop normalize count half" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_count_half( &myValues[0], SIZE, count_half_goto_opt<T>, myTypeName + " goto count_half opt" );
    test_count_half( &myValues[0], SIZE, count_half_goto_uncountable<T>, myTypeName + " goto count_half no_opt" );
    test_count_half( &myValues[0], SIZE, count_half_goto_pointer1<T>, myTypeName + " goto count_half pointer1" );
    test_count_half( &myValues[0], SIZE, count_half_goto_pointer2<T>, myTypeName + " goto count_half pointer2" );
    test_count_half( &myValues[0], SIZE, count_half_goto_pointer3<T>, myTypeName + " goto count_half pointer3" );
    test_count_half( &myValues[0], SIZE, count_half_goto_index1<T>, myTypeName + " goto count_half index1" );
    test_count_half( &myValues[0], SIZE, count_half_goto_index2<T>, myTypeName + " goto count_half index2" );
    test_count_half( &myValues[0], SIZE, count_half_goto_index3<T>, myTypeName + " goto count_half index3" );
    test_count_half( &myValues[0], SIZE, count_half_goto_index4<T>, myTypeName + " goto count_half index4" );
    test_count_half( &myValues[0], SIZE, count_half_goto_index5<T>, myTypeName + " goto count_half index5" );
    
    std::string temp4( myTypeName + " goto loop normalize count half" );
    summarize( temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/

int main(int argc, char** argv) {

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int) atoi(argv[2]);
    
    // Zero is reserved for our unoptimizeable marker,
    // and the value must fit in int8_t, and uint8_t.
    if (init_value == 0 || init_value > 127)
        init_value = 42;


    TestLoops<int32_t>();

/*
    Currently the success/fail pattern is the same for all types, on all compilers tested.
    Yay! We can skip the pain of testing all types, for now.

    TestLoops<uint32_t>();
 
    TestLoops<int64_t>();
    TestLoops<uint64_t>();
    
    TestLoops<int16_t>();
    TestLoops<uint16_t>();
 
    TestLoops<int8_t>();
    TestLoops<uint8_t>();

    TestLoops<float>();
    TestLoops<double>();

*/

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
