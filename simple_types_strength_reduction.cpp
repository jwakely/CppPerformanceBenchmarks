/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )



Goal:  Test compiler optimizations related to strength reduction.


Assumptions:

    1) The compiler will simplify integer multiplication by a power of 2
        when the simplification would be faster.
        y = x * 2;        ==>        y = x + x;
                                 OR  y = x << 1;
        y = x * 8;        ==>        y = x << 3;

    2) The compiler will simplify integer division by a power of 2
        when the simplification would be faster.
        ( required sign oddity from C99: a = ((a / b) * b) + (a % b) )
        y = (unsigned)x / 8;      ==>        y = x >> 3;
        y = (signed)x / 8;        ==>        y = (x<0) ? -((-x) >> 3) : (x >> 3);

    3) The compiler will simplify integer remainder by a power of 2
        when the simplification would be faster.
        ( required sign oddity from C99: a = ((a / b) * b) + (a % b) )
        y = (unsigned)x % 8;      ==>        y = x & 7;
        y = (signed)x % 8;        ==>        t = x & 7; y = ((x<0)&&(t!=0))?-(8-t):t;

    4) The compiler will simplify integer multiplication by a constant
        when the simplification would be faster.
        Different CPUs will have a different complexity cutoff, and sometimes
        the more complex approach can pipeline well to keep other functional units busy.
        y = x * 63;        ==>        y = (x << 6) - x;
        y = x * 67;        ==>        y = (x << 6) + (x << 2) - x;
        y = x * 83;        ==>        y = (x << 6) + (x << 4) + (x << 2) - x;

    5) The compiler will simplify combinations of shifts, adds, subtracts, and multiplies
        when replacement by a single multiplication would be faster.
        This will probably involve algebraic simplification.
        y = (x << 6) + (x << 5) + (x << 4) + (x << 3) + (x << 2) + (x << 1)     ==>        y = x * 126
        y = (x << 6) - (x << 5) - (x << 4) - (x << 3) - (x << 2) - (x << 1)     ==>        y = x * 2
        y = (x << 6) - (x << 5) + (x << 4) - (x << 3) + (x << 2) - (x << 1)     ==>        y = x * 42
        y = (x * 64) + (x * 32) + (x * 16) + (x * 8) + (x * 4) + (x * 2)        ==>        y = x * 126




NOTE: Some problems seen here appear to be due to bugs in compiler constant propagation and algebraic simplification.


NOTE: integer division/modulo by an arbitrary constant is covered in divide.cpp

NOTE: algebraic identities are covered in simple_types_algebraic_simplification.cpp

NOTE: induction variable manipulation is covered in loop_induction.cpp



TODO - multistep multiplies (x * 5 * 9) ==> t = ((x << 2) + x); result = ((t << 3) + t)
    45 = 5 * 9 = (4+1) * (8+1) = 32 + 8 + 4 + 1
    Different CPUs will require different approaches.

TODO - what are valid floating point strength reductions?
    sqrt(X) = pow(x,0.5) ?
    x*x = pow(x,2) ?

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <string>
#include <iostream>
#include "benchmark_algorithms.h"
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// This constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 4000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 63.0;

/******************************************************************************/

static std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

// TODO - put these into shared tests
template <typename T, const int shift>
    struct shift_right {
      static T do_shift(T input) { return (input >> shift); }
    };

/******************************************************************************/

template <typename T, const int shift>
    struct shift_left {
      static T do_shift(T input) { return (input << shift); }
    };

/******************************************************************************/

template <typename T, const int divisor>
    struct custom_divide {
      static T do_shift(T input) { return (input / divisor); }
    };

/******************************************************************************/

template <typename T, const int factor>
    struct custom_multiply {
      static T do_shift(T input) { return (input * factor); }
    };

/******************************************************************************/

// this will usually be faster than a multiply
template <typename T>
    struct custom_multiply_shiftadd_63 {
      static T do_shift(T input) { return ((input << 6) - input); }
    };

/******************************************************************************/

// this may be slower than a multiply, but it depends on the CPU, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_shiftadd_67 {
      static T do_shift(T input) { return ((input << 6) + (input << 2) - input); }
    };

/******************************************************************************/

// this will almost always be slower than a single multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_muladd_67 {
      static T do_shift(T input) { return ((input * 64) + (input * 4) - input); }
    };

/******************************************************************************/

