/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test performance of various absolute value idioms.

Assumptions:

    1) The compiler will optimize absolute value operations.
    
    2) The compiler may recognize ineffecient absolute value idioms
        and substitute efficient methods.



NOTE - Seeing poor vector code generation, not using conditional moves well.
    But scalar code uses conditional moves instead of branches.
    64 bit int is not vectorized in most cases.

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 3700000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = -3;

/******************************************************************************/

// our global arrays of numbers to be summed

int8_t data8[SIZE];
int16_t data16[SIZE];
int32_t data32[SIZE];
int64_t data64[SIZE];
float dataFloat[SIZE];
double dataDouble[SIZE];

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct fabs_functor {
    static inline T do_shift(T input)
        {
        return T( fabs( input ) );
        }
};

/******************************************************************************/

template <typename T>
struct fabsf_functor {
    static inline T do_shift(T input)
        {
        return T( fabsf( input ) );
        }
};

/******************************************************************************/

template <typename T>
struct abs_functor_std {
    static inline T do_shift(T input)
        {
        return T( std::abs( input ) );
        }
};

/******************************************************************************/

template <typename T>
struct abs_functor1 {
    static inline T do_shift(T value)
        {
        if (value < T(0))
            return -value;
        else
            return value;
        }
};

/******************************************************************************/

// Some compilers optimize this comparison better than the reverse
// especially on floating point values
// and some do just the reverse, optimizing it very poorly
template <typename T>
struct abs_functor8 {
    static inline T do_shift(T value)
        {
        if (value >= T(0))
            return value;
        else
            return -value;
        }
};

/******************************************************************************/

template <typename T>
struct abs_functor2 {
    static inline T do_shift(T value)
        {
        return ( (value < T(0) ) ? -value : value );
        }
};

/******************************************************************************/

// Some compilers optimize this comparison better than the reverse
// especially on floating point values
// and some do just the reverse, optimizing it very poorly
template <typename T>
struct abs_functor9 {
    static inline T do_shift(T value)
        {
        return ( (value >= T(0) ) ? value : -value );
        }
};

/******************************************************************************/

// this will only work for types where signed right shift and xor are defined (signed integers)
template <typename T>
struct abs_functor3 {
    static inline T do_shift(T value)
        {
        T mask = value >> ( 8*sizeof(T) - 1 );
        return ( (value + mask) ^ mask );
        }
};

/******************************************************************************/

// this will only work for types where signed right shift and xor are defined (signed integers)
template <typename T>
struct abs_functor4 {
    static inline T do_shift(T value)
        {
        T mask = value >> ( 8*sizeof(T) - 1 );
        return ( (value ^ mask) - mask );
        }
};

/******************************************************************************/

// this will only work for types where signed right shift and xor are defined (signed integers)
// Seems like a silly way to do it, but I found this in real world code.
template <typename T>
struct abs_functor5 {
    static inline T do_shift(T value)
        {
        T mask = value >> ( 8*sizeof(T) - 1 );
        return ( (value ^ mask) + (mask&1) );
        }
};

/******************************************************************************/

// this will only work for IEEE 754 floating point types
struct abs_functor6 {
    static inline float do_shift(float value)
        {
        uint32_t temp = *(reinterpret_cast<uint32_t*>(&value));
        const uint32_t mask = 0x7FFFFFFFL;
        temp &= mask;
        return *(reinterpret_cast<float*>(&temp));
        }
    
    static inline double do_shift(double value)
        {
        uint64_t temp = *(reinterpret_cast<uint64_t*>(&value));
        const uint64_t mask = 0x7FFFFFFFFFFFFFFFLL;
        temp &= mask;
        return *(reinterpret_cast<double*>(&temp));
        }
};

/******************************************************************************/

// this will only work for IEEE 754 floating point types
struct abs_functor7 {
    static inline float do_shift(float value)
        {
        if (value > 0.0f)
            return value;
        uint32_t temp = *(reinterpret_cast<uint32_t*>(&value));
        const uint32_t mask = 0x7FFFFFFFL;
        temp &= mask;
        return *(reinterpret_cast<float*>(&temp));
        }
    
    static inline double do_shift(double value)
        {
        if (value > 0.0)
            return value;
        uint64_t temp = *(reinterpret_cast<uint64_t*>(&value));
        const uint64_t mask = 0x7FFFFFFFFFFFFFFFLL;
        temp &= mask;
        return *(reinterpret_cast<double*>(&temp));
        }
};

/******************************************************************************/
/******************************************************************************/

