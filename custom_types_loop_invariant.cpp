/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test loop invariant code motion optimizations related to user defined types.

Assumptions:

    1) The compiler, where possible, will move redundant custom type calculations out of a loop.
        for (i = 0; i < N; ++i)                    temp = A + B + C + D;
            result = input[i] + A+B+C+D;    ==>    for (i = 0; i < N; ++i)
                                                     result = input[i] + temp;


*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 1000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

#include "custom_types.h"
#include "benchmark_shared_tests.h"

/******************************************************************************/

static std::deque<std::string> gLabels;

/******************************************************************************/

// TODO - move these back to shared tests
template <typename T, typename Shifter>
void test_variable1(T* first, int count, const T v1, const std::string label) {
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

// TODO - move these back to shared tests
template <typename T, typename Shifter>
void test_variable4(T* first, int count, const T v1, const T v2, const T v3, const T v4, const std::string label) {
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

typedef SingleItemClass<int32_t> Int1Class;
typedef TwoItemClass<int32_t> Int2Class;
typedef FourItemClass<int32_t> Int4Class;
typedef SixItemClass<int32_t> Int6Class;

typedef SingleItemClass<double> Double1Class;
typedef TwoItemClass<double> Double2Class;
typedef FourItemClass<double> Double4Class;
typedef SixItemClass<double> Double6Class;

/******************************************************************************/

namespace benchmark {

template<>
std::string getTypeName<Int1Class>() { return std::string("int1Class"); }

template<>
std::string getTypeName<Int2Class>() { return std::string("int2Class"); }

template<>
std::string getTypeName<Int4Class>() { return std::string("int4Class"); }

template<>
std::string getTypeName<Int6Class>() { return std::string("int6Class"); }

template<>
std::string getTypeName<Double1Class>() { return std::string("double1Class"); }

template<>
std::string getTypeName<Double2Class>() { return std::string("double2Class"); }

template<>
std::string getTypeName<Double4Class>() { return std::string("double4Class"); }

template<>
std::string getTypeName<Double6Class>() { return std::string("double6Class"); }

};

/******************************************************************************/

template< typename T >
void TestOneType(double temp)
{
    T data[SIZE];
    
    std::string myTypeName( getTypeName<T>() );
    gLabels.clear();

    ::fill(data, data+SIZE, T(init_value));
    T var1, var2, var3, var4;

    var1 = T(temp);
    var2 = var1 * T(2);
    var3 = var1 + T(2);
    var4 = var1 + var2 / var3;

    // test moving redundant calcs out of loop
    test_variable1< T, custom_add_variable<T> > (data, SIZE, var1, myTypeName + " variable add");
    test_variable4< T, custom_add_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable adds");
    test_variable4< T, custom_add_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable adds2");
    
    test_variable1< T, custom_sub_variable<T> > (data, SIZE, var1, myTypeName + " variable subtract");
    test_variable4< T, custom_sub_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable subtracts");
    test_variable4< T, custom_sub_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable subtracts2");
    
    test_variable1< T, custom_multiply_variable<T> > (data, SIZE, var1, myTypeName + " variable multiply");
    test_variable4< T, custom_multiply_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable multiplies");
    test_variable4< T, custom_multiply_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable multiplies2");
    test_variable4< T, custom_multiply_multiple_variable3<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable multiplies3");

    test_variable1< T, custom_divide_variable<T> > (data, SIZE, var1, myTypeName + " variable divide");
    //test_variable4< T, custom_divide_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable divides");   // not optimizable, slow
    test_variable4< T, custom_divide_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable divides2");
    
    test_variable4< T, custom_mixed_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable mixed");
    test_variable4< T, custom_mixed_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable mixed2");
    
    std::string temp1( myTypeName + " loop invariant" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

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

    int base_iterations = iterations;


    iterations = base_iterations;
    TestOneType<Int1Class>(temp);
    TestOneType<Int2Class>(temp);
    iterations = base_iterations / 8;
    TestOneType<Int4Class>(temp);
    TestOneType<Int6Class>(temp);


    iterations = base_iterations / 4;
    TestOneType<Double1Class>(temp);
    TestOneType<Double2Class>(temp);
    iterations = base_iterations / 12;
    TestOneType<Double4Class>(temp);
    TestOneType<Double6Class>(temp);


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
