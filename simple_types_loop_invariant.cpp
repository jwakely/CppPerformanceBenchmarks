/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to simple language defined types,
        loop invariant code motion.

Assumptions:

    1) the compiler will move loop invariant calculations on simple types out of a loop
        aka: loop invariant code motion
        
        for (i = 0; i < N; ++i)                        temp = A + B + C + D;
            result = input[i] + A+B+C+D;    ==>        for (i = 0; i < N; ++i)
                                                          result = input[i] + temp;

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <deque>
#include <string>
#include <type_traits>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 200000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

static std::deque<std::string> gLabels;

/******************************************************************************/

// v1 is constant in the function, so we can move the addition or subtraction of it outside the loop entirely,
// converting it to a multiply and a summation of the input array.
// Note that this is always legal for integers.
// It can only be applied to floating point if using inexact math (relaxed IEEE rules).
template <typename T, typename Shifter>
void test_hoisted_variable1(T* first, int count, T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += first[n];
    }
    result += count * v1;
    check_shifted_variable_sum<T, Shifter>(result, v1);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_variable1(T* first, int count, T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n], v1 );
    }
    check_shifted_variable_sum<T, Shifter>(result, v1);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_variable4(T* first, int count, T v1, T v2, T v3, T v4, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n], v1, v2, v3, v4 );
    }
    check_shifted_variable_sum<T, Shifter>(result, v1, v2, v3, v4);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
typename std::enable_if<std::is_floating_point<T>::value,void>::type
 TestLoopsIntegerOnly(T *data, T var1int1, T var1int2, T var1int3, T var1int4 )
{
    // can't do bit operations on floating point values
}

template< typename T >
typename std::enable_if<std::is_integral<T>::value,void>::type
 TestLoopsIntegerOnly(T *data, T var1int1, T var1int2, T var1int3, T var1int4 )
{
    std::string myTypeName( getTypeName<T>() );
    
    test_variable1< T, custom_variable_and<T> > (data, SIZE, var1int1, myTypeName + " variable and");
    test_variable4< T, custom_multiple_variable_and<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable and");
    test_variable4< T, custom_multiple_variable_and2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable and2");

    test_variable1< T, custom_variable_or<T> > (data, SIZE, var1int1, myTypeName + " variable or");
    test_variable4< T, custom_multiple_variable_or<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable or");
    test_variable4< T, custom_multiple_variable_or2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable or2");

    test_variable1< T, custom_variable_xor<T> > (data, SIZE, var1int1, myTypeName + " variable xor");
    test_variable4< T, custom_multiple_variable_xor<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable xor");
    test_variable4< T, custom_multiple_variable_xor2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable xor2");
}

/******************************************************************************/

template< typename T >
void TestLoops(double temp)
{
    T data[ SIZE ];
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    ::fill(data, data+SIZE, T(init_value));
    T var1int1, var1int2, var1int3, var1int4;
    var1int1 = T(temp);
    var1int2 = var1int1 * T(2);
    var1int3 = var1int1 + T(2);
    var1int4 = var1int1 + var1int2 / var1int3;
    
    
    test_variable1< T, custom_add_variable<T> > (data, SIZE, var1int1, myTypeName + " variable add");
    test_hoisted_variable1< T, custom_add_variable<T> > (data, SIZE, var1int1, myTypeName + " variable add hoisted");
    test_variable4< T, custom_add_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable adds");
    test_variable4< T, custom_add_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable adds2");

    test_variable1< T, custom_sub_variable<T> > (data, SIZE, var1int1, myTypeName + " variable subtract");
    test_variable4< T, custom_sub_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable subtracts");
    test_variable4< T, custom_sub_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable subtracts2");
    
    test_variable1< T, custom_multiply_variable<T> > (data, SIZE, var1int1, myTypeName + " variable multiply");
    test_variable4< T, custom_multiply_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies");
    test_variable4< T, custom_multiply_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies2");
    test_variable4< T, custom_multiply_multiple_variable3<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies3");

    test_variable1< T, custom_divide_variable<T> > (data, SIZE, var1int1, myTypeName + " variable divide");
    test_variable4< T, custom_divide_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable divides");
    test_variable4< T, custom_divide_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable divides2");
    
    test_variable4< T, custom_mixed_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable mixed");
    test_variable4< T, custom_mixed_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable mixed2");
    
    TestLoopsIntegerOnly<T>(data, var1int1, var1int2, var1int3, var1int4);
    
    std::string temp1( myTypeName + " loop invariant" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    double temp = 1.0;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) temp = (double)atof(argv[3]);
    
    TestLoops<int8_t>(temp);
    TestLoops<uint8_t>(temp);
    TestLoops<int16_t>(temp);
    TestLoops<uint16_t>(temp);
    TestLoops<int32_t>(temp);
    TestLoops<uint32_t>(temp);
    TestLoops<int64_t>(temp);
    TestLoops<uint64_t>(temp);
    TestLoops<float>(temp);
    TestLoops<double>(temp);
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
