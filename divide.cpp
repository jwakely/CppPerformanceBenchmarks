/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to division and modulo calculations.

Assumptions:

    1) The compiler will change integer division or modulo by constant to an exact reciprocal multiply.
        Division by a constant should be faster than division by a variable.
        Modulo by a constant should be faster than modulo by a variable.
    
    3) The integer reciprocal code used by the compiler will be the same speed or faster than the brute force code
         from  "Division by Invariant Integers using Multiplication"
                T. Granlund and P.L. Montgomery
                SIGPLAN Notices Vol 29, Number 6 (June 1994), page 61

         and   "Improved division by invariant integers"
                Niels Moller and Torbjorn Granlund
                IEEE Transactions on Computers ( Volume: 60, Issue: 2, Feb. 2011 )

    4) The compiler will correctly remove common subexpressions for combined divide and modulo by constant.

    5) The compiler will recognize integer division or modulo by a loop invariant value and substitute a reciprocal multiply.
            Do any compilers do this correctly yet?



NOTE: At present, no good optimizations for floating point constant divides or modulus are known.
    Some vector algorithms for division do exist, but they are not expressible directly in C,
    and are processor family specific.

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2000000;


// 4000 items, or between 4k and 32k of data
#define SIZE     4000


// initial value for filling our arrays, may be changed from the command line
double init_value = 16000.0;

/******************************************************************************/

// our global arrays of numbers to be operated upon

double dataDouble[SIZE];
float dataFloat[SIZE];

uint64_t data64unsigned[SIZE];
int64_t data64[SIZE];

uint32_t data32unsigned[SIZE];
int32_t data32[SIZE];

uint16_t data16unsigned[SIZE];
int16_t data16[SIZE];

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

inline int32_t Abs32( int32_t x ) {
    return (x < 0) ? (-x) : x;
}

inline
int32_t Min32(int32_t a, int32_t b) {
    return (a < b) ? a : b;
}

inline
int32_t Max32(int32_t a, int32_t b) {
    return (a > b) ? a : b;
}

/******************************************************************************/

// Multiply 2 x 32 bit numbers into a 64 bit intermediate and return high 32 bits of result,
// These may be handled by special instructions on some CPUs.

#define MULSH(x,y)    (int32_t)(((x) * (int64_t)(y)) >> 32)
#define MULUH(x,y)    (uint32_t)(((x) * (uint64_t)(y)) >> 32)

/******************************************************************************/

// Count leading zeros, may be handled by special instructions on some CPUs.
inline int32_t CNTLZW( unsigned long x ) {
    if (x == 0)
        return 32;
    uint32_t count = 0;
    uint32_t mask = 0x80000000;
    while ( !(x & mask) ) {
        count ++;
        mask >>= 1;
    }
    return count;
}

/******************************************************************************/

#define DECLARE_RECIP_SIGNED    \
        uint32_t postshift_signed; \
        int32_t mhigh_signed, dsign_signed;

#define DECLARE_RECIP_UNSIGNED    \
        uint32_t mhigh_unsigned, preshift_unsigned, postshift_unsigned;

#define SetupUnsignedRecip(d)       ChooseGeneralUnsignedMultiplier( (d), mhigh_unsigned, preshift_unsigned, postshift_unsigned )
#define SetupSignedRecip(d)         ChooseGeneralSignedMultiplier( (d), mhigh_signed, postshift_signed, dsign_signed )
#define DoUnsignedRecip(n)          DoGeneralUnsignedRecip( (n), mhigh_unsigned, preshift_unsigned, postshift_unsigned )
#define DoSignedRecip(n)            DoGeneralSignedRecip( (n), mhigh_signed, postshift_signed, dsign_signed )
#define DoSignedUnsignedRecip(n)    DoGeneralSignedRecip_UnsignedDenominator( (n), mhigh_signed, postshift_signed )
#define DoUnsignedModulus(n,d)      DoGeneralUnsignedModulus( (n), (d), mhigh_unsigned, preshift_unsigned, postshift_unsigned )
#define DoSignedModulus(n,d)        DoGeneralSignedModulus( (n), (d), mhigh_signed, postshift_signed, dsign_signed )

