/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to algebraic simplification applied to simple language defined types.


Assumptions:

    1) The compiler will simplify common algebraic and logical identities.
        aka algebraic simplification
        
        input + 0 ==> input
        input - 0 ==> input
        0 - input ==> -input
        -(-input) ==> input
        input * 1 ==> input
        input / 1 ==> input
        input | input ==> input        for integer types
        input & input ==> input        for integer types
        input | 0 ==> input            for integer types
        input ^ 0 ==> input            for integer types
        input & ~0 ==> input           for integer types
 
        input * 0 ==> 0
        input - input ==> 0
        input ^ input ==> 0            for integer types
        input & 0 ==> 0                for integer types
        input % 1 ==> 0                for integer types
        input == input ==> true        for integer types (NaNs complicate FloatingPoint)
        input <= input ==> true        for integer types (NaNs complicate FloatingPoint)
        input >= input ==> true        for integer types (NaNs complicate FloatingPoint)
        input != input ==> false       for integer types (NaNs complicate FloatingPoint)
        input > input ==> false        for integer types (NaNs complicate FloatingPoint)
        input < input ==> false        for integer types (NaNs complicate FloatingPoint)
        
    
    2) The compiler will simplify multiplication using the distributive property.
        And division where possible.
        See https://en.wikipedia.org/wiki/Distributive_property
        
        a*b + a*c + a*d + a*e                ==>    a*(b+c+d+e)
        b/a + c/a + d/a + e/a                ==>    (b+c+d+e)/a     for floating point types (integer may change result)
        a*c + a*d + a*e + b*c + b*d + b*e    ==>    (a + b) * (c + d + e)


    3) The compiler will simplify common 2 term algebraic identities.
        technically also a distributive property
        (yes, these do occur in the real world, usually as part of larger expressions)
        
        result = x*x + 2*y*x + y*y     ==>     result = (x + y) * (x + y)
        result = x*x - 2*y*x + y*y             result = (x - y) * (x - y)
        result = x*x - y*y                     result = (x + y) * (x - y)


    4) The compiler will simplify common 2 term logical identities.
        (yes, this does occur in the real world, usually as part of larger expressions)
        See https://en.wikipedia.org/wiki/De_Morgan%27s_laws
 
        (A | B) & ~(A & B)    ==>        (A ^ B)    for integer types
        ~(~A | ~B)            ==>        (A & B)    for integer types
        ~(~A & ~B)            ==>        (A | B)    for integer types





TODO - ccox - other random stuff found unoptimized in code:

    block = index >> 3;
    offset = index - (block << 3);      ==>        offset = index & 7;
 
    offset = index - ((index/8)*8);     ==>        offset = index % 8 = index & 7;
        Handled as mask_low in shift.cpp, technically a strength reduction

*/

/******************************************************************************/

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
// on 3Ghz desktop CPUs, 4000k iterations is about 1.0 seconds
int iterations = 4000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

// TODO - ccox - move these to shared tests

template <typename T>
    struct custom_multiply_distributive_variable_opt {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (input * (v1 + v2 + v3 + v4)); }
    };

/******************************************************************************/

template <typename T>
    struct custom_multiply_distributive_variable {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (input * v1 + input * v2 + input * v3 + input * v4); }
    };

/******************************************************************************/

template <typename T>
    struct custom_multiply_distributive_variable_opt2 {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return ((v1 + v2 + v3 + v4) * input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_multiply_distributive_variable2 {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (v1 * input + v2 * input + v3 * input + v4 * input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_multiply_distributive_variable_opt3 {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (input * (v1 - v2 + v3 - v4)); }
    };

/******************************************************************************/

template <typename T>
    struct custom_multiply_distributive_variable3 {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (input * v1 - input * v2 + input * v3 - input * v4); }
    };

/******************************************************************************/

template <typename T>
    struct custom_poly_distributive_variable_opt {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return ((input + v1) * (v2 + v3 + v4)); }
    };

/******************************************************************************/

template <typename T>
    struct custom_poly_distributive_variable {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (input * v2 + input * v3 + input * v4 + v1 * v2 + v1 * v3 + v1 * v4); }
    };