template <typename Iterator, typename T>
void fill_PosNeg(Iterator first, size_t count, T value) {
    size_t i;
    
    for (i=0; i < (count-1); i+=2) {
        first[i+0] = value;
        first[i+1] = -value;
    }
    
    for (; i < count; ++i)
        first[i] = value;
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Shifter>
void validate_abs_value(const char *label, T value) {
    if (value < 0)    value = -value;    // make sure we start with a positive value
    T result0 = Shifter::do_shift( -value );
    T result1 = Shifter::do_shift( value );
    if (result0 != value || result1 != value)
        printf("%s failed to validate\n", label);
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_abs(const char *label) {
    validate_abs_value<T,Shifter>( label, 1 );
    validate_abs_value<T,Shifter>( label, 2 );
    validate_abs_value<T,Shifter>( label, 4 );
    validate_abs_value<T,Shifter>( label, 7 );
    validate_abs_value<T,Shifter>( label, 100 );
    validate_abs_value<T,Shifter>( label, 125 );
    validate_abs_value<T,Shifter>( label, 126 );
    validate_abs_value<T,Shifter>( label, 127 );
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
    
    
    // validate functions
    validate_abs<int8_t,abs_functor_std<int8_t> >("std abs int8");
    validate_abs<int8_t,abs_functor1<int8_t> >("abs1 int8");
    validate_abs<int8_t,abs_functor2<int8_t> >("abs2 int8");
    validate_abs<int8_t,abs_functor3<int8_t> >("abs3 int8");
    validate_abs<int8_t,abs_functor4<int8_t> >("abs4 int8");
    validate_abs<int8_t,abs_functor5<int8_t> >("abs5 int8");
    validate_abs<int8_t,abs_functor8<int8_t> >("abs8 int8");
    validate_abs<int8_t,abs_functor9<int8_t> >("abs9 int8");
    validate_abs<int16_t,abs_functor_std<int16_t> >("std abs int16");
    validate_abs<int16_t,abs_functor1<int16_t> >("abs1 int16");
    validate_abs<int16_t,abs_functor2<int16_t> >("abs2 int16");
    validate_abs<int16_t,abs_functor3<int16_t> >("abs3 int16");
    validate_abs<int16_t,abs_functor4<int16_t> >("abs4 int16");
    validate_abs<int16_t,abs_functor5<int16_t> >("abs5 int16");
    validate_abs<int16_t,abs_functor8<int16_t> >("abs8 int16");
    validate_abs<int16_t,abs_functor9<int16_t> >("abs9 int16");
    validate_abs<int32_t,abs_functor_std<int32_t> >("std abs int32");
    validate_abs<int32_t,abs_functor1<int32_t> >("abs1 int32");
    validate_abs<int32_t,abs_functor2<int32_t> >("abs2 int32");
    validate_abs<int32_t,abs_functor3<int32_t> >("abs3 int32");
    validate_abs<int32_t,abs_functor4<int32_t> >("abs4 int32");
    validate_abs<int32_t,abs_functor5<int32_t> >("abs5 int32");
    validate_abs<int32_t,abs_functor8<int32_t> >("abs8 int32");
    validate_abs<int32_t,abs_functor9<int32_t> >("abs9 int32");
    validate_abs<int64_t,abs_functor_std<int64_t> >("std abs int64");
    validate_abs<int64_t,abs_functor1<int64_t> >("abs1 int64");
    validate_abs<int64_t,abs_functor2<int64_t> >("abs2 int64");
    validate_abs<int64_t,abs_functor3<int64_t> >("abs3 int64");
    validate_abs<int64_t,abs_functor4<int64_t> >("abs4 int64");
    validate_abs<int64_t,abs_functor5<int64_t> >("abs5 int64");
    validate_abs<int64_t,abs_functor8<int64_t> >("abs8 int64");
    validate_abs<int64_t,abs_functor9<int64_t> >("abs9 int64");
    validate_abs<float,abs_functor_std<float> >("std abs float");
    validate_abs<float, fabs_functor<float> >("fabs float");
    validate_abs<float, fabsf_functor<float> >("fabsf float");
    validate_abs<float,abs_functor1<float> >("abs1 float");
    validate_abs<float,abs_functor2<float> >("abs2 float");
    validate_abs<float, abs_functor6 >("abs6 float");
    validate_abs<float, abs_functor7 >("abs7 float");
    validate_abs<float,abs_functor8<float> >("abs8 float");
    validate_abs<float,abs_functor9<float> >("abs9 float");
    validate_abs<double,abs_functor_std<double> >("std abs dbl");
    validate_abs<double, fabs_functor<double> >("fabs dbl");
    validate_abs<double,abs_functor1<double> >("abs1 dbl");
    validate_abs<double,abs_functor2<double> >("abs2 dbl");
    validate_abs<double, abs_functor6 >("abs6 dbl");
    validate_abs<double, abs_functor7 >("abs7 dbl");
    validate_abs<double,abs_functor8<double> >("abs8 dbl");
    validate_abs<double,abs_functor9<double> >("abs9 dbl");



    fill_PosNeg( data8, SIZE, int8_t(init_value) );
    test_constant<int8_t, abs_functor_std<int8_t> >(data8,SIZE,"int8_t std abs");
    test_constant<int8_t, abs_functor1<int8_t> >(data8,SIZE,"int8_t abs1");
    test_constant<int8_t, abs_functor2<int8_t> >(data8,SIZE,"int8_t abs2");
    test_constant<int8_t, abs_functor3<int8_t> >(data8,SIZE,"int8_t abs3");
    test_constant<int8_t, abs_functor4<int8_t> >(data8,SIZE,"int8_t abs4");
    test_constant<int8_t, abs_functor5<int8_t> >(data8,SIZE,"int8_t abs5");
    test_constant<int8_t, abs_functor8<int8_t> >(data8,SIZE,"int8_t abs8");
    test_constant<int8_t, abs_functor9<int8_t> >(data8,SIZE,"int8_t abs9");

    summarize("int8_t absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    fill_PosNeg( data16, SIZE, int16_t(init_value) );
    test_constant<int16_t, abs_functor_std<int16_t> >(data16,SIZE,"int16_t std abs");
    test_constant<int16_t, abs_functor1<int16_t> >(data16,SIZE,"int16_t abs1");
    test_constant<int16_t, abs_functor2<int16_t> >(data16,SIZE,"int16_t abs2");
    test_constant<int16_t, abs_functor3<int16_t> >(data16,SIZE,"int16_t abs3");
    test_constant<int16_t, abs_functor4<int16_t> >(data16,SIZE,"int16_t abs4");
    test_constant<int16_t, abs_functor5<int16_t> >(data16,SIZE,"int16_t abs5");
    test_constant<int16_t, abs_functor8<int16_t> >(data16,SIZE,"int16_t abs8");
    test_constant<int16_t, abs_functor9<int16_t> >(data16,SIZE,"int16_t abs9");

    summarize("int16_t absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    fill_PosNeg( data32, SIZE, int32_t(init_value) );
    test_constant<int32_t, abs_functor_std<int32_t> >(data32,SIZE,"int32_t std abs");
    test_constant<int32_t, abs_functor1<int32_t> >(data32,SIZE,"int32_t abs1");
    test_constant<int32_t, abs_functor2<int32_t> >(data32,SIZE,"int32_t abs2");
    test_constant<int32_t, abs_functor3<int32_t> >(data32,SIZE,"int32_t abs3");
    test_constant<int32_t, abs_functor4<int32_t> >(data32,SIZE,"int32_t abs4");
    test_constant<int32_t, abs_functor5<int32_t> >(data32,SIZE,"int32_t abs5");
    test_constant<int32_t, abs_functor8<int32_t> >(data32,SIZE,"int32_t abs8");
    test_constant<int32_t, abs_functor9<int32_t> >(data32,SIZE,"int32_t abs9");

    summarize("int32_t absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    fill_PosNeg( data64, SIZE, int64_t(init_value) );
    test_constant<int64_t, abs_functor_std<int64_t> >(data64,SIZE,"int64_t std abs");
    test_constant<int64_t, abs_functor1<int64_t> >(data64,SIZE,"int64_t abs1");
    test_constant<int64_t, abs_functor2<int64_t> >(data64,SIZE,"int64_t abs2");
    test_constant<int64_t, abs_functor3<int64_t> >(data64,SIZE,"int64_t abs3");
    test_constant<int64_t, abs_functor4<int64_t> >(data64,SIZE,"int64_t abs4");
    test_constant<int64_t, abs_functor5<int64_t> >(data64,SIZE,"int64_t abs5");
    test_constant<int64_t, abs_functor8<int64_t> >(data64,SIZE,"int64_t abs8");
    test_constant<int64_t, abs_functor9<int64_t> >(data64,SIZE,"int64_t abs9");

    summarize("int64_t absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    fill_PosNeg( dataFloat, SIZE, float(init_value) );
    test_constant<float, fabs_functor<float> >(dataFloat,SIZE,"float fabs");
    test_constant<float, fabsf_functor<float> >(dataFloat,SIZE,"float fabsf");
    test_constant<float, abs_functor_std<float> >(dataFloat,SIZE,"float std abs");
    test_constant<float, abs_functor1<float> >(dataFloat,SIZE,"float abs1");
    test_constant<float, abs_functor2<float> >(dataFloat,SIZE,"float abs2");
    test_constant<float, abs_functor6 >(dataFloat,SIZE,"float abs6");
    test_constant<float, abs_functor7 >(dataFloat,SIZE,"float abs7");
    test_constant<float, abs_functor8<double> >(dataFloat,SIZE,"float abs8");
    test_constant<float, abs_functor9<double> >(dataFloat,SIZE,"float abs9");

    summarize("float absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    fill_PosNeg( dataDouble, SIZE, double(init_value) );
    test_constant<double, fabs_functor<double> >(dataDouble,SIZE,"double fabs");
    test_constant<double, abs_functor_std<double> >(dataDouble,SIZE,"double std abs");
    test_constant<double, abs_functor1<double> >(dataDouble,SIZE,"double abs1");
    test_constant<double, abs_functor2<double> >(dataDouble,SIZE,"double abs2");
    test_constant<double, abs_functor6 >(dataDouble,SIZE,"double abs6");
    test_constant<double, abs_functor7 >(dataDouble,SIZE,"double abs7");
    test_constant<double, abs_functor8<double> >(dataDouble,SIZE,"double abs8");
    test_constant<double, abs_functor9<double> >(dataDouble,SIZE,"double abs9");

    summarize("double absolute value", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
