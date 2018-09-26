/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to loop unrolling.

Assumptions:

    1) The compiler will unroll loops to hide instruction latency and maximize performance
        for() {}
        while() {}
        do {} while()
        goto

    2) If the compiler unrolls the loop, it should not be slower than the original loop without unrolling.

    3) The compiler should unroll a multi-calculation loop as well as a single calculation loop,
        up to the limit of performance gain for unrolling that loop.
        In other words: no penalty for manually unrolling,
                        as long as the manual unroll is less than or equal to the optimum unroll factor.

    4) The compiler should recognize and unroll all loop styles with the same efficiency.
        In other words: do, while, for, and goto should have identical performance
        See also: loop_normalize.cpp

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400000;

// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000

// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

// how far are we willing to unroll loops for this test?
// beyond 50, some compilers break.
const int UnrollLimit = 32;

/******************************************************************************/

// so we keep labels around until printed
std::deque< std::string > gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

// overflow is entirely expected: this is a hash function, not an exact math function.
template <typename T>
T hash_func2(T seed) {
    return (914237 * (seed + 12345)) - 13;
}

template <typename T>
T complete_hash_func(T seed) {
    return hash_func2( hash_func2( hash_func2( seed ) ) );
}

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
  T temp = (T)SIZE * complete_hash_func( (T)init_value );
  if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

// this is the heart of our loop unrolling - a class that unrolls itself to generate the inner loop code
// at least as long as we keep F < 50 (or some compilers won't compile it)
template< int F, typename T >
struct loop_inner_body {
    inline static void do_work(T &result, const T *first, int n) {
        loop_inner_body<F-1,T>::do_work(result, first, n);
        T temp = first[ n + (F-1) ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/
/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_for_loop_unroll_factor(const T* first, int count, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int n = 0;
    
    for (; n < (count - (F-1)); n += F) {
        loop_inner_body<F,T>::do_work(result,first, n);
    }
    
    for (; n < count; ++n) {
        result += complete_hash_func( first[n] );
    }
    
    check_sum<T>(result, label);
  }
  
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_while_loop_unroll_factor(const T* first, int count, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int n = 0;
    
    while ( n < (count - (F-1)) ) {
        loop_inner_body<F,T>::do_work(result,first, n);
        n += F;
    }
    
    while ( n < count ) {
        result += complete_hash_func( first[n] );
        ++n;
    }
    
    check_sum<T>(result, label);
  }
  
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_do_loop_unroll_factor(const T* first, int count, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int n = 0;
    
    if ((count - n) >= F)
        do {
            loop_inner_body<F,T>::do_work(result,first, n);
            n += F;
        } while (n < (count - (F-1)));
    
    if (n < count)
        do {
            result += complete_hash_func( first[n] );
            ++n;
        } while (n != count);
    
    check_sum<T>(result, label);
  }
  
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_goto_loop_unroll_factor(const T* first, int count, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    int n = 0;
    
    if ((count - n) >= F) {
loop2_start:
        loop_inner_body<F,T>::do_work(result,first, n);
        n += F;
        
        if (n < (count - (F-1)))
            goto loop2_start;
    }

    if (n < count) {
loop_start:
        result += complete_hash_func( first[n] );
        ++n;
        
        if (n != count)
            goto loop_start;
    }
    
    check_sum<T>(result, label);
  }
  
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct for_loop_tests {
    static void do_test( const T *data, const std::string &label_base ) {
        for_loop_tests<N-1, T>::do_test(data, label_base);
        std::string temp_string( label_base + " " + std::to_string(N) );
        test_for_loop_unroll_factor<N>( data, SIZE, temp_string );
    }
};

template<typename T>
struct for_loop_tests<0,T> {
    static void do_test( const T *, const std::string & ) {
    }
};

/******************************************************************************/

template< int N, typename T >
struct while_loop_tests {
    static void do_test( const T *data, const std::string &label_base ) {
        while_loop_tests<N-1, T>::do_test(data, label_base);
        std::string temp_string( label_base + " " + std::to_string(N) );
        test_while_loop_unroll_factor<N>( data, SIZE, temp_string );
    }
};

template<typename T>
struct while_loop_tests<0,T> {
    static void do_test( const T *, const std::string & ) {
    }
};

/******************************************************************************/

template< int N, typename T >
struct do_loop_tests {
    static void do_test( const T *data, const std::string &label_base ) {
        do_loop_tests<N-1, T>::do_test(data, label_base);
        std::string temp_string( label_base + " " + std::to_string(N) );
        test_do_loop_unroll_factor<N>( data, SIZE, temp_string );
    }
};

template<typename T>
struct do_loop_tests<0,T> {
    static void do_test( const T *, const std::string & ) {
    }
};

/******************************************************************************/

template< int N, typename T >
struct goto_loop_tests {
    static void do_test( const T *data, const std::string &label_base ) {
        goto_loop_tests<N-1, T>::do_test(data, label_base);
        std::string temp_string( label_base + " " + std::to_string(N) );
        test_goto_loop_unroll_factor<N>( data, SIZE, temp_string );
    }
};

template<typename T>
struct goto_loop_tests<0,T> {
    static void do_test( const T *, const std::string & ) {
    }
};

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestUnrollType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();

    T data[ SIZE ];
    ::fill(data, data+SIZE, T(init_value));

    for_loop_tests<UnrollLimit, T>::do_test( data, myTypeName + " for loop unroll" );
    std::string temp1( myTypeName + " for loop unrolling" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    while_loop_tests<UnrollLimit, T>::do_test( data, myTypeName + " while loop unroll" );
    std::string temp2( myTypeName + " while loop unrolling" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    do_loop_tests<UnrollLimit, T>::do_test( data, myTypeName + " do loop unroll" );
    std::string temp3( myTypeName + " do loop unrolling" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    goto_loop_tests<UnrollLimit, T>::do_test( data, myTypeName + " goto loop unroll" );
    std::string temp4( myTypeName + " goto loop unrolling" );
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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


    // Yes, too many compilers are sloppy about loop unrolling and instruction scheduling, with results varying by type
    TestUnrollType<uint8_t> ();
    TestUnrollType<uint16_t> ();
    TestUnrollType<uint32_t> ();
    TestUnrollType<uint64_t> ();
    
    iterations /= 2;
    TestUnrollType<float> ();
    TestUnrollType<double> ();

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