/******************************************************************************/

void ChooseGeneralUnsignedMultiplier( const uint32_t d, uint32_t &mhigh, uint32_t &preshift, uint32_t &postshift ) {
    const uint32_t    N = sizeof(uint32_t) * 8;        // bits in a word, also max size of input!

    uint32_t l = N - CNTLZW(d - 1);    // log2(d) rounded up

    if (d == (0x01 << l)) {
        preshift = 0;    // if preshift is zero, evals to (j >> postshift)
        postshift = l;    // this will be zero for d==1
        mhigh = 0;        // simplify the multiply (if it's done at all)
    } else {
        preshift = Min32( l , 1 );
        postshift = Max32( (int)l - 1, 0 );
#if 1
// faster only if 64 bit division is natively supported
        int64_t dhigh = 1 + ( ( (int64_t) 1 << (N+l) ) / d ) - ((int64_t)1 << N);
#else
        double dhigh = 1.0 + floor( pow((double)2.0,(int)(N+l)) / (double)d );
        dhigh -= pow((double)2.0,(int)(N));
#endif
        mhigh = dhigh;
    }
}

inline uint32_t DoGeneralUnsignedRecip( uint32_t ul, uint32_t mhigh, uint32_t preshift, uint32_t postshift ) {
    uint32_t temp = MULUH(mhigh, ul);
    return ((temp + ((ul - temp) >> preshift)) >> postshift);
}

inline uint32_t DoGeneralUnsignedModulus( uint32_t ul, uint32_t d, uint32_t mhigh, uint32_t preshift, uint32_t postshift ) {
    uint32_t temp = MULUH(mhigh, ul);
    uint32_t temp2 = ((temp + ((ul - temp) >> preshift)) >> postshift);
    return  ul - (d * temp2);
}

/******************************************************************************/

void ChooseGeneralSignedMultiplier( const int32_t d, int32_t &mhigh, uint32_t &postshift, int32_t &dsign ) {
    const uint32_t    N = sizeof(uint32_t) * 8;        // bits in a word, also max size of input!
    const uint32_t    dabs = Abs32(d);

    dsign = (d >> 31);
    uint32_t l = N - CNTLZW(dabs - 1);    // log2(d) rounded up

    l = Max32( l, 1 );
    postshift = l - 1;
#if 1
// faster only if 64 bit division is natively supported
    int64_t dhigh = 1 + ( ( (int64_t)1 << (N+l-1) ) / dabs ) - ((int64_t)1 << N);
#else
    double dhigh = 1.0 + floor( pow((double)2.0,(int)(N+l-1)) / (double)dabs );
    dhigh -= pow((double)2.0,(int)(N));
#endif
    mhigh = dhigh;
}

inline int32_t DoGeneralSignedRecip( int32_t sl, int32_t mhigh, uint32_t postshift, int32_t dsign ) {
    int32_t temp = sl + MULSH(mhigh, sl);
    temp = (temp >> postshift) - (sl >> 31);
    return ((temp ^ dsign) - dsign);    // if using signed denominator
}

inline int32_t DoGeneralSignedModulus( int32_t sl, int32_t d, int32_t mhigh, uint32_t postshift, int32_t dsign ) {
    int32_t temp = sl + MULSH(mhigh, sl);
    temp = (temp >> postshift) - (sl >> 31);
    int32_t temp2 = ((temp ^ dsign) - dsign);
    return sl - (d * temp2);
}

/******************************************************************************/

inline int32_t SDiv360( int32_t n ) {
    const int32_t mhigh = 0xB60B60B7;
    const uint32_t postshift = 8;
    return ((int32_t)((int32_t)n + MULSH(mhigh,(int32_t)n)) >> postshift) - ((int32_t)n >> 31);
}