/******************************************************************************/

template <typename T>
    struct custom_divide_distributive_variable_opt {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return ((v1 + v2 + v3 + v4) / input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_divide_distributive_variable {
      static T do_shift(T input, T v1, T v2, T v3, T v4) { return (v1 / input + v2 / input + v3 / input + v4 / input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_add_opt {
      static T do_shift(T input1, T input2) { T temp = input1 + input2; return (temp*temp); }
    };
    
template <typename T>
    struct custom_2term_add {
      static T do_shift(T input1, T input2) { return (input1*input1 + 2*input1*input2 + input2*input2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_sub_opt {
      static T do_shift(T input1, T input2) { T temp = input1 - input2; return (temp*temp); }
    };
    
template <typename T>
    struct custom_2term_sub {
      static T do_shift(T input1, T input2) { return (input1*input1 - 2*input1*input2 + input2*input2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_addsub_opt {
      static T do_shift(T input1, T input2) {
        T temp1 = input1 - input2;
        T temp2 = input1 + input2;
        return (temp1*temp2); }
    };

template <typename T>
    struct custom_2term_addsub {
      static T do_shift(T input1, T input2) { return (input1*input1 - input2*input2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_xor_opt {
      static T do_shift(T input1, T input2) { return (input1 ^ input2); }
    };

template <typename T>
    struct custom_2term_xor {
      static T do_shift(T input1, T input2) { return ( (input1 | input2) & ~(input1 & input2) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_or_opt {
      static T do_shift(T input1, T input2) { return (input1 | input2); }
    };

template <typename T>
    struct custom_2term_or {
      static T do_shift(T input1, T input2) { return ( ~(~input1 & ~input2) ) ; }
    };

/******************************************************************************/

template <typename T>
    struct custom_2term_and_opt {
      static T do_shift(T input1, T input2) { return (input1 & input2); }
    };

template <typename T>
    struct custom_2term_and {
      static T do_shift(T input1, T input2) { return ( ~(~input1 | ~input2) ) ; }
    };

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_2term(T* first, int count, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    result += Shifter::do_shift( first[0], first[1] );
    for (int n = 1; n < count; ++n) {
        result += Shifter::do_shift( first[n-1], first[n] );
    }
    check_shifted_variable_sum<T, Shifter>(result, T(init_value));
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_constantS(T* first, int count, const std::string label) {
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

template <typename T, typename Shifter>
void test_variable4S(T* first, int count, T v1, T v2, T v3, T v4, const std::string label) {
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
void TestOneType( double temp ) {
    const bool isFloat = T(2.1) > T(2);     // darn - can't get this to work at compile time yet

    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];

    ::fill(data, data+SIZE, T(init_value));
    T var_1 = T(temp);
    T var_2 = var_1 * T(2.0);
    T var_3 = var_1 + T(2.0);
    T var_4 = var_1 + var_2 / var_3;

    test_constantS<T, custom_identity<T> >(data,SIZE,myTypeName + " copy");
    test_constantS<T, custom_add_zero<T> >(data,SIZE,myTypeName + " add zero");
    test_constantS<T, custom_sub_zero<T> >(data,SIZE,myTypeName + " subtract zero");
    test_constantS<T, custom_negate<T> >(data,SIZE,myTypeName + " negate");
    test_constantS<T, custom_negate_twice<T> >(data,SIZE,myTypeName + " negate twice");
    test_constantS<T, custom_zero_minus<T> >(data,SIZE,myTypeName + " zero minus");
    test_constantS<T, custom_times_one<T> >(data,SIZE,myTypeName + " times one");
    test_constantS<T, custom_divideby_one<T> >(data,SIZE,myTypeName + " divide by one");
    if (!isFloat) {
        test_constantS<T, custom_and_self<T> >(data,SIZE,myTypeName + " and self");
        test_constantS<T, custom_or_self<T> >(data,SIZE,myTypeName + " or self");
        test_constantS<T, custom_or_zero<T> >(data,SIZE,myTypeName + " or zero");
        test_constantS<T, custom_xor_zero<T> >(data,SIZE,myTypeName + " xor zero");
        test_constantS<T, custom_andnot_zero<T> >(data,SIZE,myTypeName + " andnot zero");
    }
    test_constantS<T, custom_algebra_mixed<T> >(data,SIZE,myTypeName + " mixed algebra");
    
    std::string temp1( myTypeName + " algebraic simplification");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_constantS<T, custom_zero<T> >(data,SIZE,myTypeName + " zero");
    test_constantS<T, custom_times_zero<T> >(data,SIZE,myTypeName + " times zero");
    test_constantS<T, custom_subtract_self<T> >(data,SIZE,myTypeName + " subtract self");
    if (!isFloat) {
        test_constantS<T, custom_mod_one<T> >(data,SIZE,myTypeName + " mod one");
        test_constantS<T, custom_equal_self<T> >(data,SIZE,myTypeName + " equal self");
        test_constantS<T, custom_notequal_self<T> >(data,SIZE,myTypeName + " not equal self");
        test_constantS<T, custom_greaterthan_self<T> >(data,SIZE,myTypeName + " greater than self");
        test_constantS<T, custom_lessthan_self<T> >(data,SIZE,myTypeName + " less than self");
        test_constantS<T, custom_greaterthanequal_self<T> >(data,SIZE,myTypeName + " greater than equal self");
        test_constantS<T, custom_lessthanequal_self<T> >(data,SIZE,myTypeName + " less than equal self");
        test_constantS<T, custom_xor_self<T> >(data,SIZE,myTypeName + " xor self");
        test_constantS<T, custom_and_zero<T> >(data,SIZE,myTypeName + " and zero");
    }
    test_constantS<T, custom_algebra_mixed_constant<T> >(data,SIZE,myTypeName + " mixed constant");
    
    std::string temp2( myTypeName + " algebraic simplification to constant");
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_variable4S<T, custom_multiply_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal");
    test_variable4S<T, custom_multiply_distributive_variable_opt2<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal2");
    test_variable4S<T, custom_multiply_distributive_variable_opt3<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal3");
    test_variable4S<T, custom_multiply_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive");
    test_variable4S<T, custom_multiply_distributive_variable2<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive2");
    test_variable4S<T, custom_multiply_distributive_variable3<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive3");
    test_variable4S<T, custom_poly_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " polynomial distributive optimal");
    test_variable4S<T, custom_poly_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " polynomial distributive");
    
    std::string temp3( myTypeName + " multiply distributive");
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // should not be optimized for integer, and really slow to test in integer
    if (isFloat) {
        test_variable4S<T, custom_divide_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " divide distributive optimal");
        test_variable4S<T, custom_divide_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " divide distributive");
        
        std::string temp5( myTypeName + " divide distributive");
        summarize(temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    }


    test_2term<T, custom_2term_add_opt<T> >(data, SIZE, myTypeName + " 2term add opt");
    test_2term<T, custom_2term_add<T> >(data, SIZE, myTypeName + " 2term add");
    test_2term<T, custom_2term_sub_opt<T> >(data, SIZE, myTypeName + " 2term sub opt");
    test_2term<T, custom_2term_sub<T> >(data, SIZE, myTypeName + " 2term sub");
    test_2term<T, custom_2term_addsub_opt<T> >(data, SIZE, myTypeName + " 2term addsub opt");
    test_2term<T, custom_2term_addsub<T> >(data, SIZE, myTypeName + " 2term addsub");
    if (!isFloat) {
        test_2term<T, custom_2term_xor_opt<T> >(data, SIZE, myTypeName + " 2term xor opt");
        test_2term<T, custom_2term_xor<T> >(data, SIZE, myTypeName + " 2term xor");
        test_2term<T, custom_2term_or_opt<T> >(data, SIZE, myTypeName + " 2term or opt");
        test_2term<T, custom_2term_or<T> >(data, SIZE, myTypeName + " 2term or");
        test_2term<T, custom_2term_and_opt<T> >(data, SIZE, myTypeName + " 2term and opt");
        test_2term<T, custom_2term_and<T> >(data, SIZE, myTypeName + " 2term and");
    }

    std::string temp4( myTypeName + " algebraic simplification 2term");
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template< typename T >
void TestOneTypeFloat( double temp ) {
    //const bool isFloat = T(2.1) > T(2);

    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];

    ::fill(data, data+SIZE, T(init_value));
    T var_1 = T(temp);
    T var_2 = var_1 * T(2.0);
    T var_3 = var_1 + T(2.0);
    T var_4 = var_1 + var_2 / var_3;

    test_constantS<T, custom_identity<T> >(data,SIZE,myTypeName + " copy");
    test_constantS<T, custom_add_zero<T> >(data,SIZE,myTypeName + " add zero");
    test_constantS<T, custom_sub_zero<T> >(data,SIZE,myTypeName + " subtract zero");
    test_constantS<T, custom_negate<T> >(data,SIZE,myTypeName + " negate");
    test_constantS<T, custom_negate_twice<T> >(data,SIZE,myTypeName + " negate twice");
    test_constantS<T, custom_zero_minus<T> >(data,SIZE,myTypeName + " zero minus");
    test_constantS<T, custom_times_one<T> >(data,SIZE,myTypeName + " times one");
    test_constantS<T, custom_divideby_one<T> >(data,SIZE,myTypeName + " divide by one");
    test_constantS<T, custom_algebra_mixed<T> >(data,SIZE,myTypeName + " mixed algebra");
    
    std::string temp1( myTypeName + " algebraic simplification");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_constantS<T, custom_zero<T> >(data,SIZE,myTypeName + " zero");
    test_constantS<T, custom_times_zero<T> >(data,SIZE,myTypeName + " times zero");
    test_constantS<T, custom_subtract_self<T> >(data,SIZE,myTypeName + " subtract self");
    test_constantS<T, custom_algebra_mixed_constant<T> >(data,SIZE,myTypeName + " mixed constant");
    
    std::string temp2( myTypeName + " algebraic simplification to constant");
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_variable4S<T, custom_multiply_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal");
    test_variable4S<T, custom_multiply_distributive_variable_opt2<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal2");
    test_variable4S<T, custom_multiply_distributive_variable_opt3<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive optimal3");
    test_variable4S<T, custom_multiply_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive");
    test_variable4S<T, custom_multiply_distributive_variable2<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive2");
    test_variable4S<T, custom_multiply_distributive_variable3<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " multiply distributive3");
    test_variable4S<T, custom_poly_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " polynomial distributive optimal");
    test_variable4S<T, custom_poly_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " polynomial distributive");
    
    std::string temp3( myTypeName + " multiply distributive");
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_variable4S<T, custom_divide_distributive_variable_opt<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " divide distributive optimal");
    test_variable4S<T, custom_divide_distributive_variable<T> >( data, SIZE, var_1, var_2, var_3, var_4, myTypeName + " divide distributive");
    
    std::string temp5( myTypeName + " divide distributive");
    summarize(temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_2term<T, custom_2term_add_opt<T> >(data, SIZE, myTypeName + " 2term add opt");
    test_2term<T, custom_2term_add<T> >(data, SIZE, myTypeName + " 2term add");
    test_2term<T, custom_2term_sub_opt<T> >(data, SIZE, myTypeName + " 2term sub opt");
    test_2term<T, custom_2term_sub<T> >(data, SIZE, myTypeName + " 2term sub");
    test_2term<T, custom_2term_addsub_opt<T> >(data, SIZE, myTypeName + " 2term addsub opt");
    test_2term<T, custom_2term_addsub<T> >(data, SIZE, myTypeName + " 2term addsub");

    std::string temp4( myTypeName + " algebraic simplification 2term");
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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


    TestOneType<int8_t>( temp );
    TestOneType<uint8_t>( temp );
    TestOneType<int16_t>( temp );
    TestOneType<uint16_t>( temp );
    TestOneType<int32_t>( temp );
    TestOneType<uint32_t>( temp );
    TestOneType<int64_t>( temp );
    TestOneType<uint64_t>( temp );

    TestOneTypeFloat<float>( temp );
    TestOneTypeFloat<double>( temp );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
