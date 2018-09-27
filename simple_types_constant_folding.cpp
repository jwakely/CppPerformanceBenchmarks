/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to constant folding of simple language defined types.

Assumptions:

    1) The compiler will combine constant calculations into a single constant for simple types.
        aka constant folding
        result = A + B            ==>        result = constant
        result = A - B            ==>        result = constant
        result = A * B            ==>        result = constant
        result = A / B            ==>        result = constant
        result = A % B            ==>        result = constant    for integer types
        result = (A == B)         ==>        result = constant    for integer types
        result = (A != B)         ==>        result = constant    for integer types
        result = (A > B)          ==>        result = constant    for integer types
        result = (A < B)          ==>        result = constant    for integer types
        result = (A >= B)         ==>        result = constant    for integer types
        result = (A <= B)         ==>        result = constant    for integer types
        result = (A & B)          ==>        result = constant    for integer types
        result = (A | B)          ==>        result = constant    for integer types
        result = (A ^ B)          ==>        result = constant    for integer types
        
        result = input + A + B + C + D    ==>        result = input + (A+B+C+D)
        result = input - A - B - C - D    ==>        result = input - (A+B+C+D)
        result = input * A * B * C * D    ==>        result = input * (A*B*C*D)
        result = input + A * B * C * D    ==>        result = input + (A*B*C*D)
        result = ((((input/A) /B) /C) /D)    ==>     result = input / (A*B*C*D)
        result = input + (((A /B) /C) /D)    ==>     result = input + (A/B/C/D)
        result = input & A & B & C & D    ==>        result = input & (A&B&C&D)            for integer types
        result = input | A | B | C | D    ==>        result = input | (A|B|C|D)            for integer types
        result = input ^ A ^ B ^ C ^ D    ==>        result = input ^ (A^B^C^D)            for integer types


NOTE - In some cases, loop invariant code motion might move the constant calculation out of the inner loop,
    making it appear that the constants were folded.
    But in the constant result cases, we want the compiler to recognize the constant and move it out of the loop

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int base_iterations = 20000000;
int iterations = base_iterations;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;