inline uint32_t UDiv360( uint32_t n ) {
    const uint32_t mhigh = 0x16C16C17;
    const uint32_t preshift = 3;
    const uint32_t postshift = 2;
    return (MULUH(mhigh,((uint32_t)n >> preshift)) >> postshift);
}

inline uint32_t UMod360( uint32_t n ) {
    return n - 360 * UDiv360(n);
}

inline int32_t SMod360( int32_t n ) {
    return n - 360 * SDiv360(n);
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct custom_divide_360 {
      static T do_shift(T input) { return (input / 360); }
    };

template <typename T>
    struct custom_recip_signed360 {
      static T do_shift(T input) { return SDiv360(input); }
    };

template <typename T>
    struct custom_recip_unsigned360 {
      static T do_shift(T input) { return UDiv360(input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_modulo_variable {
      static T do_shift(T input, T v1) { return (input % v1); }
    };

template <typename T>
    struct custom_modulo_360 {
      static T do_shift(T input) { return (input % 360); }
    };

template <typename T>
    struct custom_recip_modulo_signed360 {
      static T do_shift(T input) { return SMod360(input); }
    };

template <typename T>
    struct custom_recip_modulo_unsigned360 {
      static T do_shift(T input) { return UMod360(input); }
    };

/******************************************************************************/

template <typename T>
    struct custom_divplusmod_variable {
      static T do_shift(T input, T v1) { return ((input / v1) + (input % v1)); }
    };

template <typename T>
    struct custom_divplusmod_360 {
      static T do_shift(T input) { return ((input / 360) + (input % 360)); }
    };

template <typename T>
    struct custom_recip_divplusmod_signed360 {
      static T do_shift(T input) { return (SDiv360(input) + SMod360(input)); }
    };

template <typename T>
    struct custom_recip_divplusmod_unsigned360 {
      static T do_shift(T input) { return (UDiv360(input) + UMod360(input)); }
    };

/******************************************************************************/

template <typename T>
void test_variable_divide_unsigned(T* first, int count, T v1, const char *label) {
  int i;
  DECLARE_RECIP_UNSIGNED;
  
  start_timer();
  
  // this is legal because we are dividing by the same value in all loop iterations
  // loop invariant code motion should have brought the calculation out to this point
  SetupUnsignedRecip( v1 );
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += DoUnsignedRecip( first[n] );
    }
   check_shifted_variable_sum<T, custom_divide_variable<T> >(result, v1);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

template <typename T>
void test_variable_divide_signed(T* first, int count, T v1, const char *label) {
  int i;
  DECLARE_RECIP_SIGNED;
  
  start_timer();
  
  // this is legal because we are dividing by the same value in all loop iterations
  // loop invariant motion should have brought the calculation out to this point
  SetupSignedRecip( v1 );
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += DoSignedRecip( first[n] );
    }
   check_shifted_variable_sum<T, custom_divide_variable<T> >(result, v1);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

template <typename T>
void test_variable_modulo_unsigned(T* first, int count, T v1, const char *label) {
  int i;
  DECLARE_RECIP_UNSIGNED;
  
  start_timer();
  
  // this is legal because we are dividing by the same value in all loop iterations
  // loop invariant motion should have brought the calculation out to this point
  SetupUnsignedRecip( v1 );
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += DoUnsignedModulus( first[n], v1 );
    }
   check_shifted_variable_sum<T, custom_modulo_variable<T> >(result, v1);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

template <typename T>
void test_variable_modulo_signed(T* first, int count, T v1, const char *label) {
  int i;
  DECLARE_RECIP_SIGNED;
  
  start_timer();
  
  // this is legal because we are dividing by the same value in all loop iterations
  // loop invariant motion should have brought the calculation out to this point
  SetupSignedRecip( v1 );
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += DoSignedModulus( first[n], v1 );
    }
   check_shifted_variable_sum<T, custom_modulo_variable<T> >(result, v1);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/


int main(int argc, char** argv) {
    double temp = 360.0;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) temp = (double)atof(argv[3]);



// int16_t
    ::fill(data16, data16+SIZE, int16_t(init_value));
    int16_t var1int16_t_1(temp);

    test_variable1<int16_t, custom_divide_variable<int16_t> >(data16,SIZE,var1int16_t_1,"int16_t variable divide");
    test_constant<int16_t, custom_divide_360<int16_t> >(data16,SIZE,"int16_t constant divide");
    test_variable1<int16_t, custom_modulo_variable<int16_t> >(data16,SIZE,var1int16_t_1,"int16_t variable modulo");
    test_constant<int16_t, custom_modulo_360<int16_t> >(data16,SIZE,"int16_t constant modulo");
    test_variable1<int16_t, custom_divplusmod_variable<int16_t> >(data16,SIZE,var1int16_t_1,"int16_t variable div plus mod");
    test_constant<int16_t, custom_divplusmod_360<int16_t> >(data16,SIZE,"int16_t constant div plus mod");

    summarize("int16_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// uint16_t
    ::fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    uint16_t var1uint16_t_1(temp);

    test_variable1<uint16_t, custom_divide_variable<uint16_t> >(data16unsigned,SIZE,var1uint16_t_1,"uint16_t variable divide");
    test_constant<uint16_t, custom_divide_360<uint16_t> >(data16unsigned,SIZE,"uint16_t constant divide");
    test_variable1<uint16_t, custom_modulo_variable<uint16_t> >(data16unsigned,SIZE,var1uint16_t_1,"uint16_t variable modulo");
    test_constant<uint16_t, custom_modulo_360<uint16_t> >(data16unsigned,SIZE,"uint16_t constant modulo");
    test_variable1<uint16_t, custom_divplusmod_variable<uint16_t> >(data16unsigned,SIZE,var1uint16_t_1,"uint16_t variable div plus mod");
    test_constant<uint16_t, custom_divplusmod_360<uint16_t> >(data16unsigned,SIZE,"uint16_t constant div plus mod");
        
    summarize("uint16_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    ::fill(data32, data32+SIZE, int32_t(init_value));
    int32_t var1int32_t_1(temp);

    test_variable1<int32_t, custom_divide_variable<int32_t> >(data32,SIZE,var1int32_t_1,"int32_t variable divide");
    test_variable_divide_signed(data32,SIZE,var1int32_t_1,"int32_t variable reciprocal divide");
    test_constant<int32_t, custom_divide_360<int32_t> >(data32,SIZE,"int32_t constant divide");
    test_constant<int32_t, custom_recip_signed360<int32_t> >(data32,SIZE,"int32_t constant reciprocal divide");
    test_variable1<int32_t, custom_modulo_variable<int32_t> >(data32,SIZE,var1int32_t_1,"int32_t variable modulo");
    test_variable_modulo_signed(data32,SIZE,var1int32_t_1,"int32_t variable reciprocal modulo");
    test_constant<int32_t, custom_modulo_360<int32_t> >(data32,SIZE,"int32_t constant modulo");
    test_constant<int32_t, custom_recip_modulo_signed360<int32_t> >(data32,SIZE,"int32_t constant reciprocal modulo");
    test_variable1<int32_t, custom_divplusmod_variable<int32_t> >(data32,SIZE,var1int32_t_1,"int32_t variable div plus mod");
    test_constant<int32_t, custom_recip_divplusmod_signed360<int32_t> >(data32,SIZE,"int32_t constant reciprocal div plus mod");
    test_constant<int32_t, custom_divplusmod_360<int32_t> >(data32,SIZE,"int32_t constant div plus mod");

    summarize("int32_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

// uint32_t
    ::fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    uint32_t var1uint32_t_1(temp);

    test_variable1<uint32_t, custom_divide_variable<uint32_t> >(data32unsigned,SIZE,var1uint32_t_1,"uint32_t variable divide");
    test_variable_divide_unsigned(data32unsigned,SIZE,var1uint32_t_1,"uint32_t variable reciprocal divide");
    test_constant<uint32_t, custom_divide_360<uint32_t> >(data32unsigned,SIZE,"uint32_t constant divide");
    test_constant<uint32_t, custom_recip_unsigned360<int32_t> >(data32unsigned,SIZE,"uint32_t constant reciprocal divide");
    test_variable1<uint32_t, custom_modulo_variable<uint32_t> >(data32unsigned,SIZE,var1uint32_t_1,"uint32_t variable modulo");
    test_variable_modulo_unsigned(data32unsigned,SIZE,var1uint32_t_1,"uint32_t variable reciprocal modulo");
    test_constant<uint32_t, custom_modulo_360<uint32_t> >(data32unsigned,SIZE,"uint32_t constant modulo");
    test_constant<uint32_t, custom_recip_modulo_unsigned360<uint32_t> >(data32unsigned,SIZE,"uint32_t constant reciprocal modulo");
    test_variable1<uint32_t, custom_divplusmod_variable<uint32_t> >(data32unsigned,SIZE,var1uint32_t_1,"uint32_t variable div plus mod");
    test_constant<uint32_t, custom_recip_divplusmod_unsigned360<uint32_t> >(data32unsigned,SIZE,"uint32_t constant reciprocal div plus mod");
    test_constant<uint32_t, custom_divplusmod_360<uint32_t> >(data32unsigned,SIZE,"uint32_t constant div plus mod");
        
    summarize("uint32_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int64_t
    ::fill(data64, data64+SIZE, int64_t(init_value));
    int64_t var1int64_t_1(temp);

    test_variable1<int64_t, custom_divide_variable<int64_t> >(data64,SIZE,var1int64_t_1,"int64_t variable divide");
    test_constant<int64_t, custom_divide_360<int64_t> >(data64,SIZE,"int64_t constant divide");
    test_variable1<int64_t, custom_modulo_variable<int64_t> >(data64,SIZE,var1int64_t_1,"int64_t variable modulo");
    test_constant<int64_t, custom_modulo_360<int64_t> >(data64,SIZE,"int64_t constant modulo");
    test_variable1<int64_t, custom_divplusmod_variable<int64_t> >(data64,SIZE,var1int64_t_1,"int64_t variable div plus mod");
    test_constant<int64_t, custom_divplusmod_360<int64_t> >(data64,SIZE,"int64_t constant div plus mod");

    summarize("int64_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// uint64_t
    ::fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    uint64_t var1uint64_t_1(temp);

    test_variable1<uint64_t, custom_divide_variable<uint64_t> >(data64unsigned,SIZE,var1uint64_t_1,"uint64_t variable divide");
    test_constant<uint64_t, custom_divide_360<uint64_t> >(data64unsigned,SIZE,"uint64_t constant divide");
    test_variable1<uint64_t, custom_modulo_variable<uint64_t> >(data64unsigned,SIZE,var1uint64_t_1,"uint64_t variable modulo");
    test_constant<uint64_t, custom_modulo_360<uint64_t> >(data64unsigned,SIZE,"uint64_t constant modulo");
    test_variable1<uint64_t, custom_divplusmod_variable<uint64_t> >(data64unsigned,SIZE,var1uint64_t_1,"uint64_t variable div plus mod");
    test_constant<uint64_t, custom_divplusmod_360<uint64_t> >(data64unsigned,SIZE,"uint64_t constant div plus mod");
        
    summarize("uint64_t division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// float
    ::fill(dataFloat, dataFloat+SIZE, float(init_value));
    float var1float(temp);

    test_variable1<float, custom_divide_variable<float> >(dataFloat,SIZE,var1float,"float variable divide");
    test_constant<float, custom_divide_360<float> >(dataFloat,SIZE,"float constant divide");
        
    summarize("float division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    ::fill(dataDouble, dataDouble+SIZE, float(init_value));
    double var1double(temp);

    test_variable1<double, custom_divide_variable<double> >(dataDouble,SIZE,var1double,"double variable divide");
    test_constant<double, custom_divide_360<double> >(dataDouble,SIZE,"double constant divide");
    
    summarize("double division", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

            
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