// this will usually be slower than a multiply, but it depends on the CPU, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_shiftadd_83 {
      static T do_shift(T input) { return ((input << 6) + (input << 4) + (input << 2) - input); }
    };

/******************************************************************************/

// this will almost always be slower than a single multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_muladd_83 {
      static T do_shift(T input) { return ((input * 64) + (input * 16) + (input * 4) - input); }
    };

/******************************************************************************/

// this will almost always be slower than a multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_shiftadd_126 {
      static T do_shift(T input) { return ((input << 6) + (input << 5) + (input << 4) + (input << 3) + (input << 2) + (input << 1) ); }
    };

/******************************************************************************/

// this will almost always be slower than a single multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_muladd_126 {
      static T do_shift(T input) { return ((input * 64) + (input * 32) + (input * 16) + (input * 8) + (input * 4) + (input * 2) ); }
    };

/******************************************************************************/

// this will almost always be slower than a multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_shiftadd_2 {
      static T do_shift(T input) { return ((input << 6) - (input << 5) - (input << 4) - (input << 3) - (input << 2) - (input << 1) ); }
    };

/******************************************************************************/

// this will almost always be slower than a multiply, unless algebraic simplification reduces it
template <typename T>
    struct custom_multiply_shiftadd_42 {
      static T do_shift(T input) { return ((input << 6) - (input << 5) + (input << 4) - (input << 3) + (input << 2) - (input << 1) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_constant_addself {
      static T do_shift(T input) { return (input + input); }
    };

/****************************************************************1**************/

template <typename T>
    struct custom_multiply_inline2 {
      static T do_shift(T input) { return (input * 2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_divide_inline2 {
      static T do_shift(T input) { return (input / 2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline2 {
      static T do_shift(T input) { return (input % 2); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline4 {
      static T do_shift(T input) { return (input % 4); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline8 {
      static T do_shift(T input) { return (input % 8); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline16 {
      static T do_shift(T input) { return (input % 16); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline32 {
      static T do_shift(T input) { return (input % 32); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline64 {
      static T do_shift(T input) { return (input % 64); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline128 {
      static T do_shift(T input) { return (input % 128); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline256 {
      static T do_shift(T input) { return (input % 256); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_inline1024 {
      static T do_shift(T input) { return (input % 1024); }
    };

/******************************************************************************/

template <typename T, const int val>
    struct and_constant {
      static T do_shift(T input) { return (input & val); }
    };

/******************************************************************************/

template <typename T, const int val>
    struct and_remainder {
      static T do_shift(T input) {
        if (isSigned<T>()) {
            auto temp = (input & val);
            return (input < 0 && temp != 0) ? -((val+1)-temp) : temp;
        }
        else
            return (input & val);
      }
    };

/******************************************************************************/

template <typename T, const int val>
    struct and_remainder2 {
      static T do_shift(T input) {
        auto result = (input & val);
        if (isSigned<T>() && (input < 0) && (result != 0))
            result = -((val+1)-result);
        return result;
     }
    };

/******************************************************************************/

template <typename T, const int shift>
    struct shift_divide_toward_zero {
     static T do_shift(T input) {
        if (isSigned<T>() && (input < 0))
            return -((-input)>>shift);
        else
            return (input >> shift);
     }
    };

/******************************************************************************/

template <typename T, const int divisor>
    struct custom_remainder {
      static T do_shift(T input) { return (input % divisor); }
    };

/******************************************************************************/

template <typename T>
    struct custom_remainder_variable {
      static T do_shift(T input, const int v1) { return (input % v1); }
    };

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Shifter>
void test_constant(const T* first, const int count, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
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
void test_variable1(const T* first, const int count, const T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
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
/******************************************************************************/

template<typename T, const T value>
void UnitTestValue()
{
    std::string myTypeName( getTypeName<T>() );
    
    
    if ( shift_left<T, 1>::do_shift(value) != T(value*2) )
        std::cout << myTypeName << " shift_left multiply by 2 failed with " << value << "\n";
    
    if ( custom_constant_addself<T>::do_shift(value) != T(value*2) )
        std::cout << myTypeName << " addself multiply by 2 failed with " << value << "\n";
    
    if ( custom_multiply_inline2<T>::do_shift(value) != T(value*2) )
        std::cout << myTypeName << " inline multiply by 2 failed with " << value << "\n";
    
    if ( custom_multiply_shiftadd_2<T>::do_shift(value) != T(value*2) )
        std::cout << myTypeName << " shift_add multiply by 2 failed with " << value << "\n";
    
    if ( custom_multiply_variable<T>::do_shift(value,2) != T(value*2) )
        std::cout << myTypeName << " variable multiply by 2 failed with " << value << "\n";
    
    if ( custom_multiply<T,2>::do_shift(value) != T(value*2) )
        std::cout << myTypeName << " constant multiply by 2 failed with " << value << "\n";
    
    
    if ( shift_left<T, 3>::do_shift(value) != T(value*8) )
        std::cout << myTypeName << " shift_left multiply by 8 failed with " << value << "\n";
    
    if ( custom_multiply_variable<T>::do_shift(value,8) != T(value*8) )
        std::cout << myTypeName << " variable multiply by 8 failed with " << value << "\n";
    
    if ( custom_multiply<T,8>::do_shift(value) != T(value*8) )
        std::cout << myTypeName << " constant multiply by 8 failed with " << value << "\n";
    
    
    if ( shift_left<T, 6>::do_shift(value) != T(value*64) )
        std::cout << myTypeName << " shift_left multiply by 64 failed with " << value << "\n";
    
    if ( custom_multiply_variable<T>::do_shift(value,64) != T(value*64) )
        std::cout << myTypeName << " variable multiply by 64 failed with " << value << "\n";
    
    if ( custom_multiply<T,64>::do_shift(value) != T(value*64) )
        std::cout << myTypeName << " constant multiply by 64 failed with " << value << "\n";
    
    
    if ( custom_multiply_shiftadd_42<T>::do_shift(value) != T(value*42) )
        std::cout << myTypeName << " shift_add multiply by 42 failed with " << value << "\n";
    
    if ( custom_multiply_shiftadd_63<T>::do_shift(value) != T(value*63) )
        std::cout << myTypeName << " shift_add multiply by 63 failed with " << value << "\n";
    
    if ( custom_multiply_shiftadd_67<T>::do_shift(value) != T(value*67) )
        std::cout << myTypeName << " shift_add multiply by 67 failed with " << value << "\n";
    
    if ( custom_multiply_shiftadd_83<T>::do_shift(value) != T(value*83) )
        std::cout << myTypeName << " shift_add multiply by 83 failed with " << value << "\n";
    
    if ( custom_multiply_shiftadd_126<T>::do_shift(value) != T(value*126) )
        std::cout << myTypeName << " shift_add multiply by 126 failed with " << value << "\n";
    
    if ( custom_multiply_muladd_67<T>::do_shift(value) != T(value*67) )
        std::cout << myTypeName << " mul_add multiply by 67 failed with " << value << "\n";
    
    if ( custom_multiply_muladd_83<T>::do_shift(value) != T(value*83) )
        std::cout << myTypeName << " mul_add multiply by 83 failed with " << value << "\n";
    
    if ( custom_multiply_muladd_126<T>::do_shift(value) != T(value*126) )
        std::cout << myTypeName << " mul_add multiply by 126 failed with " << value << "\n";
    
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && shift_right<T, 1>::do_shift(value) != T(value/2) )
        std::cout << myTypeName << " shift_right divide by 2 failed with " << value << "\n";
    
    if ( shift_divide_toward_zero<T, 1>::do_shift(value) != T(value/2) )
        std::cout << myTypeName << " shift_divide_toward_zero divide by 2 failed with " << value << "\n";
    
    if ( custom_divide_inline2<T>::do_shift(value) != T(value/2) )
        std::cout << myTypeName << " inline divide by 2 failed with " << value << "\n";
    
    if ( custom_divide_variable<T>::do_shift(value,2) != T(value/2) )
        std::cout << myTypeName << " variable divide by 2 failed with " << value << "\n";
    
    if ( custom_divide<T,2>::do_shift(value) != T(value/2) )
        std::cout << myTypeName << " constant divide by 2 failed with " << value << "\n";
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && shift_right<T, 3>::do_shift(value) != T(value/8) )
        std::cout << myTypeName << " shift_right divide by 8 failed with " << value << "\n";
    
    if ( shift_divide_toward_zero<T, 3>::do_shift(value) != T(value/8) )
        std::cout << myTypeName << " shift_divide_toward_zero divide by 8 failed with " << value << "\n";
    
    if ( custom_divide_variable<T>::do_shift(value,8) != T(value/8) )
        std::cout << myTypeName << " variable divide by 8 failed with " << value << "\n";
    
    if ( custom_divide<T,8>::do_shift(value) != T(value/8) )
        std::cout << myTypeName << " constant divide by 8 failed with " << value << "\n";
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && shift_right<T, 6>::do_shift(value) != T(value/64) )
        std::cout << myTypeName << " shift_right divide by 64 failed with " << value << "\n";
    
    if ( shift_divide_toward_zero<T, 6>::do_shift(value) != T(value/64) )
        std::cout << myTypeName << " shift_divide_toward_zero divide by 64 failed with " << value << "\n";
    
    if ( custom_divide_variable<T>::do_shift(value,64) != T(value/64) )
        std::cout << myTypeName << " variable divide by 64 failed with " << value << "\n";
    
    if ( custom_divide<T,64>::do_shift(value) != T(value/64) )
        std::cout << myTypeName << " constant divide by 64 failed with " << value << "\n";



    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 1>::do_shift(value) != T(value%2) )
        std::cout << myTypeName << " and_constant remainder by 2 failed with " << value << "\n";
    
    if ( and_remainder<T, 1>::do_shift(value) != T(value%2) )
        std::cout << myTypeName << " and_remainder by 2 failed with " << value << "\n";
    
    if ( and_remainder2<T, 1>::do_shift(value) != T(value%2) )
        std::cout << myTypeName << " and_remainder2 by 2 failed with " << value << "\n";
    
    if ( custom_remainder_inline2<T>::do_shift(value) != T(value%2) )
        std::cout << myTypeName << " inline remainder by 2 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,2) != T(value%2) )
        std::cout << myTypeName << " variable remainder by 2 failed with " << value << "\n";
    
    if ( custom_remainder<T,2>::do_shift(value) != T(value%2) )
        std::cout << myTypeName << " constant remainder by 2 failed with " << value << "\n";
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 3>::do_shift(value) != T(value%4) )
        std::cout << myTypeName << " and_constant remainder by 4 failed with " << value << "\n";
    
    if ( and_remainder<T, 3>::do_shift(value) != T(value%4) )
        std::cout << myTypeName << " and_remainder by 4 failed with " << value << "\n";
    
    if ( and_remainder2<T, 3>::do_shift(value) != T(value%4) )
        std::cout << myTypeName << " and_remainder2 by 4 failed with " << value << "\n";
    
    if ( custom_remainder_inline4<T>::do_shift(value) != T(value%4) )
        std::cout << myTypeName << " inline remainder by 4 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,4) != T(value%4) )
        std::cout << myTypeName << " variable remainder by 4 failed with " << value << "\n";
    
    if ( custom_remainder<T,4>::do_shift(value) != T(value%4) )
        std::cout << myTypeName << " constant remainder by 4 failed with " << value << "\n";

    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 7>::do_shift(value) != T(value%8) )
        std::cout << myTypeName << " and_constant remainder by 8 failed with " << value << "\n";
    
    if ( and_remainder<T, 7>::do_shift(value) != T(value%8) )
        std::cout << myTypeName << " and_remainder by 8 failed with " << value << "\n";
    
    if ( and_remainder2<T, 7>::do_shift(value) != T(value%8) )
        std::cout << myTypeName << " and_remainder2 by 8 failed with " << value << "\n";
    
    if ( custom_remainder_inline8<T>::do_shift(value) != T(value%8) )
        std::cout << myTypeName << " inline remainder by 8 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,8) != T(value%8) )
        std::cout << myTypeName << " variable remainder by 8 failed with " << value << "\n";
    
    if ( custom_remainder<T,8>::do_shift(value) != T(value%8) )
        std::cout << myTypeName << " constant remainder by 8 failed with " << value << "\n";
    
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 15>::do_shift(value) != T(value%16) )
        std::cout << myTypeName << " and_constant remainder by 16 failed with " << value << "\n";
    
    if ( and_remainder<T, 15>::do_shift(value) != T(value%16) )
        std::cout << myTypeName << " and_remainder by 16 failed with " << value << "\n";
    
    if ( and_remainder2<T, 15>::do_shift(value) != T(value%16) )
        std::cout << myTypeName << " and_remainder2 by 16 failed with " << value << "\n";
    
    if ( custom_remainder_inline16<T>::do_shift(value) != T(value%16) )
        std::cout << myTypeName << " inline remainder by 16 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,16) != T(value%16) )
        std::cout << myTypeName << " variable remainder by 16 failed with " << value << "\n";
    
    if ( custom_remainder<T,16>::do_shift(value) != T(value%16) )
        std::cout << myTypeName << " constant remainder by 16 failed with " << value << "\n";
    
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 31>::do_shift(value) != T(value%32) )
        std::cout << myTypeName << " and_constant remainder by 32 failed with " << value << "\n";
    
    if ( and_remainder<T, 31>::do_shift(value) != T(value%32) )
        std::cout << myTypeName << " and_remainder by 32 failed with " << value << "\n";
    
    if ( and_remainder2<T, 31>::do_shift(value) != T(value%32) )
        std::cout << myTypeName << " and_remainder2 by 32 failed with " << value << "\n";
    
    if ( custom_remainder_inline32<T>::do_shift(value) != T(value%32) )
        std::cout << myTypeName << " inline remainder by 32 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,32) != T(value%32) )
        std::cout << myTypeName << " variable remainder by 32 failed with " << value << "\n";
    
    if ( custom_remainder<T,32>::do_shift(value) != T(value%32) )
        std::cout << myTypeName << " constant remainder by 32 failed with " << value << "\n";
    
    
    
    /*  Known to fail for negative inputs - used as baseline for performance */
    if ( !isSigned<T>() && and_constant<T, 63>::do_shift(value) != T(value%64) )
        std::cout << myTypeName << " and_constant remainder by 64 failed with " << value << "\n";
    
    if ( and_remainder<T, 63>::do_shift(value) != T(value%64) )
        std::cout << myTypeName << " and_remainder by 64 failed with " << value << "\n";
    
    if ( and_remainder2<T, 63>::do_shift(value) != T(value%64) )
        std::cout << myTypeName << " and_remainder2 by 64 failed with " << value << "\n";
    
    if ( custom_remainder_inline64<T>::do_shift(value) != T(value%64) )
        std::cout << myTypeName << " inline remainder by 64 failed with " << value << "\n";
    
    if ( custom_remainder_variable<T>::do_shift(value,64) != T(value%64) )
        std::cout << myTypeName << " variable remainder by 64 failed with " << value << "\n";
    
    if ( custom_remainder<T,64>::do_shift(value) != T(value%64) )
        std::cout << myTypeName << " constant remainder by 64 failed with " << value << "\n";


    
    if (sizeof(T) > 1) {

        /*  Known to fail for negative inputs - used as baseline for performance */
        if ( !isSigned<T>() && and_constant<T, 127>::do_shift(value) != T(value%128) )
            std::cout << myTypeName << " and_constant remainder by 128 failed with " << value << "\n";

        if ( and_remainder<T, 127>::do_shift(value) != T(value%128) )
            std::cout << myTypeName << " and_remainder by 128 failed with " << value << "\n";

        if ( and_remainder2<T, 127>::do_shift(value) != T(value%128) )
            std::cout << myTypeName << " and_remainder2 by 128 failed with " << value << "\n";

        if ( custom_remainder_inline128<T>::do_shift(value) != T(value%128) )
            std::cout << myTypeName << " inline remainder by 128 failed with " << value << "\n";

        if ( custom_remainder_variable<T>::do_shift(value,128) != T(value%128) )
            std::cout << myTypeName << " variable remainder by 128 failed with " << value << "\n";

        if ( custom_remainder<T,128>::do_shift(value) != T(value%128) )
            std::cout << myTypeName << " constant remainder by 128 failed with " << value << "\n";


        /*  Known to fail for negative inputs - used as baseline for performance */
        if ( !isSigned<T>() && and_constant<T, 255>::do_shift(value) != T(value%256) )
            std::cout << myTypeName << " and_constant remainder by 256 failed with " << value << "\n";
        
        if ( and_remainder<T, 255>::do_shift(value) != T(value%256) )
            std::cout << myTypeName << " and_remainder by 256 failed with " << value << "\n";
        
        if ( and_remainder2<T, 255>::do_shift(value) != T(value%256) )
            std::cout << myTypeName << " and_remainder2 by 256 failed with " << value << "\n";
        
        if ( custom_remainder_inline256<T>::do_shift(value) != T(value%256) )
            std::cout << myTypeName << " inline remainder by 256 failed with " << value << "\n";
        
        if ( custom_remainder_variable<T>::do_shift(value,256) != T(value%256) )
            std::cout << myTypeName << " variable remainder by 256 failed with " << value << "\n";
        
        if ( custom_remainder<T,256>::do_shift(value) != T(value%256) )
            std::cout << myTypeName << " constant remainder by 256 failed with " << value << "\n";
    
    
    
        /*  Known to fail for negative inputs - used as baseline for performance */
        if ( !isSigned<T>() && and_constant<T, 1023>::do_shift(value) != T(value%1024) )
            std::cout << myTypeName << " and_constant remainder by 1024 failed with " << value << "\n";
        
        if ( and_remainder<T, 1023>::do_shift(value) != T(value%1024) )
            std::cout << myTypeName << " and_remainder by 1024 failed with " << value << "\n";
        
        if ( and_remainder2<T, 1023>::do_shift(value) != T(value%1024) )
            std::cout << myTypeName << " and_remainder2 by 1024 failed with " << value << "\n";
        
        if ( custom_remainder_inline1024<T>::do_shift(value) != T(value%1024) )
            std::cout << myTypeName << " inline remainder by 1024 failed with " << value << "\n";
        
        if ( custom_remainder_variable<T>::do_shift(value,1024) != T(value%1024) )
            std::cout << myTypeName << " variable remainder by 1024 failed with " << value << "\n";
        
        if ( custom_remainder<T,1024>::do_shift(value) != T(value%1024) )
            std::cout << myTypeName << " constant remainder by 1024 failed with " << value << "\n";

    }

}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];
    
    ::fill(data, data+SIZE, T(init_value));


    UnitTestValue<T,T(0)>();
    UnitTestValue<T,T(1)>();
    UnitTestValue<T,T(2)>();
    UnitTestValue<T,T(6)>();
    UnitTestValue<T,T(7)>();
    UnitTestValue<T,T(8)>();
    UnitTestValue<T,T(31)>();
    UnitTestValue<T,T(42)>();
    UnitTestValue<T,T(64)>();
    UnitTestValue<T,T(85)>();
    UnitTestValue<T,T(127)>();
    
    if (isSigned<T>()) {
        UnitTestValue<T,T(-1)>();
        UnitTestValue<T,T(-2)>();
        UnitTestValue<T,T(-6)>();
        UnitTestValue<T,T(-7)>();
        UnitTestValue<T,T(-8)>();
        UnitTestValue<T,T(-31)>();
        UnitTestValue<T,T(-42)>();
        UnitTestValue<T,T(-64)>();
        UnitTestValue<T,T(-85)>();
        UnitTestValue<T,T(-127)>();
    }


    test_constant<T, shift_left<T, 1> >(data,SIZE, myTypeName + " shift left by 1");
    test_constant<T, custom_constant_addself<T> >(data,SIZE, myTypeName + " add self");
    test_constant<T, custom_multiply_inline2<T> >(data,SIZE, myTypeName + " multiply by inline 2");
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 2, myTypeName + " multiply by variable 2");
    test_constant<T, custom_multiply<T, 2> >(data,SIZE, myTypeName + " multiply by constant 2");
    test_constant<T, custom_multiply_shiftadd_2<T> >(data,SIZE, myTypeName + " shift_add by 2");

    test_constant<T, shift_left<T, 3> >(data,SIZE, myTypeName + " shift left by 3");
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 8, myTypeName + " multiply by variable 8");
    test_constant<T, custom_multiply<T, 8> >(data,SIZE, myTypeName + " multiply by constant 8");

    test_constant<T, shift_left<T, 6> >(data,SIZE, myTypeName + " shift left by 6");
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 64, myTypeName + " multiply by variable 64");
    test_constant<T, custom_multiply<T, 64> >(data,SIZE, myTypeName + " multiply by constant 64");
    
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 42, myTypeName + " multiply by variable 42");
    test_constant<T, custom_multiply<T, 42> >(data,SIZE, myTypeName + " multiply by constant 42");
    test_constant<T, custom_multiply_shiftadd_42<T> >(data,SIZE, myTypeName + " shift_add by 42");
    
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 63, myTypeName + " multiply by variable 63");
    test_constant<T, custom_multiply<T, 63> >(data,SIZE, myTypeName + " multiply by constant 63");
    test_constant<T, custom_multiply_shiftadd_63<T> >(data,SIZE, myTypeName + " shift_add by 63");
    
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 67, myTypeName + " multiply by variable 67");
    test_constant<T, custom_multiply<T, 67> >(data,SIZE, myTypeName + " multiply by constant 67");
    test_constant<T, custom_multiply_shiftadd_67<T> >(data,SIZE, myTypeName + " shift_add by 67");
    test_constant<T, custom_multiply_muladd_67<T> >(data,SIZE, myTypeName + " mul_add by 67");
    
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 83, myTypeName + " multiply by variable 83");
    test_constant<T, custom_multiply<T, 83> >(data,SIZE, myTypeName + " multiply by constant 83");
    test_constant<T, custom_multiply_shiftadd_83<T> >(data,SIZE, myTypeName + " shift_add by 83");
    test_constant<T, custom_multiply_muladd_83<T> >(data,SIZE, myTypeName + " mul_add by 83");
    
    test_variable1<T, custom_multiply_variable<T> >(data,SIZE, 126, myTypeName + " multiply by variable 126");
    test_constant<T, custom_multiply<T, 126> >(data,SIZE, myTypeName + " multiply by constant 126");
    test_constant<T, custom_multiply_shiftadd_126<T> >(data,SIZE, myTypeName + " shift_add by 126");
    test_constant<T, custom_multiply_muladd_126<T> >(data,SIZE, myTypeName + " mul_add by 126");

    std::string temp1( myTypeName + " strength reduction multiply");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    
    test_constant<T, shift_right<T, 1> >(data,SIZE, myTypeName + " shift right by 1");
    test_constant<T, shift_divide_toward_zero<T, 1> >(data,SIZE, myTypeName + " shift_divide_toward_zero by 1");
    test_constant<T, custom_divide_inline2<T> >(data,SIZE, myTypeName + " divide by inline 2");
    test_variable1<T, custom_divide_variable<T> >(data,SIZE, 2, myTypeName + " divide by variable 2");
    test_constant<T, custom_divide<T, 2> >(data,SIZE, myTypeName + " divide by constant 2");
    
    test_constant<T, shift_right<T, 3> >(data,SIZE, myTypeName + " shift right by 3");
    test_constant<T, shift_divide_toward_zero<T, 3> >(data,SIZE, myTypeName + " shift_divide_toward_zero by 3");
    test_variable1<T, custom_divide_variable<T> >(data,SIZE, 8, myTypeName + " divide by variable 8");
    test_constant<T, custom_divide<T, 8> >(data,SIZE, myTypeName + " divide by constant 8");
    
    test_constant<T, shift_right<T, 6> >(data,SIZE, myTypeName + " shift right by 6");
    test_constant<T, shift_divide_toward_zero<T, 6> >(data,SIZE, myTypeName + " shift_divide_toward_zero by 6");
    test_variable1<T, custom_divide_variable<T> >(data,SIZE, 64, myTypeName + " divide by variable 64");
    test_constant<T, custom_divide<T, 64> >(data,SIZE, myTypeName + " divide by constant 64");

    std::string temp2( myTypeName + " strength reduction divide");
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_constant<T, and_constant<T, 1> >(data,SIZE, myTypeName + " and 1");
    test_constant<T, and_remainder<T, 1> >(data,SIZE, myTypeName + " and_remainder 1");
    test_constant<T, and_remainder2<T, 1> >(data,SIZE, myTypeName + " and_remainder2 1");
    test_constant<T, custom_remainder_inline2<T> >(data,SIZE, myTypeName + " remainder by inline 2");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 2, myTypeName + " remainder by variable 2");
    test_constant<T, custom_remainder<T, 2> >(data,SIZE, myTypeName + " remainder by constant 2");

    test_constant<T, and_constant<T, 3> >(data,SIZE, myTypeName + " and 3");
    test_constant<T, and_remainder<T, 3> >(data,SIZE, myTypeName + " and_remainder 3");
    test_constant<T, and_remainder2<T, 3> >(data,SIZE, myTypeName + " and_remainder2 3");
    test_constant<T, custom_remainder_inline4<T> >(data,SIZE, myTypeName + " remainder by inline 4");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 4, myTypeName + " remainder by variable 4");
    test_constant<T, custom_remainder<T, 4> >(data,SIZE, myTypeName + " remainder by constant 4");
    
    test_constant<T, and_constant<T, 7> >(data,SIZE, myTypeName + " and 7");
    test_constant<T, and_remainder<T, 7> >(data,SIZE, myTypeName + " and_remainder 7");
    test_constant<T, and_remainder2<T, 7> >(data,SIZE, myTypeName + " and_remainder2 7");
    test_constant<T, custom_remainder_inline8<T> >(data,SIZE, myTypeName + " remainder by inline 8");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 8, myTypeName + " remainder by variable 8");
    test_constant<T, custom_remainder<T, 8> >(data,SIZE, myTypeName + " remainder by constant 8");
    
    test_constant<T, and_constant<T, 15> >(data,SIZE, myTypeName + " and 15");
    test_constant<T, and_remainder<T, 15> >(data,SIZE, myTypeName + " and_remainder 15");
    test_constant<T, and_remainder2<T, 15> >(data,SIZE, myTypeName + " and_remainder2 15");
    test_constant<T, custom_remainder_inline16<T> >(data,SIZE, myTypeName + " remainder by inline 16");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 16, myTypeName + " remainder by variable 16");
    test_constant<T, custom_remainder<T, 16> >(data,SIZE, myTypeName + " remainder by constant 16");
    
    test_constant<T, and_constant<T, 31> >(data,SIZE, myTypeName + " and 31");
    test_constant<T, and_remainder<T, 31> >(data,SIZE, myTypeName + " and_remainder 31");
    test_constant<T, and_remainder2<T, 31> >(data,SIZE, myTypeName + " and_remainder2 31");
    test_constant<T, custom_remainder_inline32<T> >(data,SIZE, myTypeName + " remainder by inline 32");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 32, myTypeName + " remainder by variable 32");
    test_constant<T, custom_remainder<T, 32> >(data,SIZE, myTypeName + " remainder by constant 32");
    
    test_constant<T, and_constant<T, 63> >(data,SIZE, myTypeName + " and 63");
    test_constant<T, and_remainder<T, 63> >(data,SIZE, myTypeName + " and_remainder 63");
    test_constant<T, and_remainder2<T, 63> >(data,SIZE, myTypeName + " and_remainder2 63");
    test_constant<T, custom_remainder_inline64<T> >(data,SIZE, myTypeName + " remainder by inline 64");
    test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 64, myTypeName + " remainder by variable 64");
    test_constant<T, custom_remainder<T, 64> >(data,SIZE, myTypeName + " remainder by constant 64");
    
    
    if (sizeof(T) > 1) {
    
        test_constant<T, and_constant<T, 127> >(data,SIZE, myTypeName + " and 127");
        test_constant<T, and_remainder<T, 127> >(data,SIZE, myTypeName + " and_remainder 127");
        test_constant<T, and_remainder2<T, 127> >(data,SIZE, myTypeName + " and_remainder2 127");
        test_constant<T, custom_remainder_inline128<T> >(data,SIZE, myTypeName + " remainder by inline 128");
        test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 128, myTypeName + " remainder by variable 128");
        test_constant<T, custom_remainder<T, 128> >(data,SIZE, myTypeName + " remainder by constant 128");
    
        test_constant<T, and_constant<T, 255> >(data,SIZE, myTypeName + " and 255");
        test_constant<T, and_remainder<T, 255> >(data,SIZE, myTypeName + " and_remainder 255");
        test_constant<T, and_remainder2<T, 255> >(data,SIZE, myTypeName + " and_remainder2 255");
        test_constant<T, custom_remainder_inline256<T> >(data,SIZE, myTypeName + " remainder by inline 256");
        test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 256, myTypeName + " remainder by variable 256");
        test_constant<T, custom_remainder<T, 256> >(data,SIZE, myTypeName + " remainder by constant 256");
        
        test_constant<T, and_constant<T, 1023> >(data,SIZE, myTypeName + " and 1023");
        test_constant<T, and_remainder<T, 1023> >(data,SIZE, myTypeName + " and_remainder 1023");
        test_constant<T, and_remainder2<T, 1023> >(data,SIZE, myTypeName + " and_remainder2 1023");
        test_constant<T, custom_remainder_inline1024<T> >(data,SIZE, myTypeName + " remainder by inline 1024");
        test_variable1<T, custom_remainder_variable<T> >(data,SIZE, 1024, myTypeName + " remainder by variable 1024");
        test_constant<T, custom_remainder<T, 1024> >(data,SIZE, myTypeName + " remainder by constant 1024");
    }

    std::string temp3( myTypeName + " strength reduction remainder");
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    for (int i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);


    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();

    iterations /= 4;
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    TestOneType<int64_t>();
    TestOneType<uint64_t>();


    // no float strength reduction yet
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