/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_constant(T* first, int count, const std::string &label) {
    int i;
  
    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n] );
        }
        check_shifted_sum<T, Shifter>(result);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename T >
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];
    ::fill(data, data+SIZE, T(init_value));
    
    
    iterations = base_iterations;
    test_constant<T, custom_two<T> >(data,SIZE,myTypeName + " constant");
    test_constant<T, custom_add_constants<T> >(data,SIZE,myTypeName + " add constants");
    test_constant<T, custom_sub_constants<T> >(data,SIZE,myTypeName + " subtract constants");
    test_constant<T, custom_multiply_constants<T> >(data,SIZE,myTypeName + " multiply constants");
    test_constant<T, custom_divide_constants<T> >(data,SIZE,myTypeName + " divide constants");
    test_constant<T, custom_mod_constants<T> >(data,SIZE,myTypeName + " mod constants");
    test_constant<T, custom_equal_constants<T> >(data,SIZE,myTypeName + " equal constants");
    test_constant<T, custom_notequal_constants<T> >(data,SIZE,myTypeName + " notequal constants");
    test_constant<T, custom_greaterthan_constants<T> >(data,SIZE,myTypeName + " greater than constants");
    test_constant<T, custom_lessthan_constants<T> >(data,SIZE,myTypeName + " less than constants");
    test_constant<T, custom_greaterthanequal_constants<T> >(data,SIZE,myTypeName + " greater than equal constants");
    test_constant<T, custom_lessthanequal_constants<T> >(data,SIZE,myTypeName + " less than equal constants");
    test_constant<T, custom_and_constants<T> >(data,SIZE,myTypeName + " and constants");
    test_constant<T, custom_or_constants<T> >(data,SIZE,myTypeName + " or constants");
    test_constant<T, custom_xor_constants<T> >(data,SIZE,myTypeName + " xor constants");
    
    std::string temp1( myTypeName + " simple constant folding" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    iterations = base_iterations / 10;
    test_constant<T, custom_constant_add<T> >(data,SIZE,myTypeName + " constant add");
    test_constant<T, custom_multiple_constant_add<T> >(data,SIZE,myTypeName + " multiple constant adds");
    test_constant<T, custom_constant_sub<T> >(data,SIZE,myTypeName + " constant subtract");
    test_constant<T, custom_multiple_constant_sub<T> >(data,SIZE,myTypeName + " multiple constant subtracts");
    test_constant<T, custom_constant_multiply<T> >(data,SIZE,myTypeName + " constant multiply");
    test_constant<T, custom_multiple_constant_multiply<T> >(data,SIZE,myTypeName + " multiple constant multiplies");
    test_constant<T, custom_multiple_constant_multiply2<T> >(data,SIZE,myTypeName + " multiple constant multiply2");
    test_constant<T, custom_constant_divide<T> >(data,SIZE,myTypeName + " constant divide");
    test_constant<T, custom_multiple_constant_divide<T> >(data,SIZE,myTypeName + " multiple constant divides");
    test_constant<T, custom_multiple_constant_divide2<T> >(data,SIZE,myTypeName + " multiple constant divide2");
    test_constant<T, custom_multiple_constant_mixed<T> >(data,SIZE,myTypeName + " multiple constant mixed");
    test_constant<T, custom_constant_and<T> >(data,SIZE,myTypeName + " constant and");
    test_constant<T, custom_multiple_constant_and<T> >(data,SIZE,myTypeName + " multiple constant and");
    test_constant<T, custom_constant_or<T> >(data,SIZE,myTypeName + " constant or");
    test_constant<T, custom_multiple_constant_or<T> >(data,SIZE,myTypeName + " multiple constant or");
    test_constant<T, custom_constant_xor<T> >(data,SIZE,myTypeName + " constant xor");
    test_constant<T, custom_multiple_constant_xor<T> >(data,SIZE,myTypeName + " multiple constant xor");

    std::string temp2( myTypeName + " constant folding" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations;
}

/******************************************************************************/

template< typename T >
void TestOneTypeFloating()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];
    ::fill(data, data+SIZE, T(init_value));
    
    
    iterations = base_iterations;
    test_constant<T, custom_two<T> >(data,SIZE,myTypeName + " constant");
    test_constant<T, custom_add_constants<T> >(data,SIZE,myTypeName + " add constants");
    test_constant<T, custom_sub_constants<T> >(data,SIZE,myTypeName + " subtract constants");
    test_constant<T, custom_multiply_constants<T> >(data,SIZE,myTypeName + " multiply constants");
    test_constant<T, custom_divide_constants<T> >(data,SIZE,myTypeName + " divide constants");
    test_constant<T, custom_equal_constants<T> >(data,SIZE,myTypeName + " equal constants");
    test_constant<T, custom_notequal_constants<T> >(data,SIZE,myTypeName + " notequal constants");
    test_constant<T, custom_greaterthan_constants<T> >(data,SIZE,myTypeName + " greater than constants");
    test_constant<T, custom_lessthan_constants<T> >(data,SIZE,myTypeName + " less than constants");
    test_constant<T, custom_greaterthanequal_constants<T> >(data,SIZE,myTypeName + " greater than equal constants");
    test_constant<T, custom_lessthanequal_constants<T> >(data,SIZE,myTypeName + " less than equal constants");
    
    std::string temp1( myTypeName + " simple constant folding" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    iterations = base_iterations / 10;
    test_constant<T, custom_constant_add<T> >(data,SIZE,myTypeName + " constant add");
    test_constant<T, custom_multiple_constant_add<T> >(data,SIZE,myTypeName + " multiple constant adds");
    test_constant<T, custom_constant_sub<T> >(data,SIZE,myTypeName + " constant subtract");
    test_constant<T, custom_multiple_constant_sub<T> >(data,SIZE,myTypeName + " multiple constant subtracts");
    test_constant<T, custom_constant_multiply<T> >(data,SIZE,myTypeName + " constant multiply");
    test_constant<T, custom_multiple_constant_multiply<T> >(data,SIZE,myTypeName + " multiple constant multiplies");
    test_constant<T, custom_multiple_constant_multiply2<T> >(data,SIZE,myTypeName + " multiple constant multiply2");
    test_constant<T, custom_constant_divide<T> >(data,SIZE,myTypeName + " constant divide");
    test_constant<T, custom_multiple_constant_divide<T> >(data,SIZE,myTypeName + " multiple constant divides");
    test_constant<T, custom_multiple_constant_divide2<T> >(data,SIZE,myTypeName + " multiple constant divide2");
    test_constant<T, custom_multiple_constant_mixed<T> >(data,SIZE,myTypeName + " multiple constant mixed");

    std::string temp2( myTypeName + " constant folding" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    iterations = base_iterations;
}

/******************************************************************************/


int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) base_iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);

    
    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    TestOneType<int64_t>();
    TestOneType<uint64_t>();

    TestOneTypeFloating<float>();
    TestOneTypeFloating<double>();

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
