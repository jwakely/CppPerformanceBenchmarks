/*
    Copyright 2009 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various idioms for counting items matching a value in a sequence.


Assumptions:

    1) The compiler will optimize counting operations.
        NOTE - The optimal approach usually varies with the data type.
    
    2) The compiler may recognize ineffecient counting idioms and substitute efficient methods.
 
    3) std::count and std::count_if should be well optimized.


*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
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
int iterations = 400000;


// 8000 items, or about 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

size_t gCount = 0;

inline void check_count( size_t result, const std::string label ) {
    if (result != gCount)
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template < typename T >
size_t count1(T* first, int length, T value) {
    size_t count = 0;
    
    for ( int i = 0; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }

    return count;
}

/******************************************************************************/

// unroll 2X
template < typename T >
size_t count2(T* first, int length, T value) {
    size_t count = 0;
    int i;
    
    for ( i = 0; i < (length - 1); i += 2 ) {
        if (first[i+0] == value)
            ++count;
        if (first[i+1] == value)
            ++count;
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count;
}

/******************************************************************************/

// unroll 4X
template < typename T >
size_t count3(T* first, int length, T value) {
    size_t count = 0;
    int i;
    
    for ( i = 0; i < (length - 3); i += 4 ) {
        if (first[i+0] == value)
            ++count;
        if (first[i+1] == value)
            ++count;
        if (first[i+2] == value)
            ++count;
        if (first[i+3] == value)
            ++count;
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count;
}

/******************************************************************************/

// unroll 4X, use 2 counters
template < typename T >
size_t count4(T* first, int length, T value) {
    size_t count = 0;
    size_t count1 = 0;
    int i;
    
    for ( i = 0; i < (length - 3); i += 4 ) {
        if (first[i+0] == value)
            ++count;
        if (first[i+1] == value)
            ++count1;
        if (first[i+2] == value)
            ++count;
        if (first[i+3] == value)
            ++count1;
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count + count1;
}

/******************************************************************************/

// unroll 4X, use 4 counters
template < typename T >
size_t count5(T* first, int length, T value) {
    size_t count = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    int i;

    for ( i = 0; i < (length - 3); i += 4 ) {
        if (first[i+0] == value)
            ++count;
        if (first[i+1] == value)
            ++count1;
        if (first[i+2] == value)
            ++count2;
        if (first[i+3] == value)
            ++count3;
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count + count1 + count2 + count3;
}

/******************************************************************************/

// unroll 4X, use 4 counters, always increment using bools
template < typename T >
size_t count6(T* first, int length, T value) {
    size_t count = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    int i;

    for ( i = 0; i < (length - 3); i += 4 ) {
        count  += size_t(first[i+0] == value);
        count1 += size_t(first[i+1] == value);
        count2 += size_t(first[i+2] == value);
        count3 += size_t(first[i+3] == value);
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count + count1 + count2 + count3;
}

/******************************************************************************/

// unroll 8X, use 4 counters
template < typename T >
size_t count7(T* first, int length, T value) {
    size_t count = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    int i;

    for ( i = 0; i < (length - 7); i += 8 ) {
        if (first[i+0] == value)
            ++count;
        if (first[i+1] == value)
            ++count1;
        if (first[i+2] == value)
            ++count2;
        if (first[i+3] == value)
            ++count3;
        if (first[i+4] == value)
            ++count;
        if (first[i+5] == value)
            ++count1;
        if (first[i+6] == value)
            ++count2;
        if (first[i+7] == value)
            ++count3;
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count + count1 + count2 + count3;
}

/******************************************************************************/

// unroll 8X, use 4 counters, always increment using bools
template < typename T >
size_t count8(T* first, int length, T value) {
    size_t count = 0;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t count3 = 0;
    int i;

    for ( i = 0; i < (length - 7); i += 8 ) {
        count  += size_t(first[i+0] == value);
        count1 += size_t(first[i+1] == value);
        count2 += size_t(first[i+2] == value);
        count3 += size_t(first[i+3] == value);
        count  += size_t(first[i+4] == value);
        count1 += size_t(first[i+5] == value);
        count2 += size_t(first[i+6] == value);
        count3 += size_t(first[i+7] == value);
    }
    
    for ( ; i < length; ++i ) {
        if (first[i] == value)
            ++count;
    }
    
    return count + count1 + count2 + count3;
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template < typename T >
void test_std_count(T* first, int length, T value, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    size_t count = std::count( first, first+length, value );
    check_count( count, label );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename T>
bool matches(T x) { return (x == T(init_value)); }

template < typename T >
void test_std_count_if(T* first, int length, T value, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    //size_t count = std::count_if( first, first+length, [] (T x) {return (x == value);} );   // sigh, not yet
    size_t count = std::count_if( first, first+length, matches<T> );
    check_count( count, label );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template < typename T, typename Counter >
void test_count(T* first, int length, T value, Counter count_func, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    size_t count = count_func( first, length, value );
    check_count( count, label );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    // seed the random number generator so we get repeatable results
    srand( (int)init_value + 123 );
    
    T data8[SIZE];
    
    // fill one set of random numbers
    fill_random( data8, data8+SIZE );
    // add several copies of our counted item
    fill( data8, data8+42, T(init_value) );
    // shuffle
    random_shuffle( data8, data8+SIZE );
    // and get our offical count (because the random values may include our item as well)
    gCount = std::count( data8, data8+SIZE, T(init_value) );
    
    test_std_count( data8, SIZE, T(init_value), myTypeName + " std::count");
    test_std_count_if( data8, SIZE, T(init_value), myTypeName + " std::count_if");
    test_count( data8, SIZE, T(init_value), count1<T>, myTypeName + " count1");
    test_count( data8, SIZE, T(init_value), count2<T>, myTypeName + " count2");
    test_count( data8, SIZE, T(init_value), count3<T>, myTypeName + " count3");
    test_count( data8, SIZE, T(init_value), count4<T>, myTypeName + " count4");
    test_count( data8, SIZE, T(init_value), count5<T>, myTypeName + " count5");
    test_count( data8, SIZE, T(init_value), count6<T>, myTypeName + " count6");
    test_count( data8, SIZE, T(init_value), count7<T>, myTypeName + " count7");
    test_count( data8, SIZE, T(init_value), count8<T>, myTypeName + " count8");

    std::string temp1( myTypeName + " count sequence" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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
    
    
    TestOneType<uint8_t>();
    TestOneType<int8_t>();
    TestOneType<uint16_t>();
    TestOneType<int16_t>();
    TestOneType<uint32_t>();
    TestOneType<int32_t>();
    TestOneType<uint64_t>();
    TestOneType<int64_t>();
    TestOneType<float>();
    TestOneType<double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
