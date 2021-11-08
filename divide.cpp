/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2021 Chris Cox
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
 
    6) The compiler really should use remainder code from
            "Faster Remainder by Direct Computation: Applications to Compilers and Software Libraries"
            by Daniel Lemire, Owen Kaser, and Nathan Kurz



NOTE: At present, no good optimizations for floating point constant divides or modulus are known.
    Some vector algorithms for division do exist, but they are not expressible directly in C,
    and are processor family specific.


TODO -
Stephan T. Lavavej 2019-03-20 15:43:09 PDT
New algorithms can improve the performance of remainder and divisibility. See "Faster remainders when the divisor is a constant: beating compilers and libdivide" at https://lemire.me/blog/2019/02/08/faster-remainders-when-the-divisor-is-a-constant-beating-compilers-and-libdivide/
and the paper "Faster Remainder by Direct Computation: Applications to Compilers and Software Libraries" by Daniel Lemire, Owen Kaser, and Nathan Kurz at https://arxiv.org/pdf/1902.01961.pdf .

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
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"


/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2000000;
//int iterations = 20000;


// 4000 items, or between 4k and 32k of data
#define SIZE     4000


// initial value for filling our arrays, may be changed from the command line
double init_value = 16000.0;

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

#define MULSH(x,y)    (int32_t)( ((x) * (int64_t)(y)) >> 32 )
#define MULUH(x,y)    (uint32_t)( ((x) * (uint64_t)(y)) >> 32 )

// Multiply 2 x 32 bit numbers into a 64 bit intermediate and return low 32 bits of result,
// These may be handled by special instructions on some CPUs.

#define MULSL(x,y)    (int32_t)( (x) * (int64_t)(y) )
#define MULUL(x,y)    (uint32_t)( (x) * (uint64_t)(y) )

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

#if 0

uint32_t d = 360; // your divisor > 0

uint64_t c = (uint64_t(0xFFFFFFFFFFFFFFFFULL) / d) + 1;

// fastmod computes (n mod d) given precomputed c
uint32_t fastmod(uint32_t n) {
  uint64_t lowbits = c * n;
  return ((__uint128_t)lowbits * d) >> 64;
}

// given precomputed c, checks whether n % d == 0
bool is_divisible(uint32_t n) {
  return (n * c) <= (c - 1);
}

#endif

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

template <typename T, typename Shifter>
inline void check_shifted_sum(T result, const std::string label) {
    T temp = (T)SIZE * Shifter::do_shift((T)init_value);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_variable_sum(T result, T var, const std::string &label) {
    T temp = (T)SIZE * Shifter::do_shift((T)init_value, var);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T>
void test_variable_divide_unsigned(T* first, int count, T v1, const std::string label) {
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
   check_shifted_variable_sum<T, custom_divide_variable<T> >(result, v1, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_variable_divide_signed(T* first, int count, T v1, const std::string label) {
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
   check_shifted_variable_sum<T, custom_divide_variable<T> >(result, v1, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_variable_modulo_unsigned(T* first, int count, T v1, const std::string label) {
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
   check_shifted_variable_sum<T, custom_modulo_variable<T> >(result, v1, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_variable_modulo_signed(T* first, int count, T v1, const std::string label) {
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
   check_shifted_variable_sum<T, custom_modulo_variable<T> >(result, v1, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_variable1(T* first, int count, T v1, const std::string label) {
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n], v1 );
    }
    check_shifted_variable_sum<T, Shifter>(result, v1, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_constant(T* first, int count, const std::string label) {
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n] );
    }
    check_shifted_sum<T, Shifter>(result, label);
  }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void TestOneType(double temp)
{
    std::string myTypeName( getTypeName<T>() );
    bool typeIsSigned = isSigned<T>();

    T data[SIZE];

    ::fill(data, data+SIZE, T(init_value));
    T var (temp);


    test_variable1<T, custom_divide_variable<T> >(data,SIZE,var,myTypeName + " variable divide");
    
    if (typeIsSigned)
        test_variable_divide_signed(data,SIZE,var,myTypeName + " variable reciprocal divide");
    else
        test_variable_divide_unsigned(data,SIZE,var,myTypeName + " variable reciprocal divide");
    
    test_constant<T, custom_divide_360<T> >(data,SIZE,myTypeName + " constant divide");
    
    if (typeIsSigned)
        test_constant<T, custom_recip_signed360<T> >(data,SIZE,myTypeName + " constant reciprocal divide");
    else
        test_constant<T, custom_recip_unsigned360<T> >(data,SIZE,myTypeName + " constant reciprocal divide");
    
    test_variable1<T, custom_modulo_variable<T> >(data,SIZE,var,myTypeName + " variable modulo");
    
    if (typeIsSigned)
        test_variable_modulo_signed(data,SIZE,var,myTypeName + " variable reciprocal modulo");
    else
        test_variable_modulo_unsigned(data,SIZE,var,myTypeName + " variable reciprocal modulo");
    
    test_constant<T, custom_modulo_360<T> >(data,SIZE,myTypeName + " constant modulo");
    
    if (typeIsSigned)
        test_constant<T, custom_recip_modulo_signed360<T> >(data,SIZE,myTypeName + " constant reciprocal modulo");
    else
        test_constant<T, custom_recip_modulo_unsigned360<T> >(data,SIZE,myTypeName + " constant reciprocal modulo");
    
    test_variable1<T, custom_divplusmod_variable<T> >(data,SIZE,var,myTypeName + " variable div plus mod");
    
    if (typeIsSigned)
        test_constant<T, custom_recip_divplusmod_signed360<T> >(data,SIZE,myTypeName + " constant reciprocal div plus mod");
    else
        test_constant<T, custom_recip_divplusmod_unsigned360<T> >(data,SIZE,myTypeName + " constant reciprocal div plus mod");
    
    test_constant<T, custom_divplusmod_360<T> >(data,SIZE,myTypeName + " constant div plus mod");


    std::string temp1 = myTypeName + " division";
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    gLabels.clear();
}

/******************************************************************************/

template <typename T>
void TestOneTypeFloat(double temp)
{
    std::string myTypeName( getTypeName<T>() );

    T data[SIZE];

    ::fill(data, data+SIZE, T(init_value));
    T var (temp);


    test_variable1<T, custom_divide_variable<T> >(data,SIZE,var,myTypeName + " variable divide");
    test_constant<T, custom_divide_360<T> >(data,SIZE,myTypeName + " constant divide");
    
    
    std::string temp1 = myTypeName + " division";
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    gLabels.clear();
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


// TODO - unit test custom code!


    TestOneType<int16_t>( temp );
    TestOneType<uint16_t>( temp );
    TestOneType<int32_t>( temp );
    TestOneType<uint32_t>( temp );
    TestOneType<int64_t>( temp );
    TestOneType<uint64_t>( temp );
    TestOneTypeFloat<float>( temp );
    TestOneTypeFloat<double>( temp );
    TestOneTypeFloat<long double>( temp );

            
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
